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


#define MAGIC_HASH_NUMBER 			5381
#define DATA_BASE_NAME   			"database"

#define MAX_LENGTH_KEY				32
#define MAX_LENGTH_VALUE			256
#define NUMBER_OF_PODS				256
#define MAX_PODS					NUMBER_OF_PODS * 256;
#define POD_INDEXING				NUMBER_OF_PODS * (MAX_LENGTH_KEY + MAX_LENGTH_VALUE) //256 * (256 +32)


////This pointer will hold the address in the VM space of the shared memory object
//char *VIRTUAL_SPACE_ADDRESS;
/*I define a key value pair as a struct with a key string with an associated value string*/
typedef struct
{
	char key[MAX_LENGTH_KEY];
	char value[MAX_LENGTH_VALUE];
}Key_Value_Pair;

typedef struct
{
	int WRITE_COUNTERS[256];
	int READ_COUNTERS[256];
	int read_counter;
	int initiliazed;
}Data;

unsigned long generate_hash(const char *str);

int kv_store_create(const char *name);
int kv_store_write(const char *key, const char *value);
char *kv_store_read(const char *key);
char **kv_store_read_all(const char *key);
int kv_delete_db();


//int kv_store_create(char *kv_store_name);
//int kv_store_write(char *key, char *value);
//char *kv_store_read(char *key);
//char **kv_store_read_all(char *key);
//int kv_delete_db();
#endif
