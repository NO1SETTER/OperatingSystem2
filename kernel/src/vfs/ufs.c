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

int locate_file_entry(char* path_name)///根据path_name(绝对路径)定位offset
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

//standard realizations
  int ufs_write(int fd, void *buf, int count)
  {
    if(ref_table[fd].thread_id!=_cpu()||!ref_table[fd].valid)
    {
      printf("File not opened in this thread\n");
      return -1;
    }
    int file_id=ref_table[fd].id;
    
  }

  int ufs_read(int fd, void *buf, int count)
  {

  }

  int ufs_close(int fd)
  {

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

  }
  
  int ufs_mkdir(const char *pathname)
  {

  }