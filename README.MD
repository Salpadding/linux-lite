# 精简版 linux 0.12

## 总体架构

### qemu

借助 qemu 提供的接口, 我们无需写磁盘io相关的汇编代码

qemu 的作用

1. 把 raw 格式的内核映像加载到 0x100000
2. 把根文件系统,也就是硬盘映像 hdc.img 加载到 0x800000 也就是 8MB 

于是我们的内核可以把内存作为根文件系统

### bootloader

bootloader 只干两件事

1. 从实模式进入保护模式

源代码位于 mbr/mbr.c
三步走: 开启a20, 设置并且加载 gdt, 长跳转到 jump.c

2. 跳转到 0x100000

避免 32位代码和 16位代码混用, 跳转指令写在了 mbr/jump.c


### 内核-进程管理

1. pid 生成

代码:

```c
int find_empty_process() {
  int i;

repeat: 
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
```

分析:

    1. last_pid 初始值为0 和语义一致, 就是最近创建的进程的 id, 存放于 bss
    2. find_empty_process 调用后, 会对 last_pid 作累加, 直到 last_pid 不与任何 pid 重复
    3. task 数组静态分配了 64 个指针, 用于保存已创建的进程


2. 新进程初始化

    1. 新进程需要分配栈内存 这块内存大小正好是一个页面 所以通过 `get_free_page` 分配
    2. `task[nr] = p` 把新进程记录到全局进程表, 注意这里 nr 不是 pid
    3. `memcpy(p, current, sizeof(struct task_struct))` 拷贝进程元数据
    4. `p->state = TASK_UNINTERRUPTIBLE` 新的进程还没有完全初始化 不可以被调度上 cpu 所以设置为不可以被调度的状态
    5. `p->pid = last_pid` 分配刚才申请到的 pid
    6. `p->counter = p->priority` 分配时间片
    7.  父进程的很多数据不用传递给子进程
        `p->signal = 0; p->alarm = 0; `
        `p->leader = 0; p->utime = p->stime = 0;`
        `p->cutime = p->cstime = 0;`
        `p->start_time = jiffies;` 补充启动时间

    8. 初始化 tss
        
        tss 是 intel 硬件曾提供的进程切换机制, linux 早期也是采用的基于硬件的进程切换, 后期 linux 考虑到了兼容性和性能逐渐转向了通过软件实现进程切换。

        但是即便是基于软件的进程切换, 我们的系统调用功能仍依赖于 tss。因为系统调用前后发生了特权级的变更, 不同的特权级的栈内存不可以有重叠, tss 可以实现在切换特权级的同时切换栈顶指针。这就要用到 tss 中的 esp0 和 ss0, 这两个字段是 tss 最重要的字段, 是系统调用实现的关键部分。


        ```c
          p->tss.back_link = 0;
          p->tss.esp0 = PAGE_SIZE + (long)p;

          // linux0.12 用户态程序使用的是 ldt, 把 tss.ss0 指向  gdt
          p->tss.ss0 = 0x10;
          // 下面就是基本的拷贝上下文的操作了
          // 这个 eip 指向的是 int 0x80 指令的下一条
          p->tss.eip = eip;
          p->tss.eflags = eflags;
          // 这里很有意思, fork 后子进程会读取到 fork() 的返回值是 0
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
          p->tss.ldt = _LDT(nr); // 新进程的 ldt 号也是根据 nr 静态分配的
          p->tss.trace_bitmap = 0x80000000;
        ```

3. 进程虚拟地址分配

每个进程的虚拟地址空间是 nr * 64MB ~ (nr + 1) * 64MB
通过 ldt 实现


## 内核-缓冲区

1. getblk 函数

首先 free_list 是一个有点误导性的命名，我认为命名为 buffer_list 更合理，因为代码里面 buffer_head 不会离开 free_list

getblk 函数有一个很有意思的逻辑, 如果在 hash_table 中发现了 device 对应的 block 直接就 return 了。
但如果不是在 hash_table 发现的 block, 而是在 free_list 中发现的 block 就有点复杂了。 

首先这个函数的作者会认为 free_list 中可能存在大量的 b_dirt = 1 的 block, 如果我在 free_list 里面找了一圈都没有找到 b_dirt = 0 的话，说明积累的buffer就有点多了，那么就有必要进行一次 sync 操作，把buffer中的内容同步到外部设备。



## 常见问题

1. 为什么特权级变更的时候 cpu 要切换 esp, 但是特权级不变更的时候不会自动切换esp

- 这里面引出第一个问题: 用户态和内核态之间能否共享一个栈?

从内核态的角度来说显然是不行的, 内核态的栈如果能被用户态访问，这显然是个严重的安全漏洞，很容易被恶意软件利用。
从用户态的角度来说也不合理，如果内核态的代码把用户态的栈写坏了，那么用户态的进程就会出现段错误。

- 继续引出第二个问题: 内核态之间通过 中断/系统调用的方式互相调用, 是否要切换栈区?

首先要明确一点，任何一个内核态的栈区都有对应的进程，进程的元数据保存在栈的底部。在linux中，内核代码不属于任意一个特定的进程，而是被所有进程共享。如果在内核态与内核态之间发生了栈区的切换，那必然是发生了进程的切换。如果是在当前进程中执行系统调用，那必然不可以切换栈区。


- 假设我们认为用户态和内核态不共享栈区

有了以上的假设，那么首先切换栈区的权利在内核态手里。我们假设 cpu 没有自动切换栈区的能力，每一段处理中断/系统调用的内核态代码都要判断调用方是谁，如果是用户态那么就要切换栈区，如果是内核态就不能切换栈区。那么问题就来了，作为一段内核态的中断/系统调用处理函数来说，我就躺平等着eip指到我，我就执行，我也不知道是用户态代码在调用我还是内核态的代码在调用我。也就是说，内核代码必须依赖 cpu 作出判断是否要切换栈区。


2. 操作系统如何切换浮点运算相关的寄存器 例如 xmm0-xmm15

这个问题可以分成四个部分: 硬件支持 何时保存 保存到哪 何时恢复 从哪里恢复


- 硬件支持

cpu/fpu 提供了 fnsave, frstor 指令用户保存和恢复浮点寄存器
当 cr0 的 ts 标志位被设置位 1 时, 浮点相关的指令会触发 device not found 异常, 操作系统可以在异常处理中保存和恢复浮点寄存器

- 何时保存

    1. 有新的任务执行浮点寄存器相关的指令时
    2. fork 新进程时，要把调用 fork 的那个进程的 fpu 寄存器 保存到新的进程

- 保存到哪 

通过 fnsave 保存到 task_struct 中的 i387 结构体, 这个 task_struct 可以用过全局变量 last_task_used_math 找到

- 何时恢复

当有进程尝试调用浮点相关指令时, 要把 task_struct 中的 i387 恢复到浮点寄存器



3. 被 fork 后的进程执行的第一个代码是在用户态还是内核态

必然是用户态的代码 所以 copy-on-write 就显得尤为重要, 如果 copy-on-write 处理的不好，有可能父进程会把子进程的栈写坏。

4. 对于新的进程, 我们有必要去 mmap esp0 所在的内存吗。

完全没有必要，这段内存只有内核有权限访问。记住 mmap 只需要服务于用户态程序
