#include "../kvdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main() {
  //remove("a.db");
  //remove("a.txt");
  struct kvdb *db;
  char key[1024];
  char value[1024];
  char *ret;
  db=kvdb_open("a.db");
  char choice[128];
  while(1)
  {
    scanf("%s",choice);
    if(strcmp(choice,"put")==0)
    {
    printf("enter key:\n");
    scanf("%s",key);
    printf("enter value:\n");
    scanf("%s",value);
    kvdb_put(db,key,value);
    }
    else if(strcmp(choice,"get")==0)
    {
      ret=kvdb_get(db,key);
      printf("[%s]:[%s]\n",key,ret);
    }
  }
  //kvdb_put(db, key, "three-easy-pieces"); // db[key] = "three-easy-pieces"
  //value = kvdb_get(db, key); // value = db[key];
  kvdb_close(db); // 关闭数据库
  printf("[%s]: [%s]\n", key, value);
  free(value);
  return 0;
}
