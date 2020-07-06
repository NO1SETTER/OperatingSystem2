#include<common.h>
#define FL_IF          0x00000200 

void push_cli()
{
  int readcli=_intr_read();
  _intr_write(0);
  if(intrdepth==0)
    intena=readcli&FL_IF;
  intrdepth=intrdepth+1;
}

void pop_cli()
{
  //int readcli=_intr_read();
  /*if(readcli&FL_IF)
    assert(0);
  if(--intrdepth<0)
    assert(0);*/
  --intrdepth;
  if(intrdepth==0&&intena)
    _intr_write(1);
}

void sp_lock(spinlock_t* lk)
{
  push_cli();
  while(_atomic_xchg(&lk->locked,1));
}

void sp_unlock(spinlock_t *lk)
{
  _atomic_xchg(&lk->locked,0);
  pop_cli();
}

void sp_lockinit(spinlock_t* lk,const char *name)
{
  strcpy(lk->name,name);
  lk->locked=0;
}
