//
// Created by ljw on 10/21/21.
//

#ifndef TLC_TEST_BASEAGENT_H
#define TLC_TEST_BASEAGENT_H

#include <set>
#include "Port.h"
#include "../Utils/Common.h"
#include "../Utils/ScoreBoard.h"

namespace tl_agent {

    enum Resp {OK, FAIL};

    // return value for `do_acquireBlock`
    // 0 for successful sending
    // 1 for unable to send now (congestion)
    // 2 for failure due to action not allowed
    // 3 for permission already satisfied (no need to send Acquire)
    enum TransResp {SUCCESS, PENDING, FAILURE, PASS};

    enum {
        S_INVALID = 0,
        S_VALID,
        S_SENDING_A,        // ready to send A request actively
        S_REACTING_B,       // ready to react B request actively
        S_SENDING_C,        // ready to send C request actively
        S_C_WAITING_D,      // C wait for D response
        S_A_WAITING_D,      // A wait for D response
        S_C_WAITING_D_INTR, // C wait for D response while probe interrupted
        S_A_WAITING_D_INTR, // A wait for D response while probe interrupted
        S_SENDING_E,        // ready to send E request actively
        S_SENDING_CMO
    };

    class ReqField {
    public:
        uint8_t value;
    };

    class RespField {
    public:
        uint8_t value;
    };

    class EchoField {
    public:
        uint8_t value;
    };

    template<typename T>
    class PendingTrans {
    public:
        int beat_cnt;
        int nr_beat;
        std::shared_ptr<T> info;

        PendingTrans() {
            nr_beat = 0;
            beat_cnt = 0;
        }
        ~PendingTrans() = default;

        bool is_multiBeat() { return (this->nr_beat != 1); };
        bool is_pending() { return (beat_cnt != 0); }
        void init(std::shared_ptr<T> &info, int nr_beat) {
            this->info = info;
            this->nr_beat = nr_beat;
            beat_cnt = nr_beat;
        }
        void update() {
            beat_cnt--;
            tlc_assert(beat_cnt >= 0, "More beats received than expected!");
        }
    };

    class IDPool {
    private:
        std::set<int> *idle_ids;
        std::set<int> *used_ids;
        int pending_freeid;
    public:
        IDPool(int start, int end) {
            idle_ids = new std::set<int>();
            used_ids = new std::set<int>();
            for (int i = start; i < end; i++) {
                idle_ids->insert(i);
            }
            used_ids->clear();
            pending_freeid = -1;
        }
        ~IDPool() {
            delete idle_ids;
            delete used_ids;
        }
        int getid() {
            if (idle_ids->size() == 0)
                return -1;
            int ret = *idle_ids->begin();
            used_ids->insert(ret);
            idle_ids->erase(ret);
            return ret;
        }
        void freeid(int id) {
            this->pending_freeid = id;
        }
        void update() {
            if (pending_freeid != -1) {
                tlc_assert(used_ids->count(pending_freeid) > 0, "Try to free unused SourceID!");
                used_ids->erase(pending_freeid);
                idle_ids->insert(pending_freeid);
                pending_freeid = -1;
            }
        }
        bool full() {
            return idle_ids->empty();
        }
    };

    class BaseAgent {
    protected:
        Port<ReqField, RespField, EchoField, BEATSIZE> *port;
        GlobalBoard<paddr_t> *globalBoard;
        IDPool idpool;
        virtual void timeout_check() = 0;
        int id;

    public:
        virtual Resp send_a(std::shared_ptr<ChnA<ReqField, EchoField, DATASIZE>> &a) = 0;
        virtual void handle_b(std::shared_ptr<ChnB> &b) = 0;
        virtual Resp send_c(std::shared_ptr<ChnC<ReqField, EchoField, DATASIZE>> &c) = 0;
        virtual void fire_a() = 0;
        virtual void fire_b() = 0;
        virtual void fire_c() = 0;
        virtual void fire_d() = 0;
        virtual void fire_e() = 0;
        virtual void handle_channel() = 0;
        virtual void update_signal() = 0;
        BaseAgent(): idpool(0, NR_SOURCEID) {};
        virtual ~BaseAgent() = default;

        void connect(Port<ReqField, RespField, EchoField, BEATSIZE> *p){ this->port = p; }
    };

}

#endif //TLC_TEST_BASEAGENT_H
