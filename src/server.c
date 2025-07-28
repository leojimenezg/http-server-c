#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define IP_VERSION AF_INET
#define SOCKET_TYPE SOCK_STREAM

void errorExit(const char *err) {
	perror(err);
	exit(1);
}

typedef struct {
	int socket;  // File descriptor of the socket.
	int max_connections;  // Maximum number of connections.
	struct sockaddr_in server_address;  // Struct to hold socket address info.
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
	return 0;
}
