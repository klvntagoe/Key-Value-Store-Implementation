//clear; gcc -o store a2_lib.c -lrt; ./store
#include "a2_lib.h"

unsigned long hash(unsigned char *str) {
	return generate_hash(str) % NUM_PODS;
}

int kv_store_create(char* name){
	int fd;
	char* address;
	
	sharedMemoryObject = name;
	//table = malloc(SIZE_OF_STORE);
	fd = shm_open(sharedMemoryObject, O_CREAT | O_RDWR, S_IRWXU);
	if (fd < 0) {
		perror("Unable to open shared object\n");
		return -1;
	}
	
	ftruncate(fd, SIZE_OF_STORE);

	address = mmap(0, SIZE_OF_STORE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	memset(address, '\0', SIZE_OF_STORE);
	table = (Store*) address;
	if ( address == (char *) -1) {
		perror("Unable to map key-value structure to shared object");
		return -1;
	}

	for (int i = 0; i < NUM_PODS; i++) {
		//(*table).podList[i].oldestRecord = 0;
		for (int j = 0; j < NUM_RECORDS_PER_POD; j++){
			(*table).podList[i].recordList[j].oldest_Location = 0;
			(*table).podList[i].recordList[j].available_Location = 0;
			//(*table).podList[i].recordList[j].key = NULL;
		}
	}

	close(fd);
	munmap(address, SIZE_OF_STORE);

	/*
	sem_t *w_lock = sem_open(WRITER_SEM_NAME, O_CREAT, 0644, 1);
	if (w_lock == SEM_FAILED){
		perror("Unable to create semaphore");
		return -1;
	}

	sem_t *r_lock = sem_open(READER_SEM_NAME, O_CREAT, 0644, 1);
	if (r_lock == SEM_FAILED){
		perror("Unable to create semaphore");
		return -1;
	}
	*/

	return 0; 
}



int kv_store_write(char *key, char *value){
	int fd, hashedKey, entryFound, head, tail;
	char *address;
	bool newEntryFound, keyEntryFound;


	//OBTAIN WRITE_LOCK
	
	
	fd = shm_open(sharedMemoryObject, O_RDWR, 0);
	if (fd < 0) {
		perror("Unable to open shared object for writing process\n");
		return -1;
	}

	address = mmap(&table, SIZE_OF_STORE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	//char *truncatedKey, *truncatedValue, *oldest_value;
	char truncatedKey[MAX_KEY_SIZE] = {'\0'};
	char truncatedValue[MAX_VALUE_SIZE] = {'\0'};
	char oldest_value[MAX_VALUE_SIZE] = {'\0'};
	//memset(truncatedKey, '\0', MAX_KEY_SIZE);
	//memset(truncatedValue, '\0', MAX_VALUE_SIZE);
	//memset(oldest_value, '\0', MAX_VALUE_SIZE);

	printf("Debug Test\n");
	printf("Current value for our Key is %s\n", key);
	printf("Current value for our Value is %s\n", value);
	printf("Current value for our truncated Key is %s\n", truncatedKey);
	printf("Current value for our truncated Value is %s\n", truncatedValue);

	table = (Store*) address;
	if ( address == (char *) -1) {
		perror("Unable to map key-value structure to shared object");
		return -1;
	}

	hashedKey = hash( (unsigned char *) key);	//Hashed key
	//memset(truncatedKey, '\0', MAX_KEY_SIZE);
	//memset(truncatedValue, '\0', MAX_VALUE_SIZE);
	strncpy(truncatedKey, key, MAX_KEY_SIZE);	//Truncated value
	strncpy(truncatedValue, value, MAX_VALUE_SIZE);	//Truncated value
	//truncatedValue = value;

	newEntryFound = false;		//We want to find an empty index in our pod
	keyEntryFound = false;		//We will use this to find the key's index in our pod
	entryFound = -1;				//Earliest available location in a pod
	for (int i = 0; i < NUM_RECORDS_PER_POD && (!newEntryFound || !keyEntryFound); i++){
		if ( (*table).podList[hashedKey].recordList[i].key == NULL ){ //CHANGE THIS CHANGE THIS CHANGE THIS CHANGE THIS CHANGE THIS
			newEntryFound = true;
			entryFound = i;
		}else if ( strcmp((*table).podList[hashedKey].recordList[i].key, truncatedKey) == 0 ){ 
			keyEntryFound = true;
			entryFound = i;
		}
	}
	if (entryFound == -1){
		perror("Unable to write to key-value store\n");
		return -1;
	}

	if (newEntryFound == true && keyEntryFound == false){
		//(*table).podList[hashedKey].recordList[entryFound].key = truncatedKey;
		//(*table).podList[hashedKey].recordList[entryFound].value[0] = truncatedValue;
		memcpy( (*table).podList[hashedKey].recordList[entryFound].key, truncatedKey, MAX_KEY_SIZE );
		memcpy( (*table).podList[hashedKey].recordList[entryFound].value[0], truncatedValue, MAX_VALUE_SIZE );
	}else if (keyEntryFound == true && newEntryFound == false){
		tail = (*table).podList[hashedKey].recordList[entryFound].available_Location;
		head = (*table).podList[hashedKey].recordList[entryFound].oldest_Location;
		oldest_value = (*table).podList[hashedKey].recordList[entryFound].value[head];

		if ( head != tail ){
			memcpy( (*table).podList[hashedKey].recordList[entryFound].value[tail], truncatedValue, MAX_VALUE_SIZE );
			(*table).podList[hashedKey].recordList[entryFound].available_Location = ( tail + 1) % NUM_RECORDS_PER_POD;
		}else{
			if (oldest_value == NULL){
				memcpy( (*table).podList[hashedKey].recordList[entryFound].value[tail], truncatedValue, MAX_VALUE_SIZE );
				(*table).podList[hashedKey].recordList[entryFound].available_Location = ( tail + 1) % NUM_RECORDS_PER_POD;
			}else{
				memcpy( (*table).podList[hashedKey].recordList[entryFound].value[tail], truncatedValue, MAX_VALUE_SIZE );
				(*table).podList[hashedKey].recordList[entryFound].oldest_Location = ( head + 1) % MAX_NUM_VALUES_PER_KEY;
				(*table).podList[hashedKey].recordList[entryFound].available_Location = ( tail + 1) % MAX_NUM_VALUES_PER_KEY;
			}
		}
	}else{
		perror("Unable to write to key-value store\n");
		return -1;
	}


	close(fd);
	munmap(address, SIZE_OF_STORE);

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

	printf("Debug Test\n");
	printf("Current value for our Key is %s\n", key);
	printf("Current value for our truncated Key is %s\n", k);
	printf("Current value for our Value is %s\n", v);

	fd = shm_open(sharedMemoryObject, O_RDONLY, 0);
	if (fd < 0) {
		perror("Unable to open shared object for reading process\n");
		return NULL;
	}


	address = mmap(0, SIZE_OF_STORE, PROT_READ, MAP_SHARED, fd, 0);
	table = (Store*) address;
	if ( address == (char *) -1) {
		perror("Unable to map key-value structure to shared object");
		return NULL;
	}


	hashedKey = hash( (unsigned char *) key);	//Hashed key
	strncpy(k, key, MAX_KEY_SIZE);	//Truncated value
	//strncpy(v, value, MAX_VALUE_SIZE);	//Truncated value

	keyEntryFound = false;		//We will use this to find the key's index in our pod
	entryFound = -1;				//Earliest available location in a pod
	for (int i = 0; i < NUM_RECORDS_PER_POD && !keyEntryFound; i++){
		if ( strcmp((*table).podList[hashedKey].recordList[i].key, k) == 0 ){ 
			keyEntryFound = true;
			entryFound = i;
		}
	}
	if (entryFound == -1){
		perror("Key not found\n");
		return NULL;
	}

	head = (*table).podList[hashedKey].recordList[entryFound].oldest_Location;
	oldest_value = (*table).podList[hashedKey].recordList[entryFound].value[head];
	v = oldest_value;


	close(fd);
	munmap(address, SIZE_OF_STORE);

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
		perror("Unable to open shared object for reading process\n");
		return NULL;
	}

	address = mmap(0, SIZE_OF_STORE, PROT_READ, MAP_SHARED, fd, 0);
	table = (Store*) address;
	if ( address == (char *) -1) {
		perror("Unable to map key-value structure to shared object");
		return NULL;
	}

	hashedKey = hash( (unsigned char *) key);	//Hashed key
	strncpy(k, key, MAX_KEY_SIZE);	//Truncated value
	//strncpy(v, value, MAX_VALUE_SIZE);	//Truncated value

	keyEntryFound = false;		//We will use this to find the key's index in our pod
	entryFound = -1;				//Earliest available location in a pod
	for (int i = 0; i < NUM_RECORDS_PER_POD && !keyEntryFound; i++){
		if ( strcmp((*table).podList[hashedKey].recordList[i].key, k) == 0 ){ 
			keyEntryFound = true;
			entryFound = i;
		}
	}
	if (entryFound == -1){
		perror("Key not found\n");
		return NULL;
	}

	oldest_value = (*table).podList[hashedKey].recordList[entryFound].value;
	v = oldest_value;


	close(fd);
	munmap(address, SIZE_OF_STORE);

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
