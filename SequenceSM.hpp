#ifndef __SEQUENCESM_HPP_1467713051__
#define __SEQUENCESM_HPP_1467713051__

#include <cstdlib>
#include <functional>

#include <vector>
#include <unordered_map>

#include <iostream>

/**
 * @brief 基于字符序列的状态机；
 *        如果实在不行的话，还有退路，是二叉查找树；然后叶子节点保存动作(替换序
 *        列)；
 */
class SequenceSM
{
public:
    /**
     * @brief 状态对象；
     * 内部用唯一的ID值，以示区别；
     * 同时，可以绑定动作；
     * std::function<std::string(void)>
     */
    struct State
    {
        // NOTE
        // 由于 unordered_map 中，second 保存的是下一条的index；
        // 值！
        // 那么冲突如何检测呢？
        // 因为理论上，我这种方式保存的数据，实际模拟的是一棵多叉树；
        // 就是说，内部没有闭环结构；所以，不可能有多条路径，可以通到某具体状态
        // （当然，初始状态除外——所有状态在结束后，都会跳回它）
        // 在ensure_jump() 函数中，除了检测
        // 所以，State::m_hash_value 应该保存的是"上一跳index"+"路径"，而不是
        // hash值——因为，我已经假设，发生了冲突！
        size_t                       m_prev_index;
        char                         m_prev_path;
        size_t                       m_jump_cnt; // 距S0，多少条边的路径？
        std::function<std::string()> m_action;

        State()
            : m_prev_index(0u), m_prev_path('\0'), m_jump_cnt(0u), m_action(nullptr)
        {}

        State(size_t prev_index, char prev_path, size_t jump_cnt = 0u, std::function<std::string()> action = nullptr)
            : m_prev_index(prev_index), m_prev_path(prev_path), m_jump_cnt(jump_cnt), m_action(action)
        {}
    };

    typedef std::pair<uint32_t, char>   sm_key_t;

    struct State_hash
    {
        size_t operator()(const sm_key_t& x) const {
            return std::hash<int>()(x.first) ^ std::hash<char>()(x.second);
        }
    };

protected:
    // uint32_t            m_init_id;
    // State               m_init_st;
    std::vector<State>  m_statuss;
    // NOTE TODO 应该在数组中，存放所有状态；
    // 然后数组index，作为ID值；
    // 即，key，应该表示索引，而不应该保存对象……
    typedef std::unordered_map<sm_key_t, uint32_t, State_hash> HashJump_t;
    HashJump_t m_sm;

    size_t  m_max_jump_cnt;

public:
    SequenceSM();
    ~SequenceSM() = default;

public:
    SequenceSM(SequenceSM&& ) = default;
    SequenceSM& operator = (SequenceSM&& ) = default;

public:
    SequenceSM(const SequenceSM& ) = default;
    SequenceSM& operator = (const SequenceSM& ) = default;

public:
    /**
     * @brief 检查是否存在某分支
     *
     * @return 分支编号；0u，表示不存在
     */
    size_t find_jump(size_t from, char input) const;
    // size_t ensure_jump(size_t from, char input);
    size_t ensure_jump(size_t from, char input, const std::function<std::string()>& action = nullptr);

    void translate(std::istream& in, std::ostream& out);

private:
    
};


#endif /* __SEQUENCESM_HPP_1467713051__ */
