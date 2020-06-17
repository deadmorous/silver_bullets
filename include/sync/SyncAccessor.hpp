#pragma once

#include <boost/noncopyable.hpp>

namespace ctm {

template <class T, class L>
class SyncAccessor
{
public:
    SyncAccessor() = default;

    SyncAccessor(T& data, L& lockable) :
        m_data(&data),
        m_lockable(&lockable)
    {}

    class Accessor : boost::noncopyable
    {
    public:
        Accessor(T& data, L& lockable) :
            m_data(data),
            m_lockable(lockable)
        {
            m_lockable.lock();
        }
        ~Accessor() {
            m_lockable.unlock();
        }
        T& get() const {
            return m_data;
        }
        T* operator->() const {
            return &m_data;
        }
    private:
        T& m_data;
        L& m_lockable;
    };

    Accessor access() const {
        return Accessor(*m_data, *m_lockable);
    }

    L *lockable() const {
        return m_lockable;
    }

    T *unsafeData() const {
        return m_data;
    }

    operator bool() const {
        return m_data != nullptr;
    }

    bool operator==(const SyncAccessor<T, L>& that) const {
        return m_data == that.m_data;
    }

    bool operator!=(const SyncAccessor<T, L>& that) const {
        return !(*this == that);
    }

private:
    T *m_data = nullptr;
    L *m_lockable = nullptr;
};

} // namespace ctm
