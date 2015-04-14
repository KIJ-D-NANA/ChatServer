#include "rc4encryption.h"
#include <string.h>

void initRC4key(RC4Container* container, char* seed, size_t key_len){
	int i;
	for(i = 0; i < 256; i++){
		container->S[i] = i;
		container->T[i] = *(seed + (i % key_len));
	}
	RC4InitPremutation(container);
}

void RC4InitPremutation(RC4Container* container){
	int j = 0;
	int i;
	unsigned char temp;
	for(i = 0; i < 256; i++){
		j = (j + container->S[i] + container->T[i]) % 256;
		temp = container->S[i];
		container->S[i] = container->S[j];
		container->S[j] = temp;
	}
}

unsigned char RC4Genkey(unsigned char* tempS, int* iter1, int* iter2){
	unsigned char temp;
	*iter1 = (*iter1 + 1) % 256;
	*iter2 = (*iter2 + tempS[*iter1]) % 256;
	temp = tempS[*iter1];
	tempS[*iter1] = tempS[*iter2];
	tempS[*iter2] = temp;
	int t = (tempS[*iter1] + tempS[*iter2]) % 256;
	return tempS[t];
}

int RC4Crypt(size_t input_len, unsigned char* input, unsigned char* output, RC4Container* key){
	int* iter1 = key->iter1;
	int* iter2 = key->iter2;
	*iter1 = *iter2 = 0;

	unsigned char tempS[256];
	strncpy(key->S, tempS, sizeof(key->S));
	int i;
	for(i = 0; i < input_len; i++){
		*(output + i) = RC4Genkey(tempS, iter1, iter2) ^ *(input + i);
	}
	return i;
}
