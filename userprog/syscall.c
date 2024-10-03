#include "userprog/syscall.h"
#include "intrinsic.h"
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/loader.h"
#include "threads/thread.h"
#include "userprog/gdt.h"
#include <stdio.h>
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

void check_address(void *addr) {
    // TODO
    /* 포인터가 가리키는 주소가 유저 영역의 주소인지 확인 */
    /* 잘못된 접근일 경우 프로세스 종료 */
}

void get_argument(void *esp, int *arg, int cnt) {
    // TODO
    /* 유저 스택에 저장된 인자값들을 커널로 저장 */
    /* 인자가 저장된 위치가 유저영역인지 확인 */
}

/* The main system call interface */
void syscall_handler(struct intr_frame *f UNUSED) { // Q. 이건 어디서 불리는거지?
    // [ref] 한양대 자료 70p
    printf("system call!\n");

    // TODO: get stack stack pointer from interrupt frame
    int sys_call_number = 0;
    // TODO: get system call number from stack  parsing from rax (f->R.rax)
    // TODO: 시스템 콜 핸들러에서 유저 스택 포인터(esp) 주소와 인자가 가리키는 주소(포인터)가 유저 영역인지 확인
    // TODO: 유저 스택에 있는 인자들을 커널에 저장

    int exit_status = 0; // TODO

    switch (sys_call_number) {
    case SYS_HALT:
        halt();
        break;

    case SYS_EXIT:
        // exit(exit_status);
        break;
    case SYS_FORK:
        // fork();
        break;
    case SYS_EXEC:
        // exec();
        break;
    case SYS_WAIT:
        wait(f->R.rdi);
        break;
    case SYS_CREATE:
        // create();
        break;
    case SYS_REMOVE:
        // remove();
        break;
    case SYS_OPEN:
        // open();
        break;
    case SYS_FILESIZE:
        // filesize();
        break;
    case SYS_READ:
        // read();
        break;
    case SYS_WRITE:
        // write();
        break;
    case SYS_SEEK:
        // seek();
        break;
    case SYS_TELL:
        // tell();
        break;
    case SYS_CLOSE:
        // close();
        break;
    default:
        printf("Unknown system call: %d\n", sys_call_number);
        thread_exit();
        break;
    }

    // TODO: 시스템 콜의 함수의 리턴 값은 인터럽트 프레임의 eax에 저장
}

void halt() { power_off(); }

// TODO: 함수 구현 필요
int wait(pid_t tid) { return process_wait(tid); }