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

void* simulateCustomer(void *args) {
    Arguments *arguments = (Arguments*)args;
    arguments->gone = true;
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
    while (dayCounter < 50){
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