#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>
#include <amdev.h>
#define _DEBUG_LOCAL //控制是否進行測試
#define _DEBUG       //控制是否輸出本地測試的調試信息
//#define DEV_ENABLE
#define STACK_SIZE 4096
#define INT_MIN -2147483648
#define INT_MAX 2147483647
#define MAX_CPU 8
enum t_status {
  T_READY = 1, // 活跃状态,且没有在任何一个CPU上运行 
  T_RUNNING,   // 活跃状态,但正在某个CPU上运行
  T_WAITING,   // 阻塞状态,在sem上等待
  T_DEAD,      // 
};

struct spinlock 
{
  char name[20];//锁名
  int lockid;//锁的序号
  intptr_t locked;//锁控制
  int holder;//锁的持有者
};

void sp_lockinit(spinlock_t* lk,const char *name);
void sp_lock(spinlock_t* lk);
void sp_unlock(spinlock_t *lk);

struct task
{
  struct
  {
    char name[15];
    int id;
    enum t_status status;
    int is_trap;
    spinlock_t lk;//加鎖保護訪問
    struct task* next;//指向all_thread[id+1]
    _Context *ctx;//貌似只要保证它指向栈顶就ok了，上面的可以不管分配在哪里
  };
  uint8_t stack[4096];
};//管理一个线程的信息
task_t* currents[MAX_CPU];
task_t* all_thread[105];
int active_thread[105];
//只记录线程的id,id对应它在all_thread中的位置
//状态均为T_READY或T_RUNNING

extern int thread_num;
extern int active_num;
extern spinlock_t thread_ctrl_lock;//管理控制这三个链表的锁


struct semaphore
{
spinlock_t lock;
char name[15];
int val;
int waiter[105];
int wnum;
};
void sem_init(sem_t *sem, const char *name, int value);
void sem_wait(sem_t *sem);
void sem_signal(sem_t *sem);

void *kalloc_safe(size_t size);
void kfree_safe(void *ptr);

//中断事件
struct irq
{
  int seq;
  int event;
  handler_t handler;
  struct irq* next;
};
struct irq* irq_head;

task_t* currents[MAX_CPU];
#define current currents[_cpu()]
int intrdepths[MAX_CPU];
#define intrdepth intrdepths[_cpu()]
int intenas[MAX_CPU];
#define intena intenas[_cpu()]
task_t* trap_tasks[MAX_CPU];//每个处理器的上一个trap线程
#define trap_task trap_tasks[_cpu()]
void push_cli();
void pop_cli();

void set_trapped(task_t* t);