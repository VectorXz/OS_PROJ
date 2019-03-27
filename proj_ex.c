/*
ITCS343 Project 1 Startup Code
Thanapon Noraset
*/
#define _POSIX_C_SOURCE 199309L //IDK, stackoverflow said that if i put this, BLOCK_MONOTONIC_RAW will not error
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

#define FREE 0
#define LEARN 1
#define HELP 2

struct timespec start;
int E, N;
int *status; //0=available | 1=helping
int *reportCount;
int *learningTime;
int *priority;
int totalReport;
sem_t *simu;
pthread_mutex_t checkingLock;
pthread_mutex_t server;
pthread_mutex_t priorityLock;

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
    if(tid == N-1) {
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
    int acquireStatus = -1;
    int allowToRun;
    while((checkLockStatus = pthread_mutex_trylock(&checkingLock)) == 0) {
        //printf("[LOCK] %d Acquired friend lock!\n", tid);
        if(status[tid] != LEARN && status[lefttid] != LEARN && status[righttid] != LEARN) {
                int locker = pthread_mutex_lock(&priorityLock);
                if(locker == 0) {
                    if(priority[tid] > 0) {
                        int value;
                        sem_getvalue(simu, &value);
                        if(value > 0) {
                            allowToRun = 1;
                        } else {
                            priority[tid]--;
                            allowToRun = 0;
                        }
                    } else {
                        allowToRun = 1;
                    }
                    pthread_mutex_unlock(&priorityLock);
                }
                if(allowToRun == 1) {
                status[tid] = LEARN;
                status[lefttid] = HELP;
                status[righttid] = HELP;
                //printf("[LOCK] %d Acquired friends successfully!\n",tid);
                acquireStatus = 0;
                //acquire friends successfully
                } else {
                    acquireStatus = 1;
                }
            } else {
            //printf("[LOCK] %d Cannot acquire friends!\n",tid);
            acquireStatus = 1;
            //friends not available
        }
        if(checkLockStatus == 0) {
            pthread_mutex_unlock(&checkingLock);
            //printf("[LOCK] %d Friend Unlock!\n",tid);
        }
        //checkLockStatus = -1;
        break;
        }
    return acquireStatus;
}

int releaseFriend(int tid) {
    int checkLockStatus;
    //printf("%d is going to finish learning\n", tid);
    while((checkLockStatus = pthread_mutex_trylock(&checkingLock)) == 0) {
        //printf("[RELEASE] %d Acquired friend lock!\n", tid);
        status[tid] = FREE;
        pthread_mutex_unlock(&checkingLock);
        break;
    }
    return 0;
}

void sendReport(int tid) {
    //printf("[%d] is sending report\n", tid);
    int lockStatus;
    while((lockStatus = pthread_mutex_trylock(&server)) == 0) {
        //printf("%d Acquired Server lock!\n",tid);
        reportCount[tid]++;
        totalReport++;
        pthread_mutex_unlock(&server);
        //printf("%d Server Unlock!\n",tid);
        //printf("\t\tUPDATE[%"PRIu64"]: ID:%d ReportID:%d \n", delta_us(), tid, reportCount[tid]); //for debug print
        printf("UPDATE[%"PRIu64"]: %d, %d \n", delta_us(), tid, totalReport);
        break;
    }
}


void *tachikoma(void *arg)
{
    int e;
    int *g2g = (int *)arg;
    int tid = *(int *)arg;
    //printf("%d is created\n", tid);
    while (*g2g != -1)
    {
        if(checkFriendStatus(tid) == 0) {
            //printf("[%d,%d,%d] We're formed group! We want to learn!\n", getLeftRobot(tid), tid, getRightRobot(tid));
            sem_wait(simu);
            e = rand() % E + 1;
            //printf("LEARN[%"PRIu64"]: L:%d [ID:%d] R:%d \n", delta_us(), getLeftRobot(tid), tid, getRightRobot(tid)); //for debug print
            printf("LEARN[%"PRIu64"]: %d, %d, %d \n", delta_us(), tid, getLeftRobot(tid), getRightRobot(tid));
            fflush(stdout);
            sleep(e);
            learningTime[tid] += e;
            //printf("\tDONE[%"PRIu64"]: L:%d [ID:%d] R:%d \n", delta_us(), getLeftRobot(tid), tid, getRightRobot(tid)); //for debug print
            printf("DONE[%"PRIu64"]: %d, %d, %d \n", delta_us(), tid, getLeftRobot(tid), getRightRobot(tid));
            fflush(stdout);
            sem_post(simu);
            releaseFriend(tid);
            sendReport(tid);
            int locker = pthread_mutex_lock(&priorityLock);
            if(locker == 0) {
                priority[tid] = 6;
                pthread_mutex_unlock(&priorityLock);
            }
            sleep(1);
         } else {
            sleep(1);
         }
    }

    if (*g2g == -1)
    {
        //printf("SHUTDOWN[%"PRIu64"]: %d \n", delta_us(), tid);
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
    learningTime = (int *)malloc(sizeof(int) * N);
    priority = (int *)malloc(sizeof(int) * N);

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
        pthread_create(&threads[i], NULL, tachikoma, (void *)&tachikomaId[i]);
        status[i] = FREE;
        priority[i] = 0;
        learningTime[i] = 0;
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
    /* printf("====[STATUS]====\n");
    for (i = 0; i < N; i++)
    {
        printf("[%d] : %d\n",i ,status[i]);
    } */
    /* printf("\n====[REPORT]====\n");
    for (i = 0; i < N; i++)
    {
        printf("[%d] : %d, %d\n",i ,learningTime[i] ,reportCount[i]);
    } */
    for (i = 0; i < N; i++)
    {
        printf("%d: %d, %d\n",i ,learningTime[i] ,reportCount[i]);
    }
    printf("MASTER: %d\n",totalReport);
    free(status);
    free(reportCount);
    free(priority);
    free(learningTime);
    sem_close(simu);
    sem_unlink("/simulator");
    printf("MASTER: Bye\n");
    return 0;
}
