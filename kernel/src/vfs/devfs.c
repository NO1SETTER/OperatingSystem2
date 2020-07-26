#include<mkfs.h>
//extensions
#define DEV_ZERO 0
#define DEV_NULL 1
#define DEV_RANDOM 2

#define EOF -1
struct dev_inode
{
    int fd;
    int valid;
};
struct dev_inode dev_table[3];//直接分配好

//standard realizations
  int devfs_write(int fd, void *buf, int count)
  {
      if(fd!=dev_table[DEV_NULL].fd) return -1;
      if(!dev_table[DEV_NULL].valid) return -1;
      return 0;
  }

  int devfs_read(int fd,  void *buf, int count)
  {
      int dev_id=-1;
      for(int i=0;i<3;i++)
      { if(dev_table[i].fd==fd)
          {  dev_id=i;break;
          }
      }
      if(dev_id==-1)
      {
          printf("device not supported\n");
          return -1;
      }
      switch(dev_id)
      {
          case DEV_ZERO:
            for(int i=0;i<count;i++)  *(char*)(buf+i)=0;
            return 0;
          case DEV_NULL:
            return EOF;
          case DEV_RANDOM:
            for(int i=0;i<count;i++)  *(char*)(buf+i)=rand()%256;
            return 0;
      }
      return -1;
  }

  int devfs_close(int fd)
  {
      for(int i=0;i<3;i++)
      {
          if(dev_table[i].fd==fd)
          {
              dev_table[i].valid = 0;
              return 0;
          }
      }
    return -1;
  }

  int devfs_open(const char *pathname, int flags)
  {
      int fd=alloc_fd();
      int dev_id=-1;
      if(strcmp(pathname,"/dev/zero")==0) dev_id=DEV_ZERO;
      else if(strcmp(pathname,"/dev/null")==0) dev_id=DEV_NULL;
      else if(strcmp(pathname,"/dev/random")==0) dev_id=DEV_RANDOM;
      if(dev_id==-1)
      {
          printf("device not supported\n");
          return -1;
      }
      dev_table[dev_id].fd=fd;
      dev_table[dev_id].valid=1;
      return 0;
  }

  int devfs_lseek(int fd, int offset, int whence)
  {
      return -1;
  }

  int devfs_link(const char* oldpath,const char* newpath)
  {
      return -1;
  }

  int devfs_unlink(const char* pathname)
  {
      return -1;
  }

  int devfs_fstat(int fd,struct ufs_stat* buf)
  {
    return -1;
  }
  
  int devfs_mkdir(const char *pathname)
  {
      return -1;
  }

  filesystem_t* devfs_init()
  {
    devfs=(filesystem_t*)kalloc_safe(sz(filesystem));
    strcpy(devfs->name,"devfs");
    devfs->ops->write=devfs_write;
    devfs->ops->read=devfs_read;
    devfs->ops->close=devfs_close;
    devfs->ops->open=devfs_open;
    devfs->ops->lseek=devfs_lseek;
    devfs->ops->link=devfs_link;
    devfs->ops->unlink=devfs_unlink;
    devfs->ops->fstat=devfs_fstat;
    devfs->ops->mkdir=devfs_mkdir;
    return devfs;
  }