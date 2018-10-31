//clear; gcc -o hash a2_lib.c -lrt; ./hash
#include "a2_lib.h"
#include "a2_helpers"


int kv_store_create(char* name){
	int fd;
	char *address, *current_Address;
	
	sharedMemoryObject = name;
	fd = shm_open(name, O_EXCL | O_RDWR, 0);
	if (fd < 0) {
		perror("Unable to create shared object\n");
		exit(EXIT_FAILURE);
	}
	int kv_store_size = ((MAX_KEY_SIZE + MAX_VALUE_SIZE) * NUM_RECORDS) + (32 * NUM_PODS);
	ftruncate(fd, kv_store_size);
	address = mmap(NULL, kv_store_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if ( strcmp(address, MAP_FAILED) != 0) {
		perror("Unable to map key-value structure to shared object");
		exit(EXIT_FAILURE);
	}


	current_Address = address;
	while (current_Address - address <= kv_store_size){
		&current_Address += (MAX_KEY_SIZE + MAX_VALUE_SIZE) * NUM_RECORDS_PER_POD;
		*current_Address = NUM_RECORDS_PER_POD;
		&current_Address = 32;
	}



	close(fd);
	munmap(address, kv_store_size);
	lock = sem_open(SEM_NAME, O_CREAT, 0644, 1);
	if (lock == SEM_FAILED){
		perror("Unable to create semaphore");
		exit(EXIT_FAILURE);
	}
	return 0; 
}





int kv_store_write(char *key, char *value){
	//OBTAIN LOCK
	int fd;

	fd = shm_open(sharedMemoryObject, O_RDWR, 0);
	if (fd < 0) {
		perror("Unable to create shared object\n");
		exit(EXIT_FAILURE);
	}
	int kv_store_size = ((MAX_KEY_SIZE + MAX_VALUE_SIZE) * NUM_RECORDS) + (32 * NUM_PODS);
	address = mmap(NULL, kv_store_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);



	close(fd);
	munmap(address, kv_store_size);
	//RELEASE LOCK
	return 0;
}



char *kv_store_read(char *key){
	int fd, hashedKey, k, oldest_location;
	char *address, *tempKey, *v;

	fd = shm_open(sharedMemoryObject, O_RDONLY, 0);
	if (fd < 0) {
		perror("Unable to create shared object\n");
		exit(EXIT_FAILURE);
	}

	address = mmap(&table, sizeof(table), PROT_READ, MAP_SHARED, fd, 0);
	if ( strcmp(address, MAP_FAILED) != 0) {
		perror("Unable to map key-value structure to shared object");
		exit(EXIT_FAILURE);
	}
	//close(fd);

	hashedKey = hash(key);	//Hashed key
	k = atoi( strncpy(tempKey, key, MAX_KEY_SIZE) );	//Trauncated key 
	oldest_location = (*table).podList[hashedKey].recordList[k/NUM_RECORDS].oldest_Location;
	memcpy( v, (*table).podList[hashedKey].recordList[k/NUM_RECORDS].value[oldest_location], MAX_VALUE_SIZE );
	
	return v;
}



char **kv_store_read_all(char *key){
	int fd, hashedKey, k, oldest_location;
	char *address, *tempKey;
	char **v;

	fd = shm_open(sharedMemoryObject, O_RDONLY, 0);
	if (fd < 0) {
		perror("Unable to create shared object\n");
		exit(EXIT_FAILURE);
	}

	address = mmap(&table, sizeof(table), PROT_READ, MAP_SHARED, fd, 0);
	if ( strcmp(address, MAP_FAILED) != 0) {
		perror("Unable to map key-value structure to shared object");
		exit(EXIT_FAILURE);
	}
	//close(fd);

	hashedKey = hash(key);	//Hashed key
	k = atoi( strncpy(tempKey, key, MAX_KEY_SIZE) );	//Trauncated key
	oldest_location = (*table).podList[hashedKey].recordList[k/NUM_RECORDS].oldest_Location;
	v = (*table).podList[hashedKey].recordList[k/NUM_RECORDS].value;

	return v;
}



void main(){
	printf("\nHello World\n\n");
}
