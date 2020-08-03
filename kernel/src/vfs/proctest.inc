printf("proctest start!\n");
char buf[33];
char proc_name[100];
for(int i=0;i<_ncpu();i++)
{
sprintf(proc_name,"/proc/%d/name",i);
int fd=vfs->open(proc_name,O_RDONLY);
vfs->read(fd,buf,32);
printf("thread %d name:%s\n",i,buf);
vfs->close(fd);
}