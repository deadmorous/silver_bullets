#pragma once

#include <atomic>

namespace silver_bullets {
namespace task_engine {

class CancelController
{
public:
    class Checker;
    class Canceller;
    class Resumer;
    friend class Checker;

    class Checker
    {
    public:
        explicit Checker(CancelController& controller) : m_cancelled(&controller.m_cancelled)
        {}

        bool isCancelled() const {
            return *m_cancelled;
        }

        operator bool() const {
            return *m_cancelled;
        }

    protected:
        std::atomic<bool>& cancelled() {
            return *m_cancelled;
        }

    private:
        std::atomic<bool> *m_cancelled;
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
    }

    void resume() {
        m_cancelled = false;
    }

private:
    std::atomic<bool> m_cancelled = false;

};

} // namespace task_engine
} // namespace silver_bullets
