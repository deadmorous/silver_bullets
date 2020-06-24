#pragma once

#include <condition_variable>
#include <mutex>

namespace silver_bullets {

class ThreadNotifier
{
public:
    void notify_one() {
        m_ready = true;
        m_cond.notify_one();
    }

    void notify_all() {
        m_ready = true;
        m_cond.notify_all();
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cond.wait(lock, [this] {
            return m_ready;
        });
        m_ready = false;
    }

    template< class Rep, class Period >
    bool wait_for(const std::chrono::duration<Rep, Period>& rel_time)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        auto status = m_cond.wait_for(lock, rel_time, [this] {
            return m_ready;
        });
        if (status)
            m_ready = false;
        return status;
    }

    std::mutex& mutex() {
        return m_mutex;
    }

private:
    std::condition_variable m_cond;
    std::mutex m_mutex;
    bool m_ready = false;
};

} // namespace silver_bullets
