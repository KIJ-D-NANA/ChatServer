#include "RSACrypto.h"
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <stdio.h>

RSA* createRSA(unsigned char* key, int is_public){
	RSA* rsa = NULL;
	BIO* keybio;
	keybio = BIO_new_mem_buf(key, -1);
	if(keybio == NULL){
		printf("Failed to create key BIO\n");
		return 0;
	}

	if(is_public){
		rsa = PEM_read_bio_RSA_PUBKEY(keybio, &rsa, NULL, NULL);
	}
	else{
        rsa = PEM_read_bio_RSAPrivateKey(keybio, &rsa, NULL, NULL);
	}

	if(rsa == NULL){
		printf("Failed to create RSA\n");
	}
	BIO_free(keybio);

	return rsa;
}

int public_encrypt(unsigned char* data, int data_len, unsigned char* key, unsigned char* encrypted, int padding){
	RSA* rsa = createRSA(key, 1);
	int result = RSA_public_encrypt(data_len, data, encrypted, rsa, padding);
	RSA_free(rsa);
	return result;
}

int public_decrypt(unsigned char* data, int data_len, unsigned char* key, unsigned char* decrypted, int padding){
	RSA* rsa = createRSA(key, 1);
	int result = RSA_public_decrypt(data_len, data, decrypted, rsa, padding);
	RSA_free(rsa);
	return result;
}

int private_encrypt(unsigned char* data, int data_len, unsigned char* key, unsigned char* encrypted, int padding){
	RSA* rsa = createRSA(key, 0);
	int result = RSA_private_encrypt(data_len, data, encrypted, rsa, padding);
	RSA_free(rsa);
	return result;
}

int private_decrypt(unsigned char* data, int data_len, unsigned char* key, unsigned char* decrypted, int padding){
	RSA* rsa = createRSA(key, 1);
	int result = RSA_private_decrypt(data_len, data, decrypted, rsa, padding);
	RSA_free(rsa);
	return result;
}

void printLastError(char* msg){
	char* err = (char*)malloc(130);
	ERR_load_crypto_strings();
	ERR_error_string(ERR_get_error(), err);
	printf("%s ERROR: %s\n", msg, err);
	free(err);
}
