#include "parallel/task_engine.hpp"

#include <iostream>

using namespace std;
using namespace silver_bullets;

template<class F> inline TaskFunc taskFunc_2(F f)
{
    return [f](pany_range out, const_pany_range in) {
        BOOST_ASSERT(out.size() == 1);
        BOOST_ASSERT(in.size() == 2);
        using args_t = typename boost::function_types::parameter_types<decltype(&F::operator())>::type;
        using A1 = typename boost::mpl::at<args_t, boost::mpl::int_<1>>::type;
        using A2 = typename boost::mpl::at<args_t, boost::mpl::int_<2>>::type;
        *(out[0]) = f(boost::any_cast<A1>(*(in[0])), boost::any_cast<A2>(*(in[1])));
    };
}

void test_01()
{
    auto plus = taskFunc_2([](int a, int b) {
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
    x.start(&g, cache, &taskFuncRegistry);
    x.join();

    cout << boost::any_cast<int>(g.output(task2, 0)) << endl;
}

void test_02()
{
    auto plus = taskFunc_2([](int a, int b) {
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
    x.start(&g, cache, &taskFuncRegistry);
    x.join();

    cout << boost::any_cast<int>(g.output(bottomTask, 0)) << endl;
}

/*
void f()
{
    TaskGraph g;

    auto plus = taskFunc_2([](int a, int b) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return a + b;
    });
    TaskFuncRegistry taskFuncRegistry;
    constexpr auto addId = 123;
    constexpr auto resType = 444;
    taskFuncRegistry[addId] = plus;
    Task add = {2, 1, addId, resType};
    ThreadedTaskExecutor x(resType);

    vector<boost::any> inputs(2);
    inputs[0] = 1;
    inputs[1] = 2;
    vector<boost::any> outputs(1);
    auto done = false;
    auto cb = [&]() {
        cout << "WOW! " << boost::any_cast<int>(outputs[0]) << endl;
        done = true;
    };
    x.start(add, vectorRange(outputs), vectorRange(inputs), cb, taskFuncRegistry);

    // g.setResourceCapacity(123, 10);
//    auto idAdd = g.addTask(&add, 1);

//    g.setCb([&]() {
//        cout << "WOW! " << boost::any_cast<int>(add.getOutputs()[0]) << endl;
//        done = true;
//    });

//    g.start({{idAdd, 0}});

//    add.start();

    while(!done) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        x.propagateCb();
    }
}
//*/
int main()
{
    // test_01();
    test_02();
    // f();

    return 0;
}
