#include <user.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/file.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define MB_SIZE (1024*1024)//0x1000是4KB,也是每一个cluster的大小,1MB有256个cluster
//1MB的预留空间
#define FS_START MB_SIZE
//4KB的文件头,占1个cluster
#define FAT_START (FS_START+0x1000)  //0x00101000
//256KB的FAT表,占64个cluster,最多256*32个Fat_Entry
#define ENTRY_START (FAT_START+64*0x1000)//0x00141000
//764KB的目录项文件区域,占191个cluster,最多191*128条目录项,所有的entry集中存放
#define DATA_START (ENTRY_START+191*0x1000)//0x00200000
//数据区事实上开始于2MB的地方

#define FAT_FREE 0xFFFFFFFA
#define FAT_BAD 0xFFFFFFF7
#define FAT_EOF 0xFFFFFFFF
#define FatEntrySize 0x4//4字节
#define Fat(x) (FAT_START+FatEntrySize*x) 
#define EntrySize 0x20
#define Entry(x) (ENTRY_START+EntrySize*x)
#define ClusterSize 0x1000
#define Clu(x) (FS_START+ClusterSize*x)
inline int min(int a,int b){return a<b?a:b;}
inline int max(int a,int b){return a>b?a:b;}

int fd;
void* disk;
int IMG_SIZE;
int BASE_SIZE;
int inode_ct=1;
struct dir_entry
{
  uint8_t  DIR_Valid;//指示当前目录项是否还有效(由于此文件系统没有删除,valid应该一直为1)
  uint8_t  DIR_FileType;//文件还是文件夹
  uint8_t  DIR_RefCt;//引用计数
  uint32_t DIR_Inode;//Inode号，对应它们是第几个Entry项目
  uint32_t DIR_FstClus;//第一块
  uint32_t DIR_FileSize;//文件大小,它与目前已经写入的大小保持一致!
  uint8_t  DIR_Sign[3];//标识
  uint8_t  DIR_Padding[14];
}__attribute__((packed));//32byte
//entry项均不存储名字,名字由目录文件内的ufs_dirent项存储,任意一个文件的entry项即为

struct fat_entry
{
  uint32_t id;
};
struct fat_header//类似FAT32实现,但是事实上它包括了Data之前的所有部分
{
    uint8_t  BS_jmpBoot[3];//boot code
    uint8_t  BS_OEMName[8];//代工厂名字?
    uint32_t BPB_BytePerSec:16;//每一个扇区的字节数
    uint8_t  BPB_SecPerClus;//每一个cluster的扇区数
    uint32_t BPB_RsvdSecCnt:16;//reserved region的扇区数
    uint8_t  BPB_NumFATs;//FAT表的数量1或２
    uint32_t BPB_RootEntCnt:16;//不重要，必须是０
    uint32_t BPB_TotSec32:16;//不重要，必须是０
    uint8_t  BPB_Media;//不重要
    uint32_t BPB_FATSz32:16;//不重要，必须是０
    uint32_t BS_VolID;//????
    uint8_t  BS_VolLab[11];
    uint8_t  BS_FilSysType[8];//字符串"FAT32"
    uint32_t BS_ExistFiles;
    uint8_t  padding1[459];//补充空位
    uint32_t signature_word:16;//0xaa55
    uint8_t  padding2[512*7];
    struct   fat_entry fat[(ENTRY_START-FAT_START)/sizeof(struct fat_entry)];
}__attribute__((packed));
struct fat_header* fh;


void fat_init();
int  cluster_alloc();
void parse_args(int argc,char *argv[]);
int make_dir_entry(int type,char* buf);//type指示文件/目录,attr指示属性;//type指示文件/目录,attr指示属性
int write_data(struct dir_entry* dir,int offset,char* buf,int size);
void recursive_mkfs(char* pathname,int inode,int depth);
/*每个Sec大小为0x200即0.5KB,每个cluster大小为0x1000即4KB,考虑最大256MB有64*1024个block
那么FAT表只用64*1024*4byte=256KB即可管理,FATSz为512Sec,16KB
*/

int main(int argc,char* argv[]){
  parse_args(argc,argv);
  fh=(struct fat_header*)(disk+FS_START);
  fh->BPB_BytePerSec=0x0200;
  fh->BPB_SecPerClus=0x8;
  fh->BPB_RsvdSecCnt=0x8;
  fh->BPB_NumFATs=0x1;
  fh->BPB_TotSec32=IMG_SIZE/fh->BPB_BytePerSec;
  fh->BPB_FATSz32=0x200;
  fh->BS_ExistFiles=0;
  fh->signature_word=0xaa55;
  strcpy((char*)fh->BS_FilSysType,"FAT32");
  fat_init();
  char buf[256];//Entry(0)写入的是根目录
  int start_node=make_dir_entry(T_DIR,buf);
  assert(start_node==0);
  lseek(fd,Entry(start_node),SEEK_SET);
  assert(write(fd,buf,sizeof(struct dir_entry))!=-1);
  recursive_mkfs(argv[3],start_node,0);
  fh->BS_ExistFiles=inode_ct;
  close(fd);
  munmap(disk, IMG_SIZE);
  printf("%d files in total\n",inode_ct);
}

//文件第一块分配,在这里完成
void fat_init()
{
for(int i=0;i<(ENTRY_START-FAT_START)/sizeof(struct fat_entry);i++)
  fh->fat[i].id=FAT_FREE;
}

int min_cid=(DATA_START-FS_START)/ClusterSize;//最小可用块
int cluster_alloc()
{
  if(fh->fat[min_cid].id==FAT_FREE)
  {
    min_cid=min_cid+1;
    return min_cid-1;
  }
  else
  {
    while(fh->fat[min_cid].id!=FAT_FREE)
    {min_cid=min_cid+1;}
    min_cid=min_cid+1;
    return min_cid-1;
  }
  return 0;
}


int make_dir_entry(int type,char* buf)//type指示文件/目录,attr指示属性
{ 
  uint32_t cid=cluster_alloc();//最小未用块的id
  fh->fat[cid].id=FAT_EOF;//该文件的第一块都写成EOF状态

  struct dir_entry* dir=(struct dir_entry* )malloc(sizeof(struct dir_entry));
  dir->DIR_Valid=1;
  dir->DIR_FileType=type;
  dir->DIR_RefCt=1;//初始时RefCt均为1
  dir->DIR_Inode=inode_ct++;
  dir->DIR_FstClus=cid;
  dir->DIR_FileSize=0;//写入目录项时文件的大小还是0
  strncpy((void*)dir->DIR_Sign,"DYG",3);
  memcpy(buf,(void*)dir,sizeof(struct dir_entry));
  free(dir);
  return inode_ct-1;
}

int write_data(struct dir_entry* dir,int offset,char* buf,int size)
{ //向dir指向的文件offset处写入来自buf的size个字节,返回所写到的磁盘偏移量
  assert(offset<=dir->DIR_FileSize);
  dir->DIR_FileSize=max(dir->DIR_FileSize,offset+size);//DIR_FileSize是已经写入的size
  int cid=dir->DIR_FstClus;
  while(offset>ClusterSize)
  {
    cid=fh->fat[cid].id;
    offset=offset-ClusterSize;
  } 
  int ret=lseek(fd,Clu(cid)+offset,SEEK_SET);
  assert(write(fd,buf,min(ClusterSize-offset,size))!=-1);
  assert(fsync(fd)!=-1);
  int write_bytes=min(ClusterSize-offset,size);
  int nextid;
  while(write_bytes<size)
  {
    nextid=fh->fat[cid].id;
    if(nextid==FAT_EOF)
    {
      nextid=cluster_alloc();
      fh->fat[cid].id=nextid;
      fh->fat[nextid].id=FAT_EOF;
    }
    cid=nextid;
    lseek(fd,Clu(cid),SEEK_SET);
    assert(write(fd,buf+write_bytes,min(ClusterSize,size))!=-1);
    assert(fsync(fd)!=-1);
    write_bytes=write_bytes+min(ClusterSize,size);
  }
  fh->fat[cid].id=FAT_EOF;
  return ret;
}

void recursive_mkfs(char* pathname,int inode,int depth)//pathname是路径,
//inode是上层目录dir_entry项的inode,depth指示在目录树中的深度
{
/*每一层要做两件事:
制作当前目录项并写入上层目录文件,写入当前文件的内容
*/
DIR* dir=opendir(pathname);
struct dirent* ptr;
int nr_file=0;
  while((ptr=readdir(dir))!=NULL)
  {
    if(strcmp(ptr->d_name,".")==0||strcmp(ptr->d_name,"..")==0) continue;//暂且不处理这种情况
    char d_name[256];//当前名字
      strcpy(d_name,ptr->d_name);
    char cur_path[256];//当前路径
      snprintf(cur_path,256,"%s/%s",pathname,d_name);

    char buf[33];
    int type=ptr->d_type==DT_DIR?T_DIR:T_FILE;
    int this_inode=make_dir_entry(type,buf); 
    lseek(fd,Entry(this_inode),SEEK_SET);
    assert(write(fd,buf,sizeof(struct dir_entry))!=-1);

    struct dir_entry* pre_dir=(struct dir_entry*)(disk+Entry(inode));
    struct dir_entry* now_dir=(struct dir_entry*)(disk+Entry(this_inode));
    struct ufs_dirent drt;
    drt.inode=this_inode;
    strncpy(drt.name,d_name,28);
    write_data(pre_dir,nr_file*sizeof(struct ufs_dirent),(void*)&drt,sizeof(struct ufs_dirent));
    nr_file=nr_file+1;
    if(type==T_DIR)
    {
        recursive_mkfs(cur_path,this_inode,depth+1);
    }
    else
    {
      int fd_temp;
      assert((fd_temp=open(cur_path,O_RDONLY))!=-1);
      int fsize=lseek(fd_temp,0,SEEK_END);
      lseek(fd_temp,0,SEEK_SET);
      char writebuf[ClusterSize+1];//一次写一个cluster
      while(fsize)
      {
        assert(read(fd_temp,writebuf,min(ClusterSize,fsize))!=-1);
        write_data(now_dir,now_dir->DIR_FileSize,writebuf,min(fsize,ClusterSize));
        fsize=fsize-min(fsize,ClusterSize);
      }
    }
  }
}

void parse_args(int argc,char *argv[])
{
  assert(argc==4);
  assert((fd = open(argv[2], O_RDWR|O_CREAT,S_IRWXU)) > 0);
  IMG_SIZE=atoi(argv[1])*MB_SIZE;
  BASE_SIZE=(lseek(fd,0,SEEK_END)/MB_SIZE+1)*MB_SIZE;
  assert(ftruncate(fd,BASE_SIZE+IMG_SIZE)==0);
  assert((disk = mmap(NULL, IMG_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) != (void *)-1);
}
