#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"
#define CONNECTED 1
#define DISCONNECTED 0
#define NO_SOCKET -1

int main(int argc, char *argv[])
{	
	/* Datele de care ma voi folosi de-a lungul serverului*/
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	int sockfd1, sockfd2, newsockfd, port_number, fdmax, subs_type = 0;
	int subscribed = 0;
	int ignore = 0;
	int different_subscribed = 0;
	int position = 0;
	char buffer[256], buffer2[1551], one = 1;
	struct sockaddr_in serv_data, cli_data;
	socklen_t cli;
	fd_set rd_fds, aux_fds;	 
	struct data_cli_message payload;


	/* subscriptiile */
	vector<pair<string, int>> *subscriptions;
	/* Map pentru clientii care sunt abonati la anumite topicuri */
	map<string, vector<pair<string, int>>> map3;
	/* doua mapuri, unul de la socket id la client id
	 * iar celalalt, de la cliend id la clientul tcp
	 * Motivul? accesare mai rapida
	 */
	map<int, string> map1;
	map<string, struct data_tcp_client> map2;

	/* setamn pentru data de la tastatura*/
	FD_SET(0, &rd_fds);
	aux_fds = rd_fds;

	/* deschidem scotem pt UDP */
	sockfd1 = socket(AF_INET, SOCK_DGRAM, 0);
	/* deschidem socket pentru tcp */
	sockfd2 = socket(AF_INET, SOCK_STREAM, 0);
	/* Luam portul din linia de comanda*/
	port_number = atoi(argv[1]);

	/* setam datele pentru server*/
	memset((char *)&serv_data, 0, sizeof(sockaddr_in));
	serv_data.sin_family = AF_INET;
	serv_data.sin_addr.s_addr = INADDR_ANY;
	serv_data.sin_port = htons(port_number);

	bind(sockfd1, (struct sockaddr *)&serv_data, sizeof(struct sockaddr));
	setsockopt(sockfd2, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
	bind(sockfd2, (struct sockaddr *)&serv_data, sizeof(struct sockaddr));
	listen(sockfd2, 5);

	/* adaugam noul file descriptor in setul de rd */
	FD_SET(sockfd2, &rd_fds);
	FD_SET(sockfd1, &rd_fds);
	if (sockfd2 > sockfd1)
		fdmax = sockfd2;
	else
		fdmax = sockfd1;

	while (true) {
		aux_fds = rd_fds;

		select(fdmax + 1, &aux_fds, NULL, NULL, NULL);

		/* vedem daca userul da o comanda */
		if (FD_ISSET(0, &aux_fds)) {
			char key_buff[6];
			memset(key_buff, 0, 6);
			fgets(key_buff, 5, stdin);
			char *command = strtok(key_buff, " ");
			if (strncmp(command, "exit", 5) == 0)
			{
				/* inchidem toate socketurile */
				/* Pentru tcp inchidem doar interceptarea datelor
				 * dar cele de transmitere nu, a.i ei vor putea trimite
				 * in continuare
				 */
				shutdown(sockfd2, SHUT_RD);
				for (int i = 0; i < fdmax + 1; i++)
					if (FD_ISSET(i, &aux_fds))
						 shutdown(i, SHUT_RD);
				/* acum inchidem tot */
				shutdown(sockfd1, SHUT_RDWR);
				return 0;
			}
		}

		for (int i = 0; i < fdmax + 1; i++) {
			int isSet = FD_ISSET(i, &aux_fds); 
			if (isSet) {
				if (i == sockfd1) {
					memset(buffer2, 0, 1551);
					cli = sizeof(cli_data);
					recvfrom(sockfd1, buffer2, 1551, 0, (struct sockaddr *)&cli_data, &cli);

					/* luam ip-ul si portul clientilor udp */
					in_addr for_ip = cli_data.sin_addr;
					in_port_t for_port = cli_data.sin_port;
					char *ip = inet_ntoa(for_ip);
					int port = ntohs(for_port);

					/* punem mesajul pentru udp */
					struct data_udp_message *recv_message = (struct data_udp_message *)buffer2;

					/* Daca nimeni nu e abonat, putem sa ignoram */
					std::map<string, vector<pair<string, int>>>::iterator topics = map3.find(recv_message->topic_message);
					if (topics != map3.end())
					{
						/* Facem mesajul */
						payload.port = port;
						payload.type_message = recv_message->type_message;
						strncpy(payload.ip, ip, strlen(ip));
						memset(payload.topic_message, 0, 50);
						strncpy(payload.topic_message, recv_message->topic_message, strlen(recv_message->topic_message));
						memcpy(payload.payload, recv_message->payload, 1500);

						/* Pentru fiecare client abonat la topic */
						for (int i = 0; i < topics->second.size(); i++) {
							pair<string, int> element = topics->second[i];
							/* Ne uitam dupa clientul cu id-ul respectiv*/
							map<string, struct data_tcp_client>::iterator to_find = map2.find(element.first);
							if (to_find != map2.end()) {
								struct data_tcp_client *elem = &(to_find->second);
								/* Daca sunt pe server, trimitem mesajul */
								if (!elem->exists) {
									/* Daca sunt offline dar au SF-ul 1, pune l in lista de mesaje */
									if (element.second == 1) {
										elem->messages.push_back(payload);
									}
									
								} else {
									/* Serverul nu ar trebui sa se opreasca din a trimite mesaje, ba chiar
									 * sa trimita incontinuu
									 */
									int ret = 0;
									do {
										ret = send(elem->sockfd, (char *)&payload, sizeof(struct data_cli_message), 0);
									} while(ret == 0);
								}
							}
						}
					}
				} else if (i == sockfd2) {
					/* o noua cerere pe socketul inactiv */
					cli = sizeof(cli_data);
					newsockfd = accept(sockfd2, (struct sockaddr *)&cli_data, &cli);
					/* dam turn off la algoritmul lui Neagle */
					int disable_alg = 1;
					setsockopt(newsockfd, IPPROTO_TCP, TCP_NODELAY, &(disable_alg), sizeof(int));

					/* adaugam noul socket acceptat */
					FD_SET(newsockfd, &rd_fds);
					fdmax = std::max(newsockfd, fdmax);

					/* luam mesajul de la client */
					struct data_tcp_message message;
					recv(newsockfd, buffer, 256, 0);
					memcpy(&message, buffer, sizeof(struct data_tcp_message));

					/*vedem daca clientul exista  */
					string id_cl = message.id_clients;
					map<string, struct data_tcp_client>::iterator to_find = map2.find(id_cl);
					if (to_find != map2.end())
					{
						/* verificam daca clientul este deconectat */
						if (to_find->second.exists == 0)
						{
							/* Il punem in mapa */
							map1.insert(make_pair(newsockfd, id_cl));

							/* ii setam socketul */
							to_find->second.sockfd = newsockfd;
							char *address = inet_ntoa(cli_data.sin_addr);
							uint16_t port = ntohs(cli_data.sin_port);
							printf("New client %s connected from %s:%d.\n",
								   message.id_clients, address, port);

							/* setam ca clientul este conectat */
							to_find->second.exists = CONNECTED;

							/* si acum vom trimite toate mesajele din lista */
							for (int i = 0; i < to_find->second.messages.size(); i++) {
								struct data_cli_message element = to_find->second.messages[i];
								int ret = 0;
								/* Daca un user nu a fost notifica va trebui retrimis*/
								do {
									ret = send(newsockfd, (char *)&element, sizeof(struct data_cli_message), 0);
								} while (ret == 0);
							}
							to_find->second.messages.clear();
						} else {
							printf("Client %s already connected.\n", message.id_clients);
							/* Inchidem socketul */
							close(newsockfd);
							/* Si il si stergem */
							FD_CLR(newsockfd, &rd_fds);
						}
					} else {
						/* In cazul acesta doar creem un nou client */
						struct data_tcp_client cl;
						/* el exista */
						cl.exists = CONNECTED;
						/* setam socketul */
						cl.sockfd = newsockfd;
						/* adaugam noul client */
						map2.insert(make_pair(id_cl, cl));
						/* Il punem si cu socketul respectiv */
						map1.insert(make_pair(newsockfd, id_cl));
						
						char *address = inet_ntoa(cli_data.sin_addr);
						uint16_t port = ntohs(cli_data.sin_port);
						printf("New client %s connected from %s:%d.\n",
							   message.id_clients, address, port);
					}
				} else {
					/* Aici primim date de la un socket */
					strcpy(buffer, "0");
					int ret = recv(i, buffer, 256, 0);
					/*
					 * Nu s-a primit mesajul de la clientul tc. Serverul nu se opreste si se va reincerca
					 * conectarea
					 */
					if(ret < 0) { 
						continue;
					}
					struct data_tcp_message message;
					memcpy(&message, buffer, sizeof(struct data_tcp_message));

					/* inchidem conexiunea */
					if (ret == 0)
					{
						/* ne uitam dupa clientul asociat cu acel id */
						map<int, string>::iterator to_find = map1.find(i);
						cout << "Client " << to_find->second << " disconnected." << endl;

						/* inchidem socketul */
						close(i);
						/* stergem socketul */
						FD_CLR(i, &rd_fds);
						/* il stergem si din mapa odata ce nu mai este nevoie de el */
						map1.erase(i);

						/* marcam clientul offline */
						map<string, struct data_tcp_client>::iterator to_find2 = map2.find(to_find->second);
						to_find2->second.exists = DISCONNECTED;
						/* setam socketul pe -1 */
						to_find2->second.sockfd = NO_SOCKET;
					} else {
						if (message.msg_type == CONNECTED)
						{
							/* Ia topicul la care un client vrea sa se aboneze */
							string topic_elem;
							char *aux = strtok(message.buffer, " ");
							topic_elem = aux;

							/* luam tipul de subscriptie */
							aux = strtok(NULL, " ");
							if (aux != NULL)
								subs_type = atoi(aux);

							/* luam id ul */
							string id = message.id_clients;

							/* daca topicul nu e il adaugam in lista */
							map<string, vector<pair<string, int>>>::iterator to_find = map3.find(topic_elem);
							if (to_find != map3.end()) {
								/* ne asiguram ca un client nu a dat subscribe de doua ori in acelasi fel*/
								map<string, vector<pair<string, int>>>::iterator to_find = map3.find(topic_elem);

								for (int i = 0; i < to_find->second.size(); i++) {
									pair<string, int> subs_top = to_find->second[i];
									/* acelasi client alt tip de subscriptie*/
									if (id == subs_top.first) {
										subscribed = 1;
										if (subs_type != subs_top.second)
										{
											different_subscribed = 1;
											break;
										}
									}
									position++;
								}

								/* clientul vrea o noua subscriptie */
								if (different_subscribed != DISCONNECTED)
									to_find->second[position].second = subs_type;

								/* e un nou client */
								if (subscribed == DISCONNECTED)
									to_find->second.push_back(make_pair(id, subs_type));
							} else {
								/* creeaza o noua lista si adauga topicul si clientul */
								subscriptions = new vector<pair<string, int>>;
								(*subscriptions).push_back({id, subs_type});

								/* retinem topicul si subscriptiile */
								map3.insert({topic_elem, *subscriptions});
							}
						} if (message.msg_type == 2) {
							/* Cerere de unsubscriptie */
							/* luam topicul de la care clientul vrea sa si dea unsubscribe */
							char *aux = strtok(message.buffer, " ");
							string topic_element = aux;

							/* luam id ul */
							string id = message.id_clients;

							/* Daca clientul se dezaboneaza de la un topic care nu exista il ignoram */
							position = 0;
							map<string, vector<pair<string, int>>>::iterator to_find = map3.find(topic_element);
							if (to_find != map3.end()) {
								/* daca clientul vrea sa se dezaboneze de la un topic la care este abonat, ignoram  */
								for (int i = 0; i < to_find->second.size(); i++) {
									pair<string, int> top = to_find->second[i];
									if (id == top.first) {
										ignore = true;
										break;
									}
								}
								position++;
							}

							/* stergem clientul din lista */
							if (ignore) {
								(to_find->second).erase((to_find->second).begin() + position - 1);
							}
						}
					}
				}
			}
		}
	}

	/* inchidem toate socketurile */
	close(sockfd2);
	for (int i = 0; i < fdmax + 1; i++)
		if (FD_ISSET(i, &aux_fds))
			close(i);
	close(sockfd1);

	return 0;
}
