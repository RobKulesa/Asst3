#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <ctype.h>

#define BACKLOG 5
#define GOODMSG 0
#define ERRCONTENT 1
#define ERRLENGTH 2
#define ERRFORMAT 3
#define ERRORMSG 4
struct connection {
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int fd;
};
char* setupLine = "Joe";
char* setupLineWBars = "|Joe.|";
char* punchLine = "|Joe Mama!|";
int debug = 1;
int server(char *port);
void *echo(void *arg);
char* getResponse(struct connection* c, int* msgCount);
char* geterrstr(int err, int msgcount);
int isGoodMessage(char* str, int msgCount);

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
    /* struct sockaddr_storage address;
    socklen_t address_len;
    int clientSFD;
    char buf1[100];
    for(;;){
        address_len = sizeof(struct sockaddr_storage);
        clientSFD = accept(sfd, (struct sockaddr*) &address, &address_len);
        if(clientSFD == -1){
            perror("accept");
            continue;
        } else{
             //TODO: SEND KNOCK KNOCK BY USING THE SOCKETFD WE UST GOT
            char* kk = "Knock, knock.";
            send(clientSFD, kk, strlen(kk),0);
            printf("We want to receive buf now\n");

            while(recv(clientSFD, buf1, 99,0)>0){
                printf("Received: %s", buf1);
    
            }
	        
            
            
        } 
    } */
    
    //!AARONS TEST CODE ENDS HERE
    int msgCount = 0;
    int lpCt = 0;
    for (;;) {
        msgCount = 0;
		con = malloc(sizeof(struct connection));
        con->addr_len = sizeof(struct sockaddr_storage);
        con->fd = accept(sfd, (struct sockaddr *) &con->addr, &con->addr_len);
        if (con->fd == -1) {
            perror("accept");
            continue;
        }
        if(msgCount == 0){
            printf("Sending KnockKnock\n");
            char* kk = "REG|13|Knock, knock.|";
            send(con->fd, kk, strlen(kk),0);
        }
        
        
        char* response;
        for(lpCt = 0; lpCt < 4; lpCt++){
            getResponse(con, &msgCount);
        }
        
    }
    // never reach here
    return 0;
}


char* getResponse(struct connection* c, int* msgCount) {
    char host[100], port[10], buf[101];
    int error, nread;

	// find out the name and port of the remote host
    error = getnameinfo((struct sockaddr *) &c->addr, c->addr_len, host, 100, port, 10, NI_NUMERICSERV);
    if (error != 0) {
        fprintf(stderr, "getnameinfo: %s\n", gai_strerror(error));
        close(c->fd);
        return NULL;
    }
    
    char* input = NULL;
    while((nread = recv(c->fd, buf, 100, 0)) > 0) {
        buf[nread] = '\0';
        if(input == NULL) {
            input = malloc(strlen(buf)+1);
            strcpy(input, buf);
        } else {
            strcat(input, buf);
        }
    //    printf("\t[%s:%s] got partial input as: %s\n", host, port, input);
    }
    ++(*msgCount);
    printf("{getResponse} msgCount is: %d\n", *msgCount);
    printf("{getResponse} [%s:%s] finished reading input as: %s\n", host, port, input);
    
    // begin analysis on input
    error = isGoodMessage(input, *msgCount);
    printf("{getResponse} input is code %d\n", error);
    
    char* response = NULL;
    if(error != 0) {
        if(error == ERRORMSG) {
            close(c->fd);
            free(input);
            free(c);
            printf("{getResponse} Received error msg from client! Exiting...");
            return response;
        } else {
            //send error msg and exit! make sure to free response!
            response = geterrstr(error, *msgCount);
            return response;
        }
    }
    switch(*msgCount) {
        case 1:
            response = setupLineWBars;
            return response;
        case 2:
            response = punchLine;
            return response;
        case 3:
            //exit, client is done sending and we received valid A/D/S.
            return response;
        default:
            break;
    }

    //!Need to get rid of this once we done
    close(c->fd);
    free(input);
    free(c);
    close(c->fd);
    free(input);
    free(c);
    close(c->fd);
    free(input);
    free(c);
    close(c->fd);
    free(input);
    free(c);
    return response;
}

int isGoodMessage(char* str, int msgCount) {
    // seq1 should be REG or ERR, seq2 should be a positive integer, seq3 should be |string| of length seq2+2
    int i;
    int barCounter = 0;
    for(i = 0; i < strlen(str); ++i) {
        if(str[i] == '|') ++barCounter;
        if(barCounter > 3) return ERRFORMAT;
    }
    
    if(str[i-1] != '|') return ERRLENGTH;

    char* seq1 = malloc(5);
    memset(seq1, 'a', 5);
    seq1[4] = '\0';
    strncpy(seq1, str, strlen(seq1));
    printf("seq1, of length %ld, is: %s\n", strlen(seq1), seq1);

    if(strcmp(seq1, "ERR|") == 0) {
        return ERRORMSG;
    }
    if(strcmp(seq1, "REG|") == 0) {
        int i;
        int seq2len = 0;
        for(i = 4; i < strlen(str); ++i) {
            if(isdigit(str[i])) {
                ++seq2len;
            } else if(str[i] == '|') {
                break;
            } else {
                free(seq1);
                return ERRFORMAT;
            }
        }
        char* seq2str = malloc(seq2len + 1);
        memset(seq2str, 'a', seq2len + 1);
        seq2str[seq2len] = '\0';
        strncpy(seq2str, &str[strlen(seq1)], strlen(seq2str));
        int seq2 = atoi(seq2str);
        printf("seq2 int is: %d\n", seq2);
        
        char* seq3 = malloc(strlen(str) - strlen(seq1) - strlen(seq2str) + 1);
        memset(seq3, 'a', strlen(str) - strlen(seq1) - strlen(seq2str) + 1);
        seq3[strlen(str) - strlen(seq1) - strlen(seq2str)] = '\0';
        strncpy(seq3, &str[strlen(seq1) + strlen(seq2str)], seq2 + 2);
        if(!(seq3[0] == '|' && seq3[strlen(seq3) - 1] == '|')) {
            free(seq1);
            free(seq2str);
            free(seq3);
            return ERRFORMAT;
        }
        printf("seq3, of length %ld, is: %s\n", strlen(seq3), seq3);
        if(strlen(seq3) != seq2 + 2) {
            free(seq1);
            free(seq2str);
            free(seq3);
            return ERRLENGTH;
        }
        free(seq1);
        free(seq2str);
        //seq3 is the string with message content!
        int err = 0;
        switch(msgCount) {
            case 1: //|Who's there?|
                if(strcmp(seq3, "|Who's there?|") != 0) {
                    free(seq3);
                    return ERRCONTENT;
                }
                break;
            case 2:
                if(strcmp(seq3, "|Joe, who?|") != 0) {
                    free(seq3);
                    return ERRCONTENT;
                }
                break;
            case 3:
                if(strlen(seq3) < 4) {
                    free(seq3);
                    return ERRCONTENT;
                }
                if(seq3[strlen(seq3) - 2] != '.' && seq3[strlen(seq3) - 2] != '?' && seq3[strlen(seq3) - 2] != '!' ) {
                    free(seq3);
                    return ERRCONTENT;
                }
                break;
            default:
                break;   
        }
        free(seq3);
        return GOODMSG;
    }
    free(seq1);
    return ERRFORMAT;
}

char* geterrstr(int err, int msgcount) {
    if(debug) printf("executing geterrstr");
    char* errStr= (char*)malloc(5);
    errStr[0] = 'M';
    errStr[4] = '\0';
    switch(msgcount){
        case 1:
            errStr[1] = '1'; break;
        case 3:
            errStr[1] = '3'; break;
        case 5:
            errStr[1] = '5'; break;
        default:
            errStr[1] = '7'; break;
    }
    switch(err) {
        case ERRLENGTH:
            //return "LN";
            errStr[2] = 'L';
            errStr[3] = 'N';
            break;
        case ERRCONTENT:
            //return "CT";
            errStr[2] = 'C';
            errStr[3] = 'T';
            break;
        case ERRFORMAT:
            //return "FT";
            errStr[2] = 'F';
            errStr[3] = 'T';
            break;
        default:
            //return "FT";
            errStr[2] = 'P';
            errStr[3] = 'P';
            break;
    }
    return errStr;
}
