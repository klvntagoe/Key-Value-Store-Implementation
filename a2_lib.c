//clear; gcc -o store a2_lib.c -lrt; ./store
#include "a2_lib.h"

unsigned long hash(unsigned char *str) {
	return generate_hash(str) % NUM_PODS;
}

int kv_store_create(char* name){
	int fd;
	char* address;
	
	sharedMemoryObject = name;
	fd = shm_open(sharedMemoryObject, O_CREAT, 0);
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
	
	for (int i = 0; i < NUM_PODS; i++) {
		//(*table).podList[i].oldestRecord = 0;
		for (int j = 0; j < NUM_RECORDS; j++){
			(*table).podList[i].recordList[j].oldest_Location = 0;
			(*table).podList[i].recordList[j].available_Location = 0;
			(*table).podList[i].recordList[j].key = NULL;
		}
	}

	close(fd);
	munmap(address, sizeof(*table));

	sem_t *w_lock = sem_open(WRITER_SEM_NAME, O_CREAT, 0644, 1);
	if (w_lock == SEM_FAILED){
		perror("Unable to create semaphore");
		exit(EXIT_FAILURE);
	}

	sem_t *r_lock = sem_open(READER_SEM_NAME, O_CREAT, 0644, 1);
	if (r_lock == SEM_FAILED){
		perror("Unable to create semaphore");
		exit(EXIT_FAILURE);
	}

	return 0; 
}



int kv_store_write(char *key, char *value){
	int fd, hashedKey, entryFound, head, tail;
	char *address, *k, *v, *oldest_value;
	bool newEntryFound, keyEntryFound;


	/*
	OBTAIN WRITE_LOCK
	*/

	
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

	hashedKey = hash( (unsigned char *) key);	//Hashed key
	strncpy(k, key, MAX_KEY_SIZE);	//Trauncated value
	strncpy(v, value, MAX_VALUE_SIZE);	//Trauncated value

	newEntryFound = false;		//We want to find an empty index in our pod
	keyEntryFound = false;		//We will use this to find the key's index in our pod
	entryFound = 0;				//Earliest available location in a pod
	for (int i = 0; i < NUM_RECORDS_PER_POD && (!newEntryFound || !keyEntryFound); i++){
		if ( (*table).podList[hashedKey].recordList[i].key == NULL ){ //CHANGE THIS CHANGE THIS CHANGE THIS CHANGE THIS CHANGE THIS
			newEntryFound = true;
			entryFound = i;
		}else if ( strcmp((*table).podList[hashedKey].recordList[i].key, k) == 0 ){ 
			keyEntryFound = true;
			entryFound = i;
		}
	}

	if (newEntryFound == true && keyEntryFound == false){
		//(*table).podList[hashedKey].recordList[entryFound].key = k;
		//(*table).podList[hashedKey].recordList[entryFound].value[0] = v;
		memcpy( (*table).podList[hashedKey].recordList[entryFound].key, k, MAX_KEY_SIZE );
		memcpy( (*table).podList[hashedKey].recordList[entryFound].value[0], v, MAX_VALUE_SIZE );
	}else if (keyEntryFound == true && newEntryFound == false){
		tail = (*table).podList[hashedKey].recordList[entryFound].available_Location;
		head = (*table).podList[hashedKey].recordList[entryFound].oldest_Location;
		oldest_value = (*table).podList[hashedKey].recordList[entryFound].value[head];

		if ( head != tail ){
			memcpy( (*table).podList[hashedKey].recordList[entryFound].value[tail], v, MAX_VALUE_SIZE );
			(*table).podList[hashedKey].recordList[entryFound].available_Location = ( tail + 1) % NUM_RECORDS_PER_POD;
		}else{
			if (oldest_value == NULL){
				memcpy( (*table).podList[hashedKey].recordList[entryFound].value[tail], v, MAX_VALUE_SIZE );
				(*table).podList[hashedKey].recordList[entryFound].available_Location = ( tail + 1) % NUM_RECORDS_PER_POD;
			}else{
				memcpy( (*table).podList[hashedKey].recordList[entryFound].value[tail], v, MAX_VALUE_SIZE );
				(*table).podList[hashedKey].recordList[entryFound].oldest_Location = ( head + 1) % MAX_NUM_VALUES_PER_KEY;
				(*table).podList[hashedKey].recordList[entryFound].available_Location = ( tail + 1) % MAX_NUM_VALUES_PER_KEY;
			}
		}
	}else{
		perror("Unable to write to key-value store\n");
		exit(EXIT_FAILURE);
	}


	close(fd);
	munmap(address, sizeof(*table));
	/*
	RELEASE WRITE_LOCK
	*/
	
	
	return 0;
}



char *kv_store_read(char *key){


	/*
	OBTAIN READ_LOCK
	INCREMENT READER COUNT
	IF (READER COUNT == 1) then OBTAIN WRITE_LOCK
	OBTAIN READ_LOCK
	*/
	

	int	fd, hashedKey, head, entryFound;
	char *address, *k, *v, *oldest_value;
	bool keyEntryFound;

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

	hashedKey = hash( (unsigned char *) key);	//Hashed key
	strncpy(k, key, MAX_KEY_SIZE);	//Trauncated value
	//strncpy(v, value, MAX_VALUE_SIZE);	//Trauncated value

	keyEntryFound = false;		//We will use this to find the key's index in our pod
	entryFound = 0;				//Earliest available location in a pod
	for (int i = 0; i < NUM_RECORDS_PER_POD && !keyEntryFound; i++){
		if ( strcmp((*table).podList[hashedKey].recordList[i].key, k) == 0 ){ 
			keyEntryFound = true;
			entryFound = i;
		}
	}

	head = (*table).podList[hashedKey].recordList[entryFound].oldest_Location;
	oldest_value = (*table).podList[hashedKey].recordList[entryFound].value[head];
	v = oldest_value;


	close(fd);
	munmap(address, sizeof(*table));
	/*
	OBTAIN READ_LOCK
	*/


	/*
	DECREMENT READER COUNT
	if (READER COUNT == 0) then RELEASE WRITE_LOCK
	RELEASE READ_LOCK
	*/
	
	return v;
}



char **kv_store_read_all(char *key){


	/*
	OBTAIN READ_LOCK
	INCREMENT READER COUNT
	IF (READER COUNT == 1) then OBTAIN WRITE_LOCK
	OBTAIN READ_LOCK
	*/
	

	int	fd, hashedKey, entryFound;
	char *address, *k;
	char **oldest_value, **v;
	bool keyEntryFound;

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

	hashedKey = hash( (unsigned char *) key);	//Hashed key
	strncpy(k, key, MAX_KEY_SIZE);	//Trauncated value
	//strncpy(v, value, MAX_VALUE_SIZE);	//Trauncated value

	keyEntryFound = false;		//We will use this to find the key's index in our pod
	entryFound = 0;				//Earliest available location in a pod
	for (int i = 0; i < NUM_RECORDS_PER_POD && !keyEntryFound; i++){
		if ( strcmp((*table).podList[hashedKey].recordList[i].key, k) == 0 ){ 
			keyEntryFound = true;
			entryFound = i;
		}
	}

	oldest_value = (*table).podList[hashedKey].recordList[entryFound].value;
	v = oldest_value;


	close(fd);
	munmap(address, sizeof(*table));
	/*
	OBTAIN READ_LOCK
	*/


	/*
	DECREMENT READER COUNT
	if (READER COUNT == 0) then RELEASE WRITE_LOCK
	RELEASE READ_LOCK
	*/
	
	return v;
}
