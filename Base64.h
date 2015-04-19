int Base64encode(const unsigned char* buffer, int length, char** base64_text);
int Base64decode(char* b64message, unsigned char** buffer, int length);
int calcDecodeLength(const char* b64input);
