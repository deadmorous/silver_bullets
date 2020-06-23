#include "parallel/task_engine_func.hpp"

#include <iostream>

using namespace std;
using namespace silver_bullets;

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
    auto plus = makeTaskFunc([](int a, int b) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return a + b;
    });
    auto plusId = 1;
    TaskFuncRegistry taskFuncRegistry;
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

    TaskGraphExecutor x;
    for (auto i=0; i<10; ++i)
        x.addTaskExecutor(std::make_shared<ThreadedTaskExecutor>(resType));

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
    auto plus = makeTaskFunc([](int a, int b) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return a + b;
    });
    auto plusId = 1;
    TaskFuncRegistry taskFuncRegistry;
    taskFuncRegistry[plusId] = plus;

    auto resType = 333;
    TaskGraphExecutor x;
    for (auto i=0; i<10; ++i)
        x.addTaskExecutor(std::make_shared<ThreadedTaskExecutor>(resType));

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

int main()
{
    test_01();
    test_02();

    return 0;
}
