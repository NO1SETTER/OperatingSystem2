#include <user.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#define MB_SIZE 1024*1024

int fd;
void* disk;
int IMG_SIZE;
int BASE_SIZE;

void parse_args(int argc,char *argv[])
{
assert(argc==4);
assert((fd = open(argv[2], O_RDWR)) > 0);

IMG_SIZE=atoi(argv[1])*MB_SIZE;
BASE_SIZE=(lseek(fd,0,SEEK_END)/MB_SIZE+1)*MB_SIZE;
assert(ftruncate(fd,BASE_SIZE+IMG_SIZE)==0);
assert((disk = mmap(NULL, IMG_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0)) != (void *)-1);

}

int main(int argc, char *argv[]){
  parse_args;
  munmap(disk, IMG_SIZE);
  close(fd);
}
