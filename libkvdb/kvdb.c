#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#define DATA_OFFSET 1<<20 //数据区的起始位置,留1MB记录偏移量,可记录1MB/32yte=很多很多个
#define LOG_SIZE 16
#define LOG_MSG(i) LOG_SIZE*i
#define REC_MSG(i) LOG_SIZE*i

#define FREE 147//选取256以内的某两个数代表FREE和USED
#define USED 82
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
  // your definition here
};
typedef struct kvdb kvdb_t;

struct log//每条log 16 byte
{
  uint8_t status;
  uint32_t klen;
  uint32_t vlen;
  uint32_t offset;
  uint8_t pad[2];//仅用于填空,补足16字节
  uint8_t end;
}__attribute__((packed));
typedef struct log log_t;

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
  int fd1=open(filename,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
  int fd2=open(logname,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR);
  printf("Data_fd=%d jnl_fd=%d\n",fd1,fd2);
  kvdb_t* ptr=(kvdb_t *)malloc(sizeof(kvdb_t));
  ptr->data_fd=fd1;
  ptr->jnl_fd=fd2;
  return ptr;
}

int kvdb_close(struct kvdb *db) {
  return -1;
}

void Int2Str(char *s,uint32_t d)
{
  s[0]=(char)d&0xff;
  s[1]=(char)((d&0xff00)>>8);
  s[2]=(char)((d&0xff0000)>>16);
  s[3]=(char)((d&0xff0000000)>>24);
  s[4]='\0';
}

int kvdb_put(struct kvdb *db, const char *key, const char *value) {
  while(flock(db->data_fd,LOCK_EX)!=0);
  while(flock(db->jnl_fd,LOCK_EX)!=0);
  int key_len=strlen(key);
  int val_len=strlen(value);
  int offset=DATA_OFFSET;//offset只能通过访问每一个rec_msg直到最后一个获得
  for(int i=0;;i++)
  {
    lseek(db->data_fd,REC_MSG(i),SEEK_SET);
    char buf[LOG_SIZE+1];
    read(db->data_fd,buf,LOG_SIZE);
    log_t* temp=(log_t *)buf;
    if(temp->status!=USED) break;//已经访问完成所有的rec_msg
    offset=temp->offset+temp->klen+temp->vlen;
  }

  lseek(db->data_fd,offset,SEEK_SET);
  write(db->data_fd,key,key_len);
  write(db->data_fd,value,val_len);
  may_crash();
  fsync(db->data_fd);
  

  char kstr[5],vstr[5],offstr[5];
  Int2Str(kstr,key_len);
  Int2Str(vstr,val_len);
  Int2Str(offstr,offset);
  printf("key_len=%d,val_len=%d,offset=%x\n",key_len,val_len,offset);
  char validch[1]={(char)USED};
  char endch[1]={(char)ENDCHAR};
  
  char writebuf[LOG_SIZE+1];
  writebuf[0]=USED;
  for(int i=0;i<4;i++)
  {
    writebuf[i+1]=kstr[i];
    writebuf[i+5]=vstr[i];
    writebuf[i+9]=offstr[i];
  }
  assert(offset>=DATA_OFFSET);
  for(int i=0;;i++)
  {
    lseek(db->jnl_fd,LOG_MSG(i),SEEK_SET);
    char buf[LOG_SIZE+1];
    int ret=read(db->jnl_fd,buf,LOG_SIZE);
    log_t *temp=(log_t*)buf;
    if(temp->status!=USED) break;
    if(ret==0) break;//说明读到末尾了,也可以退出
  }

  write(db->jnl_fd,writebuf,LOG_SIZE-1);
  may_crash();
  write(db->jnl_fd,endch,1);
  may_crash();
  fsync(db->jnl_fd);
  //这里是在数据库文件里写,类似文件系统信息,但是和上面写的内容一致
  for(int i=0;;i++)
  {
    lseek(db->data_fd,REC_MSG(i),SEEK_SET);
    if(REC_MSG(i)>DATA_OFFSET)
    {
      printf("No more space for recording");
      assert(0);
    }
    char buf[2];
    read(db->data_fd,buf,1);
    if(buf[0]!=USED) break;//这里好像不存在上面那个问题
  }

  write(db->data_fd,writebuf,LOG_SIZE);
  may_crash();
  fsync(db->data_fd);
  
  flock(db->data_fd,LOCK_UN);
  flock(db->jnl_fd,LOCK_UN);
  return -1;
}

char *kvdb_get(struct kvdb *db, const char *key) {
  while(flock(db->data_fd,LOCK_EX)!=0);//get只依据Data中的REC区进行检索
  while(flock(db->jnl_fd,LOCK_EX)!=0);
  char buf[LOG_SIZE+1];
  for(int i=0;;i++)
  {
    lseek(db->data_fd,REC_MSG(i),SEEK_SET);
    read(db->data_fd,buf,LOG_SIZE);
    log_t* msg=(log_t*)buf;
    int klen=msg->vlen;
    int vlen=msg->klen;
    int offset=msg->offset;
    char *k=(char *)malloc(klen+1);
    char *v=(char *)malloc(vlen+1);
    lseek(db->data_fd,offset,SEEK_SET);
    read(db->data_fd,k,klen);
    if(strcmp(k,key)==0)
    {
      lseek(db->data_fd,offset+klen,SEEK_SET);
      read(db->data_fd,v,vlen);
      return v;
    }
    free(k);
    free(v);
  }
  
  flock(db->data_fd,LOCK_UN);
  flock(db->jnl_fd,LOCK_UN);
  return NULL;
}
