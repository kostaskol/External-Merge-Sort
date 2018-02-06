#ifndef SORTED_H
#define SORTED_H
#include "record.h"

#define FILE_TYPE_OFFSET BLOCK_SIZE / sizeof(int) - 1
#define HEAP_FILE 256
#define SORTED_FILE_OFFSET BLOCK_SIZE / sizeof(int) - 2
#define FILE_SORTED 255
#define FILE_NOT_SORTED 254
#define SORTED_BY_OFFSET BLOCK_SIZE / sizeof(int) - 3
#define MAX_RECORDS BLOCK_SIZE / sizeof(Record) - 1
#define FILLED_OFFSET BLOCK_SIZE / sizeof(int) - 1

int get_new_block(int file_id);

void write_block(int file_id, int block_num);

void *read_block(int file_id, int block_num);

int Sorted_CreateFile(const char *fileName);

int Sorted_OpenFile(const char *fileName);

int Sorted_CloseFile(const int file_desc);

/**
 * Saves all of the records in a serial manner
 */
int Sorted_InsertEntry(int fileDesc, Record record);

/**
 * Sorts the file
 */
int Sorted_SortFile(const char *fileName, int fieldNo);

/**
 * Checks whether the given file is sorted
 */
int Sorted_CheckSortedFile(const char *fileName, int fieldNo);

/**
 * Prints out:
 * - The number of read blocks
 *  - The entries whose *fieldNo (0, 1, 2, 3) is equal to *value
 *  - All entries if value is NULL
 */
void Sorted_GetAllEntries(int fileDesc, int *fieldNo, void *value);

#endif // SORTED_H
