#include "../headers/BF.h"
#include "../headers/record.h"
#include "../headers/sorted.h"
#include <assert.h>
#include <cstring>
#include <iostream>

/*
 * Returns true if the first record is less (according to <fieldNo>)
 * than the second record.
 */
extern bool checkLessThan(Record rec, Record other, int fieldNo) {
  switch (fieldNo) {
  case 0:
    return rec.id < other.id;
  case 1:
    return (strcmp(rec.name, other.name) < 0);
  case 2:
    return (strcmp(rec.surname, other.surname) < 0);
  case 3:
    return (strcmp(rec.city, other.city) < 0);
  default:
    // Should never be in here
    std::cerr << "Unknown field number" << std::endl;
    return false;
  }
}

/*
 * Same as above, but works for a given value instead of another record
 */
extern bool checkLessThan(Record rec, void *value, int fieldNo) {
  switch (fieldNo) {
  case 0:
    return rec.id < *(int *)value;
  case 1:
    return (strcmp(rec.name, (char *)value) < 0);
  case 2:
    return (strcmp(rec.surname, (char *)value) < 0);
  case 3:
    return (strcmp(rec.city, (char *)value) < 0);
  default:
    std::cerr << "Unknown field number" << std::endl;
    return false;
  }
}

/*
 * Same as above, but returns true if the two records are equal
 */
extern bool checkEqual(Record rec, Record other, int fieldNo) {
  switch (fieldNo) {
  case 0:
    return rec.id == other.id;
  case 1:
    return (strcmp(rec.name, other.name) == 0);
  case 2:
    return (strcmp(rec.surname, other.surname) == 0);
  case 3:
    return (strcmp(rec.city, other.city) == 0);
  default:
    // Should never be in here
    std::cerr << "Unknown field number" << std::endl;
    return false;
  }
}

/*
 * Same as above, but returns true if the record's <fieldNo> is equal to the
 * given value
 */
extern bool checkEqual(Record rec, void *value, int fieldNo) {
  switch (fieldNo) {
  case 0:
    return rec.id == *(int *)value;
  case 1:
    return (strcmp(rec.name, (char *)value) == 0);
  case 2:
    return (strcmp(rec.surname, (char *)value) == 0);
  case 3:
    return (strcmp(rec.city, (char *)value) == 0);
  default:
    std::cerr << "Unknown field number" << std::endl;
    return false;
  }
}

/*
 * Saves the record at offset <offset> of a given block
 */
extern void save_record(Record record, int offset, void *beg) {
  memcpy(((Record *)beg + offset), &record, sizeof(record));
}

/*
 * Returns the record at offset <offset> of a given block
 * also checks that the requested offset is not out of bounds
 */
extern Record get_record(int offset, void *beg) {
  assert(offset >= 0 && offset < MAX_RECORDS);
  Record *rec = (Record *)beg + offset;
  return *rec;
}

/*
 * Copies the record <other> to record <record>
 */
extern void copy_record(Record *record, const Record other) {
  record->id = other.id;
  strcpy(record->name, other.name);
  strcpy(record->surname, other.surname);
  strcpy(record->city, other.city);
}

/*
 * Prints the given record
 */
extern void print_record(Record rec) {
  std::cout << "Printing Record: "
            << "\nID: " << rec.id << "\nNAME: " << rec.name
            << "\nSURNAME: " << rec.surname << "\nCITY: " << rec.city
            << std::endl;
}
