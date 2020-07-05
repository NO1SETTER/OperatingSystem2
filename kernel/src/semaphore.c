#include<common.h>
#define P kmt->sem_wait
#define V kmt->sem_signal
#define current currents[_cpu()]
#ifdef _DEBUG_LOCAL
    sem_t empty;
    sem_t fill;
    void producer(void *arg)
    {
        while(1)
        {
        P(&empty);
        printf("(");
        #ifdef _DEBUG
          printf("from %s\n",current->name);
        #endif
        V(&fill);
        }
    }

    void consumer(void *arg)
    {
        while(1)
        {
        P(&fill);
        printf(")");
        #ifdef _DEBUG
          printf("from %s\n",current->name);
        #endif
        V(&empty);
        }
    }
#endif

void print_task()
{
#ifdef _DEBUG
  printf("Active:");
  for(int i=0;i<active_num;i++)
  printf("%s ",all_thread[active_thread[i]]->name);
  printf("\n");
#endif
}

void sem_init(sem_t *sem, const char *name, int value)
{
  char lock_name[128];
  sprintf(lock_name,"%s_lock",name);
  kmt->spin_init(&sem->lock,lock_name);
  strcpy(sem->name,name);
  sem->val=value;
  sem->wnum=0;
}

void sem_wait(sem_t *sem)
{
  assert(getip()<0x100000||getip()>0x110000);
  sp_lock(&sem->lock);//sem->lock用于控制一切对sem的修改
  sp_lock(&thread_ctrl_lock);  
  assert(getip()<0x100000||getip()>0x110000);
  #ifdef _DEBUG
  printf("Task %s running on CPU#%d\n",current->name,_cpu());
  printf("wait:%s->val = %d\n",sem->name,sem->val);
  #endif
  assert(getip()<0x100000||getip()>0x110000);
  if(--sem->val<0) 
  {
    sp_lock(&current->lk);
      current->status=T_WAITING;
      sem->waiter[sem->wnum++]=current->id;
      assert(getip()<0x100000||getip()>0x110000);
      #ifdef _DEBUG
      printf("%s blocked\n",current->name);
      #endif
      int pos=-1;   
      for(int i=0;i<active_num;i++)
      if(active_thread[i]==current->id)
        { pos=i;break;}
        assert(getip()<0x100000||getip()>0x110000);
    sp_unlock(&current->lk);
assert(getip()<0x100000||getip()>0x110000);
    assert(pos!=-1);
    for(int i=pos;i<active_num-1;i++)
      active_thread[i]=active_thread[i+1];
    active_num=active_num-1;
assert(getip()<0x100000||getip()>0x110000);
    sp_unlock(&thread_ctrl_lock);
    sp_unlock(&sem->lock);
    assert(getip()<0x100000||getip()>0x110000);
    print_task();
    assert(getip()<0x100000||getip()>0x110000);
    _yield();
    return;
  }
  assert(getip()<0x100000||getip()>0x110000);
  sp_unlock(&thread_ctrl_lock);
  sp_unlock(&sem->lock);
}

void sem_signal(sem_t *sem)
{
  sp_lock(&sem->lock);
  sp_lock(&thread_ctrl_lock);
  sem->val=sem->val+1;
  assert(getip()<0x100000||getip()>0x110000);
  #ifdef _DEBUG
  printf("Task %s running on CPU#%d\n",current->name,_cpu());
  printf("signal:%s->val = %d\n",sem->name,sem->val);
  #endif
  assert(getip()<0x100000||getip()>0x110000);
    if(sem->wnum)
    {
      int no=rand()%sem->wnum;
      assert(getip()<0x100000||getip()>0x110000);
      active_thread[active_num++]=sem->waiter[no];
      sp_lock(&all_thread[sem->waiter[no]]->lk);
      assert(getip()<0x100000||getip()>0x110000);
      all_thread[sem->waiter[no]]->status=T_READY;//刚恢复活跃的线程一定尚未被调度
      sp_unlock(&all_thread[sem->waiter[no]]->lk);
      assert(getip()<0x100000||getip()>0x110000);
      #ifdef _DEBUG
        printf("%s activated\n",all_thread[sem->waiter[no]]->name);
      #endif
      for(int i=no;i<sem->wnum-1;i++)
      sem->waiter[i]=sem->waiter[i+1];
      sem->wnum=sem->wnum-1;
      assert(getip()<0x100000||getip()>0x110000);
    }
    assert(getip()<0x100000||getip()>0x110000);
  sp_unlock(&thread_ctrl_lock);
  sp_unlock(&sem->lock);
  print_task();
  assert(getip()<0x100000||getip()>0x110000);
}
