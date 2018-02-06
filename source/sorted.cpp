#include "../headers/record.h"
#include "../headers/sorted.h"
#include "../headers/u_functions.h"
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>

/*
 * The next 3 functions handle the most often used BF operations
 */
int get_new_block(int file_id) {
  if (BF_AllocateBlock(file_id) < 0) {
    std::cerr << "Error allocating block!" << std::endl;
    BF_PrintError("Allocation");
    exit(1);
  }

  int new_block;
  if ((new_block = BF_GetBlockCounter(file_id)) < 0) {
    std::cerr << "Error getting block counter" << std::endl;
    BF_PrintError("Block counter!");
    exit(1);
  }

  return --new_block;
}

void write_block(int file_id, int block_num) {
  if (BF_WriteBlock(file_id, block_num) < 0) {
    std::cerr << "Error writing block #" << block_num << std::endl;
    BF_PrintError("Write");
    exit(1);
  }
}

void *read_block(int file_id, int block_num) {
  void *beg;
  if (BF_ReadBlock(file_id, block_num, &beg) < 0) {
    std::cerr << "Error reading block #" << block_num << std::endl;
    BF_PrintError("Read");
    exit(1);
  }
  return beg;
}

/*
 * Creates a heap file and sets its type to HEAP_FILE
 * and an indicator that it is not sorted
 */
int Sorted_CreateFile(const char *filename) {
  if (BF_CreateFile(filename) < 0) {
    BF_PrintError("Error in creation");
    return -1;
  }

  int file_desc;
  if ((file_desc = BF_OpenFile(filename)) < 0) {
    BF_PrintError("Error in opening the file");
    return -1;
  }

  int new_block = get_new_block(file_desc);

  /*
   * Upon file creation, we set the file type to be a
   * heap file and we
   */
  void *beg = read_block(file_desc, new_block);

  int *type_offset = (int *)beg + FILE_TYPE_OFFSET;
  *type_offset = HEAP_FILE;

  int *sorted_offset = (int *)beg + SORTED_FILE_OFFSET;
  *sorted_offset = FILE_NOT_SORTED;

  write_block(file_desc, new_block);
  return 0;
}

/*
 * Creates a file, sets its type to a HEAP_FILE, an indicator that it is sorted,
 * and the field by which it is sorted
 */
int create_sorted_file(const char *filename, int fieldNo) {
  if (BF_CreateFile(filename) < 0) {
    BF_PrintError("Error in creation");
    return -1;
  }

  int file_desc;
  if ((file_desc = BF_OpenFile(filename)) < 0) {
    BF_PrintError("Error in opening the file");
    return -1;
  }

  int new_block = get_new_block(file_desc);

  void *beg = read_block(file_desc, new_block);

  int *type_offset = (int *)beg + FILE_TYPE_OFFSET;
  *type_offset = HEAP_FILE;

  int *sorted_offset = (int *)beg + SORTED_FILE_OFFSET;
  *sorted_offset = FILE_SORTED;

  int *sorted_by_offset = (int *)beg + SORTED_BY_OFFSET;
  *sorted_by_offset = fieldNo;

  write_block(file_desc, new_block);
  return 0;
}

/*
 * Opens a file and returns its file descriptor
 */
int Sorted_OpenFile(const char *filename) {
  int file_desc;
  if ((file_desc = BF_OpenFile(filename)) < 0) {
    BF_PrintError("Error in opening (Sorted_OpenFile)");
    return -1;
  }
  return file_desc;
}

/*
 * Simply closes the file
 */
int Sorted_CloseFile(const int file_desc) { return BF_CloseFile(file_desc); }

/*
 * Inserts the given record into the given file
 */
int Sorted_InsertEntry(int file_desc, Record record) {
  int block_num = BF_GetBlockCounter(file_desc) - 1;
  // We first check if a block other that the description
  // block has been created
  if (block_num <= 0) {
    block_num = get_new_block(file_desc);
  }

  // Then we check if there is enough room for one more
  // record to be saved on the current block
  void *beg = read_block(file_desc, block_num);

  int *filled_spots = (int *)beg + FILLED_OFFSET;

  // If not, we allocate a new block, update
  // all the pointers to the current block (for later use)
  // and continue
  if (*filled_spots == MAX_RECORDS) {
    block_num = get_new_block(file_desc);
    beg = read_block(file_desc, block_num);
    filled_spots = (int *)beg + FILLED_OFFSET;
  }

  // We save the record
  save_record(record, *filled_spots, beg);

  // and increase the block's filled spots
  (*filled_spots)++;
  write_block(file_desc, block_num);

  return 0;
}

void print_file_filled(int file_desc) {
  int max = BF_GetBlockCounter(file_desc);
  for (int i = 1; i < max; i++) {
    void *beg = read_block(file_desc, i);
    int *filled = (int *)beg + FILLED_OFFSET;
    // std::cout << "Block #" << i << "\nFilled: " << *filled << std::endl;
  }
}

/*
 * Sorts a given heap file
 */
extern int Sorted_SortFile(const char *filename, int fieldNo) {
  if (fieldNo > 3 && fieldNo < 0) {
    std::cerr << "Unknown field number. Exiting..." << std::endl;
    return -1;
  }
  system("exec mkdir ../tmp_files");
  std::cout << "Sorting file: " << filename << std::endl;
  int file_desc;
  if ((file_desc = BF_OpenFile(filename)) < 0) {
    BF_PrintError("Error opening heap file");
    return -1;
  }

  // First, we check whether the file is a heap file
  void *beginning = read_block(file_desc, 0);
  int *file_type = (int *)beginning + FILE_TYPE_OFFSET;
  if (*file_type != HEAP_FILE) {
    std::cerr << "Given file is not a heap file. Exiting..." << std::endl;
    return -1;
  }

  // Then, we check whether the file is already sorted.
  int *sorted_offset = (int *)beginning + SORTED_FILE_OFFSET;
  if (*sorted_offset == FILE_SORTED) {
    int *sorted_by = (int *)beginning + SORTED_BY_OFFSET;
    if (*sorted_by == fieldNo) {
      std::cout << "File already sorted by " + field_number_value(fieldNo)
                << ". Nothing to do. Exiting..." << std::endl;
      return 0;
    }
  }

  int n = BF_GetBlockCounter(file_desc);

  // If there is an odd number of max blocks, we will be creating
  // max_blocks + 1 files, since we need an even number of files
  int max_files = n % 2 == 0 ? n : n + 1;
  int curr_run = 0;

  // We create <max_files> files (and keep a vector of the names)
  std::vector<std::string> file_names = create_files(max_files, curr_run);

  // We need the last output file's name because it's the one that is sorted
  std::string last_outp_file_name;

  // We need an input buffer to fill and flush
  Record *buffer = new Record[BUFFER_SIZE];

  // We start at the first block that contains records.
  // We fill each of the newly created files' first block with
  // one block from the unsorted input file
  for (int block_number = 1; block_number < n; block_number++) {
    beginning = read_block(file_desc, block_number);
    int block_size;

    // Fill the buffer with the next block of the starting heap file
    fill_buffer(buffer, beginning, &block_size);

    // Sort it
    merge_sort(buffer, 0, block_size - 1, fieldNo);

    int tmp_desc;

    // Open the next temporary file
    if ((tmp_desc = BF_OpenFile(file_names[block_number - 1].c_str())) < 0) {
      BF_PrintError("Error opening file");
      return -1;
    }

    // And flush the buffer into its first block
    flush_buffer(buffer, tmp_desc, block_size);

    BF_CloseFile(tmp_desc);
    print_file_filled(file_desc);
  }

  delete[] buffer;

  do {
    // After each run, we will have ceil(n / 2) output files
    n = (int)ceil(n / 2);

    // same logic as above
    max_files = n % 2 == 0 ? n : n + 1;
    int output_file_num = 0;

    // Create output files
    create_files(max_files, ++curr_run);

    for (int file_num = 0; file_num < n * 2; file_num += 2) {

      // Open 2 input and one output file
      std::string inp1_name = get_tmp_file_name(file_num, curr_run - 1);
      std::string inp2_name = get_tmp_file_name(file_num + 1, curr_run - 1);
      std::string outp_name = get_tmp_file_name(output_file_num++, curr_run);

      int inp1_desc, inp2_desc, outp_desc;
      inp1_desc = BF_OpenFile(inp1_name.c_str());
      inp2_desc = BF_OpenFile(inp2_name.c_str());
      outp_desc = BF_OpenFile(outp_name.c_str());

      if (inp1_desc < 0 || inp2_desc < 0 || outp_desc < 0) {
        BF_PrintError("Unable to open file");
        return -1;
      }

      // Merge the two input files into one output file
      merge_into_block(inp1_desc, inp2_desc, outp_desc, fieldNo);

      // Keep track of the last output file's name
      last_outp_file_name = outp_name;

      // If (for any reason) the temporary output file is not sorted,
      // we don't continue with the sorting. Used for debugging.
      // Comment it out/remove it for the programme to run faster
      if (Sorted_CheckSortedFile(inp1_name.c_str(), fieldNo) == 1) {
        std::cerr << "Temporary file not sorted. Exiting..." << std::endl;
        return -1;
      }

      // Close the temporary files
      BF_CloseFile(inp1_desc);
      BF_CloseFile(inp2_desc);
      BF_CloseFile(outp_desc);
    }

    // After all the input files have been merged into output ones, delete them
    remove_input_files(n * 2, curr_run - 1);

    // We stop once n / 2 == 1
  } while (n != 1);

  // We create a new file where we will copy the sorted file
  // (plus the extra information we need)
  create_sorted_file(get_sorted_file_name(filename, fieldNo), fieldNo);

  // We copy each of the last output file's records into the final Sorted file
  int outp_file_desc = Sorted_OpenFile(get_sorted_file_name(filename, fieldNo));

  if ((file_desc = BF_OpenFile(last_outp_file_name.c_str())) < 0) {
    BF_PrintError("Error opening file");
    return -1;
  }

  int last_file_max_blocks = BF_GetBlockCounter(file_desc);

  for (int last_file_block_num = 0; last_file_block_num < last_file_max_blocks;
       last_file_block_num++) {

    void *beg = read_block(file_desc, last_file_block_num);
    int *filled_spots = (int *)beg + FILLED_OFFSET;
    for (int rec_num = 0; rec_num < *filled_spots; rec_num++) {
      Record rec = get_record(rec_num, beg);
      Sorted_InsertEntry(outp_file_desc, rec);
    }
  }

  // We delete the tmp file folder
  system("exec rm -rf ../tmp_files");
  return 0;
}

/*
 * Goes through the whole file. If a record's <fieldNo> value is greater
 * than the previous record's value, we return false. Otherwise, we return true
 */
extern int Sorted_CheckSortedFile(const char *filename, int fieldNo) {

  if (fieldNo > 3 && fieldNo < 0) {
    std::cerr << "Unknown field number. Exiting..." << std::endl;
    return -1;
  }
  int file_desc;
  if ((file_desc = BF_OpenFile(filename)) < 0) {
    BF_PrintError("Error opening file");
    return -1;
  }

  int max_blocks = BF_GetBlockCounter(file_desc);
  if (max_blocks == 0) {
    std::cerr << "Given file is empty" << std::endl;
    return -1;
  }

  int first_block = 0;
  void *beg = read_block(file_desc, 0);

  int *file_type = (int *)beg + FILE_TYPE_OFFSET;
  if (*file_type == HEAP_FILE && max_blocks > 1) {
    first_block = 1;
  }

  // If the file is indicated to be sorted but by a different fieldNo,
  // we notify the user and continue at their command only
  int *sorted_file = (int *)beg + SORTED_FILE_OFFSET;
  if (*sorted_file == FILE_SORTED) {
    int *sorted_by = (int *)beg + SORTED_BY_OFFSET;
    if (*sorted_by != fieldNo) {
      std::cout << "The file seems to be sorted by "
                << field_number_value(*sorted_by)
                << " but the check is being performed for "
                << field_number_value(fieldNo)
                << ". The result will not be accurate. Continue? [y/n]"
                << std::endl;
      char inp;
      do {
        std::cin >> inp;
        switch (inp) {
        case 'N':
        case 'n':
          return -1;
        default:
          std::cout << "[y/n]" << std::endl;
        }
      } while (inp != 'n' && inp != 'N' && inp != 'y' && inp != 'Y');
    }
  }
  int *filled_spots;
  Record curr_record;
  Record prev_record;
  // Check each record contained inside the file.
  for (int block_num = first_block; block_num < max_blocks; block_num++) {
    beg = read_block(file_desc, block_num);
    filled_spots = (int *)beg + FILLED_OFFSET;
    prev_record = get_record(0, beg);
    for (int record_num = 1; record_num < *filled_spots; record_num++) {
      curr_record = get_record(record_num, beg);
      if (!checkLessThan(prev_record, curr_record, fieldNo) &&
          !checkEqual(prev_record, curr_record, fieldNo)) {
        std::cerr << "File not sorted" << std::endl;
        return -1;
      }
      copy_record(&prev_record, curr_record);
    }
  }
  BF_CloseFile(file_desc);
  return 0;
}

void Sorted_GetAllEntries(int file_desc, int *fieldNo, void *value) {
  if (*fieldNo > 3 && *fieldNo < 0) {
    std::cerr << "Unknown field number. Exiting..." << std::endl;
    return;
  }
  int max_blocks = BF_GetBlockCounter(file_desc);
  int records_found = 0;
  int records_read = 0;
  void *beg;
  int *filled_spots;
  int starting_block = 0;
  beg = read_block(file_desc, 0);
  int *sorted_offset = (int *)beg + SORTED_FILE_OFFSET;
  if (*sorted_offset != FILE_SORTED) {
    std::cerr << "Given file is not sorted. Binary search will "
                 "not work. Exiting..."
              << std::endl;
    return;
  }

  int *sorted_by = (int *)beg + SORTED_BY_OFFSET;
  if (*sorted_by != *fieldNo) {
    std::cerr << "Given file is sorted by " << field_number_value(*sorted_by)
              << " while the requested field number is "
              << field_number_value(*fieldNo)
              << ". The results will not be accurate. Exiting..." << std::endl;
    return;
  }
  int *file_type = (int *)beg + FILE_TYPE_OFFSET;
  if (*file_type == HEAP_FILE) {
    starting_block = 1;
  }

  /*
   * We print all of the records
   */
  if (value == NULL) {
    for (int block_num = starting_block; block_num < max_blocks; block_num++) {
      beg = read_block(file_desc, block_num);
      filled_spots = (int *)beg + FILLED_OFFSET;
      if (*filled_spots == 0) {
        std::cerr << "Filled spots are 0" << std::endl;
        return;
      }
      for (int record_index = 0; record_index < *filled_spots; record_index++) {
        records_found++;
        records_read++;
        Record rec = get_record(record_index, beg);
        print_record(rec);
      }
    }

    // We perform binary search until we find one or more records that match the
    // given value, or if none exists
  } else {
    int lowest = 0;
    int highest = max_blocks;
    int middle;
    Record rec;
    bool found = false;
    bool exists = true;
    while ((highest - lowest) != 0) {
      middle = (int)ceil((highest + lowest) / 2);
      beg = read_block(file_desc, middle);
      records_read++;
      rec = get_record(0, beg);
      /*
       * If the first record's fieldNo is equal to the given
       * value, we have found it, so we print the surrounding
       * records
       */
      if (checkEqual(rec, value, *fieldNo)) {
        int tmp_rec_read;
        records_found = print_surrounding_records(middle, file_desc, 0, value,
                                                  *fieldNo, &tmp_rec_read);
        records_read += tmp_rec_read;
        found = true;

        /*
         * If it's greater than the value, we go to the first half
         * of the block span and check again
         */
      } else if (!checkLessThan(rec, value, *fieldNo)) {
        highest = middle;

        /*
         * If it's less than the value, we search inside the block
         * (in a serial manner). If we find it, we print the
         * surrounding records
         */
      } else {
        filled_spots = (int *)beg + FILLED_OFFSET;
        for (int rec_num = 1; rec_num < *filled_spots; rec_num++) {
          rec = get_record(rec_num, beg);
          if (checkEqual(rec, value, *fieldNo)) {
            int tmp_rec_read;
            records_found = print_surrounding_records(
                middle, file_desc, rec_num, value, *fieldNo, &tmp_rec_read);
            records_read += tmp_rec_read;
            found = true;
          } else if (!checkLessThan(rec, value, *fieldNo)) {

            /*
             * If, while inside a block whose first record
             * was less than the value we are searching
             * for, we reach a record with a greater value
             * while *not* having found a record,
             * we know that the records does not exist
             */
            exists = false;
            break;
          }
        }
        if (!exists || found) {
          break;
        }

        /*
         * If we have searched through the whole block
         * and have not found the record we are looking for,
         * we search in the upper half of the block span
         */

        lowest = middle;
      }
    }
    std::cout << "Max blocks are: " << max_blocks << std::endl;

    if (found) {
      if (records_found > 1) {
        std::cout << "Found " << records_found << " records" << std::endl;
      } else {
        std::cout << "Found 1 record" << std::endl;
      }
    } else if (!exists) {
      std::cout << "No records could be found with the requested";
    }

    std::cout << "Read " << records_read << " records" << std::endl;
  }
}
