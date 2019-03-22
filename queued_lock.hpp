#pragma once

#include <list>

#include <pthread.h>

class queued_lock {
public:
    queued_lock() : busy(false), queue_edit_mutex(PTHREAD_MUTEX_INITIALIZER) { }

    void lock(int id) {
        pthread_mutex_lock(&queue_edit_mutex);

        if (busy) {
            conditionals_list.push_back(PTHREAD_COND_INITIALIZER);
            ids_list.push_back(id);
            pthread_cond_t &condition = conditionals_list.back();
            pthread_cond_wait(&condition, &queue_edit_mutex);

            conditionals_list.pop_front();
            ids_list.pop_front();
            pthread_mutex_unlock(&queue_edit_mutex);
        } else {
            busy = true;
            pthread_mutex_unlock(&queue_edit_mutex);
        }
    }

    bool try_lock(int id, const struct timespec &abstime) {
        pthread_mutex_lock(&queue_edit_mutex);

        if (busy) {
            conditionals_list.push_back(PTHREAD_COND_INITIALIZER);
            ids_list.push_back(id);
            pthread_cond_t &condition = conditionals_list.back();
            int result = pthread_cond_timedwait(&condition, &queue_edit_mutex, &abstime);
            if (result == 0) {
                conditionals_list.pop_front();
                ids_list.pop_front();
            } else {
                conditionals_list.pop_back();
                ids_list.pop_back();
            }
            pthread_mutex_unlock(&queue_edit_mutex);
            return result == 0;
        } else {
            busy = true;
            pthread_mutex_unlock(&queue_edit_mutex);
            return true;
        }
    }

    void unlock() {
        pthread_mutex_lock(&queue_edit_mutex);

        if (conditionals_list.size() == 0) {
            busy = false;
        } else {
            pthread_cond_signal(&conditionals_list.front());
        }
        pthread_mutex_unlock(&queue_edit_mutex);
    }

    const std::list<int> get_locked_ids() const {
        return ids_list;
    }

private:
    std::list<pthread_cond_t> conditionals_list;
    std::list<int> ids_list;
    pthread_mutex_t queue_edit_mutex;
    bool busy;
};
