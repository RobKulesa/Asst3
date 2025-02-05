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
char* completeSetUpLine = "REG|4|Joe.|";
char* punchLine = "|Joe Mama!|";
char* completePunchLine = "REG|9|Joe Mama!|";
char* endStr = "end";
int debug = 0;
int server(char *port);
void *echo(void *arg);
char* getResponse(struct connection* c, int* msgCount);
char* geterrstr(int err, int msgcount);
int isGoodMessage(char* str, int msgCount);
int readErrorMessage(char* str);

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
            if(debug) printf("Sending KnockKnock\n");
            char* kk = "REG|13|Knock, knock.|";
            send(con->fd, kk, strlen(kk),0);
        }
        
        
        char* response;
        //printf("Message Count before calling getResponse is: %d\n", msgCount);
        //response = getResponse(con, &msgCount);
        //printf("Response is: %s and message count is %d\n", response, msgCount);
        
        for(lpCt = 0; lpCt < 3; lpCt++){
            response = getResponse(con, &msgCount);
            if(debug) printf("Reponse is: %s\n", response);
            if(response!=NULL){
                if(strcmp(endStr, response)!=0)
                    send(con->fd, response, strlen(response), 0);
                char a, b, c;
                a = response[0];
                b = response[1];
                c = response[2];
                if(debug) printf("a is: %c\tb is: %c\tc is %c\n", a , b, c);
                if(a == 'E' && b == 'R' && c == 'R') {
                    printf("Sending error message (shutting down...): %s\n", response);
                    shutdown(con->fd, SHUT_RDWR);
                    close(con->fd);
                    free(response);
                    free(con);
                    return 0;
                }
                free(response);
            } else {
                printf("Connection closed by remote host (shutting down...)\n");
                close(con->fd);
                free(con);
                return 0;
            }
            
        }
        close(con->fd);
        free(con);
    }
    // never reach here
    return 0;
}

char* getResponse(struct connection* c, int* msgCount) {
    if(debug) printf("Calling getresponse\n");
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
    int barCount = 0;
    int i;
    for(i = 0; i < 3 && (nread = recv(c->fd, buf, 1, 0) > 0); i++){
        //if(debug) printf("nread: %d\n", nread);
        buf[nread] = '\0';
        if(input == NULL) {
            input = malloc(strlen(buf)+1);
            strcpy(input, buf);
        } else {
            strcat(input, buf);
        }
        if(debug) printf("\t[%s:%s] got partial input as: %s\n", host, port, input);
    }
    if(input == NULL) {
        return NULL;
    }
    if(debug) printf("i is: %d\tinput[0] = %c\tinput[1] = %c\tinput[2] = %c\n", i, input[0], input[1], input[2]);
    if(i < 3 && !(input[0] == 'R' && input[1] == 'E' && input[2]== 'G') && !(input[0] == 'E' && input[1] == 'R' && input[2] == 'R')){
        if(debug) printf("We made it here\n");
        error = ERRFORMAT;
        ++(*msgCount);
    } else{
        int maxBarCount = 2;
        if((input[0] == 'R' && input[1] == 'E' && input[2]== 'G'))
            maxBarCount++;
        while((barCount < maxBarCount && (nread = recv(c->fd, buf, 1, 0)) > 0)) {
        //if(debug) printf("nread: %d\n", nread);
        buf[nread] = '\0';
        if(input == NULL) {
            input = malloc(strlen(buf)+1);
            strcpy(input, buf);
        } else {
            strcat(input, buf);
        }
        if(buf[0]=='|')
            barCount++;
        if(debug) printf("\t[%s:%s] got partial input as: %s\n", host, port, input);
        }
        ++(*msgCount);
        if(debug) printf("{getResponse} msgCount is: %d\n", *msgCount);
        if(debug) printf("{getResponse} [%s:%s] finished reading input as: %s\n", host, port, input);
        error = isGoodMessage(input, *msgCount);
    }
    
    // begin analysis on input
    
    if(debug) printf("{getResponse} input is code %d\n", error);
    
    char* response = NULL;
    if(error != 0) {
        if(error == ERRORMSG) {
            if(!readErrorMessage(input)){
                error = ERRFORMAT;
                response = geterrstr(error, *msgCount);
            }
            close(c->fd);
            free(input);
            printf("Received error msg from client! Shutting down...\n");
            return response;
        } else {
            //send error msg and exit! make sure to free response!
            response = geterrstr(error, *msgCount);
            free(input);
            return response;
        }
    }
    switch(*msgCount) {
        case 1:
            response = malloc(sizeof(completeSetUpLine)+1);
            strcpy(response, completeSetUpLine);
            //return response;
            break;
        case 2:
            response = malloc(sizeof(completePunchLine)+1);
            strcpy(response, completePunchLine);
            //return response;
            break;
        case 3:
            response = malloc(sizeof(endStr)+1);
            strcpy(response, endStr);
            break;
        default:
            response = NULL;
            break;
    }
    free(input);
    return response;


}

int isGoodMessage(char* str, int msgCount) {
    if(debug) printf("{isGoodMessage}: str is: %s\n", str);
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
    if(debug) printf("seq1, of length %ld, is: %s\n", strlen(seq1), seq1);

    if(strcmp(seq1, "ERR|") == 0) {
        free(seq1);
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
        if(debug) printf("seq2 int is: %d\n", seq2);
        if(seq2 < 1){
            free(seq1);
            free(seq2str);
            return ERRFORMAT;
        }
        
        char* seq3 = malloc(strlen(str) - strlen(seq1) - strlen(seq2str) + 1);
        if(debug) printf("strlen(str): %lu\tstrlen(seq1): %lu\tstrlen(seq2str): %lu\n", strlen(str), strlen(seq1), strlen(seq2str));
        memset(seq3, 'a', strlen(str) - strlen(seq1) - strlen(seq2str) + 1);
        seq3[strlen(str) - strlen(seq1) - strlen(seq2str)] = '\0';
        if(debug) printf("Index set to nullterminator: %lu\n", strlen(str) - strlen(seq1) - strlen(seq2str));
        if(debug) printf("seq 3 is: %s\t before assignment &str[strlen(seq1) + strlen(seq2str)] is: %s\n", seq3, &str[strlen(seq1) + strlen(seq2str)]);
        strncpy(seq3, &str[strlen(seq1) + strlen(seq2str)], strlen(str) - strlen(seq1) - strlen(seq2str));
        if(debug) printf("seq 3 is: %s\n", seq3);
        if(!(seq3[0] == '|' && seq3[strlen(seq3) - 1] == '|')) {
            free(seq1);
            free(seq2str);
            free(seq3);
            return ERRFORMAT;
        }
        if(debug) printf("seq3, of length %ld, is: %s\n", strlen(seq3), seq3);
        if(strlen(seq3) != seq2 + 2) {
            free(seq1);
            free(seq2str);
            free(seq3);
            return ERRLENGTH;
        }
        free(seq1);
        free(seq2str);
        //seq3 is the string with message content!
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
                if((strlen(seq3) <= 3) || (seq3[strlen(seq3) - 2] != '.' && seq3[strlen(seq3) - 2] != '?' && seq3[strlen(seq3) - 2] != '!' )) {
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
    char* errStr= (char*)malloc(10);
    errStr[0] = 'E';
    errStr[1] = 'R';
    errStr[2] = 'R';
    errStr[3] = '|';
    errStr[4] = 'M';
    errStr[8] = '|';
    errStr[9] = '\0';
    switch(msgcount){
        case 1:
            errStr[5] = '1'; break;
        case 2:
            errStr[5] = '3'; break;
        case 3:
            errStr[5] = '5'; break;
        default:
            errStr[5] = 'e'; break;
    }
    switch(err) {
        case ERRLENGTH:
            //return "LN";
            errStr[6] = 'L';
            errStr[7] = 'N';
            break;
        case ERRCONTENT:
            //return "CT";
            errStr[6] = 'C';
            errStr[7] = 'T';
            break;
        case ERRFORMAT:
            //return "FT";
            errStr[6] = 'F';
            errStr[7] = 'T';
            break;
        default:
            //return "FT";
            errStr[6] = 'P';
            errStr[7] = 'P';
            break;
    }
    if(debug) printf("{errStr}: strlen is: %lu\terrStr: %s\n", strlen(errStr), errStr);
    return errStr;
}

int readErrorMessage(char* str){
    if(debug) printf("{readErrorMessage} was called: %s\n", str);
    if(debug) printf("{readErrorMessage}: Info: strlen: %lu\t str[4] = %c\n", strlen(str), str[4]);
    if(debug) printf("\tstr[5] = %c\tstr[8] = %c\n", str[5], str[8]);
    if(strlen(str) != 9){
        if(debug) printf("{readErrorMessage} strlen was incorrect\n");
    }   
    if(str[4]!= 'M' || !isdigit(str[5]) || str[8] != '|')
        return 0;
    else if(str[5]!= '0' && str[5]!= '1' && str[5]!= '2' && str[5]!= '3' && str[5]!= '4' && str[5]!= '5')
        return 0;
    else if(str[6]=='C' && str[7]== 'T'){
        printf("message %d content was not correct\n", str[5] - '0');
        return 1;
    } else if(str[6] == 'L' && str[7]=='N'){
        printf("message %d length value was incorrect\n", str[5] - '0');
        return 1;
    }else if(str[6] == 'F' && str[7]=='T'){
        printf("message %d format was broken\n", str[5] - '0');
        return 1;
    } else{
        if(debug) printf("Why are we here lmfao\n");
        return 0;
    }
}