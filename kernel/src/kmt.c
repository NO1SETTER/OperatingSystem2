#include<common.h>
extern sem_t empty;
extern sem_t fill;
extern void producer(void *arg);
extern void consumer(void *arg);
_Context* kmt_context_save(_Event ev,_Context* c);
_Context* kmt_schedule(_Event ev,_Context* c);


task_t* task_alloc(){ return (task_t*)kalloc_safe(sizeof(task_t));}
static void kmt_init()
{
  for(int i=0;i<_ncpu();i++)
  {
    task_t *new_task=(task_t*)kalloc_safe(sizeof(task_t));
    new_task->id=-1;
    new_task->status=T_RUNNING;
    new_task->is_trap=0;
    char name[15];
    sprintf(name,"mainthread_%d",_cpu());
    strcpy(new_task->name,name);
    currents[i]=new_task;
    intrdepths[i]=0;
    trap_tasks[i]=NULL;
  }//currents全部設置爲空
  kmt->spin_init(&thread_ctrl_lock,"thread_ctrl_lock");//初始化鎖
  irq_head=NULL;
  os->on_irq(INT_MIN,_EVENT_NULL,kmt_context_save);
  os->on_irq(INT_MAX,_EVENT_NULL,kmt_schedule);

  #ifdef _DEBUG_LOCAL
    kmt->sem_init(&empty, "empty", 5);  // 缓冲区大小为 5
    kmt->sem_init(&fill,  "fill",  0);
    
    char p[4][3]={"p1","p2","p3","p4"};
    char c[5][3]={"c1","c2","c3","c4","c5"};
    for(int i=0;i<4;i++)
      kmt->create(task_alloc(), p[i], producer, NULL);
    for(int i=0;i<5;i++)
      kmt->create(task_alloc(), c[i], consumer, NULL);
  #endif
}

//task提前分配好,那么我们用一个指针数组管理所有这些分配好的task
//_Area{*start,*end;},

static int kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg) {
  strcpy(task->name,name);//名字
  task->status=T_READY;//状态
  _Area stack=(_Area){ task->stack,task->stack+STACK_SIZE};
  task->ctx=_kcontext(stack,entry,arg);//设置栈空间以及上下文
  //上下文存在于栈顶,task中的ctx指针指向该位置
  sp_lockinit(&task->lk,"task_lock");
  sp_lock(&thread_ctrl_lock);
    task->id=thread_num;//id设置为当前进程数
    if(thread_num > 0)
    {
      all_thread[thread_num-1]->next=task;//设置链表形成环路
      task->next=all_thread[0];
    }
    all_thread[thread_num++]=task;//添加到所有线程中
    active_thread[active_num++]=task->id;//添加到活跃线程中
  sp_unlock(&thread_ctrl_lock);
  #ifdef _DEBUG
    printf(" task %d:%s created:%p\n",task->id,task->name,(void *)task);
  #endif
  return 0;
}

static void kmt_teardown(task_t *t)
{
  sp_lock(&thread_ctrl_lock);
  int pos=-1;
  int id;
  for(int i=0;i<active_num;i++)
  {
    if (all_thread[active_thread[i]]==t) {
      pos = i;
      id=active_thread[i];
      break;
     }
  }
  if(pos==-1)
  {
    sp_unlock(&thread_ctrl_lock);
    return; 
  }
  for(int i=pos;i<active_num-1;i++)
  active_thread[i]=active_thread[i+1];
  all_thread[id]->status=T_DEAD;
  sp_unlock(&thread_ctrl_lock);
  kfree_safe(t->stack);
}

MODULE_DEF(kmt) = {
  .init=kmt_init,
  .spin_init=sp_lockinit,
  .spin_lock=(void*)sp_lock,
  .spin_lock=(void*)sp_unlock,
  .create=kmt_create,
  .teardown=kmt_teardown,
  .sem_init=sem_init,
  .sem_wait=sem_wait,
  .sem_signal=sem_signal,
};

_Context* kmt_context_save(_Event ev,_Context* c)
{
  sp_lock(&current->lk);
    current->ctx=c;
    #ifdef _DEBUG
      printf("CPU#%d save context for %s\n",_cpu(),current->name);
    #endif
  sp_unlock(&current->lk);
  return NULL;
}



_Context* kmt_schedule(_Event ev,_Context* c)//传入的c是current的最新上下文,要保存下来
{
      sp_lock(&thread_ctrl_lock);
      #ifdef _DEBUG
        printf("CPU#%d Schedule\n",_cpu());
      #endif
      if(current->id==-1)
        current=all_thread[0];
      
      do{
        current=current->next;
        printf("hahaha\n");
      }while((current->id%_ncpu()!=_cpu())&&current->status!=T_READY);
      #ifdef _DEBUG
        printf("CPU#%d Scheduled to %s\n",_cpu(),current->name);
      #endif
      return current->ctx;
} 
      

