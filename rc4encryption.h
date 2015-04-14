
typedef struct RC4Container{
	unsigned char S[256];
	unsigned char T[256];
	unsigned char K[256];
	int* iter1;
	int* iter2;
}RC4Container;

void initRC4key(RC4Container* container, char* seed, size_t key_len);
void RC4InitPremutation(RC4Container* container);
unsigned char RC4Genkey(unsigned char* tempS, int* iter1, int* iter2);
int RC4Crypt(size_t input_len, unsigned char* input, unsigned char* output, RC4Container* key);
