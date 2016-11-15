#define _GNU_SOURCE             /* See feature_test_macros(7) */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

void err(const char *msg) { perror(msg); exit(EXIT_FAILURE); }

/**
 * Copies data from intfd into outfd
 * file descriptor.
 * Returns number of copied bytes or
 * -1 on error (and leaves errno set)
 */

ssize_t copyFds(int infd, int outfd)
{
	char buffer[64*1024];
	ssize_t totalyTotalWritten = 0;
	while (1) {
		ssize_t bytesRead = 0;
		bytesRead = read(infd, buffer , sizeof(buffer));
		if (bytesRead == 0) {
			fprintf(stderr, "EOF\n");
			return totalyTotalWritten;
		}
		if (bytesRead < 0) {
			if (errno != EINTR && errno != EAGAIN) {
				return -1;
			}
			bytesRead = 0;
		}

		ssize_t totalWritten = 0;
		while (totalWritten < bytesRead) {
			ssize_t bytesWritten = write(outfd,
				buffer + totalWritten,
				bytesRead - totalWritten);
			if (bytesWritten == -1) {
				if (errno != EINTR && errno != EAGAIN) {
					return -1;
				}
				bytesWritten = 0;
			}
			totalWritten += bytesWritten;
			totalyTotalWritten += bytesWritten;
		}
	}
	return 0;
}

struct client {
	int fd;
	char addr[20];
	uint16_t port;
	int lock;
};


struct client clients[1000];
int nClients = 0;

int main()
{

	int sockfd;
	int ret = 0;

	sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (sockfd == -1)
		err("socket");
	
	uint16_t portno = 1235;
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		err("bind");

	if (listen(sockfd, 10) == -1)
		err("listen");

	while(1) {

		struct sockaddr_in clientAddr;
		socklen_t clientAddrSize = sizeof(clientAddr);
		int clientSock = accept4(sockfd, (struct sockaddr *) &clientAddr,
				&clientAddrSize, SOCK_NONBLOCK);
		if (clientSock == -1 && errno != EAGAIN)
			err("accept");

		if (clientSock != -1) {
			fprintf(stderr, "New connection from %s:%d\n",
					inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
			clients[nClients].fd = clientSock;
			clients[nClients].port = ntohs(clientAddr.sin_port);
			clients[nClients].lock = 0;
			strncpy(clients[nClients].addr, inet_ntoa(clientAddr.sin_addr),
					sizeof(clients[nClients].addr));
			nClients++;
		}

		int i=0;
		for (i=0; i<nClients; ++i) {
			struct client *c = &clients[i];
			char buffer[1024*1024];
			int bytesRead = read(c->fd, buffer, sizeof(buffer));

			if(bytesRead==-1){
				c->lock = 0;
			}

			if (bytesRead == -1 && errno != EAGAIN) err("reading from client");

			if (bytesRead > 0) {
				if (c->lock==0) {
					c->lock = 1;
					fprintf(stderr, "%s:%d: ", c->addr, c->port);
					write(1, buffer, bytesRead);

					int j;
					for (j=0; j<nClients; j++){
						if(i!=j){
							struct client *nc = &clients[j];

						char converted_port[10];
						sprintf(converted_port, "%d", c->port);
						//fprintf(stderr, "convertovany port: %s:%d ", converted_port, c->port);			

						char result_string[20];
						strcpy (result_string, c->addr);
						strcat (result_string, ":");
						strcat (result_string, converted_port);
						strcat (result_string, ": ");
/*
						puts (result_string);
*/

						//write message
							write(nc->fd, result_string, sizeof(result_string));
							write(nc->fd, buffer, bytesRead);
						
						}
					}
				
				}
				
				else if (c->lock==1) {
					fprintf(stderr, "Warning! You have lost data from %s:%d \n", c->addr, c->port);
				}
			}


			else if (bytesRead == 0) {
				fprintf(stderr, "Connection from %s:%d closed\n", c->addr, c->port);
				shutdown(c->fd, SHUT_RDWR); 
				close(c->fd);
				if (i<nClients-1)
					memcpy( &clients[i], &clients[nClients-1], sizeof(clients[i]));
				nClients--;
			}
		}
	}

	close(sockfd);
	return ret;
}



/* vim: set sw=4 sts=4 ts=4 noet : */
