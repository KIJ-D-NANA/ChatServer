#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include "rc4encryption.h"
#include "RSACrypto.h"

#define KEY_LENGTH 2048
#define PUB_EXP 3
#define PRINT_KEYS

typedef struct Client{
	char Name[256];
	int sockfd;
	char public_key[2048];
	RSA* keypair;
	struct Client* Next;
	struct Client* Previous;
}Client;

Client* head;
Client* tail;
char *public_key;
char *private_key;

void InitRSA();
int CheckHashValidation(size_t input_len, unsigned char* raw, char* hash_value);

void* SomeAwesomeThings(void* Param){
	Client* theClient = (Client*)Param;
	char message[4086];
	char sendMessage[4086];
	char encrypt[4086];
	char* receiver;
	char* tmp;
	char* err;
	Client* ClientCounter;
	int msgSize;
	int usernameSet = 0;
	int nameLength;
	int decrypt_len;
	int encrypt_len;

	//RC4 component
	RC4Container RC4key;
	int iter1;
	int iter2;
    RC4key.iter1 = &iter1;
    RC4key.iter2 = &iter2;
	int RC4KeySet = 0;
	//
	unsigned char hash_out[SHA_DIGEST_LENGTH];
	char hash_string[SHA_DIGEST_LENGTH * 2 + 1];
	int iterator;
	memset(sendMessage, '\0', sizeof(sendMessage));
	err = (char*) malloc(130);

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
		else if((strcmp(message, "Mode: Private") == 0) || (strcmp(message, "Mode: InitPriv") == 0) || (strcmp(message, "Mode: AccPriv") == 0) ){

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
				if(RC4KeySet == 1){
					printf("Setting username\n");
					nameLength = strstr(tmp, "\r\n.\r\n") - tmp;
					encrypt_len = RC4Crypt(nameLength, (unsigned char*)tmp, (unsigned char*)encrypt, &RC4key);
					tmp = strstr(encrypt, "\r\n.,\r\n");
					nameLength = tmp - encrypt;
					*tmp = '\0';
					tmp = tmp + 6;

					if(CheckHashValidation(nameLength, (unsigned char*)encrypt, tmp) == 1){
						for(msgSize = 0; msgSize < nameLength; msgSize++){
							theClient->Name[msgSize] = *(encrypt + msgSize);
						}
						theClient->Name[nameLength] = '\0';
						printf("%s\n", theClient->Name);
						usernameSet++;
					}
				}
			}
		}
		else if(strcmp(message, "Mode: GetCA") == 0){
			sprintf(sendMessage, "Mode: ServCA\r\n");
			strcat(sendMessage, public_key);
			strcat(sendMessage, "\r\n.\r\n");

			write(theClient->sockfd, sendMessage, sizeof(sendMessage));
		}
		else if(strcmp(message, "Mode: SetPubKey") == 0){
			tmp = tmp + 2;
			nameLength = strstr(tmp, "\r\n.\r\n") - tmp;
			if(RC4KeySet == 1){
				encrypt_len = RC4Crypt(nameLength, (unsigned char*)tmp, (unsigned char*)encrypt, &RC4key);
				encrypt[encrypt_len] = '\0';
				tmp = strstr(encrypt, "\r\n.,\r\n");
				nameLength = tmp - encrypt;
				*tmp = '\0';
				tmp = tmp + 6;
				if(CheckHashValidation(nameLength, (unsigned char*)encrypt, tmp) == 1){
					for(iterator = 0; iterator < nameLength; iterator++){
						*(theClient->public_key + iterator) = *(encrypt + iterator);
					}
					*(theClient->public_key + nameLength) = '\0';
				}
				else{
					//TODO: Return some error
				}
			}
		}
		else if(strcmp(message, "Mode: SetRC4Key") == 0){
			if(RC4KeySet == 0){
				tmp = tmp + 2;
				nameLength = strstr(tmp, "\r\n.\r\n") - tmp;
				if((encrypt_len = private_decrypt((unsigned char*)tmp, nameLength, (unsigned char*)private_key, (unsigned char*)encrypt, RSA_PKCS1_OAEP_PADDING)) == -1){
					ERR_load_crypto_strings();
					ERR_error_string(ERR_get_error(), err);
					fprintf(stderr, "Error decrypting message: %s\n", err);
					sprintf(sendMessage, "Mode: FailRC4Key\r\n.\r\n");
					write(theClient->sockfd, sendMessage, sizeof(sendMessage));
				}
				else{
					tmp = strstr(encrypt, "\r\n.,\r\n");
					nameLength = tmp - encrypt;
					*tmp = '\0';
					tmp = tmp + 6;
					//TODO: Check if hash of encrypt is the same with tmp
					if(CheckHashValidation(nameLength, (unsigned char*)encrypt, tmp) == 1){
						initRC4key(&RC4key, encrypt, nameLength);
						RC4KeySet = 1;
					}
					else{
						sprintf(sendMessage, "Mode: FailRC4Key\r\n.\r\n");
						write(theClient->sockfd, sendMessage, sizeof(sendMessage));
					}
				}
				else{
					sprintf(sendMessage, "Mode: FailRC4Key\r\n.\r\n");
					write(theClient->sockfd, sendMessage, sizeof(sendMessage));
				}
			}
		}
		else if(strcmp(message, "Mode: GetPubKey") == 0){
			tmp = tmp + 2;
			tmp = strstr(tmp, " ") + 1;
			receiver = tmp;
			tmp = strstr(tmp, "\r\n");
			*tmp = '\0';
			for(ClientCounter = head; ClientCounter != NULL; ClientCounter = ClientCounter->Next){
				if(strcmp(receiver, ClientCounter->Name) == 0)
					break;
			}
			if(ClientCounter != NULL){
				if(ClientCounter->public_key[0] != '\0' && RC4KeySet == 1){
					SHA1((unsigned char*)ClientCounter->public_key, strlen(ClientCounter->public_key), (unsigned char*)hash_out);
                    for(iterator = 0; iterator < SHA_DIGEST_LENGTH; iterator++){
                        sprintf(&hash_string[iterator * 2], "%02x", (unsigned int)hash_out[iterator]);
					}
					sprintf(sendMessage, "%s\r\n.,\r\n%s", ClientCounter->public_key, hash_string);
					encrypt_len = RC4Crypt(strlen(sendMessage), (unsigned char*)sendMessage, (unsigned char*)encrypt, &RC4key);
					encrypt[encrypt_len] = '\0';
					sprintf(sendMessage, "Mode: ClientPubKey\r\nUser: %s\r\n%s\r\n.\r\n", ClientCounter->Name, encrypt);
					write(theClient->sockfd, sendMessage, sizeof(sendMessage));
				}
				else{
					sprintf(sendMessage, "Mode: FailPubKey\r\nUser: %s\r\n.\r\n", ClientCounter->Name);
				}
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
	if(theClient->keypair != NULL)
		RSA_free(theClient->keypair);
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
		newClient->public_key[0] = '\0';
		strcpy(newClient->Name, "");
		newClient->keypair = NULL;

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
	/*RSA_free(keypair);*/
	free(public_key);
	free(private_key);

	return 0;
}

void InitRSA(){
	//Generating RSA key
	size_t file_size;
	FILE* private_key_file = fopen("./private.pem", "r");
	FILE* public_key_file = fopen("./public.pem", "r");

	fseek(private_key_file, 0, SEEK_END);
	file_size = ftell(private_key_file);
	fseek(private_key_file, 0, SEEK_SET);
	private_key = (char*)malloc(file_size + 1);
	file_size = fread(private_key, 1, file_size, private_key_file);
	fclose(private_key_file);


	fseek(public_key_file, 0, SEEK_END);
	file_size = ftell(public_key_file);
	fseek(public_key_file, 0, SEEK_SET);
	public_key = (char*)malloc(file_size + 1);
	file_size = fread(public_key, 1, file_size, public_key_file);
	fclose(public_key_file);
}

int CheckHashValidation(size_t input_len, unsigned char* raw, char* hash_value){
	//TODO: Check if hash of encrypt is the same with tmp
	char hash_out[20];
	char hash_string[SHA_DIGEST_LENGTH * 2 + 1];
    SHA1(raw, input_len, (unsigned char*)hash_out);
	int i;
	int hash_valid;
	for(i = 0; i < SHA_DIGEST_LENGTH; i++){
		sprintf(&hash_string[i * 2], "%02x", (unsigned int)hash_out[i]);
	}
	hash_valid = 1;
	for(i = 0; i < SHA_DIGEST_LENGTH * 2; i++){
		if(hash_string[i] != hash_value[i]){
			hash_valid = 0;
			break;
		}
	}
	return hash_valid;
}
