#include "silver_bullets/factory.hpp"

#include <iostream>

using namespace std;

class Interface
{
public:
    virtual ~Interface() = default;
    virtual void hello() const = 0;
};

class A : public Interface,
        public silver_bullets::FactoryMixin<A, Interface>
{
public:
    A() {
        cout << "A::A()" << endl;
    }
    ~A() override {
        cout << "A::~A()" << endl;
    }
    void hello() const override {
        cout << "A::hello()" << endl;
    }
};

class B : public Interface,
        public silver_bullets::FactoryMixin<B, Interface>
{
public:
    B() {
        cout << "B::B()" << endl;
    }
    ~B() override {
        cout << "B::~B()" << endl;
    }
    void hello() const override {
        cout << "B::hello()" << endl;
    }
};

SILVER_BULLETS_FACTORY_REGISTER_TYPE(A, "A");
SILVER_BULLETS_FACTORY_REGISTER_TYPE(B, "B");


int main()
{
    try {
        auto x = silver_bullets::Factory<Interface>::newInstance("A");
        x->hello();
        x = silver_bullets::Factory<Interface>::newInstance("B");
        x->hello();
        return EXIT_SUCCESS;
    }
    catch(const exception& e) {
        cerr << e.what() << endl;
        return EXIT_FAILURE;
    }
}
