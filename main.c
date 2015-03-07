#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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

Client* head;
Client* tail;

void* SomeAwesomeThings(void* Param){
    Client* theClient = (Client*)Param;
    char buffer[256];
    char message[2048];
    char sendMessage[2048];
    char* receiver;
    char* tmp;
    Client* ClientCounter;

    memset(buffer, '\0', sizeof(buffer));
    memset(message, '\0', sizeof(message));
    memset(sendMessage, '\0', sizeof(sendMessage));

    int msgSize;
    while(1){
        message[0] = '\0';
        while((msgSize = read(theClient->sockfd, buffer, sizeof(buffer) - 1)) > 0){
            buffer[msgSize] = '\0';
            strcat(message, buffer);
        }
        if(msgSize == -1){
            printf("%s is disconnected\n", theClient->Name);
            break;
        }
        else{
            //Do things here
            tmp = strstr(message, "\r\n");
            *tmp = '\0';
            if(strcmp(message, "Mode: Public") == 0){

                tmp = tmp + 2;
                sprintf(sendMessage,"%s\r\nUser: %s\r\n%s", message, theClient->Name, tmp);

                for(ClientCounter = head; ClientCounter != NULL; ClientCounter = ClientCounter->Next){
                    if(ClientCounter == theClient) continue;
                    write(ClientCounter->sockfd, sendMessage, sizeof(sendMessage));
                }
            }
            else if(strcmp(message, "Mode: Private") == 0){

                tmp = tmp + 2;
                tmp = strstr(tmp, " ") + 1;
                receiver = tmp;
                tmp = strstr(tmp, "\r\n");
                *tmp = '\0';
                tmp = tmp + 2;

                sprintf(sendMessage,"%s\r\nUser: %s\r\n%s", message, theClient->Name, tmp);

                for(ClientCounter = head; ClientCounter != NULL; ClientCounter = ClientCounter->Next){
                    if(strcmp(receiver, ClientCounter->Name) == 0){
                        write(ClientCounter->sockfd, sendMessage, sizeof(sendMessage));
                        break;
                    }
                }
            }
            else if(strcmp(message, "Mode: GetList") == 0){
                sprintf(sendMessage, "Mode: List\r\n");
                for(ClientCounter = head; ClientCounter != NULL; ClientCounter = ClientCounter->Next){
                    if(ClientCounter == theClient)continue;
                    strcat(sendMessage, ClientCounter->Name);
                    strcat(sendMessage, "\r\n");
                }
                strcat(sendMessage, "\r\n.\r\n");

                write(theClient->sockfd, sendMessage, sizeof(sendMessage));
            }
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

int main(int argc, char **argv)
{
    int portNum;
    int listenfd = 0, connfd = 0;
    char buffer[50];
    char name[256];

    int msgSize;
    int nameLength;
    int i;

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

    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr));

    if(listen(listenfd, 10) == -1){
        printf("Failed to listen");
        return -1;
    }

    while(1){
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        Client* newClient = (Client*) malloc(sizeof(Client));
        newClient->sockfd = connfd;

        while((msgSize = read(connfd, buffer, sizeof(buffer) - 1)) > 0){
            buffer[msgSize] = '\0';
            strcat(name, buffer);
        }
        nameLength = strstr(name, "\r\n.\r\n") - name;

        for(i = 0; i < nameLength; i++){
            newClient->Name[i] = name[i];
        }

        newClient->Name[nameLength] = '\0';

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

