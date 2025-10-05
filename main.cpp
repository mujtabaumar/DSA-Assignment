#include <iostream>
#include "Polynomial.h"
using namespace std;

int main() {
    // p1 = 3x^4 + 2x^2 - x + 5
    Polynomial p1;
    p1.insertTerm(3, 4);
    p1.insertTerm(2, 2);
    p1.insertTerm(-1, 1);
    p1.insertTerm(5, 0);
    cout << "p1: " << p1.toString() << endl;

    // p2 = x^4 + 1
    Polynomial p2;
    p2.insertTerm(1, 4);
    p2.insertTerm(1, 0);
    cout << "p2: " << p2.toString() << endl;

    // sum = p1 + p2
    Polynomial sum = p1.add(p2);
    cout << "sum (p1 + p2): " << sum.toString() << endl;

    // p3 = 2x
    Polynomial p3;
    p3.insertTerm(2, 1);
    cout << "p3: " << p3.toString() << endl;

    // prod = sum * p3
    Polynomial prod = sum.multiply(p3);
    cout << "product ((p1 + p2) * p3): " << prod.toString() << endl;

    // derivative of p1
    Polynomial deriv = p1.derivative();
    cout << "derivative of p1: " << deriv.toString() << endl;

    return 0;
}