#include<common.h>
#define P kmt->sem_wait
#define V kmt->sem_signal
#define _DEBUG_
#ifdef _DEBUG_LOCAL
    sem_t empty;
    sem_t fill;
    void producer(void *arg)
    {
        while(1)
        {
        P(&empty);
        printf("(");
        #ifdef _DEBUG_
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
        #ifdef _DEBUG_
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
  sp_lock(&sem->lock);//sem->lock用于控制一切对sem的修改
  #ifdef _DEBUG
  printf("Task %s running on CPU#%d\n",cur->name,_cpu());
  printf("wait:%s->val = %d\n",sem->name,sem->val);
  #endif
  if(--sem->val<0) 
  {
      sp_lock(&cur->lk);
        cur->is_trap=1;
        cur->status=T_WAITING;
        sem->waiter[sem->wnum++]=cur->id;
        #ifdef _DEBUG
          printf("%s blocked\n",cur->name);
        #endif
      sp_unlock(&cur->lk);

    sp_unlock(&sem->lock);
    _yield();
    return;
  }
  sp_unlock(&sem->lock);
}

void sem_signal(sem_t *sem)
{
  sp_lock(&sem->lock);
  sem->val=sem->val+1;
  #ifdef _DEBUG
  printf("Task %s running on CPU#%d\n",cur->name,_cpu());
  printf("signal:%s->val = %d\n",sem->name,sem->val);
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

      sp_lock(&all_thread[sem->waiter[no]]->lk);
      all_thread[sem->waiter[no]]->status=T_READY;//刚恢复活跃的线程一定尚未被调度
      sp_unlock(&all_thread[sem->waiter[no]]->lk);
      #ifdef _DEBUG
        printf("%s activated\n",all_thread[sem->waiter[no]]->name);
      #endif
      for(int i=no;i<sem->wnum-1;i++)
      sem->waiter[i]=sem->waiter[i+1];
      sem->wnum=sem->wnum-1;
    }
  sp_unlock(&sem->lock);
}