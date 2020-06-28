#include "../kvdb.h"
#include <stdio.h>
#include <stdlib.h>
int main() {
  remove("a.db");
  remove("a.txt");
  struct kvdb *db;
  const char *key = "operating-systems";
  char *value;
  db=kvdb_open("a.db");
  kvdb_put(db, key, "three-easy-pieces"); // db[key] = "three-easy-pieces"
  value = kvdb_get(db, key); // value = db[key];
  kvdb_close(db); // 关闭数据库
  printf("[%s]: [%s]\n", key, value);
  free(value);
  return 0;
}
