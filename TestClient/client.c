#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
struct connection {
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int fd;
};

char* getResponse(struct connection* c) {
    //char host[100], port[10]
	char buf[101];
    //int error;
	int nread;

	// find out the name and port of the remote host
    // error = getnameinfo((struct sockaddr *) &c->addr, c->addr_len, host, 100, port, 10, NI_NUMERICSERV);
    // if (error != 0) {
    //     fprintf(stderr, "getnameinfo: %s", gai_strerror(error));
    //     close(c->fd);
    //     return NULL;
    // }
    
    char* input = NULL;
    while((nread = read(c->fd, buf, 100)) > 0) {
        buf[nread] = '\0';
        if(input == NULL) {
            input = malloc(strlen(buf)+1);
            strcpy(input, buf);
        } else {
            strcat(input, buf);
        }
    //    printf("\t[%s:%s] got partial input as: %s\n", host, port, input);
    }
    //printf("[%s:%s] finished reading input as: %s\n", host, port, input);
    
    // begin analysis on input
    
    
    
    
    //char* response;
    close(c->fd);
    //free(input);
    free(c);
    return input;
}



int main(int argc, char **argv) {
	struct addrinfo hints, *address_list, *addr;
	int error;
	int sock;
	struct connection *con;
	
	if (argc < 4) {
		printf("Usage: %s [host] [port] [message(s)...]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	error = getaddrinfo(argv[1], argv[2], &hints, &address_list);
	if (error) {
		fprintf(stderr, "%s", gai_strerror(error));
		exit(EXIT_FAILURE);
	}
	for (addr = address_list; addr != NULL; addr = addr->ai_next) {
		sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		if (sock < 0) continue;
		if (connect(sock, addr->ai_addr, addr->ai_addrlen) == 0) {
			break;
		}
		close(sock);
	}
	if (addr == NULL) {
		fprintf(stderr, "Could not connect to %s:%s\n", argv[1], argv[2]);
		exit(EXIT_FAILURE);
	}
	

	// Read the knock knock from the server
	con = malloc(sizeof(struct connection));
    con->addr_len = sizeof(struct sockaddr_storage);
	//con->addr = (struct sockaddr_storage)(*(addr->ai_addr));
	con->fd = sock;
	//char* response = getResponse(con);
	char buf[100];
	
	
	for(;;){
		recv(con->fd, buf, 100, 0);
		printf("Received from server: %s\n", buf);
		printf("Send message to server: \n");
		scanf("%s", buf);
		send(con->fd, buf, 100, 0);	
		}

	//TIME FOR OUR OWN SHIT
	


	// close the socket
	freeaddrinfo(address_list);
	close(sock);

	return EXIT_SUCCESS;	
}
