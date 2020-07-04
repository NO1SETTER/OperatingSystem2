#include<common.h>
/*void _intr_write_safe(int x)
{
  assert(intrdepth>=0);
  if(x==0)
  {
    intrdepth=intrdepth+1;
    _intr_write(0);
    //printf("close intr\n");
  }
  else
  {
    intrdepth=intrdepth-1;
    if(intrdepth==0)  
    {_intr_write(1);
     //printf("open intr\n");
    }
  }
}*/

void sp_lock(spinlock_t* lk)
{
  _intr_write(0);
  while(_atomic_xchg(&lk->locked,1))
  {
    printf("acquring\n");
  }
}

void sp_unlock(spinlock_t *lk)
{
  _atomic_xchg(&lk->locked,0);
}

void sp_lockinit(spinlock_t* lk,const char *name)
{
  strcpy(lk->name,name);
  lk->locked=0;
}
