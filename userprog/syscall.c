#include "userprog/syscall.h"
#include "devices/input.h"
#include "filesys/file.h"
#include "intrinsic.h"
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/loader.h"
#include "threads/thread.h"
#include "userprog/gdt.h"
#include "userprog/process.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>

void syscall_entry(void);
void syscall_handler(struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void syscall_init(void) {
    write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48 | ((uint64_t)SEL_KCSEG) << 32);
    write_msr(MSR_LSTAR, (uint64_t)syscall_entry);

    /* The interrupt service rountine should not serve any interrupts
     * until the syscall_entry swaps the userland stack to the kernel
     * mode stack. Therefore, we masked the FLAG_FL. */
    write_msr(MSR_SYSCALL_MASK, FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

void validate_n_update_argv(struct intr_frame *ifp, uint64_t *argv) {
    // TODO: 시스템 콜 핸들러에서 유저 스택 포인터(rsp) 주소와 인자가 가리키는 주소(포인터)가 유저 영역인지 확인
    dev_printf("============== check_addr ==============\n");
    argv[0] = ifp->R.rdi;
    argv[1] = ifp->R.rsi;
    argv[2] = ifp->R.rdx;
    argv[3] = ifp->R.rcx;
    argv[4] = ifp->R.r8;

    for (int i = 0; i < 5; i++) {
        dev_printf("argv[%d]: %llu\n", i, argv[i]);
    }
    dev_printf("============== result ==============\n");
}

/* The main system call interface */
void syscall_handler(struct intr_frame *ifp) {
    uint64_t argv[5];
    int sys_call_num = ifp->R.rax;
    dev_printf("system call! [%d]\n", sys_call_num);
    validate_n_update_argv(ifp, argv);

    switch (sys_call_num) {
    case SYS_HALT:
        halt();
        break;
    case SYS_EXIT:
        int exit_status = argv[0];
        exit(exit_status);
        break;
    case SYS_FORK:
        // fork();
        break;
    case SYS_EXEC:
        char *file_name = argv[0];
        exec(file_name);
        break;
    case SYS_WAIT:
        wait(argv[0]);
        break;
    case SYS_CREATE:
        ifp->R.rax = create(argv[0], argv[1]);
        break;
    case SYS_REMOVE:
        remove(argv[0]);
        break;
    case SYS_OPEN:
        open(argv[0]);
        break;
    case SYS_FILESIZE:
        filesize(argv[0]);
        break;
    case SYS_READ:
        read(argv[0], argv[1], argv[2]);
        break;
    case SYS_WRITE:
        write(argv[0], argv[1], argv[2]);
        break;
    case SYS_SEEK:
        seek(argv[0], argv[1]);
        break;
    case SYS_TELL:
        tell(argv[0]);
        break;
    case SYS_CLOSE:
        close(argv[0]);
        break;
    default:
        dev_printf("Unknown system call: %d\n", sys_call_num);
        thread_exit();
        break;
    }

    // TODO: 시스템 콜의 함수의 리턴 값은 인터럽트 프레임의 eax에 저장
}

void halt() { power_off(); }

void exit(int exit_code) {
    struct thread *curr = thread_current();
    printf("%s: exit(%d)\n", curr->name, exit_code);
    thread_exit();
}

int wait(pid_t tid) { return process_wait(tid); }

int exec(const char *file) { return process_create_initd(file); }

// ============== FILE SYSTEM ==============
int create(const char *file, unsigned initial_size) {
    if (file == NULL)
        exit(-1);
    else if (strlen(file) == 0)
        return 0;

    return filesys_create(file, initial_size);
}

int remove(const char *file) { return filesys_remove(file); }

int open(const char *file) {
    struct file *fp = filesys_open(file);
    return process_add_file(fp);
}

int filesize(int fd) {
    struct file *fp = process_get_file(fd);
    return file_length(fp);
}

int read(int fd, void *buffer, unsigned length) {
    if (fd == 0) {
        return input_getc();
    } else {
        struct file *fp = process_get_file(fd);
        return file_read(fd, buffer, length);
    }
}

int write(int fd, const void *buffer, unsigned length) {
    if (fd == 1) {
        putbuf(buffer, length);
        return 0;
    } else {
        struct file *fp = process_get_file(fd);
        int written_n = file_write(fp, buffer, length);
        return written_n;
    }
}

void seek(int fd, unsigned position) {
    struct file *fp = process_get_file(fd);
    file_seek(fp, position);
}

unsigned tell(int fd) {
    struct file *fp = process_get_file(fd);
    return file_tell(fd);
}

void close(int fd) {
    struct file *fp = process_get_file(fd);
    file_close(fp);
}