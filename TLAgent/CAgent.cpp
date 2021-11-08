//
// Created by wkf on 2021/11/2.
//

#include "CAgent.h"

namespace tl_agent {

    int capGenPriv(int param) {
        switch (param) {
            case toT: return TIP;
            case toB: return BRANCH;
            case toN: return INVALID;
            default:
                tlc_assert(false, "Invalid param!");
        }
    }

    int shrinkGenPriv(int param) {
        switch (param) {
            case TtoB: return BRANCH;
            case TtoN: return INVALID;
            case BtoN: return INVALID;
            default:
                tlc_assert(false, "Invalid param!");
        }
    }

    CAgent::CAgent(GlobalBoard<paddr_t> *const gb, uint64_t *cycles):
        BaseAgent(), pendingA(), pendingC(), pendingD(), pendingE()
    {
        this->globalBoard = gb;
        this->cycles = cycles;
        this->localBoard = new ScoreBoard<paddr_t , C_SBEntry>();
        this->idMap = new ScoreBoard<int, C_IDEntry>();
    }

    Resp CAgent::send_a(std::shared_ptr<ChnA<ReqField, EchoField, DATASIZE>> &a) {
        switch (*a->opcode) {
            case AcquireBlock: {
                std::shared_ptr<C_IDEntry> idmap_entry(new C_IDEntry(*a->address));
                idMap->update(*a->source, idmap_entry);

                if (localBoard->haskey(*a->address)) {
                    localBoard->query(*a->address)->update_status(S_SENDING_A, *cycles);
                } else {
                    std::shared_ptr<C_SBEntry> entry(new C_SBEntry(S_SENDING_A, INVALID, *cycles)); // Set pending as INVALID
                    localBoard->update(*a->address, entry);
                }
                break;
            }
            default:
                tlc_assert(false, "Unknown opcode for channel A!");
        }
        *this->port->a.opcode = *a->opcode;
        *this->port->a.address = *a->address;
        *this->port->a.size = *a->size;
        *this->port->a.mask = *a->mask;
        *this->port->a.source = *a->source;
        *this->port->a.valid = true;
        return OK;
    }

    void CAgent::handle_b() {

    }

    Resp CAgent::send_c(std::shared_ptr<ChnC<ReqField, EchoField, DATASIZE>> &c) {
        switch (*c->opcode) {
            case ReleaseData: {
                std::shared_ptr<C_IDEntry> idmap_entry(new C_IDEntry(*c->address));
                idMap->update(*c->source, idmap_entry);

                if (localBoard->haskey(*c->address)) {
                    localBoard->query(*c->address)->update_status(S_SENDING_A, *cycles);
                } else {
                    tlc_assert(false, "Localboard key not found!");
                }
                int beat_num = pendingC.nr_beat - pendingC.beat_cnt;
                for (int i = BEATSIZE * beat_num; i < BEATSIZE * (beat_num + 1); i++) {
                    this->port->c.data[i - BEATSIZE * beat_num] = c->data[i];
                }
                break;
            }
            default:
                tlc_assert(false, "Unknown opcode for channel C!");
        }
        *this->port->c.opcode = *c->opcode;
        *this->port->c.address = *c->address;
        *this->port->c.size = *c->size;
        *this->port->c.source = *c->source;
        *this->port->c.valid = true;
        return OK;
    }

    void CAgent::handle_d() {

    }

    Resp CAgent::send_e(std::shared_ptr<ChnE> &e) {
        *this->port->e.sink = *e->sink;
        *this->port->e.valid = true;
        return OK;
    }

    void CAgent::fire_a() {
        if (this->port->a.fire()) {
            auto chnA = this->port->a;
            *chnA.valid = false;
            tlc_assert(pendingA.is_pending(), "No pending A but A fired!");
            pendingA.update();
            if (!pendingA.is_pending()) { // req A finished
                this->localBoard->query(*pendingA.info->address)->update_status(S_WAITING_D, *cycles);
            }
        }
    }

    void CAgent::fire_b() {

    }

    void CAgent::fire_c() {
        if (this->port->c.fire()) {
            auto chnC = this->port->c;
            bool hasData = *chnC.opcode == ReleaseData;
            // TODO: consider when to disable chnC
            *chnC.valid = false;
            tlc_assert(pendingC.is_pending(), "No pending C but C fired!");
            pendingC.update();
            if (!pendingC.is_pending()) { // req C finished
                auto info = this->localBoard->query(*pendingC.info->address);
                info->update_status(S_WAITING_D, *cycles);
                if (hasData) {
                    std::shared_ptr<Global_SBEntry> global_SBEntry(new Global_SBEntry());
                    global_SBEntry->pending_data = pendingC.info->data;
                    if (this->globalBoard->get().count(*pendingC.info->address) == 0) {
                        global_SBEntry->data = nullptr;
                    } else {
                        global_SBEntry->data = this->globalBoard->get()[*pendingC.info->address]->data;
                    }
                    global_SBEntry->status = Global_SBEntry::SB_PENDING;
                    this->globalBoard->update(*pendingC.info->address, global_SBEntry);
                }
                if (*chnC.opcode == ReleaseData || *chnC.opcode == Release) {
                    info->update_pending_priviledge(shrinkGenPriv(*pendingC.info->param), *cycles);
                }
            }
        }
    }

    void CAgent::fire_d() {
        if (this->port->d.fire()) {
            auto chnD = this->port->d;
            bool hasData = *chnD.opcode == GrantData;
            auto addr = idMap->query(*chnD.source)->address;
            auto info = localBoard->query(addr);
            tlc_assert(info->status == S_WAITING_D, "Status error!");
            if (pendingD.is_pending()) { // following beats
                tlc_assert(*chnD.opcode == *pendingD.info->opcode, "Opcode mismatch among beats!");
                tlc_assert(*chnD.param == *pendingD.info->param, "Param mismatch among beats!");
                tlc_assert(*chnD.source == *pendingD.info->source, "Source mismatch among beats!");
                pendingD.update();
            } else { // new D resp
                std::shared_ptr<ChnD<RespField, EchoField, DATASIZE>> resp_d(new ChnD<RespField, EchoField, DATASIZE>());
                resp_d->opcode = new uint8_t(*chnD.opcode);
                resp_d->param = new uint8_t(*chnD.param);
                resp_d->source = new uint8_t(*chnD.source);
                resp_d->data = hasData ? new uint8_t[DATASIZE] : nullptr;
                int nr_beat = (*chnD.opcode == Grant || *chnD.opcode == ReleaseAck) ? 0 : 1; // TODO: parameterize it
                pendingD.init(resp_d, nr_beat);
            }
            if (hasData) {
                int beat_num = pendingD.nr_beat - pendingD.beat_cnt;
                for (int i = BEATSIZE * beat_num; i < BEATSIZE * (beat_num + 1); i++) {
                    pendingD.info->data[i] = chnD.data[i - BEATSIZE * beat_num];
                }
            }
            if (!pendingD.is_pending()) {
                if (hasData) {
                    Log("[GrantData] addr: %hx data: ", addr);
                    for(int i = 0; i < DATASIZE; i++) {
                        Log("%02hhx", pendingD.info->data[i]);
                    }
                    Log("\n");
                    this->globalBoard->verify(addr, pendingD.info->data);
                }
                if (*chnD.opcode == GrantData || *chnD.opcode == Grant) {
                    std::shared_ptr<ChnE> req_e(new ChnE());
                    req_e->sink = new uint8_t(*chnD.sink);
                    req_e->addr = new paddr_t(addr);
                    pendingE.init(req_e, 1);
                    info->update_status(S_SENDING_E, *cycles);
                    info->update_priviledge(capGenPriv(*chnD.param), *cycles);
                }
                if (*chnD.opcode == ReleaseAck) {
                    Log("[ReleaseAck] addr: %hx\n", addr);
                    info->update_status(S_VALID, *cycles);
                    info->unpending_priviledge(*cycles);
                }
                idMap->erase(*chnD.source);
                this->idpool.freeid(*chnD.source);
            }
        }
    }

    void CAgent::fire_e() {
        if (this->port->e.fire()) {
            auto chnE = this->port->e;
            *chnE.valid = false;
            tlc_assert(pendingE.is_pending(), "No pending A but A fired!");
            auto info = localBoard->query(*pendingE.info->addr);
            info->update_status(S_VALID, *cycles);
            pendingE.update();
        }
    }

    void CAgent::handle_channel() {
        fire_a();
        fire_c();
        fire_d();
        fire_e();
    }

    void CAgent::update_signal() {
        *this->port->d.ready = true; // TODO: do random here
        if (pendingA.is_pending()) {
            // TODO: do delay here
            send_a(pendingA.info);
        } else {
            // TODO: is this necessary?
            *this->port->a.valid = false;
        }
        if (pendingC.is_pending()) {
            send_c(pendingC.info);
        } else {
            // TODO: is this necessary?
            *this->port->c.valid = false;
        }
        if (pendingE.is_pending()) {
            send_e(pendingE.info);
        }
        // do timeout check lazily
        if (*this->cycles % TIMEOUT_INTERVAL == 0) {
            this->timeout_check();
        }
        idpool.update();
    }

    bool CAgent::do_acquireBlock(paddr_t address, int param) {
        if (pendingA.is_pending() || idpool.full())
            return false;
        if (localBoard->haskey(address)) { // check whether this transaction is legal
            auto entry = localBoard->query(address);
            auto privilege = entry->privilege;
            auto status = entry->status;
            if (status != S_VALID && status != S_INVALID) {
                return false;
            }
            if (privilege == TIP) return false;
            if (privilege == BRANCH && param != BtoT) return false;
            if (privilege == INVALID && param == BtoT) return false;
        }
        std::shared_ptr<ChnA<ReqField, EchoField, DATASIZE>> req_a(new ChnA<ReqField, EchoField, DATASIZE>());
        req_a->opcode = new uint8_t(AcquireBlock);
        req_a->address = new paddr_t(address);
        req_a->param = new uint8_t(param);
        req_a->size = new uint8_t(ceil(log2((double)DATASIZE)));
        req_a->mask = new uint32_t(0xffffffffUL);
        req_a->source = new uint8_t(this->idpool.getid());
        pendingA.init(req_a, 1);
        Log("[AcquireData] addr: %x\n", address);
        return true;
    }

    bool CAgent::do_releaseData(paddr_t address, int param, uint8_t data[]) {
        if (pendingC.is_pending() || idpool.full() || !localBoard->haskey(address))
            return false;
        // TODO: checkout pendingA
        auto entry = localBoard->query(address);
        auto privilege = entry->privilege;
        auto status = entry->status;
        if (status != S_VALID && status != S_INVALID) {
            return false;
        }
        if (privilege == INVALID) return false;
        if (privilege == BRANCH && param != BtoN) return false;
        if (privilege == TIP && param == BtoN) return false;

        std::shared_ptr<ChnC<ReqField, EchoField, DATASIZE>> req_c(new ChnC<ReqField, EchoField, DATASIZE>());
        req_c->opcode = new uint8_t(ReleaseData);
        req_c->address = new paddr_t(address);
        req_c->param = new uint8_t(param);
        req_c->size = new uint8_t(ceil(log2((double)DATASIZE)));
        req_c->source = new uint8_t(this->idpool.getid());
        req_c->data = data;
        pendingC.init(req_c, DATASIZE / BEATSIZE);
        Log("[ReleaseData] addr: %x data: ", address);
        for(int i = 0; i < DATASIZE; i++) {
            Log("%02hhx", data[i]);
        }
        Log("\n");
        return true;
    }

    void CAgent::timeout_check() {
        if (localBoard->get().empty()) {
            return;
        }
        for (auto it = this->localBoard->get().begin(); it != this->localBoard->get().end(); it++) {
            auto value = it->second;
            if (value->status != S_INVALID && value->status != S_VALID) {
                if (*this->cycles - value->time_stamp > TIMEOUT_INTERVAL) {
                    printf("Now time:   %lu\n", *this->cycles);
                    printf("Last stamp: %lu\n", value->time_stamp);
                    printf("Status:     %d\n", value->status);
                    tlc_assert(false,  "Transaction time out");
                }
            }
        }
    }
}