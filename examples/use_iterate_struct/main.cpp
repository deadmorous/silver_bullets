#include "use_iterate_struct.hpp"
#include "use_json_config.hpp"

#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    try {
        // use_iterate_struct();
        use_json_config(argc, argv);
        return EXIT_SUCCESS;
    }
    catch(const exception& e) {
        cerr << "ERROR: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}
