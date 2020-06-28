#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#define DATA_OFFSET 1<<20 //数据区的起始位置,留1MB记录偏移量,可记录1MB/32yte=很多很多个
#define LOG_SIZE 34
#define LOG_MSG(i) LOG_SIZE*i//假设每条信息33Byte,即每个长度可以用16个字符记录+1个有效位
#define REC_MSG(i) LOG_SIZE*i

#define VALID 147//选取256以内的某两个数代表valid和invalid
#define INVALID 82
#define ENDCHAR 100
/*
对应metadata journaling:
文件系统的信息--->key的offset和值的offset
数据块--->键值对
log--->仅仅写入offset不写入键值对本身
*/

void may_crash()
{

}


struct kvdb {
  int data_fd;
  int jnl_fd;
  int max_offset;//数据区的最大offset,也即下一个写入的位置
  int nr_log;//在log里记录到的所有有过的MSG用到的总空间
  int nr_rec;//在数据库里记录到所有有过的MSG用到的总空间
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
       strcat(logname,".txt");
       break;
     }
  }
  
  int fd1=open(filename,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
  int fd2=open(logname,O_WRONLY|O_CREAT,S_IRUSR|S_IWUSR);
  kvdb_t* ptr=(kvdb_t *)malloc(sizeof(kvdb_t));
  ptr->data_fd=fd1;
  ptr->jnl_fd=fd2;
  ptr->max_offset=DATA_OFFSET;
  ptr->nr_log = 0;
  ptr->nr_rec = 0;
  return NULL;
}

int kvdb_close(struct kvdb *db) {
  return -1;
}

int kvdb_put(struct kvdb *db, const char *key, const char *value) {
  while(flock(db->data_fd,EX)==0);
  while(flock(db->jnl_fd,EX)==0);
  int key_len=strlen(key);
  int val_len=strlen(value);
  
  lseek(db->data_fd,db->max_offset,SEEK_SET);
  write(db->data_fd,key,key_len);
  write(db->data_fd,val,val_len);

  may_crash();
  fsync(db->data_fd);

  char klen[16];
  char vlen[16];
  sprintf(klen,"%d",key_len);
  sprintf(vlen,"%d",val_len);
  for(int i=0;i<16;i++)
  {
    if(!(klen[i]>='0'&&klen[i]<='9'))
      klen[i]=0x20;
    if(!(vlen[i]>='0'&&klen[i]<='9'))
      vlen[i]=0x20;
  }
  for(int i=0;;i++)
  {
    lseek(db->jnl_fd,LOG_MSG(i),SEEK_SET);
    if(LOG_MSG==db->nr_msg*LOG_SIZE)//尚未写过的地方可以直接写
      {
        nr_msg=nr_msg+1;
        break;
      }
    char buf[2];
    read(db->jnl_fd,buf,1);
    if(buf[0]==VALID) break;
  }
  char validch[1]={(char)VALID};
  char endch[1]={(char)ENDCHAR};
  char writebuf[LOG_SIZE];
  writebuf[0]=INVALID;
  sprintf(writebuf+1,"%s%s",klen,vlen);
  
  write(db->jnl_fd,writebuf,LOG_SIZE);
  may_crash();
  write(db->jnl_fd,endch,1);
  may_crash();
  fsync();

  //这里是在数据库文件里写,类似文件系统信息,但是和上面写的内容一致
  for(int i=0;;i++)
  {
    lseek(db->jnl_fd,LOG_MSG(i),SEEK_SET);
    if(LOG_MSG==db->nr_msg*LOG_SIZE)//尚未写过的地方可以直接写
      {
        nr_msg=nr_msg+1;
        break;
      }
    char buf[2];
    read(db->jnl_fd,buf,1);
    if(buf[0]==VALID) break;
  }
  char validch[1]={(char)VALID};
  char endch[1]={(char)ENDCHAR};
  char writebuf[LOG_SIZE];
  writebuf[0]=INVALID;
  sprintf(writebuf+1,"%s%s",klen,vlen);
  
  write(db->jnl_fd,writebuf,LOG_SIZE);
  may_crash();
  write(db->jnl_fd,endch,1);
  may_crash();
  fsync();
  

  flock(db->data_fd,UN);
  flock(db->jnl_fd,UN);
  
  return -1;
}

char *kvdb_get(struct kvdb *db, const char *key) {
  while(flock(db->data_fd,EX)==0);
  while(flock(db->jnl_fd,EX)==0);


  
  flock(db->data_fd,UN);
  flock(db->jnl_fd,UN);
  return NULL;
}
