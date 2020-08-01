#include<devices.h> //这里包含三个虚拟文件的抽象
#include<klib.h>

static int zero_init(device_t *dev)
{   return 0;}

static ssize_t zero_read(device_t *dev, off_t offset, void *buf, size_t count)
{
    assert(0);
    for(int i=0;i<count;i++)
      *(char *)(buf+i)='\0';
    return count;
}

static ssize_t zero_write(device_t *dev, off_t offset, const void *buf, size_t count)
{
    return -1;
}

static int null_init(device_t *dev)
{   return 0;}

static ssize_t null_read(device_t *dev, off_t offset, void *buf, size_t count)
{
    assert(0);
    return -1;
}
static ssize_t null_write(device_t *dev, off_t offset, const void *buf, size_t count)
{
    return 0;
}

static int random_init(device_t *dev)
{   return 0;}

static ssize_t random_read(device_t *dev, off_t offset, void *buf, size_t count)
{
    assert(0);
    for(int i=0;i<count;i++)
      *(char *)(buf+i)=rand()%256;
    return count;
}

static ssize_t random_write(device_t *dev, off_t offset, const void *buf, size_t count)
{
    return -1;
}


devops_t zero_ops = {
    .init  = zero_init,
    .read  = zero_read,
    .write = zero_write,
};


devops_t null_ops = {
    .init  = null_init,
    .read  = null_read,
    .write = null_write,
};

devops_t random_ops = {
    .init  = random_init,
    .read  = random_read,
    .write = random_write,
};