/**
 * 實驗目的：在傳送相同資料量的情況下，使用實體核心 (0 ~ 63 核心) 與使用 SMT (0 ~ 127 核心) 的傳輸時間比較。
 * 
 * 實驗設定 (單通道，順向: 0 -> 63 核心)：
 *  執行緒存取次數 (iteration_count) 設定為 400000
 *  僅有 group0 存取 shared_array_group0
 * 
 * 實驗設定 (單通道，逆向: 127 -> 64 核心)：
 *  執行緒存取次數 (iteration_count) 設定為 400000
 *  僅有 group1 存取 shared_array_group1
 * 
 * 實驗設定 (雙通道，順向: 0 -> 63, 逆向: 127 -> 64)：
 *  執行緒存取次數 (iteration_count) 設定為 20000
 *  group0 存取 shared_array_group0
 *  group1 存取 shared_array_group1
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>

#define NUM_THREADS 128
#define ARRAY_SIZE 256

volatile int current_thread_group0 = 0;      // 控制第 0~63 組執行緒的順序
volatile int current_thread_group1 = 127;    // 控制第 64~127 組執行緒的順序
char shared_array_group0[ARRAY_SIZE];        // 共享陣列，給第 0~63 組使用
char shared_array_group1[ARRAY_SIZE];        // 共享陣列，給第 64~127 組使用

int iteration_count = 0;
int group_code = 0;
volatile int start = 0;


typedef struct {
    int thread_id;
    int group_id; // 0 表示 0 ~ 63 組， 1 表示 64 ~ 127 組
} thread_arg_t;

void *thread_func(void *arg) {
    thread_arg_t *thread_args = (thread_arg_t *)arg;
    int thread_id = thread_args->thread_id;
    int group_id = thread_args->group_id;

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(thread_id, &cpuset);
    if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) != 0) {
        fprintf(stderr, "Failed to set CPU affinity for thread %d\n", thread_id);
    }

    while (start == 0)
        ;
    
    for (int iter = 0; iter < iteration_count; ++iter) {

        if (group_id == 0) { // 0 ~ 63 的邏輯
            while (current_thread_group0 != thread_id);
            if (group_code == 0 || group_code == 2) {
                // 存取陣列
                for (int i = 0; i < ARRAY_SIZE; i++) {
                    shared_array_group0[i] = thread_id;
                }
                
            }
            // 標記下一個執行緒
            current_thread_group0 = (current_thread_group0 + 1) % 64;

        } else if (group_id == 1) { // 64 ~ 127 的邏輯
            while (current_thread_group1 != thread_id);
            if (group_code == 1 || group_code == 2) {
                // 存取陣列
                for (int i = 0; i < ARRAY_SIZE; i++) {
                    shared_array_group1[i] = thread_id;
                }
            }
            // 標記下一個執行緒
            current_thread_group1 = (current_thread_group1 == 64) ? 127 : current_thread_group1 - 1;
            
        }
    }

    return NULL;
}

int main(int argc, char **argv) {

    printf("0: Forward\n1: Reverse\n2: Forward & Reverse\n");
    if(scanf(" %d", &group_code) == 0) {
        printf("error\n");
        exit(0);
    }

    if (group_code == 0 || group_code == 1) {
        iteration_count = 400000;
    } else if (group_code == 2) {
        iteration_count = 200000;
    }

    printf("iteration times: %d\n", iteration_count);

    pthread_t threads[NUM_THREADS];
    thread_arg_t thread_args[NUM_THREADS];

    // 初始化所有執行緒
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i].thread_id = i;
        thread_args[i].group_id = (i < 64) ? 0 : 1; // 0 ~ 63 為 Group 1， 64 ~ 127 為 Group 2
        if (pthread_create(&threads[i], NULL, thread_func, &thread_args[i]) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            return 1;
        }
    }

    struct timespec ts1, ts2;
    
    clock_gettime(CLOCK_REALTIME, &ts1);
    start = 1;

    // 等待所有執行緒完成
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }
    clock_gettime(CLOCK_REALTIME, &ts2);
    printf("%ld\n", (ts2.tv_sec - ts1.tv_sec)*1000000000 + (ts2.tv_nsec - ts1.tv_nsec));

    printf("All threads have completed their tasks.\n");

    return 0;


}