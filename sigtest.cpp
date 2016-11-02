#include <boost/signals2.hpp>
//#include <boost/bind.hpp>
#include <iostream>

using namespace boost;
using namespace std;

struct ClassA
{
    boost::signals2::signal<void ()>    SigA;
    boost::signals2::signal<void (int)> SigB;
};

struct ClassB
{
    void PrintFoo()      { cout << "Foo" << endl; }
    void PrintInt(int i) { cout << "Bar: " << i << endl; }
};

int main()
{
    ClassA a;
    ClassB b, b2;

    a.SigA.connect(bind(&ClassB::PrintFoo, &b));
    a.SigB.connect(bind(&ClassB::PrintInt, &b,  _1));
    a.SigB.connect(bind(&ClassB::PrintInt, &b2, _1));

    a.SigA();
    a.SigB(4);
}