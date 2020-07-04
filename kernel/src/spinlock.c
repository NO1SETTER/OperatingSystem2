#include<common.h>

void _intr_write_safe(int x)
{
  assert(intrdepth[_cpu()]>=0);
  if(x==0)
  {
    intrdepth[_cpu()]=intrdepth[_cpu()]+1;
    _intr_write_safe(0);
  }
  else
  {
    intrdepth[_cpu()]=intrdepth[_cpu()]-1;
    if(intrdepth[_cpu()]==0)
      _intr_write_safe(1);
  }
}

void sp_lock(spinlock_t* lk)
{
  _intr_write(0);
  while(_atomic_xchg(&lk->locked,1));
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
