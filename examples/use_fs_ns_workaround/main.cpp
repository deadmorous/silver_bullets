#include <iostream>
#include <iomanip>

#include "fs_ns_workaround/fs_ns_workaround.hpp"

using namespace std;

int main(int /*argc*/, char **argv)
{
    try {
        auto ok = filesystem::exists(argv[0]);
        cout << "exists('" << argv[0] << "') returned " << boolalpha << ok << endl;
        return EXIT_SUCCESS;
    }
    catch(const exception& e) {
        cerr << "ERROR: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}
