#include<common.h>
#include<devices.h>
#include<user.h>
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
int entry;//entry偏移量
int id;//文件的编号,它事实上就是文件在file_table中的下标
int refct;//引用计数
int offset;//指针偏移
filesystem_t* fs;//所属于的文件系统
indops_t* ops;//操作
sem_t sem;//信号量控制互斥

int valid;//是否有效
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
#define nr_link 1000
#define nr_ref 1000

typedef struct mounttable
{
char path[100];
filesystem_t *fs;
int valid;
}mount_table[100];


inode_t file_table[1000];//设定:id即为下标
//linkitem事实上有两种,本体项目和指代项目，前者id为-1,refct>=0，后者refct为-1,id>=0;
/*访问时本体的valid只代表能否通过本体的路径访问该文件entity:
本体valid --> 成功
指代valid+本体refct>0 --> 成功
*/
typedef struct linkitem
{
  char* pathname;
  int id;//它指向的link_table下标
  int refct;
  int valid;
}link_t;
link_t link_table[1000];

typedef struct refitem
{
  int fd;
  int flags;
  int id;
  int thread_id;
  int valid;
}ref_t;
ref_t ref_table[1000];//设定:fd即为下标

int alloc_file_id();
int alloc_link_id();
int alloc_fd();//分配一个最小未用fd,也就是我们想实现的alloc_ref_id

filesystem_t* ufs;
filesystem_t* profs;
filesystem_t* devfs;