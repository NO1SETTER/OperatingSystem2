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
#define MB_SIZE 1024*1024

//0x1000是4KB,也是每一个cluster的大小,1MB有256个cluster
#define SDA_START 1024*1024//1MB的预留空间
#define FAT_START 1024*1024+0x1000//4KB的文件头,占1个cluster
#define ENTRY_START 1024*1024+0x1000+5*0x1000//20KB的根目录项文件区域,占5个cluster,最多640条目录项,根目录本身的一个短目录项存放在该区域的起始
#define DATA_START 1024*1024+0x1000+0x1000+64*0x1000//256KB的FAT表,占64个cluster

#define FAT_FREE 0
#define FAT_BAD 0xFFFFFFF7
#define FAT_EOF 0xFFFFFFFF
#define FatEntrySize 0x4//32字节
#define Fat(x) FAT_START+FatEntrySize*x 
#define ClusterSize 0x1000
#define Clu(x) SDA_START+ClusterSize*x

//初始化为data_start后的第一个cluster
//#define R_OK 2
//#define W_OK 4
//#define O_RDONLY        00000000
//#define O_WRONLY        00000001
//#define O_RDWR          00000002
#define T_DIR  1
#define T_FILE 2
#define SDIRENTRY 1
#define LDIRENTRY 2
#define DIR_END 1
#define DIR_NOTEND 2
inline int min(int a,int b){return a<b?a:b;}
inline int max(int a,int b){return a>b?a:b;}

int fd;
void* disk;
int IMG_SIZE;
int BASE_SIZE;



struct sdir_entry//这个和标准Fat实现不一样,sdir在ldir前面，并去掉了一些不必要的属性
{
  uint8_t  DIR_EntryType;//长文件头还是短文件头
  uint32_t DIR_FileType;//
  uint32_t DIR_FstClus;//第一块
  uint32_t DIR_FileSize;//文件大小,它与目前已经写入的大小保持一致!
  uint32_t DIR_NameLen;//名字长度
  uint8_t  DIR_Name[15];
}__attribute__((packed));//32byte

struct ldir_entry
{
  uint8_t LDIR_EntryType;
  uint8_t LDIR_Name[30];
  uint8_t LDIR_EndSign;
}__attribute__((packed));//32byte

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
    uint32_t BPB_TotSec16:16;//不重要，必须是０
    uint8_t  BPB_Media;//不重要
    uint32_t BPB_FATSz16:16;//不重要，必须是０
    uint32_t BS_VolID;//????
    uint8_t  BS_VolLab[11];
    uint8_t  BS_FilSysType[8];//字符串"FAT32"
    uint8_t  padding1[420];//补充空位
    uint32_t signature_word:16;//0xaa55
    uint8_t  padding2[512*7];
    struct   fat_entry fat[(ENTRY_START-FAT_START)/sizeof(struct fat_entry)];
}__attribute__((packed));
struct fat_header* fh;


void fat_init();
int cluster_alloc();
void parse_args(int argc,char *argv[]);
struct sdir_entry* make_dir_entry(struct dirent* ptr,char* pathname,char* name,char* buf);//type指示文件/目录,attr指示属性
void write_file(struct sdir_entry* sdir,int offset,int size,char* buf);
void write_dir(char* pathname,int dir_offset,int depth);
/*每个Sec大小为0x200即0.5KB,每个cluster大小为0x1000即4KB,考虑最大256MB有64*1024个block
那么FAT表只用64*1024*4byte=256KB即可管理,FATSz为512Sec,16KB
*/

int main(int argc,char* argv[]){
  parse_args(argc,argv);
  fat_init();
  fh=(struct fat_header*)disk
  fh->BPB_BytePerSec=0x0200;
  fh->BPB_SecPerClus=0x8;
  fh->BPB_RsvdSecCnt=0x8;
  fh->BPB_NumFATs=0x1;
  fh->BPB_TotSec32=IMG_SIZE/BPB_BytePerSec;
  fh->FATSz=0x200;
  write_dir(argv[3],0,0);
  munmap(disk, IMG_SIZE);
  close(fd);
}

//文件第一块分配,在这里完成
void fat_init()
{
for(int i=0;i<(ENTRY_START-FAT_START)/sizeof(struct fat_entry);i++)
fh->fat[i].id=FAT_FREE;
}

int min_cid=(DATA_START-SDA_START)/ClusterSize;//最小可用块
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

struct sdir_entry* make_dir_entry(struct dirent* ptr,char* pathname,char* name,char* buf)//type指示文件/目录,attr指示属性
{ //根据ptr创造一个dir_entry到buf中
  uint32_t type;
  if(ptr->d_type==DT_DIR) 
    type=T_DIR;
  else type=T_FILE;
  uint32_t len=ptr->d_reclen;
  uint32_t cid=cluster_alloc();//最小未用块的id
  struct sdir_entry* sdir=(struct sdir_entry* )malloc(sizeof(struct sdir_entry));
  sdir->DIR_EntryType=SDIRENTRY;
  sdir->DIR_FileType=type;
  sdir->DIR_FstClus=cid;
  fh->fat[cid].id=FAT_EOF;//有该文件数据的最后一块是EOF
  sdir->DIR_FileSize=0;//写入目录项时文件的大小还是0
  sdir->DIR_NameLen=len;
  
  strncpy((void*)sdir->DIR_Name,name,15);
  strncpy(buf,(void*)sdir,sizeof(struct sdir_entry));

  if(len>15)
  {
    len=len-15;
    int nr_ldir=0;
    while(len>30)
    {
      struct ldir_entry* ldir=(struct ldir_entry*)malloc(sizeof(struct ldir_entry));
      ldir->LDIR_EntryType=LDIRENTRY;
      ldir->LDIR_EndSign=DIR_NOTEND;
      strncpy((void*)ldir->LDIR_Name,name+15+nr_ldir*30,30);
      strncpy(buf+sizeof(struct sdir_entry)+nr_ldir*sizeof(struct ldir_entry)
      ,(void*)ldir,sizeof(struct ldir_entry));
      free(ldir);
      nr_ldir=nr_ldir+1;
      len=len-30;
    }
      struct ldir_entry* ldir=(struct ldir_entry*)malloc(sizeof(struct ldir_entry));
      ldir->LDIR_EntryType=LDIRENTRY;
      ldir->LDIR_EndSign=DIR_END;
      strncpy((void*)ldir->LDIR_Name,name+15+nr_ldir*30,len);
      strncpy(buf+sizeof(struct sdir_entry)+nr_ldir*sizeof(struct ldir_entry)
      ,(void*)ldir,sizeof(struct ldir_entry));
      free(ldir);
  }
  return sdir;
}

void write_data(struct sdir_entry* sdir,int offset,char* buf,int size)
{//像sdir指向的文件offset处写入来自buf的size个字节
  assert(offset<=sdir->DIR_FileSize);
  sdir->DIR_FileSize=max(sdir->DIR_FileSize,offset+size);
  int cid=sdir->DIR_FstClus;
  printf("First Cluster id:%d\n",cid);
  while(offset>ClusterSize)
  {
    cid=fh->fat[cid].id;
    offset=offset-ClusterSize;
  }
  lseek(fd,Clu(cid)+offset,SEEK_SET);
  assert(write(fd,buf,min(ClusterSize-offset,size))!=-1);
  int write_bytes=min(ClusterSize-offset,size);
  int nextid;
  while(write_bytes<size)
  {
    nextid=fh->fat[cid].id;
    printf("next id:%d\n",nextid);
    if(nextid==FAT_EOF)
    {
      nextid=cluster_alloc();
      printf("new id:%d\n",nextid);
      fh->fat[cid].id=nextid;
    }
    cid=nextid;
    lseek(fd,Clu(cid),SEEK_SET);
    assert(write(fd,buf,min(ClusterSize,size))!=-1);
    write_bytes=write_bytes+min(ClusterSize,size);
  }
  fh->fat[cid].id=FAT_EOF;
}

void write_dir(char* pathname,int dir_offset,int depth)//pathname是路径,
//diroffset是上层目录sdir项的偏移量,depth指示在目录树中的深度
{
DIR* dir=opendir(pathname);
struct dirent* ptr;
int write_offset=0;
  while((ptr=readdir(dir))!=NULL)
  {
    if(strcmp(ptr->d_name,".")==0||strcmp(ptr->d_name,"..")==0) continue;//暂且不处理这种情况
    char buf[256];//允许一个文件最多十个目录项
    char d_name[256];
    strcpy(d_name,ptr->d_name);
    char cur_path[256];
    snprintf(cur_path,256,"%s/%s",pathname,d_name);
    struct sdir_entry* now_dir= make_dir_entry(ptr,cur_path,ptr->d_name,buf);
    //make entry --> 写entry --> 写data
    printf("Making entry for %s\n",d_name);
    if(depth==0)
    {
      lseek(fd,ENTRY_START+write_offset,SEEK_SET);
      assert(write(fd,buf,strlen(buf))!=-1);
    }
    else
    {
      struct sdir_entry* sdir=(struct sdir_entry*)malloc(sizeof(struct sdir_entry));
      lseek(fd,dir_offset,SEEK_SET);
      assert(read(fd,sdir,sizeof(struct sdir_entry))!=-1);//读取上层目录项的目录项
      write_data(sdir,sdir->DIR_FileSize,buf,strlen(buf));
    }
    printf("Entry for %s written\n",d_name);
    //目录项的文件内容在下一层递归写,非目录项在本层写
    if(ptr->d_type==DT_DIR)
    {
      write_dir(cur_path,ENTRY_START+write_offset,depth+1);
    }
    else
    {
      int fd_temp=open(cur_path,O_RDONLY);
      int fsize=lseek(fd_temp,0,SEEK_END);
      printf("fsize=%d\n",fsize);
      printf("fsize=%d\n",now_dir->DIR_FileSize);
      lseek(fd_temp,0,SEEK_SET);
      char buf[ClusterSize+1];
      while(fsize)
      {
        assert(read(fd,buf,min(ClusterSize,fsize))!=-1);
        write_data(now_dir,now_dir->DIR_FileSize,buf,min(fsize,ClusterSize));
        fsize=fsize-min(fsize,ClusterSize);
      }
    }
    printf("Data for %s written\n",d_name);
    write_offset=write_offset+strlen(buf);
  }

}

void parse_args(int argc,char *argv[])
{
assert(argc==4);
assert((fd = open(argv[2], O_RDWR)) > 0);
IMG_SIZE=atoi(argv[1])*MB_SIZE;
BASE_SIZE=(lseek(fd,0,SEEK_END)/MB_SIZE+1)*MB_SIZE;
assert(ftruncate(fd,BASE_SIZE+IMG_SIZE)==0);
assert((disk = mmap(NULL, IMG_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) != (void *)-1);
}