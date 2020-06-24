#pragma once

#include <boost/any.hpp>
#include <boost/range/iterator_range.hpp>

#include <type_traits>

namespace silver_bullets {
namespace task_engine {

using const_pany_range = boost::iterator_range<boost::any const* const*>;
using pany_range = boost::iterator_range<boost::any* const*>;

template<class TaskFunc> struct TaskFuncTraits;
template<class TaskFunc> class ThreadedTaskExecutorInit;
template<class TaskFunc> struct TaskExecutorStartParam;
template<class TaskFunc> struct IsCancellable;

template<class TaskFunc>
static constexpr const bool IsCancellable_v = IsCancellable<TaskFunc>::value;

template<class TaskFunc, class = void> struct TaskExecutorCancelParam;

template<class TaskFunc>
using TaskExecutorCancelParam_t = typename TaskExecutorCancelParam<TaskFunc>::type;

} // namespace task_engine
} // namespace silver_bullets
