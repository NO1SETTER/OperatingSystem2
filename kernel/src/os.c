#include <common.h>
//每個線程的鎖用與保護該線程的修改
int thread_num=0;
spinlock_t thread_ctrl_lock;

extern void tty_reader(void *arg);
extern void vfs_test();
void error_dfs(int k)
{
  error_dfs(k+1);
}

static void os_init() {
  pmm->init();
  kmt->init(); // 模块先初始化
  #ifdef DEV_ENABLE
    dev->init();
    //kmt->create(task_alloc(), "tty_reader", tty_reader, "tty1");
    //kmt->create(task_alloc(), "tty_reader", tty_reader, "tty2");
  #endif
  vfs->init();
  #ifdef VFS_DEBUG
    kmt->create(task_alloc(), "vfs_test"  ,  vfs_test ,  NULL );
  #endif
  error_dfs(0);  
}

extern void check_allocblock(void *ptr);
extern void check_freeblock();
extern void print_FreeBlock();
extern void print_AllocatedBlock();

static void os_run() {
  /*for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    _putc(*s == '*' ? '0' + _cpu() : *s);
  }*/
  _intr_write(1);
  while (1);
}


int sane_context(_Context* ctx)//主要通过检查寄存器的合法性判断context合法性
{ 
  /*#ifdef __x86_64__
    if(ctx->cs!=8) return 1;
  #else
    if(ctx->ds!=16) return 1;
    if(ctx->cs!=8) return 1;
  #endif
  #ifdef _DEBUG
    printf("Vaild Context\n");  
  #endif*/
  return 0;
}

void set_trap(task_t* t)
{
  if(trap_task==t)//由于上次自陷调度原线程出现的情况，这种情况下简单置1即可
  {
    kmt->spin_lock(&t->lk);
    t->is_trap=1;
    kmt->spin_unlock(&t->lk);
    return;
  }
  if(trap_task)
  {
    kmt->spin_lock(&trap_task->lk);
    trap_task->is_trap=0;
    #ifdef _DEBUG
      printf("%s set free from trap with status:%d\n",trap_task->name,trap_task->status);
    #endif
    kmt->spin_unlock(&trap_task->lk);
  }
    kmt->spin_lock(&t->lk);
    t->is_trap=1;
    trap_task=t;
    #ifdef _DEBUG
      printf("%s set trapped with status:%d\n",trap_task->name,trap_task->status);
    #endif
    kmt->spin_unlock(&t->lk);
}

void print_thread()
{
  printf("\nReady and nonblock threads:");
  for(int i=0;i<thread_num;i++)
  {
    if(all_thread[i]->status==T_READY&&all_thread[i]->is_trap==0)
      printf(" %s",all_thread[i]->name);
  }
  printf("\n");
}

static _Context *os_trap(_Event ev,_Context *context)//对应_am_irq_handle + do_event
{//整个过程中current栈不能被其他处理器修改!!!
   #ifdef _DEBUG
    printf("Task %s on CPU#%d trap with event %d\n",cur->name,_cpu(),ev.event);
  #endif
  set_trap(cur);
  #ifdef _DEBUG
    print_thread();
  #endif
  _Context *next = NULL;
  struct irq *ptr=irq_head;
  while(ptr)
  {
    if (ptr->event == _EVENT_NULL || ptr->event == ev.event) {
      _Context *r = ptr->handler(ev, context);
      if (r) next = r;
    }
    ptr=ptr->next;
  }
  //panic_on(!next, "returning NULL context");
  //panic_on(sane_context(next), "returning to invalid context");
  #ifdef _DEBUG
    printf("Task %s on CPU#%d is about to return from event %d\n",current->name,_cpu(),ev.event);
  #endif

  return next;
}

static void on_irq (int seq,int event,handler_t handler)//原本是_cte_init中的一部分
{
  struct irq* new_irq=(struct irq* )kalloc_safe(sizeof(struct irq));
  new_irq->seq=seq;
  new_irq->event=event;
  new_irq->handler=handler;
  if(irq_head==NULL)
    irq_head=new_irq;
  else
  {
    struct irq* ptr=irq_head;
    while(1)
    {
      if(ptr->next==NULL) break;
      if(ptr->seq<=new_irq->seq &&ptr->next->seq >= new_irq->seq) break;
      if(ptr->seq>=new_irq->seq) break;
      ptr=ptr->next;
    }
    if(ptr->next==NULL) ptr->next=new_irq;
    else if(ptr->seq<=new_irq->seq&&ptr->next->seq >= new_irq->seq)
    {
      new_irq->next=ptr->next;
      ptr->next=new_irq;
    }
    else if(ptr->seq>=new_irq->seq)
    {
      assert(ptr==irq_head);
      new_irq->next=ptr;
      irq_head=new_irq;
    }
  }
  return;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
  .on_irq = on_irq,
};










