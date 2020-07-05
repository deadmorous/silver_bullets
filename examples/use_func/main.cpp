#include "silver_bullets/func/func_arg_convert.hpp"

#include <iostream>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace silver_bullets;

struct ArgConverter
{
    template<class T>
    T operator()(const string& s) const {
        return boost::lexical_cast<T>(s);
    }
};

int main()
{
    auto f = func_arg_convert<string&, const vector<string>&>(
                [](int a, int b) { return a + b; },
                [](string& s, auto x) {
                    s = boost::lexical_cast<string>(x);
                },
                ArgConverter());

    auto call_f = [&f](const vector<string>& args) {
        string result;
        f(result, args);
        cout << args[0] << " + " << args[1] << " = " << result << endl;
    };

    call_f({ "20", "22" });
    call_f({ "23", "77" });
    return 0;
}
