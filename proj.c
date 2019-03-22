/*
ITCS343 Project 1 Startup Code
Thanapon Noraset
*/
#include <sys/time.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

struct timespec start;
int E, N;
int *status; //0=available | 1=helping
int *reportCount;
int totalReport;
sem_t *simu;
pthread_mutex_t checkingLock;
pthread_mutex_t server;

uint64_t delta_us()
{
    struct timespec current;
    clock_gettime(CLOCK_MONOTONIC_RAW, &current);
    return (current.tv_sec - start.tv_sec) * 1000000 + (current.tv_nsec - start.tv_nsec) / 1000;
}

int getLeftRobot(int tid) {
    int ltid;
    if(tid == 0) {
        ltid = N-1;
    } else {
        ltid = tid-1;
    }
    return ltid;
}

int getRightRobot(int tid) {
    int rtid;
    if(tid == N) {
        rtid = 0;
    } else {
        rtid = tid+1;
    }
    return rtid;
}

//to check left and right robot status
int checkFriendStatus(int tid) {
    int lefttid = getLeftRobot(tid);
    int righttid = getRightRobot(tid);
    int checkLockStatus;
    int acquireStatus;
    while((checkLockStatus = pthread_mutex_trylock(&checkingLock)) == 0) {
        printf("[LOCK] %d Acquired friend lock!\n", tid);
        if(status[lefttid] == 0 && status[righttid] == 0) {
            status[lefttid] = 1;
            status[righttid] = 1;
            printf("[LOCK] %d Acquired friends successfully!\n",tid);
            acquireStatus = 0;
            break;
            //acquire friends successfully
        } else {
            printf("[LOCK] %d Cannot acquire friends!\n",tid);
            acquireStatus = 1;
            break;
            //friends not available
        }
        if(checkLockStatus == 0) {
            pthread_mutex_unlock(&checkingLock);
            printf("[LOCK] %d Friend Unlock!\n",tid);
        }
    }
    return acquireStatus;
}

int releaseFriend(int tid) {
    int lefttid = getLeftRobot(tid);
    int righttid = getRightRobot(tid);
    int checkLockStatus;
    int releaseStatus;
    while((checkLockStatus = pthread_mutex_trylock(&checkingLock)) == 0) {
        printf("[RELEASE] %d Acquired friend lock!\n", tid);
        if(status[lefttid] == 1 && status[righttid] == 1) {
            status[lefttid] = 0;
            status[righttid] = 0;
            releaseStatus = 0;
            break;
            //release friends successfully
        } else {
            releaseStatus = 1;
            break;
            //friends cannot release
        }
    }
    if(checkLockStatus == 0) {
            pthread_mutex_unlock(&checkingLock);
            printf("[RELEASE]%d Friend Unlock!\n", tid);
    }
    return releaseStatus;
}

void sendReport(int tid) {
    int lockStatus;
    while((lockStatus = pthread_mutex_trylock(&server)) == 0) {
        printf("%d Acquired Server lock!\n",tid);
        reportCount[tid]++;
        totalReport++;
        if(lockStatus == 0) {
            pthread_mutex_unlock(&checkingLock);
            printf("%d Server Unlock!\n",tid);
            printf("UPDATE[%"PRIu64"]: ID:%d ReportID:%d \n", delta_us(), tid, reportCount[tid]);
            break;
        }
    }
}


void *tachikoma(void *arg)
{
    int e;
    int *g2g = (int *)arg;
    int tid = *(int *)arg;
    printf("%d is created\n", tid);
    while (*g2g != -1)
    {
        if(checkFriendStatus(tid) == 0) {
            printf("[%d,%d,%d] We're formed group! We want to learn!\n", getLeftRobot(tid), tid, getRightRobot(tid));
            sem_wait(simu);
            e = rand() % E + 1;
            printf("LEARN[%"PRIu64"]: ID:%d L:%d R:%d \n", delta_us(), tid, getLeftRobot(tid), getRightRobot(tid));
            fflush(stdout);
            sleep(e);
            status[tid] += e;
            printf("DONE[%"PRIu64"]: ID:%d L:%d R:%d \n", delta_us(), tid, getLeftRobot(tid), getRightRobot(tid));
            fflush(stdout);
            sem_post(simu);
            releaseFriend(tid);
            sendReport(tid);
        } else {
            //printf("[%d] Failed to form group\n", tid);
        }
    }

    if (*g2g == -1)
    {
        printf("SHUTDOWN[%"PRIu64"]: ID:%d L:%d R:%d \n", delta_us(), tid, getLeftRobot(tid), getRightRobot(tid));
    }

    return NULL;
}

int main(int argc, char **argv)
{
    srand(42);
    int M, T, i;
    N = atoi(argv[1]); // number of Tachikoma robots
    M = atoi(argv[2]); // number of Simulators
    E = atoi(argv[3]); // max time each learning (in seconds)
    T = atoi(argv[4]); // max time of the program
    pthread_t threads[N];
    int tachikomaId[N];

    status = (int *)malloc(sizeof(int) * N);
    reportCount = (int *)malloc(sizeof(int) * N);

    simu = sem_open("/simulator", O_CREAT, 0666, M);
    sem_close(simu);
    sem_unlink("/simulator");
    simu = sem_open("/simulator", O_CREAT, 0666, M);
    pthread_mutex_init(&server, NULL);
    pthread_mutex_init(&checkingLock, NULL);

    clock_gettime(CLOCK_MONOTONIC_RAW, &start);

    // Initialize Tachikoma Id and // Create Tachikoma Threads
    for (i = 0; i < N; i++)
    {
        tachikomaId[i] = i;
        status[i] = 0;
        pthread_create(&threads[i], NULL, tachikoma, (void *)&tachikomaId[i]);
    }

    // Wait until time over
    sleep(T);

    // Signal the time over
    for (i = 0; i < N; i++)
    {
        tachikomaId[i] = -1;
    }

    // Wait tachikoma to shutdown
    for (i = 0; i < N; i++)
    {
        pthread_join(threads[i], NULL);
    }
    for (i = 0; i < N; i++)
    {
        printf("%d\n", status[i]);
    }
    free(status);
    free(reportCount);
    sem_close(simu);
    sem_unlink("/simulator");
    printf("MASTER: Bye");
    return 0;
}
