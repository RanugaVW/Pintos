#include "threads/vaddr.h"
#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static int get_user (const uint8_t *uaddr) {
  if (!uaddr || !is_user_vaddr(uaddr)) return -1;
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}


static bool put_user (uint8_t *udst, uint8_t byte) {
  if (!udst || !is_user_vaddr(udst)) return false;
  int error_code;
  asm ("movl $1f, %0; movb %b2, %1; 1:"
       : "=&a" (error_code), "=m" (*udst) : "q" (byte));
  return error_code != -1;
}

static void syscall_handler (struct intr_frame *);

void syscall_init (void) {
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void syscall_handler (struct intr_frame *f UNUSED) 
{
  int syscall_number = get_user((uint8_t *)f->esp);
  printf("[DEBUG] System call number: %d\n", syscall_number);

  switch (syscall_number) {
    case SYS_HALT:
      shutdown_power_off();
      break;
    case SYS_EXIT: {
      int status = get_user((uint8_t *)f->esp + 4);
      thread_current()->exit_status = status;
      thread_exit();
      break;
    }
    case SYS_WRITE: {
      int fd = get_user((uint8_t *)f->esp + 4);
      uint32_t buf_ptr = *(uint32_t *)((uint8_t *)f->esp + 8);
      unsigned size = get_user((uint8_t *)f->esp + 12);
      if (fd == 1 && is_user_vaddr((void *)buf_ptr) && size > 0) {
        char local_buf[256];
        unsigned to_write = size > 256 ? 256 : size;
        for (unsigned i = 0; i < to_write; i++) {
          int byte = get_user((uint8_t *)(buf_ptr + i));
          if (byte == -1) {
            f->eax = -1;
            return;
          }
          local_buf[i] = (char)byte;
        }
        putbuf(local_buf, to_write);
        f->eax = to_write;
      } else {
        f->eax = -1;
      }
      break;
    }
    default:
      printf("Unknown system call: %d\n", syscall_number);
      thread_exit();
  }
}
