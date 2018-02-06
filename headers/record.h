#ifndef RECORD
#define RECORD

struct Record {
  int id;
  char name[15];
  char surname[20];
  char city[25];
};

bool checkLessThan(Record rec, Record other, int fieldNo);

bool checkLessThan(Record rec, void *value, int fieldNo);

bool checkEqual(Record rec, Record other, int fieldNo);

bool checkEqual(Record rec, void *value, int fieldNo);

void save_record(Record record, int offset, void *beg);

Record get_record(int offset, void *beg);

void copy_record(Record *record, const Record other);

void print_record(Record);
#endif // RECORD
