#include<mkfs.h>
//extensions

int locate_file(char* path_name)//默认传进来的都是绝对路径
{//如果找到了则返回inode,没找到返回上一级inode的负值,上一级inode也没找到则不允许创建返回INT_MIN,
  assert(path_name[0]=='/');
  inode_t* now_node=&file_table[1];//根目录的inode
  int next_node=-1;
  int len=strlen(path_name);
  int lid=1,rid=1;
  char cur_name[32];//当前检索到的文件夹
  for(int i=1;i<=len;i++)
  {
    if(path_name[i]!='/'&&i!=len) continue;//i==len时代表访问到末尾,单独访问
    rid=i;
    strncpy(cur_name,path_name+lid,rid-lid);
    struct ufs_dirent* drt=(struct ufs_dirent*)kalloc_safe(sz(ufs_dirent));
    int nr_files=now_node->size/sz(ufs_dirent);
    next_node=-1;
    for(int j=0;j<nr_files;j++)
    {
      read_data(now_node,j*sz(ufs_dirent),(char*)drt,sz(ufs_dirent));
      if(strcmp(drt->name,cur_name)==0)//找到了
      {
        next_node=drt->inode;
        break;
      }
    }
    if(next_node==-1)
    {
      if(i==len) return -now_node->node;
      return INT_MIN;
    }
    now_node=&file_table[next_node];
    lid=i+1;
  }
  return now_node->node;
}

int get_abs_path(const char *path,char* abs_path)
{
  if(path[0]=='/')
    strcpy(abs_path,path);
  else
    sprintf(abs_path,"%s/%s",cur->cur_path,path);
  return 1;
}

int get_name(const char* path,char* name)
{
  int len=strlen(path);
  int pos=0;
  for(int i=len-1;i>=0;i--)
  { if(path[i]=='/') 
    {
      pos=i;break;
    }
  }
  strcpy(name,path+pos);
  return 0;
}
//standard realizations
  int ufs_write(int fd, void *buf, int count)
  { //检查已经在vfs层完成了
    int inode=ref_table[fd].id;
    if(file_table[inode].refct==0)//refct才表明本体状态,valid不表明
    {
      printf("File not opened\n");
      return -1;
    }
    inode_t* node=&file_table[inode];
    return write_data(node,node->offset,(char*)buf,count);
  }

  int ufs_read(int fd, void *buf, int count)
  {
    int inode=ref_table[fd].id;
    if(file_table[inode].refct==0)
    {
      printf("File not opened\n");
      return -1;
    }
    inode_t* node=&file_table[inode];
    return read_data(node,node->offset,(char*)buf,count);
  }

  int ufs_close(int fd)//只是使该文件描述符无效，不直接使得inode无效
  {
    int inode=ref_table[fd].id;
    if(file_table[inode].refct==0)
    {
      printf("File not opened\n");
      return -1;
    }
    struct dir_entry* dir=(struct dir_entry*)kalloc_safe(sz(dir_entry));
    ufs->dev->ops->read(ufs->dev,Entry(inode),dir,sz(dir_entry));
    dir->DIR_RefCt=file_table[inode].refct;
    dir->DIR_FileSize=file_table[inode].size;
    ufs->dev->ops->write(ufs->dev,Entry(inode),dir,sz(dir_entry));
    
    ref_table[fd].valid=0;
    return 0;
  }

  int ufs_open(const char *pathname, int flags)
  {
    char abs_path[100];
    get_abs_path(pathname,abs_path);

    int inode=locate_file(abs_path);
    int fd=alloc_fd();
    if(inode==INT_MIN)
    {
      printf("No such file/directory\n");
      return -1;
    }
    else if(inode<0)
    {
      inode=-inode;
      if(flags&O_CREAT)//创建新文件
      {
        struct dir_entry* dir=(struct dir_entry*)kalloc_safe(sz(dir_entry));
        int new_inode=make_dir_entry(T_FILE,dir);
        ufs->dev->ops->write(ufs->dev,Entry(new_inode),(char*)dir,sz(dir_entry));
        
        char name[32];
        get_name(pathname,name);
        struct ufs_dirent* drt=(struct ufs_dirent*)kalloc_safe(sz(ufs_dirent));
        drt->inode=new_inode;
        strcpy(drt->name,name);
        write_data(&file_table[inode],file_table[inode].size,(char*)drt,sz(ufs_dirent));
        ref_table[fd].id=new_inode;
        file_table[new_inode].node=new_inode;
        file_table[new_inode].refct=1;
        file_table[new_inode].offset=0;
        file_table[new_inode].link_id=-1;
        file_table[new_inode].fs=ufs;
        file_table[new_inode].valid=1;
        file_table[new_inode].type=T_DIR;
        file_table[new_inode].size=0;
        char sem_name[32];
        sprintf(sem_name,"semlock_file_%d",new_inode);
        sem_init(&file_table[new_inode].sem,sem_name,1);
      }
      else
      {
        printf("No such file/directory\n");
        return -1;
      }
    }
    else
    {
      ref_table[fd].id=inode;
    }

    ref_table[fd].fd=fd;
    ref_table[fd].flags=flags;
    ref_table[fd].thread_id=_cpu();
    ref_table[fd].valid=1;
  
    return fd;
  }

  int ufs_lseek(int fd, int offset, int whence)
  {
    int node=ref_table[fd].id;
    if(!file_table[node].valid)
    {
      printf("File not opened\n");
      return -1;
    }
    int off=file_table[node].offset;
    int end=file_table[node].size;
    switch(whence)
    {
      case SEEK_SET:off=offset;break;
      case SEEK_CUR:off=off+offset;break;
      case SEEK_END:off=end+offset;break;
      default:break;
    }
    file_table[node].offset=off;
    return off;
  }

  int ufs_link(const char* oldpath,const char* newpath)
  {/*link改为直接在文件中创建一个文件，给它设定和oldpath相同的node
  新增加的inode的linkid设置为oldpath的inode*/
  char abs_newpath[256];
  char abs_oldpath[256];
  get_abs_path(newpath,abs_newpath);
  get_abs_path(oldpath,abs_oldpath);
  int old_inode=locate_file(abs_oldpath);
    assert(old_inode>=0);
  int pre_inode=locate_file(abs_newpath);
    assert(pre_inode<0&&pre_inode>INT_MIN);
    pre_inode=-pre_inode;

  struct dir_entry* dir=(struct dir_entry*)kalloc_safe(sz(dir_entry));
  int new_inode=make_dir_entry(T_FILE,dir);
  ufs->dev->ops->write(ufs->dev,Entry(new_inode),dir,sz(dir_entry));  

  file_table[new_inode].node=new_inode;
  file_table[new_inode].link_id=old_inode;
  file_table[new_inode].valid=1;
  char name[32];
  get_name(abs_newpath,name);
  struct ufs_dirent* drt=(struct ufs_dirent*)kalloc_safe(sz(ufs_dirent));
  strcpy(drt->name,name);
  drt->inode=new_inode;
  write_data(&file_table[pre_inode],file_table[pre_inode].size,(char*)drt,sz(ufs_dirent));
  return 0;
  }

  int ufs_unlink(const char* pathname)
  {
    char abs_path[256];
    get_abs_path(pathname,abs_path);
    int inode=locate_file(abs_path);
    assert(inode>=0);
    file_table[inode].valid=0;
    struct dir_entry* dir=(struct dir_entry*)kalloc_safe(sz(dir_entry));
    if(file_table[inode].link_id!=-1)//指代被disable
    {
      file_table[inode].valid=0;
      ufs->dev->ops->read(ufs->dev,Entry(inode),dir,sz(dir_entry));
      dir->DIR_Valid=0;
      ufs->dev->ops->write(ufs->dev,Entry(inode),dir,sz(dir_entry)); 
      
      while(file_table[inode].link_id!=-1)
      {inode=file_table[inode].link_id;
      }
    }

    file_table[inode].refct-=1;//检查本体需不需要disable
    if(file_table[inode].refct==0)
    {
      ufs->dev->ops->read(ufs->dev,Entry(inode),dir,sz(dir_entry));
      dir->DIR_Valid=0;
      ufs->dev->ops->write(ufs->dev,Entry(inode),dir,sz(dir_entry)); 
    }
    return 0;
  }

  int ufs_fstat(int fd,struct ufs_stat* buf)
  {
    
      int inode=ref_table[fd].id;
      filesystem_t* fs=file_table[inode].fs;
      assert(fs==ufs);
      
      buf->id=file_table[inode].node;
      buf->type=file_table[inode].type;
      buf->size=file_table[inode].size;
      return 0;
  }
  

  int ufs_mkdir(const char *pathname)
  {

    char abs_path[256];//该文件夹要被创建的路径
    get_abs_path(pathname,abs_path);
    int inode=locate_file((char*)pathname);
    printf("mkdir inode=%d\n",inode);
    if(inode>=0)
    {
      printf("Directory exists\n");
      return -1;
    }
    else
    {
          assert(0);
      struct dir_entry* dir=(struct dir_entry*)kalloc_safe(sz(dir_entry));
      int new_inode=make_dir_entry(T_DIR,dir);
      ufs->dev->ops->write(ufs->dev,Entry(new_inode),dir,sz(dir_entry));

      inode=-inode;
      char name[32];
      get_name(pathname,name);
      struct ufs_dirent* drt=(struct ufs_dirent*)kalloc_safe(sz(ufs_dirent));
      strcpy(drt->name,name);
      drt->inode=new_inode;
      write_data(&file_table[inode],file_table[inode].size,(char*)drt,sz(ufs_dirent));
      assert(0);
      return 0;
    }
    assert(0);
    return -1;
  }

  int exist_files=0;
  filesystem_t* ufs_init()
  {
    ufs=(filesystem_t*)kalloc_safe(sz(filesystem));
    ufs->ops=(fsops_t*)kalloc_safe(sz(fsops));
    strcpy(ufs->name,"ufs");
    ufs->dev=dev->lookup("sda");
    ufs->ops->write=ufs_write;
    ufs->ops->read=ufs_read;
    ufs->ops->close=ufs_close;
    ufs->ops->open=ufs_open;
    ufs->ops->lseek=ufs_lseek;
    ufs->ops->link=ufs_link;
    ufs->ops->unlink=ufs_unlink;
    ufs->ops->fstat=ufs_fstat;
    ufs->ops->mkdir=ufs_mkdir;
    ufs->dev->ops->read(ufs->dev,FS_START+47,(void*)&exist_files,4);
    struct dir_entry* dir=(struct dir_entry*)kalloc_safe(sz(dir_entry));
    for(int i=1;i<=exist_files;i++)//初始inode的加载在这里完成,文件从1开始,因为0无法区分locate_file的结果
    {
    ufs->dev->ops->read(ufs->dev,Entry(i),dir,sz(dir_entry));
    file_table[i].node=i;
    file_table[i].refct=1;
    file_table[i].offset=0;
    char sem_name[32];
    sprintf(sem_name,"semlock_file_%d",i);
    sem_init(&file_table[i].sem,sem_name,1);
    file_table[i].type=dir->DIR_FileType;
    file_table[i].size=dir->DIR_FileSize;
    file_table[i].valid=1;
    file_table[i].type=T_FILE;
    file_table[i].size=0;
    }
    return ufs;
  }