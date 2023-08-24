#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "crc.h"
    
char* modDivRemainder(char* rawMessage, char* key){
    int msgLength = strlen(rawMessage), keyLength = strlen(key);
    char* rem = (char*) malloc(keyLength);
    char quot[msgLength];
    char temp[keyLength];
    char pp[keyLength];

    for (int i=0; i < keyLength; i++)
    	temp[i] = rawMessage[i];

    for (int i=0; i<msgLength-keyLength+1; i++) {
    	quot[i]=temp[0];
    	if(quot[i]=='0')
    		for (int j=0; j<keyLength; j++)
    		    pp[j] = '0'; 
        else
    		for (int j=0; j<keyLength; j++)
    		    pp[j] = key[j];
    	
        for (int j=keyLength-1;j>0;j--) {
    		if(temp[j]==pp[j])
    	    	rem[j-1] = '0';
            else
    		    rem[j-1] = '1';
    	}
    	
        rem[keyLength-1]=rawMessage[i+keyLength];
    	strcpy(temp,rem);
    }
    strcpy(rem,temp);
    return rem;
}

char* getTx(char* rawMessage){
    int msgLen = strlen(rawMessage);
    char* key = "100000111";
    char* msgCopy = malloc(strlen(rawMessage) + 9);
    memset(msgCopy,0,sizeof(msgCopy));
    strcpy(msgCopy, rawMessage);
    for(int i = 0; i < 8; i++){
        msgCopy[i+msgLen] = '0';
    }
    char* rem = modDivRemainder(msgCopy, key);
    for(int i = 0; i < 8; i++){
        msgCopy[i+msgLen] = rem[i];
    }
    msgCopy[msgLen+9] = '\0';
    return msgCopy;
}

int checkCRC(char* Tx){
    char* key = "100000111";
    char* rem = modDivRemainder(Tx, key);
    for(int i = 0; i < strlen(rem); i++){
        if(rem[i] != '0'){
            printf("\nbit error encountered, remainder = %s\n", rem);
            return 0;
        }
    }
    return 1;
}

// int main(){
//     char* m = "1011001100001010001111";
//     char* key = "10011";
//     printf("%s\n", getTx(m));
// }
