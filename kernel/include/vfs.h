#include<devices.h>
#include<user.h>
#pragma once
#define sz(x) sizeof(struct x)
int min(int a,int b);
int max(int a,int b);
//以上是mkfs中的实现
/*
架构

     |
     | ufs
     |
vfs->| devfs
     |
     | procfs
     |
vfs和ufs/devfs/procfs具有相同的API,对于某个文件,我们用vfsAPI进行操作，事实上是确定它所属的文件系统后
转化成用该文件的API进行操作
*/
typedef struct filesystem filesystem_t;
typedef struct fsops fsops_t;
typedef struct inode inode_t;
typedef struct indops indops_t;

struct filesystem
{
fsops_t* ops;
device_t* dev;
};

struct fsops
{
  int (*init)();
  int (*write)(int fd, void *buf, int count);
  int (*read)(int fd, void *buf, int count);
  int (*close)(int fd);
  int (*open)(const char *pathname, int flags);
  int (*lseek)(int fd, int offset, int whence);

  int (*link)(const char *oldpath, const char *newpath);
  int (*unlink)(const char *pathname);
  int (*fstat)(int fd, struct ufs_stat *buf);

  int (*mkdir)(const char *pathname);
};

//open时,建立一个inode节点,此后关于文件信息的修改只在inode上完成，只在close时把信息回写到磁盘中
struct inode
{
  int node;//node的计数,根据node可以直接确定entry的偏移量为Entry(node)
  int refct;//引用计数
  int offset;//指针偏移

  int link_id;//对于没有链接到其他文件的文件linkid为-1
  filesystem_t* fs;//所属于的文件系统
  indops_t* ops;//操作
  sem_t sem;//信号量控制互斥

  int valid;//是否有效,一旦被加载,该inode保持有效
  int type;//文件或目录
  int size;//文件大小
};
/*逻辑:每个文件都对应一个节点,打开某个文件时先在file_table中查找,如果有添加新的ref项即可,如没有创建新的inode
加入file_table,当文件关闭时不释放,只设置taskid即可*/

struct indops
{
};

//vfs
#define nr_mnt 100
#define nr_file 1000
#define nr_ref 1000

typedef struct mountitem
{
char path[100];
filesystem_t *fs;
int valid;
}mount_t;
mount_t mount_table[100];

inode_t file_table[2000];
extern int exist_files;

typedef struct refitem
{
  int fd;
  int flags;
  int id;
  int thread_id;
  int valid;
}ref_t;
ref_t ref_table[1000];//设定:fd即为下标

int alloc_inode();//分配一个持久存在的inode唯一始终指向某一个文件
int alloc_fd();//分配一个最小未用fd,也就是我们想实现的alloc_ref_id

filesystem_t* ufs;
filesystem_t* procfs;
filesystem_t* devfs;

filesystem_t* find_fs(const char* path);