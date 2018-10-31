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
#define NUM_PODS 10	//Number of Pods in our table
#define NUM_RECORDS_PER_POD 10	//Number of entries per pod
#define NUM_RECORDS NUM_PODS * NUM_RECORDS_PER_POD	//Number of records in our table
#define MAX_NUM_VALUES_PER_KEY 5	//Max number of values in a key
#define MAX_KEY_SIZE 32	//Max size of key in bytes
#define MAX_VALUE_SIZE 256	//Max value size in bytes
#define WRITER_SEM_NAME "writer_lock_ktagoe"
#define READER_SEM_NAME "reader_lock_ktagoe"
#define MAGIC_HASH_NUMBER 5381



typedef struct KVRecord{
	int oldest_Location;	//Index for the oldest value in the list - head of array
	int available_Location;	//Index of available location in value list - tail of array
	char* key;				//Hashed key
	char *value[MAX_NUM_VALUES_PER_KEY];	//Array of pointers to values
}Record;

typedef struct KVPod{
	//int oldest_Record;				//Index for the oldest record in the list
	Record recordList[NUM_RECORDS];	//List of records
}Pod;

typedef struct KVStore{
	int num_readers;
	Pod podList[NUM_PODS];	//List of pods
}Store;



Store *table;	//Key-Value Store Structure global variable
char* sharedMemoryObject;	//Shared memory object global variable



int kv_store_create(char* name);

int kv_store_write(char *key, char * value);

char *kv_store_read(char *key);

char **kv_store_read_all(char *key);

