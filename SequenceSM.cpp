#include "SequenceSM.hpp"

#include "TCircleBuffer.hpp"
#include <deque>

#include <sss/util/PostionThrow.hpp>
#include <sss/bit_operation/bit_operation.h>

SequenceSM::SequenceSM()
    : m_max_jump_cnt(0u)
{
    this->m_statuss.push_back(State{});
}

size_t SequenceSM::find_jump(size_t from, char input) const
{
    if (this->m_statuss.size() > from) {
        const auto it = this->m_sm.find(sm_key_t{from, input});
        size_t next = it != this->m_sm.end() ? it->second : 0;
        if (next >= this->m_statuss.size()) {
            SSS_POSTION_THROW(std::runtime_error,
                              next << " >= " << this->m_statuss.size());
        }
        return next;
    }
    return 0;
}

size_t SequenceSM::ensure_jump(size_t from, char input, const std::function<std::string()>& action)
{
    size_t next_st = this->find_jump(from, input);
#ifdef _DEBUG
    std::cout
        << __func__ << ":" << __LINE__
        << ":from (" << from << ", " << int(input) << ", " << next_st << ", " << this->m_statuss.size() << ")"
        << std::endl;
#endif
    if (next_st) {
        // NOTE no need to collapse checking!
        // if (this->m_statuss[next_st].m_prev_index != from || this->m_statuss[next_st].m_prev_path != input) {
        //     SSS_POSTION_THROW(std::runtime_error,
        //                       "collapse");
        // }
        return next_st;
    }
    size_t current_jump_cnt = m_statuss[from].m_jump_cnt + 1; 
    if (this->m_max_jump_cnt < current_jump_cnt) {
        this->m_max_jump_cnt = current_jump_cnt;
    }
    this->m_statuss.push_back(State{from, input, current_jump_cnt, action});
    this->m_sm[sm_key_t{from, input}] = this->m_statuss.size() - 1;
    return this->m_statuss.size() - 1;
}

namespace  {
    template<typename C>
    void dump2stream(std::ostream& o, const C& c) {
        for (const auto& item : c) {
            o << item;
        }
    }
} // namespace 

// #define _DEBUG

// #define _USE_CB_

// TODO
// 使用定长循环buffer，而不是用std::deque，因为，需要的buffer长度，可以通过最长
// 匹配序列，而提前知道！
void SequenceSM::translate(std::istream& in, std::ostream& out)
{
    char ch;
    size_t st = 0;

#ifdef _USE_CB_
    std::cout << __func__ << " m_max_jump_cnt = " << this->m_max_jump_cnt << std::endl;
    TCircleBuffer<char> buffer(this->m_max_jump_cnt);
#else
    std::deque<char> buffer;
#endif

#ifdef _DEBUG
    int cnt = 0;
#endif
    while (in.get(ch)) {
#ifdef _DEBUG
        size_t st_old = st;
#endif
        st = this->find_jump(st, ch);
#ifdef _DEBUG
        std::cout
            << __func__ << ":" << __LINE__<<":" << cnt++ << "(" << st_old << ", " << ext::binary << ch << ", " << st << ")"
            << std::endl;
#endif
        if (!st) {
            dump2stream(out, buffer);
            buffer.clear();
            out << ch;
        }
        else {
            if (this->m_statuss[st].m_action) {
                out << this->m_statuss[st].m_action();
#ifdef _DEBUG
                dump2stream(std::cout, buffer);
                std::cout << " -> " << this->m_statuss[st].m_action();
#endif
                buffer.clear();
                st = 0; // NOTE jump to init state
            }
            else {
#ifdef _DEBUG
                std::cout << __func__ << ":" << __LINE__ << ":" << cnt << ":push_back(" << ext::binary << ch << ")" << std::endl;
#endif

#ifdef _USE_CB_
                buffer.push(ch);
#else
                buffer.push_back(ch);
#endif
            }
        }
    }
    dump2stream(out, buffer);
    buffer.clear();
}
