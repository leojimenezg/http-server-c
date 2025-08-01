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

typedef struct {
	char *url;
	char *file_path;
} Resource_Info;

Resource_Info allowed_resources[] = {
	{"/", "public/html/index.html"},
	{"/index", "public/html/index.html"},
	{"/about", "public/html/about.html"},
	{"/styles", "public/css/styles.css"},
	{"/script", "public/js/script.js"},
	{"/favicon.ico", "public/assets/favicon.ico"},
	{NULL, NULL},
};

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
	printf(">>Thread%p: New client connection accepted\n", (void*)pthread_self());
	fflush(stdout);
	int *socketfd_ptr = (int*) arg;  // File descriptor allocated in heap.
	char buffer[BUFFER_SIZE + 1];
	int receiving = recv(*socketfd_ptr, (void *)buffer, BUFFER_SIZE, 0);
	if (receiving < 1) goto error_process;
	buffer[receiving] = '\0';
	char *separation1_ptr = strchr(buffer, ' ');
	if (separation1_ptr == NULL) goto error_process;
	int http_verb_len = separation1_ptr - buffer;
	if (http_verb_len > HTTP_VERB_LEN) goto error_process;
	char http_verb[HTTP_VERB_LEN + 1];
	strncpy(http_verb, buffer, http_verb_len);
	http_verb[http_verb_len] = '\0';
	if (strcmp(http_verb, "GET") != 0) goto error_process;
	char *separation2_ptr = strchr(separation1_ptr + 1, ' ');
	if (separation2_ptr == NULL) goto error_process;
	int url_len = separation2_ptr - (separation1_ptr + 1);
	if (url_len > HTTP_URL_LEN) goto error_process;
	char url[HTTP_URL_LEN + 1];
	strncpy(url, separation1_ptr + 1, url_len);
	url[url_len] = '\0';
	int resource = -1;
	for (int i = 0; allowed_resources[i].url != NULL; i++) {
		if (strcmp(url, allowed_resources[i].url) == 0) {
			resource = i;
			break;
		}
	}
	if (resource < 0) goto error_request;
	// TODO: Handle files bigger than BUFFER_SIZE and change Content-Type.
	FILE *file_ptr = fopen(allowed_resources[resource].file_path, "r");
	if (file_ptr == NULL) goto error_request;
	fseek(file_ptr, 0, SEEK_END);
	int file_size = ftell(file_ptr);
	fseek(file_ptr, 0, SEEK_SET);
	char *data_ptr_heap = malloc(file_size + 1);
	if (data_ptr_heap == NULL) {
		fclose(file_ptr);
		goto error_request;
	}
	fread(data_ptr_heap, sizeof(char), file_size, file_ptr);
	data_ptr_heap[file_size] = '\0';
	fclose(file_ptr);
	char response[BUFFER_SIZE];
	// Successful request-answer (200 status code).
	sprintf(response, "HTTP/1.1 200 OK\r\n"
		 "Content-Type: text/html\r\n"
		 "Content-Length: %d\r\n"
		 "\r\n"
		 "%s", file_size, data_ptr_heap);
	int sending_ok = send(*socketfd_ptr, (void *)response, strlen(response), 0);
	free(data_ptr_heap);
	if (sending_ok < 0) perror(">>Could not sent response to client");
	goto end_connection;
	// Invalid request-answer (404 status code).
	error_request:;
	char body_invalid[] = "<html><body><h1>Invalid Resource</h1></body></html>";
	char response_invalid[BUFFER_SIZE];
	sprintf(response_invalid, "HTTP/1.1 404 Not Found\r\n"
		 "Content-Type: text/html\r\n"
		 "Content-Length: %d\r\n"
		 "\r\n"
		 "%s", (int)strlen(body_invalid), body_invalid);
	int sending_invalid = send(*socketfd_ptr, (void *)response_invalid, strlen(response_invalid), 0);
	if (sending_invalid < 0) perror(">>Could not sent response to client");
	goto end_connection;
	// Error request-answer (400 status code).
	error_process:;
	char body_fail[] = "<html><body><h1>Bad Request</h1></body></html>";
	char response_fail[BUFFER_SIZE];
	sprintf(response_fail, "HTTP/1.1 400 Bad Request\r\n"
		 "Content-Type: text/html\r\n"
		 "Content-Length: %d\r\n"
		 "\r\n"
		 "%s", (int)strlen(body_fail), body_fail);
	int sending_fail = send(*socketfd_ptr, (void *)response_fail, strlen(response_fail), 0);
	if (sending_fail < 0) perror(">>Could not sent response to client");
	end_connection:;
	close(*socketfd_ptr);
	current_connections--;
	free(socketfd_ptr);
	printf(">>Thread%p: Closed connection with client\n", (void*)pthread_self());
	fflush(stdout);
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
