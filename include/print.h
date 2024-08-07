#ifndef _PRINT_H
#define _PRINT_H

void print_init();
int printk(const char *fmt, ...);

extern char printk_buffer[4096];

#endif
