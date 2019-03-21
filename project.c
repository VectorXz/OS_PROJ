#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>
#include <time.h>

#define SEM_FLAG_NAME "/oslover"

//N = no. of tachikoma
//M = no. of simulator
//E = maximum time of playing per robot
//T = total time that this program consume

int N,M,E,T;

sem_t *flag;

void *LearningRobot(void *id)
{
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    int tid = (int) id;
    // down
    printf("\n[%s] %d: Down..\n",asctime (timeinfo), tid);
    sem_wait(flag);
    printf("\n[%s] %d: Entered Critical Region..\n",asctime (timeinfo), tid);

    // critical section
    sleep(4);

    // up
    printf("\n[%s] %d: Just Exiting Critical Region...\n",asctime (timeinfo), tid);
    printf("\n[%s] %d: Up..\n",asctime (timeinfo), tid);
    sem_post(flag);
    return NULL;
}


int main() {

    printf("Please input no. of Tachikoma : ");
    scanf("%d",&N);
    printf("Please input no. of simulator : ");
    scanf("%d",&M);
    printf("Please input max learning time : ");
    scanf("%d",&E);
    printf("Please input max running time : ");
    scanf("%d",&T);

    printf("\n====[CONFIGURATION SUMMARY]====\n");
    printf("Tachikoma : %d\n",N);
    printf("Simulator : %d\n",M);
    printf("Max learning time : %d\n",E);
    printf("Max running time : %d\n",T);

    flag = sem_open(SEM_FLAG_NAME, O_CREAT, 0666, 2);

    if (flag == SEM_FAILED) {
        printf("Open semaphore fails");
        return 1;
    }

    pthread_t threads[N];
    int t;
    for(t=0;t<N;t++) {
        pthread_create(&threads[t], NULL, LearningRobot, (void*) t);
    }
    for(t=0;t<N;t++) {
        pthread_join(threads[t], NULL);
    }

    if(sem_close(flag)){

        printf("Close semaphore failes");
    }
    if(sem_unlink(SEM_FLAG_NAME)) {
        printf("Unlink semaphore fails");
    }

    return 0;

}
