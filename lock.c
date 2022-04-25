/*
 * A pseudo writer first lock by using one pthread_mutex
 */

#include <pthread.h>

/*
 * Debug macros, which can be enabled by adding -DDEBUG in the Makefile
 */

#ifdef DEBUGLOCK
#define dbg_assert_lock(...) assert(__VA_ARGS__)
#define dbg_printf_lock(...) fprintf(stderr, __VA_ARGS__)
#else
#define dbg_assert_lock(...)
#define dbg_printf_lock(...)
#endif

typedef struct {
    int count;
} cache_counter;

pthread_mutex_t cache_lock;
int writerCnt = 0;
int readerCnt = 0;
cache_counter *pre_reader;
cache_counter *after_reader;
cache_counter *writer;
cache_counter *tmp;
pthread_cond_t *writer_cond;
pthread_cond_t *reader_cond;
pthread_cond_t *reader_cond_2;
pthread_cond_t *reader_cond_tmp;

// pthread_mutex_t, writer first
void read_waitp() {
    pthread_mutex_lock(&cache_lock);
    if (writer->count > 0) {
        after_reader->count++;
        dbg_printf_lock("read_wait AFTER: pre_reader %d, writer %d, after_reader: %d\n",
                        pre_reader->count, writer->count, after_reader->count);
        // while (writer->count > 0) {
        // what if after writer comes 
        if (writer->count > 0) {
            dbg_printf_lock("   wait for pre reader & writer\n");
            pthread_cond_wait(reader_cond, &cache_lock);
        }
        dbg_printf_lock("   read_wait AFTER OK: pre_reader %d, writer %d, after_reader: %d\n",
                        pre_reader->count, writer->count, after_reader->count);
    } else {
        pre_reader->count++;
        dbg_printf_lock("read_wait PRE: pre_reader %d, writer %d, after_reader: %d\n",
                        pre_reader->count, writer->count, after_reader->count);
    }
    pthread_mutex_unlock(&cache_lock);
}

void read_postp() {
    pthread_mutex_lock(&cache_lock);
    pre_reader->count--;
    dbg_printf_lock("read_post: pre_reader %d, writer %d, after_reader: %d\n",
        pre_reader->count, writer->count, after_reader->count);

    if (pre_reader->count == 0) {
        pthread_cond_signal(writer_cond);
        dbg_printf_lock("   pre reader count = 0, writer get signals\n");
    }

    // after change, if there are after reader, how to wake previous waiting reader
    else {
        pthread_cond_signal(reader_cond_2);
        dbg_printf_lock("   other reader get signals\n");
    }
    pthread_mutex_unlock(&cache_lock);
}

void write_waitp() {
    pthread_mutex_lock(&cache_lock);
    writer->count++;
    dbg_printf_lock("write_wait: pre_reader %d, writer %d, after_reader: %d\n",
        pre_reader->count, writer->count, after_reader->count);
    while (pre_reader->count > 0) {
        dbg_printf_lock("   wait for pre reader\n");
        pthread_cond_wait(writer_cond, &cache_lock);
    }
    dbg_printf_lock("   write_wait OK: pre_reader %d, writer %d, after_reader: %d\n",
                    pre_reader->count, writer->count, after_reader->count);
}

void write_postp() {
    writer->count--;
    dbg_printf_lock("write_post: pre_reader %d, writer %d, after_reader: %d\n",
        pre_reader->count, writer->count, after_reader->count);
    if (writer->count == 0) {
        dbg_printf_lock("   writer count = 0, after reader get signals\n");
        tmp = pre_reader;
        pre_reader = after_reader;
        after_reader = tmp;
        reader_cond_tmp = reader_cond;
        reader_cond = reader_cond_2;
        reader_cond_2 = reader_cond_tmp;
        pthread_cond_signal(reader_cond_2);
        dbg_printf_lock("   re after CHANGE\n");
        dbg_printf_lock("   CHANGE: pre_reader %d, writer %d, after_reader: %d\n",
            pre_reader->count, writer->count, after_reader->count);
    } else {
        pthread_cond_signal(writer_cond);
        dbg_printf_lock("   other writer get signals\n");
    }
    pthread_mutex_unlock(&cache_lock);
}

void lock_init() {
    pthread_mutex_init(&cache_lock, NULL);

    pre_reader = malloc(sizeof(cache_counter));
    after_reader = malloc(sizeof(cache_counter));
    writer = malloc(sizeof(cache_counter));

    writer_cond = malloc(sizeof(pthread_cond_t));
    reader_cond = malloc(sizeof(pthread_cond_t));
    reader_cond_2 = malloc(sizeof(pthread_cond_t));

    pthread_cond_init(writer_cond, NULL);
    pthread_cond_init(reader_cond, NULL);
    pthread_cond_init(reader_cond_2, NULL);
}
