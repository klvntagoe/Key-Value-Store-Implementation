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
#define NUM_PODS 100	//Number of Pods in our table
#define NUM_RECORDS_PER_POD 10	//Number of entries per pod
#define NUM_RECORDS NUM_PODS * NUM_RECORDS_PER_POD	//Number of records in our table
#define MAX_NUM_VALUES_PER_KEY 256	//Max number of values in a key
#define MAX_KEY_SIZE 32	//Max size of key in bytes
#define MAX_VALUE_SIZE 256	//Max value size in bytes
#define SIZE_OF_STORE 75000000 //(75MB)
#define WRITER_SEM_NAME "writer_lock_ktagoe"
#define READER_SEM_NAME "reader_lock_ktagoe"
#define MAGIC_HASH_NUMBER 5381

//(8+32+(256*5))*100*100

typedef struct KVRecord{
	int oldest_Location;	//Index for the oldest value in the list - head of array
	int available_Location;	//Index of available location in value list - tail of array
	int read_Index;			//Index of the next value to be read
	char key[MAX_KEY_SIZE];				//Hashed key
	char value[MAX_NUM_VALUES_PER_KEY][MAX_VALUE_SIZE];	//Array of pointers to values
}Record;

typedef struct KVPod{
	//int oldest_Record;				//Index for the oldest record in the list
	Record recordList[NUM_RECORDS_PER_POD];	//List of recordsMAX_KEY_SIZE
}Pod;

typedef struct KVStore{
	int num_readers;
	Pod podList[NUM_PODS];	//List of pods
}Store;


char* sharedMemoryObject;	//Shared memory object global variable


unsigned long generate_hash(unsigned char *str);

int kv_store_create(char* name);

int kv_store_write(char *key, char * value);

char *kv_store_read(char *key);

char **kv_store_read_all(char *key);

