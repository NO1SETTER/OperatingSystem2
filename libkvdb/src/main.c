#include "../kvdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int main() {
  struct kvdb *db;
  char key[1024];
  char value[1024];
  char *ret;
  db=kvdb_open("a.db");
  char choice[128];
  for(int i=0;i<10;i++)
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
      scanf("%s",key);
      ret=kvdb_get(db,key);
      printf("[%s]:[%s]\n",key,ret);
    }
  }
  assert(kvdb_close(db)!=-1);
  return 0;
}
