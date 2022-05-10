#include "silver_bullets/iterate_struct/iterate_struct.hpp"
#include "silver_bullets/iterate_struct/value_printer.hpp"
#include "silver_bullets/iterate_struct/ptree_converter.hpp"
#include "silver_bullets/iterate_struct/json_doc_converter.hpp"
#include "silver_bullets/iterate_struct/json_doc_io.hpp"
#include "silver_bullets/iterate_struct/collect_paths_struct.hpp"
#include "silver_bullets/iterate_struct/collect_paths_json_doc.hpp"

#include <boost/property_tree/json_parser.hpp>
#include <rapidjson/pointer.h>

#include <iostream>
#include <map>

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

SILVER_BULLETS_DESCRIBE_STRUCTURE_FIELDS(my::Foo, bar, s, d)
SILVER_BULLETS_DESCRIBE_STRUCTURE_FIELDS(my::Bar, x, foo)

// Not working due to unresolvable template parameters
// SILVER_BULLETS_DESCRIBE_TEMPLATE_STRUCTURE_FIELDS(((class, T), (int, i)), WP, x)

SILVER_BULLETS_DESCRIBE_TEMPLATE_STRUCTURE_FIELDS(((class, T), (int, i)), my::WP2, x)


// Fieldless structs are ok
struct Fieldless {};
SILVER_BULLETS_DESCRIBE_STRUCTURE_FIELDS(Fieldless)

// Fieldless template structs are ok, too
template<class, class> struct FieldlessTemplate {};
SILVER_BULLETS_DESCRIBE_TEMPLATE_STRUCTURE_FIELDS(((class, A), (class, B)), FieldlessTemplate)


using namespace std;

struct Containers
{
    vector<string> v_str;
    pair<int, string> pair_int_str;
    pair<string, vector<string>> pair_str_v_str;
    map<string, vector<string>> map_str_v_str;
    map<int, int> map_int_int;
};

SILVER_BULLETS_DESCRIBE_STRUCTURE_FIELDS(
        Containers
        , v_str
        , pair_int_str
        , pair_str_v_str
        , map_str_v_str
        , map_int_int
        )


using namespace std;
using namespace silver_bullets;

void use_iterate_struct()
{
    cout << "\n********** use_iterate_struct() **********" << endl;

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

    iterate_struct::write_json_doc(cout, jsdoc);
    cout << endl;

    cout << "Bar read from JSON: using rapidjson" << endl;
    auto bar3 = iterate_struct::from_json_doc<Bar>(jsdoc);
    iterate_struct::print(cout, bar3);

    cout << "Paths collected from Foo" << endl;
    for (auto& path : iterate_struct::collect_paths(bar3, true))
        cout << path << endl;

    cout << "Paths collected from JSON (rapidjson)" << endl;
    for (auto& path : iterate_struct::collect_paths(jsdoc, true))
        cout << path << endl;


    auto jsdc = iterate_struct::to_json_doc(Containers{
            {"a", "s", "d"},    // v_str;
            {1, "a"},           // pair_int_str;
            { "qwe", {"a", "s", "d"} }, // pair_str_v_str;
            { { "a", {"s", "d"}}, {"q", {"w", "e"}} },  // map_str_v_str;
            { { 1, 3 }, {5,8}, {91, 42} } // map_int_int
    });
    auto containers = iterate_struct::from_json_doc<Containers>(jsdc);
    cout << endl << "Paths from a Containers instance:" << endl;
    for (auto& path : iterate_struct::collect_paths(containers, false))
        cout << path << endl;
    cout << endl << "Paths from the JSON doc for the same Containers instance:" << endl;
    for (auto& path : iterate_struct::collect_paths(jsdc, false))
        cout << path << endl;
    iterate_struct::print(cout, containers, [](const std::string& path) {
        cout << " ========= " << path;
    });
}
