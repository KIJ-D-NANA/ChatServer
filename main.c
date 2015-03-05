#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>

typedef struct Client{
    char Name[256];
    int sockfd;
    struct Client* Next;
    struct Client* Previous;
}Client;

void* SomeAwesomeThings(void* Param){
    Client* theClient = (Client*)Param;
    char buffer[256];
    char message[2048];

    memset(buffer, '\0', sizeof(buffer));
    memset(message, '\0', sizeof(message));

    int msgSize;
    while(1){
        while((msgSize = readv(theClient->sockfd, buffer, sizeof(buffer) - 1)) > 0){
            buffer[msgSize] = '\0';
            strcat(message, buffer);
        }
        if(msgSize == -1){
            printf("%s is disconnected\n", theClient->Name);
            break;
        }
        else{
            //Do things here
        }
    }

    if(theClient == head){
        head = theClient->Next;
    }
    else{
        theClient->Previous->Next = theClient->Next;
    }
    free(theClient);

    return NULL;
}

Client* head;
Client* tail;

int main(int argc, char **argv)
{
    int portNum;
    int listenfd = 0, connfd = 0;
    char buffer[50];
    char name[256];

    int msgSize;

    head = tail = NULL;

    memset(buffer, '\0', sizeof(buffer));
    memset(name, '\0', sizeof(name));

    if(argc > 2){
        portNum = atoi(argv[2]);
    }
    else{
        portNum = 5050;
    }

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    printf("Socket retrival success\n");

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(portNum);

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(sockaddr));

    if(listen(listenfd, 10) == -1){
        printf("Failed to listen");
        return -1;
    }

    while(1){
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        Client* newClient = (Client*) malloc(sizeof(Client));
        newClient->sockfd = connfd;

        while((msgSize = readv(connfd, buffer, sizeof(buffer) - 1)) > 0){
            buffer[msgSize] = '\0';
            strcat(name, buffer);
        }
        strcpy(newClient->Name, name);
        name[0] = '\0';

        if(head == NULL){
            head = newClient;
            newClient->Previous = NULL;
        }
        else{
            tail->Next = newClient;
        }
        newClient->Previous = tail;
        tail = newClient;
        newClient->Next = NULL;

        pthread_t clientThread;
        pthread_create(&clientThread, NULL, SomeAwesomeThings, (void*)newClient);
    }

    return 0;
}

