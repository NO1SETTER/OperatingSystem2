#include<mkfs.h>
#define PROC_ROOT 0
#define PROC_MEMINFO 1
#define PROC_CPUINFO 2
#define PROC_DIR 3
#define PROC_NAME 4
#define DATA_SIZE 32

int PROC_ROOT_ID;
struct data_block//设定
{
char ptr[DATA_SIZE];
int size;
struct data_block* prev;
struct data_block* next;
};

struct proc_inode//proc_inode通过inode继承一些文件属性
{
char path_name[64];
int proc_type;
int size;
struct data_block* data;
int valid;
};

struct proc_inode* proc_table[10000]; 

int nr_proc=0;
int alloc_proc_inode()//要完成proc_inode的分配和空间的申请,但是不用管data_block的分配
{
  int ret=-1;
  for(int i=0;i<nr_proc;i++)
  {
    if(proc_table[i]==NULL||(proc_table[i]&&!proc_table[i]->valid))
    {ret=i;break;}
  }
  if(ret==-1)
  {
    while(proc_table[nr_proc]&&proc_table[nr_proc]->valid)
    nr_proc=nr_proc+1;
  }
  ret=nr_proc;
  if(!proc_table[ret]) proc_table[ret]=(struct proc_inode*)kalloc_safe(sz(proc_inode));
  proc_table[ret]->valid=1;
  return ret;
}

int get_proc_type(char* pathname)
{
int len=strlen(pathname);
int pos=-1;
for(int i=len-1;i>=0;i--)
{
    if(pathname[i]=='/')
    {pos=i;
    break;}
}
assert(pos!=-1);
if(strcmp(pathname+pos,"proc")==0) return PROC_ROOT;
if(strcmp(pathname+pos,"meminfo")==0) return PROC_MEMINFO;
if(strcmp(pathname+pos,"cpuinfo")==0) return PROC_CPUINFO;
if(strcmp(pathname+pos,"name")==0) return PROC_NAME;
return PROC_NAME;
}

int proc_write_data(int node,void* buf,int count)//只支持在末尾写
{
    int write_bytes=0;
    if(proc_table[node]->data==NULL)
    {
        proc_table[node]->data=(struct data_block*)kalloc_safe(sz(data_block));
        strncpy(proc_table[node]->data->ptr,buf,min(count,DATA_SIZE));
        proc_table[node]->data->size=min(count,DATA_SIZE);
        write_bytes=min(count,DATA_SIZE);
    }
    struct data_block* data=proc_table[node]->data;
    while(data->next!=NULL)
    data=data->next;
    while(write_bytes<count)
    {
        struct data_block* nblock=(struct data_block*)kalloc_safe(sz(data_block));
        data->next=nblock;
        nblock->prev=data;
        strncpy(nblock->ptr,buf+write_bytes,min(count-write_bytes,DATA_SIZE));
        nblock->size=min(count-write_bytes,DATA_SIZE);
        write_bytes=write_bytes+min(count-write_bytes,DATA_SIZE);
    }
    return 0;
}
//standard realizations
  int procfs_write(int fd, void *buf, int count)
  {
    return -1;
  }

  int procfs_read(int fd, void *buf, int count)
  {
      int node=ref_table[fd].id;
      if(!proc_table[node]->valid)
      {return -1;}
      int read_bytes=0;
      struct data_block* data=proc_table[node]->data;
      while(read_bytes<count&&data)
      {
          strncpy(buf+read_bytes,data->ptr,min(count-read_bytes,data->size));
          read_bytes=read_bytes+min(count-read_bytes,data->size);
      }
      if(read_bytes<count) return -1;
      return 0;
  }

  int procfs_close(int fd)//只是使该文件描述符无效，不直接使得inode无效
  {
      ref_table[fd].valid=0;
      return 0;
  }

  int procfs_open(const char *pathname, int flags)
  {
    if(!(flags&O_RDONLY))
    { printf("Files in procfs are read-only\n");
      return -1;}
    
    int proc_id=-1;
    for(int i=0;i<nr_proc;i++)
    {
        if(proc_table[i]->valid&&strcmp(proc_table[i]->path_name,pathname)==0)
        { proc_id=i;break;
        }
    }
    if(proc_id==-1) return -1;
    int fd=alloc_fd();
    ref_table[fd].fd=fd;
    ref_table[fd].flags=flags;
    ref_table[fd].id=proc_id;
    ref_table[fd].thread_id=_cpu();
    ref_table[fd].valid=1;

    return fd;
  }

  int procfs_fstat(int fd,struct ufs_stat* buf)
  {
    int node=ref_table[fd].id;
    if(!proc_table[fd]->valid)
    {return -1;}

    buf->id=node;
    buf->size=proc_table[node]->size;
    switch(proc_table[node]->proc_type)
    {
        case PROC_ROOT:case PROC_DIR:buf->type=T_DIR; break;
        case PROC_MEMINFO:case PROC_CPUINFO:case PROC_NAME:
        buf->type=T_FILE;buf->size=0;break;
        default:break;
    }
    return 0;
  }

  int procfs_lseek(int fd, int offset, int whence)
  {return -1;
  }

  int procfs_link(const char* oldpath,const char* newpath)
  {return -1;
  }

  int procfs_unlink(const char* pathname)
  {return -1;
  }
  int procfs_mkdir(const char *pathname)
  {return -1;
  }

extern int ufs_mkdir(const char* pathname);
  filesystem_t* procfs_init()
  {
    procfs=(filesystem_t*)kalloc_safe(sz(filesystem));
    strcpy(procfs->name,"procfs");
    procfs->ops->write=procfs_write;
    procfs->ops->read=procfs_read;
    procfs->ops->close=procfs_close;
    procfs->ops->open=procfs_open;
    procfs->ops->lseek=procfs_lseek;
    procfs->ops->link=procfs_link;
    procfs->ops->unlink=procfs_unlink;
    procfs->ops->fstat=procfs_fstat;
      printf("6%p %p\n",(void*)ufs_mkdir,(void*)ufs->ops->mkdir);
    procfs->ops->mkdir=procfs_mkdir;
      printf("5%p %p\n",(void*)ufs_mkdir,(void*)ufs->ops->mkdir);
    return procfs;
  }
  //extra

int procfs_create(int pid,char* name)//线程创建时调用
  {
    char path[64];
    sprintf(path,"/proc/%d",pid);
    int proc_id1=alloc_proc_inode();
    strcpy(proc_table[proc_id1]->path_name,path);
    proc_table[proc_id1]->proc_type=PROC_DIR;
    proc_table[proc_id1]->size=0;
    proc_table[proc_id1]->valid=1;
    nr_proc=max(nr_proc,proc_id1);

    sprintf(path,"/proc/%d/name",pid);
    int proc_id2=alloc_proc_inode();
    strcpy(proc_table[proc_id2]->path_name,path);
    proc_table[proc_id2]->proc_type=PROC_NAME;
    proc_table[proc_id2]->size=0;
    proc_table[proc_id2]->valid=1;
    nr_proc=max(nr_proc,proc_id2);

    struct ufs_dirent* drt1=(struct ufs_dirent*)kalloc_safe(sz(ufs_dirent));
    drt1->inode=proc_id1;
    sprintf(drt1->name,"%d",pid);
    struct ufs_dirent* drt2=(struct ufs_dirent*)kalloc_safe(sz(ufs_dirent));
    drt2->inode=proc_id2;
    strcpy(drt2->name,"name");
    proc_write_data(PROC_ROOT_ID,drt1,sz(ufs_dirent));
    proc_write_data(proc_id1,drt2,sz(ufs_dirent));
    proc_write_data(proc_id2,name,strlen(name));
    return 0;
  }


int procfs_teardown(int pid)//线程结束时调用
  {
    char path1[64];
    strncpy(path1,"/proc/%d",pid);
    int proc_id1=-1;
    for(int i=0;i<nr_proc;i++)
    {
      if(strcmp(path1,proc_table[i]->path_name)==0)
      {  proc_id1=i;break;
      }
    }
    if(proc_id1==-1) return -1;
    struct data_block* ptr1=proc_table[proc_id1]->data;
    struct data_block* prev1=ptr1;
    while(ptr1)
    {
      ptr1=ptr1->next;
      kfree_safe(prev1);
      prev1=ptr1;
    }
    free(&proc_table[proc_id1]);
    proc_table[proc_id1]=NULL;

    char path2[64];
    strncpy(path2,"/proc/%d/name",pid);
    int proc_id2=-1;
    for(int i=0;i<nr_proc;i++)
    {
      if(strcmp(path2,proc_table[i]->path_name)==0)
      {  proc_id2=i;break;
      }
    }
    if(proc_id2==-1) return -1;
    struct data_block* ptr2=proc_table[proc_id2]->data;
    struct data_block* prev2=ptr2;
    while(ptr2)
    {
      ptr2=ptr2->next;
      kfree_safe(prev2);
      prev2=ptr2;
    }
    free(&proc_table[proc_id2]);
    proc_table[proc_id2]=NULL;

    struct data_block* root_ptr=proc_table[PROC_ROOT_ID]->data;
    while(1)
    {
      if(root_ptr==NULL)
      {
        printf("error in /proc");
        return -1;
      }
      struct ufs_dirent* drt=(struct ufs_dirent*)root_ptr->ptr;
      if(drt->inode==proc_id1)
      {
        root_ptr->prev->next=root_ptr->next;
        root_ptr->next->prev=root_ptr->prev;
        break;
      } 
      root_ptr=root_ptr->next;     
    }
    return 0;
  }