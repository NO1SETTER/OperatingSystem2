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
  #ifdef _DEBUG
    printf("Vaild Context\n");  
  #endif
  return 0;
}

static _Context *os_trap(_Event ev,_Context *context)//对应_am_irq_handle + do_event
{
  _intr_write(0);
  #ifdef _DEBUG
    printf("Task %s on CPU#%d trap with event %d\n",current->name,_cpu(),ev.event);
  #endif
  _Context *next = context;
  struct irq *ptr=irq_head;
  while(ptr)
  {
    if (ptr->event == _EVENT_NULL || ptr->event == ev.event) {
      _Context *r = ptr->handler(ev, context);
      if (r) next = r;
    }
    ptr=ptr->next;
  }
  panic_on(!next, "returning NULL context");
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
  while(ptr->next)
    ptr=ptr->next;
  ptr->next=new_irq;
  }
  #ifdef _DEBUG
   printf("on irq\n");
  #endif
  return;
}

MODULE_DEF(os) = {
  .init = os_init,
  .run  = os_run,
  .trap = os_trap,
  .on_irq = on_irq,
};










