#include<vfs.h>
#include<klib.h>
/*
path like this/is/a/very/long/path
*/

//extensions
void vfs_mount(const char* path,filesystem_t* fs)//把fs挂载在dir下,dir是一个可以直接访问的目录
{
  for(int i=0;i<nr_mnt;i++)
  {
    if(mount_table[i].valid&&strcmp(path,mount_table[i].mount_table)==0) 
      assert(0);
  }
  int next=-1;
  for(int i=0;i<nr_mnt;i++)
  { if(!mount_table[i].valid)
    {next=i;break;
    } 
  }
  assert(next!=-1);
  strcpy(mount_table[next].path,path);
  mount_table[next].fs=fs;
  mount_table[next].valid=1;
}

filesystem_t* find_fs(const char* path)//找到某一个文件所在的文件系统
{
  for(int i=0;i<nr_mnt;i++)//和每一个文件系统作比较
  {
    if(!mount_table[i].valid) continue;
    assert(path[0]=='/'&&mount_table[i].path[0]=='/');
    
    int len1=strlen(path),len2=strlen(mount_table[i].path);
    for(int j=1;j<len1;j++)
    {
      if(path[j]=='/') 
      { len1=j;break;
      }
    }
    for(int j=1;j<len2;j++)
    {
      if(mount_table[i].path[j]=='/')
      { len2=j;break;
      }
    }

    if(len1!=len2) continue;
    if(strncmp(path,mount_table[i].path,len1)!=0) continue;
    return mount_table[i].fs;
  }//循环内没成功默认返回ufs
  return ufs;
}

int alloc_file_id()
{
for(int i=0;i<nr_file;i++)
{ if(!file_table[i].valid)
  {
    file_table[i].valid=1;
    file_table[i].id=i;
    return i;
  }
}
return -1;
}

int alloc_link_id()
{
for(int i=0;i<nr_link;i++)
{ if(!link_table[i].valid)
  {
    link_table[i].valid=1;
    return i;
  }
}
return -1;
}

int alloc_fd()
{
for(int i=0;i<nr_ref;i++)
{ if(!ref_table[i].valid)
  {
    ref_table[fd].fd=i;
    ref_table[i].valid=1;
    return i;
  }
}
return -1;
}

//standard realizations
  void vfs_init()
  {
    ufs=(filesystem_t*)kalloc_safe(sizeof(filesystem_t));
    procfs=(filesystem_t*)kalloc_safe(sizeof(filesystem_t));
    devfs=(filesystem_t*)kalloc_safe(sizeof(filesystem_t));
    ufs->dev=dev_lookup("sda");
    procfs->dev=dev_lookup("sda");
    devfs->dev=dev_lookup("sda");

    vfs_mount("/",ufs);
    vfs_mount("/proc",procfs);
    vfs_mount("/dev",devfs);

    for(int i=0;i<nr_file;i++)
    ref_table[i].valid=link_table[i].valid=file_table[i].valid=0;
  }
   
  //read和write的前提都是在cur中open过了,那么需要到cur中去找fd
  int vfs_write(int fd, void *buf, int count)
  {
    if(ref_table[fd].valid)
    {
      if(ref_table[fd].thread_id=_cpu())
      { 
      int file_id=ref_table[fd].id;
      filesystem_t* fs=file_table[file_id].fs;
      return fs->ops->write(fd,buf,count);
      }
    }
    return -1;
  }

  int vfs_read(int fd, void *buf, int count)
  {
    if(ref_table[fd].valid)
    {
      if(ref_table[fd].thread_id=_cpu())
      { 
      int file_id=ref_table[fd].id;
      filesystem_t* fs=file_table[file_id].fs;
      return fs->ops->read(fd,buf,count);
      }
    }
    return -1;
  }

  int vfs_close(int fd)
  {    
    if(ref_table[fd].valid)
    {
      if(ref_table[fd].thread_id=_cpu())
      { 
      int file_id=ref_table[fd].id;
      filesystem_t* fs=file_table[file_id].fs;
      return fs->ops->close(fd);
      }
    }
    return -1;
  }

  //设定是根据pathname直接可以确定它属于哪个文件系统?
  int vfs_open(const char *pathname, int flags)
  {
      filesystem_t* fs=find_fs(pathname);
      return fs->ops->open(pathname,flags);
  }

  int vfs_lseek(int fd, int offset, int whence)
  {
    if(ref_table[fd].valid)
    {
      if(ref_table[fd].thread_id=_cpu())
      { 
      int file_id=ref_table[fd].id;
      filesystem_t* fs=file_table[file_id].fs;
      return fs->ops->lseek(fd,offset,whence);
      }
    }
    return -1;
  } 
  
  //link不需要打开..且link的结果是持久存在的
  //link_table每个项存储一个pathname和指向的id
  int vfs_link(const char *oldpath, const char *newpath)//创建ref
  {
    filesystem_t* fs = find_fs(oldpath);
    assert(fs==ufs);
    return fs->ops->link(oldpath,newpath);
  }

  int vfs_unlink(const char *pathname)
  {
    filesystem_t* fs=find_fs(pathname);
    assert(fs==ufs);
    return fs->ops->unlink(pathname);
  }
  
  int vfs_fstat(int fd, struct ufs_stat *buf)
  {
    return ufs->ops->fstat(fd,buf);
  }

  int vfs_mkdir(const char *pathname)
  {
      filesystem_t* fs=find_fs(pathname);
      assert(fs==ufs);
      return fs->ops->mkdir(pathname);
  }

  int vfs_chdir(const char *path)
  {
    strcpy(cur->cur_path,path);
    return 0;
  }

  int vfs_dup(int fd)
  {
    int new_id=alloc_fd();
    ref_table[new_id].fd=new_id;
    ref_table[new_id].flags=ref_table[fd].flags;
    ref_table[new_id].id=ref_table[fd].id;
    ref_table[new_id].thread_id=ref_table[fd].thread_id;
    ref_table[new_id].valid=1;
    return new_id;
  }

MODULE_DEF(vfs) {
  .init=vfs_init,
  .write=vfs_write,
  .read=vfs_read,
  .close=vfs_close,
  .open=vfs_open,
  .lseek=vfs_lseek,
  .link=vfs_link,
  .unlink=vfs_unlink,
  .fstat=vfs_fstat,
  .mkdir=vfs_mkdir,
  .chdir=vfs_chdir,
  .dup=vfs_dup,
};