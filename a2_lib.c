//clear; gcc -o store a2_lib.c -lrt; ./store
#include "a2_lib.h"


int kv_store_create(char* name){
	int fd;
	char* address;
	
	sharedMemoryObject = name;
	fd = shm_open(name, O_EXCL | O_RDWR, 0);
	if (fd < 0) {
		perror("Unable to create shared object\n");
		exit(EXIT_FAILURE);
	}
	
	ftruncate(fd, sizeof(*table));
	
	address = mmap(&table, sizeof(table), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if ( strcmp(address, MAP_FAILED) != 0) {
		perror("Unable to map key-value structure to shared object");
		exit(EXIT_FAILURE);
	}
	close(fd);
	munmap(address, sizeof(*table));

	for (int i = 0; i < NUM_PODS; i++) {
		//(*table).podList[i].oldestRecord = 0;
		for (int j = 0; j < NUM_RECORDS; j++){
			(*table).podList[i].recordList[j].oldest_Location = 0;
			(*table).podList[i].recordList[j].available_Location = 0;
		}
	}
	lock = sem_open(SEM_NAME, O_CREAT, 0644, 1);
	if (lock == SEM_FAILED){
		perror("Unable to create semaphore");
		exit(EXIT_FAILURE);
	}
	return 0; 
}



int kv_store_write(char *key, char *value){
	int fd, hashedKey, k, available_location, oldest_location;
	char *address, *tempKey, *v, *oldest_value;
	
	fd = shm_open(sharedMemoryObject, O_RDWR, 0);
	if (fd < 0) {
		perror("Unable to create shared object\n");
		exit(EXIT_FAILURE);
	}
	
	address = mmap(&table, sizeof(table), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if ( strcmp(address, MAP_FAILED) != 0) {
		perror("Unable to map key-value structure to shared object");
		exit(EXIT_FAILURE);
	}
	close(fd);

	hashedKey = hashFunction(key);	//Hashed key
	k = atoi( strncpy(tempKey, key, MAX_KEY_SIZE) );	//Trauncated key
	strncpy(v, value, MAX_VALUE_SIZE);	//Trauncated valu

	available_location = (*table).podList[hashedKey].recordList[k/NUM_RECORDS].available_Location;
	oldest_location = (*table).podList[hashedKey].recordList[k/NUM_RECORDS].oldest_Location;
	oldest_value = (*table).podList[hashedKey].recordList[k/NUM_RECORDS].value[oldest_location];

	if ( available_location != oldest_location ){	//Standard requirement
		//memcpy( (*table).podList[hashedKey].recordList[k/NUM_RECORDS].key, k, MAX_KEY_SIZE );
		memcpy( (*table).podList[hashedKey].recordList[k/NUM_RECORDS].value[available_location], v, MAX_VALUE_SIZE );
		(*table).podList[hashedKey].recordList[k/NUM_RECORDS].key = k;
		//(*table).podList[hashedKey].recordList[k/NUM_RECORDS].value[available_location] = v;
		(*table).podList[hashedKey].recordList[k/NUM_RECORDS].available_Location = ( available_location + 1) % MAX_NUM_VALUES;
	}else if ( available_location == oldest_location && oldest_value == NULL ){		//There is nothing in this record
		//memcpy( (*table).podList[hashedKey].recordList[k/NUM_RECORDS].key, k, MAX_KEY_SIZE );
		memcpy( (*table).podList[hashedKey].recordList[k/NUM_RECORDS].value[available_location], v, MAX_VALUE_SIZE );
		(*table).podList[hashedKey].recordList[k/NUM_RECORDS].key = k;
		//(*table).podList[hashedKey].recordList[k/NUM_RECORDS].value[available_location] = v;
		(*table).podList[hashedKey].recordList[k/NUM_RECORDS].available_Location = ( available_location + 1) % MAX_NUM_VALUES;
	}else if ( available_location == oldest_location && oldest_value != NULL ){		//We want to evict the value from the oldest location
		//memcpy( (*table).podList[hashedKey].recordList[k/NUM_RECORDS].key, k, MAX_KEY_SIZE );
		memcpy( (*table).podList[hashedKey].recordList[k/NUM_RECORDS].value[available_location], v, MAX_VALUE_SIZE );
		(*table).podList[hashedKey].recordList[k/NUM_RECORDS].key = k;
		//(*table).podList[hashedKey].recordList[k/NUM_RECORDS].value[available_location] = v;
		(*table).podList[hashedKey].recordList[k/NUM_RECORDS].oldest_Location = ( oldest_location + 1) % MAX_NUM_VALUES;
		(*table).podList[hashedKey].recordList[k/NUM_RECORDS].available_Location = ( available_location + 1) % MAX_NUM_VALUES;
	}else{
		perror("Unable to write to key-value store\n");
		exit(EXIT_FAILURE);
	}
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

	hashedKey = hashFunction(key);	//Hashed key
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

	hashedKey = hashFunction(key);	//Hashed key
	k = atoi( strncpy(tempKey, key, MAX_KEY_SIZE) );	//Trauncated key
	oldest_location = (*table).podList[hashedKey].recordList[k/NUM_RECORDS].oldest_Location;
	v = (*table).podList[hashedKey].recordList[k/NUM_RECORDS].value;

	return v;
}



int main(){
	printf("\nHello World\n\n");
	return 0;
}
