#include "comp310_a2_test.h"

#define MAGIC_HASH_NUMBER 5381

unsigned long generate_hash(unsigned char *str) {
	int c;
    #ifndef SDBM
	unsigned long hash = MAGIC_HASH_NUMBER;
	while (c = *str++)
            hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	
	#else
        unsigned long hash = 0;
        while (c = *str++)
            hash = c + (hash << 6) + (hash << 16) - hash;
	#endif
	return hash;
}

void generate_string(char buf[], int length){
    int type;
    for(int i = 0; i < length; i++){
        type = rand() % 3;
        if(type == 2)
            buf[i] = rand() % 26 + 'a';
        else if(type == 1)
            buf[i] = rand() % 10 + '0';
        else
            buf[i] = rand() % 26 + 'A';
    }
    buf[length - 1] = '\0';
}

void generate_unique_data(char buf[], int length, char **keys_buf, int num_keys){
    generate_string(buf, __TEST_MAX_DATA_LENGTH__);
    int counter = 0;
    for(int i = 0; i < num_keys; i++){
        if(strcmp(keys_buf[i], buf) == 0){
            counter++;
        }
    }
    if(counter > 1){
        generate_unique_data(buf, length, keys_buf, num_keys);
    }
    return;
}

void generate_key(char buf[], int length, char **keys_buf, int num_keys){
    generate_string(buf, __TEST_MAX_KEY_SIZE__);
    int counter = 0;
    for(int i = 0; i < num_keys; i++){
        if(strcmp(keys_buf[i], buf) == 0){
            counter++;
        }
    }
    if(counter > 1){
        generate_key(buf, length, keys_buf, num_keys);
    }
    return;
}
