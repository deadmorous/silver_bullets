#include <iostream>

#include "silver_bullets/trie.hpp"

using namespace std;

int main(int argc, char **argv)
{
    try {
        using namespace silver_bullets;

        Trie<AsciiCharAlphabet, size_t> t;
        t.add("foo");
        t.add("bar");
        t.add("foo_bar");
        cout << "foo: " << t.find("foo") << endl;
        cout << "bar: " << t.find("bar") << endl;
        cout << "foo_bar: " << t.find("foo_bar") << endl;
        cout << "foobar: " << t.find("foobar") << endl;
        return EXIT_SUCCESS;
    }
    catch(const exception& e) {
        cerr << "ERROR: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}
