#include "Base64.h"
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <stdlib.h>
#include <string.h>

int Base64encode(const unsigned char* buffer, int length, char** base64_text){
	BIO *bio, *b64;
	BUF_MEM *bufferPtr;

	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new(BIO_s_mem());
	bio = BIO_push(b64, bio);

	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
	BIO_write(bio, buffer, length);
	BIO_flush(bio);
	BIO_get_mem_ptr(bio, &bufferPtr);

	*base64_text = (char*)malloc(bufferPtr->length);
	memcpy(*base64_text, bufferPtr->data, bufferPtr->length);
	int length2 = bufferPtr->length;

	BIO_free_all(bio);

	return length2;
}

int calcDecodeLength(const char* b64input){
	size_t len = strlen(b64input),
		   padding = 0;

	if(b64input[len - 1] == '=' && b64input[len - 2] == '=')
		padding = 2;
	else if(b64input[len - 1] == '=')
		padding = 1;

	return (len * 3)/4 - padding;
}

int Base64decode(char* b64message, unsigned char** buffer, int length){
	BIO *bio, *b64;
	int decodeLen = calcDecodeLength(b64message);
	*buffer = (unsigned char*)malloc(decodeLen);

	bio = BIO_new_mem_buf(b64message, length);
	b64 = BIO_new(BIO_f_base64());
	bio = BIO_push(b64, bio);

	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
	int result = BIO_read(bio, *buffer, strlen(b64message));
	BIO_free_all(bio);

	return result;
}
