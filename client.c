#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>
#include "CRC/crc.h"

#define BIT_ERROR_RATE 0.1
#define MAX_PCK_SIZE 100

//function that generates a random string
char* randomString(){
    srand(time(0));
    int n = rand()%30 + 20;
    char* str = malloc(n);
    memset(str,'0',n);
    for(int i = 0; i < n; i++)
        str[i] = rand()%2 + '0';
        
    str[n] = '\0';
    return str;
}

int main(int argc, char* argv[]){

    int PORT;
    int sock = 0;
    struct timeval tv = {10,0};

    if(argc != 3)
    {
        printf("Please enter : <executable code><Server IP Address><Server Port number>\n");
        exit(0);
    }
    else
    {
        //printf("Host : %s Port : %s\n",argv[1],argv[2]);
        PORT = atoi(argv[2]);
    }
    int server_port, socket_fd;
    struct hostent *server_host;
    struct sockaddr_in server_address;

    if((sock = socket(AF_INET, SOCK_STREAM,0)) < 0)
    {
        printf("Socket creation error\n");
        return -1;
    }

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    memset(&server_address,'0',sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    if(inet_pton(AF_INET, argv[1], &server_address.sin_addr) <=0)
    {
        printf("Invalid address.. Address not supported\n");
        return -1;
    }

    if(connect(sock,(struct sockaddr*)&server_address, sizeof(server_address)) < 0)
    {
        printf("Connection Failed\n");
        return -1;
    }

    printf("\nConnection with server %s (port %s) established\n", argv[1], argv[2]);

    int i;
    for(i = 1; i <= 5; i++)
    {
    
    	printf("\niteration %d\n", i);
    	
    	//Generating raw message
    	char* orgStr = randomString();
    	printf("\nRaw message = %s\n", orgStr);
    	
    	//Generating T(x)
    	char* crcStr = getTx(orgStr);
    	printf("T(x) = %s ", orgStr);

    	printf("%s\n", crcStr+strlen(orgStr));


    	char buff[MAX_PCK_SIZE];
    	while(1)
    	{
        	sleep(3);
        	char* Tx = malloc(strlen(crcStr)+100);
        	strcpy(Tx, crcStr);

        	if((double)rand()/RAND_MAX < BIT_ERROR_RATE)
        	{
        	    int ind = rand()%strlen(Tx);
        	    Tx[ind] = (Tx[ind] == '1') ? '0' : '1';
        	    printf("\nintroducing bit error, new Tx = %s\n", Tx);
        	}

        	send(sock, Tx, strlen(Tx), 0);
        	printf("\nMessage sent\n");
        	int count = read(sock, buff, MAX_PCK_SIZE);
        
        	if(count == -1)
        	{
        	    printf("\ntimeout, retransmitting......\n");
        	    continue;
        	}
        
        	int valid = checkCRC(buff);
        	
        	if(!valid)
        	{
        		printf("\nretransmitting.......\n");
            		continue;
        	}
        
        	if(buff[0] == '0')
        	{
            		printf("\nNACK received, retransmitting....\n");
            		continue;
        	}

        	else
            	printf("\nACK received......\n");
        
        	break;
    	}
    }
    close(sock);
}
