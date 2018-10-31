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
#define NUM_RECORDS 10	//Number of entries per pod
#define MAX_NUM_VALUES 5	//Max number of values in a key
#define MAX_KEY_SIZE 32	//Max size of key in bytes
#define MAX_VALUE_SIZE 256	//Max value size in bytes
#define SEM_NAME "lock_ktagoe"

typedef struct KVRecord{
	int oldest_Location;	//Index for the oldest value in the list
	int available_Location;	//Index of available location in value list
	int key;	//Hashed key
	char *value[MAX_NUM_VALUES];	//Array of pointers to values
}Record;

typedef struct KVPod{
	//int oldest_Record;				//Index for the oldest record in the list
	Record recordList[NUM_RECORDS];	//List of records
}Pod;

typedef struct KVStore{
	Pod podList[NUM_PODS];	//List of pods
}Store;

int hash(char* key){
	return atoi(key) % NUM_PODS;
}

Store *table;	//Key-Value Store Structure global variable
char* sharedMemoryObject;	//Shared memory object global variable
sem_t *lock;

int kv_store_create(char* name);

int kv_store_write(char *key, char * value);

char *kv_store_read(char *key);

char **kv_store_read_all(char *key);

