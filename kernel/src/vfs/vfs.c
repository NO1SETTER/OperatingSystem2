#include<common.h>
#include<vfs.h>
#include<mkfs.h>

sem_t inode_lock;
sem_t fd_lock;

//extensions
int min(int a,int b){return a<b?a:b;}
int max(int a,int b){return a>b?a:b;}

int error_dfs(int x)
{return error_dfs(x+1);}

void vfs_mount(const char* path,filesystem_t* fs)//把fs挂载在dir下,dir是一个可以直接访问的目录
{
  int next=-1;
  for(int i=0;i<nr_mnt;i++)
  { if(!mount_table[i].valid)
    {next=i;break;
    } 
  }
  //assert(next!=-1);
  strcpy(mount_table[next].path,path);
  mount_table[next].fs=fs;
  mount_table[next].valid=1;
}

filesystem_t* find_fs(const char* path)//找到某一个文件所在的文件系统
{
  char abs_path[256];
  get_abs_path(path,abs_path);
  for(int i=0;i<nr_mnt;i++)//和每一个文件系统作比较
  {
    if(!mount_table[i].valid) continue;
    assert(mount_table[i].path[0]=='/');
    int len1=strlen(abs_path),len2=strlen(mount_table[i].path);
    for(int j=1;j<len1;j++)
    {
      if(abs_path[j]=='/') 
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
    if(strncmp(abs_path,mount_table[i].path,len1)!=0) continue;
    return mount_table[i].fs;
  }//循环内没成功默认返回ufs
  return ufs;
}

void xxd(const char *str,int n)
{
  printf("xxd:");
  for(int i=0;i<n;i++)
  {
    if(i%16==0) printf("\n");
    printf("%02x",str[i]);
    if(i%2) printf(" ");
  }
  printf("\n");
}

int alloc_fd()//所有fd从1开始
{
  sem_wait(&fd_lock);
  int ret=-1;
  for(int i=1;i<nr_ref;i++)
  { if(!ref_table[i].valid)
    {
      ref_table[i].fd=i;
      ref_table[i].valid=1;
      ret=i;
      break;
    }
  }
  sem_signal(&fd_lock);
  return ret;
}

//希望做的是通过管理inode直接管理文件号的分配，并且同步到磁盘中的文件系统中去
int inode_set[10000];//记录小于max_inode可用的inode
int max_inode=-1;
int nr_inode=0;
void push_inode(int x){inode_set[nr_inode++]=x;}
int pop_inode(){return inode_set[--nr_inode];}
int alloc_inode()
{
  sem_wait(&inode_lock);
  if(max_inode==-1) ufs->dev->ops->read(ufs->dev,FS_START+47,(void*)&max_inode,4);
  //assert(nr_inode>=0);
  int ret=-1;
  if(nr_inode) ret=pop_inode();
  else ret=max_inode;
  while(file_table[ret].valid)
  {ret=ret+1;}
  max_inode=ret+1;
  sem_signal(&inode_lock);
  return ret;
}

#include "traverse.inc"
void vfs_test()
{
  #include "workload.inc"
  printf("traverse starts\n");
  traverse("");
  #include "proctest.inc"
  #include "devtest.inc"
  #include "linktest.inc"
}

//standard realizations
extern filesystem_t* ufs_init();
extern filesystem_t* procfs_init();
extern filesystem_t* devfs_init();
extern int ufs_mkdir(const char *pathname);
  void vfs_init()
  {
    assert(0);
    for(int i=0;i<nr_mnt;i++) 
      mount_table[i].valid = 0;
    vfs_mount("/",ufs_init());
    vfs_mount("/proc",procfs_init());
    vfs_mount("/dev",devfs_init());
  }
  
  //read和write的前提都是在cur中open过了,那么需要到cur中去找fd
  int vfs_write(int fd, void *buf, int count)
  {
    #ifdef VFS_DEBUG
      printf("\nwrite to fd:%d\n",fd);
    #endif
    int ret=-1;
    if(ref_table[fd].valid)
    {
      if(ref_table[fd].thread_id==cur->id)
      { 
      filesystem_t* fs=ref_table[fd].fs;
      #ifdef VFS_DEBUG
        printf("fs:%s\n",fs->name);
      #endif
      ret=fs->ops->write(fd,buf,count);;
      }
    }
    return ret;
  }

  int vfs_read(int fd, void *buf, int count)
  {
    #ifdef VFS_DEBUG
      printf("\nread from fd:%d\n",fd);
    #endif
    int ret=-1;
    if(ref_table[fd].valid)
    {
      if(ref_table[fd].thread_id==cur->id)
      { 
      filesystem_t* fs=ref_table[fd].fs;
      #ifdef VFS_DEBUG
        printf("fs:%s\n",fs->name);
      #endif
      ret=fs->ops->read(fd,buf,count);;
      }
    }
    return ret;
  }

  int vfs_close(int fd)
  { 
    #ifdef VFS_DEBUG
      printf("\nclose fd:%d\n",fd);
    #endif
    int ret=-1;
    if(ref_table[fd].valid)
    {
      if(ref_table[fd].thread_id==cur->id)
      { 
      filesystem_t* fs=ref_table[fd].fs;
      ret=fs->ops->close(fd);
      }
    }
    return ret;
  }

  //设定是根据pathname直接可以确定它属于哪个文件系统?
  int vfs_open(const char *pathname, int flags)
  {
    #ifdef VFS_DEBUG
      printf("\nopen: %s\n",pathname);
    #endif
    filesystem_t* fs=find_fs(pathname);
    #ifdef VFS_DEBUG
      printf("fs:%s\n",fs->name);
    #endif
    int fd = fs->ops->open(pathname,flags);
    #ifdef VFS_DEBUG
      printf("%s is allocated fd:%d\n",pathname,fd);
    #endif
    return fd;
  }

  int vfs_lseek(int fd, int offset, int whence)
  {
    #ifdef VFS_DEBUG
      printf("\nlseek fd:%d\n",fd);
    #endif
    int ret=-1;
    if(ref_table[fd].valid)
    {
      if(ref_table[fd].thread_id==cur->id)
      { 
      filesystem_t* fs=ref_table[fd].fs;
      ret=fs->ops->lseek(fd,offset,whence);
      }
    }
    return ret;
  } 
  
  //link不需要打开..且link的结果是持久存在的
  //link_table每个项存储一个pathname和指向的id
  int vfs_link(const char *oldpath, const char *newpath)//创建ref
  {
    #ifdef VFS_DEBUG
      printf("\nlink %s to %s\n",newpath,oldpath);
    #endif
    filesystem_t* fs = find_fs(oldpath);
    //assert(fs==ufs);
    return fs->ops->link(oldpath,newpath);
  }


  int vfs_unlink(const char *pathname)
  {
    #ifdef VFS_DEBUG
      printf("\nunlink %s\n",pathname);
    #endif
    filesystem_t* fs=find_fs(pathname);
    //assert(fs==ufs);
    return fs->ops->unlink(pathname);
  }

  
  int vfs_fstat(int fd, struct ufs_stat *buf)
  {
    #ifdef VFS_DEBUG
      printf("\nfstat fd:%d\n",fd);
    #endif
    if(ref_table[fd].valid)
    {
      filesystem_t* fs=ref_table[fd].fs;
      printf("fs:%s\n",fs->name);
      if(ref_table[fd].thread_id==cur->id)
        return fs->ops->fstat(fd,buf);
    }
    return -1;
  }


  int vfs_mkdir(const char *pathname)
  {
    #ifdef VFS_DEBUG
      printf("\nmkdir:%s\n",pathname);
    #endif
      filesystem_t* fs=find_fs(pathname);
      //assert(fs==ufs);
      return fs->ops->mkdir(pathname);
  }

  int vfs_chdir(const char *path)
  {
    char abs_path[256];
    get_abs_path(path,abs_path);
    #ifdef VFS_DEBUG
    printf("\nchdir thread:%d from %s to %s\n",cur->id,cur->cur_path,abs_path);
    #endif
    strcpy(cur->cur_path,abs_path);
    return 0;
  }

  int vfs_dup(int fd)
  {
    #ifdef VFS_DEBUG
      printf("\n dup:%d\n",fd);
    #endif
    int new_id=alloc_fd();
    ref_table[new_id].fd=new_id;
    ref_table[new_id].flags=ref_table[fd].flags;
    ref_table[new_id].id=ref_table[fd].id;
    ref_table[new_id].thread_id=ref_table[fd].thread_id;
    ref_table[new_id].valid=1;
    return new_id;
  }


MODULE_DEF(vfs) = {
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