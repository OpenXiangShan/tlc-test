#pragma once

#ifndef __TILELINK_DPI_HPP
#define __TILELINK_DPI_HPP

#include <cstdint>

#include "tilelink_dpi_configs.hpp"


#ifndef TLTEST_LOCAL_SEED
#   define TLTEST_LOCAL_SEED                        0
#endif


#ifndef TLTEST_LOCAL_CORE_COUNT
#   define TLTEST_LOCAL_CORE_COUNT                  1
#endif

#ifndef TLTEST_LOCAL_MASTER_COUNT_PER_CORE_TLC
#   define TLTEST_LOCAL_MASTER_COUNT_PER_CORE_TLC   1
#endif

#ifndef TLTEST_LOCAL_MASTER_COUNT_PER_CORE_TLUL
#   define TLTEST_LOCAL_MASTER_COUNT_PER_CORE_TLUL  0
#endif


/*
* DPI system interactions : Initialize
*/
extern "C" void TileLinkSystemInitialize();

/*
* DPI system interactions : Finalize
*/
extern "C" void TileLinkSystemFinalize();

/*
* DPI system interactions : Tick
*/
extern "C" void TileLinkSystemTick(
    const uint64_t      cycles);

/*
* DPI system interactions : Tock
*/
extern "C" void TileLinkSystemTock();


/*
* DPI function to connect TileLink Channel A
*/
extern "C" void TileLinkPushChannelA(
    const int           deviceId,
    const uint8_t       ready);

extern "C" void TileLinkPullChannelA(
    const int           deviceId,
    uint8_t*            valid,
    uint8_t*            opcode,
    uint8_t*            param,
    uint8_t*            size,
    uint8_t*            source,
    uint64_t*           address,
    uint8_t*            user_alias,
    uint32_t*           mask,
    uint64_t*           data0,
    uint64_t*           data1,
    uint64_t*           data2,
    uint64_t*           data3,
    uint8_t*            corrupt);
//


/*
* DPI function to connect TileLink Channel B
*/
extern "C" void TileLinkPushChannelB(
    const int           deviceId,
    const uint8_t       valid,
    const uint8_t       opcode,
    const uint8_t       param,
    const uint8_t       size,
    const uint8_t       source,
    const uint64_t      address,
    const uint32_t      mask,
    const uint64_t      data0,
    const uint64_t      data1,
    const uint64_t      data2,
    const uint64_t      data3,
    const uint8_t       corrupt);

extern "C" void TileLinkPullChannelB(
    const int           deviceId,
    uint8_t*            ready);
//


/*
* DPI function to connect TileLink Channel C
*/
extern "C" void TileLinkPushChannelC(
    const int           deviceId,
    const uint8_t       ready);

extern "C" void TileLinkPullChannelC(
    const int           deviceId,
    uint8_t*            valid,
    uint8_t*            opcode,
    uint8_t*            param,
    uint8_t*            size,
    uint8_t*            source,
    uint64_t*           address,
    uint8_t*            user_alias,
    uint64_t*           data0,
    uint64_t*           data1,
    uint64_t*           data2,
    uint64_t*           data3,
    uint8_t*            corrupt);
//


/*
* DPI function to connect TileLink Channel D
*/
extern "C" void TileLinkPushChannelD(
    const int           deviceId,
    const uint8_t       valid,
    const uint8_t       opcode,
    const uint8_t       param,
    const uint8_t       size,
    const uint8_t       source,
    const uint8_t       sink,
    const uint8_t       denied,
    const uint64_t      data0,
    const uint64_t      data1,
    const uint64_t      data2,
    const uint64_t      data3,
    const uint8_t       corrupt);

extern "C" void TileLinkPullChannelD(
    const int           deviceId,
    uint8_t*            ready);
//


/*
* DPI function to connect TileLink Channel E
*/
extern "C" void TileLinkPushChannelE(
    const int           deviceId,
    const uint8_t       ready);

extern "C" void TileLinkPullChannelE(
    const int           deviceId,
    uint8_t*            valid,
    uint8_t*            sink);
//


#endif // __TILELINK_DPI_HPP
