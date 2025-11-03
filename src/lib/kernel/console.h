#ifndef __LIB_KERNEL_CONSOLE_H
#define __LIB_KERNEL_CONSOLE_H

void console_init (void);
void console_panic (void);
void console_print_stats (void);
void console_getline(char *input); //added this
#endif /* lib/kernel/console.h */
