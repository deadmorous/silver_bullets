/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <chrono>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include "RemoteTaskExecutor.hpp"

#include "silver_bullets/task_engine.hpp"

#include <boost/lexical_cast.hpp>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using namespace silver_bullets;
using namespace task_engine;

// Computes the following graph (each node computes the sum of its two input).
//
//       1 2
//       | |
//       +-+
//       |1|
//  4    +-+
//   \   /
//    \ /
//    +-+
//    |4|
//    +-+
//     |
//     7
void test_01()
{

    using TaskFunc = SimpleTaskFunc;
    using TTX = RemoteTaskExecutor<TaskFunc>;
    using TGX = TaskGraphExecutor<TaskFunc>;

    auto plusId = 1;

    ParametersRegistry paramregistry;
    toString t = [](const boost::any& param) {
        auto value = boost::any_cast<int>(param);
        return boost::lexical_cast<std::string>(value);
    };

    fromString f = [](const std::string& param) {
        auto value = boost::lexical_cast<int>(param);
        return boost::any(value);
    };

    paramregistry[plusId] = std::make_pair(t, f);

    auto resType = 333;

    TaskGraphBuilder b;
    auto task1 = b.addTask(2, 1, plusId, resType);
    auto task2 = b.addTask(2, 1, plusId, resType);
    b.connect(task1, 0, task2, 1);

    auto g = b.taskGraph();

    g.input(task1, 0) = 1;
    g.input(task1, 1) = 2;
    g.input(task2, 0) = 4;

    TGX x;
    for (auto i = 0; i < 1; ++i)
    {
        auto channel = grpc::CreateChannel(
            "localhost:50051", grpc::InsecureChannelCredentials());
        x.addTaskExecutor(
            std::make_shared<TTX>(channel, paramregistry, resType, nullptr));
    }

    auto cache = x.makeCache();
    x.start(&g, cache).wait();

    std::cout << boost::any_cast<int>(g.output(task2, 0)) << std::endl;
}

// Example of use of a stateful cancellable task functions
//
// Computes the graph similar to that of in test_03, but smaller:
//
//         +-----+
//         | t11 |
//         +-----+
//          |   |
//     +-----+ +-----+
//     | t21 | | t22 |
//     +-----+ +-----+
//          |   |
//         +-----+
//         | t31 |
//         +-----+
//            |
//            15
void test_04(const sync::CancelController::Checker& isCancelled)
{
    // TODO: use cancel
    using TaskFunc = StatefulCancellableTaskFunc;

    // Outputs the sum of the local state and all inputs
    class ComputeFunc : public StatefulCancellableTaskFuncInterface
    {
    public:
        void call(
            boost::any& threadLocalData,
            const pany_range& out,
            const const_pany_range& in,
            const sync::CancelController::Checker& isCancelled) const override
        {
            // Wait for 300 ms, but check if the computation is cancelled each
            // 10 ms.
            for (auto x = 0; x < 30; ++x)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                if (isCancelled)
                    return;
            }
            auto result = boost::any_cast<int>(threadLocalData)
                          + boost::any_cast<int>(threadLocalData);
            for (auto& item: in)
                result += boost::any_cast<int>(*item);
            *(out[0]) = result;
        }
    };
    auto computeFuncId = 2;

    using TFR = TaskFuncRegistry<TaskFunc>;
    using TTX = RemoteTaskExecutor<TaskFunc>;
    using TGX = TaskGraphExecutor<TaskFunc>;

    TFR taskFuncRegistry;
    taskFuncRegistry[computeFuncId] = TaskFunc(std::make_shared<ComputeFunc>());

    ParametersRegistry paramregistry;
    toString t = [](const boost::any& param) {
        auto value = boost::any_cast<int>(param);
        return boost::lexical_cast<std::string>(value);
    };

    fromString f = [](const std::string& param) {
        auto value = boost::lexical_cast<int>(param);
        return boost::any(value);
    };

    paramregistry[computeFuncId] = std::make_pair(t, f);

    TGX x(isCancelled);
    auto resType = 1;
    auto port = 50051;
    for (auto i = 0; i < 2; ++i)
    {
        auto channel = grpc::CreateChannel(
            "localhost:" + std::to_string(port),
            grpc::InsecureChannelCredentials());
        auto tx =
            std::make_shared<TTX>(channel, paramregistry, resType, &isCancelled);
        x.addTaskExecutor(tx);
        port++;
    }

    TaskGraphBuilder b;
    auto t11 = b.addTask(0, 1, computeFuncId, resType);
    auto t21 = b.addTask(1, 1, computeFuncId, resType);
    auto t22 = b.addTask(1, 1, computeFuncId, resType);
    auto t31 = b.addTask(2, 1, computeFuncId, resType);
    b.connect(t11, 0, t21, 0);
    b.connect(t11, 0, t22, 0);
    b.connect(t21, 0, t31, 0);
    b.connect(t22, 0, t31, 1);

    auto g = b.taskGraph();
    auto cache = x.makeCache();
    x.start(&g, cache);
    std::cout << "Started..." << std::endl;

    using namespace std::chrono;
    auto startTime = system_clock::now();

    auto reportTimeElapsed = [&startTime]() {
        auto endTime = system_clock::now();
        auto duration = endTime - startTime;
        std::cout
            << "Time elapsed: " << duration_cast<milliseconds>(duration).count()
            << " ms" << std::endl;
    };

    x.wait();

    if (isCancelled)
        std::cout << "cancelled" << std::endl;
    else
        std::cout << boost::any_cast<int>(g.output(t31, 0)) << std::endl;

    reportTimeElapsed();
}

int main(int argc, char** argv)
{

    TaskQueueFuncRegistry funcRegistry;

    //    funcRegistry[0] = [](boost::any& threadLocalState,
    //                         const sync::CancelController::Checker&) {
    //        std::cout << "********** SETTING THREAD LOCAL STATE TO 42
    //        **********"
    //                  << std::endl;
    //        threadLocalState = 42;
    //        std::cout << "********** STARTING test_01 **********" <<
    //        std::endl; test_01(); std::cout << "********** FINISHED test_01
    //        **********" << std::endl
    //                  << std::endl;
    //    };

    funcRegistry[0] = [](boost::any&,
                         const sync::CancelController::Checker& isCancelled) {
        std::cout << "********** STARTING test_04 **********" << std::endl;
        test_04(isCancelled);
        std::cout << "********** FINISHED test_04 **********" << std::endl
                  << std::endl;
    };

    sync::CancelController cc;

    TaskQueueExecutor x(cc.checker());
    x.setTaskFuncRegistry(&funcRegistry);

    x.post(0);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    // cc.cancel();

    return 0;

    return 0;
}
