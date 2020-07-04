#include<common.h>

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
