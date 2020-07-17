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
typedef struct ref ref_t;

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
  int (*mkdir)(const char *pathname);
};

//open时,建立一个inode节点,此后关于文件信息的修改只在inode上完成，只在close时把信息回写到磁盘中
struct inode
{
int offset;//entry偏移量
int id;//文件的编号,它事实上就是文件在file_table中的下标
int refct;//引用计数
filesystem_t* fs;//所属于的文件系统
indops_t* ops;//操作
sem_t sem;//信号量控制互斥

int valid;//是否有效
int type;//文件或目录
int size;
};
/*逻辑:每个文件都对应一个节点,打开某个文件时先在file_table中查找,如没有创建新的inode
加入file_table,当文件关闭时不释放,只设置taskid即可*/

struct indops
{

};

//vfs
#define nr_mnt 100
#define nr_ref_thread 100 //认为的一个线程的文件数
#define nr_link
#define nr_ref 1000

typedef struct mounttable
{
char path[100];
filesystem_t *fs;
int valid;
}mount_table[100];


inode_t file_table[1000];//设定:id即为下标
int alloc_file_id();
/*把link产生的复用和dup/reopen产生的复用分开,它们都用id指向实体inode*/
typedef struct linkitem
{
  char* pathname;
  int id;
  int valid;
}link_t;
link_t link_table[1000];
int alloc_link_id();
/*设定不同线程产生的文件描述符不相同,且文件描述符是某一线程所有的
这种从属关系我们可以在线程中加以记录*/
typedef struct refitem
{
  int fd;
  int flags;
  int id;
  int thread_id;
  int valid;
}ref_t;
ref_t ref_table[1000];//设定:fd即为下标
int alloc_fd();//分配一个最小未用fd,也就是我们想实现的alloc_ref_id

filesystem_t* ufs;
filesystem_t* profs;
filesystem_t* devfs;