#include<common.h>
#include<mkfs.h>


struct sdir_entry* make_dir_entry(char* pathname,char* name,char* buf)//type指示文件/目录,attr指示属性
{ 
  uint32_t type=T_FILE;
  uint32_t len=strlen(name);
  uint32_t cid=cluster_alloc();//最小未用块的id
  struct sdir_entry* sdir=(struct sdir_entry* )malloc(sizeof(struct sdir_entry));
  sdir->DIR_EntryType=SDIRENTRY;
  sdir->DIR_FileType=type;
  sdir->DIR_EntryNum=1;
  sdir->DIR_FstClus=cid;
  fh->fat[cid].id=FAT_EOF;//该文件的第一块都写成EOF状态
  sdir->DIR_FileSize=0;//写入目录项时文件的大小还是0
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
    struct sdir_entry sdir;
    sd_read(fs->dev,offset,&sdir,sizeof(struct sdir_entry));
    if(offset>sdir->DIR_FileSize)
    {
        printf("Write overflow\n");
        return -1;
    }
    sdir->DIR_FileSize=max(sdir->DIR_FileSize,offset+size);
    int cid = sdir->DIR_FstClus;
    while(offset>ClusterSize)
    {
        sd_read(fs->dec,Fat(cid),&cid,4);
        offset=offset-ClusterSize;
    }
    int ret=Clu(cid)+offset;
    sd_write()
}

int read_data(inode_t* node,int offset,char* buf,int size)
{

}