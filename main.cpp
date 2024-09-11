#include <iostream>
#include <csignal>
#include "Emu/Emu.h"

extern int trans_count;
int main(int argc, char **argv) {
    std::signal(SIGABRT, abortHandler);

    Emu emu(argc, argv);
    emu.reset(10);
    emu.execute(emu.exe_cycles);
    std::cout << std::endl << "Finished" << std::endl;
    std::cout << "Transactions: " << trans_count << std::endl;
    return 0;
}
