#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "CRC/crc.h"


#define MAX_REQ 100
#define MAX_PCK_SIZE 100
#define BIT_ERROR_RATE 0.1
#define PCK_LOSS_RATE 0.1

//Structure to pass arguments to thread
typedef struct pthread_arg_t 
{
    int socket_fd;
    struct sockaddr_in clientAddr;
} pthread_arg_t;


void* verifier(void*);
void signal_handler(int);

//server file descriptor
int serverSocket;


int main(int argc, char* argv[])
{
	
    if(argc != 2)
    {
        printf("Please enter : <executable code><Server Port number>\n");
        exit(0);
    }

    // Creating socket file descriptor
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    int newSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int port = atoi(argv[1]);
    socklen_t addr_size;

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,&opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    pthread_t pthread;
    pthread_attr_t pthread_attr;
    pthread_arg_t *pthread_arg;

    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    //binds the socket to the address and port number specified in serverAddr		
    if(bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr))<0)
    {
        perror("bind failed\n");
        exit(EXIT_FAILURE);
    }

    //Handling SIGINT signal to close the socket
    if (signal(SIGINT, signal_handler) == SIG_ERR) 
    {
        perror("signal failed");
        exit(EXIT_FAILURE);
    }

    //setting pthread state to be detached
    if(pthread_attr_init(&pthread_attr) != 0) 
    {
        perror("pthread_attr_init error\n");
        exit(EXIT_FAILURE);
    }

    if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0) 
    {
        perror("pthread_attr_setdetachstate error");
        exit(EXIT_FAILURE);
    }

    if(listen(serverSocket, MAX_REQ) != 0)
    {
        perror("listen error\n");
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        pthread_arg = (pthread_arg_t *)malloc(sizeof *pthread_arg);

        if (!pthread_arg) 
        {
            perror("malloc failed\n");
            continue;
        }

        addr_size = sizeof pthread_arg->clientAddr;

        newSocket = accept(serverSocket, (struct sockaddr *)&pthread_arg->clientAddr, &addr_size);

        if (newSocket == -1) 
        {
            perror("accept failed\n");
            free(pthread_arg);
            continue;
        }

        printf("\nConnection with client %d established\n", newSocket);
        pthread_arg->socket_fd = newSocket;

        if(pthread_create(&pthread, &pthread_attr, verifier, (void *)pthread_arg) != 0) 
        {
            perror("pthread_create fail\n");
            free(pthread_arg);
            continue;
        }
    }

}

void signal_handler(int signal_number) 
{
    printf("\nTerminating gracefully..\n");
    if(serverSocket > 0)
        close(serverSocket);
    exit(0);
}


//Function to handle each thread routine
void* verifier(void* arg)
{
    srand(time(0));
    pthread_arg_t *pthread_arg = (pthread_arg_t *)arg;
    int socket_fd = pthread_arg->socket_fd;
    struct sockaddr_in clientAddr = pthread_arg->clientAddr;
    free(arg);

    char* buff = malloc(MAX_PCK_SIZE);

    //reading from the server
    while(read(socket_fd, buff, MAX_PCK_SIZE) > 0)
    {
        printf("\nreceived ");
        
        //printing the received data
        for(int i = 0; i < strlen(buff) ; i++)
           printf("%c", buff[i]);
        
        //CRC check on received data
        int valid = checkCRC(buff); 
        char* Tx, *msg;
        
        //Generating packet loss
        if((double)rand()/RAND_MAX < PCK_LOSS_RATE)
        {
            printf("\nsimulating packet loss (No ACK/NACK sent) to client socket %d\n", socket_fd);
            continue;
        }
	
	//sending ACK/NACK accordingly
        if(valid)
        {
            printf("\nsending ACK to client socket %d\n", socket_fd);
            msg = "1";
        }
        else
        {
            printf("\nsending NACK to client socket %d\n", socket_fd);
            msg = "0";   
        }
        
        //Applying CRC on ACK/NACK
        Tx = getTx(msg);
    
        //Introducing bit error in acknowledgement to client socket
        if((double)rand()/RAND_MAX < BIT_ERROR_RATE)
        {
            printf("\nintroducing bit error in acknowledgement to client socket %d\n", socket_fd);
            Tx[0] = (Tx[0] == '1') ? '0' : '1';
        }

        sleep(3);
	
	//sending the ACK/NACK to the client
        send(socket_fd, Tx, strlen(Tx), 0);
        
        //Clearing the buffer
        free(buff);
        buff = malloc(MAX_PCK_SIZE);
        memset(buff,'\0',MAX_PCK_SIZE);

    }
    
    //Connection terminates after client connection is completed
    printf("\nConnection with client %d terminated\n", socket_fd);
    close(socket_fd);

    return 0;
 
    
}

