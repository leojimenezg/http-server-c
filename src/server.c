#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/_types/_socklen_t.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define IP_VERSION AF_INET
#define SOCKET_TYPE SOCK_STREAM
#define MAX_PENDING 10

void errorExit(const char *err) {
	perror(err);
	exit(1);
}

static int current_connections = 0;

typedef struct {
	int socket;  // File descriptor of the socket.
	int max_connections;  // Maximum number of connections.
	struct sockaddr_in server_address;  // Specific to IPV4
} Server_Info;

int main(int argc, char **argv) {
	/* Args order:
	* 1. IP
	* 2. Port
	* 3. Maximum Connections
	*/
	if (argc < 4) errorExit("Missing required arguments to build the server");
	int port = atoi(argv[2]);
	if (port < 1) errorExit("Invalid port");
	int max_connections = atoi(argv[3]);
	if (max_connections < 1) errorExit("Invalid maximum number of connections");
	Server_Info server = {
		.max_connections = max_connections,
		.server_address.sin_family = IP_VERSION,
		.server_address.sin_port = htons(port)  // Port in machine byte order to network byte order (big endian)
	};
	int ip = inet_pton(server.server_address.sin_family, argv[1], &server.server_address.sin_addr);
	if (ip < 1) errorExit("Invalid IP address");
	server.socket = socket(IP_VERSION, SOCKET_TYPE, 0);
	if (server.socket < 0) errorExit("Could not create the socket");
	int binding = bind(server.socket, (struct sockaddr *)&server.server_address, sizeof(server.server_address));
	if (binding < 0) errorExit("Could not bind the socket");
	int listening = listen(server.socket, MAX_PENDING);
	if (listening < 0) errorExit("Could not listen in socket");
	while (true) {
		if (current_connections >= server.max_connections) break;
		struct sockaddr_in connection_info;
		socklen_t available_space = sizeof(connection_info);
		int connection = accept(server.socket, (struct sockaddr *)&connection_info, &available_space);
		if (connection < 0) perror("Could not connect to new client");
		current_connections++;
		// recv() information from the connection.
		// Respond to the connection.
		// Close connection.
	}
	return 0;
}
