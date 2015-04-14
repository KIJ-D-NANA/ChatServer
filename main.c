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
#include <openssl/sha.h>
#include "rc4encryption.h"

#define KEY_LENGTH 2048
#define PUB_EXP 3
#define PRINT_KEYS

typedef struct Client{
	char Name[256];
	int sockfd;
	char* public_key;
	RSA* keypair;
	struct Client* Next;
	struct Client* Previous;
}Client;

Client* head;
Client* tail;
RSA *keypair;
char *public_key;
size_t public_len;

void InitRSA();
int CheckHashValidation(size_t input_len, unsigned char* raw, unsigned char* hash_value);

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
	key.iter1 = &iter1;
	key.iter2 = &iter2;
	int RC4KeySet = 0;
	//
	char hash_out[20];
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
					tmp = strstr(encrypt, "\r\n\r\n");
					nameLength = tmp - encrypt;
					*tmp = '\0';
					tmp = tmp + 4;

					if(CheckHashValidation(nameLength, (unsigned char*)encrypt, (unsigned char*)tmp) == 1){
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
				tmp = strstr(encrypt, "\r\n\r\n");
				nameLength = tmp - encrypt;
				*tmp = '\0';
				tmp = tmp + 4;
				if(CheckHashValidation(nameLength, (unsigned char*)encrypt, (unsigned char*)tmp) == 1){
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
				if((encrypt_len = RSA_private_decrypt(nameLength, (unsigned char*)tmp, (unsigned char*)encrypt, keypair, RSA_PKCS1_OAEP_PADDING)) == -1){
					ERR_load_crypto_strings();
					ERR_error_string(ERR_get_error(), err);
					fprintf(stderr, "Error decrypting message: %s\n", err);
					sprintf(sendMessage, "Mode: FailRC4Key\r\n.\r\n");
					write(theClient->sockfd, sendMessage, sizeof(sendMessage));
				}
				else{
					tmp = strstr(encrypt, "\r\n\r\n");
					nameLength = tmp - encrypt;
					*tmp = '\0';
					tmp = tmp + 4;
					//TODO: Check if hash of encrypt is the same with tmp
					if(CheckHashValidation(nameLength, (unsigned char*)encrypt, (unsigned char*)tmp) == 1){
						initRC4key(&RC4key, encrypt, nameLength);
						RC4KeySet = 1;
					}
					else{
						sprintf(sendMessage, "Mode: FailRC4Key\r\n.\r\n");
						write(theClient->sockfd, sendMessage, sizeof(sendMessage));
					}
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
				if(ClientCounter->public_key[0] != '\0'){
					if((encrypt_len = RSA_public_encrypt(strlen(ClientCounter->public_key), (unsigned char*)ClientCounter->public_key, (unsigned char*)encrypt, ClientCounter->keypair, RSA_PKCS1_OAEP_PADDING)) == -1){
						ERR_load_crypto_strings();
						ERR_error_string(ERR_get_error(), err);
						fprintf(stderr, "Error decrypting message: %s\n", err);
						sprintf(sendMessage, "Mode: FailPubKey\r\nUser: %s\r\n.\r\n", ClientCounter->Name);
					}
					else{
						encrypt_len = RSA_private_encrypt(encrypt_len, (unsigned char*)encrypt, (unsigned char*)message, keypair, RSA_PKCS1_PADDING);
						message[encrypt_len] = '\0';
						sprintf(sendMessage, "Mode: ClientPubKey\r\nUser: %s\r\n%s\r\n.\r\n", ClientCounter->Name, message);
						write(theClient->sockfd, sendMessage, sizeof(sendMessage));
					}
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
	free(theClient->public_key);
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
		newClient->public_key = (char*) malloc(2048);
		newClient->public_key[0] = '\0';

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

	FILE* private_key = fopen("./private.pem", "r");
	FILE* public_key = fopen("./public.pem", "r");
	PEM_read_RSAPrivateKey(private_key, &keypair, NULL, NULL);
	PEM_read_RSAPublicKey(public_key, &keypair, NULL, NULL);
	fclose(private_key);
	fclose(public_key);
	size_t pri_len;
	size_t pub_len;
	char *pri_key;
	char *pub_key;
	char *err;
	printf("Generating RSA (%d bits) keypair...\n", KEY_LENGTH);
	fflush(stdout);

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

int CheckHashValidation(size_t input_len, unsigned char* raw, unsigned char* hash_value){
	//TODO: Check if hash of encrypt is the same with tmp
	char hash_out[20];
	SHA1(raw, input_len, hash_out);
	hash_valid = 1;
	int i;
	for(i = 0; i < 20; i++){
		if(hash_out[i] != *(hash_value + i)){
			hash_valid = 0;
			break;
		}
	}
	return hash_valid;
}
