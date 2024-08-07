//
// Created by wkf on 2021/10/29.
//

#ifndef TLC_TEST_FUZZER_H
#define TLC_TEST_FUZZER_H

#include "../TLAgent/ULAgent.h"
#include "../TLAgent/CAgent.h"

// used in Fuzzer::traceTest()
inline uint16_t chnAndOp(uint8_t a, uint8_t b) {
    return (uint16_t)((a << 8) | b);
}

class Fuzzer {
protected:
    uint64_t *cycles;
    std::queue<Transaction> transactions;
public:
    Fuzzer() = default;
    ~Fuzzer() = default;
    virtual void tick() = 0;
    virtual void traceTest() = 0;
    void set_cycles(uint64_t *cycles) {
        this->cycles = cycles;
    }
    void enqueue_transaction(Transaction &transaction) {
        this->transactions.push(transaction);
    }
    int get_queue_size() {
        return this->transactions.size();
    }
};

class ULFuzzer: public Fuzzer {
private:
    tl_agent::ULAgent *ulAgent;
public:
    ULFuzzer(tl_agent::ULAgent *ulAgent);
    void randomTest(bool put);
    void caseTest();
    void caseTest2();
    void traceTest();
    void tick();
};

class CFuzzer: public Fuzzer {
private:
    tl_agent::CAgent *cAgent;
public:
    CFuzzer(tl_agent::CAgent *cAgent);
    void randomTest(bool do_alias);
    void caseTest();
    void traceTest();
    void tick();
};

#endif //TLC_TEST_FUZZER_H
