#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <pthread.h>

struct kvdb {
  int data_fd;
  int jnl_fd;
  // your definition here
};
typedef struct kvdb kvdb_t;

struct kvdb *kvdb_open(const char *filename) {//把log和数据库分开存放
  char logname[128];
  for(int i=0;i<1000;i++)
  {
    if(filename[i]!='.')
      logname[i]=filename[i];
    else
     {
       logname[i]='\0';
       strcat(logname[i],".txt");
       break;
     }
  }
  
  int fd1=open(filename,O_WRONLY|O_CREAT);
  int fd2=open(logname,O_WRONLY|O_CREAT);
  kvdb_t* ptr=(kvdb_t *)malloc(sizeof(kvdb_t));
  ptr->data_fd=fd1;
  ptr->jnl_fd=fd2;
  return NULL;
}

int kvdb_close(struct kvdb *db) {
  return -1;
}

int kvdb_put(struct kvdb *db, const char *key, const char *value) {
  return -1;
}

char *kvdb_get(struct kvdb *db, const char *key) {
  return NULL;
}
