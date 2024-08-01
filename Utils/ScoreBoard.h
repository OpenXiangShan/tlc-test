//
// Created by Kaifan Wang on 2021/10/25.
//

#include <map>
#include <array>
#include "Common.h"

#ifndef TLC_TEST_SCOREBOARD_H
#define TLC_TEST_SCOREBOARD_H

const int ERR_NOTFOUND = 1;
const int ERR_MISMATCH = 2;

template<typename Tk, typename Tv>
class ScoreBoard {
protected:
    std::map<Tk, std::shared_ptr<Tv>> mapping;
public:
    ScoreBoard();
    ~ScoreBoard();
    std::map<Tk, std::shared_ptr<Tv>>& get();
    void update(const Tk& key, std::shared_ptr<Tv>& data);
    std::shared_ptr<Tv> query(const Tk& key);
    void erase(const Tk& key);
    int verify(const Tk& key, const Tv& data) const;
    bool haskey(const Tk& key);
};

class Global_SBEntry {
public:
    enum {
        SB_INVALID = 0,
        SB_VALID,
        SB_PENDING
    };
    int status;
    uint8_t* data;
    uint8_t* pending_data; // used for put&release
};

template<typename T>
class GlobalBoard : public ScoreBoard<T, Global_SBEntry> {
private:
    int data_check(const uint8_t* dut, const uint8_t* ref, std::string assert_info);
    uint8_t init_zeros[DATASIZE];
public:
    int verify(const T& key, const uint8_t* data);
    void unpending(const T& key);
#ifdef NOCHECK
    std::map<T, std::shared_ptr<Global_SBEntry>>& get();
    void update(const T& key, std::shared_ptr<Global_SBEntry>& data);
    std::shared_ptr<Global_SBEntry> query(const T& key);
    void erase(const T& key);
    bool haskey(const T& key);
#endif
};

/************************** Implementation **************************/

template<typename Tk, typename Tv>
ScoreBoard<Tk, Tv>::ScoreBoard() {
    mapping.clear();
}

template<typename Tk, typename Tv>
ScoreBoard<Tk, Tv>::~ScoreBoard() {
}

template<typename Tk, typename Tv>
std::map<Tk, std::shared_ptr<Tv>>& ScoreBoard<Tk, Tv>::get() {
    return this->mapping;
}

template<typename Tk, typename Tv>
void ScoreBoard<Tk, Tv>::update(const Tk& key, std::shared_ptr<Tv>& data) {
    if (mapping.count(key) != 0) {
        mapping[key]= data;
    } else {
        mapping.insert(std::make_pair(key, data));
    }
}

template<typename Tk, typename Tv>
std::shared_ptr<Tv> ScoreBoard<Tk, Tv>::query(const Tk& key) {
    if (mapping.count(key) > 0) {
        return mapping[key];
    } else {
        tlc_assert(false, "Key no found!");
    }
}

template<typename Tk, typename Tv>
void ScoreBoard<Tk, Tv>::erase(const Tk& key) {
    int num = mapping.erase(key);
    tlc_assert(num == 1, "Multiple value mapped to one key!");
}

template<typename Tk, typename Tv>
int ScoreBoard<Tk, Tv>::verify(const Tk& key, const Tv& data) const {
    if (mapping.count(key) > 0) {
        if (*mapping[key] != data) {
            return ERR_MISMATCH;
        }
        return 0;
    }
    return ERR_NOTFOUND;
}

template<typename Tk, typename Tv>
bool ScoreBoard<Tk, Tv>::haskey(const Tk &key) {
    return mapping.count(key) > 0;
}

/************************** GlobalBoard **************************/
template<typename T>
int GlobalBoard<T>::data_check(const uint8_t *dut, const uint8_t *ref, std::string assert_info) {
#ifndef NOCHECK
    for (int i = 0; i < DATASIZE; i++) {
        if (dut[i] != ref[i]) {
            printf("dut: ");
            for (int j = 0; j < DATASIZE; j++) {
                printf("%02hhx", dut[j]);
            }
            printf("\nref: ");
            for (int j = 0; j < DATASIZE; j++) {
                printf("%02hhx", ref[j]);
            }
            printf("\n");
            tlc_assert(false, assert_info.data());
            return -1;
        }
    }
#endif
    return 0;
}

template<typename T>
int GlobalBoard<T>::verify(const T& key, const uint8_t* data) {
#ifndef NOCHECK
    if (this->mapping.count(key) == 0) { // we assume data is all zero initially
        return this->data_check(data, init_zeros, "Init data is non-zero!");
    }
    tlc_assert(this->mapping.count(key) == 1, "Duplicate records found in GlobalBoard!");

    Global_SBEntry value = *this->mapping.at(key).get();
    if (value.status == Global_SBEntry::SB_VALID) {
        tlc_assert(value.data != nullptr, "NULL occured in valid entry of GlobalBoard!");
        return this->data_check(data, value.data, "Data mismatch!");
    } else if (value.status == Global_SBEntry::SB_PENDING) {
        bool flag = true;
        if (value.data != nullptr) {
            for (int i = 0; i < DATASIZE; i++) {
                if (data[i] != value.data[i]) {
                    flag = false;
                    break;
                }
            }
            if (flag) return 0;
        } else {
            for (int i = 0; i < DATASIZE; i++) {
                if (data[i] != init_zeros[i]) {
                    flag = false;
                    break;
                }
            }
            if (flag) return 0;
        }
        tlc_assert(value.pending_data != nullptr, "NULL occured in pending entry of GlobalBoard!");
        this->data_check(data, value.pending_data, "Data mismatch!");
        return 0;
    } else {
        // TODO: handle other status
        tlc_assert(false, "Unknown GlobalBoard entry status!");
        return -1;
    }
#else
    return 0;
#endif
}

template<typename T>
void GlobalBoard<T>::unpending(const T& key) {
#ifndef NOCHECK
    tlc_assert(this->mapping.count(key) == 1, "Un-pending non-exist entry in GlobalBoard!");
    Global_SBEntry* value = this->mapping.at(key).get();
    // tlc_assert(value->pending_data != nullptr, "Un-pending entry with NULL ptr in GlobalBoard!");
    if (value->pending_data == nullptr) {
        return;
    }
    value->data = value->pending_data;
    value->pending_data = nullptr;
    value->status = Global_SBEntry::SB_VALID;
#else
    return;
#endif
}

#ifdef NOCHECK
template<typename T>
std::map<T, std::shared_ptr<Global_SBEntry>>& GlobalBoard<T>::get() {
    return this->mapping;
}

template<typename T>
void GlobalBoard<T>::update(const T& key, std::shared_ptr<Global_SBEntry>& data) {
    return;
}

template<typename T>
std::shared_ptr<Global_SBEntry> GlobalBoard<T>::query(const T& key) {
    // TODO: this may be fine,
    // cuz we also haskey to check its existence before query
    // so hopefully we won't reach here
    return NULL;
}

template<typename T>
void GlobalBoard<T>::erase(const T& key) {
    return;
}

template<typename T>
bool GlobalBoard<T>::haskey(const T& key) {
    return false;
}

#endif // NOCHECK

#endif // TLC_TEST_SCOREBOARD_H
