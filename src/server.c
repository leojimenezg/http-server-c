#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/_types/_socklen_t.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define IP_VERSION AF_INET
#define SOCKET_TYPE SOCK_STREAM
#define MAX_PENDING 10
#define BUFFER_SIZE 1024

void errorAndExit(const char *err) {
	perror(err);
	exit(1);
}

static int current_connections = 0;

typedef struct sockaddr_in socket_info_ipv4;
typedef struct {
	int socket;
	int max_connections;
	socket_info_ipv4 socket_info;
} Server_Info;

int main(int argc, char **argv) {
	/* Args order:
	* 1. IP
	* 2. Port
	* 3. Maximum connections
	*/
	if (argc < 4) errorAndExit("Missing required arguments to build the server");
	int port = atoi(argv[2]);
	if (port < 1) errorAndExit("Invalid port");
	int max_connections = atoi(argv[3]);
	if (max_connections < 1) errorAndExit("Invalid maximum number of connections");
	Server_Info server = {
		.max_connections = max_connections,
		.socket_info.sin_family = IP_VERSION,
		.socket_info.sin_port = htons(port)  // Port in machine byte order to network byte order (big endian).
	};
	int ip = inet_pton(server.socket_info.sin_family, argv[1], &server.socket_info.sin_addr);
	if (ip < 1) errorAndExit("Invalid IP address");
	server.socket = socket(IP_VERSION, SOCKET_TYPE, 0);
	if (server.socket < 0) errorAndExit("Could not create the socket");
	int binding = bind(server.socket, (struct sockaddr *)&server.socket_info, sizeof(server.socket_info));
	if (binding < 0) errorAndExit("Could not bind the socket");
	int listening = listen(server.socket, MAX_PENDING);
	if (listening < 0) errorAndExit("Could not listen in socket");
	// ->Accept client connection starts
	socket_info_ipv4 connection_info;
	socklen_t connection_space = sizeof(connection_info);
	int connection = accept(server.socket, (struct sockaddr *)&connection_info, &connection_space);
	if (connection < 0) perror("Could not connect to new client");
	// Client connection accepted<-
	// ->Receive request from client
	char buffer[BUFFER_SIZE + 1];
	int receiving = recv(connection, (void *)buffer, BUFFER_SIZE, 0);
	if (receiving < 1) {
		close(connection);
		errorAndExit("Could not receive information from client");
	} 
	buffer[receiving] = '\0';
	printf("Received from client:\n%s\n", buffer);
	// Client request received<-
	// ->Process client request<-
	// ->Form answer based on client request
	char body[] = "<html><body><h1>Hello world!</h1></body></html>";
	char response[BUFFER_SIZE];
	sprintf(response, "HTTP/1.1 200 OK\r\n"
					  "Content-Type: text/html\r\n"
					  "Content-Length: %d\r\n"
					  "\r\n"
					  "%s", (int)strlen(body), body);
	// Finish forming answer to client<-
	// ->Send answer to client
	int sending = send(connection, (void *)response, strlen(response), 0);
	if (sending < 0) errorAndExit("Could not sent response to client");
	// Finish sending answer to client<-
	// ->Close server-client connection
	printf("Closed connection with client\n");
	close(connection);
	// Finish closing server-client connection<-
	printf("Server finished\n");

	close(server.socket);
	return 0;
}
