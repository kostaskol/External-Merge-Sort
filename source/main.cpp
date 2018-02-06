#include "../headers/sorted.h"
#include "../headers/u_functions.h"
#include <assert.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdlib.h>

void create_file(char *fileName) { assert(!Sorted_CreateFile(fileName)); }

int open_file(const char *fileName) {
  int file_desc;
  assert((file_desc = Sorted_OpenFile(fileName)) > 0);
  return file_desc;
}

void insert_Entries(int file_desc, char *csv) {
  FILE *stream = fopen(csv, "rw");
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  Record record;
  while ((read = getline(&line, &len, stream)) != -1) {
    line[read - 2] = 0;
    char *pch;

    pch = strtok(line, ",");
    record.id = atoi(pch);

    pch = strtok(NULL, ",");
    pch++;
    pch[strlen(pch) - 1] = 0;
    strncpy(record.name, pch, sizeof(record.name));

    pch = strtok(NULL, ",");
    pch++;
    pch[strlen(pch) - 1] = 0;
    strncpy(record.surname, pch, sizeof(record.surname));

    pch = strtok(NULL, ",");
    pch++;
    pch[strlen(pch) - 1] = 0;
    strncpy(record.city, pch, sizeof(record.city));

    Sorted_InsertEntry(file_desc, record);
  }
  std::cout << "Inserted everything!" << std::endl;
  free(line);
}

void get_AllEntries(int file_desc, int *fieldNo, void *value) {
  std::cout << "Printing everything" << std::endl;
  Sorted_GetAllEntries(file_desc, fieldNo, value);
}

// We create the starting heap file in the ./io_files folder
#define fileName "./io_files/starting_file"
int main(int argc, char **argv) {

  // and also create the folder
  system("exec mkdir ./io_files");

  BF_Init();

  // -- create index
  char *filename = new char[50];
  strcpy(filename, fileName);

  create_file(filename);

  // -- open index
  int file_desc = open_file(filename);

  // -- insert entries
  insert_Entries(file_desc, argv[1]);

  // We sorted the file by name
  Sorted_SortFile(filename, 0);

  int value = 14289946;

  // We also create the output file in the ./io_files folder
  strcpy(filename, "./io_files/starting_file_Sorted_0");

  if (!Sorted_CheckSortedFile(filename, 0)) {
    std::cout << "Heap file is sorted." << std::endl;
  } else {
    std::cerr << "Heap file is NOT sorted." << std::endl;
  }
  file_desc = Sorted_OpenFile(filename);
  int fieldNo = 0;
  get_AllEntries(file_desc, &fieldNo, &value);

  Sorted_CloseFile(file_desc);

  return EXIT_SUCCESS;
}
