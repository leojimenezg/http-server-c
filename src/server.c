#include <stdio.h>      // Standard I/O.
#include <stdlib.h>     // Standard library.
#include <string.h>     // String manipulation.
#include <sys/socket.h> // Socket API.
#include <arpa/inet.h>  // Network conversions.
#include <unistd.h>     // UNIX system calls.
#include <pthread.h>    // POSIX threads.
#include <stdatomic.h>  // Atomic types.

#define IP_VERSION AF_INET
#define SOCKET_TYPE SOCK_STREAM
#define MAX_PENDING 10
#define BUFFER_SIZE 1024
#define HTTP_VERB_LEN 7
#define HTTP_URL_LEN 63

_Atomic static int current_connections = 0;

typedef struct sockaddr_in socket_info_ipv4;
typedef struct {
	int socketfd;
	int max_current_connections;
	socket_info_ipv4 socket_info;
} Server_Info;

void errorAndExit(const char *err) {
	perror(err);
	exit(1);
}

int newClientConnection(Server_Info *server) {
	socket_info_ipv4 connection_info;
	socklen_t space = sizeof(connection_info);
	int connection = accept(server->socketfd, (struct sockaddr *)&connection_info, &space);
	return connection;
}

void* handleClientConnection(void *arg) {
	int *socketfd_ptr = (int*) arg;  // File descriptor allocated in heap.
	char buffer[BUFFER_SIZE + 1];
	int receiving = recv(*socketfd_ptr, (void *)buffer, BUFFER_SIZE, 0);
	if (receiving < 1) {
		perror(">>No information received from client"); 
		goto end_connection;
	}
	buffer[receiving] = '\0';
	char *separation1_ptr = strchr(buffer, ' ');
	if (separation1_ptr == NULL) {
		// Answer with error.
		goto end_connection;
	}
	int http_verb_len = separation1_ptr - buffer;
	if (http_verb_len > HTTP_VERB_LEN) {
		// Answer with error.
		goto end_connection;
	}
	char http_verb[HTTP_VERB_LEN + 1];
	strncpy(http_verb, buffer, http_verb_len);
	http_verb[http_verb_len] = '\0';
	char *separation2_ptr = strchr(separation1_ptr + 1, ' ');
	if (separation2_ptr == NULL) {
		// Answer with error.
		goto end_connection;
	}
	int url_len = separation2_ptr - (separation1_ptr + 1);
	if (url_len > HTTP_URL_LEN) {
		// Answer with error.
		goto end_connection;
	}
	char url[HTTP_URL_LEN + 1];
	strncpy(url, separation1_ptr + 1, url_len);
	url[url_len] = '\0';
	char body[] = "<html><body><h1>Hello world!</h1></body></html>";
	char response[BUFFER_SIZE];
	sprintf(response, "HTTP/1.1 200 OK\r\n"
		 "Content-Type: text/html\r\n"
		 "Content-Length: %d\r\n"
		 "\r\n"
		 "%s", (int)strlen(body), body);
	int sending = send(*socketfd_ptr, (void *)response, strlen(response), 0);
	if (sending < 0) perror(">>Could not sent response to client");
	printf(">>Answer sent to client successfully!\n");
	end_connection:;
	close(*socketfd_ptr);
	current_connections--;
	free(socketfd_ptr);
	printf(">>Closed connection with client\n");
	return 0;
}

int main(int argc, char **argv) {
	/* Args order:
	* 1. IP
	* 2. Port
	* 3. Maximum connections
	*/
	if (argc < 4) errorAndExit(">>Missing required arguments to build the server");
	int port = atoi(argv[2]);
	if (port < 1) errorAndExit(">>Invalid port");
	int max_connections = atoi(argv[3]);
	if (max_connections < 1) errorAndExit(">>Invalid maximum number of current connections");
	Server_Info server = {
		.max_current_connections = max_connections,
		.socket_info.sin_family = IP_VERSION,
		.socket_info.sin_port = htons(port)  // Port in machine byte order to network byte order (big endian).
	};
	int ip = inet_pton(server.socket_info.sin_family, argv[1], &server.socket_info.sin_addr);
	if (ip < 1) errorAndExit(">>Invalid IP address");
	server.socketfd = socket(IP_VERSION, SOCKET_TYPE, 0);
	if (server.socketfd < 0) errorAndExit(">>Could not create the socket");
	int binding = bind(server.socketfd, (struct sockaddr *)&server.socket_info, sizeof(server.socket_info));
	if (binding < 0) errorAndExit(">>Could not bind the socket");
	int listening = listen(server.socketfd, MAX_PENDING);
	if (listening < 0) errorAndExit(">>Could not listen in socket");
	while (1) {
		if (current_connections >= server.max_current_connections) goto next_iteration;
		int new_connection = newClientConnection(&server);
		if (new_connection < 0) {
			perror(">>Could not accept new client connection");
			goto next_iteration;
		}
		current_connections++;
		int *connection_ptr_heap = malloc(sizeof(int));
		if (connection_ptr_heap == NULL) {
			perror(">>Could not allocate memory for new client connection");
			close(new_connection);
			current_connections--;
			goto next_iteration;
		}
		*connection_ptr_heap = new_connection;
		pthread_t thread; pthread_attr_t attributes;
		pthread_attr_init(&attributes);
		pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
		if (pthread_create(&thread, &attributes, handleClientConnection, connection_ptr_heap) != 0) {
			close(new_connection);
			free(connection_ptr_heap);
			current_connections--;
			perror(">>Could not create new thread to handle client connection");
		}
		pthread_attr_destroy(&attributes);
		next_iteration:;
	}
	printf(">>Server finished\n");
	close(server.socketfd);
	return 0;
}
