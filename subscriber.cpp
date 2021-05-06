#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#define SUBSCRIBE 1
#define UNSUBSCRIBE 1
#define MOD 1000000007

void usage(char *file)
{
	fprintf(stderr, "Usage: %s client_id server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    char client_id[1024], recv_buffer[1585], buffer[256];
	char *result;
	int sockfd, fdmax;
	fd_set rd_fds;
	fd_set aux_fds;
	struct sockaddr_in serv_data;
	struct data_tcp_message message;
	memset(recv_buffer, 0, 1585);

	if (argc < 4) {
		usage(argv[0]);
	}
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

    /* seteaza id ul clientului */
    strncpy (client_id, argv[1], sizeof(client_id));
	serv_data.sin_family = AF_INET;
    /* Seteaza portul */
	serv_data.sin_port = htons(atoi(argv[3]));
    /* Seteaza adresa */
	inet_aton(argv[2], &serv_data.sin_addr);
	connect(sockfd, (struct sockaddr*) &serv_data, sizeof(serv_data));

	/* Trebuie sa trimitem un mesaj cu id-ul clientului */
	message.msg_type = 0;
	memset(message.id_clients, 0, 10);
	strncpy(message.id_clients, client_id, 10);

	/* Ataseaza mesajul si trimite l */
	memset(message.buffer, 0, 256);
	memcpy (buffer, &message, sizeof(struct data_tcp_message));
	send(sockfd, buffer, 256, 0);
	fdmax = sockfd;

	/* zeroizam zona */
	FD_ZERO(&rd_fds);
	FD_ZERO(&aux_fds);

	/* setam zona*/
	FD_SET(sockfd, &rd_fds);
	FD_SET(0, &rd_fds);
	aux_fds = rd_fds;

	/* mesaje de eroare pt subs si unsubs*/
	int err_unsub = 0;
	int err_sub = 0;
	while (1) {
		/* Luam vechiul file descriptor */
		aux_fds = rd_fds;
		
		/* Selectam fdmaxul.*/
		select(fdmax + 1, &aux_fds, NULL, NULL, NULL);

		int is_set = FD_ISSET(0, &aux_fds); 
		int is_set2 = FD_ISSET(sockfd, &aux_fds);
		if (is_set2){
			memset(recv_buffer, 0, 1585);
			socklen_t server = sizeof(serv_data);
			int ret = recvfrom(sockfd, recv_buffer, 1585, 0, (struct sockaddr *)&serv_data, &server);

			/* Eroare, socket inchis!*/
			if (ret == 0) {
				printf("Server closed\n");
				break;
			}

			/* afiseaza mesajul la output*/
			struct data_cli_message *recv_message = (struct data_cli_message *) recv_buffer;
			printf("%s:%d - %s", recv_message->ip, recv_message->port, recv_message->topic_message);

			if (recv_message->type_message == 0) {
				/* INT */
				struct data_int *_int = (struct data_int *)recv_message->payload;
				printf(" - INT - ");
				if (_int->how) {
					printf("-");
				} 
				printf("%d", ntohl(_int->val));
			} else if (recv_message->type_message == 1) {
				/* SHORT */
				struct data_short *_short= (struct data_short *)recv_message->payload;
				uint16_t aux = ntohs(_short->val);
				printf(" - SHORT_REAL - %.2f",  aux * 0.01);
			} else if (recv_message->type_message == 2) {
				/* FLOAT */
				struct data_float *_float = (struct data_float *)recv_message->payload;
				printf(" - FLOAT - ");
				if (_float->how) {
					printf("-");
				}
				if (!_float->exponent) {
					printf("%d", ntohl(_float->val));
				} else {
					uint32_t aux = ntohl(_float->val);
					printf("%.*f", _float->exponent, (aux * (0.1 / pow(10, _float->exponent - 1))));
				}
			} else {
				/* STRING */
				recv_message->payload[1500] = '\0';
				printf(" - STRING - %s", recv_message->payload);
			}
			printf("\n");
			memset(recv_buffer, 0, 1585);
		} else if (is_set) {

			// se citeste de la tastatura
			memset(message.buffer, 0, 256);
			fgets(buffer, 255, stdin);

			char *command = strtok(buffer, " ");
			err_unsub = 0;
			err_sub = 0;

			/* comanda de subscribe */
			if (strncmp(command, "subscribe", 10) == 0) {

				/* tipul de mesaj este 1 */
				message.msg_type = SUBSCRIBE;

				/* seteaza id-ul clientului */
				memset(message.id_clients, 0, 10);
				strncpy(message.id_clients, client_id, 10);

				/* punem topicul si SF ul in buffer pentru a fi trimis */
				memset(message.buffer, 0, 242);
				memcpy(message.buffer, buffer + 10, 255);

				/* pune messajul in buffer si trimite-l */
				memset(buffer, 0, 256);
				memcpy (buffer, &message, sizeof(struct data_tcp_message));
				send(sockfd, buffer, 256, 0);

				printf("Subscribed to topic.\n");
			} else if (strncmp(command, "unsubscribe", 12) == 0) {
				/* comanda de unsubscribe*/

				/* tipul de mesaj este 2 */
				message.msg_type = UNSUBSCRIBE;

				/* setam id-ul clientului */
				memset(message.id_clients, 0, sizeof (message.id_clients));
				strncpy(message.id_clients, client_id, sizeof(message.id_clients));

				/* luam titlul topicului si punem in buffer */
				command = strtok(NULL, " ");
				command[strlen(command) - 1] = '\0';
				memset(message.buffer, 0, sizeof (message.buffer));
				memcpy(message.buffer, command, strlen(command));
				
				/* Punem mesajul in buffer si il trimitem*/
				memset(buffer, 0, 256);
				memcpy (buffer, &message, sizeof(struct data_tcp_message));
				send(sockfd, buffer, 256, 0);

				printf("Unsubscribed to topic\n.");
			} else if (strncmp(command, "exit\n", 6) == 0) {
				/* comanda de exit*/
				break;
			}
		}
	}
	/* Se primesc mesajele care inca mai sunt in buffer */
	shutdown(sockfd, SHUT_WR);
	return 0;
}
