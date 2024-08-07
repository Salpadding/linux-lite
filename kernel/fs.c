#include <fs.h>
#include <sys_call.h>
#include <print.h>


struct super_block super_block[NR_SUPER];

void sync_inodes() {}

int sys_setup() {
    printk("setup system");

    // 挂载 root 文件系统
    mount_root();

    return 0;
}
