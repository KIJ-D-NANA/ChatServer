#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#define KEY_LENGTH 2048
#define PUB_EXP 3
#define PRINT_KEYS

typedef struct Client{
    char Name[256];
    int sockfd;
    struct Client* Next;
    struct Client* Previous;
}Client;

Client* head;
Client* tail;
RSA *keypair;
char *public_key;
size_t public_len;

void InitRSA();

void* SomeAwesomeThings(void* Param){
    Client* theClient = (Client*)Param;
    char message[2048];
    char sendMessage[2048];
    char* receiver;
    char* tmp;
    Client* ClientCounter;
    int msgSize;
    int usernameSet = 0;
    int nameLength;

    memset(sendMessage, '\0', sizeof(sendMessage));


//    if((msgSize = read(theClient->sockfd, message, sizeof(message) - 1)) > 0){
//        message[msgSize] = '\0';
//        int nameLength = strstr(message, "\r\n.\r\n") - message;
//        for(msgSize = 0; msgSize < nameLength; msgSize++){
//            theClient->Name[msgSize] = message[msgSize];
//        }
//        theClient->Name[nameLength] = '\0';
//        printf("%s has connected\n", theClient->Name);
//    }


    while((msgSize = read(theClient->sockfd, message, sizeof(message) - 1)) > 0){

        message[msgSize] = '\0';
        printf("%s", message);

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
        else if(strcmp(message, "Mode: Username") == 0){
            tmp = tmp + 2;
            if(usernameSet == 0){
                printf("Setting username\n");
                nameLength = strstr(tmp, "\r\n.\r\n") - tmp;
                for(msgSize = 0; msgSize < nameLength; msgSize++){
                    theClient->Name[msgSize] = *(tmp + msgSize);
                }
                theClient->Name[nameLength] = '\0';
                printf("%s\n", theClient->Name);
                usernameSet++;
            }
        }
    }

    printf("%s has been disconnected\n", theClient->Name);
    if(theClient == head){
        head = theClient->Next;
        if(head != NULL)head->Previous = NULL;
    }
    else{
        theClient->Previous->Next = theClient->Next;
        if(theClient == tail){
            tail = theClient->Previous;
        }
    }
    free(theClient);

    return NULL;
}

int main(int argc, char **argv)
{
	InitRSA();
	int portNum;
    int listenfd = 0, connfd = 0;

    head = tail = NULL;

    if(argc > 2){
        portNum = atoi(argv[2]);
    }
    else{
        portNum = 5050;
    }

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenfd == -1){
        printf("Socket retrival failed\n");
        return -1;
    }
    printf("Socket retrival success\n");

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(portNum);

    if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr)) == -1){
        printf("Failed to bind port\n");
        return -1;
    }

    if(listen(listenfd, 10) == -1){
        printf("Failed to listen");
        return -1;
    }

    while(1){
        connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        printf("New user has connected\n");
        Client* newClient = (Client*) malloc(sizeof(Client));
        newClient->sockfd = connfd;

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
	RSA_free(keypair);
	free(public_key);

    return 0;
}

void InitRSA(){
	//Generating RSA key
	size_t pri_len;
	size_t pub_len;
	char *pri_key;
	char *pub_key;
	char *err;
	printf("Generating RSA (%d bits) keypair...\n", KEY_LENGTH);
	fflush(stdout);
	keypair = RSA_generate_key_ex(KEY_LENGTH, PUB_EXP, NULL, NULL);

	// To get C-string PEM form
	BIO *pri = BIO_new(BIO_s_mem());
	BIO *pub = BIO_new(BIO_s_mem());

	PEM_write_bio_RSAPrivateKey(pri, keypair, NULL, NULL, 0, NULL, NULL);
	PEM_write_bio_RSAPublicKey(pub, keypair);

	pri_len = BIO_pending(pri);
	pub_len = BIO_pending(pub);

	pri_key = (char*) malloc(pri_len + 1);
	pub_key = (char*) malloc(pub_len + 1);

	BIO_read(pri, pri_key, pri_len);
	BIO_read(pub, pub_key, pub_len);

	pri_key[pri_len] = '\0';
	pub_key[pub_len] = '\0';
	public_key = pub_key;
	public_len = pub_len;

	#ifdef PRINT_KEYS
		printf("%s\n%s\n", pri_key, pub_key);
	#endif
	printf("done.\n");
	BIO_free_all(pri);
	BIO_free_all(pub);
	free(pri_key);
	//
}
