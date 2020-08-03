#define rsize 32
#define MAX_PATH_LEN 256

static void traverse(const char *root) {
  char *buf = pmm->alloc(rsize); // asserts success
  struct ufs_stat s;

  
  int fd = vfs->open(strcmp(root, "") == 0 ? "/" : root, O_RDONLY), nread;
  if (fd < 0) goto release;

  vfs->fstat(fd, &s);
  if (s.type == T_DIR) {
    while ( (nread = vfs->read(fd, buf, rsize)) > 0) {
      for (int offset = 0;
           offset +  sizeof(struct ufs_dirent) <= nread;
           offset += sizeof(struct ufs_dirent)) {
        struct ufs_dirent *d = (struct ufs_dirent *)(buf + offset);
        if (d->name[0] != '.') { // 小彩蛋：你这下知道为什么
                                 // Linux 以 “.” 开头的文件是隐藏文件了吧
          char *fname = pmm->alloc(MAX_PATH_LEN); // assert success
          sprintf(fname, "%s/%s", root, d->name);
          printf("fname=%s\n",fname);
          traverse(fname);
          pmm->free(fname);
        }
      }
    }
  }

release:
  if (fd >= 0) vfs->close(fd);
  pmm->free(buf);
}