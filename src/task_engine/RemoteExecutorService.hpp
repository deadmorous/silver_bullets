#pragma once

#include "silver_bullets/task_engine/TaskFuncRegistry.hpp"
#include "silver_bullets/task_engine/types.hpp"

#include "silver_bullets/sync/CancelController.hpp"

#include "proto/task.grpc.pb.h"
#include "proto/task.pb.h"

#include <grpc/grpc.h>

namespace silver_bullets
{
namespace task_engine
{

namespace
{
pany_range prepareRange(
    std::vector<boost::any>& data, std::vector<boost::any*>& dataPtr, int size)
{
    data = std::vector<boost::any>(size, std::string(""));

    for (auto& t: data)
    {
        dataPtr.push_back(&t);
    }

    auto d = dataPtr.data();
    return pany_range({d, d + size});
}

} // namespace

template <class TaskFunc>
class RemoteServiceImpl final : public Executor::Service
{
public:
    template <class... InitArgs>
    explicit RemoteServiceImpl(
        const TaskFuncRegistry<TaskFunc>* taskRegistry,
        const ParametersRegistry& paramregistry,
        sync::CancelController* controller,
        InitArgs... initArgs) :
      m_initParam(
          ThreadedTaskExecutorInit<TaskFunc>(taskRegistry, initArgs...)),
      m_paramregistry(paramregistry),
      m_controller(controller)
    {
        m_threadLocalData = m_initParam.initThreadLocalData();
    }

    grpc::Status Cancel(
        grpc::ServerContext* context,
        const CancelParam* request,
        CancelReply* response)
    {
        if (m_controller)
        {
            m_controller->cancel();
        }

        return grpc::Status::OK;
    }

    grpc::Status Run(
        grpc::ServerContext* context,
        const RunParam* request,
        RunReply* response)
    {
        auto taskFuncId = request->task().taskfuncid();
        auto& f = m_initParam.taskFuncRegistry->at(taskFuncId);

        std::vector<boost::any> inputData;
        std::vector<boost::any*> inputDataPtr;
        auto inputs =
            prepareRange(inputData, inputDataPtr, request->inputs_size());
        std::vector<boost::any> outputData;
        std::vector<boost::any*> outputDataPtr;
        auto outputs = prepareRange(
            outputData, outputDataPtr, request->task().outputcount());

        auto& fromString = m_paramregistry.at(taskFuncId).second;
        for (int i = 0; i < request->inputs_size(); i++)
        {
            *(inputs[i]) = fromString(request->inputs(i));
        }
        callTaskFunc(
            f,
            outputs,
            inputs,
            m_initParam.cancelParam,
            &m_threadLocalData,
            nullptr);

        if (m_controller->isCancelled())
        {
            m_controller->resume();
            return grpc::Status::OK;
        }

        auto& toString = m_paramregistry.at(taskFuncId).first;
        // convert outputs to response
        for (auto& output: outputs)
        {
            response->add_outputs(toString(*output));
        }
        return grpc::Status::OK;
    }

private:
    ThreadedTaskExecutorInit<TaskFunc> m_initParam;
    const ParametersRegistry m_paramregistry;
    sync::CancelController* m_controller;
    using ThreadLocalData = ThreadLocalData_t<TaskFunc>;
    ThreadLocalData m_threadLocalData;
};

} // namespace task_engine
} // namespace silver_bullets
