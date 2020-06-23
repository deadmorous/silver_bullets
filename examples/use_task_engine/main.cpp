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
        x.addTaskExecutor(std::make_shared<TTX>(resType));

    auto cache = x.makeCache();
    x.start(&g, cache, &taskFuncRegistry).join();

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
        x.addTaskExecutor(std::make_shared<TTX>(resType));

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
        x.start(&g, cache, &taskFuncRegistry);
        x.join();
    }
    else {
        // In the asynchronous mode, we specify a callback;
        // however, to get that callback called from the main thread,
        // we need to call propagateCb() sometimes.
        auto done = false;
        x.start(&g, cache, &taskFuncRegistry, [&done]() {
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
        auto tx = std::make_shared<TTX>(resType, [](){ return boost::any(1); });
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
    x.start(&g, cache, &taskFuncRegistry).join();

    cout << boost::any_cast<int>(g.output(t51, 0)) << endl;
}

int main()
{
    test_01();
    test_02();
    test_03();

    return 0;
}
