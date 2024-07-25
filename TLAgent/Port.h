//
// Created by ljw on 10/21/21.
//

#ifndef TLC_TEST_PORT_H
#define TLC_TEST_PORT_H

#include "../Utils/Common.h"
#include <cstddef>
#include <cstdint>
#include <array>

namespace tl_agent {

    enum {
        PutFullData = 0,
        PutPartialData,
        ArithmeticData,
        LogicalData,
        Get,
        Hint,
        AcquireBlock,
        AcquirePerm
    };
    enum {
        Probe = 6
    };
    enum {
        AccessAck = 0,
        AccessAckData,
        HintAck,
        ProbeAck = 4,
        ProbeAckData,
        Release,
        ReleaseData
    };
    enum {
        Grant = 4,
        GrantData,
        ReleaseAck
    };
    enum {
        GrantAck = 0
    };

    enum {
        toT = 0,
        toB,
        toN
    };
    enum {
        NtoB = 0,
        NtoT,
        BtoT
    };
    enum {
        TtoB = 0,
        TtoN,
        BtoN,
        TtoT,
        BtoB,
        NtoN
    };
    enum {
        INVALID = 0,
        BRANCH,
        TRUNK,
        TIP
    };

    class Decoupled {
    public:
        uint8_t *valid;
        uint8_t *ready;

        bool fire() const {
            return *valid && *ready;
        }
    };

    template<class Usr, class Echo, std::size_t N>
    class ChnA : public Decoupled {
    public:
        uint8_t *opcode;
        uint8_t *param;
        uint8_t *size;
        uint8_t *source;
        uint32_t *mask;
        paddr_t *address;
        uint8_t *data;
        Usr *usr;
        Echo *echo;
        uint8_t *corrupt;
        uint8_t *alias;
    };

    class ChnB : public Decoupled {
    public:
        uint8_t *opcode;
        uint8_t *param;
        uint8_t *size;
        uint8_t *source;
        paddr_t *address;
        uint8_t *corrupt;
        uint8_t *alias;
        uint8_t *needdata;
    };

    template<class Usr, class Echo, std::size_t N>
    class ChnC : public Decoupled {
    public:
        uint8_t *opcode;
        uint8_t *param;
        uint8_t *size;
        uint8_t *source;
        paddr_t *address;
        Usr *usr;
        Echo *echo;
        uint8_t *dirty;
        uint8_t *data;
        uint8_t *corrupt;
        uint8_t *alias;
    };

    template<class Usr, class Echo, std::size_t N>
    class ChnD : public Decoupled {
    public:
        uint8_t *opcode;
        uint8_t *param;
        uint8_t *size;
        uint8_t *source;
        sink_t *sink;
        uint8_t *denied;
        Usr *usr;
        Echo *echo;
        uint8_t *dirty;
        uint8_t *data;
        uint8_t *corrupt;
    };

    class ChnE : public Decoupled {
    public:
        sink_t *sink;
        paddr_t *addr;  // used for index scoreboard
        uint8_t *alias;
    };

    template<class ReqField, class RespField, class EchoField, std::size_t N>
    class Port {
    public:
        ChnA<ReqField, EchoField, N> a;
        ChnB b;
        ChnC<ReqField, EchoField, N> c;
        ChnD<RespField, EchoField, N> d;
        ChnE e;
    };

}

#endif //TLC_TEST_PORT_H
