#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>

#define BACKLOG 5

struct connection {
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int fd;
};

int server(char *port);
void *echo(void *arg);
char* getResponse(struct connection* c);


/*
 * ASS3 Server Algorithm
 * 1. Analyze the amount of arguments
 *      IF 1 -> error (not enough args)
 *      IF 2 -> proceed (normal non-Extra-credit)
 *      IF 3 -> proceed (extra credit assignment)
 *      IF 4 -> error (too many arguments)
 * 2. Analyze Port Number Argument
 *      IF invalid-> Error
 *      IF valid ->  Continue
 * 3. Set up listening socket to detect a connection from a client
 * 4. Upon receiving a connection, set up a socket to send "REG|13|Knock, knock." to the client
 * 5. Set up listening socket(s) to read bytes from the client until a valid message is read or it is detected to be invalid
 *      MUST BE: |REG|12|Who's there?|
 * 6. Send an appropriate response
 *      ERR|<error code>| if error
 *      REG|<jokelen>|<joke start|
 * 7. Read a response from client
 * 8. Send the punchline
 * 9. Receive the the remark of disgust
 * 10. End
 */
int main(int argc, char **argv) {
	if (argc <= 1 || argc >=4) {
		printf("Error: Invalid argument count\n");
		return EXIT_FAILURE;
	}
    int portNum = atoi(argv[1]);
    if(portNum <= 5000 || portNum >= 65536){
        printf("Error: Invalid Port Number\n");
		return EXIT_FAILURE;
    }
    (void) server(argv[1]);
    return EXIT_SUCCESS;
}


int server(char *port) {
    struct addrinfo hint, *address_list, *addr;
    struct connection *con;
    int error, sfd;
    pthread_t tid;

    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;
    
    error = getaddrinfo(NULL, port, &hint, &address_list);
    if (error != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }

    // attempt to create socket
    for (addr = address_list; addr != NULL; addr = addr->ai_next) {
        sfd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        
        // if we couldn't create the socket, try the next method
        if (sfd == -1) {
            continue;
        }

        // if we were able to create the socket, try to set it up for
        // incoming connections;
        if ((bind(sfd, addr->ai_addr, addr->ai_addrlen) == 0) &&
            (listen(sfd, BACKLOG) == 0)) {
            break;
        }

        // unable to set it up, so try the next method
        close(sfd);
    }

    if (addr == NULL) {
        // we reached the end of result without successfuly binding a socket
        fprintf(stderr, "Could not bind\n");
        return -1;
    }

    freeaddrinfo(address_list);

    // at this point sfd is bound and listening
    printf("Waiting for connection\n");
    
    //!AARONS TEST CODE STARTS HERE
    struct sockaddr_storage address;
    socklen_t address_len;
    int clientSFD;
    for(;;){
        address_len = sizeof(struct sockaddr_storage);
        clientSFD = accept(sfd, (struct sockaddr*) &address, &address_len);
        if(clientSFD == -1){
            perror("accept");
            continue;
        } else{
            //TODO: SEND KNOCK KNOCK BY USING THE SOCKETFD WE UST GOT
            continue;
        } 
    }
    //!AARONS TEST CODE ENDS HERE
    for (;;) {
    	// create argument struct for child thread
		con = malloc(sizeof(struct connection));
        con->addr_len = sizeof(struct sockaddr_storage);
        
        // wait for an incoming connection
        con->fd = accept(sfd, (struct sockaddr *) &con->addr, &con->addr_len);
        
        // if we got back -1, it means something went wrong
        if (con->fd == -1) {
            perror("accept");
            continue;
        }
        char* output;

        getResponse(con);
    }

    // never reach here
    return 0;
}

void *echo(void *arg) {
    char host[100], port[10], buf[101];
    struct connection *c = (struct connection *) arg;
    int error, nread;

	// find out the name and port of the remote host
    error = getnameinfo((struct sockaddr *) &c->addr, c->addr_len, host, 100, port, 10, NI_NUMERICSERV);

    if (error != 0) {
        fprintf(stderr, "getnameinfo: %s", gai_strerror(error));
        close(c->fd);
        return NULL;
    }

    printf("[%s:%s] connection\n", host, port);

    while ((nread = read(c->fd, buf, 100)) > 0) {
        buf[nread] = '\0';
        printf("[%s:%s] read %d bytes |%s|\n", host, port, nread, buf);
    }

    printf("[%s:%s] got EOF\n", host, port);

    close(c->fd);
    free(c);
    return NULL;
}

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