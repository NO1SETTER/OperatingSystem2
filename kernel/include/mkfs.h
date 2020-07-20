#include<vfs.h>
#include<common.h>
//这个文件是照搬mkfs中的文件系统实现,为需要用到磁盘读写的函数提供方便

#define MB_SIZE 1024*1024//0x1000是4KB,也是每一个cluster的大小,1MB有256个cluster
//1MB的预留空间
#define FS_START MB_SIZE
//4KB的文件头,占1个cluster
#define FAT_START FS_START+0x1000
//256KB的FAT表,占64个cluster,最多256*32个Fat_Entry
#define ENTRY_START FAT_START+64*0x1000
//20KB的根目录项文件区域,占5个cluster,最多640条目录项,根目录本身的一个短目录项存放在该区域的起始
#define DATA_START ENTRY_START+5*0x1000

#define FAT_FREE 0xFFFFFFFA
#define FAT_BAD 0xFFFFFFF7
#define FAT_EOF 0xFFFFFFFF
#define FatEntrySize 0x4//32字节
#define Fat(x) FAT_START+FatEntrySize*x 
#define ClusterSize 0x1000
#define Clu(x) FS_START+ClusterSize*x

#define SDIRENTRY 1
#define LDIRENTRY 2
#define DIR_END 1
#define DIR_NOTEND 2

struct sdir_entry//这个和标准Fat实现不一样,sdir在ldir前面，并去掉了一些不必要的属性
{
  uint8_t  valid;//指示当前目录项是否还有效(由于此文件系统没有删除,valid应该一直为1)
  uint8_t  DIR_EntryType;//长文件头还是短文件头
  uint8_t  DIR_FileType;//文件还是文件夹
  uint8_t  DIR_RefCt;//链接计数
  uint8_t  DIR_EntryNum;//目录项有几条
  uint32_t DIR_FstClus;//第一块
  uint32_t DIR_FileSize;//文件大小,它与目前已经写入的大小保持一致!
  uint8_t  DIR_NameLen;//名字长度
  uint8_t  DIR_Sign[3];//标识
  uint8_t  DIR_Name[15];
}__attribute__((packed));//32byte

struct ldir_entry
{
  uint8_t LDIR_EntryType;
  uint8_t LDIR_Name[30];
  uint8_t LDIR_EndSign;
}__attribute__((packed));//32byte


struct sdir_entry* make_dir_entry(int type,char* name,char *buf);
int write_data(struct sdir_entry* sdir,int offset,char* buf,int size);
int read_data(struct sdir_entry* sdir,int offset,char* buf,int size);
void cluster_alloc();
