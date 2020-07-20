#include<common.h>
#include<mkfs.h>


struct sdir_entry* make_dir_entry(int type,char* name,char* buf)//type指示文件/目录,attr指示属性
{ 
  uint32_t len=strlen(name);
  uint32_t cid=cluster_alloc();//最小未用块的id
  struct sdir_entry* sdir=(struct sdir_entry* )malloc(sizeof(struct sdir_entry));
  sdir->valid=1;
  sdir->DIR_EntryType=SDIRENTRY;
  sdir->DIR_FileType=type;
  sdir->DIR_RefCt=1;
  sdir->DIR_EntryNum=1;
  sdir->DIR_FstClus=cid;
  sdir->DIR_FileSize=0;//写入目录项时文件的大小均初始化为0
  sdir->DIR_NameLen=len;
  
  memcpy((void*)sdir->DIR_Name,name,15);
  memcpy(buf,(void*)sdir,sizeof(struct sdir_entry));
  if(len>15)
  {
    len=len-15;
    int nr_ldir=0;
    while(len>30)
    {
      struct ldir_entry* ldir=(struct ldir_entry*)malloc(sizeof(struct ldir_entry));
      ldir->LDIR_EntryType=LDIRENTRY;
      ldir->LDIR_EndSign=DIR_NOTEND;
      memcpy((void*)ldir->LDIR_Name,name+15+nr_ldir*30,30);
      memcpy(buf+sizeof(struct sdir_entry)+nr_ldir*sizeof(struct ldir_entry)
      ,(void*)ldir,sizeof(struct ldir_entry));
      free(ldir);
      nr_ldir=nr_ldir+1;
      len=len-30;
      sdir->DIR_EntryNum=sdir->DIR_EntryNum+1;
    }
      struct ldir_entry* ldir=(struct ldir_entry*)malloc(sizeof(struct ldir_entry));
      ldir->LDIR_EntryType=LDIRENTRY;
      ldir->LDIR_EndSign=DIR_END;
      memcpy((void*)ldir->LDIR_Name,name+15+nr_ldir*30,len);
      memcpy(buf+sizeof(struct sdir_entry)+nr_ldir*sizeof(struct ldir_entry)
      ,(void*)ldir,sizeof(struct ldir_entry));
      free(ldir);
      sdir->DIR_EntryNum=sdir->DIR_EntryNum+1;
  }
  return sdir;
}

int write_data(filesystem_t* fs,inode_t* node,int offset,char* buf,int size)
{
    struct sdir_entry* sdir=(struct sdir_entry*kalloc_safe(sizeof(struct sdir_entry));
    sd_read(fs->dev,node->entry,&sdir,sizeof(struct sdir_entry));
    if(offset>sdir->DIR_FileSize)
    {
        printf("Write overflow\n");
        return -1;
    }

    node->size=max(node->size,offset+size);
    int cid = sdir->DIR_FstClus;
    while(offset>ClusterSize)
    {
        sd_read(fs->dev,Fat(cid),&cid,4);
        offset=offset-ClusterSize;
    }
    int write_start=Clu(cid)+offset;
    sd_write(fs->dev,write_start,buf,min(size,ClusterSize));
    int write_bytes=min(ClusterSize-write_start,size);
    int next_id;
    
    char EOF[4];
    *(int *)EOF=FAT_EOF;
    while(write_bytes<size)
    {
        sd_read(fs->dev,Fat(cid),&next_id,4);
        if(next_id==FAT_EOF)
        {

            next_id=alloc_cluster();
            sd_write(fs->dev,Fat(cid),&next_id,4);
            sd_write(fs->dev,Fat(next_id),&EOF,4);
        }
        cid=next_id;
        sd_write(fs->dev,Clu(cid),buf+write_bytes,min(size,ClusterSize));
        write_bytes=write_bytes+min(ClusterSize+size);
    }
    sd_write(fs->dev,Fat(cid),&EOF,4);
    return write_start;
}

int read_data(inode_t* node,int offset,char* buf,int size)
{
    struct sdir_entry sdir;
    sd_read(fs->dev,offset,&sdir,sizeof(struct sdir_entry));
    if(offset+size>sdir->DIR_FileSize)
    {
        printf("Read overflow\n");
        return -1;
    }
    int cid = sdir->DIR_FstClus;
    while(offset>ClusterSize)
    {
        sd_read(fs->dev,Fat(cid),&cid,4);
        offset=offset-ClusterSize;
    }

    int read_start=Clu(cid)+offset;
    sd_read(fs->dev,write_start,buf,min(size,ClusterSize));
    int write_bytes=min(ClusterSize-write_start,size);
    int next_id;

    while(read_bytes<size)
    {
        sd_read(fs->dev,Clu(cid),buf+read_bytes,min(size,ClusterSize));
        read_bytes=read_bytes+min(ClusterSize+size);
        
        sd_read(fs->dev,Fat(cid),&next_id,4);
        if(next_id==FAT_EOF&&read_bytes!=size)
        {
            printf("read outside file\n");
            return -1;
        }
        cid=next_id;
    }

    return read_start;
}

int write_data_ondisk(filesystem_t* fs,int entry,int offset,char* buf,int size)
{
    struct sdir_entry* sdir=(struct sdir_entry*)kalloc_safe(sizeof(struct sdir_entry));
    sd_read(fs->dev,entry,sdir,sizeof(struct sdir_entry));
    if(offset>sdir->DIR_FileSize)
    {
        printf("Write overflow\n");
        return -1;
    }

    int cid = sdir->DIR_FstClus;
    while(offset>ClusterSize)
    {
        sd_read(fs->dev,Fat(cid),&cid,4);
        offset=offset-ClusterSize;
    }
    int write_start=Clu(cid)+offset;
    sd_write(fs->dev,write_start,buf,min(size,ClusterSize));
    int write_bytes=min(ClusterSize-write_start,size);
    int next_id;
    
    char EOF[4];
    *(int *)EOF=FAT_EOF;
    while(write_bytes<size)
    {
        sd_read(fs->dev,Fat(cid),&next_id,4);
        if(next_id==FAT_EOF)
        {

            next_id=alloc_cluster();
            sd_write(fs->dev,Fat(cid),&next_id,4);
            sd_write(fs->dev,Fat(next_id),&EOF,4);
        }
        cid=next_id;
        sd_write(fs->dev,Clu(cid),buf+write_bytes,min(size,ClusterSize));
        write_bytes=write_bytes+min(ClusterSize+size);
    }
    sd_write(fs->dev,Fat(cid),&EOF,4);
    sdir->DIR_FileSize=max(sdir->DIR_FileSize,offset+size);
    sd_write(fs->dev,offset,sdir,sizeof(struct sdir_entry));
    return write_start;
}