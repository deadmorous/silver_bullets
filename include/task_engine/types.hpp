#pragma once

#include <boost/any.hpp>
#include <boost/range/iterator_range.hpp>

namespace silver_bullets {
namespace task_engine {

using const_pany_range = boost::iterator_range<boost::any const* const*>;
using pany_range = boost::iterator_range<boost::any* const*>;

template<class TaskFunc> struct TaskFuncTraits;

template<class TaskFunc> class ThreadedTaskExecutorInit;

} // namespace task_engine
} // namespace silver_bullets
