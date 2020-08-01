#include<stdint.h>
#include<mkfs.h>

sem_t cluster_lock;
int cluster_set[10000];//记录小于max_cluster可用的cluster
int max_cluster=-1;
int nr_cluster=0;
void push_cluster(int x) {cluster_set[nr_cluster++]=x;}
int pop_cluster() {return cluster_set[--nr_cluster];}
int alloc_cluster()
{
    sem_wait(&cluster_lock);
    if(max_cluster==-1) ufs->dev->ops->read(ufs->dev,FS_START+51,(void*)&max_cluster,4);
    //assert(nr_cluster>=0);
    int ret=-1;
    if(nr_cluster) ret=pop_cluster();
    else ret=max_cluster;
    max_cluster=ret+1;
    sem_signal(&cluster_lock);
    return ret;
}


int make_dir_entry(int type,int fid,struct dir_entry* dir)//type指示文件/目录,attr指示属性
{ 
  uint32_t cid=alloc_cluster();//最小未用块的id
  char EOF[4];
  *(int*)EOF=FAT_EOF;
  ufs->dev->ops->write(ufs->dev,Fat(cid),EOF,4);
  int inode=alloc_inode();//由于新inode的建立必定伴随着新entry的建立,新inode的申请均在此中进行
  dir->DIR_Valid=1;
  dir->DIR_FileType=type;
  dir->DIR_RefCt=1;//初始时RefCt均为1
  dir->DIR_Inode=inode;
  dir->DIR_FatherInode=fid;
  dir->DIR_FstClus=cid;
  dir->DIR_FileSize=0;//写入目录项时文件的大小还是0
  strncpy((void*)dir->DIR_Sign,"DYG",3);
  return inode;
}

//向node指向的文件的offset处写入来自buf的size个字节
int write_data(inode_t* node,int offset,char* buf,int size)
{
    #ifdef DEBUG_
    printf("Write to file:%d, size:%d\n",node->node,node->size);
    printf("Write %d bytes at offset %d\n",size,offset);
    #endif
    if(offset>node->size)
    {
        #ifdef DEBUG_
        printf("Write overflow\n");
        #endif
        return -1;
    }

    node->size=max(node->size,offset+size);
    int cid = node->cid;
    while(offset>ClusterSize)
    {
        ufs->dev->ops->read(ufs->dev,Fat(cid),&cid,4);
        offset=offset-ClusterSize;
    }
    int write_start=Clu(cid)+offset;
    ufs->dev->ops->write(ufs->dev,write_start,buf,min(ClusterSize-offset,size));
    int write_bytes=min(ClusterSize-offset,size);
    int next_id;
    
    char EOF[4];
    *(int *)EOF=FAT_EOF;
    while(write_bytes<size)
    {
         ufs->dev->ops->read(ufs->dev,Fat(cid),&next_id,4);
        if(next_id==FAT_EOF)
        {
            next_id=alloc_cluster();
            ufs->dev->ops->write(ufs->dev,Fat(cid),&next_id,4);
            ufs->dev->ops->write(ufs->dev,Fat(next_id),&EOF,4);
        }
        cid=next_id;
        ufs->dev->ops->write(ufs->dev,Clu(cid),buf+write_bytes,min(size,ClusterSize));
        write_bytes=write_bytes+min(ClusterSize,size);
    }
    ufs->dev->ops->write(ufs->dev,Fat(cid),&EOF,4);
    return size;
}



int read_data(inode_t* node,int offset,char* buf,int size)
{//应该以node中的数据为准,磁盘中的数据可能未更新
    #ifdef DEBUG_
    printf("Read from file:%d, size:%d\n",node->node,node->size);
    printf("Read %d bytes at offset %d\n",size,offset);
    #endif
    if(offset+size>node->size)
    {
        #ifdef DEBUG_
        printf("Read overflow\n");
        #endif
        return -1;
    }
    int cid=node->cid;
    while(offset>ClusterSize)
    {
      ufs->dev->ops->read(ufs->dev,Fat(cid),&cid,4);
      offset=offset-ClusterSize;
    }

    int read_start=Clu(cid)+offset;
    ufs->dev->ops->read(ufs->dev,read_start,buf,min(ClusterSize-offset,size));
    int read_bytes=min(ClusterSize-offset,size);
    int next_id;

    while(read_bytes<size)
    {
        ufs->dev->ops->read(ufs->dev,Clu(cid),buf+read_bytes,min(size,ClusterSize));
        read_bytes=read_bytes+min(ClusterSize,size);
        ufs->dev->ops->read(ufs->dev,Fat(cid),&next_id,4);
        if(next_id==FAT_EOF&&read_bytes!=size)
        {
            #ifdef DEBUG_
            printf("read outside file\n");
            #endif
            return -1;
        }
        cid=next_id;
    }
    return size;
}


