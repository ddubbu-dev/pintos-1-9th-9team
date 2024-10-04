#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#if defined(USERPROG) || defined(FILESYS) || defined(VM)
#define dev_printf(...)                                                                                                                                                                                \
    do {                                                                                                                                                                                               \
    } while (0)
#else
#define dev_printf(...) printf(__VA_ARGS__)
#endif

typedef int pid_t;

void syscall_init(void);
void halt(void);
void exit(int exit_code);
int fork(const char *thread_name);
int exec(const char *file);
int wait(pid_t pid);
int create(const char *file, unsigned initial_size);
int remove(const char *file);
int open(const char *file);
int filesize(int fd);
int read(int fd, void *buffer, unsigned length);
int write(int fd, const void *buffer, unsigned length);
void seek(int fd, unsigned position);
unsigned tell(int fd);
void close(int fd);

#endif /* userprog/syscall.h */
