//rm os_test1; rm -r /dev/shm/writer_lock_ktagoe; rm -r /dev/shm/reader_lock_ktagoe; clear; make test1; gdb os_test1

//rm os_test2; rm -r /dev/shm/writer_lock_ktagoe; rm -r /dev/shm/reader_lock_ktagoe; clear; make test2; gdb os_test2


#include "a2_lib.h"

unsigned long hash(unsigned char *str) {
	return generate_hash(str) % NUM_PODS;
}

int kv_store_create(char* name){
	int fd;
	Store* table;
	
	sharedMemoryObject = name;
	//table = malloc(SIZE_OF_STORE);
	fd = shm_open(sharedMemoryObject, O_CREAT | O_RDWR, S_IRWXU);
	if (fd < 0) {
		perror("Unable to open shared object\n");
		return -1;
	}
	
	ftruncate(fd, SIZE_OF_STORE);

	table = (Store *) mmap(0, SIZE_OF_STORE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	memset(table, '\0', SIZE_OF_STORE);
	if ( (char *) table == (char *) -1) {
		perror("Unable to map key-value structure to shared object");
		close(fd);
		return -1;
	}

	(*table).num_readers = 0;
	for (int i = 0; i < NUM_PODS; i++) {
		//(*table).podList[i].oldestRecord = 0;
		for (int j = 0; j < NUM_RECORDS_PER_POD; j++){
			(*table).podList[i].recordList[j].oldest_Location = 0;
			(*table).podList[i].recordList[j].available_Location = 0;
			(*table).podList[i].recordList[j].read_Index = 0;
		}
	}

	close(fd);
	munmap(table, SIZE_OF_STORE);

	sem_t *write_lock = sem_open(WRITER_SEM_NAME, O_CREAT, 0644, 1);
	if (write_lock == SEM_FAILED){
		perror("Unable to create semaphore");
		return -1;
	}

	sem_t *read_lock = sem_open(READER_SEM_NAME, O_CREAT, 0644, 1);
	if (read_lock == SEM_FAILED){
		perror("Unable to create semaphore");
		return -1;
	}

	return 0; 
}



int kv_store_write(char *key, char *value){
	int fd, hashedKey, entryFound, head, tail;
	Store * table;
	char *oldest_value;
	bool newEntryFound, keyEntryFound;


	//OBTAIN WRITE_LOCK
	sem_t *write_lock = sem_open(WRITER_SEM_NAME, O_CREAT);
	if (write_lock == SEM_FAILED){
		perror("Unable to open the write semaphore");
		return -1;
	}

	int lock = sem_wait(write_lock);
	if (lock == -1){
		perror("Unable to lock the write semaphore");
		return -1;
	}
	
	fd = shm_open(sharedMemoryObject, O_RDWR, 0);
	if (fd < 0) {
		perror("Unable to open shared object for writing process\n");
		return -1;
	}

	table = (Store *) mmap(0, SIZE_OF_STORE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if ( (char *) table == (char *) -1) {
		perror("Unable to map key-value structure to shared object");
		close(fd);
		return -1;
	}

	hashedKey = hash( (unsigned char *) key);	//Hashed key
	newEntryFound = false;		//We want to find an empty index in our pod
	keyEntryFound = false;		//We will use this to find the key's index in our pod
	entryFound = -1;				//Earliest available location in a pod
	for (int i = 0; i < NUM_RECORDS_PER_POD && (!newEntryFound || !keyEntryFound); i++){
		if ( (*table).podList[hashedKey].recordList[i].key[0] == '\0' ){ //CHANGE THIS CHANGE THIS CHANGE THIS CHANGE THIS CHANGE THIS
			newEntryFound = true;
			entryFound = i;
			break;
		}else if ( strcmp((*table).podList[hashedKey].recordList[i].key, key) == 0 ){ 
			keyEntryFound = true;
			entryFound = i;
			break;
		}
	}
	if (entryFound == -1){
		perror("+++Unable to write to key-value store\n");
		close(fd);
		munmap(table, SIZE_OF_STORE);
		return -1;
	}

	if (newEntryFound == true && keyEntryFound == false){
		//OUR KEY IS NOT PRESENT IN OUR POD
		memcpy( (*table).podList[hashedKey].recordList[entryFound].key, key, MAX_KEY_SIZE );
		memcpy( (*table).podList[hashedKey].recordList[entryFound].value[0], value, MAX_VALUE_SIZE );
	}else if (newEntryFound == false && keyEntryFound == true){
		//OUR KEY IS PRESENT IN OUR POD
		tail = (*table).podList[hashedKey].recordList[entryFound].available_Location;
		head = (*table).podList[hashedKey].recordList[entryFound].oldest_Location;
		//memcpy( oldest_value, (*table).podList[hashedKey].recordList[entryFound].value[head], MAX_VALUE_SIZE );
		oldest_value = (*table).podList[hashedKey].recordList[entryFound].value[head];

		if ( head != tail ){	//THE LOCATION OF THE OLDEST VALUE AND NEXT AVAILABLE LOCATOIN ARE DIFFERENT
			memcpy( (*table).podList[hashedKey].recordList[entryFound].value[tail], value, MAX_VALUE_SIZE );
			(*table).podList[hashedKey].recordList[entryFound].available_Location = (tail + 1) % MAX_NUM_VALUES_PER_KEY;
		}else{	//THE LOCATION OF THE OLDEST VALUE AND NEXT AVAILABLE LOCATOIN ARE DIFFERENT
			if (oldest_value[0] == '\0'){
			//NO KEY HAS BEEN INSERTED YET
				memcpy( (*table).podList[hashedKey].recordList[entryFound].value[tail], value, MAX_VALUE_SIZE );
				(*table).podList[hashedKey].recordList[entryFound].available_Location = ( tail + 1) % MAX_NUM_VALUES_PER_KEY;
			}else{
			//OUR VALUE LIST IS FULL AND THE VALUE AT THE OLDEST LOCATOIN IS TO BE EVICTED
				memcpy( (*table).podList[hashedKey].recordList[entryFound].value[tail], value, MAX_VALUE_SIZE );
				(*table).podList[hashedKey].recordList[entryFound].oldest_Location = ( head + 1) % MAX_NUM_VALUES_PER_KEY;
				(*table).podList[hashedKey].recordList[entryFound].available_Location = ( tail + 1) % MAX_NUM_VALUES_PER_KEY;
				int rIndex = (*table).podList[hashedKey].recordList[entryFound].read_Index;
				if (head == rIndex) (*table).podList[hashedKey].recordList[entryFound].read_Index = (rIndex + 1) % MAX_NUM_VALUES_PER_KEY;
			}
		}
	}else{
		perror("Unable to write to key-value store\n");
		close(fd);
		munmap(table, SIZE_OF_STORE);
		return -1;
	}

	close(fd);
	munmap(table, SIZE_OF_STORE);


	//RELEASE WRITE_LOCK
	int unlock = sem_post(write_lock);
	if (unlock == -1){
		perror("Unable to unlock the write semaphore");
		return -1;
	}
	
	return 0;
}



char *kv_store_read(char *key){

	//OBTAIN READ_LOCK
	sem_t *read_lock1 = sem_open(READER_SEM_NAME, O_CREAT);
	if (read_lock1 == SEM_FAILED){
		perror("Unable to open the read semaphore");
		return NULL;
	}

	int rlock1 = sem_wait(read_lock1);
	if (rlock1 == -1){
		perror("Unable to lock the read semaphore");
		return NULL;
	}

	
	int	fd, hashedKey, head, entryFound;
	Store * table;
	char *value;
	bool keyEntryFound;

	fd = shm_open(sharedMemoryObject, O_RDWR, 0);
	if (fd < 0) {
		perror("Unable to open shared object for reading process\n");
		return NULL;
	}


	table = (Store *) mmap(0, SIZE_OF_STORE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if ( (char *) table == (char *) -1) {
		perror("Unable to map key-value structure to shared object");
		close(fd);
		return NULL;
	}

	//INCREMENT READER COUNT
	(*table).num_readers++;

	//OBTAIN WRITE_LOCK
	if ((*table).num_readers == 1){
		sem_t *write_lock1 = sem_open(WRITER_SEM_NAME, O_CREAT);
		if (write_lock1 == SEM_FAILED){
			perror("Unable to open the write semaphore");
			close(fd);
			munmap(table, SIZE_OF_STORE);
			return NULL;
		}
		int wlock1 = sem_wait(write_lock1);
		if (wlock1 == -1){
			perror("Unable to lock the write semaphore");
			close(fd);
			munmap(table, SIZE_OF_STORE);
			return NULL;
		}
	}

	//RELEASE READ_LOCK
	int unlock1 = sem_post(read_lock1);
	if (unlock1 == -1){
		perror("Unable to unlock the read semaphore");
		close(fd);
		munmap(table, SIZE_OF_STORE);
		return NULL;
	}

	hashedKey = hash( (unsigned char *) key);	//Hashed key
	keyEntryFound = false;		//We will use this to find the key's index in our pod
	entryFound = -1;				//Earliest available location in a pod
	for (int i = 0; i < NUM_RECORDS_PER_POD && !keyEntryFound; i++){
		if ( strcmp((*table).podList[hashedKey].recordList[i].key, key) == 0 ){ 
			keyEntryFound = true;
			entryFound = i;
		}
	}
	if (entryFound == -1){
		perror("Key not found\n");
		close(fd);
		munmap(table, SIZE_OF_STORE);
		return NULL;
	}

	int rIndex = (*table).podList[hashedKey].recordList[entryFound].read_Index;
	value = calloc(sizeof(char), MAX_VALUE_SIZE);
	memcpy( value, (*table).podList[hashedKey].recordList[entryFound].value[rIndex], MAX_VALUE_SIZE );
	(*table).podList[hashedKey].recordList[entryFound].read_Index = (rIndex + 1) % MAX_NUM_VALUES_PER_KEY;

	close(fd);
	munmap(table, SIZE_OF_STORE);


	//OBTAIN READ_LOCK
	sem_t *read_lock2 = sem_open(READER_SEM_NAME, O_CREAT);
	if (read_lock2 == SEM_FAILED){
		perror("Unable to open the read semaphore");
		return NULL;
	}

	int rlock2 = sem_wait(read_lock2);
	if (rlock2 == -1){
		perror("Unable to lock the read semaphore");
		close(fd);
		munmap(table, SIZE_OF_STORE);
		return NULL;
	}

	//DECREMENT READER COUNT
	(*table).num_readers--;

	//OBTAIN WRITE_LOCK
	if ((*table).num_readers == 1){
		sem_t *write_lock2 = sem_open(WRITER_SEM_NAME, O_CREAT);
		if (write_lock2 == SEM_FAILED){
			perror("Unable to open the write semaphore");
			close(fd);
			munmap(table, SIZE_OF_STORE);
			return NULL;
		}
		int wlock2 = sem_wait(write_lock2);
		if (wlock2 == -1){
			perror("Unable to lock the write semaphore");
			close(fd);
			munmap(table, SIZE_OF_STORE);
			return NULL;
		}
	}

	//RELEASE READ_LOCK
	int unlock2 = sem_post(read_lock2);
	if (unlock2 == -1){
		perror("Unable to unlock the read semaphore");
		close(fd);
		munmap(table, SIZE_OF_STORE);
		return NULL;
	}

	return value;
}



char **kv_store_read_all(char *key){


	/*
	OBTAIN READ_LOCK
	INCREMENT READER COUNT
	IF (READER COUNT == 1) then OBTAIN WRITE_LOCK
	OBTAIN READ_LOCK
	*/
	

	int	fd, hashedKey, entryFound;
	Store *table;
	char **values;
	bool keyEntryFound;

	fd = shm_open(sharedMemoryObject, O_RDONLY, 0);
	if (fd < 0) {
		perror("Unable to open shared object for reading process\n");
		return NULL;
	}

	table = (Store *) mmap(0, SIZE_OF_STORE, PROT_READ, MAP_SHARED, fd, 0);
	if ( (char *) table == (char *) -1) {
		perror("Unable to map key-value structure to shared object");
		close(fd);
		return NULL;
	}

	hashedKey = hash( (unsigned char *) key);	//Hashed key
	keyEntryFound = false;		//We will use this to find the key's index in our pod
	entryFound = -1;				//Earliest available location in a pod
	for (int i = 0; i < NUM_RECORDS_PER_POD && !keyEntryFound; i++){
		if ( strcmp((*table).podList[hashedKey].recordList[i].key, key) == 0 ){ 
			keyEntryFound = true;
			entryFound = i;
			break;
		}
	}
	if (entryFound == -1){
		perror("Key not found\n");
		close(fd);
		munmap(table, SIZE_OF_STORE);
		return NULL;
	}

	values = calloc(MAX_NUM_VALUES_PER_KEY + 1, sizeof(char *));
	for (int i = 0; i < MAX_NUM_VALUES_PER_KEY; i++){
		values[i] = calloc(sizeof(char), MAX_VALUE_SIZE);
		if ((*table).podList[hashedKey].recordList[entryFound].value[i][0] == '\0'){
			values[i] = NULL;
			break;
		}
		memcpy( values[i], (*table).podList[hashedKey].recordList[entryFound].value[i], MAX_VALUE_SIZE );
	}
	values[MAX_NUM_VALUES_PER_KEY] = NULL;

	close(fd);
	munmap(table, SIZE_OF_STORE);

	/*
	OBTAIN READ_LOCK
	*/

	/*
	DECREMENT READER COUNT
	if (READER COUNT == 0) then RELEASE WRITE_LOCK
	RELEASE READ_LOCK
	*/
	
	return values;
}
