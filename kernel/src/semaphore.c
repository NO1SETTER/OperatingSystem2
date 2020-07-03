#include<common.h>
#define P kmt->sem_wait
#define V kmt->sem_signal
#define current currents[_cpu()]

#ifdef DEBUG_LOCAL
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
  printf("Active:");
  for(int i=0;i<active_num;i++)
  printf("%s ",all_thread[active_thread[i]]->name);
  printf("\n");
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
  sp_lock(&sem->lock);//sem->lock用于控制一切对sem的修改
  sp_lock(&thread_ctrl_lock);
  sem->val--;
  #ifdef _DEBUG
  printf("Task %s on CPU#%d\n",current->name,_cpu());
  printf("wait:%s->val = %d\n",sem->name,sem->val);
  #endif
  if(sem->val<0) 
  {
    current->status=T_WAITING;
    if(sem->wnum==0)
    {
      sem->waiter[sem->wnum++]=current->id;
      #ifdef _DEBUG
      printf("%s blocked\n",current->name);
      #endif
    }
    else
    {
      int judge=1;
      for(int i=0;i<sem->wnum;i++)
      {
        if(sem->waiter[i]==current->id) judge=0;
      }
      if(judge) {
        sem->waiter[sem->wnum++]=current->id;
        #ifdef _DEBUG
        printf("%s blocked\n",current->name);
        #endif
        }
    }
    int pos=-1;
    for(int i=0;i<active_num;i++)
    {
      if(active_thread[i]==current->id)
      {
        pos=i;break;
      }
    }
    assert(pos!=-1);
    for(int i=pos;i<active_num-1;i++)
    {
      active_thread[i]=active_thread[i+1];
    }
    active_num=active_num-1;

    sp_unlock(&thread_ctrl_lock);
    sp_unlock(&sem->lock);
         print_task();
    #ifdef _DEBUG
    #endif
    _intr_write(1);
    _yield();
    return;
  }
  sp_unlock(&thread_ctrl_lock);
  sp_unlock(&sem->lock);
  _intr_write(1);
}

void sem_signal(sem_t *sem)
{
  sp_lock(&sem->lock);
  sp_lock(&thread_ctrl_lock);
  sem->val++;
  #ifdef _DEBUG
  printf("Task %s on CPU#%d\n",current->name,_cpu());
  printf("signal:%s->val = %d\n",sem->name,sem->val);
  #endif
    if(sem->wnum)
    {
      int no=rand()%sem->wnum;
      active_thread[active_num++]=sem->waiter[no];
      all_thread[sem->waiter[no]]->status=T_READY;//刚恢复活跃的线程一定尚未被调度
      #ifdef _DEBUG
      printf("%s activated\n",all_thread[sem->waiter[no]]->name);
      #endif
      for(int i=no;i<sem->wnum-1;i++)
      sem->waiter[i]=sem->waiter[i+1];
      sem->wnum--;
    }
  sp_unlock(&thread_ctrl_lock);
  sp_unlock(&sem->lock);
      print_task();
  #ifdef _DEBUG
  #endif
  _intr_write(1);
}
