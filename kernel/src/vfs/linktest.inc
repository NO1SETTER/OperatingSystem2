printf("linktest start\n");
vfs->chdir("/jw3f4nsa23antht8");
vfs->link("a2.txt","/hsiysh7mcz2xm3/dkipsvdc40j5fjw/link1.txt");
char buff[100]="linkcontent-zyx";
{
int fd = vfs->open("a2.txt",O_RDWR);
vfs->write(fd,buff,20);
vfs->close(fd);
}
char buff2[100];
vfs->chdir("../hsiysh7mcz2xm3/dkipsvdc40j5fjw");
{
int fd = vfs->open("link1.txt",O_RDWR);
vfs->read(fd,buff2,20);
vfs->close(fd);
}
printf("read content:%s\n",buff2);

