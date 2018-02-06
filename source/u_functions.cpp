#include "../headers/sorted.h"
#include "../headers/u_functions.h"
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

/*
 * The temporary file names follow the format:
 * tmp_file_<current_run>_<number_of_file>
 */
extern std::string get_tmp_file_name(int file_num, int curr_run) {
  std::string tmp_file_name = "../tmp_files/tmp_file_";
  std::stringstream *ss = new std::stringstream();
  *ss << curr_run;
  tmp_file_name = tmp_file_name + ss->str() + "_";
  delete ss;
  ss = new std::stringstream();
  *ss << file_num;
  tmp_file_name += ss->str();
  delete ss;
  return tmp_file_name;
}

/*
 * Creates <num> temporary files for run <run>
 */
extern std::vector<std::string> create_files(int num, int run) {
  std::vector<std::string> tmp = std::vector<std::string>((ulong)num);
  for (int i = 0; i < num; i++) {
    std::string tmp_name = get_tmp_file_name(i, run);
    if (BF_CreateFile(tmp_name.c_str()) < 0) {
      std::cerr << "Error creating file. Exiting" << std::endl;
      exit(1);
    }
    tmp[i] = tmp_name;
  }
  return tmp;
}

/*
 * Removes the <num> temporary files created for run <run>
 */
extern void remove_input_files(int num, int run) {
  for (int file_num = 0; file_num < num; file_num++) {
    std::string tmp_file_name = get_tmp_file_name(file_num, run);
    remove(tmp_file_name.c_str());
  }
}

/*
 * Merges the inp1_fd file and inp2_fd file into the outp_fd file
 * according to <fieldNo>
 */
extern void merge_into_block(int inp1_fd, int inp2_fd, int outp_fd,
                             int fieldNo) {

  int inp1_max_blocks = BF_GetBlockCounter(inp1_fd);
  int inp2_max_blocks = BF_GetBlockCounter(inp2_fd);
  bool reached_inp1_end = false;
  bool reached_inp2_end = false;

  /*
   * The actual sorting will happen in main memory
   * (2 buffers as input and 1 as output)
   */
  Record *inp1_buffer = new Record[BUFFER_SIZE];
  Record *inp2_buffer = new Record[BUFFER_SIZE];
  Record *output_buffer = new Record[BUFFER_SIZE];

  /*
   * It can happen that the second file parameter
   * doesn't contain any records
   * (if we have an odd number of blocks)
   * In that case, copy the first one into the
   * output buffer and flush it
   */
  if (inp2_max_blocks == 0) {
    bool outp_flushed = false;
    void *inp1_beg;
    int output_index = 0;
    for (int block_num = 0; block_num < inp1_max_blocks; block_num++) {
      inp1_beg = read_block(inp1_fd, block_num);
      int inp1_size;
      fill_buffer(inp1_buffer, inp1_beg, &inp1_size);
      // Fill the output buffer with values from the input
      // buffer
      for (int record_num = 0; record_num < inp1_size; record_num++) {
        output_buffer[output_index++] = inp1_buffer[record_num];
        outp_flushed = false;

        // Flush the output buffer once full
        if (output_index == BUFFER_SIZE) {
          flush_buffer(output_buffer, outp_fd, output_index);
          output_index = 0;
          outp_flushed = true;
        }
      }
    }

    // If the input file didn't contain the max number of records,
    // the output buffer will not have been flushed, so we flush it
    // before exiting
    if (!outp_flushed) {
      flush_buffer(output_buffer, outp_fd, output_index);
    }
    return;
  }

  // If both files contain blocks, we merge them into the output file
  if (inp1_max_blocks == 0) {
    std::cerr << "Bad input for file 1" << std::endl;
    return;
  }

  int inp1_curr_block_num = 0;
  int inp2_curr_block_num = 0;

  // Read the first block of each input file
  void *inp1_beg = read_block(inp1_fd, 0);
  void *inp2_beg = read_block(inp2_fd, 0);

  int inp1_size;
  fill_buffer(inp1_buffer, inp1_beg, &inp1_size);
  int inp2_size;
  fill_buffer(inp2_buffer, inp2_beg, &inp2_size);

  int buff1_curr_rec = 0;
  int buff2_curr_rec = 0;
  int outp_curr_rec = 0;

  do {
    // If the first buffer's current value is smaller, add it to the output
    // buffer (simple merge sort)
    if (checkLessThan(inp1_buffer[buff1_curr_rec], inp2_buffer[buff2_curr_rec],
                      fieldNo) ||
        checkEqual(inp1_buffer[buff1_curr_rec], inp2_buffer[buff2_curr_rec],
                   fieldNo)) {
      output_buffer[outp_curr_rec++] = inp1_buffer[buff1_curr_rec++];
      if (buff1_curr_rec == inp1_size) {
        // We have reached the end of buffer1.
        // Fill it with the next block
        if (++inp1_curr_block_num < inp1_max_blocks) {
          // if it exists
          buff1_curr_rec = 0;
          inp1_beg = read_block(inp1_fd, inp1_curr_block_num);

          // Refill the inp1 buffer
          fill_buffer(inp1_buffer, inp1_beg, &inp1_size);
        } else {
          reached_inp1_end = true;
          break;
        }
      }
    } else {
      output_buffer[outp_curr_rec++] = inp2_buffer[buff2_curr_rec++];
      if (buff2_curr_rec == inp2_size) {
        // We have reached the end of buffer 2
        if (++inp2_curr_block_num < inp2_max_blocks) {
          // if it exists
          buff2_curr_rec = 0;
          inp2_beg = read_block(inp2_fd, inp2_curr_block_num);

          // Refill the inp2 array
          fill_buffer(inp2_buffer, inp2_beg, &inp2_size);
        } else {
          reached_inp2_end = true;
          break;
        }
      }
    }

    if (outp_curr_rec == BUFFER_SIZE) {

      // After flushing it,
      // we will insert the next record into the array's
      // first index
      flush_buffer(output_buffer, outp_fd, outp_curr_rec);
      outp_curr_rec = 0;
    }

  } while (true);

  // After breaking, we need to know which of the two files reached
  // its end, and copy the other one into the output array

  // If we've reached inp file 1's end, we need to flush the
  // remaining records of the inp file 2
  bool inp1_flushed = false;
  if (reached_inp1_end) {
    int block_num = inp2_curr_block_num;
    // Iterate over the remaining blocks (if any) and copy them
    // into the output file
    if (outp_curr_rec == BUFFER_SIZE) {
      flush_buffer(output_buffer, outp_fd, outp_curr_rec);
      outp_curr_rec = 0;
    }
    do {
      for (int record_num = buff2_curr_rec; record_num < inp2_size;
           record_num++) {
        // If, at any point while copying, we fill out the
        // output buffer, flush it
        output_buffer[outp_curr_rec++] = inp2_buffer[record_num];
        inp1_flushed = false;
        if (outp_curr_rec == BUFFER_SIZE) {
          flush_buffer(output_buffer, outp_fd, outp_curr_rec);
          // Reset the index into which we will write
          outp_curr_rec = 0;
          inp1_flushed = true;
        }
      }

      if (block_num + 1 == inp2_max_blocks) {
        break;
      }

      // If we run out of records in the current buffer
      // *and* there are remaining blocks in the file,
      // reset everything and copy them over into main memory
      inp2_beg = read_block(inp2_fd, ++block_num);
      fill_buffer(inp2_buffer, inp2_beg, &inp2_size);
      buff2_curr_rec = 0;
    } while (true);
  }
  // If we've reached inp file 2's end, we need to flush the
  // remaining records of the inp file 1 into the output file
  bool inp2_flushed = false;
  if (reached_inp2_end) {
    if (outp_curr_rec == BUFFER_SIZE) {
      flush_buffer(output_buffer, outp_fd, outp_curr_rec);
      outp_curr_rec = 0;
    }
    int block_num = inp1_curr_block_num;
    // Iterate over the remaining blocks (if any) and copy them
    // into the output file
    do {
      for (int record_num = buff1_curr_rec; record_num < inp1_size;
           record_num++) {
        // If, at any point while copying, we fill out the
        // output buffer, flush it
        output_buffer[outp_curr_rec++] = inp1_buffer[record_num];
        inp2_flushed = false;
        if (outp_curr_rec == BUFFER_SIZE) {
          flush_buffer(output_buffer, outp_fd, outp_curr_rec);
          // Reset the index into which we will write
          outp_curr_rec = 0;
          inp2_flushed = true;
        }
      }

      if (block_num + 1 == inp1_max_blocks) {
        break;
      }

      // If we run out of records in the current buffer
      // *and* there are remaining blocks in the file,
      // reset everything and copy them over into main memory
      inp1_beg = read_block(inp1_fd, ++block_num);
      fill_buffer(inp1_buffer, inp1_beg, &inp1_size);
      buff1_curr_rec = 0;
    } while (true);
  }

  // If neither of the input buffers caused the output buffer
  // to be flushed, we flush it now
  if (!inp1_flushed && !inp2_flushed) {
    flush_buffer(output_buffer, outp_fd, outp_curr_rec);
  }

  // Memory cleaning
  delete[] inp1_buffer;
  delete[] inp2_buffer;
  delete[] output_buffer;
}

/*
 * Simple function to flush a given buffer (up until a <max> index)
 */
extern void flush_buffer(Record *buffer, int file_desc, int max) {
  int new_block = get_new_block(file_desc);
  void *outp_beg = read_block(file_desc, new_block);
  int *filled_spots = (int *)outp_beg + FILLED_OFFSET;
  for (int index = 0; index < max; index++) {
    save_record(buffer[index], index, outp_beg);
    (*filled_spots)++;
  }
  write_block(file_desc, new_block);
}

extern void fill_buffer(Record *buffer, void *beg, int *size) {
  int *filled_spots = (int *)beg + FILLED_OFFSET;
  *size = *filled_spots;

  for (int rec_num = 0; rec_num < *filled_spots; rec_num++) {
    buffer[rec_num] = get_record(rec_num, beg);
  }
}

/*
 * The next two algorithms are the simple merge sort algorithm
 * for an array
 */
extern void merge(Record *arr, int l, int m, int r, int fieldNo) {
  int i, j, k;
  int n1 = m - l + 1;
  int n2 = r - m;

  Record L[n1], R[n2];

  for (i = 0; i < n1; i++) {
    L[i] = arr[l + i];
  }

  for (j = 0; j < n2; j++) {
    R[j] = arr[m + 1 + j];
  }

  i = 0;
  j = 0;
  k = l;
  while (i < n1 && j < n2) {
    if (checkLessThan(L[i], R[j], fieldNo) || checkEqual(L[i], R[j], fieldNo)) {
      arr[k] = L[i];
      i++;
    } else {
      arr[k] = R[j];
      j++;
    }
    k++;
  }

  while (i < n1) {
    arr[k] = L[i];
    i++;
    k++;
  }

  while (j < n2) {
    arr[k] = R[j];
    j++;
    k++;
  }
}

extern void merge_sort(Record *arr, int l, int r, int fieldNo) {
  if (l < r) {
    int m = l + (r - l) / 2;
    merge_sort(arr, l, m, fieldNo);
    merge_sort(arr, m + 1, r, fieldNo);

    merge(arr, l, m, r, fieldNo);
  }
}

/*
 * Returns a string with a requested output file name format
 */
extern char *get_sorted_file_name(const char *file_name, int fieldNo) {
  char *new_name = new char[50];
  strcpy(new_name, file_name);
  strcat(new_name, "_Sorted_");
  std::stringstream ss = std::stringstream();
  ss << fieldNo;
  strcat(new_name, ss.str().c_str());
  return new_name;
}

/*
 * After finding a matching record (in Sorted_GetAllEntries())
 * we go through all the records before that one until we find one that
 * is not equal to the value.
 * We also do that for all the records after that one
 */
extern int print_surrounding_records(int curr_block, int file_desc,
                                     int curr_rec_index, void *value,
                                     int fieldNo, int *rec_read) {

  int prev_block = curr_block;
  int next_block = curr_block;
  void *starting_beg = read_block(file_desc, curr_block);
  int curr_rec = curr_rec_index;
  int records_found = 0;
  int records_read = 0;
  /*
   * Get the found record and print it
   */
  Record record = get_record(curr_rec_index, starting_beg);
  /*
   * Go back until we find a record that doesn't match
   */
  void *prev_beg = starting_beg;
  while (checkEqual(record, value, fieldNo)) {
    if (curr_rec < 0) {
      prev_beg = read_block(file_desc, --prev_block);
      curr_rec = *(int *)prev_beg + FILLED_OFFSET;
    }
    record = get_record(curr_rec--, prev_beg);
    records_read++;
    if (checkEqual(record, value, fieldNo)) {
      print_record(record);
      records_found++;
    }
  }
  curr_rec = curr_rec_index + 1;
  void *next_beg = starting_beg;
  record = get_record(curr_rec, prev_beg);
  int *filled_spots = (int *)next_beg + FILLED_OFFSET;

  // Go forward until we find a record that is not equal to the requested
  // value
  while (checkEqual(record, value, fieldNo)) {
    if (curr_rec == *filled_spots) {
      next_beg = read_block(file_desc, ++next_block);
      filled_spots = (int *)next_beg + FILLED_OFFSET;
    }

    record = get_record(curr_rec++, next_beg);
    records_read++;
    if (checkEqual(record, value, fieldNo)) {
      print_record(record);
      records_found++;
    }
  }
  *rec_read = records_read;
  return records_found;
}

extern std::string field_number_value(int fieldNo) {
  switch (fieldNo) {
  case 0:
    return "id";
  case 1:
    return "name";
  case 2:
    return "surname";
  case 3:
    return "city";
  default:
    return "Unknown fieldNo";
  }
}
