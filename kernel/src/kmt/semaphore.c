#include<common.h>
#define P kmt->sem_wait
#define V kmt->sem_signal
#define PRINT_THREAD
#ifdef KMT_TEST
    sem_t empty;
    sem_t fill;
    void producer(void *arg)
    {
        while(1)
        {
        P(&empty);
        printf("(");
        #ifdef PRINT_THREAD
          printf("from %s\n",cur->name);
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
        #ifdef PRINT_THREAD
          printf("from %s\n",cur->name);
        #endif
        V(&empty);
        }
    }
#endif

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
  kmt->spin_lock(&sem->lock);//sem->lock用于控制一切对sem的修改
  #ifdef KMT_DEBUG
  printf("Task %s running on CPU#%d\n",cur->name,_cpu());
  printf("wait:%s->val = %d\n",sem->name,sem->val);
  #endif
  if(--sem->val<0) 
  {
      kmt->spin_lock(&cur->lk);
        cur->is_trap=1;
        cur->status=T_WAITING;
        cur->sem_ct=cur->sem_ct+1;//只有被阻塞的才需要增加sem_ct
        sem->waiter[sem->wnum++]=cur->id;
        #ifdef KMT
          printf("%s blocked\n",cur->name);
        #endif
      kmt->spin_unlock(&cur->lk);

    kmt->spin_unlock(&sem->lock);
    _yield();
    return;
  }
  kmt->spin_unlock(&sem->lock);
}

void sem_signal(sem_t *sem)
{
  kmt->spin_lock(&sem->lock);
  sem->val=sem->val+1;
  #ifdef KMT
  printf("Task %s running on CPU#%d\n",cur->name,_cpu());
  printf("signal: %s->val = %d\n",sem->name,sem->val);
  #endif
    if(sem->wnum)
    {
        int no=-1;
        int val=INT_MAX;
        for(int i=0;i<sem->wnum;i++)
        {
          if(all_thread[sem->waiter[i]]->ct<val)
          {
            val=all_thread[sem->waiter[i]]->ct;
            no=i;
          }
        }
        assert(no!=-1);

      task_t* tsk=all_thread[sem->waiter[no]];
      kmt->spin_lock(&tsk->lk);
        tsk->sem_ct=tsk->sem_ct-1;//在waiter中的task的sem_ct一定大于0,否则不会被加入到waiter中
        if(tsk->sem_ct==0)
          tsk->status=T_READY;//刚恢复活跃的线程一定尚未被调度
      kmt->spin_unlock(&tsk->lk);
      #ifdef KMT
        printf("%s activated\n",all_thread[sem->waiter[no]]->name);
      #endif
      for(int i=no;i<sem->wnum-1;i++)//无论该线程有没有被唤醒，它都不受此sem的限制,因此从waiter中移除
      sem->waiter[i]=sem->waiter[i+1];
      sem->wnum=sem->wnum-1;
    }
  kmt->spin_unlock(&sem->lock);
}
