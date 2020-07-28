#include<stdint.h>
#include<mkfs.h>

int cluster_set[10000];//记录小于max_cluster可用的cluster
int start_cluster=(DATA_START-FS_START)/ClusterSize;
int max_cluster=(DATA_START-FS_START)/ClusterSize;
int nr_cluster=0;
void push_cluster(int x) {cluster_set[nr_cluster++]=x;}
int pop_cluster() {return cluster_set[--nr_cluster];}
int alloc_cluster()
{
    assert(nr_cluster>=0);
    if(nr_cluster) return pop_cluster();
    return max_cluster;
}


int make_dir_entry(int type,struct dir_entry* dir)//type指示文件/目录,attr指示属性
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
  dir->DIR_FstClus=cid;
  dir->DIR_FileSize=0;//写入目录项时文件的大小还是0
  strncpy((void*)dir->DIR_Sign,"DYG",3);
  return inode;
}

//向node指向的文件的offset处写入来自buf的size个字节
int write_data(inode_t* node,int offset,char* buf,int size)
{
    printf("Write to file:%d, size:%d\n",node->node,node->size);
    printf("Write %d bytes at offset %d\n",size,offset);
    filesystem_t* fs=node->fs;
    if(offset>node->size)
    {
        printf("Write overflow\n");
        return -1;
    }

    node->size=max(node->size,offset+size);
    int cid = node->cid;
    while(offset>ClusterSize)
    {
        ufs->dev->ops->read(fs->dev,Fat(cid),&cid,4);
        offset=offset-ClusterSize;
    }
    int write_start=Clu(cid)+offset;
    fs->dev->ops->write(fs->dev,write_start,buf,min(ClusterSize-offset,size));
    int write_bytes=min(ClusterSize-offset,size);
    int next_id;
    
    char EOF[4];
    *(int *)EOF=FAT_EOF;
    while(write_bytes<size)
    {
         fs->dev->ops->read(fs->dev,Fat(cid),&next_id,4);
        if(next_id==FAT_EOF)
        {
            next_id=alloc_cluster();
            fs->dev->ops->write(fs->dev,Fat(cid),&next_id,4);
            fs->dev->ops->write(fs->dev,Fat(next_id),&EOF,4);
        }
        cid=next_id;
        fs->dev->ops->write(fs->dev,Clu(cid),buf+write_bytes,min(size,ClusterSize));
        write_bytes=write_bytes+min(ClusterSize,size);
    }
    fs->dev->ops->write(fs->dev,Fat(cid),&EOF,4);
    return write_start;
}



int read_data(inode_t* node,int offset,char* buf,int size)//应该以node中的数据为准,磁盘中的数据可能未更新
{
    //printf("Read from file:%d, size:%d\n",node->node,node->size);
    //printf("Read %d bytes at offset %d\n",size,offset);
    filesystem_t* fs=node->fs;
    if(offset+size>node->size)
    {
        printf("Read overflow\n");
        return -1;
    }
    
    int cid=node->cid;
    while(offset>ClusterSize)
    {
        fs->dev->ops->read(fs->dev,Fat(cid),&cid,4);
        offset=offset-ClusterSize;
    }

    int read_start=Clu(cid)+offset;
    fs->dev->ops->read(fs->dev,read_start,buf,min(ClusterSize-offset,size));
    int read_bytes=min(ClusterSize-offset,size);
    int next_id;

    while(read_bytes<size)
    {
        fs->dev->ops->read(fs->dev,Clu(cid),buf+read_bytes,min(size,ClusterSize));
        read_bytes=read_bytes+min(ClusterSize,size);
        fs->dev->ops->read(fs->dev,Fat(cid),&next_id,4);
        if(next_id==FAT_EOF&&read_bytes!=size)
        {
            printf("read outside file\n");
            return -1;
        }
        cid=next_id;
    }
    return read_start;
}


