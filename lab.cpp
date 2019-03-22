#include <pthread.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <queue>

#define CUSTOMER_COUNT 1000

using namespace std;

int cashbox_count[2];

pthread_mutex_t cashbox_1;

pthread_t customers[CUSTOMER_COUNT];

struct ticket_lock {
    pthread_mutex_t mutex;
    queue<pthread_cond_t*> Queue;
    bool busy;

    ticket_lock(){
        pthread_mutex_init(&mutex, NULL);
        busy = false;
    }

    int lock(timespec time){
        int result = 0;
        pthread_mutex_lock(&mutex);
        if (busy != false) {
            pthread_cond_t newCondition;
            pthread_cond_init(&newCondition, NULL);
            Queue.push(&newCondition);
            if (busy) {
                result = pthread_cond_timedwait(&newCondition, &mutex, &time);
            }
        }
        else {
            printf("Никого нет!\n");
            busy = true;
        }
        pthread_mutex_unlock(&mutex);
        return result;
    }

    void unlock(){
        pthread_mutex_lock(&mutex);
        if (Queue.empty()){
            printf("Пусто\n");
            busy = false;
        }
        else {
            pthread_cond_signal(Queue.front());
            Queue.pop();
        }
        pthread_mutex_unlock(&mutex);
    }
};

ticket_lock queue_1;
ticket_lock queue_2;
ticket_lock queue_manager;

struct Arguments {
    int id;
    int gone;
    struct timespec t1;
    struct timespec t2;
    struct timespec t3;
};

Arguments customer_Arguments[CUSTOMER_COUNT];

timespec getTimespec(unsigned int msTime){
    struct timespec time;
    time.tv_sec = msTime/1000;
    time.tv_nsec = (msTime - time.tv_sec*1000)*1000*1000;
    return time;
}

void Sleep(unsigned int msTime){
    timespec time = getTimespec(msTime);
    int res = nanosleep(&time, NULL);
}

timespec GetCorrectTime(timespec time){
    timeval now;
    
    gettimeofday(&now, NULL);
    
    timespec wait;

    wait.tv_sec = now.tv_sec + time.t1.tv_sec;
    wait.tv_nsec = now.tv_usec*1000 + time.t1.tv_nsec;
    return wait;
}

void* simulateCustomer(void *args) {
    Arguments *arguments = (Arguments*)args;

    ticket_lock *queue_first = NULL;
    ticket_lock *queue_second = NULL;
    int variant = 0;

    if (random()%1) {
        queue_first = &queue_1;
        queue_second = &queue_2;
        variant = 1;
    }
    else {
        queue_first = &queue_2;
        queue_second = &queue_1;
        variant = 2;
    }

    Sleep(random()%10000);
    printf("Пришел клиент №%d\n", arguments->id);
    
    int result = queue_first->lock(GetCorrectTime(arguments->t1));
    if (result == 0){
        Sleep(100 + random() % 100);
        queue_first->unlock();
        printf("Обслужили клиента №%d в кассе №%d\n", arguments->id, variant == 1 ? 1: 2);
        return (void*)1;
    }

    result = queue_second->lock(GetCorrectTime(arguments->t1));
    if (result == 0){
        Sleep(100 + random() % 100);
        queue_second->unlock();
        printf("Обслужили клиента №%d в кассе №%d\n", arguments->id, variant == 1 ? 2: 1);
        return (void*)2;
    }

    result = queue_first->lock(GetCorrectTime(arguments->t2));
    if (result == 0){
        Sleep(100 + random() % 100);
        queue_first->unlock();
        printf("Обслужили клиента №%d в кассе №%d(1)\n", arguments->id, variant == 1 ? 1: 2);
        return (void*)3;
    }

    result = queue_manager.lock(GetCorrectTime(arguments->t3));
    if (result == 0){
        Sleep(100 + random() % 100);
        queue_manager.unlock();
        printf("Обслужили клиента №%d у менеджера\n", arguments->id);
        return (void*)4;
    }
    
    arguments->gone = true;
    printf("Клиент №%d ушел\n", arguments->id);
}

int main() {
    for (int i =0; i < CUSTOMER_COUNT; i++) {
        customer_Arguments[i].id = i;
        customer_Arguments[i].gone = 0;
        customer_Arguments[i].t1 = getTimespec(750 + random() % 750);
        customer_Arguments[i].t2 = getTimespec(1500 + random() % 1500);
        customer_Arguments[i].t3 = getTimespec(500 + random() % 500);
    }

    int dayCounter = 0;
    while (dayCounter < 10){
        int current_day_customer = 0;
        for (int i = 0; i < CUSTOMER_COUNT; i++){
            if (!customer_Arguments[i].gone)
            {
                int result = pthread_create(&customers[i], NULL, simulateCustomer, (void *) &customer_Arguments[i]);
                current_day_customer++;
            }
        }

        for (int i=0; i < CUSTOMER_COUNT; i++){
            if (!customer_Arguments[i].gone){
                int result = pthread_join(customers[i], NULL);
            }
        }
        printf("День %d: магазин пришло %d посетителей\n", dayCounter, current_day_customer);
        dayCounter++;
    }
    return 0;
}