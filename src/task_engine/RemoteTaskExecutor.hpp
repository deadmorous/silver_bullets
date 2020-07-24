#pragma once

#include "silver_bullets/sync/ThreadNotifier.hpp"
#include "silver_bullets/task_engine/TaskExecutor.hpp"

#include "proto/task.grpc.pb.h"
#include "proto/task.pb.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>

#include <boost/assert.hpp>

namespace silver_bullets
{
namespace task_engine
{

template <class TaskFunc>
class RemoteTaskExecutor : public TaskExecutor<TaskFunc>
{
public:
    using Cb = typename TaskExecutor<TaskFunc>::Cb;

    template <class... InitArgs>
    explicit RemoteTaskExecutor(
        std::shared_ptr<grpc::Channel> channel,
        const ParametersRegistry& paramregistry,
        int resourceType,
        const sync::CancelController::Checker* cancelParam) :
      stub_(Executor::NewStub(channel)),
      m_paramregistry(paramregistry),
      m_resourceType(resourceType),
      m_thread([this]() { run(); })
    {
        if (cancelParam)
        {
            cancelParam->onCanceled([this]() {
                grpc::ClientContext context;
                CancelParam param;
                CancelReply reply;
                stub_->Cancel(&context, param, &reply);
            });
        }
    }

    ~RemoteTaskExecutor()
    {
        std::unique_lock<std::mutex> lk(m_incomingTaskNotifier.mutex());
        m_flags |= ExitRequested;
        lk.unlock();
        m_incomingTaskNotifier.notify_one();
        m_thread.join();
    }

    int resourceType() const override
    {
        return m_resourceType;
    }

protected:
    void doStart(TaskExecutorStartParam&& startParam) override
    {
        m_startParam = std::move(startParam);
        std::unique_lock<std::mutex> lk(m_incomingTaskNotifier.mutex());
        m_flags = HasInput;
        lk.unlock();
        m_incomingTaskNotifier.notify_one();
    }

public:
    void wait()
    {
        BOOST_ASSERT(m_taskCompletionNotifier);
        {
            std::unique_lock<std::mutex> lk(m_incomingTaskNotifier.mutex());
            if (!(m_flags & HasInput))
                return; // Nothing is being done
        }
        while (true)
        {
            std::unique_lock<std::mutex> lk(m_incomingTaskNotifier.mutex());
            if (m_flags & HasOutput)
                return;
            else
            {
                lk.unlock();
                m_taskCompletionNotifier->wait();
            }
        }
    }

    bool propagateCb() override
    {
        std::unique_lock<std::mutex> lk(m_incomingTaskNotifier.mutex());
        if (m_flags & HasOutput)
        {
            m_flags = 0;
            if (m_startParam.cb)
                m_startParam.cb();
            m_startParam = TaskExecutorStartParam();
            return true;
        }
        else
            return false;
    }

    void setTaskCompletionNotifier(
        sync::ThreadNotifier* taskCompletionNotifier) override
    {
        m_taskCompletionNotifier = taskCompletionNotifier;
    }

    sync::ThreadNotifier* taskCompletionNotifier() const override
    {
        return m_taskCompletionNotifier;
    }

private:
    std::unique_ptr<Executor::Stub> stub_;
    const ParametersRegistry m_paramregistry;
    int m_resourceType;
    TaskExecutorStartParam m_startParam;
    sync::ThreadNotifier m_incomingTaskNotifier;
    sync::ThreadNotifier* m_taskCompletionNotifier = nullptr;
    enum
    {
        HasInput = 0x01,
        HasOutput = 0x2,
        ExitRequested = 0x4
    };
    unsigned int m_flags = 0; // A combination of elements of the above enum
    using ThreadLocalData = ThreadLocalData_t<TaskFunc>;

    ThreadLocalData m_threadLocalData;

    // Note: Declare the thread last, such that all fields it can access
    // are initialized before the thread starts.
    std::thread m_thread;

    void run()
    {
        while (true)
        {
            m_incomingTaskNotifier.wait();
            if (m_flags & ExitRequested)
                return;
            else if (m_flags & HasInput)
            {
                grpc::ClientContext context;
                RunParam param;
                RunReply reply;

                auto& toString =
                    m_paramregistry.at(m_startParam.task.taskFuncId).first;

                for (const auto& input: m_startParam.inputs)
                {
                    param.add_inputs(toString(*input));
                }

                param.mutable_task()->set_inputcount(
                    m_startParam.task.inputCount);
                param.mutable_task()->set_taskfuncid(
                    m_startParam.task.taskFuncId);
                param.mutable_task()->set_outputcount(
                    m_startParam.task.outputCount);
                param.mutable_task()->set_resourcetype(
                    m_startParam.task.resourceType);

                grpc::Status status = stub_->Run(&context, param, &reply);
                if (!status.ok())
                {
                    // TODO
                    std::cout << "Run rpc failed." << std::endl;

                    std::unique_lock<std::mutex> lk(
                        m_incomingTaskNotifier.mutex());
                    m_flags |= ExitRequested;
                    lk.unlock();
                    m_incomingTaskNotifier.notify_one();

                    return;
                }

                auto& fromString =
                    m_paramregistry.at(m_startParam.task.taskFuncId).second;
                int index = 0;
                if (reply.outputs_size() > 0)
                {
                    for (auto& output: m_startParam.outputs)
                    {
                        *output = fromString(reply.outputs(index));
                        index++;
                    }
                }

                std::unique_lock<std::mutex> lk(m_incomingTaskNotifier.mutex());
                m_flags = HasOutput;
                lk.unlock();
                if (m_taskCompletionNotifier)
                    m_taskCompletionNotifier->notify_all();
            }
        }
    }
};

} // namespace task_engine
} // namespace silver_bullets
