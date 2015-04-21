#include <openssl/rsa.h>

RSA* createRSA(unsigned char* key, int is_public);
int public_encrypt(unsigned char* data, int data_len, unsigned char* key, unsigned char* encrypted, int padding);
int public_decrypt(unsigned char* data, int data_len, unsigned char* key, unsigned char* decrypted, int padding);
int private_encrypt(unsigned char* data, int data_len, unsigned char* key, unsigned char* encrypted, int padding);
int private_decrypt(unsigned char* data, int data_len, unsigned char* key, unsigned char* decrypted, int padding);
void printLastError(char* msg);
