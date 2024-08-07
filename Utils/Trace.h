#ifndef TRACE_H
#define TRACE_H

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <queue>

#define READ_ONCE 10000
#define END_TIMER 20000 // cycles to run after trace all sent

// TODO: consider alias and other user fields
class Transaction {
public:
    uint32_t timestamp;
    uint8_t agentId;
    uint8_t channel; // 0: A, 1: B, 2: C, 3: D, 4: E
    uint8_t opcode;
    paddr_t addr;
    uint8_t param;

    Transaction(uint32_t timestamp, uint8_t agentId, uint8_t channel, uint8_t opcode, paddr_t addr, uint8_t param) {
        this->timestamp = timestamp;
        this->agentId = agentId;
        this->channel = channel;
        this->opcode = opcode;
        this->addr = addr;
        this->param = param;
    }

    Transaction(std::string line) {
        std::istringstream iss(line);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(iss, token, ',')) {
            tokens.push_back(token);
        }
        if (tokens.size() != 6) {
            std::cerr << "Error: invalid trace line: " << line << std::endl;
            exit(1);
        }
        this->timestamp = std::stoi(tokens[0]);
        this->agentId = std::stoi(tokens[1]);
        this->channel = std::stoi(tokens[2]);
        this->opcode = std::stoi(tokens[3]);
        this->addr = std::stoul(tokens[4]);
        this->param = std::stoi(tokens[5]);
    }

    std::string to_string() {
        std::ostringstream oss;
        oss << timestamp << ":" << (int)agentId << ":" << (int)channel << "," << (int)opcode << "," << std::hex << addr << "," << (int)param;
        return oss.str();
    }
};


#endif //TRACE_H