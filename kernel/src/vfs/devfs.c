#include<mkfs.h>
//extensions
char device_name[8][32]={"zero","null","random","input","fb","tty1","tty2","sda"};
#define EOF -1

struct dev_inode
{
    int fd;
    int valid;
    char name[32];
};

struct dev_inode dev_table[8];//直接分配好

//standard realizations
  int devfs_write(int fd,void* buf, int count)
  {
    int dev_id=-1;
    for(int i=0;i<8;i++)
    {
      if(dev_table[i].valid&&dev_table[i].fd==fd)
      { dev_id=i;break;
      }
    }
    if(dev_id==-1)
    {
      #ifdef VFS_DEBUG
      printf("device not supported\n");
      #endif
      return -1;
    }
    
    device_t* devv=dev->lookup(dev_table[dev_id].name);
    return devv->ops->write(devv,0,buf,count);
  }

  int devfs_read(int fd,void *buf, int count)
  {
    int dev_id=-1;
    for(int i=0;i<8;i++)
    {
      if(dev_table[i].valid&&dev_table[i].fd==fd)
      { dev_id=i;break;
      }
    }
    if(dev_id==-1)
    {
      #ifdef VFS_DEBUG
      printf("device not supported\n");
      #endif
      return -1;
    }
    
    device_t* devv=dev->lookup(dev_table[dev_id].name);
    return devv->ops->read(devv,0,buf,count);
  }

  int devfs_close(int fd)
  {
    int dev_id=-1;
    for(int i=0;i<8;i++)
    {
      if(dev_table[i].valid&&dev_table[i].fd==fd)
      {  dev_id=i;break;
      }
    }
    if(dev_id==-1)
    {
      #ifdef VFS_DEBUG
      printf("device not opened\n");
      #endif
      return -1;
    }
    ref_table[fd].valid=0;
    dev_table[dev_id].valid=0;
    return 0;
  }

  int devfs_open(const char *pathname, int flags)
  {
    char name[32];
    get_name(pathname,name);
    int dev_id=-1;  
    for(int i=0;i<8;i++)
    {
        if(strcmp(name,device_name[i])==0)
        {  dev_id=i;break;
        }
    }
    if(dev_id==-1)
    {
      #ifdef VFS_DEBUG
        printf("device not supported\n");
      #endif
      return -1;}

    int fd = alloc_fd();
    dev_table[dev_id].fd=fd;
    dev_table[dev_id].valid=1;
    strcpy(dev_table[dev_id].name,name);

    ref_table[fd].fd=fd;
    ref_table[fd].flags=flags;
    ref_table[fd].id=dev_id;
    ref_table[fd].thread_id=cur->id;
    ref_table[fd].fs=devfs;
    ref_table[fd].valid=1;
    return fd;
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
    devfs->ops=(fsops_t*)kalloc_safe(sz(fsops));
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