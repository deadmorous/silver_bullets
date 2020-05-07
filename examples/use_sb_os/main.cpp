#include "sb_os/get_program_dir.hpp"

#include <iostream>

using namespace std;

int main()
{
    try {
        cout << get_program_dir() << endl;
        return EXIT_SUCCESS;
    }
    catch(const exception& e) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }
}
