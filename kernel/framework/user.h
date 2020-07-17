#include <stdint.h>

#define T_DIR     1
#define T_FILE    2
#define T_REF     3

#define SEEK_CUR  0
#define SEEK_SET  1
#define SEEK_END  2

#define O_RDONLY  00000000
#define O_WRONLY  00000001
#define O_RDWR    00000002
#define O_CREAT   00000100

struct ufs_stat {
  uint32_t id, type, size;
};

struct ufs_dirent {
  uint32_t inode;
  char name[28];
} __attribute__((packed));

//以下为mkfs中的原始实现
#define MB_SIZE 1024*1024//0x1000是4KB,也是每一个cluster的大小,1MB有256个cluster
#define FS_START MB_SIZE//1MB的预留空间
#define FAT_START 1024*1024+0x1000//4KB的文件头,占1个cluster
#define ENTRY_START 1024*1024+0x1000+5*0x1000//20KB的根目录项文件区域,占5个cluster,最多640条目录项,根目录本身的一个短目录项存放在该区域的起始
#define DATA_START 1024*1024+0x1000+0x1000+64*0x1000//256KB的FAT表,占64个cluster

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
  uint8_t  valid;//指示当前目录项是否还有效
  uint8_t  DIR_EntryType;//长文件头还是短文件头
  uint32_t DIR_FileType:16;//文件/文件夹
  uint32_t DIR_EntryNum:16;//目录项有几条
  uint32_t DIR_FstClus;//第一块
  uint32_t DIR_FileSize;//文件大小,它与目前已经写入的大小保持一致!
  uint8_t DIR_NameLen;//名字长度
  uint8_t DIR_Sign[3];//标识
  uint8_t  DIR_Name[15];
}__attribute__((packed));//32byte

struct ldir_entry
{
  uint8_t LDIR_EntryType;
  uint8_t LDIR_Name[30];
  uint8_t LDIR_EndSign;
}__attribute__((packed));//32byte



