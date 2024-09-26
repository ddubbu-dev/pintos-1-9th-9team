/* Creates N threads, each of which sleeps a different, fixed
   duration, M times.  Records the wake-up order and verifies
   that it is valid. */

#include "devices/timer.h"
#include "tests/threads/tests.h"
#include "threads/init.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include <stdio.h>

static void test_sleep(int thread_cnt, int iterations);

void test_alarm_single(void) { test_sleep(5, 1); }

void test_alarm_multiple(void) { test_sleep(5, 7); }

/* Information about the test. */
struct sleep_test {
    int64_t start;  /* Current time at start of test. */
    int iterations; /* Number of iterations per thread. */

    /* Output. */
    struct lock output_lock; /* Lock protecting output buffer. */
    int *output_pos;         /* Current position in output buffer. */
};

/* Information about an individual thread in the test. */
struct sleep_thread {
    struct sleep_test *test; /* Info shared between all threads. */
    int id;                  /* Sleeper ID. */
    int duration;            /* Number of ticks to sleep. */
    int iterations;          /* Iterations counted so far. */
};

static void sleeper(void *);

/* Runs THREAD_CNT threads thread sleep ITERATIONS times each. */
static void test_sleep(int thread_cnt, int iterations) {
    struct sleep_test test;
    struct sleep_thread *threads;
    int *output, *op;
    int product;
    int i;

    /* This test does not work with the MLFQS. */
    ASSERT(!thread_mlfqs);

    msg("Creating %d threads to sleep %d times each.", thread_cnt, iterations);
    msg("Thread 0 sleeps 10 ticks each time,");
    msg("thread 1 sleeps 20 ticks each time, and so on.");
    msg("If successful, product of iteration count and");
    msg("sleep duration will appear in nondescending order."); // 반복횟수 x 수면시간 오름차순

    /* Allocate memory. */
    threads = malloc(sizeof *threads * thread_cnt);
    output = malloc(sizeof *output * iterations * thread_cnt * 2); // 각 쓰레드가 깨어나는 순서대로 자신의 ID가 저장
    if (threads == NULL || output == NULL)
        PANIC("couldn't allocate memory for test");

    /* Initialize test. */
    test.start = timer_ticks() + 100; // 100만큼 지연 시작
    test.iterations = iterations;
    lock_init(&test.output_lock);
    test.output_pos = output; // output 쓰기 위치

    /* Start threads. */
    ASSERT(output != NULL);
    for (i = 0; i < thread_cnt; i++) {
        struct sleep_thread *t = threads + i; // threads 배열의 i번째 요소
        char name[16];

        t->test = &test;
        t->id = i;
        t->duration = (i + 1) * 10; // 10 간격으로 실행됨
        t->iterations = 0;

        snprintf(name, sizeof name, "thread %d", i);
        thread_create(name, PRI_DEFAULT, sleeper, t);
    }

    /* Wait long enough for all the threads to finish. */
    timer_sleep(100 + thread_cnt * iterations * 10 + 100); // main thread sleep; 총 이만큼의 tick 발생

    // ============================ main thread 깨어난 뒤 테스트 검증 ============================
    /**
     * Acquire the output lock in case some rogue thread is still running.
     * lock_acquire, lock_release 는 동기화를 위해 사용됨.
     * 여러 쓰레드가 동시에 공유 자원에 접근할 때 데이터의 일관성을 보장하기 위해 Mutual Exclusion 구현
     *  */
    lock_acquire(&test.output_lock);

    /* Print completion order. */
    product = 0;
    for (op = output; op < test.output_pos; op++) {
        struct sleep_thread *t;
        int new_prod;

        ASSERT(*op >= 0 && *op < thread_cnt); // op = idx; 0 ~ (thread_cnt-1)
        t = threads + *op;                    // threads 배열의 i번째 요소

        new_prod = ++t->iterations * t->duration;

        msg("thread %d: duration=%d, iteration=%d, product=%d", t->id, t->duration, t->iterations, new_prod);

        if (new_prod >= product)
            product = new_prod;
        else
            fail("thread %d woke up out of order (%d > %d)!", t->id, product, new_prod); // product 오름차순 보장
    }

    /* Verify that we had the proper number of wakeups. */
    for (i = 0; i < thread_cnt; i++)
        if (threads[i].iterations != iterations)
            fail("thread %d woke up %d times instead of %d", i, threads[i].iterations, iterations);

    lock_release(&test.output_lock);
    free(output);
    free(threads);
}

/* Sleeper thread. */
static void sleeper(void *t_) {
    struct sleep_thread *t = t_;
    struct sleep_test *test = t->test;
    int i;

    for (i = 1; i <= test->iterations; i++) {
        int64_t sleep_until = test->start + i * t->duration;
        timer_sleep(sleep_until - timer_ticks());
        lock_acquire(&test->output_lock); // 깨어난 뒤 lock 후 output_pos 업데이트
        *test->output_pos++ = t->id;      // output_pos를 이용해서 기록 후 위치 이동
        lock_release(&test->output_lock); // lock 해제
    }
}
