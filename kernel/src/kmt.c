#include<common.h>
#define STACK_SIZE 4096
#define current currents[_cpu()]
#define _DEBUG
extern sem_t empty;
extern sem_t fill;
extern void producer(void *arg);
extern void consumer(void *arg);
_Context* schedule(_Event ev,_Context* c);
_Context* cyield();

task_t* task_alloc(){ return (task_t*)kalloc_safe(sizeof(task_t));}
static void kmt_init()
{
  for(int i=0;i<_ncpu();i++)
    currents[i]=NULL;
  kmt->spin_init(&thread_ctrl_lock,"thread_ctrl_lock");
  irq_head=(struct irq*)kalloc_safe(sizeof(struct irq));
  os->on_irq(0,_EVENT_YIELD,schedule);
  os->on_irq(1,_EVENT_IRQ_TIMER,cyield);

  #ifdef DEBUG_LOCAL
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
  sp_lock(&thread_ctrl_lock);
  task->id=thread_num;//id设置为当前进程数
  if(thread_num > 0)
  {
    all_thread[thread_num-1]->next=task;//设置链表形成环路
    task->next=all_thread[0];
  }
  _Area stack=(_Area){ task->stack,task->stack+STACK_SIZE};
  task->ctx=_kcontext(stack,entry,arg);//设置栈空间以及上下文
  //上下文存在于栈顶,task中的ctx指针指向该位置
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
    _intr_write(1);
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

_Context* schedule(_Event ev,_Context* c)//传入的c是current的最新上下文,要保存下来
{
      sp_lock(&thread_ctrl_lock);
      #ifdef _DEBUG
        printf("CPU#%d Schedule\n",_cpu());
      #endif
      if(!current)
          current=all_thread[0];//暂时的
      else
        {
          current->ctx=c;
          if(current->status==T_RUNNING)
            current->status=T_READY;//此时current也属于可被调度的线程,设置READY
        }
      task_t* rec=current;
      int reschedule=0;
      do{
        current=current->next;
        if(rec==current)//转了一轮都没找到
          reschedule=1;
        if(reschedule&&current->status==T_READY)//由于指定队列内的都被阻塞,允许调度指定队列外的线程
         break;
      }while((current->id)%_ncpu()!=_cpu()||current->status!=T_READY);
      //理解:是某个CPU在调用schedule,这里不是在切换CPU,而是为该CPU找到合适的task
      assert(current);
      current->status=T_RUNNING;//被选中的线程设置RUNNING
      printf("CPU#%d Schedule to %s\n",_cpu(),current->name);
      sp_unlock(&thread_ctrl_lock);
      return current->ctx;
}

_Context* cyield(_Event ev,_Context* c)
{
  #ifdef _DEBUG
    printf("Yield\n");
  #endif
  _yield();
  return NULL;
}

