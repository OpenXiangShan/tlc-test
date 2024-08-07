//
// Created by wkf on 2021/10/29.
//

#include "Fuzzer.h"
extern int trans_count;
ULFuzzer::ULFuzzer(tl_agent::ULAgent *ulAgent) {
    this->ulAgent = ulAgent;
}

void ULFuzzer::randomTest(bool put) {
    paddr_t addr = (rand() % 0x400) << 6;
    if (!put || rand() % 2) {  // Get
        ulAgent->do_getAuto(addr);
    } else { // Put
        uint8_t* putdata = new uint8_t[DATASIZE];
        for (int i = 0; i < DATASIZE; i++) {
            putdata[i] = (uint8_t)rand();
        }
        ulAgent->do_putfulldata(addr, putdata);
    }
}

void ULFuzzer::caseTest() {
    if (*cycles == 500) {
        uint8_t* putdata = new uint8_t[DATASIZE];
        for (int i = 0; i < DATASIZE/2; i++) {
            putdata[i] = (uint8_t)rand();
        }
        for (int i = DATASIZE/2; i < DATASIZE; i++) {
            putdata[i] = putdata[i-DATASIZE/2];
        }
        ulAgent->do_putpartialdata(0x1070, 2, 0xf0000, putdata);
    }
    if (*cycles == 600) {
      ulAgent->do_getAuto(0x1040);
    }
}

void ULFuzzer::caseTest2() {
  if (*cycles == 100) {
    uint8_t* putdata = new uint8_t[DATASIZE];
    for (int i = 0; i < DATASIZE/2; i++) {
      putdata[i] = (uint8_t)rand();
    }
    for (int i = DATASIZE/2; i < DATASIZE; i++) {
      putdata[i] = putdata[i-DATASIZE/2];
    }
    ulAgent->do_putpartialdata(0x1000, 2, 0xf, putdata);
  }
  if (*cycles == 500) {
    ulAgent->do_get(0x1000, 2, 0xf);
  }
}

void ULFuzzer::tick() {
    this->randomTest(false);
//    this->caseTest();
}

void ULFuzzer::traceTest() {
  if (this->transactions.empty()) {
      return;
  }
  Transaction t = this->transactions.front();
  paddr_t addr = t.addr;
  uint8_t channel = t.channel;
  uint8_t opcode = t.opcode;
  int send_status;
  uint8_t* putdata = new uint8_t[DATASIZE];

  switch (chnAndOp(channel, opcode))
  {
  case (0 << 8) | tl_agent::Get:
      send_status = this->ulAgent->do_getAuto(addr);    break;
  case (0 << 8) | tl_agent::PutFullData:
      for (int i = 0; i < DATASIZE; i++) {
          putdata[i] = (uint8_t)rand();
      }
      send_status = this->ulAgent->do_putfulldata(addr, putdata);    break;
  default:
      std::cerr << "Error: Invalid UL Transaction " << channel << " Opcode " << opcode << std::endl;
      break;
  }
  // printf("[DEBUG] UL tring to send %s\n", t.to_string().c_str());
  // if succeeded/fail/pass to send, remove it from queue
  // TODO: whether still to send for PASS transations (whose permission is already satisfied)
  if (send_status != tl_agent::PENDING) {
      this->transactions.pop();
  }
  if (send_status == tl_agent::SUCCESS) trans_count++;
  // otherwise try it next cycle
}