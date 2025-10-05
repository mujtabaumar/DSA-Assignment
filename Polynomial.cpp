#include "Polynomial.h"
#include <map>
#include <sstream>
#include <unordered_map>
#include <mutex>
#include <vector>
using namespace std;

// thread-safe storage for polynomials
static mutex storageMutex;
static unordered_map<const Polynomial*, map<int,int>> polyStorage;

// stores temporary results for return values
static map<int,int> lastResultData;
static bool hasLastResult = false;

// tracks if a polynomial has been initialized
static unordered_map<const Polynomial*, bool> polyInitialized;

// helper to copy pending data into a new polynomial (like return values)
static void checkAndAdoptPendingData(const Polynomial* poly) {
    lock_guard<mutex> lock(storageMutex);

    // if new polynomial not initialized and we have pending data
    if (polyInitialized.find(poly) == polyInitialized.end() && hasLastResult) {
        polyStorage[poly] = lastResultData;
        polyInitialized[poly] = true;
        hasLastResult = false;
        lastResultData.clear();
    } else if (polyInitialized.find(poly) == polyInitialized.end()) {
        // just mark empty one as initialized
        polyInitialized[poly] = true;
    }
}

void Polynomial::insertTerm(int coefficient, int exponent) {
    lock_guard<mutex> lock(storageMutex);

    // check if we need to adopt pending result
    if (polyInitialized.find(this) == polyInitialized.end() && hasLastResult) {
        polyStorage[this] = lastResultData;
        polyInitialized[this] = true;
        hasLastResult = false;
        lastResultData.clear();
    }

    polyInitialized[this] = true;

    // ignore negative or too big exponents
    if (exponent < 0 || exponent > 10000) return;
    // ignore zero coefficients
    if (coefficient == 0) return;

    auto& terms = polyStorage[this];
    long long newCoeff = (long long)terms[exponent] + coefficient;

    // clamp to int range
    if (newCoeff > INT_MAX) terms[exponent] = INT_MAX;
    else if (newCoeff < INT_MIN) terms[exponent] = INT_MIN;
    else terms[exponent] = (int)newCoeff;

    // remove term if zero
    if (terms[exponent] == 0) terms.erase(exponent);
    if (terms.empty()) polyStorage.erase(this);
}

string Polynomial::toString() const {
    checkAndAdoptPendingData(this);
    lock_guard<mutex> lock(storageMutex);

    auto it = polyStorage.find(this);
    if (it == polyStorage.end() || it->second.empty()) return "0";

    const auto& terms = it->second;
    ostringstream ss;
    bool first = true;

    // go from highest to lowest exponent
    for (auto itr = terms.rbegin(); itr != terms.rend(); ++itr) {
        int coeff = itr->second;
        int exp = itr->first;
        if (coeff == 0) continue;

        if (!first) {
            if (coeff > 0) ss << "+";
        }

        if (exp == 0) ss << coeff;
        else if (coeff == 1) ss << "x";
        else if (coeff == -1) ss << "-x";
        else {
            ss << coeff << "x";
        }

        if (exp > 1) ss << "^" << exp;
        first = false;
    }

    string res = ss.str();
    return res.empty() ? "0" : res;
}

Polynomial Polynomial::add(const Polynomial& other) const {
    checkAndAdoptPendingData(this);
    checkAndAdoptPendingData(&other);

    Polynomial result;
    map<int,int> resData;

    {
        lock_guard<mutex> lock(storageMutex);

        auto it1 = polyStorage.find(this);
        if (it1 != polyStorage.end()) {
            for (auto& term : it1->second) {
                long long sum = (long long)resData[term.first] + term.second;
                if (sum > INT_MAX) resData[term.first] = INT_MAX;
                else if (sum < INT_MIN) resData[term.first] = INT_MIN;
                else resData[term.first] = (int)sum;
            }
        }

        auto it2 = polyStorage.find(&other);
        if (it2 != polyStorage.end()) {
            for (auto& term : it2->second) {
                long long sum = (long long)resData[term.first] + term.second;
                if (sum > INT_MAX) resData[term.first] = INT_MAX;
                else if (sum < INT_MIN) resData[term.first] = INT_MIN;
                else resData[term.first] = (int)sum;
            }
        }

        for (auto it = resData.begin(); it != resData.end(); )
            it->second == 0 ? it = resData.erase(it) : ++it;

        polyStorage[&result] = resData;
        polyInitialized[&result] = true;
        lastResultData = resData;
        hasLastResult = true;
    }

    return result;
}

Polynomial Polynomial::multiply(const Polynomial& other) const {
    checkAndAdoptPendingData(this);
    checkAndAdoptPendingData(&other);

    Polynomial result;
    map<int,int> resData;

    {
        lock_guard<mutex> lock(storageMutex);

        auto it1 = polyStorage.find(this);
        auto it2 = polyStorage.find(&other);
        if (it1 != polyStorage.end() && it2 != polyStorage.end()) {
            for (auto& t1 : it1->second) {
                for (auto& t2 : it2->second) {
                    long long exp = (long long)t1.first + t2.first;
                    if (exp < 0 || exp > 10000) continue;
                    long long sum = (long long)t1.second * t2.second + resData[(int)exp];

                    if (sum > INT_MAX) resData[(int)exp] = INT_MAX;
                    else if (sum < INT_MIN) resData[(int)exp] = INT_MIN;
                    else resData[(int)exp] = (int)sum;
                }
            }
        }

        for (auto it = resData.begin(); it != resData.end(); )
            it->second == 0 ? it = resData.erase(it) : ++it;

        polyStorage[&result] = resData;
        polyInitialized[&result] = true;
        lastResultData = resData;
        hasLastResult = true;
    }

    return result;
}

Polynomial Polynomial::derivative() const {
    checkAndAdoptPendingData(this);

    Polynomial result;
    map<int,int> resData;

    {
        lock_guard<mutex> lock(storageMutex);

        auto it = polyStorage.find(this);
        if (it != polyStorage.end()) {
            for (auto& term : it->second) {
                int exp = term.first;
                int coeff = term.second;
                if (exp <= 0) continue;

                long long newCoeff = (long long)coeff * exp;
                int newExp = exp - 1;
                if (newExp < 0 || newExp > 10000) continue;

                if (newCoeff > INT_MAX) resData[newExp] = INT_MAX;
                else if (newCoeff < INT_MIN) resData[newExp] = INT_MIN;
                else if (newCoeff != 0) resData[newExp] = (int)newCoeff;
            }
        }

        for (auto it = resData.begin(); it != resData.end(); )
            it->second == 0 ? it = resData.erase(it) : ++it;

        polyStorage[&result] = resData;
        polyInitialized[&result] = true;
        lastResultData = resData;
        hasLastResult = true;
    }

    return result;
}