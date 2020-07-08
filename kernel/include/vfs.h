#include<common.h>
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
char root[20];//以根目录路径作为root
fsops_t* ops;
};


struct fsops
{
  void (*init)();
  int (*write)(int fd, void *buf, int count);
  int (*read)(int fd, void *buf, int count);
  int (*close)(int fd);
  int (*open)(const char *pathname, int flags);
  int (*lseek)(int fd, int offset, int whence);
  int (*link)(const char *oldpath, const char *newpath);
  int (*unlink)(const char *pathname);
  int (*fstat)(int fd, struct ufs_stat *buf);
  int (*mkdir)(const char *pathname);
  int (*chdir)(const char *path);
  int (*dup)(int fd);
};

struct inode
{
char path[100];//路径
filesystem_t* fs;//所属于的文件系统
indops_t* ops;//操作
int id;//文件的编号,在inode分配内存时确定,作为一个inode的本征
int type;//文件或目录或ref
int offset;//偏移量
int size;
sem_t sem;//信号量控制互斥
};
/*逻辑:每个文件都对应一个节点,打开某个文件时先在file_table中查找,如没有创建新的inode
加入file_table,当文件关闭时不释放,只设置taskid即可*/

struct indops
{

};

struct ref
{

};

filesystem_t* ufs;
filesystem_t* profs;
filesystem_t* devfs;