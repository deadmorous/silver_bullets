#pragma once

#include <atomic>

#include "boost/signals2.hpp"

namespace silver_bullets {
namespace sync {

class CancelController
{
public:
    class Checker;
    class Canceller;
    class Resumer;
    friend class Checker;
    using Cancelled = boost::signals2::signal<void()>;

    class Checker
    {
    public:
        explicit Checker(CancelController& controller) : m_cancelled(&controller.m_cancelled), m_Cancelled(&controller.m_Cancelled)
        {}

        bool isCancelled() const {
            return *m_cancelled;
        }

        operator bool() const {
            return *m_cancelled;
        }


        boost::signals2::connection onCanceled(
            typename Cancelled::slot_type subscriber) const
        {
            return m_Cancelled->connect(subscriber);
        }


    protected:
        std::atomic<bool>& cancelled() {
            return *m_cancelled;
        }

    private:
        std::atomic<bool> *m_cancelled;
        Cancelled* m_Cancelled;
    };

    class Canceller : public Checker
    {
    public:
        explicit Canceller(CancelController& controller) : Checker(controller)
        {}
        void cancel() {
            cancelled() = true;
        }
    };

    class Resumer : public Canceller
    {
    public:
        explicit Resumer(CancelController& controller) : Canceller(controller)
        {}
        void resume() {
            cancelled() = false;
        }
    };

    Checker checker() {
        return Checker(*this);
    }

    Canceller canceller() {
        return Canceller(*this);
    }

    Resumer resumer() {
        return Resumer(*this);
    }

    bool isCancelled() const {
        return m_cancelled;
    }

    void cancel() {
        m_cancelled = true;
        m_Cancelled();
    }

    void resume() {
        m_cancelled = false;
    }

private:
    std::atomic<bool> m_cancelled = false;
    mutable Cancelled m_Cancelled;

};

} // namespace sync
} // namespace silver_bullets
