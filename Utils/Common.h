//
// Created by Kaifan Wang on 2021/10/27.
//

#ifndef TLC_TEST_COMMON_H
#define TLC_TEST_COMMON_H

#include <memory>
#include <cmath>
#include <string>
#include <cstring>
#include <assert.h>
#include <unistd.h>

#ifdef ENABLE_CHISEL_DB
extern void init_db(bool en, bool select_enable, const char *select_db);
extern void save_db(const char * filename);
#endif

extern uint64_t Cycles;
extern bool Verbose;
extern bool dump_db;

inline char* logdb_filename(time_t t) {
    static char buf[1024];
    char buf_time[64];
    strftime(buf_time, sizeof(buf_time), "%F@%T", localtime(&t));
    char *pwd = getcwd(NULL, 0);
    assert(pwd != NULL);
    int len = snprintf(buf, 1024, "%s/%s", pwd, buf_time);
    strcpy(buf + len, ".db");
    return buf;
}

enum {
    DATASIZE = 64, // Cache line is 64B
    BEATSIZE = 32,
    NR_ULAGENTS = 0,
    NR_CAGENTS = 1,
    NR_SOURCEID = 16,
    TIMEOUT_INTERVAL = 5000
};

typedef uint16_t paddr_t;

#ifdef ENABLE_CHISEL_DB
#define tlc_assert(cond, info) \
    do { \
        if (!(cond)) { \
            printf("\33[1;34m%s\n", info); \
            printf("Cycles: %ld\33[0m\n", Cycles); \
            fflush(stdout); \
            fflush(stderr); \
            assert(cond); \
        } \
    } while (0)
#else
#define tlc_assert(cond, info) \
    do { \
        if (!(cond)) { \
            printf("\33[1;34m%s\n", info); \
            printf("Cycles: %ld\33[0m\n", Cycles); \
            fflush(stdout); \
            fflush(stderr); \
            assert(cond); \
        } \
    } while (0)
#endif

#define Log(...) \
    do { \
        if (Verbose) { \
            printf("#%d ", this->id); \
            printf(__VA_ARGS__); \
            fflush(stdout); \
            fflush(stderr); \
        } \
    } while(0)

#define Dump(...) \
    do { \
        if (Verbose) { \
            printf(__VA_ARGS__); \
            fflush(stdout); \
            fflush(stderr); \
        } \
    } while(0)

#endif //TLC_TEST_COMMON_H
