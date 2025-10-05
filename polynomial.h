#ifndef POLYNOMIAL_H
#define POLYNOMIAL_H

#include <string>


class Polynomial {


public:
    // Insert a term into the polynomial
    void insertTerm(int coefficient, int exponent);

    // Return polynomial as a human-readable string
    std::string toString() const;

    // Return a new polynomial that is the sum of this and other
    Polynomial add(const Polynomial& other) const;

    // Return a new polynomial that is the product of this and other
    Polynomial multiply(const Polynomial& other) const;

    // Return a new polynomial that is the derivative of this polynomial
    Polynomial derivative() const;
};

#endif