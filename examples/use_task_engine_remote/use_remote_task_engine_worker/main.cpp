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

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include "silver_bullets/task_engine.hpp"

#include "RemoteExecutorService.hpp"

#include <boost/lexical_cast.hpp>

using namespace silver_bullets;
using namespace task_engine;

using TaskFunc = SimpleTaskFunc;
using TFR = TaskFuncRegistry<TaskFunc>;

void RunServer(const TFR& funcRegistry, const ParametersRegistry& registry)
{
    std::string server_address("0.0.0.0:50051");
    RemoteServiceImpl<TaskFunc> service(&funcRegistry, registry, nullptr);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

using TaskFunc2 = StatefulCancellableTaskFunc;
using TFR2 = TaskFuncRegistry<TaskFunc2>;
void RunServer2(
    const TFR2& funcRegistry,
    const ParametersRegistry& registry,
    const std::string& host)
{
    sync::CancelController cc;
    std::string server_address(host);
    RemoteServiceImpl<TaskFunc2> service(
        &funcRegistry,
        registry,
        &cc,
        []() { return boost::any(1); },
        cc.checker());

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char** argv)
{
    auto plus = makeSimpleTaskFunc([](int a, int b) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return a + b;
    });

    auto plusId = 1;

    auto computeFuncId = 2;
    TFR taskFuncRegistry;
    taskFuncRegistry[plusId] = plus;

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
    paramregistry[computeFuncId] = std::make_pair(t, f);
    // RunServer(taskFuncRegistry, paramregistry);

    TFR2 taskFuncRegistry2;

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

    taskFuncRegistry2[computeFuncId] =
        TaskFunc2(std::make_shared<ComputeFunc>());

    std::thread t1(
        RunServer2, taskFuncRegistry2, paramregistry, "0.0.0.0:50051");
    std::thread t2(
        RunServer2, taskFuncRegistry2, paramregistry, "0.0.0.0:50052");

    t1.join();
    t2.join();

    return 0;
}
