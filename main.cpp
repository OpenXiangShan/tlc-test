#include <iostream>
#include <csignal>
#include "Emu/Emu.h"

Emu *globalEmuPtr = nullptr;
extern int trans_count;

int main(int argc, char **argv) {
    std::signal(SIGABRT, abortHandler);

    Emu emu(argc, argv);
    globalEmuPtr = &emu;
    emu.reset(10);
    emu.execute(emu.exe_cycles);
    std::cout << std::endl << "Finished" << std::endl;
    std::cout << "Transactions: " << trans_count << std::endl;
    return 0;
}
