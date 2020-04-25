#include "iterate_struct/iterate_struct.h"
#include "iterate_struct/value_printer.h"
#include "iterate_struct/ptree_converter.h"
#include "iterate_struct/json_doc_converter.h"
#include "iterate_struct/json_doc_io.h"
#include "iterate_struct/collect_paths_struct.h"
#include "iterate_struct/collect_paths_json_doc.h"

#include <boost/property_tree/json_parser.hpp>
#include <rapidjson/pointer.h>

#include <iostream>

// Example of usage

namespace my {

struct Foo {
    std::vector<int> bar = {1, 2, 3};
    std::string s = "s";
    double d = 2.3;
};

struct Bar {
    int x = 1;
    std::vector<Foo> foo {Foo(), Foo()};
};

template <class T, int i>
struct W
{
    struct P
    {
        T x;
    };
};

template<class T, int i>
using WP = typename W<T, i>::P;

template <class T, int i>
struct WP2
{
    T x = i;
};

} // namespace my

DESCRIBE_STRUCTURE_FIELDS(my::Foo, bar, s, d)
DESCRIBE_STRUCTURE_FIELDS(my::Bar, x, foo)

// Not working due to unresolvable template parameters
// DESCRIBE_TEMPLATE_STRUCTURE_FIELDS(((class, T), (int, i)), WP, x)

DESCRIBE_TEMPLATE_STRUCTURE_FIELDS(((class, T), (int, i)), my::WP2, x)

using namespace std;

int main()
{
    using namespace my;
    Foo foo;
    auto t = iterate_struct::asTuple(foo);
    get<0>(t)[1] = 42;

    const Foo& cfoo = foo;
    auto ct = iterate_struct::asTuple(cfoo);
    // get<0>(ct) = 42;    // Doesn't compile

    cout << "Printing fields of Foo:" << endl;
    iterate_struct::print(cout, foo);

    Bar bar;
    bar.foo[0].s = "Hello";
    bar.foo[1].bar[2] = 100500;
    cout << "Printing fields of Bar:" << endl;
    iterate_struct::print(cout, bar);

    cout << "Bar as JSON using ptree:" << endl;
    auto pt = iterate_struct::to_ptree(bar);
    boost::property_tree::write_json(cout, pt);

    cout << "Bar read from JSON using ptree:" << endl;
    auto bar2 = iterate_struct::from_ptree<Bar>(pt);
    iterate_struct::print(cout, bar2);

    cout << "JSON generated with rapidjson" << endl;
    auto jsdoc = iterate_struct::to_json_doc(bar);

    auto p = rapidjson::GetValueByPointer(jsdoc, "/foo/0/bar");
    auto v = p->GetArray();
    rapidjson::SetValueByPointer(jsdoc, "/a/s/d", 123);

    write_json_doc(cout, jsdoc);
    cout << endl;

    cout << "Bar read from JSON: using rapidjson" << endl;
    auto bar3 = iterate_struct::from_json_doc<Bar>(jsdoc);
    iterate_struct::print(cout, bar3);

    cout << "Paths collected from Foo" << endl;
    for (auto& path : iterate_struct::collect_paths(bar3))
        cout << path << endl;

    cout << "Paths collected from JSON (rapidjson)" << endl;
    for (auto& path : iterate_struct::collect_paths(jsdoc))
        cout << path << endl;

    return 0;
}
