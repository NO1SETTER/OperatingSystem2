#include <common.h>
#define current currents[_cpu()]

int thread_num=0;
int active_num=0;
spinlock_t thread_ctrl_lock;

static void os_init() {
  pmm->init();
  kmt->init(); // 模块先初始化
  #ifdef DEV_ENBALE
    dev->init();
  #endif
}

extern void check_allocblock(void *ptr);
extern void check_freeblock();
extern void print_FreeBlock();
extern void print_AllocatedBlock();

static void os_run() {
  for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    _putc(*s == '*' ? '0' + _cpu() : *s);
  }
  _intr_write(1);
  while (1);
}

int sane_context(_Context* ctx)//主要通过检查寄存器的合法性判断context合法性
{ 
  #ifdef __x86_64__
    if(ctx->cs!=8) return 1;
  #else
    if(ctx->ds!=16) return 1;
    if(ctx->cs!=8) return 1;
  #endif
  printf("Vaild Context\n");
  return 0;
}

static _Context *os_trap(_Event ev,_Context *context)//对应_am_irq_handle + do_event
{
  _intr_write(0);
  //printf("CPU#%d ev.event=%d\n",_cpu(),ev.event);
  //printf("Task %s on CPU#%d:trap\n",current->name,_cpu());
  _Context *next = context;
  struct irq *ptr=irq_head->next;
  while(ptr)
  {
    if (ptr->event == _EVENT_NULL || ptr->event == ev.event) {
      _Context *r = ptr->handler(ev, context);
      //panic_on(r && next, "returning multiple contexts");
      if (r) next = r;
    }
    ptr=ptr->next;
  }
  panic_on(!next, "returning NULL context");
  panic_on(sane_context(next), "returning to invalid context");
  if(ev.event==_EVENT_IRQ_TIMER)
    printf("Time interrupt:Task %s on CPU#%d:before ret\n",current->name,_cpu());
  else if(ev.event==_EVENT_YIELD)
    printf("Yield:Task %s on CPU#%d:before ret\n",current->name,_cpu());
  return next;
}


static void on_irq (int seq,int event,handler_t handler)//原本是_cte_init中的一部分
{
  struct irq* new_irq=(struct irq* )kalloc_safe(sizeof(struct irq));
  new_irq->seq=seq;
  new_irq->event=event;
  new_irq->handler=handler;
  struct irq* ptr=irq_head;
  while(ptr)
  {
    if(ptr->seq<seq)
    {
      if(ptr->next==NULL)
      { ptr->next=new_irq;
        break;
      }
      if((ptr->next)->seq>seq)
      { new_irq->next=ptr->next;
        ptr->next=new_irq;
        break;
      }
    }
    ptr=ptr->next;
  }
  return;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
  .on_irq = on_irq,
};










