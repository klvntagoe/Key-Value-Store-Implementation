#ifndef __A2_LIB_HEADER__
#define __A2_LIB_HEADER__

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

/* -------------------------------------
	Define your own globals here
----------------------------------------

#define __KEY_VALUE_STORE_SIZE__	50*1024*1024        			//50MB
#define __KV_WRITERS_SEMAPHORE__	"WRITER_SHABIRMEAN_260723366"
#define __KV_READERS_SEMAPHORE__	"READER_SHABIRMEAN_260723366" 
#define __KV_STORE_NAME__			"KV_STORE_SHABIRMEAN_260723366"
#define KV_EXIT_SUCCESS				0
#define KV_EXIT_FAILURE				-1

---------------------------------------- */


unsigned long generate_hash(unsigned char *str);

int kv_store_create(char *kv_store_name);
int kv_store_write(char *key, char *value);
char *kv_store_read(char *key);
char **kv_store_read_all(char *key);

#endif
