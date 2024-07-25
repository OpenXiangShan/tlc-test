//
// Created by ljw on 10/22/21.
//

#ifndef TLC_TEST_EMU_H
#define TLC_TEST_EMU_H

#include "verilated.h"
#include "VTestTop.h"
#include <getopt.h>
#if VM_TRACE == 1
#include "verilated_vcd_c.h"
#endif
#include "../Utils/ScoreBoard.h"
#include "../Utils/Common.h"
#include "../Utils/Trace.h"
#include "../TLAgent/ULAgent.h"
#include "../TLAgent/CAgent.h"
#include "../Fuzzer/Fuzzer.h"

class Emu {
private:
    typedef tl_agent::BaseAgent BaseAgent_t;
    typedef tl_agent::ULAgent ULAgent_t;
    typedef tl_agent::CAgent CAgent_t;

    const static int NR_AGENTS = NR_CAGENTS + NR_ULAGENTS;
    VTestTop *dut_ptr;
    VerilatedVcdC* tfp;
    GlobalBoard<paddr_t> *globalBoard;
    BaseAgent_t ** const agents = new BaseAgent_t*[NR_AGENTS];
    Fuzzer ** const fuzzers = new Fuzzer*[NR_AGENTS];
    uint64_t seed = 0, wave_begin = 0, wave_end = 0;
    bool enable_wave = true;
    bool enable_trace = false;
    std::ifstream trace_file;
    std::queue<Transaction> transactions;
    bool wave_full = false;
    inline char* cycle_wavefile(uint64_t cycles, time_t t);
    void parse_args(int argc, char **argv);

public:
    Emu(int argc, char **argv);
    ~Emu();
    uint64_t exe_cycles = 10000000;
    inline void reset(uint64_t n);
    inline void neg_edge();
    inline void pos_edge();
    inline void update_cycles(uint64_t inc);
    void execute(uint64_t nr_cycle);
    tl_agent::Port<tl_agent::ReqField, tl_agent::RespField, tl_agent::EchoField, BEATSIZE>* naive_gen_port();
#ifdef DUALCORE
    tl_agent::Port<tl_agent::ReqField, tl_agent::RespField, tl_agent::EchoField, BEATSIZE>* naive_gen_port2();
#endif
};

inline void Emu::reset(uint64_t n) {
    for (uint64_t i = 0; i < n; i++) {
        dut_ptr->reset = 1;
        dut_ptr->clock = 0;
        dut_ptr->eval();
        dut_ptr->clock = 1;
        dut_ptr->eval();
    }
    dut_ptr->reset = 0;
}

inline void Emu::neg_edge() {
    dut_ptr->clock = 0;
    dut_ptr->eval();
}

inline void Emu::pos_edge() {
    dut_ptr->clock = 1;
    dut_ptr->eval();
}

inline void Emu::update_cycles(uint64_t inc) {
    Cycles += inc;
    if (Cycles % 1000000 == 0) { printf("*\n"); }
}

inline char* Emu::cycle_wavefile(uint64_t cycles, time_t t) {
    static char buf[1024];
    char buf_time[64];
    strftime(buf_time, sizeof(buf_time), "%F@%T", localtime(&t));
    char *pwd = getcwd(NULL, 0);
    assert(pwd != NULL);
    int len = snprintf(buf, 1024, "%s/%s_%lu", pwd, buf_time, cycles);
    strcpy(buf + len, ".vcd");
    printf("dump wave to %s...\n", buf);
    return buf;
}

void abortHandler(int s);

#endif //TLC_TEST_EMU_H
