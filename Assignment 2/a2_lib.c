/*
 * ECSE 427 OS : ASSIGNMENT #2 SIMPLE KEY VALUE STORE
 * STUDENT NAME: Muhammad Anza Khan
 * Student ID  : 260618490
 */

#include "a2_lib.h"

#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>
//Following sempahores have been taking using the reference of the Readers and Writers problem
//Section 2.5.2 of the course text

//the first semaphore controls access to the store
sem_t *db;

//this sempahore controls access to the rc
sem_t *mutex;

//This pointer will hold the address in the VM space of the shared memory object
char *VIRTUAL_SPACE_ADDRESS;

//This function was provided to us by the TA
unsigned long generate_hash(const char * str) {
	unsigned long hash = 5381;
	int c;

	while ((c = *str++)){
		hash = ((hash << 5) + hash) + c;
	}

	hash = (hash > 0) ? hash : -(hash);

	return hash % NUMBER_OF_PODS;
}

/*
 * FUNCTION NAME: kv_store_create
 * FUNCTION DESCRIPTION: Creates a store if it is not yet created or opens an existing one.
 * After a successful call, the calling process should have access to the store.
 * This function could fail if the system does not have enough memory.
 */

int kv_store_create(const char *name){
	// implement your create method code here
	int fd;

	//call shm_open to create a shared memory object with the name given in kv_store_name
	fd = shm_open(name, O_CREAT | O_RDWR, S_IRWXU);

	//error checking the value of fd
	if(fd == -1){
		perror("Not enough memory to create the shared memory object");
		return(-1);
	}

	//ftruncate is called to resize the shared memory object to fit our store
	ftruncate(fd, sizeof(Data) + (POD_INDEXING * NUMBER_OF_PODS));

	//now we call mmap to map the shared memory object obtained to an address in thhe virtual memory space
	VIRTUAL_SPACE_ADDRESS = (char *)mmap(NULL, sizeof(Data) + (POD_INDEXING * NUMBER_OF_PODS), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	//error check the returned value for VIRTUAL_SPACE_ADDRESS
	if(VIRTUAL_SPACE_ADDRESS == MAP_FAILED){
		perror("Failed to map the shared memory object");
		return(-1);
	}

	//casting the returned memory pointer to our book keeping pointer
	Data *book_keeping = (Data *)VIRTUAL_SPACE_ADDRESS;

	//Within the creation of our store we also initialize our semaphores
	db = sem_open("db", O_CREAT, S_IRWXU, 1);
	mutex = sem_open("mutex", O_CREAT, S_IRWXU, 1);

	if(db == SEM_FAILED){
		perror("db semaphore failed.");
		return(-1);
	}

	if(mutex == SEM_FAILED){
		perror("mutex semaphore failed.");
		return(-1);
	}

	//setting everything within our book_keeper to 0, basically initializing it.
	if(book_keeping->initiliazed == 0){
		for(int i = 0; i < 256; i++){
			book_keeping->WRITE_COUNTERS[i] = 0;
			book_keeping->READ_COUNTERS[i] = 0;
		}
		book_keeping->initiliazed = 1;
		book_keeping->read_counter = 0;
	}

	//return the file descriptor
	close(fd);
	return(0);
}

/*
 * FUNCTION NAME: kv_store_write
 * FUNCTION DESCRIPTION: Takes a key-value pair and writes them to the store.
 * If the store is full the store needs to evict an existing entry to make room for another.
 * Also, this function needs to update an index that is also maintained in the store so that the reads
 * looking for a key-value pair can be completed as fast as possible.
 */

int kv_store_write(const char *key, const char *value){
	// implement your create method code here
	int hash_table_index;

	//now we are about to enter the critical section, call semaphore to get exclusive access
	sem_wait(db);

	//firstly we need to get a index in the hash table for our key_value pair
	hash_table_index = generate_hash(key);

	//this gives us the index in the pod were we store our key_value pair
	size_t pod_index = POD_INDEXING * hash_table_index;

	//this gives us the offset needed for each individual key value pair
	Data *data_ptr = (Data *)VIRTUAL_SPACE_ADDRESS;
	size_t key_value_pair_offset = (MAX_LENGTH_VALUE + MAX_LENGTH_KEY) * data_ptr->WRITE_COUNTERS[hash_table_index];

	//than we do a memcpy to store the key firstly followed by its associated value
	memcpy(VIRTUAL_SPACE_ADDRESS + sizeof(Data) + pod_index + key_value_pair_offset, key, MAX_LENGTH_KEY);
	memcpy(VIRTUAL_SPACE_ADDRESS + sizeof(Data) + pod_index + key_value_pair_offset + MAX_LENGTH_KEY, value, MAX_LENGTH_VALUE);

	//update our write counter index
	data_ptr->WRITE_COUNTERS[hash_table_index]++;

	//need to wrap around by 256 to account for the number of pods we have
	data_ptr->WRITE_COUNTERS[hash_table_index] = data_ptr->WRITE_COUNTERS[hash_table_index] % NUMBER_OF_PODS;

	//release exclusive access
	sem_post(db);

	return(0);
}

/*
 * FUNCTION NAME: kv_store_read
 * FUNCTION_DESCRIPTION: Takes a key and searches the store for the key-value pair
 * If found, it returns a copy of the values. It duplicates the string found in the store and returns a pointer to said string.
 * If no key-value pair is found, it returns NULL
 */

char *kv_store_read(const char *key){
	// implement your create method code here
	int pod_index;
	int rc;
	char *duplicated_ptr;
	size_t pod_offset;
	size_t pair_offset_in_pod;
	Data *book_keeping;

	//get exclusive access to rc
	sem_wait(mutex);

	//one more reader now
	book_keeping = (Data *)VIRTUAL_SPACE_ADDRESS;
	book_keeping->read_counter++;
	rc = book_keeping->read_counter;

	//if this is the first reader
	if(rc == 1)
		sem_wait(db);

	//release exclusive access to rc
	sem_post(mutex);

	//First we calculate the index within the pods we need
	pod_index = generate_hash(key);

	//Next we need the offset needed to get exact location in the pod for the value with the key
	pod_offset = POD_INDEXING * pod_index;

	//now we cycle through all 256 possible locations to find value
	for(int i = 0; i < NUMBER_OF_PODS; i++){

		//we next need the pair offset within the pod
		pair_offset_in_pod = (MAX_LENGTH_KEY + MAX_LENGTH_VALUE)*book_keeping->READ_COUNTERS[pod_index];

		//next we need to compare the contents of the shared memory with the key to see of the values match
		//		memory_comparison = memcmp(VIRTUAL_SPACE_ADDRESS + sizeof(Data) + pod_offset + pair_offset_in_pod, key, strlen(key));

		//if the value of memory_comparison == 0, we found our value
		if(memcmp(VIRTUAL_SPACE_ADDRESS + sizeof(Data) + pod_offset + pair_offset_in_pod, key, strlen(key)) == 0){
			duplicated_ptr = strdup(VIRTUAL_SPACE_ADDRESS + sizeof(Data) + pod_offset + pair_offset_in_pod + MAX_LENGTH_KEY);

			//update
			book_keeping->READ_COUNTERS[pod_index]++;

			//wrap around
			book_keeping->READ_COUNTERS[pod_index] = book_keeping->READ_COUNTERS[pod_index] % 256;

			//get exclusive access to rc
			sem_wait(mutex);

			//one reader fewer now
			book_keeping->read_counter--;
			rc = book_keeping->read_counter;

			//if this is the last reader
			if(rc == 0)
				sem_post(db);

			//release exclusive access to rc
			sem_post(mutex);

			//return pointer
			return duplicated_ptr;
		}

		//update
		book_keeping->READ_COUNTERS[pod_index]++;

		//wrap around
		book_keeping->READ_COUNTERS[pod_index] = book_keeping->READ_COUNTERS[pod_index] % 256;

	}

	//get exclusive access to rc
	sem_wait(mutex);

	//one reader fewer now
	book_keeping->read_counter--;
	rc = book_keeping->read_counter;

	//if this is the last reader
	if(rc == 0){
		sem_post(db);
	}

	//release exclusive access to rc
	sem_post(mutex);

	return NULL;
}

/*
 * FUNCTION NAME: kv_store_read_all
 * FUNCTION DESCRIPTION: Function takes a key and returns all values in the store.
 * A NULL is returned if there is no record of the key
 */

char **kv_store_read_all(const char *key){
	// implement your create method code here
	int rc;
	int c;
	int pod_index;
	int read_index_pointer;
	int string_comparison_test;
	char **all_key_values;
	char *ptr;
	size_t pod_offset;
	size_t key_value_pair_offset;
	Data *book_keeping;

	//get exclusive access to rc
	sem_wait(mutex);

	//one more reader now
	book_keeping = (Data *)VIRTUAL_SPACE_ADDRESS;
	book_keeping->read_counter++;
	rc = book_keeping->read_counter;

	//if this is the first reader
	if(rc == 1){
		sem_wait(db);
	}

	//release exclusive access to rc
	sem_post(mutex);

	//allocate the pointer that holds all values on the heapS
	all_key_values = malloc(sizeof(char *));

	//get pod index
	pod_index = generate_hash(key);
	c = 0;

	//need pointer to current positon
	read_index_pointer = book_keeping->READ_COUNTERS[pod_index];

	//calculate the off set needed to access element within the pod
	pod_offset = POD_INDEXING * pod_index;

	//cycle through the pods and check the keys
	for(int i = 0; i < NUMBER_OF_PODS; i++){

		//provide offset needed to access next key_value pair
		key_value_pair_offset = read_index_pointer * 288;

		//update the read pointer
		read_index_pointer++;

		//we need to ensure it stays within [0, 256)
		read_index_pointer = read_index_pointer % NUMBER_OF_PODS;

		//now we check the memory locations to get the values and compare with our key
		string_comparison_test = strcmp(VIRTUAL_SPACE_ADDRESS + sizeof(Data) + pod_offset + key_value_pair_offset, key);
		if(string_comparison_test == 0){

			//we will now go through each value and store into our all_value pointer
			c++;
			all_key_values = realloc(all_key_values , sizeof(char *)*(c+1));

			//we will now copy this into ptr
			ptr = strdup(VIRTUAL_SPACE_ADDRESS + sizeof(Data) + pod_offset + key_value_pair_offset + MAX_LENGTH_KEY);

			//store in all_key_values
			all_key_values[c -1] = ptr;
		}
	}

	//get exclusive access to rc
	sem_wait(mutex);

	//one reader fewer now
	book_keeping->read_counter--;
	rc = book_keeping->read_counter;

	//this is the last reader
	if(rc == 0){
		sem_post(db);
	}

	//release exclusive access to rc
	sem_post(mutex);

	//if here, set to NULL, and return
	all_key_values[c] = NULL;
	if(c  == 0){
		return(NULL);
	}
	return all_key_values;
}

/*
 * FUNCTION NAME: kv_delete_db
 * FUNCTION DESCRIPTION: This function deletes the shared memory when called
 */

int kv_delete_db(){

	//unlink the named semaphores
	sem_unlink("db");
	sem_unlink("mutex");

	//removes any memory maps
	if(munmap(VIRTUAL_SPACE_ADDRESS, sizeof(Data) + (NUMBER_OF_PODS* POD_INDEXING)) == -1){
		perror("Could not delete store");
		return(-1);
	}

	shm_unlink(DATA_BASE_NAME);

	return(0);
}

/* -------------------------------------------------------------------------------
	MY MAIN:: Use it if you want to test your impementation (the above methods)
	with some simple tests as you go on implementing it (without using the tester)
	-------------------------------------------------------------------------------
 */
//int main() {
//	int a = kv_store_create("anza");
//	printf("a = %d \n",a);
//
//	int x = kv_store_write("student","Muhammad");
//	printf("x = %d \n",x);
//
//	char *key = kv_store_read("hello");
//	printf("%s \n",key);
//	return EXIT_SUCCESS;
//
//}
