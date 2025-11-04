#include "threads/vaddr.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

static int get_user (const uint8_t *uaddr) {
  if (!uaddr || !is_user_vaddr(uaddr)) return -1;
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));
  return result;
}

/* Read a 32-bit word from user memory. */
static int32_t get_user_word (const uint32_t *uaddr) {
  // Check that all 4 bytes are readable
  if (!uaddr || !is_user_vaddr((uint8_t *)uaddr) || !is_user_vaddr((uint8_t *)uaddr + 3)) 
    return -1;
  int result;
  asm ("movl $1f, %0; movl %1, %0; 1:"
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
  // Check that esp is in valid user memory range BEFORE trying to read from it
  // f->esp is the saved user ESP value at the time of interrupt
  if (!is_user_vaddr(f->esp)) {
    thread_current()->exit_status = -1;
    thread_exit();
  }
  
  // Read the syscall number using get_user_word to ensure all 4 bytes are readable
  int32_t syscall_number_word = get_user_word((uint32_t *)f->esp);
  if (syscall_number_word == -1) {
    // If we can't read syscall number, terminate process
    thread_current()->exit_status = -1;
    thread_exit();
  }
  
  int syscall_number = (int)syscall_number_word;

  switch (syscall_number) {
    case SYS_HALT:
      shutdown_power_off();
      break;
    case SYS_EXIT: {
      // Check that esp+4 is valid before reading
      uint8_t *status_addr = (uint8_t *)f->esp + 4;
      if (!is_user_vaddr(status_addr) || !is_user_vaddr(status_addr + 3)) {
        thread_current()->exit_status = -1;
        thread_exit();
      }
      int status = get_user_word((uint32_t *)status_addr);
      if (status == -1) {
        // If we couldn't read the status, set it to -1
        status = -1;
      }
      thread_current()->exit_status = status;
      thread_exit();
      break;
    }
    case SYS_EXEC: {
      // Get filename pointer from stack (first argument, offset +4)
      uint32_t filename_ptr = get_user_word((uint32_t *)((uint8_t *)f->esp + 4));
      
      // Check for NULL pointer
      if (filename_ptr == 0 || !is_user_vaddr((void *)filename_ptr)) {
        f->eax = -1;
        break;
      }
      
      // Extract filename string safely
      char filename[256];
      int i = 0;
      while (i < 255) {
        int byte = get_user((uint8_t *)filename_ptr + i);
        if (byte == -1) {
          f->eax = -1;
          return;
        }
        filename[i] = (char)byte;
        if (filename[i] == '\0') break;
        i++;
      }
      filename[255] = '\0';
      
      // Execute the program
      tid_t child_tid = process_execute(filename);
      f->eax = child_tid;
      break;
    }
    case SYS_WAIT: {
      // Get child_tid from stack (first argument, offset +4)
      tid_t child_tid = (tid_t)get_user_word((uint32_t *)((uint8_t *)f->esp + 4));
      
      // Wait for child process
      int exit_status = process_wait(child_tid);
      f->eax = exit_status;
      break;
    }
    case SYS_CREATE: {
      // Get filename and initial_size from stack
      uint32_t filename_ptr = get_user_word((uint32_t *)((uint8_t *)f->esp + 4));
      unsigned initial_size = get_user_word((uint32_t *)((uint8_t *)f->esp + 8));
      
      // Check for NULL pointer - invalid pointer should terminate process
      if (filename_ptr == 0 || !is_user_vaddr((void *)filename_ptr)) {
        thread_current()->exit_status = -1;
        thread_exit();
      }
      
      // Extract filename safely
      char filename[256];
      int i = 0;
      while (i < 255) {
        int byte = get_user((uint8_t *)filename_ptr + i);
        if (byte == -1) {
          f->eax = false;
          return;
        }
        filename[i] = (char)byte;
        if (filename[i] == '\0') break;
        i++;
      }
      filename[255] = '\0';
      
      bool success = filesys_create(filename, initial_size);
      f->eax = success;
      break;
    }
    case SYS_REMOVE: {
      // Get filename from stack (first argument, offset +4)
      uint32_t filename_ptr = get_user_word((uint32_t *)((uint8_t *)f->esp + 4));
      
      // Check for NULL pointer - invalid pointer should terminate process
      if (filename_ptr == 0 || !is_user_vaddr((void *)filename_ptr)) {
        thread_current()->exit_status = -1;
        thread_exit();
      }
      
      // Extract filename safely
      char filename[256];
      int i = 0;
      while (i < 255) {
        int byte = get_user((uint8_t *)filename_ptr + i);
        if (byte == -1) {
          f->eax = false;
          return;
        }
        filename[i] = (char)byte;
        if (filename[i] == '\0') break;
        i++;
      }
      filename[255] = '\0';
      
      bool success = filesys_remove(filename);
      f->eax = success;
      break;
    }
    case SYS_OPEN: {
      // Get filename from stack (first argument, offset +4)
      uint32_t filename_ptr = get_user_word((uint32_t *)((uint8_t *)f->esp + 4));
      
      // Check for NULL pointer - invalid pointer should terminate process
      if (filename_ptr == 0 || !is_user_vaddr((void *)filename_ptr)) {
        thread_current()->exit_status = -1;
        thread_exit();
      }
      
      // Extract filename safely
      char filename[256];
      int i = 0;
      while (i < 255) {
        int byte = get_user((uint8_t *)filename_ptr + i);
        if (byte == -1) {
          f->eax = -1;
          return;
        }
        filename[i] = (char)byte;
        if (filename[i] == '\0') break;
        i++;
      }
      filename[255] = '\0';
      
      // Open file and find available fd
      struct file *file_ptr = filesys_open(filename);
      if (file_ptr == NULL) {
        f->eax = -1;
        break;
      }
      
      // Find first available file descriptor
      struct thread *cur = thread_current();
      int fd = 2; // Start from 2 (0=stdin, 1=stdout)
      while (fd < 128 && cur->fd_table[fd] != NULL) {
        fd++;
      }
      
      if (fd >= 128) {
        file_close(file_ptr);
        f->eax = -1;
      } else {
        cur->fd_table[fd] = file_ptr;
        f->eax = fd;
      }
      break;
    }
    case SYS_FILESIZE: {
      // Get fd from stack (first argument, offset +4)
      int fd = get_user_word((uint32_t *)((uint8_t *)f->esp + 4));
      
      if (fd < 0 || fd >= 128) {
        f->eax = -1;
        break;
      }
      
      struct thread *cur = thread_current();
      struct file *file_ptr = cur->fd_table[fd];
      
      if (file_ptr == NULL) {
        f->eax = -1;
      } else {
        f->eax = file_length(file_ptr);
      }
      break;
    }
    case SYS_READ: {
      // Get fd, buffer, and size from stack (offsets +4, +8, +12)
      int fd = get_user_word((uint32_t *)((uint8_t *)f->esp + 4));
      uint32_t buffer_ptr = get_user_word((uint32_t *)((uint8_t *)f->esp + 8));
      unsigned size = get_user_word((uint32_t *)((uint8_t *)f->esp + 12));
      
      // Check for NULL pointer and valid user address range - invalid pointer terminates process
      if (buffer_ptr == 0 || !is_user_vaddr((void *)buffer_ptr)) {
        thread_current()->exit_status = -1;
        thread_exit();
      }
      
      // Also check end of buffer if size > 0
      if (size > 0 && !is_user_vaddr((void *)(buffer_ptr + size - 1))) {
        thread_current()->exit_status = -1;
        thread_exit();
      }
      
      if (fd < 0 || fd >= 128) {
        f->eax = -1;
        break;
      }
      
      struct thread *cur = thread_current();
      struct file *file_ptr = cur->fd_table[fd];
      
      if (file_ptr == NULL) {
        if (fd == 0) {
          // stdin - not supported
          f->eax = -1;
        } else {
          f->eax = -1;
        }
      } else {
        // Read from file into local buffer, then copy to user space
        char local_buf[512];
        unsigned to_read = size > 512 ? 512 : size;
        off_t bytes_read = file_read(file_ptr, local_buf, to_read);
        
        if (bytes_read > 0) {
          // Copy to user space
          for (int i = 0; i < bytes_read; i++) {
            if (!put_user((uint8_t *)buffer_ptr + i, local_buf[i])) {
              f->eax = -1;
              return;
            }
          }
        }
        f->eax = bytes_read;
      }
      break;
    }
    case SYS_WRITE: {
      // This was already handled above, but let's keep it here for clarity
      int fd = get_user_word((uint32_t *)((uint8_t *)f->esp + 4));
      uint32_t buf_ptr = get_user_word((uint32_t *)((uint8_t *)f->esp + 8));
      unsigned size = get_user_word((uint32_t *)((uint8_t *)f->esp + 12));
      
      // Check for NULL pointer and valid user address range
      if (buf_ptr == 0 || !is_user_vaddr((void *)buf_ptr)) {
        f->eax = -1;
        break;
      }
      
      // Also check end of buffer if size > 0
      if (size > 0 && !is_user_vaddr((void *)(buf_ptr + size - 1))) {
        f->eax = -1;
        break;
      }
      
      if (fd == 1) {
        // stdout
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
      } else if (fd >= 2 && fd < 128) {
        // File descriptor
        struct thread *cur = thread_current();
        struct file *file_ptr = cur->fd_table[fd];
        
        if (file_ptr == NULL) {
          f->eax = -1;
        } else {
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
          off_t bytes_written = file_write(file_ptr, local_buf, to_write);
          f->eax = bytes_written;
        }
      } else {
        f->eax = -1;
      }
      break;
    }
    case SYS_SEEK: {
      // Get fd and position from stack (offsets +4, +8)
      int fd = get_user_word((uint32_t *)((uint8_t *)f->esp + 4));
      unsigned position = get_user_word((uint32_t *)((uint8_t *)f->esp + 8));
      
      if (fd < 0 || fd >= 128) {
        break;
      }
      
      struct thread *cur = thread_current();
      struct file *file_ptr = cur->fd_table[fd];
      
      if (file_ptr != NULL) {
        file_seek(file_ptr, position);
      }
      break;
    }
    case SYS_TELL: {
      // Get fd from stack (first argument, offset +4)
      int fd = get_user_word((uint32_t *)((uint8_t *)f->esp + 4));
      
      if (fd < 0 || fd >= 128) {
        f->eax = -1;
        break;
      }
      
      struct thread *cur = thread_current();
      struct file *file_ptr = cur->fd_table[fd];
      
      if (file_ptr == NULL) {
        f->eax = -1;
      } else {
        f->eax = file_tell(file_ptr);
      }
      break;
    }
    case SYS_CLOSE: {
      // Get fd from stack (first argument, offset +4)
      int fd = get_user_word((uint32_t *)((uint8_t *)f->esp + 4));
      
      if (fd < 0 || fd >= 128) {
        break;
      }
      
      struct thread *cur = thread_current();
      struct file *file_ptr = cur->fd_table[fd];
      
      if (file_ptr != NULL) {
        file_close(file_ptr);
        cur->fd_table[fd] = NULL;
      }
      break;
    }
    default:
      printf("Unknown system call: %d\n", syscall_number);
      thread_exit();
  }
}
