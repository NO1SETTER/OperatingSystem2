#include<vfs.h> 
#include<mkfs.h>
//extensions
  int read_entry(device_t *dev, off_t offset, void *buf,char* name)
  { //从offset开始只读一个short entry到buf中,但是会把之后的long entry也解析了,并且把完整的文件名字写到name里去
    struct sdir_entry* sdir=(struct sdir_entry*)buf;
    sd_read(dev,offset,buf,sizeof(struct sdir_entry));
    strncpy(name,sdir_entry->DIR_Name,15);
    
    int nr_entry=sdir->DIR_EntryNum;
    char buffer[sizeof(struct ldir_entry)+1];
    for(int i=1;i<=nr_entry;i++)
    {
      sd_read(dev,offset+i*sizeof(struct ldir_entry),buffer,sizeof(struct ldir_entry));
      strncpy(name+15+(i-1)*30,buffer+1,30);
    }
    return (nr_entry+1)*sizeof(struct sdir_entry);
  }

int locate_file_entry(char* path_name)///根据path_name(绝对路径)定位该项的entry的offset
{//如果找到了则返回offset,没找到当前应该写入的offset的负值
      assert(path_name[0]=='/');
      char dir_name[100];//当前文件夹的名字
      char name[256];//当前检索到的名字
      int lid=1,rid=1;
      struct sdir_entry* sdir=(struct sdir_entry*)kalloc_safe(struct sdir_entry);
      int offset=ENTRYSTART;
      int len=strlen(abs_path);
      for(int i=1;i<=len;i++)
      {
        if(abs_path[i]!='/'&&i!=strlen(struct sdir)) continue;
        rid=i;
        strncpy(dir_name,name+lid,rid-lid);
        while(1)
        {
          int increment=read_entry(ufs->dev,offset,sdir,name);
            if(strncmp(sdir->DIR_Sign,"DYG",3)!=0)
            {
              printf("No such file is this system");
              if(i!=len)
                return 0;
              else
                return -offset;
            }

            if(sdir->valid&&strcmp(dir_name,name)==0)
            {
            offset=Clu(sdir.DIR_FstClus);
            break;
            }
            offset=offset+increment;
        }
        lid=i+1;
      }
  return offset;
}

int get_abs_path(char *path,char* abs_path)
{
  if(path[0]=='/')
    strcpy(abs_path,path);
  else
    sprintf(abs_path,"%s/%s",cur->cur_path,path)
  return 1;
}

//standard realizations
  int ufs_write(int fd, void *buf, int count)
  {
    if(ref_table[fd].thread_id!=_cpu()||!ref_table[fd].valid)
    {
      printf("File not opened in this thread\n");
      return -1;
    }
    int file_id=ref_table[fd].id;
    if(!file_table[file_id].valid)
    {
      printf("File not opened\n");
      return -1;
    }
    int offset=file_table[file_id].offset;
    return write_data(ufs,&file_table[file_id],offset,buf,count);
  }

  int ufs_read(int fd, void *buf, int count)
  {
    if(ref_table[fd].thread_id!=_cpu()||!ref_table[fd].valid)
    {
      printf("File not opened in this thread\n");
      return -1;
    }
    int file_id=ref_table[fd].id;
    if(!file_table[file_id].valid)
    {
      printf("File not opened\n");
      return -1;
    }
    int offset=file_table[file_id].offset;
    return read_data(ufs,&file_table[file_id],offset,buf,count);
  }

  int ufs_close(int fd)
  {
    if(ref_table[fd].thread_id!=_cpu()||!ref_table[fd].valid)
    {
      printf("File not opened in this thread\n");
      return -1;
    }
    int file_id=ref_table[fd].id;
    if(!file_table[file_id].valid)
    {
      printf("File not opened\n");
      return -1;
    }

    int entry=file_table[file_id].entry;
    struct sdir_entry* sdir=(struct sdir_entry*)kalloc_safe(sizeof(struct sdir_entry));
    read_data(ufs->dev,entry,sdir,sizeof(struct sdir_entry));
    sdir->DIR_RefCt=ref_table[file_id].refct;
    sdir->DIR_FileSize=ref_table[file_id].Size;
    write_data(ufs->dev,entry,sdir,sizeof(struct sdir_entry));
    ref_table[file_id].valid=0;
    return 0;
  }

  int ufs_open(const char *pathname, int flags)
  {
    char abs_path[100];
    if(pathname[0]=='/')//当前目录下的文件
    strcpy(abs_path,pathname);
    else
    sprintf(abs_path,"%s/%s",cur->cur_path,pathname);

    int new_fd=alloc_fd();
    int file_id=-1;
      for(int i=0;i<nr_link;i++)
      {
        if(link_table[i].valid&&strcmp(link_table[i].pathname,abs_path)==0)
        {
          file_id=link_table[i].id;
          break;
        }
      }

    if(file_id==-1)//没打开过需要读磁盘
    {
    int offset=locate_file_entry(abs_path);
    if(offset<=0)
    {
      if(flags&O_CREAT)//创建,只允许在已存在的文件夹下创建
        {
          if(offset==0) 
          {
            printf("Trying to create file in an unexist directory\n");
            return-1;}

          offset=-offset;
          char name_[256];
          char pathname_[256];
          char buf_[256];
          strncpy(name_,pathname+lid,rid-lid);
          strncpy(pathname_,pathname,lid);
          struct sdir_entry* sdir=make_dir_entry(pathname_,name_,buf_);
          sd->write(fs->dev,offset,_buf,(sdir->DIR_EntryNum+1)*sizeof(struct sdir_entry));
          break;
        }
    }
    int new_id=alloc_file_id();
      file_table[new_id].offset=offset;
      file_table[new_id].id=new_id;
      file_table[new_id].refct=1;
      file_table[new_id].fs=ufs;
      sem_init(&file_table[new_id].sem);
      file_table[new_id].type=T_FILE;
      file_table[new_id].size=sdir.DIR_FileSize;
    int new link_id=alloc_link_id();
      if(!link_table[new_link_id].pathname)
        link_id[new_link_id].pathname=(char*)malloc(100);
      strncpy(link_id[new_link_id].pathname,abs_path,strlen(abs_path));
      link_table[link_id].id=new_id;
    file_id=new_id;
    }
      ref_table[new_fd].fd=new_fd;
      ref_table[new_fd].flags=flags;
      ref_table[new_fd].id=file_id;
      ref_table[new_fd].thread_id=_cpu();
      ref_table[new_fd].valid=1;
    return new_fd;
  }

  int ufs_lseek(int fd, int offset, int whence)
  {
    if(ref_table[fd].thread_id!=_cpu()||!ref_table[fd].valid)
    {
      printf("File not opened in this thread\n");
      return -1;
    }
    int file_id=ref_table[fd].id;
    if(!file_table[file_id].valid)
    {
      printf("File not opened\n");
      return -1;
    }
    int off=file_table[file_id].offset;
    int end=file_table[file_id].size;
    switch(whence)
    {
      case SEEK_SET:off=offset;break;
      case SEEK_CUR:off=off+offset;break;
      case SEEK_END:off=end+offset;break;
      default:break;
    }
    file_table[file_id].offset=off;
    return off;
  }

  int ufs_link(const char* oldpath.const char* newpath)
  {
    int old_id=-1;//判断被链接的文件是否已经被链接过了
    int new_id=-1;
    char abs_oldpath[256];
    char abs_newpath[256];
    get_abs_path(oldpath,abs_oldpath);
    get_abs_path(newpath,abs_newpath);

    for(int i=0;i<nr_link;i++)
    {
      if(link_table[i].valid&&strcmp(abs_oldpath,link_table[i].path)==0)
      {old_id=i;break;
      }
    }

    new_id=alloc_link_id();
    if(old_id==-1)
    {
      old_id=alloc_link_id();
      if(!link_table[old_id].pathname)
        link_table[old_id].pathname=(char*)kalloc_safe(256);
      strcpy(link_table[old_id].pathname,abs_oldpath);
      link_table[old_id].id=-1;
      link_table[old_id].refct=2;
      link_table[old_id].valid=1;
    }
    else
    {
      while(link_table[old_id].id!=-1)
      {  old_id=link_table[old_id].id;
      }
      link_table[old_id].refct+=1;
    }

      if(!link_table[new_id].pathname)
        link_table[new_id].pathname=(char*)kalloc_safe(256);
      strcpy(link_table[new_id].pathname,abs_newpath);
      link_table[new_id].id=old_id;
      link_table[new_id].refct=-1;
      link_table[new_id].valid=1;
    return 0;
  }

  int ufs_unlink(const char* pathname)
  {
    char abs_path[256];
    get_abs_path(pathname,abs_path);
    int id=-1;
    for(int i=0;i<nr_link;i++)
    {
      if(link_table[i].valid&&strcmp(abs_path,link_table[i].pathname)==0)
      { id=i;break;
      }
    }

    char real_path[256];
    int is_delete=0;
    if(id==-1)//两种可能:不存在该文件或者该文件未被链接过 
    {
      strcpy(real_path,abs_path);
      is_delete=1;         
    }
    else
    {
      link_table[id].valid=0;
      while(link_table[id].id!=-1)
      { id=link_table[id].id;
      }
      link_table[id].refct-=1;
      assert(link_table[id].refct>=0);
      if(refct==0)
        {
          strcpy(real_path,link_table[id].pathname);
          is_delete=1;
        }
    }

    if(is_delete)
    {
      int entry=locate_file_entry(real_path);
      if(entry<=0)
      { printf("No such file\n");
        return -1;
      }
      struct sdir_entry* sdir=(struct sdir_entry*)kalloc_safe(sizeof(struct sdir_entry));
      sd_read(ufs->dev,entry,sdir,sizeof(struct sdir_entry));
      sdir->valid=0;
      char FREE[4];
      *(int *)FREE=FAT_FREE;
      int cid=sdir->DIR_FstClus;
      int next_id=-1;
      while(1)
      {
        sd_read(ufs->dev,Fat(cid),&next_id,4);
        sd_write(ufs->dev,Fat(cid),&FREE,4);
        if(next_id==FAT_EOF) break;
        cid=next_id;
      }
    }
  }

  int ufs_fstat(int fd,struct ufs_stat* buf)
  {
    if(ref_table[fd].valid)
    {
      if(ref_table[fd].thread_id=_cpu())
      { 
      int file_id=ref_table[fd].id;
      filesystem_t* fs=file_table[file_id].fs;
      assert(fs==ufs);
      buf->id=file_table[file_id].id;
      buf->type=file_table[file_id].type;
      buf->size=file_table[file_id].size;
      return 0;
      }
    }
    return -1;
  }
  
  //思考:文件夹不像文件一样需要inode进行记录，只要在磁盘中进行修改即可
  int ufs_mkdir(const char *pathname)
  {
    char abs_path[256];//该文件夹要被创建的路径
    get_abs_path(path,abs_path);

    char path_name[256];
    char dir_name[256];//该文件夹名字
    
    int pos=-1;
    for(int i=strlen(abs_path);i>=0;i--)
    {
      if(abs_path[i]=='/')
      {
        pos=i;
        break;}
    }
    strncpy(path_name,abs_path,pos);
    strncpy(dir_name,abs_path+pos+1,strlen(abs_path)-pos-1);
    int offset=locate_file_entry(path_name);
    if(offset<=0)
    {
      printf("Trying to create directory in an unexist directory\n");
      return -1;
    }
    struct sdir_entry* sdir=(struct sdir_entry*)kalloc_safe(sizeof(struct sdir_entry));
    char buf[256];
    make_dir_entry(dir_name,buf);
    write_data_ondisk(fs,offset,,size);
  }
