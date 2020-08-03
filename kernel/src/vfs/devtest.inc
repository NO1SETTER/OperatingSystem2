char buffer[1000]="aasfbkwjsbvibevjsfiowebjvebvdosibvwaorebigre";
{
    int fd=vfs->open("/dev/zero",O_RDONLY);
    vfs->read(fd,buffer,100);
    printf("zero:\n");
    for(int i=0;i<100;i++)
    printf("%d ",buffer[i]);
    vfs->close(fd);
}

{
    int fd=vfs->open("/dev/null",O_RDONLY);
    vfs->read(fd,buffer,100);
    printf("null:\n");
    for(int i=0;i<100;i++)
    printf("%d ",buffer[i]);
    vfs->close(fd);
}

{
    int fd=vfs->open("/dev/random",O_RDONLY);
    vfs->read(fd,buffer,100);
    printf("random:\n");
    for(int i=0;i<100;i++)
    printf("%d ",buffer[i]);
    vfs->close(fd);
}