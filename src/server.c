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
#define HEADERS_SIZE 256
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
	char *file_MIME;  // Multipurpose Internet Mail Extension
} Resource_Info;

Resource_Info allowed_resources[] = {
	{"/", "public/html/index.html", "text/html"},
	{"/index", "public/html/index.html", "text/html"},
	{"/about", "public/html/about.html", "text/html"},
	{"/styles", "public/css/styles.css", "text/css"},
	{"/script", "public/js/script.js", "application/javascript"},
	{"/favicon.ico", "public/assets/favicon.ico", "image/x-icon"},
	{NULL, NULL},
};

void errorAndExit(const char *err) {
	perror(err);
	exit(1);
}

/*
 * \brief Accepts a new client connection.
 * \param *server Struct containing the server's information.
 * \return file-descriptor of the socket dedicated to a client connection.
 * \return negative-int if the socket can't be created.
*/
int newClientConnection(Server_Info *server) {
	socket_info_ipv4 connection_info;
	socklen_t space = sizeof(connection_info);
	int connection = accept(server->socketfd, (struct sockaddr *)&connection_info, &space);
	return connection;
}

/*
 * \brief Validates if the HTTP verb in a request matches the expected verb.
 * \param request Pointer to null-terminated HTTP request string.
 * \param verb Pointer to null-terminated string containing the expected HTTP verb.
 * \return 1 if the verb matches the expected verb.
 * \return 0 if a valid verb was found but doesn't match the expected verb.
 * \return -1 if no verb could be extracted or the verb length exceeds HTTP_VERB_LEN.
*/
int isRequestVerbValid(const char *request, const char *verb) {
	char *separation_ptr = strchr(request, ' ');
	if (separation_ptr == NULL) return -1;
	int http_verb_len = separation_ptr - request;
	if (http_verb_len > HTTP_VERB_LEN) return -1;
	char http_verb[HTTP_VERB_LEN + 1];
	strncpy(http_verb, request, http_verb_len);
	http_verb[http_verb_len] = '\0';
	if (strcmp(http_verb, verb) == 0) return 1;
	return 0;
}

/*
 * \brief Validates if the URL in a request matches any of the allowed resources.
 * \param request Pointer to null-terminated HTTP request string.
 * \return index of the matched allowed resource.
 * \return -1 if no url could be extracted or matched, or the url length exceeds HTTP_URL_LEN.
*/
int isRequestUrlResourceValid(const char *request) {
	char *separation1_ptr = strchr(request, ' ');
	if (separation1_ptr == NULL) return -1;
	char *separation2_ptr = strchr(separation1_ptr + 1, ' ');
	if (separation2_ptr == NULL) return -1;
	int url_length = separation2_ptr - (separation1_ptr + 1);
	if (url_length > HTTP_URL_LEN) return -1;
	char url[url_length + 1];
	strncpy(url, separation1_ptr + 1, url_length);
	url[url_length] = '\0';
	for (int i = 0; allowed_resources[i].url != NULL; i++) {
		if (strcmp(url, allowed_resources[i].url) == 0) {
			return i;
		}
	}
	return -1;
}

/*
 * \brief Sends an answer to the client using information received (headers-body).
 * \param socketfd File descriptor of socket to be used to send answer.
 * \param code Status code of the request.
 * \param desc Short description of status code.
 * \param type MIME type of the body to be sent.
 * \param body Body to be sent.
 * \param size Size of the body to be sent.
 * \return 0 if the answer is successfully sent.
 * \return -1 if the answer could not be sent.
*/
int sendAnswerToClient(int socketfd, int code, char *desc, char *type, int size, char *body) {
	char headers[HEADERS_SIZE];
	sprintf(headers, "HTTP/1.1 %d %s\r\n"
					 "Content-Type: %s\r\n"
					 "Content-Length: %d\r\n"
					 "\r\n", code, desc, type, size);
	int send_headers = send(socketfd, (void *)headers, strlen(headers), 0);
	if (send_headers < 0) return -1;
	if (body != NULL && size > 0) {
		int send_body = send(socketfd, body, size, 0);
		if (send_body < 0) return -1;
	}
	return 0;
}

/*
 * \brief Gets the resource's content and answers to the client.
 * \param socketfd File descriptor of socket to be used to send answer.
 * \param resource Index of the allowed resource to be processed.
 * \return 0 if the resource and answer are successfully processed.
 * \return -1 if any of the processes fail.
*/
int handleResourceRequest(int socketfd, int resource) {
	FILE *file_ptr = fopen(allowed_resources[resource].file_path, "r");
	if (file_ptr == NULL) return -1;
	fseek(file_ptr, 0, SEEK_END);
	int file_size = ftell(file_ptr);
	fseek(file_ptr, 0, SEEK_SET);
	char *data_ptr_heap = malloc(file_size + 1);
	if (data_ptr_heap == NULL) {
		fclose(file_ptr);
		return -1;
	}
	fread(data_ptr_heap, sizeof(char), file_size, file_ptr);
	data_ptr_heap[file_size] = '\0';
	fclose(file_ptr);
	int sent = sendAnswerToClient(
		socketfd, 200, "Ok", allowed_resources[resource].file_MIME, file_size, data_ptr_heap
	);
	if (sent < 0) {
		free(data_ptr_heap);
		return -1;
	}
	free(data_ptr_heap);
	return 0;
}

void* handleClientConnection(void *arg) {
	int *socketfd_ptr = (int*) arg;  // File descriptor allocated in heap.
	char request[BUFFER_SIZE + 1];
	int receiving = recv(*socketfd_ptr, (void *)request, BUFFER_SIZE, 0);
	if (receiving < 1) {
		char body[] = "<html><body><h1>Bad Request</h1></body></html>";
		sendAnswerToClient(*socketfd_ptr, 400, "Bad Request", "text/html", strlen(body), body);
		goto end_connection;
	}
	request[receiving] = '\0';
	int verb = isRequestVerbValid(request, "GET");
	if (verb < 1) {
		char body[] = "<html><body><h1>Bad Request</h1></body></html>";
		sendAnswerToClient(*socketfd_ptr, 400, "Bad Request", "text/html", strlen(body), body);
		goto end_connection;
	}
	int resource = isRequestUrlResourceValid(request);
	if (resource < 0) {
		char body[] = "<html><body><h1>Not Found</h1></body></html>";
		sendAnswerToClient(*socketfd_ptr, 404, "Not Found", "text/html", strlen(body), body);
		goto end_connection;
	}
	int send_resource = handleResourceRequest(*socketfd_ptr, resource);
	if (send_resource < 0) goto end_connection;
	end_connection:;
	close(*socketfd_ptr);
	free(socketfd_ptr);
	current_connections--;
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
		if (new_connection < 0) goto next_iteration;
		current_connections++;
		int *connection_ptr_heap = malloc(sizeof(int));
		if (connection_ptr_heap == NULL) {
			close(new_connection);
			current_connections--;
			goto next_iteration;
		}
		*connection_ptr_heap = new_connection;
		pthread_t thread;
		pthread_attr_t attributes;
		pthread_attr_init(&attributes);
		pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
		if (pthread_create(&thread, &attributes, handleClientConnection, connection_ptr_heap) != 0) {
			close(new_connection);
			free(connection_ptr_heap);
			current_connections--;
		}
		pthread_attr_destroy(&attributes);
		next_iteration:;
	}
	printf(">>Server finished\n");
	close(server.socketfd);
	return 0;
}
