#include "task_engine/task_engine.hpp"

#include <iostream>

using namespace std;
using namespace silver_bullets::task_engine;

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
    using TFR = TaskFuncRegistry<TaskFunc>;
    using TTX = ThreadedTaskExecutor<TaskFunc>;
    using TGX = TaskGraphExecutor<TaskFunc>;

    auto plus = makeSimpleTaskFunc([](int a, int b) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return a + b;
    });
    auto plusId = 1;
    TFR taskFuncRegistry;
    taskFuncRegistry[plusId] = plus;

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
    for (auto i=0; i<10; ++i)
        x.addTaskExecutor(std::make_shared<TTX>(resType, &taskFuncRegistry));

    auto cache = x.makeCache();
    x.start(&g, cache).join();

    cout << boost::any_cast<int>(g.output(task2, 0)) << endl;
}

// Computes the following graph (each node computes the sum of its two input).
//
// 1 2   3 4   5 6   7 8
// | |   | |   | |   | |
// +-+   +-+   +-+   +-+
// |0|   |1|   |2|   |3|
// +-+   +-+   +-+   +-+
//   \   / \   / \   /
//    \ /   \ /   \ /
//    +-+   +-+   +-+
//    |4|   |5|   |6|
//    +-+   +-+   +-+
//      \   / \   /
//       \ /   \ /
//       +-+   +-+
//       |7|   |8|
//       +-+   +-+
//         \   /
//          \ /
//          +-+
//          |9|
//          +-+
//           |
//           72
void test_02()
{
    using TaskFunc = SimpleTaskFunc;
    using TFR = TaskFuncRegistry<TaskFunc>;
    using TTX = ThreadedTaskExecutor<TaskFunc>;
    using TGX = TaskGraphExecutor<TaskFunc>;

    auto plus = makeSimpleTaskFunc([](int a, int b) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return a + b;
    });
    auto plusId = 1;
    TFR taskFuncRegistry;
    taskFuncRegistry[plusId] = plus;

    auto resType = 1;

    TGX x;
    for (auto i=0; i<10; ++i)
        x.addTaskExecutor(std::make_shared<TTX>(resType, &taskFuncRegistry));

    auto N = 4;
    std::vector<size_t> topTasks;
    TaskGraphBuilder b;
    for (auto i=0; i<N; ++i)
        topTasks.push_back(b.addTask(2, 1, plusId, resType));
    auto upTasks = topTasks;
    std::size_t bottomTask;
    for (--N; N>0; --N) {
        std::vector<size_t> downTasks;
        for (auto i=0; i<N; ++i) {
            downTasks.push_back(b.addTask(2, 1, plusId, resType));
            b.connect(upTasks[i], 0, downTasks[i], 0);
            b.connect(upTasks[i+1], 0, downTasks[i], 1);
        }
        if (N == 1)
            bottomTask = downTasks[0];
        swap(upTasks, downTasks);
    }
    auto g = b.taskGraph();

    auto v = 1;
    for (std::size_t i=0; i<topTasks.size(); ++i) {
        g.input(topTasks[i], 0) = v++;
        g.input(topTasks[i], 1) = v++;
    }

    auto cache = x.makeCache();

    bool syncMode = true;
    if (syncMode) {
        // Call computation synchronously
        x.start(&g, cache);
        x.join();
    }
    else {
        // In the asynchronous mode, we specify a callback;
        // however, to get that callback called from the main thread,
        // we need to call propagateCb() sometimes.
        auto done = false;
        x.start(&g, cache, [&done]() {
            done = true;
        });
        while (!done) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
            x.propagateCb();
        }
    }

    cout << boost::any_cast<int>(g.output(bottomTask, 0)) << endl;
}

// Computes the graph shown below.
// Each node computes the sum of
// - thread local state (=1);
// - read-only shared state (=2);
// - all input values.
//
//         +-----+
//         | t11 |
//         +-----+
//          |   |
//     +-----+ +-----+
//     | t21 | | t22 |
//     +-----+ +-----+
//      |   |   |   |
// +-----+ +-----+ +-----+
// | t31 | | t32 | | t33 |
// +-----+ +-----+ +-----+
//      |   |   |   |
//     +-----+ +-----+
//     | t41 | | t42 |
//     +-----+ +-----+
//          |   |
//         +-----+
//         | t51 |
//         +-----+
//            |
//            57
void test_03()
{
    using TaskFunc = StatefulTaskFunc;

    // Outputs the sum of the local state and all inputs
    class ComputeFunc : public StatefulTaskFuncInterface {
    public:
        void call(const pany_range& out, const const_pany_range& in) const override
        {
            auto result =
                    boost::any_cast<int>(*threadLocalData()) +
                    boost::any_cast<int>(*readOnlySharedData());

            for (auto& item : in)
                result += boost::any_cast<int>(*item);
            *(out[0]) = result;
        }
    };
    auto computeFuncId = 1;

    using TFR = TaskFuncRegistry<TaskFunc>;
    using TTX = ThreadedTaskExecutor<TaskFunc>;
    using TGX = TaskGraphExecutor<TaskFunc>;

    TFR taskFuncRegistry;
    taskFuncRegistry[computeFuncId] = TaskFunc(make_shared<ComputeFunc>());

    TGX x;
    auto resType = 1;
    boost::any readOnlySharedData = 2;
    for (auto i=0; i<10; ++i) {
        auto tx = std::make_shared<TTX>(resType, &taskFuncRegistry, [](){ return boost::any(1); });
        tx->setReadOnlySharedData(&readOnlySharedData);
        x.addTaskExecutor(tx);
    }

    TaskGraphBuilder b;
    auto t11 = b.addTask(0, 1, computeFuncId, resType);
    auto t21 = b.addTask(1, 1, computeFuncId, resType);
    auto t22 = b.addTask(1, 1, computeFuncId, resType);
    auto t31 = b.addTask(1, 1, computeFuncId, resType);
    auto t32 = b.addTask(2, 1, computeFuncId, resType);
    auto t33 = b.addTask(1, 1, computeFuncId, resType);
    auto t41 = b.addTask(2, 1, computeFuncId, resType);
    auto t42 = b.addTask(2, 1, computeFuncId, resType);
    auto t51 = b.addTask(2, 1, computeFuncId, resType);
    b.connect(t11, 0, t21, 0);
    b.connect(t11, 0, t22, 0);
    b.connect(t21, 0, t31, 0);
    b.connect(t21, 0, t32, 0);
    b.connect(t22, 0, t32, 1);
    b.connect(t22, 0, t33, 0);
    b.connect(t31, 0, t41, 0);
    b.connect(t32, 0, t41, 1);
    b.connect(t32, 0, t42, 0);
    b.connect(t33, 0, t42, 1);
    b.connect(t41, 0, t51, 0);
    b.connect(t42, 0, t51, 1);

    auto g = b.taskGraph();
    auto cache = x.makeCache();
    x.start(&g, cache).join();

    cout << boost::any_cast<int>(g.output(t51, 0)) << endl;
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
void test_04(const CancelController::Checker& isCancelled)
{
    // TODO: use cancel
    using TaskFunc = StatefulCancellableTaskFunc;

    // Outputs the sum of the local state and all inputs
    class ComputeFunc : public StatefulCancellableTaskFuncInterface {
    public:
        void call(
                    const pany_range& out,
                    const const_pany_range& in,
                    const CancelController::Checker& isCancelled) const override
        {
            // Wait for 300 ms, but check if the computation is cancelled each 10 ms.
            for (auto x=0; x<30; ++x) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                if (isCancelled)
                    return;
            }
            auto result =
                    boost::any_cast<int>(*threadLocalData()) +
                    boost::any_cast<int>(*readOnlySharedData());
            for (auto& item : in)
                result += boost::any_cast<int>(*item);
            *(out[0]) = result;
        }
    };
    auto computeFuncId = 1;

    using TFR = TaskFuncRegistry<TaskFunc>;
    using TTX = ThreadedTaskExecutor<TaskFunc>;
    using TGX = TaskGraphExecutor<TaskFunc>;

    TFR taskFuncRegistry;
    taskFuncRegistry[computeFuncId] = TaskFunc(make_shared<ComputeFunc>());

    TGX x(isCancelled);
    auto resType = 1;
    boost::any readOnlySharedData = 2;
    for (auto i=0; i<5; ++i) {
        auto tx = std::make_shared<TTX>(resType, &taskFuncRegistry, [](){ return boost::any(1); }, isCancelled);
        tx->setReadOnlySharedData(&readOnlySharedData);
        x.addTaskExecutor(tx);
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
    cout << "Started..." << endl;

    using namespace std::chrono;
    auto startTime = system_clock::now();

    auto reportTimeElapsed = [&startTime]() {
        auto endTime = system_clock::now();
        auto duration = endTime - startTime;
        cout << "Time elapsed: " << duration_cast<milliseconds>(duration).count() << " ms" << endl;
    };

    x.join();

    if (isCancelled)
        cout << "cancelled" << endl;
    else
        cout << boost::any_cast<int>(g.output(t31, 0)) << endl;

    reportTimeElapsed();
}

int main()
{
    TaskQueueFuncRegistry funcRegistry;

    funcRegistry[0] = [](boost::any& threadLocalState, const CancelController::Checker&)
    {
        cout << "********** SETTING THREAD LOCAL STATE TO 42 **********" << endl;
        threadLocalState = 42;
        cout << "********** STARTING test_01 **********" << endl;
        test_01();
        cout << "********** FINISHED test_01 **********" << endl << endl;
    };

    funcRegistry[1] = [](boost::any&, const CancelController::Checker&)
    {
        cout << "********** STARTING test_02 **********" << endl;
        test_02();
        cout << "********** FINISHED test_02 **********" << endl << endl;
    };

    funcRegistry[2] = [](boost::any&, const CancelController::Checker&)
    {
        cout << "********** STARTING test_03 **********" << endl;
        test_03();
        cout << "********** FINISHED test_03 **********" << endl << endl;
    };

    funcRegistry[3] = [](boost::any& threadLocalState, const CancelController::Checker& isCancelled)
    {
        cout << "********** READING THREAD LOCAL STATE: "
             << boost::any_cast<int>(threadLocalState) << " **********" << endl;
        cout << "********** STARTING test_04 **********" << endl;
        test_04(isCancelled);
        cout << "********** FINISHED test_04 **********" << endl << endl;
    };

    CancelController cc;

    TaskQueueExecutor x(cc.checker());
    x.setTaskFuncRegistry(&funcRegistry);

    x.post(0);
    x.post(1);
    x.post(2);
    x.wait();
    x.post(3);
    this_thread::sleep_for(chrono::milliseconds(100));
    // cc.canceller().cancel();

    return 0;
}
