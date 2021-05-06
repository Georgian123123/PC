#ifndef _HELPERS_H
#define _HELPERS_H

#include <bits/stdc++.h> 
#include <netinet/tcp.h>

using namespace std;


/* date pentru int*/
struct data_int {
	char how;
	int val; 
}__attribute__((packed));

/* date pentru short*/
struct data_short {
	short val;
}__attribute__((packed));

/* date pentru float*/
struct data_float {
	char how;
	int val;
	char exponent;
}__attribute__((packed));

/* datele unui mesaj tcp */
struct data_tcp_message {
	int msg_type;
	char id_clients[10];
	char buffer[242];
}__attribute__((packed));

/* datele unui mesaj de la client la server */
struct data_cli_message {
	char ip[30];
	int port;
	char topic_message[50];
	char type_message;
	char payload[1500];
}__attribute__((packed));

/* datele pentru un client udp */
struct data_udp_message {
	char topic_message[50];
	char type_message;
	char payload[1500];
}__attribute__((packed));

/* date pentru client tcp */
struct data_tcp_client {
	int sockfd;
	int exists;
	vector<struct data_cli_message> messages;
};
#endif
