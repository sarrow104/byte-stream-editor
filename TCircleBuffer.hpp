#ifndef __TCIRCLEBUFFER_HPP_1467785669__
#define __TCIRCLEBUFFER_HPP_1467785669__

#include <cstdlib>
#include <memory>
#include <cassert>
#include <cstddef>
#include <iterator>
#include <utility>

/**
 * @brief FIFO 循环队列；
 *
 *  m_buffer       T*内存
 *  m_capacity     最大对象存放个数
 *  m_read_index   读的位置
 *  m_write_index  写的位置(未占用，可写)
 *
 *  NOTE 为了可以区分是写满了还是空，额外添加了一个'空'位置；
 *       该位置不占用；
 *       就是说，实际分配了 m_capacity + 1大小的空间；但最多，只使用
 *       其中的 m_capacity 个；
 *
 * @tparam T
 */
template<typename T>
class TCircleBuffer
{
public:
    /*
     * m_read_index              m_write_index
     * |-------------------------|-|
     *
     */
    enum { block_size = sizeof(T) };
    explicit TCircleBuffer(size_t len)
        : m_buffer(reinterpret_cast<T*>(new char[block_size * (len + 1)]))
        , m_capacity(len)
        , m_read_index(0u)
        , m_write_index(0u)
    {}
    ~TCircleBuffer()
    {
        this->clear();
        delete [] reinterpret_cast<char*>(m_buffer);
    }

public:
    TCircleBuffer(TCircleBuffer&& ) = default;
    TCircleBuffer& operator = (TCircleBuffer&& ) = default;

public:
    TCircleBuffer(const TCircleBuffer& ) = delete;
    TCircleBuffer& operator = (const TCircleBuffer& ) = delete;

protected:
    size_t shift_index(size_t index) const
    {
        return index == m_capacity ? 0u : index + 1;
    }

public:
    size_t   size() const
    {
        return (m_read_index > m_write_index ? m_write_index + m_capacity : m_write_index) - m_read_index;
    }
    bool empty() const
    {
        return m_read_index == m_write_index;
    }
    bool full() const
    {
        return this->m_read_index == this->shift_index(this->m_write_index);
    }

    const T& front() const
    {
        return m_buffer[m_read_index];
    }
    const T& back() const
    {
        if (!m_write_index) {
            return m_buffer[m_capacity];
        }
        else {
            return m_buffer[m_write_index - 1];
        }
    }

    const T& at(size_t idx) const
    {
        // TODO 越界检查
        size_t true_idx = m_read_index + idx;
        if (true_idx > m_capacity) {
            true_idx -= m_capacity;
        }
        return m_buffer[true_idx];
    }

    T& front()
    {
        return m_buffer[m_read_index];
    }
    T& back()
    {
        if (!m_write_index) {
            return m_buffer[m_capacity];
        }
        else {
            return m_buffer[m_write_index - 1];
        }
    }

    T& at(size_t idx)
    {
        // TODO 越界检查
        size_t true_idx = m_read_index + idx;
        if (true_idx > m_capacity) {
            true_idx -= m_capacity;
        }
        return m_buffer[true_idx];
    }

    friend class iterator;
    friend class const_iterator;

    // NOTE 模板风格Iterator参考自
    //! http://codereview.stackexchange.com/questions/74609/custom-iterator-for-a-linked-list-class
    template
    <
        typename Type,
        typename UnqualifiedType = std::remove_cv<Type>
    >
    class InnerIterator
        : public std::iterator<std::bidirectional_iterator_tag,
                               UnqualifiedType,
                               std::ptrdiff_t,
                               Type*,
                               Type&>
    {
    private:
        TCircleBuffer<UnqualifiedType>* m_cb;
        size_t                          m_idx;

    public:
        InnerIterator()
            : m_cb(nullptr), m_idx(0)
        {}
        InnerIterator(TCircleBuffer<UnqualifiedType>* cb, bool is_end = false)
            : m_cb(cb), m_idx(is_end ? cb->m_write_index : cb->m_read_index)
        {
        }

        InnerIterator(TCircleBuffer<UnqualifiedType>* cb, size_t idx, bool /*dumy*/)
            : m_cb(cb), m_idx(idx)
        {
        }
        ~InnerIterator()
        {
        }

        void swap(InnerIterator& ref) noexcept
        {
            std::swap(m_cb, ref.m_cb);
            std::swap(m_idx, ref.m_idx);
        }

        InnerIterator& operator++()
        {
            assert(m_cb != nullptr && "nullptr");
            m_idx = m_cb->shift_index(m_idx);
            return *this;
        }

        InnerIterator& operator++(int)
        {
            assert(m_cb != nullptr && "nullptr");
            InnerIterator tmp = *this;
            m_idx = m_cb->shift_index(m_idx);
            return tmp;
        }

        template<typename OtherType>
            bool operator == (const InnerIterator<OtherType>& rhs)
            {
                return this->m_cb == rhs.m_cb && this->m_idx == rhs.m_idx;
            }
        template<typename OtherType>
            bool operator != (const InnerIterator<OtherType>& rhs)
            {
                return this->m_cb != rhs.m_cb || this->m_idx != rhs.m_idx;
            }

        Type& operator* () const
        {
            assert(m_cb != nullptr && "nullptr");
            return m_cb->m_buffer[m_idx];
        }

        Type* operator->() const
        {
            assert(m_cb != nullptr && "nullptr");
            return m_cb->m_buffer + m_idx;
        }

        operator InnerIterator<const Type>() const
        {
            return InnerIterator<const Type>(m_cb, m_idx, true);
        }
    };

    typedef InnerIterator<T> iterator;
    typedef InnerIterator<const T> const_iterator;

    iterator begin()
    {
        return iterator(*this);
    }
    const_iterator begin() const
    {
        return const_iterator(*this);
    }

    iterator end()
    {
        return iterator(*this, true);
    }
    const_iterator end() const
    {
        return const_iterator(*this, true);
    }

    bool pop()
    {
        if (this->empty()) {
            return false;
        }
        (m_buffer + m_read_index)->~T();
        m_read_index = this->shift_index(m_read_index);
    }
    bool push(const T& value)
    {
        if (this->full()) {
            return false;
        }
        new (m_buffer + m_write_index) T(value);
        m_write_index = this->shift_index(m_write_index);
    }

    void clear()
    {
        for (size_t i = m_read_index; shift_index(i) != m_write_index; i = shift_index(i)) {
            (m_buffer + i)->~T();
        }
        m_write_index = m_read_index;
    }

private:
    T *     m_buffer;
    size_t  m_capacity;
    size_t  m_read_index;  
    size_t  m_write_index;  
};


#endif /* __TCIRCLEBUFFER_HPP_1467785669__ */
