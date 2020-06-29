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
#define LOG_OFFSET 64
#define LOG_SIZE 16
#define LOG_MSG(i) LOG_OFFSET+LOG_SIZE*i
#define REC_MSG(i) LOG_SIZE*i
//log文件开始留64byte做控制信息
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
  int p=rand()%10;
  if(p==3)//有10%的可能crash
  {
    printf("Crashed\n");
    exit(0);
  }
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

struct loghead//64byte
{
  uint32_t nr_log;//已用log区域总条数
  uint8_t pad[60];
}__attribute__((packed));
typedef struct loghead loghead_t;

void recover(struct kvdb* db)
{
  while(flock(db->data_fd,LOCK_EX)!=0);
  while(flock(db->jnl_fd,LOCK_EX)!=0);
  
  char buffer[65];
  lseek(db->jnl_fd,0,SEEK_SET);
  read(db->jnl_fd,buffer,64);
  loghead_t* temp=(loghead_t*)buffer;

  int nr_log=temp->nr_log;
  for(int i=0;i<nr_log;i++)
  {
    lseek(db->jnl_fd,LOG_MSG(i),SEEK_SET);
    char buf1[LOG_SIZE+1];
    read(db->jnl_fd,buf1,LOG_SIZE);
    log_t* lmsg=(log_t*)buf1;
    if(lmsg->status!=USED) continue;//已经恢复过了,不用管
    if(lmsg->end!=ENDCHAR) continue;//没有endchar,不能确认信息是否完整,也不用管
    
    for(int j=0;;j++)
    {
      lseek(db->data_fd,REC_MSG(j),SEEK_SET);
      char buf2[LOG_SIZE+1];
      read(db->data_fd,buf2,LOG_SIZE);
      log_t* rmsg=(log_t*)buf2;
      if(lmsg->offset==rmsg->offset) break;//说明已经成功写入,不用管
      if(rmsg->status!=USED)//找到一个空槽,可以写入
      {
        lseek(db->data_fd,REC_MSG(j),SEEK_SET);
        write(db->data_fd,buf1,LOG_SIZE);
        may_crash();
        fsync(db->data_fd);

        lmsg->status=FREE;
        lseek(db->jnl_fd,LOG_MSG(i),SEEK_SET);
        write(db->jnl_fd,buf1,LOG_SIZE);
        may_crash();
        fsync(db->jnl_fd);
      }
    }
  }

  flock(db->data_fd,LOCK_UN);
  flock(db->jnl_fd,LOCK_UN);
}

struct kvdb *kvdb_open(const char *filename) 
{//把log和数据库分开存放
  srand((int)time(0));
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
  //printf("Data_fd=%d jnl_fd=%d\n",fd1,fd2);
  kvdb_t* ptr=(kvdb_t *)malloc(sizeof(kvdb_t));
  ptr->data_fd=fd1;
  ptr->jnl_fd=fd2;

  char init[64];
  memset(init,0,sizeof(init));
  lseek(ptr->jnl_fd,0,SEEK_SET);
  write(ptr->jnl_fd,init,LOG_OFFSET);//开始64字节初始化为0
  recover(ptr);
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

/*写入顺序
key-val数据 -->  (log数+1) --> log信息 -->  log endchar --> 文件系统信息
*/
int kvdb_put(struct kvdb *db, const char *key, const char *value) {
  while(flock(db->data_fd,LOCK_EX)!=0);
  while(flock(db->jnl_fd,LOCK_EX)!=0);
  int key_len=strlen(key);
  int val_len=strlen(value);
  int offset=DATA_OFFSET;//offset只能通过访问每一个rec_msg直到最后一个获得
  //写数据到db文件中
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
  
  //写log到log文件中
  char kstr[5],vstr[5],offstr[5];
  Int2Str(kstr,key_len);
  Int2Str(vstr,val_len);
  Int2Str(offstr,offset);
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
  int expand=0;
  for(int i=0;;i++)
  {
    int rdoffset=lseek(db->jnl_fd,LOG_MSG(i),SEEK_SET);
    char buf[LOG_SIZE+1];
    int ret=read(db->jnl_fd,buf,LOG_SIZE);
    log_t *temp=(log_t*)buf;
    
    int fsize=lseek(db->jnl_fd,0,SEEK_END);
    //printf("offset=%d fsize=%d\n",rdoffset,fsize);
    if(rdoffset==fsize)//读到末尾,扩张大小
    { 
      expand=1;
      lseek(db->jnl_fd,LOG_MSG(i),SEEK_SET);
      break;}//说明读到末尾了,也可以退出
    if(temp->status!=USED) 
    { lseek(db->jnl_fd,LOG_MSG(i),SEEK_SET);
      break;}
  }
  
  if(expand)
  {
    //printf("Expand\n");
    lseek(db->jnl_fd,0,SEEK_SET);
    char head[65];
    read(db->jnl_fd,head,64);
    loghead_t * lgh=(loghead_t *)head;
    lgh->nr_log=lgh->nr_log+1;
    lseek(db->jnl_fd,0,SEEK_SET);
    write(db->jnl_fd,head,64);
    may_crash();
    fsync(db->jnl_fd);//把记录的log数加1
  }

  write(db->jnl_fd,writebuf,LOG_SIZE-1);
  may_crash();
  fsync(db->jnl_fd);

  write(db->jnl_fd,endch,1);
  may_crash();
  fsync(db->jnl_fd);
      
  //写文件系统信息到db文件中
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
    if(buf[0]!=USED) 
    { lseek(db->data_fd,REC_MSG(i),SEEK_SET);
      break;}//这里好像不存在上面那个问题
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
    if(msg->status!=USED) break;
    int klen=msg->klen;
    int vlen=msg->vlen;
    int offset=msg->offset;
    //printf("klen=%d vlen=%d offset=%d\n",klen,vlen,offset);
    char *k=(char *)malloc(klen+1);
    char *v=(char *)malloc(vlen+1);
    lseek(db->data_fd,offset,SEEK_SET);
    read(db->data_fd,k,klen);
    k[klen]='\0';
    if(strcmp(k,key)==0)
    {
      lseek(db->data_fd,offset+klen,SEEK_SET);
      read(db->data_fd,v,vlen);
      flock(db->data_fd,LOCK_UN);
      flock(db->jnl_fd,LOCK_UN);
      return v;
    }
    free(k);
    free(v);
  }
  
  flock(db->data_fd,LOCK_UN);
  flock(db->jnl_fd,LOCK_UN);
  return NULL;
}
