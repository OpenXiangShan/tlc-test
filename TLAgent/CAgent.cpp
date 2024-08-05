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
            case TtoT: return TIP;
            case BtoB:
            case TtoB: return BRANCH;
            case TtoN:
            case BtoN:
            case NtoN: return INVALID;
            default:
                tlc_assert(false, "Invalid param!");
        }
    }

    CAgent::CAgent(GlobalBoard<paddr_t> *const gb, int id, uint64_t *cycles):
        BaseAgent(), pendingA(), pendingB(), pendingC(), pendingD(), pendingE(), pendingCMOReq(), probeIDpool(NR_SOURCEID, NR_SOURCEID+1)
    {
        this->globalBoard = gb;
        this->id = id;
        this->cycles = cycles;
        this->localBoard = new ScoreBoard<paddr_t , C_SBEntry>();
        this->idMap = new ScoreBoard<int, C_IDEntry>();
    }

    Resp CAgent::send_a(std::shared_ptr<ChnA<ReqField, EchoField, DATASIZE>> &a) {
        switch (*a->opcode) {
            case AcquireBlock: {
                std::shared_ptr<C_IDEntry> idmap_entry(new C_IDEntry(*a->address, *a->alias));
                idMap->update(*a->source, idmap_entry);

                if (localBoard->haskey(*a->address)) {
                    localBoard->query(*a->address)->update_status(S_SENDING_A, *cycles, *a->alias);
                } else {
                    int statuses[4] = {S_INVALID};
                    int privileges[4] = {INVALID};
                    for (int i = 0; i < 4; i++) {
                        if (*a->alias == i) {
                            statuses[i] = S_SENDING_A;
                        }
                    }
                    std::shared_ptr<C_SBEntry> entry(new C_SBEntry(statuses, privileges, *cycles)); // Set pending as INVALID
                    localBoard->update(*a->address, entry);
                }
                break;
            }
            case AcquirePerm: {
                std::shared_ptr<C_IDEntry> idmap_entry(new C_IDEntry(*a->address, *a->alias));
                idMap->update(*a->source, idmap_entry);
                if (localBoard->haskey(*a->address)) {
                    localBoard->query(*a->address)->update_status(S_SENDING_A, *cycles, *a->alias);
                } else {
                    int statuses[4] = {S_INVALID};
                    int privileges[4] = {INVALID};
                    for (int i = 0; i < 4; i++) {
                        if (*a->alias == i) {
                          statuses[i] = S_SENDING_A;
                        }
                    }
                    std::shared_ptr<C_SBEntry> entry(new C_SBEntry(statuses, privileges, *cycles)); // Set pending as INVALID
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
        *this->port->a.param = *a->param;
        *this->port->a.mask = *a->mask;
        *this->port->a.source = *a->source;
        *this->port->a.alias = *a->alias;
        *this->port->a.valid = true;
        return OK;
    }

    void CAgent::handle_b(std::shared_ptr<ChnB> &b) {
        if (pendingC.is_pending()) {
            Log("[info] B wanna pendingC\n");
            return;
        }
        if (this->probeIDpool.full()) {
            Log("[info] id pool full\n");
            return;
        }

        tlc_assert(localBoard->haskey(*b->address), "Probe an non-exist block!");

        auto info = localBoard->query(*b->address);
        // auto exact_status = info->status[*b->alias];
        int exact_status[4];
        for (int i = 0; i < 4; i++){
            exact_status[i] = info->status[i];
        }
        auto exact_privilege = info->privilege[*b->alias];
        tlc_assert(exact_status[*b->alias] != S_SENDING_C, "handle_b should be mutual exclusive with pendingC!");
        // do not accept probe when there is a release with the same addr waiting for releaseAck 
        for (int i = 0; i < 4; i++){
            if (exact_status[i] == S_C_WAITING_D) {
                // Probe waits for releaseAck
                return;
            }
        }
        std::shared_ptr<ChnC<ReqField, EchoField, DATASIZE>> req_c(new ChnC<ReqField, EchoField, DATASIZE>());
        req_c->address = new paddr_t(*b->address);
        req_c->size = new uint8_t(*b->size);
        req_c->source = new uint8_t(this->probeIDpool.getid());
        req_c->dirty = new uint8_t(1);
        req_c->alias = new uint8_t(*b->alias);
        // Log("== id == handleB %d\n", *req_c->source);
        if (exact_status[*b->alias] == S_SENDING_A || exact_status[*b->alias] == S_INVALID || exact_status[*b->alias] == S_A_WAITING_D) {
            Log("Probe an non-exist block, status: %d\n", exact_status[*b->alias]);
            req_c->opcode = new uint8_t(ProbeAck);
            req_c->param = new uint8_t(NtoN);
            pendingC.init(req_c, 1);
            Log("[%ld] [ProbeAck NtoN] addr: %x alias: %x\n", *cycles, *b->address, *b->alias);
        } else {
            int dirty = (exact_privilege == TIP) && (info->dirty[*b->alias] || rand() % 3);
            // When should we probeAck with data? request need_data or dirty itself
            req_c->opcode = (dirty || *b->needdata) ? new uint8_t(ProbeAckData) : new uint8_t(ProbeAck);
            if (*b->param == toB) {
                switch (exact_privilege) {
                    case TIP:    req_c->param = new uint8_t(TtoB); break;
                    case BRANCH: req_c->param = new uint8_t(BtoB); break;
                    default: tlc_assert(false, "Try to probe toB an invalid block!");
                }
            } else if (*b->param == toN) {
                switch (exact_privilege) {
                    case TIP:    req_c->param = new uint8_t(TtoN); break;
                    case BRANCH: req_c->param = new uint8_t(BtoN); break;
                    default: tlc_assert(false, "Try to probe toB an invalid block!");
                }
            }
            if (!globalBoard->haskey(*b->address)) {
                // want to probe an all-zero block which does not exist in global board
                Log("probeAck Data all-zero\n");
                uint8_t *all_zero = new uint8_t[DATASIZE];
                for (int i = 0; i < DATASIZE; i++) {
                    all_zero[i] = 0;
                }
                req_c->data = all_zero;
            } else {
                if (*req_c->opcode == ProbeAckData && (*req_c->param == TtoN || *req_c->param == TtoB || *req_c->param == TtoT)) {
                    uint8_t *random = new uint8_t[DATASIZE];
                    for (int i = 0; i < DATASIZE; i++) {
                      random[i] = (uint8_t)rand();
                    }
                    req_c->data = random;
                } else {
                    req_c->data = globalBoard->query(*b->address)->data;
                }
            }
            if (*req_c->opcode == ProbeAckData) {
                pendingC.init(req_c, DATASIZE / BEATSIZE);
            } else {
                pendingC.init(req_c, 1);
            }

            if (*req_c->param == TtoN) {
                Log("[%ld] [ProbeAck TtoN] addr: %x alias: %x data: ", *cycles, *b->address, *b->alias);
            } else if (*req_c->param == TtoB) {
                Log("[%ld] [ProbeAck TtoB] addr: %x alias: %x data: ", *cycles, *b->address, *b->alias);
            } else if (*req_c->param == NtoN) {
                Log("[%ld] [ProbeAck NtoN] addr: %x alias: %x data: ", *cycles, *b->address, *b->alias);
            } else if (*req_c->param == BtoN) {
                Log("[%ld] [ProbeAck BtoN] addr: %x alias: %x data: ", *cycles, *b->address, *b->alias);
            } else if (*req_c->param == BtoB) {
                Log("[%ld] [ProbeAck BtoB] addr: %x alias: %x data: ", *cycles, *b->address, *b->alias);
            } else {
                tlc_assert(false, "What the hell is req_c's param?");
            }
            if (*req_c->opcode == ProbeAckData) {
                for (int i = 0; i < DATASIZE; i++) {
                  Dump("%02hhx", req_c->data[i]);
                }
            } else {
              Dump("no data");
            }
            Dump("\n");
        }
        pendingB.update();
    }

    Resp CAgent::send_c(std::shared_ptr<ChnC<ReqField, EchoField, DATASIZE>> &c) {
        switch (*c->opcode) {
            case ReleaseData: {
                std::shared_ptr<C_IDEntry> idmap_entry(new C_IDEntry(*c->address, *c->alias));
                idMap->update(*c->source, idmap_entry);

                if (localBoard->haskey(*c->address)) {
                    localBoard->query(*c->address)->update_status(S_SENDING_C, *cycles, *c->alias);
                } else {
                    tlc_assert(false, "Localboard key not found!");
                }
                int beat_num = pendingC.nr_beat - pendingC.beat_cnt;
                for (int i = BEATSIZE * beat_num; i < BEATSIZE * (beat_num + 1); i++) {
                    this->port->c.data[i - BEATSIZE * beat_num] = c->data[i];
                }
                break;
            }
            case ProbeAckData: {
                std::shared_ptr<C_IDEntry> idmap_entry(new C_IDEntry(*c->address, *c->alias));
                idMap->update(*c->source, idmap_entry);

                if (localBoard->haskey(*c->address)) {
                    // TODO: What if this is an interrupted probe?
                    localBoard->query(*c->address)->update_status(S_SENDING_C, *cycles, *c->alias);
                } else {
                    tlc_assert(false, "Localboard key not found!");
                }
                int beat_num = pendingC.nr_beat - pendingC.beat_cnt;
                for (int i = BEATSIZE * beat_num; i < BEATSIZE * (beat_num + 1); i++) {
                    this->port->c.data[i - BEATSIZE * beat_num] = c->data[i];
                }
                break;
            }
            case ProbeAck: {
                std::shared_ptr<C_IDEntry> idmap_entry(new C_IDEntry(*c->address, *c->alias));
                idMap->update(*c->source, idmap_entry);
                // tlc_assert(*c->param == NtoN, "Now probeAck only supports NtoN");
                if (localBoard->haskey(*c->address)) {
                    auto item = localBoard->query(*c->address);
                    if (item->status[*c->alias] == S_C_WAITING_D) {
                        item->update_status(S_C_WAITING_D_INTR, *cycles, *c->alias);
                    } else if (item->status[*c->alias] == S_A_WAITING_D) {
                        item->update_status(S_A_WAITING_D_INTR, *cycles, *c->alias);
                    } else {
                        item->update_status(S_SENDING_C, *cycles, *c->alias);
                    }
                } else {
                    tlc_assert(false, "Localboard key not found!");
                }
                break;
            }
            case Release: {
                std::shared_ptr<C_IDEntry> idmap_entry(new C_IDEntry(*c->address, *c->alias));
                idMap->update(*c->source, idmap_entry);

                if (localBoard->haskey(*c->address)) {
                    localBoard->query(*c->address)->update_status(S_SENDING_C, *cycles, *c->alias);
                } else {
                    tlc_assert(false, "Localboard key not found!");
                }
                break;
            }
            default:
                tlc_assert(false, "Unknown opcode for channel C!");
        }
        *this->port->c.opcode = *c->opcode;
        *this->port->c.param = *c->param;
        *this->port->c.address = *c->address;
        *this->port->c.size = *c->size;
        *this->port->c.source = *c->source;
        // *this->port->c.dirty = *c->dirty;
        *this->port->c.valid = true;
        return OK;
    }

    Resp CAgent::send_e(std::shared_ptr<ChnE> &e) {
        *this->port->e.sink = *e->sink;
        *this->port->e.valid = true;
        return OK;
    }

    void CAgent::fire_a() {
        if (this->port->a.fire()) {
            auto chnA = this->port->a;
            // Log("[%ld] [A fire] addr: %hx\n", *cycles, *chnA.address);
            *chnA.valid = false;
            tlc_assert(pendingA.is_pending(), "No pending A but A fired!");
            pendingA.update();
            if (!pendingA.is_pending()) { // req A finished
                this->localBoard->query(*pendingA.info->address)->update_status(S_A_WAITING_D, *cycles, *pendingA.info->alias);
            }
        }
    }

    void CAgent::fire_b() {
        if (this->port->b.fire()) {
            auto chnB = this->port->b;
            // Log("[%ld] [B fire] addr: %hx\n", *cycles, *chnB.address);
            std::shared_ptr<ChnB> req_b(new ChnB());
            req_b->opcode = new uint8_t(*chnB.opcode);
            req_b->address = new paddr_t(*chnB.address);
            req_b->param = new uint8_t(*chnB.param);
            req_b->size = new uint8_t(*chnB.size);
            req_b->source = new uint8_t(*chnB.source);
            req_b->alias = new uint8_t((*chnB.alias) >> 1);
            req_b->needdata = new uint8_t((*chnB.alias) & 0x1);
            pendingB.init(req_b, 1);
            Log("[%ld] [Probe] addr: %hx alias: %d\n", *cycles, *chnB.address, (*chnB.alias) >> 1);
        }
    }

    void CAgent::fire_c() {
        if (this->port->c.fire()) {
            auto chnC = this->port->c;
            bool releaseHasData = *chnC.opcode == ReleaseData;
            bool needAck = *chnC.opcode == ReleaseData || *chnC.opcode == Release;
            bool probeAckDataToB = *chnC.opcode == ProbeAckData && (*chnC.param == TtoB || *chnC.param == BtoB);
            tlc_assert(pendingC.is_pending(), "No pending C but C fired!");
            pendingC.update();
            if (!pendingC.is_pending()) { // req C finished
                *chnC.valid = false;
                // Log("[%ld] [C fire] addr: %hx opcode: %hx\n", *cycles, *chnC.address, *chnC.opcode);
                auto info = this->localBoard->query(*pendingC.info->address);
                auto exact_status = info->status[*pendingC.info->alias];
                if (needAck) {
                    info->update_status(S_C_WAITING_D, *cycles, *pendingC.info->alias);
                } else {
                    if (exact_status == S_C_WAITING_D_INTR) {
                        info->update_status(S_C_WAITING_D, *cycles, *pendingC.info->alias);
                    } else if (exact_status == S_A_WAITING_D_INTR || exact_status == S_A_WAITING_D) {
                        info->update_status(S_A_WAITING_D, *cycles, *pendingC.info->alias);
                    } else {
                        if (probeAckDataToB) {
                            info->update_status(S_VALID, *cycles, *pendingC.info->alias);
                        } else {
                            info->update_status(S_INVALID, *cycles, *pendingC.info->alias);
                        }
                    }
                }
                if (releaseHasData) {
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
                if (*chnC.opcode == ProbeAckData) {
                    std::shared_ptr<Global_SBEntry> global_SBEntry(new Global_SBEntry());
                    global_SBEntry->data = pendingC.info->data;
                    global_SBEntry->status = Global_SBEntry::SB_VALID;
                    this->globalBoard->update(*pendingC.info->address, global_SBEntry);
                }
                if (*chnC.opcode == ReleaseData || *chnC.opcode == Release) {
                    info->update_pending_priviledge(shrinkGenPriv(*pendingC.info->param), *cycles, *pendingC.info->alias);
                } else {
                    if (*chnC.opcode == ProbeAck || *chnC.opcode == ProbeAckData) {
                      info->update_priviledge(shrinkGenPriv(*pendingC.info->param), *cycles, *pendingC.info->alias);
                    }
                    // Log("== free == fireC %d\n", *chnC.source);
                    this->probeIDpool.freeid(*chnC.source);
                }

            }
        }
    }

    void CAgent::fire_d() {
        if (this->port->d.fire()) {
            auto chnD = this->port->d;
            bool hasData = *chnD.opcode == GrantData;
            bool grant = *chnD.opcode == GrantData || *chnD.opcode == Grant;
            auto addr = idMap->query(*chnD.source)->address;
            auto alias = idMap->query(*chnD.source)->alias;
            auto info = localBoard->query(addr);
            auto exact_status = info->status[alias];
            if (!(exact_status == S_C_WAITING_D || exact_status == S_A_WAITING_D || exact_status == S_C_WAITING_D_INTR || exact_status == S_A_WAITING_D_INTR || exact_status == S_INVALID)) {
              printf("fire_d: status of localboard is %d\n", exact_status);
              printf("addr: 0x%hx\n", addr);
              tlc_assert(false, "Status error!");
            }
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
                resp_d->data = grant ? new uint8_t[DATASIZE] : nullptr;
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
                switch (*chnD.opcode) {
                    case GrantData: {
                        Log("[%ld] [GrantData] addr: %hx alias: %hx data: ", *cycles, addr, alias);
                        for(int i = 0; i < DATASIZE; i++) {
                            Dump("%02hhx", pendingD.info->data[i]);
                        }
                        Dump("\n");
                        this->globalBoard->verify(addr, pendingD.info->data);
                        // info->update_dirty(*chnD.dirty, alias);
                        break;
                    }
                    case Grant: {
                        Log("[%ld] [Grant] addr: %hx alias: %hx\n", *cycles, addr, alias);
                        // Always set dirty in acquireperm txns
                        info->update_dirty(true, alias);
                        break;
                    }
                    case ReleaseAck: {
                        Log("[%ld] [ReleaseAck] addr: %hx alias: %hx\n", *cycles, addr, alias);
                        if (exact_status == S_C_WAITING_D) {
                            info->update_status(S_INVALID, *cycles, alias);
                            info->update_dirty(0, alias);
                        } else {
                            tlc_assert(exact_status == S_C_WAITING_D_INTR, "Status error!");
                            info->update_status(S_SENDING_C, *cycles, alias);
                        }
                        info->unpending_priviledge(*cycles, alias);
                        if(this->globalBoard->haskey(addr)) {
                            this->globalBoard->unpending(addr);  // ReleaseData
                        } 
                        break;
                    }
                    default:
                        tlc_assert(false, "Unknown opcode in channel D!");
                }

                // Send E
                if (grant) {
                    tlc_assert(exact_status != S_A_WAITING_D_INTR, "TODO: check this Ridiculous probe!");
                    std::shared_ptr<ChnE> req_e(new ChnE());
                    req_e->sink = new uint8_t(*chnD.sink);
                    req_e->addr = new paddr_t(addr);
                    req_e->alias = new uint8_t(alias);
                    if (pendingE.is_pending()) {
                        tlc_assert(false, "E is pending!");
                    }
                    pendingE.init(req_e, 1);
                    info->update_status(S_SENDING_E, *cycles, alias);
                    info->update_priviledge(capGenPriv(*chnD.param), *cycles, alias);
                }
                idMap->erase(*chnD.source);
                // Log("== free == fireD %d\n", *chnD.source);
                this->idpool.freeid(*chnD.source);
            }
        }
    }

    void CAgent::fire_e() {
        if (this->port->e.fire()) {
            auto chnE = this->port->e;
            *chnE.valid = false;
            tlc_assert(pendingE.is_pending(), "No pending A but E fired!");
            auto info = localBoard->query(*pendingE.info->addr);
            info->update_status(S_VALID, *cycles, *pendingE.info->alias);
            pendingE.update();
        }
    }

    void CAgent::handle_channel() {
        // Constraint: fire_e > fire_d, otherwise concurrent D/E requests will disturb the pendingE
        fire_a();
        fire_b();
        fire_c();
        fire_e();
        fire_d();
        fire_cmo_req();
    }

    void CAgent::update_signal() {
        *this->port->d.ready = true; // TODO: do random here
        *this->port->b.ready = !(pendingB.is_pending());

        if (pendingA.is_pending()) {
            // TODO: do delay here
            send_a(pendingA.info);
        }
        if (pendingB.is_pending()) {
            handle_b(pendingB.info);
        }
        if (pendingC.is_pending()) {
            send_c(pendingC.info);
        }
        if (pendingE.is_pending()) {
            send_e(pendingE.info);
        }
        if (pendingCMOReq.is_pending()) {
            send_cmo(pendingCMOReq.info);
        }
        // do timeout check lazily
        if (*this->cycles % TIMEOUT_INTERVAL == 0) {
            this->timeout_check();
        }
        idpool.update();
        probeIDpool.update();
    }

    TransResp CAgent::do_acquireBlock(paddr_t address, int param, int alias) {
        if (pendingA.is_pending() || pendingB.is_pending() || idpool.full())
            return PENDING;
        if (localBoard->haskey(address)) { // check whether this transaction is legal
            auto entry = localBoard->query(address);
            auto privilege = entry->privilege[alias];
            // auto status = entry->status[alias];
            int status[4];
            for(int i = 0; i < 4; i++) {
                status[i] = entry->status[i];
            }
            if (status[alias] != S_VALID && status[alias] != S_INVALID) {
                return PENDING; // in the process of other transaction
            }
            if (status[alias] == S_VALID) {
                if (privilege == TIP) return PASS;
                // if (privilege == BRANCH && param != BtoT) { param = BtoT; }
                if (privilege == BRANCH && param != BtoT) return PASS;
                if (privilege == INVALID && param == BtoT) return FAILURE;
            }
            for(int i = 0; i < 4; i++) {
                // do not send Release when there is an Acquire with the same physical address waiting for Grant
                if(status[i] == S_A_WAITING_D || status[i] == S_A_WAITING_D_INTR) {
                    return PENDING;
                }
            }
        }
        std::shared_ptr<ChnA<ReqField, EchoField, DATASIZE>> req_a(new ChnA<ReqField, EchoField, DATASIZE>());
        req_a->opcode = new uint8_t(AcquireBlock);
        req_a->address = new paddr_t(address);
        req_a->param = new uint8_t(param);
        req_a->size = new uint8_t(ceil(log2((double)DATASIZE)));
        req_a->mask = new uint32_t(0xffffffffUL);
        req_a->source = new uint8_t(this->idpool.getid());
        req_a->alias = new uint8_t(alias);
        // Log("== id == acquire %d\n", *req_a->source);
        pendingA.init(req_a, 1);
        switch (param) {
        case NtoB:
            Log("[%ld] [AcquireData NtoB] addr: %x alias: %d\n", *cycles, address, alias);
            break;
        case NtoT:
            Log("[%ld] [AcquireData NtoT] addr: %x alias: %d\n", *cycles, address, alias);
            break;
        }

        return SUCCESS;
    }

    TransResp CAgent::do_acquirePerm(paddr_t address, int param, int alias) {
        if (pendingA.is_pending() || pendingB.is_pending() || idpool.full())
            return PENDING;
        if (localBoard->haskey(address)) {
            auto entry = localBoard->query(address);
            auto privilege = entry->privilege[alias];
            // auto status = entry->status[alias];
            int status[4];
            for(int i = 0; i < 4; i++) {
                status[i] = entry->status[i];
            }
            if (status[alias] != S_VALID && status[alias] != S_INVALID) {
                return PENDING;
            }
            if (status[alias] == S_VALID) {
                if (privilege == TIP) return PASS;
                if (privilege == BRANCH && param != BtoT) { param = BtoT; }
                if (privilege == INVALID && param == BtoT) return FAILURE;
            }
            for(int i = 0; i < 4; i++) {
                // do not send AcquirePerm when there is an Acquire with the same physical address waiting for Grant
                if(status[i] == S_A_WAITING_D || status[i] == S_A_WAITING_D_INTR) {
                    return PENDING;
                }
            }
        }
        std::shared_ptr<ChnA<ReqField, EchoField, DATASIZE>> req_a(new ChnA<ReqField, EchoField, DATASIZE>());
        req_a->opcode = new uint8_t(AcquirePerm);
        req_a->address = new paddr_t(address);
        req_a->param = new uint8_t(param);
        req_a->size = new uint8_t(ceil(log2((double)DATASIZE)));
        req_a->mask = new uint32_t(0xffffffffUL);
        req_a->source = new uint8_t(this->idpool.getid());
        req_a->alias = new uint8_t(alias);
        // Log("== id == acquire %d\n", *req_a->source);
        pendingA.init(req_a, 1);
        Log("[%ld] [AcquirePerm] addr: %x alias: %d\n", *cycles, address, alias);
        return SUCCESS;
    }

    TransResp CAgent::do_releaseData(paddr_t address, int param, uint8_t data[], int alias) {
        if (pendingC.is_pending() || pendingB.is_pending() || idpool.full())
            return PENDING;
        if (!localBoard->haskey(address))
            return PASS;
        // TODO: checkout pendingA
        // TODO: checkout pendingB - give way?
        auto entry = localBoard->query(address);
        auto privilege = entry->privilege[alias];
        auto status = entry->status[alias];
        if (status != S_VALID) {
            return FAILURE;
        }
        if (privilege == INVALID) return FAILURE;
        if (privilege == BRANCH && param != BtoN) return FAILURE;
        if (privilege == TIP && param == BtoN) return FAILURE;

        std::shared_ptr<ChnC<ReqField, EchoField, DATASIZE>> req_c(new ChnC<ReqField, EchoField, DATASIZE>());
        req_c->opcode = new uint8_t(ReleaseData);
        req_c->address = new paddr_t(address);
        req_c->param = new uint8_t(param);
        req_c->size = new uint8_t(ceil(log2((double)DATASIZE)));
        req_c->source = new uint8_t(this->idpool.getid());
        req_c->dirty = new uint8_t(1);
        // Log("== id == release %d\n", *req_c->source);
        req_c->data = data;
        req_c->alias = new uint8_t(alias);
        pendingC.init(req_c, DATASIZE / BEATSIZE);
        Log("[%ld] [ReleaseData] addr: %x alias: %x data: ", *cycles, address, alias);
        for(int i = 0; i < DATASIZE; i++) {
            Dump("%02hhx", data[i]);
        }
        Dump("\n");
        return SUCCESS;
    }

    TransResp CAgent::do_releaseDataAuto(paddr_t address, int alias) {
        if (pendingC.is_pending() || pendingB.is_pending() || idpool.full())
            return PENDING;
        if (!localBoard->haskey(address))
            return PASS;
        // TODO: checkout pendingA
        // TODO: checkout pendingB - give way?
        auto entry = localBoard->query(address);
        auto privilege = entry->privilege[alias];
        int param;
        switch (privilege) {
        case INVALID:
            return FAILURE;
        case BRANCH:
            param = BtoN;
            break;
        case TIP:
            param = TtoN;
            break;
        default:
            tlc_assert(false, "Invalid priviledge detected!");
        }
        // auto status = entry->status[alias];
        int status[4];
        for(int i = 0; i < 4; i++) {
            status[i] = entry->status[i];
        }
        if (status[alias] != S_VALID) {
            return FAILURE;
        }
        for(int i = 0; i < 4; i++) {
         // do not send Release when there is an Acquire with the same physical address waiting for Grant
            if(status[i] == S_A_WAITING_D || status[i] == S_A_WAITING_D_INTR) {
                return PENDING;
            }
        }

        std::shared_ptr<ChnC<ReqField, EchoField, DATASIZE>> req_c(new ChnC<ReqField, EchoField, DATASIZE>());
        // if there is no entry[address] in globalBoard, send Release instead of ReleaseData
        if (!globalBoard->haskey(address) && param == BtoN) { 
            // Release
            req_c->opcode = new uint8_t(Release);
            req_c->address = new paddr_t(address);
            req_c->param = new uint8_t(param);
            req_c->size = new uint8_t(ceil(log2((double)DATASIZE)));
            req_c->source = new uint8_t(this->idpool.getid());
            req_c->dirty = new uint8_t(0);
            req_c->alias = new uint8_t(alias);
            req_c->data = 0;
            // printf("send Release, addr = %x, status = %d\n", address, status);
            pendingC.init(req_c, 1);
            switch (param) {
            case BtoN:
                Log("[%ld] [Release BtoN] addr: %x alias: %x data: ", *cycles, address, alias);
                break;
            case TtoN:
                Log("[%ld] [Release TtoN] addr: %x alias: %x data: ", *cycles, address, alias);
                break;
            }
            for(int i = 0; i < DATASIZE; i++) {
            Dump("%02hhx", 0);
            }
            Dump("\n");
        } else { 
            // ReleaseData
            req_c->opcode = new uint8_t(ReleaseData);
            req_c->address = new paddr_t(address);
            req_c->param = new uint8_t(param);
            req_c->size = new uint8_t(ceil(log2((double)DATASIZE)));
            req_c->source = new uint8_t(this->idpool.getid());
            req_c->dirty = new uint8_t(1);
            req_c->alias = new uint8_t(alias);
            if (param == BtoN) {
                uint8_t* data = globalBoard->query(address)->data;
                req_c->data = data;
            } else {
                tlc_assert(param == TtoN, "Wrong execution path!");
                uint8_t* putdata = new uint8_t[DATASIZE];
                for (int i = 0; i < DATASIZE; i++) {
                    putdata[i] = (uint8_t)rand();
                }
            req_c->data = putdata;
            }
            pendingC.init(req_c, DATASIZE / BEATSIZE);
            switch (param) {
            case BtoN:
                Log("[%ld] [ReleaseData BtoN] addr: %x alias: %x data: ", *cycles, address, alias);
                break;
            case TtoN:
                Log("[%ld] [ReleaseData TtoN] addr: %x alias: %x data: ", *cycles, address, alias);
                break;
            }
            for(int i = 0; i < DATASIZE; i++) {
                Dump("%02hhx", req_c->data[i]);
            }
            Dump("\n");
        }

        return SUCCESS;
    }

    void CAgent::timeout_check() {
        if (localBoard->get().empty()) {
            return;
        }
        for (auto it = this->localBoard->get().begin(); it != this->localBoard->get().end(); it++) {
            auto addr = it->first;
            auto value = it->second;
            for(int i = 0; i < 4; i++){
              if (value->status[i] != S_INVALID && value->status[i] != S_VALID) {
                if (*this->cycles - value->time_stamp > TIMEOUT_INTERVAL) {
                  printf("addr: %lx\n", addr);
                  printf("Now time:   %lu\n", *this->cycles);
                  printf("Last stamp: %lu\n", value->time_stamp);
                  printf("Status[%d]:  %d\n", i, value->status[i]);
                  tlc_assert(false,  "Transaction time out");
                }
              }
            }
        }
    }

    Resp CAgent::send_cmo(std::shared_ptr<CMOReq> &cmo_req){
        switch (*cmo_req->opcode) {
            case Clean: {
                if (localBoard->haskey(*cmo_req->address)) {
                    localBoard->query(*cmo_req->address)->update_status(S_SENDING_CMO, *cycles, 0);
                } else {
                    int statuses[4] = {S_INVALID};
                    int privileges[4] = {INVALID};
                    statuses[0] = S_SENDING_CMO;
                    std::shared_ptr<C_SBEntry> entry(new C_SBEntry(statuses, privileges, *cycles)); // Set pending as INVALID
                    localBoard->update(*cmo_req->address, entry);
                }
                break;
            }
            case Flush: {
                if (localBoard->haskey(*cmo_req->address)) {
                    localBoard->query(*cmo_req->address)->update_status(S_SENDING_CMO, *cycles, 0);
                } else {
                    int statuses[4] = {S_INVALID};
                    int privileges[4] = {INVALID};
                    statuses[0] = S_SENDING_CMO;
                    std::shared_ptr<C_SBEntry> entry(new C_SBEntry(statuses, privileges, *cycles)); // Set pending as INVALID
                    localBoard->update(*cmo_req->address, entry);
                }
                break;
            }
            case Inval: {
                if (localBoard->haskey(*cmo_req->address)) {
                    localBoard->query(*cmo_req->address)->update_status(S_SENDING_CMO, *cycles, 0);
                } else {
                    int statuses[4] = {S_INVALID};
                    int privileges[4] = {INVALID};
                    statuses[0] = S_SENDING_CMO;
                    std::shared_ptr<C_SBEntry> entry(new C_SBEntry(statuses, privileges, *cycles)); // Set pending as INVALID
                    localBoard->update(*cmo_req->address, entry);
                }
                break;
            }
            default:
                tlc_assert(false, "Unknown opcode for channel CMO!");
        }
        *this->port->cmo_req.opcode = *cmo_req->opcode;
        *this->port->cmo_req.address = *cmo_req->address;
        *this->port->cmo_req.valid = true;
        return OK;
    }
    
    void CAgent::fire_cmo_req(){
        if (this->port->cmo_req.fire()) {
            auto CMOreq = this->port->cmo_req;
            *CMOreq.valid = false;
            pendingCMOReq.update();
            auto info = localBoard->query(*pendingCMOReq.info->address);
            info->update_status(S_VALID, *cycles, 0);  // TODO: add status S_CMO_WAITING_RESP
        }
    }

    TransResp CAgent::do_Invalidate(paddr_t address){
        if (localBoard->haskey(address)) { // check whether this transaction is legal
            auto entry = localBoard->query(address);
            auto privilege = entry->privilege[0];
            int status[4];
            for(int i = 0; i < 4; i++) {
                status[i] = entry->status[i];
            }
            if (status[0] != S_VALID && status[0] != S_INVALID) {
                return PENDING; // in the process of other transaction
            }
        }
        std::shared_ptr<CMOReq> req_cmo(new CMOReq());
        req_cmo->opcode = new uint8_t(Inval);
        req_cmo->address = new paddr_t(address);
        pendingCMOReq.init(req_cmo, 1);
        Log("[%ld] [Invalidate] addr: %x \n", *cycles, address);

        return SUCCESS;
    }
}
