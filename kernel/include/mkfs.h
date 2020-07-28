#include<vfs.h>
#pragma once
//这个文件是照搬mkfs中的文件系统实现,为需要用到磁盘读写的函数提供方便

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

struct fat_header//类似FAT32实现,但是事实上它包括了Data之前的所有部分
{
    uint8_t  padding1[47];
    uint32_t BS_ExistFiles;
    uint32_t BS_UsedCluster;
    uint8_t  padding2[457];//补充空位
    uint8_t  padding3[512*7];
}__attribute__((packed));

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


int make_dir_entry(int type,struct dir_entry* buf);//type指示文件/目录,attr指示属性
int write_data(inode_t* node,int offset,char* buf,int size);
int read_data(inode_t* node,int offset,char* buf,int size);
int cluster_alloc();
