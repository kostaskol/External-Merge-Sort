#ifndef U_FUNCTIONS_H
#define U_FUNCTIONS_H

#include "record.h"
#include <string>
#include <vector>

extern "C" {
#include "BF.h"
};
#define BUFFER_SIZE MAX_RECORDS

void merge(Record *arr, int l, int m, int r, int fieldNo);

void merge_sort(Record *arr, int l, int r, int fieldNo);

void merge_into_block(int inp1_fd, int inp2_fd, int outp, int fieldNo);

void flush_buffer(Record *buf, int file_desc, int max);

void fill_buffer(Record *buffer, void *beg, int *size);

std::string get_tmp_file_name(int file_num, int curr_run);

void remove_input_files(int n, int curr_run);

std::vector<std::string> create_files(int num, int run);

char *get_sorted_file_name(const char *file_name, int fieldNo);

int print_surrounding_records(int curr_block, int file_desc, int curr_rec_index,
                              void *value, int fieldNo, int *rec_read);

std::string field_number_value(int fieldNo);
#endif // U_FUNCTIONS_H
