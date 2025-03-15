#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

// Fetch the uint64 at addr from the current process.
int
fetchaddr(uint64 addr, uint64 *ip)
{
  struct proc *p = myproc();
  if(addr >= p->sz || addr+sizeof(uint64) > p->sz) // both tests needed, in case of overflow
    return -1;
  if(copyin(p->pagetable, (char *)ip, addr, sizeof(*ip)) != 0)
    return -1;
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.
int
fetchstr(uint64 addr, char *buf, int max)
{
  struct proc *p = myproc();
  if(copyinstr(p->pagetable, buf, addr, max) < 0)
    return -1;
  return strlen(buf);
}

static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Fetch the nth 32-bit system call argument.
void
argint(int n, int *ip)
{
  *ip = argraw(n);
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
void
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
}

// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (including nul), -1 if error.
int
argstr(int n, char *buf, int max)
{
  uint64 addr;
  argaddr(n, &addr);
  return fetchstr(addr, buf, max);
}

// Prototypes for the functions that handle system calls.
extern uint64 sys_fork(void);
extern uint64 sys_exit(void);
extern uint64 sys_wait(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
extern uint64 sys_kill(void);
extern uint64 sys_exec(void);
extern uint64 sys_fstat(void);
extern uint64 sys_chdir(void);
extern uint64 sys_dup(void);
extern uint64 sys_getpid(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_sleep(void);
extern uint64 sys_uptime(void);
extern uint64 sys_open(void);
extern uint64 sys_write(void);
extern uint64 sys_mknod(void);
extern uint64 sys_unlink(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_close(void);
extern uint64 sys_trace(void);
extern uint64 sys_sysinfo(void);

// An array mapping syscall numbers from syscall.h
// to the function that handles the system call.
static uint64 (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
[SYS_pipe]    sys_pipe,
[SYS_read]    sys_read,
[SYS_kill]    sys_kill,
[SYS_exec]    sys_exec,
[SYS_fstat]   sys_fstat,
[SYS_chdir]   sys_chdir,
[SYS_dup]     sys_dup,
[SYS_getpid]  sys_getpid,
[SYS_sbrk]    sys_sbrk,
[SYS_sleep]   sys_sleep,
[SYS_uptime]  sys_uptime,
[SYS_open]    sys_open,
[SYS_write]   sys_write,
[SYS_mknod]   sys_mknod,
[SYS_unlink]  sys_unlink,
[SYS_link]    sys_link,
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
[SYS_sysinfo] sys_sysinfo,
[SYS_trace]   sys_trace,
};

char* syscallnames[] = {
  "",
  "fork", "exit", "wait", "pipe",
  "read", "kill", "exec", "fstat",
  "chdir", "dup", "getpid", "sbrk",
  "sleep", "uptime", "open", "write",
  "mknod", "unlink", "link", "mkdir",
  "close", "trace",
};

int syscall_argcount[] = {
  0,  // Không có syscall 0
  0,  // fork()
  1,  // exit(status)
  1,  // wait(&status)
  1,  // pipe(fd)
  3,  // read(fd, buf, count)
  2,  // kill(pid, sig)
  2,  // exec(path, argv)
  2,  // fstat(fd, stat)
  1,  // chdir(path)
  1,  // dup(fd)
  0,  // getpid()
  1,  // sbrk(size)
  1,  // sleep(ticks)
  0,  // uptime()
  2,  // open(path, mode)
  3,  // write(fd, buf, count)
  3,  // mknod(path, mode, dev)
  1,  // unlink(path)
  2,  // link(oldpath, newpath)
  1,  // mkdir(path)
  1,  // close(fd)
  1,  // trace(mask)
};



void syscall(void) {
  int num;
  struct proc *p = myproc();

  num = p->trapframe->a7;

  if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    uint64 args[6];
    args[0] = p->trapframe->a0;
    args[1] = p->trapframe->a1;
    args[2] = p->trapframe->a2;
    args[3] = p->trapframe->a3;
    args[4] = p->trapframe->a4;
    args[5] = p->trapframe->a5;

    char path[MAXPATH] = {0};
    char *argv_bufs[5] = {0}; 
    uint64 arg_ptr;
    
    if ((1 << num) & p->mask) { 
      if (num == SYS_exec) {
        if (copyinstr(p->pagetable, path, args[0], sizeof(path)) < 0)
          path[0] = '\0'; 

        for (int i = 0; i < 5; i++) {
          if (fetchaddr(args[1] + i * sizeof(uint64), &arg_ptr) < 0 || arg_ptr == 0)
            break;
          argv_bufs[i] = (char*)kalloc();
          if (argv_bufs[i]) {
            if (copyinstr(p->pagetable, argv_bufs[i], arg_ptr, PGSIZE) < 0)
              argv_bufs[i][0] = '\0';
          }
        }
      } else if (num == SYS_open || num == SYS_unlink || num == SYS_chdir || 
                 num == SYS_link || num == SYS_mkdir) {
        if (copyinstr(p->pagetable, path, args[0], sizeof(path)) < 0)
          path[0] = '\0';
      }
    }

    int ret = syscalls[num]();
    p->trapframe->a0 = ret;

    if ((1 << num) & p->mask) {
      printf("%d: syscall %s(", p->pid, syscallnames[num]);

      if (num == SYS_exec) {
        printf("\"%s\", [", path);
        for (int i = 0; i < 5 && argv_bufs[i]; i++) {
          if (i > 0) printf(", ");
          printf("\"%s\"", argv_bufs[i]);
        }
        printf("]");
      } else if (num == SYS_open) {
        printf("\"%s\", %d", path, args[1]);
      } else if (num == SYS_read || num == SYS_write) {
        printf("%d, 0x%x, %d", (int)args[0], args[1], (int)args[2]);
      } else {
        int argc = syscall_argcount[num];
        for (int i = 0; i < argc; i++) {
          if (i > 0) printf(", ");
          printf("%d", (int)args[i]);
        }
      }

      printf(") -> %d\n", ret);
    }

    if (num == SYS_exec) {
      for (int i = 0; i < 5; i++) {
        if (argv_bufs[i])
          kfree(argv_bufs[i]);
      }
    }
  } else {
    printf("%d %s: unknown sys call %d\n", p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
