#include <klib.h>
#include <amdev.h>

static unsigned long int next = 1;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

void *malloc(size_t size)
{return NULL;}

void free(void *ptr)
{return;}

intptr_t geteip_func()
{
#ifdef __x86_64
        asm volatile(
          "movq 8(%%rsp),%%rax\n"
          "pop %%rbp\n"
          "ret\n"
          ::);
#else
        asm volatile(
          "movl 4(%%rsp),%%rax\n"
          "pop %%ebp\n"
          "ret\n"
          ""
          ::);
#endif
return 0;
}

intptr_t getip()
{
  void (*func)(void *)=(void (*)(void *))geteip_func;
  #ifdef __x86_64
  asm volatile(
              "call *%0\n"
              "pop %%rbp\n"
              "ret\n"
              ::"c"(func)  );
  #else
  asm volatile(
              "call *%0\n"
              "pop %%ebp\n"
              "ret\n"
              ::"c"(func)  );
  #endif
  return 0;
}
