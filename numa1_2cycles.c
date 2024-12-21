#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>


#define NUM_THREADS 64
#define ARRAY_SIZE 256
#define NUM_ITERATIONS 200000 // 每個執行緒執行的存取次數

volatile int current_thread_group1 = 0;      // 控制第 0~31 組執行緒的順序
volatile int current_thread_group2 = 63;     // 控制第 32~63 組執行緒的順序
char shared_array_group1[ARRAY_SIZE];        // 共享陣列，給第 0~31 組使用
char shared_array_group2[ARRAY_SIZE];        // 共享陣列，給第 32~63 組使用

volatile int start=0;

typedef struct {
    int thread_id;
    int group_id; // 0 表示第 0~31 組，1 表示第 32~63 組
} thread_arg_t;

// 每個執行緒的工作函數
void* thread_func(void* arg) {
    thread_arg_t* thread_args = (thread_arg_t*)arg;
    int thread_id = thread_args->thread_id;
    int group_id = thread_args->group_id;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(thread_id, &cpuset);

    // 將執行緒綁定到指定核心
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        fprintf(stderr, "Failed to set CPU affinity for thread %d\n", thread_id);
        return NULL;
    }

    while(start == 0)
        ;
    //sleep(1);

    for (int iter = 0; iter < NUM_ITERATIONS; ++iter) {
        if (group_id == 0) { // 第 0~31 組的邏輯
            while (current_thread_group1 != thread_id);

            // 存取陣列
            //printf("  Group 1 - Thread %d is accessing the array, iteration %d.\n", thread_id, iter + 1);
            //printf("1");
            for (int i = 0; i < ARRAY_SIZE; i++) {
                //shared_array_group1[i] = thread_id;
            }

            // 標記下一個執行緒
            current_thread_group1 = (current_thread_group1 + 1) % 32;

        } else if (group_id == 1) { // 第 32~63 組的邏輯
            while (current_thread_group2 != thread_id);

            // 存取陣列
            //printf("Group 2 - Thread %d is accessing the array, iteration %d.\n", thread_id, iter + 1);
            //printf("2");
            for (int i = 0; i < ARRAY_SIZE; i++) {
                shared_array_group2[i] = thread_id;
                //asm("pause");
            }

            // 標記下一個執行緒
            current_thread_group2 = (current_thread_group2 == 32) ? 63 : current_thread_group2 - 1;
        }
    }

    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    thread_arg_t thread_args[NUM_THREADS];

    // 初始化所有執行緒
    for (int i = 0; i < NUM_THREADS; ++i) {
        thread_args[i].thread_id = i;
        thread_args[i].group_id = (i < 32) ? 0 : 1; // 第 0~31 為 Group 1，第 32~63 為 Group 2
        if (pthread_create(&threads[i], NULL, thread_func, &thread_args[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            return 1;
        }
    }

    struct timespec ts1, ts2;
    clock_gettime(CLOCK_REALTIME, &ts1);

    start=1;

    // 等待所有執行緒完成
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }
    clock_gettime(CLOCK_REALTIME, &ts2);
    printf("%ld\n", (ts2.tv_sec - ts1.tv_sec)*1000000000+(ts2.tv_nsec-ts1.tv_nsec));

    printf("All threads have completed their tasks.\n");

    return 0;
}
