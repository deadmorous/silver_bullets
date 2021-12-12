#include "silver_bullets/enum_names.hpp"

#include "silver_bullets/iterate_struct/json_doc_converter.hpp"
#include "silver_bullets/iterate_struct/json_doc_io.hpp"

#include <iostream>

using namespace std;

enum class NamedEnum
{
    One,
    Two,
    Three
};

SILVER_BULLETS_BEGIN_DEFINE_ENUM_NAMES(NamedEnum)
    {NamedEnum::One, "one"},
    {NamedEnum::Two, "two"},
    {NamedEnum::Three, "three"},
SILVER_BULLETS_END_DEFINE_ENUM_NAMES()

SILVER_BULLETS_DEF_ENUM_STREAM_IO(NamedEnum);

enum class OrderedEnum
{
    One,
    Two,
    Three
};

SILVER_BULLETS_BEGIN_DEFINE_ENUM_NAMES(OrderedEnum, int)
    {OrderedEnum::One, 0},
    {OrderedEnum::Two, 1},
    {OrderedEnum::Three, 2},
SILVER_BULLETS_END_DEFINE_ENUM_NAMES()

SILVER_BULLETS_DEF_ENUM_STREAM_IO(OrderedEnum);


struct A {
    NamedEnum named;
    OrderedEnum ordered;
};

SILVER_BULLETS_DESCRIBE_STRUCTURE_FIELDS(A, named, ordered)

void run()
{
    auto namedEnum = silver_bullets::enum_item_value<NamedEnum>("two");
    cout << namedEnum << endl;

    auto orderedEnum = silver_bullets::enum_item_value<OrderedEnum>(1);
    cout << orderedEnum << endl;

    auto a = A{ namedEnum, orderedEnum };
    auto doc = silver_bullets::iterate_struct::to_json_doc(a);
    silver_bullets::iterate_struct::write_json_doc(cout, doc);

    auto a_parsed = silver_bullets::iterate_struct::from_json_doc<A>(doc);
    BOOST_ASSERT(a.named == a_parsed.named);
    BOOST_ASSERT(a.ordered == a_parsed.ordered);
}

int main(int /*argc*/, char */*argv*/[])
{
    try {
        run();
        return EXIT_SUCCESS;
    }
    catch(const exception& e) {
        cerr << "ERROR: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}
