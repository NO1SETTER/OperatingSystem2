#include<vfs.h>
#include<common.h>

struct sdir_entry* make_dir_entry(char* pathname,char* name,char *buf);
int write_data(struct sdir_entry* sdir,int offset,char* buf,int size);
int read_data(struct sdir_entry* sdir,int offset,char* buf,int size);
