//
// Created by wkf on 2021/10/29.
//

#include "Fuzzer.h"
extern int trans_count;
CFuzzer::CFuzzer(tl_agent::CAgent *cAgent) {
    this->cAgent = cAgent;
}

void CFuzzer::randomTest(bool do_alias) {
    paddr_t addr = ((rand() % 0x8) << 13) + ((rand() % 0x80) << 6);  // Tag + Set + Offset
    int alias = (do_alias) ? (rand() % 4) : 0;
    if (rand() % 2) {
        if (rand() % 3) {
            if (rand() % 2) {
                cAgent->do_acquireBlock(addr, tl_agent::NtoT, alias); // AcquireBlock NtoT
            } else {
                cAgent->do_acquireBlock(addr, tl_agent::NtoB, alias); // AcquireBlock NtoB
            }
        } else {
            cAgent->do_acquirePerm(addr, tl_agent::NtoT, alias); // AcquirePerm
        }
    } else {
        /*
        uint8_t* putdata = new uint8_t[DATASIZE];
        for (int i = 0; i < DATASIZE; i++) {
            putdata[i] = (uint8_t)rand();
        }
        cAgent->do_releaseData(addr, tl_agent::TtoN, putdata); // ReleaseData
        */
        cAgent->do_releaseDataAuto(addr, alias); // feel free to releaseData according to its priv
    }
}

void CFuzzer::caseTest() {
    if (*cycles == 100) {
        this->cAgent->do_acquireBlock(0x1040, tl_agent::NtoT, 0);
    }
    if (*cycles == 300) {
        uint8_t* putdata = new uint8_t[DATASIZE];
        for (int i = 0; i < DATASIZE; i++) {
            putdata[i] = (uint8_t)rand();
        }
        this->cAgent->do_releaseData(0x1040, tl_agent::TtoN, putdata, 0);
    }
    if (*cycles == 400) {
      this->cAgent->do_acquireBlock(0x1040, tl_agent::NtoT, 0);
    }
}

void CFuzzer::traceTest() {
    if (this->transactions.empty()) {
        return;
    }
    Transaction t = this->transactions.front();
    paddr_t addr = t.addr;
    uint8_t channel = t.channel;
    uint8_t opcode = t.opcode;
    uint8_t param = t.param;
    int send_status;

    switch (chnAndOp(channel, opcode))
    {
    case (0 << 8) | tl_agent::AcquireBlock:
        send_status = this->cAgent->do_acquireBlock(addr, param, 0);    break;
    case (0 << 8) | tl_agent::AcquirePerm:
        send_status = this->cAgent->do_acquirePerm(addr, param, 0);     break;
    // even if transaction has param, we still use releaseDataAuto here
    // in fear of releaseData may have unknown bugs untested
    case (2 << 8) | tl_agent::ReleaseData:
    case (2 << 8) | tl_agent::Release:
        send_status = this->cAgent->do_releaseDataAuto(addr, 0);    break;
    default:
        std::cerr << "Error: Invalid C Transaction " << channel << " Opcode " << opcode << std::endl;
        break;
    }
    // printf("[DEBUG] C tring to send %s\n", t.to_string().c_str());
    // if succeeded/fail/pass to send, remove it from queue
    // TODO: whether still to send for PASS transations (whose permission is already satisfied)
    if (send_status != tl_agent::PENDING) {
        this->transactions.pop();
    }
    if (send_status == tl_agent::SUCCESS) trans_count++;
    // otherwise try it next cycle
}

void CFuzzer::tick() {
    this->randomTest(true);
//    this->caseTest();
}
