#include<devices.h>
#include<user.h>
#pragma once
#define sz(x) sizeof(struct x)
//#define DEBUG_
int min(int a,int b);
int max(int a,int b);
//以上是mkfs中的实现
/*
vfs和ufs/devfs/procfs具有相同的API,对于某个文件,我们用vfsAPI进行操作，事实上是确定它所属的文件系统后
转化成用该文件的API进行操作
*/
typedef struct filesystem filesystem_t;
typedef struct fsops fsops_t;
typedef struct inode inode_t;
typedef struct indops indops_t;

struct filesystem
{
char name[32];
fsops_t* ops;
device_t* dev;
};

struct fsops
{
  filesystem_t* (*init)();
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
struct inode//file_table里的都是ufs的文件
{
  int node;//node的计数,根据node可以直接确定entry的偏移量为Entry(node)
  int fnode;//父目录的inode
  int refct;//引用计数
  int offset;//指针偏移

  int link_id;//对于没有链接到其他文件的文件linkid为-1
  indops_t* ops;//操作
  sem_t sem;//信号量控制互斥

  int valid;//反复强调,这个inode代表的能不能通过此inode的代表的路径访问到磁盘中的本体
  int type;//文件或目录
  int size;//文件大小
  int cid;//第一块的id
};
/*逻辑:每个文件都对应一个节点,打开某个文件时先在file_table中查找,如果有添加新的ref项即可,如没有创建新的inode
加入file_table,当文件关闭时不释放,只设置taskid即可*/

struct indops
{
};

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
typedef struct refitem
{
  int fd;
  int flags;
  int id;
  int thread_id;
  filesystem_t* fs;//所属于的文件系统
  int valid;
}ref_t;
ref_t ref_table[1000];//设定:fd即为下标
/*ref_table为所有文件系统共用，id指向该文件系统的实体管理数组
file_table为ufs专有,proc_table为procfs专有*/

filesystem_t* ufs;
filesystem_t* procfs;
filesystem_t* devfs;

//alloc
int alloc_inode();//分配一个持久存在的inode唯一始终指向某一个文件
int alloc_proc_inode();
int alloc_fd();//分配一个最小未用fd,也就是我们想实现的alloc_ref_id
extern sem_t inode_lock;
extern sem_t proc_inode_lock;
extern sem_t fd_lock;

//tool_functions
void xxd(const char* str,int n);
filesystem_t* find_fs(const char* path);
int get_abs_path(const char *path,char* abs_path);
int get_name(const char* path,char* name);
int error_dfs(int x);
/*
对于互斥的保护,由于信号量嵌套起来有一些困难,我采用信号量和自旋锁混用嵌套的方式来实现
用信号量实现对文件的读写保护,用自旋锁实现对各种id分配的保护以及磁盘读写的保护
*/