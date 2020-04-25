#pragma once

#include "iterate_struct.h"
#include <ostream>

#include "enum_names/enum_names.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/join.hpp>

namespace iterate_struct {

template<class stream>
class value_printer
{
public:
    value_printer(
            stream& s,
            const std::function<void(const std::string&)>& cb = std::function<void(const std::string&)>()) :
        m_s(s), m_cb(cb)
    {}

    template<class T>
    void operator()(T& value, const char *name) const
    {
        maybePad(true);
        m_s << name << " = ";
        m_current_path_items.push_back(name);
        if (print_priv(value, false))
            m_s << std::endl;
        m_current_path_items.pop_back();
    }

    template <class T>
    void print(const T& x) const
    {
        print_priv(x, true);
    }

private:
    class scoped_inc
    {
    public:
        scoped_inc(int& x) : m_x(x) {
            ++m_x;
        }
        ~scoped_inc() {
            --m_x;
        }
    private:
        int& m_x;
    };

    template <class T, std::enable_if_t<iterate_struct::has_output_operator_v<stream, T>, int> = 0>
    bool print_priv(const T& x, bool needPad) const
    {
        maybePad(needPad);
        m_s << x;
        if (!needPad && m_cb)
            m_cb(current_path());
        return true;
    }

    template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
    bool print_priv(const T& x, bool needPad) const
    {
        maybePad(needPad);
        m_s << enum_item_name(x);
        if (!needPad && m_cb)
            m_cb(current_path());
        return true;
    }

    template <class T, std::enable_if_t<iterate_struct::has_iterate_struct_helper_v<T>, int> = 0>
    bool print_priv(const T& x, bool needPad) const
    {
        if (needPad)
            for_each(x, *this);
        else {
            m_s << std::endl;
            scoped_inc scinc(m_depth);
            for_each(x, *this);
        }
        return false;
    }

    template<class T>
    bool print_priv(const std::vector<T>& x, bool needPad) const
    {
        maybePad(needPad);
        m_s << "[" << std::endl;
        auto leaves = true;
        {
            scoped_inc scinc(m_depth);
            for (std::size_t i=0, n=x.size(); i<n; ++i) {
                auto& xi = x[i];
                m_current_path_items.push_back(boost::lexical_cast<std::string>(i));
                if (!print_priv(xi, true))
                    leaves = false;

                // Separate aggregate array items with an extra newline
                if (leaves || i+1<n)
                    m_s << std::endl;

                m_current_path_items.pop_back();
            }
        }
        maybePad(true);
        m_s << ']';
        if (leaves && m_cb)
            m_cb(current_path());
        m_s << std::endl;
        return false;
    }

    void maybePad(bool needPad) const
    {
        if (needPad)
            m_s << std::string(m_depth<<2, ' ');
    }

    stream& m_s;
    std::function<void(const std::string&)> m_cb;
    mutable int m_depth = 0;

    std::string current_path() const {
        return std::string("/") + boost::join(m_current_path_items, "/");
    }

    mutable std::vector<std::string> m_current_path_items;
};

template<class stream, class T>
inline void print(
        stream& s, const T& x,
        const std::function<void(const std::string&)>& cb = std::function<void(const std::string&)>())
{
    value_printer<stream>(s, cb).print(x);
}

} // namespace iterate_struct
