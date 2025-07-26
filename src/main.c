#include <stdbool.h>
#include <stdio.h>

#define MAX_CONNECTIONS 5
#define SERVER_PORT 4740
#define HTTP_VERSION "HTTP/2"

void newClientConnection();
bool closeClientConnection();

typedef struct {
	char *ip;
	int socket;
	bool alive;
} Connection;

int main(int argc, char *argv[]) {
	Connection con1 = {.ip = "10.10.10.10", .socket = 1, .alive = false};
	printf("Connection 1 details:\n");
	printf("IP: %s\n", con1.ip);
	printf("SOCKET: %d\n", con1.socket);
	printf("ALIVE: %s\n", con1.alive ? "true": "false");
	return 0;
}
