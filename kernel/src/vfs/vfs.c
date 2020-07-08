#include<common.h>
#include<devices.h>
#include<user.h>
#include<klib.h>
//extensions
#define nr_mnt 100
typedef struct mounttable
{
char path[100];
filesystem_t *fs;
int valid;
}mount_table[100];

#define nr_file 300
typedef struct filetable
{
  inode_t* node;
  int linkid;
  int valid;
}file_table[1000];//记录打开的文件
/*计划用树的形式规划file_table来支持link和unlink操作
对于没被链接确实存在的文件,linkid=-1,否则指向它所被链接的文件,
*/


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
  }
  return NULL;
}


char* get_linkname(char *path)//得到path link到文件的name
{
int pos=-1;
for(int i=0;i<nr_file;i++)
{
  if(strcmp(path,file_table[i].path)==0)
  {pos=i;break;
  }
}
if(pos==-1)
{
  printf("Not in any file system!\n");
  return path;
}
else
{
    while(file_table[pos].linkid!=-1)
    {
      pos=file_table[pos].linkid;
      break;
    }
    return file_table[pos].node->path;
}

return NULL;
}

//standard realizations
  void vfs_init()
  {

    vfs_mount("/",ufs);
  }
   
  //read和write的前提都是在cur中open过了,那么需要到cur中去找fd
  int vfs_write(int fd, void *buf, int count)
  {
    for(int i=0;i<nr_file;i++)
    {
      if(cur->files[i].valid&&cur->files[i].fd==fd)
      {
        filesystem_t* fs=cur->files[i].node->fs;
        int ret=fs->ops->write(fd,buf,count);
        return ret;
      } 
    }
    return -1;
  }

  int vfs_read(int fd, void *buf, int count)
  {
    for(int i=0;i<nr_file;i++)
    {
      if(cur->files[i].valid&&cur->files[i].fd==fd)
      {
        filesystem_t* fs=cur->files[i].node->fs;
        int ret=fs->ops->read(fd,buf,count);
        return ret;
      } 
    }
    return -1;
  }

  int vfs_close(int fd)
  {    
    for(int i=0;i<nr_file;i++)
    {
      if(cur->files[i].valid&&cur->files[i].fd==fd)
      {
        filesystem_t* fs=cur->files[i].node->fs;
        int ret=fs->ops->close(fd);
        return ret;
      } 
    }
    return -1;
  }

  //设定是根据pathname直接可以确定它属于哪个文件系统?
  int vfs_open(const char *pathname, int flags)
  {
    filesystem_t* fs=find_fs(pathname);
    assert(fs);

    int fileid=-1;
    for(int i=0;i<nr_file;i++)
    {
      if(file_table[i].valid)
      {
        if(strcmp(pathname,filetable[i].file->path)==0)
        {
          fileid=i;
          while(file_table[fileid].linkid!=-1)
          {
            fileid=file_table[fileid].linkid;
            break;
          }
        }
      }
    }

    if(fileid!=-1)//已经加入过文件系统
    {
    sem_wait(&file_table[i].node->sem);
      int fd=fs->ops->open(pathname,flags);
      file_table[i].node->fd=fd;
      file_table[i].node->taskid=cur->id;
      for(int i=0;i<nr_file;i++)
      {
        if(valid)
          
      }
    sem_signal(&file_table[i].node->sem);
    return fd;
    }
    else //创建新的inode
    {
      inode_t* newnode=(inode_t*)kalloc_safe(sizeof(inode_t));
      int fd=fs->ops->open(pathname,flags);
      newnode->fd=fd;
      newnode->fs=fs;
      newnode->offset=0;
      newnode->taskid=cur->id;
      sem_init(newnode->sem,1);
      for(int i=0;i<nr_file;i++)
      {
        if(!file_table[i].valid)
        {
          file_table[i].valid=1;
          file_table[i].node=newnode;
          file_table[i].linkid=-1;
          break;
        }
      }
      for(int i=0;i<nr_file;i++)
      {
        if(!cur->valid[i])
        {
          cur->id[i]=1;
          cur->files[i]=newnode;
          break;
        }
      }
    return fd;
    }
    return -1;
  }

  int vfs_lseek(int fd, int offset, int whence)
  {
    for(int i=0;i<nr_file;i++)
    {
      if(cur->files[i].valid&&cur->files[i].fd==fd)
      {
        filesystem_t* fs=cur->files[i].node->fs;
        int ret=fs->ops->lseek(fd);
        return ret;
      } 
    }
    return -1;
  } 
  
  int vfs_link(const char *oldpath, const char *newpath)
  {
    filesystem_t* fs = find_fs(oldpath);
    assert(fs==ufs);
    
    int old_id=-1;
    for(int i=0;i<nr_file;i++)
    {
      if(file_table[i].valid&&strcmp(oldpath,file_table[i]->node.path)==0)
      {old_id=i;break;
      }
    }

    if(old_id==-1)
    {
      printf("THe file:%s linked to doesn't exist\n",oldpath);
      return -1;
    }
    
    int new_id=-1;
    for(int i=0;i<nr_file;i++)
    {
      if(!file_table[i].valid)
      { new_id=i;break;
      }
    }

    inode_t* newnode=(inode_t*)=kalloc_safe(sizeof(inode_t));
    strcpy(newnode->path,newpath);
    file_table[new_id].node=newnode;
    file_table[new_id].linkid=old_id;
    file_table[new_id].valid=1;
    return 0;
  }

  int vfs_unlink(const char *pathname)
  {
    int id=-1;
    for(int i=0;i<nr_file;i++)
    {
      if(strcmp(pathname,file_table[i].inode->path)==0)
      { id=i;break;
      }
    }
    if(id==-1) return -1;
    assert(file_table[i].node->fs==ufs);
    file_table[i].valid=0;
    file_table[i].linkid=-1;
    kfree_safe(file_table[i].node);
    return 0;
  }
  
  int vfs_fstat(int fd, struct ufs_stat *buf)
  {

  }

  int vfs_mkdir(const char *pathname)
  {

  }

  int vfs_chdir(const char *path)
  {
    cur->cur_path=path;
    return 0;
  }

  int vfs_dup(int fd)
  {

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