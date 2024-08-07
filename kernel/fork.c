#include <errno.h>
#include <fs.h>
#include <kernel.h>
#include <memory.h>
#include <print.h>
#include <sched.h>
#include <string.h>
#include <sys_call.h>
#include <system.h>
#include <types.h>

#define _TSS(n) ((((unsigned long)n) << 4) + (FIRST_TSS_ENTRY << 3))
#define _LDT(n) ((((unsigned long)n) << 4) + (FIRST_LDT_ENTRY << 3))

long last_pid;

int find_empty_process() {
  int i;

repeat: // avoid pid conflict
  if ((++last_pid) < 0)
    last_pid = 1;
  for (i = 0; i < NR_TASKS; i++) //
    if (task[i] && ((task[i]->pid == last_pid) || (task[i]->pgrp == last_pid)))
      goto repeat;
  for (i = 1; i < NR_TASKS; i++) // find available
    if (!task[i])
      return i;
  return -EAGAIN;
}

int copy_process(int nr, long ebp, long edi, long esi, long gs, long none,
                 long ebx, long ecx, long edx, long orig_eax, long fs, long es,
                 long ds, long eip, long cs, long eflags, long esp, long ss) {
  struct task_struct *p;
  int i;
  struct file *f;

  p = (struct task_struct *)get_free_page();
  if (!p)
    return -EAGAIN;
  task[nr] = p;
  memcpy(p, current, sizeof(struct task_struct));
  p->state = TASK_UNINTERRUPTIBLE;
  p->pid = last_pid;
  p->counter = p->priority;
  p->signal = 0;
  p->alarm = 0;
  p->leader = 0; /* process leadership doesn't inherit */
  p->utime = p->stime = 0;
  p->cutime = p->cstime = 0;
  p->start_time = jiffies;
  p->tss.back_link = 0;
  p->tss.esp0 = PAGE_SIZE + (long)p;
  p->tss.ss0 = 0x10;
  p->tss.eip = eip;
  p->tss.eflags = eflags;
  p->tss.eax = 0;
  p->tss.ecx = ecx;
  p->tss.edx = edx;
  p->tss.ebx = ebx;
  p->tss.esp = esp;
  p->tss.ebp = ebp;
  p->tss.esi = esi;
  p->tss.edi = edi;
  p->tss.es = es & 0xffff;
  p->tss.cs = cs & 0xffff;
  p->tss.ss = ss & 0xffff;
  p->tss.ds = ds & 0xffff;
  p->tss.fs = fs & 0xffff;
  p->tss.gs = gs & 0xffff;
  p->tss.ldt = _LDT(nr);
  p->tss.trace_bitmap = 0x80000000;
  if (last_task_used_math == current) // clts 防止 fnsave 触发 device not available
    __asm__("clts ; fnsave %0 ; frstor %0" ::"m"(p->tss.i387)); // 复制 fpu相关的寄存器 这些寄存器没有被push到栈上 必须手动保存
  if (copy_mem(nr, p)) {
    task[nr] = NULL;
    free_page((long)p);
    return -EAGAIN;
  }
  for (i = 0; i < NR_OPEN; i++)
    if ((f = p->filp[i]))
      f->f_count++;
  if (current->pwd)
    current->pwd->i_count++;
  if (current->root)
    current->root->i_count++;
  if (current->executable)
    current->executable->i_count++;
  if (current->library)
    current->library->i_count++;
  set_tss_desc(((char *)gdt) + _TSS(nr), &(p->tss));
  set_ldt_desc(((char *)gdt) + _LDT(nr), &(p->ldt));
  p->p_pptr = current;
  p->p_cptr = 0;
  p->p_ysptr = 0;
  p->p_osptr = current->p_cptr;
  if (p->p_osptr)
    p->p_osptr->p_ysptr = p;
  current->p_cptr = p;
  p->state = TASK_RUNNING; /* do this last, just in case */
  return last_pid;
}

int copy_mem(int nr, struct task_struct *p) {
  unsigned long old_data_base, new_data_base, data_limit;
  unsigned long old_code_base, new_code_base, code_limit;

  code_limit = get_limit(0x0f);
  data_limit = get_limit(0x17);
  old_code_base = get_base(current->ldt[1]);
  old_data_base = get_base(current->ldt[2]);
  if (old_data_base != old_code_base)
    panic("We don't support separate I&D");
  if (data_limit < code_limit)
    panic("Bad data_limit");
  new_data_base = new_code_base = nr * TASK_SIZE;
  p->start_code = new_code_base;
  set_base(p->ldt[1], new_code_base);
  set_base(p->ldt[2], new_data_base);
  printk("new base of %d = %x %x\n", nr, get_base(p->ldt[1]),
         get_base(p->ldt[2]));

  if (current == &init_task.task) {
    copy_init(new_data_base);
  } else if (copy_page_tables(old_data_base, new_data_base, data_limit)) {
    free_page_tables(new_data_base, data_limit);
    return -ENOMEM;
  }
  return 0;
}
