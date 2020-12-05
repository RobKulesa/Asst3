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
    char host[100], port[10], buf[101];
    int error, nread;

	// find out the name and port of the remote host
    error = getnameinfo((struct sockaddr *) &c->addr, c->addr_len, host, 100, port, 10, NI_NUMERICSERV);
    if (error != 0) {
        fprintf(stderr, "getnameinfo: %s", gai_strerror(error));
        close(c->fd);
        return NULL;
    }
    
    char* input = NULL;
    while((nread = read(c->fd, buf, 100)) > 0) {
        buf[nread] = '\0';
        if(input == NULL) {
            input = malloc(strlen(buf)+1);
            strcpy(input, buf);
        } else {
            strcat(input, buf);
        }
    }
//    printf("[%s:%s] finished reading input as: %s\n", host, port, input);
    // initialize error booleans
    int ct, ln, ft;
    // begin analysis on input
    // seq1 contains 
    int i;
    char* seq1 = malloc(4);
    
    
    char* response;
    close(c->fd);
    free(input);
    free(c);
    return response;
}



int main(int argc, char **argv) {
	struct addrinfo hints, *address_list, *addr;
	int error;
	int sock;
	int i;
	struct connection *con;
	
	if (argc < 4) {
		printf("Usage: %s [host] [port] [message(s)...]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// we need to provide some additional information to getaddrinfo using hints
	// we don't know how big hints is, so we use memset to zero out all the fields
	memset(&hints, 0, sizeof(hints));
	
	// indicate that we want any kind of address
	// in practice, this means we are fine with IPv4 and IPv6 addresses
	hints.ai_family = AF_UNSPEC;
	
	// we want a socket with read/write streams, rather than datagrams
	hints.ai_socktype = SOCK_STREAM;

	// get a list of all possible ways to connect to the host
	// argv[1] - the remote host
	// argv[2] - the service (by name, or a number given as a decimal string)
	// hints   - our additional requirements
	// address_list - the list of results

	error = getaddrinfo(argv[1], argv[2], &hints, &address_list);
	if (error) {
		fprintf(stderr, "%s", gai_strerror(error));
		exit(EXIT_FAILURE);
	}

	
	// try each of the possible connection methods until we succeed
	for (addr = address_list; addr != NULL; addr = addr->ai_next) {
		// attempt to create the socket
		sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
		
		// if we somehow failed, try the next method
		if (sock < 0) continue;
		
		// try to connect to the remote host using the socket
		if (connect(sock, addr->ai_addr, addr->ai_addrlen) == 0) {
			// we succeeded, so break out of the loop
			break;
		}

		// we weren't able to connect; close the socket and try the next method		
		close(sock);
	}
	
	// if we exited the loop without opening a socket and connecting, halt
	if (addr == NULL) {
		fprintf(stderr, "Could not connect to %s:%s\n", argv[1], argv[2]);
		exit(EXIT_FAILURE);
	}
	
	// now that we have connected, we don't need the addressinfo list, so free it
	freeaddrinfo(address_list);

	// Read the knock knock from the server
	con = malloc(sizeof(struct connection));
    con->addr_len = sizeof(struct sockaddr_storage);
	con->fd = sock;
	char* response = getResponse(con);
	if(response!=NULL)
		printf("Response received: %s\n", response);
	

	//Send |REG|12|Who's there?|
	printf("Please send: |REG|12|Who's there?|\tIf you fail to do so, the server will send an error\n");
	char* userInpt;
	scanf("%s", userInpt);
    
	write(sock, userInpt, strlen(userInpt));
	


	//TIME FOR OUR OWN SHIT
	


	// close the socket
	close(sock);

	return EXIT_SUCCESS;	
}
