/*
Minimal example of pthread semaphore.

For more detail, please visit:
http://man7.org/linux/man-pages/man7/sem_overview.7.html

A producer-consumer implementation can be found:
https://www.softprayog.in/programming/posix-semaphores
*/

#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define SEM_FLAG_NAME "/iloveos"

sem_t *flag;

void *LearningRobot(void *id)
{
    int tid = (int) id;
    // down
    printf("\n%d: Down..\n", tid);
    sem_wait(flag);
    printf("\n%d: Entered Critical Region..\n", tid);

    // critical section
    sleep(4);

    // up
    printf("\n%d: Just Exiting Critical Region...\n", tid);
    printf("\n%d: Up..\n", tid);
    sem_post(flag);
    return NULL;
}

int main()
{
    pthread_t t1, t2, t3;
    /* sem_open: open a named semaphore
        - const char *: for the name of semaphore
        - int: flag (O_CREAT means create if not exist)
        - int: protection setting (0666 allows everyone to read/write)
        - int: initial value
    */
    flag = sem_open(SEM_FLAG_NAME, O_CREAT, 0666, 2);

    if (flag == SEM_FAILED) {
        printf("Open semaphore fails");
        return 1;
    }
    /* sem_getvalue does not work on macos
    int v = -1;
    if (sem_getvalue(flag, &v)) {
        printf("Get semaphore value fails");
        return 1;
    }
    printf("%d", v);
    */

    pthread_create(&t1, NULL, LearningRobot, (void *) 1);
    pthread_create(&t2, NULL, LearningRobot, (void *) 2);
    pthread_create(&t3, NULL, LearningRobot, (void *) 3);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    /* sem_close: close the named semaphore
        - semt_t *: semaphore pointer
    */
    if(sem_close(flag)){

        printf("Close semaphore failes");
    }

    /* sem_unlink: unlink the named semaphore
        - const char *: name of the semaphore
    */
    if (sem_unlink(SEM_FLAG_NAME)) {
        printf("Unlink semaphore fails");
    }
    return 0;
}
