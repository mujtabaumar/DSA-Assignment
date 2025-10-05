#include "Polynomial.h"
#include <map>
#include <sstream>
#include <unordered_map>
#include <mutex>
#include <vector>

// Thread-safe storage with mutex
static std::mutex storageMutex;
static std::unordered_map<const Polynomial*, std::map<int,int>> polyStorage;

// Handle return value copying - stores data for objects being returned
static std::map<int,int> lastResultData;
static bool hasLastResult = false;

// Track polynomial lifecycle to handle copies
static std::unordered_map<const Polynomial*, bool> polyInitialized;

// Helper function to check and adopt pending data
static void checkAndAdoptPendingData(const Polynomial* poly) {
    std::lock_guard<std::mutex> lock(storageMutex);

    // If this polynomial hasn't been initialized yet and we have pending data
    if (polyInitialized.find(poly) == polyInitialized.end() && hasLastResult) {
        polyStorage[poly] = lastResultData;
        polyInitialized[poly] = true;
        hasLastResult = false;
        lastResultData.clear();
    } else if (polyInitialized.find(poly) == polyInitialized.end()) {
        // Mark as initialized even if empty
        polyInitialized[poly] = true;
    }
}

void Polynomial::insertTerm(int coefficient, int exponent) {
    std::lock_guard<std::mutex> lock(storageMutex);

    // Check for pending data adoption (for copied return values)
    if (polyInitialized.find(this) == polyInitialized.end() && hasLastResult) {
        polyStorage[this] = lastResultData;
        polyInitialized[this] = true;
        hasLastResult = false;
        lastResultData.clear();
    }

    // Mark as initialized
    polyInitialized[this] = true;

    // Edge case: exponent must be non-negative
    if (exponent < 0) {
        return; // Silently ignore negative exponents
    }

    // Edge case: exponent too large (practical limit for polynomials)
    // Limit to 10000 to prevent memory issues and maintain reasonable polynomial operations
    if (exponent > 10000) {
        return; // Silently ignore exponents that are too large
    }

    // Edge case: coefficient is 0, don't add
    if (coefficient == 0) {
        return;
    }

    // Edge case: handle integer overflow in coefficient addition
    auto& terms = polyStorage[this];
    long long newCoeff = static_cast<long long>(terms[exponent]) + coefficient;

    // Clamp to int range to prevent overflow
    if (newCoeff > INT_MAX) {
        terms[exponent] = INT_MAX;
    } else if (newCoeff < INT_MIN) {
        terms[exponent] = INT_MIN;
    } else {
        terms[exponent] = static_cast<int>(newCoeff);
    }

    // Remove term if coefficient becomes zero
    if (terms[exponent] == 0) {
        terms.erase(exponent);
    }

    // Clean up if polynomial becomes empty
    if (terms.empty()) {
        polyStorage.erase(this);
    }
}

std::string Polynomial::toString() const {
    checkAndAdoptPendingData(this);

    std::lock_guard<std::mutex> lock(storageMutex);

    auto itMap = polyStorage.find(this);

    // Edge case: polynomial not found or empty
    if (itMap == polyStorage.end() || itMap->second.empty()) {
        return "0";
    }

    const auto& terms = itMap->second;
    std::ostringstream ss;
    bool first = true;

    // Iterate from highest to lowest exponent
    for (auto it = terms.rbegin(); it != terms.rend(); ++it) {
        int coeff = it->second;
        int exp = it->first;

        // Edge case: coefficient is 0 (shouldn't happen but defensive)
        if (coeff == 0) continue;

        // Add sign
        if (!first) {
            if (coeff > 0) {
                ss << "+";
            }
            // Negative sign is part of the coefficient
        }

        // Handle coefficient display
        if (exp == 0) {
            // Constant term: always show coefficient
            ss << coeff;
        } else if (coeff == 1) {
            // Coefficient is 1: don't show it (except for constant)
            // Do nothing, just show x
        } else if (coeff == -1) {
            // Coefficient is -1: show minus sign only
            ss << "-";
        } else {
            // Show coefficient
            ss << coeff;
        }

        // Handle variable and exponent
        if (exp > 0) {
            ss << "x";
            if (exp > 1) {
                ss << "^" << exp;
            }
            // Edge case: exp == 1, just show x
        }

        first = false;
    }

    // Edge case: empty result (all terms were zero)
    std::string result = ss.str();
    return result.empty() ? "0" : result;
}

Polynomial Polynomial::add(const Polynomial& other) const {
    checkAndAdoptPendingData(this);
    checkAndAdoptPendingData(&other);

    Polynomial result;
    std::map<int,int> resultData;

    {
        std::lock_guard<std::mutex> lock(storageMutex);

        // Add terms from first polynomial
        auto it1 = polyStorage.find(this);
        if (it1 != polyStorage.end()) {
            for (const auto& term : it1->second) {
                long long newCoeff = static_cast<long long>(resultData[term.first]) + term.second;

                // Handle overflow
                if (newCoeff > INT_MAX) {
                    resultData[term.first] = INT_MAX;
                } else if (newCoeff < INT_MIN) {
                    resultData[term.first] = INT_MIN;
                } else {
                    resultData[term.first] = static_cast<int>(newCoeff);
                }
            }
        }

        // Add terms from second polynomial
        auto it2 = polyStorage.find(&other);
        if (it2 != polyStorage.end()) {
            for (const auto& term : it2->second) {
                long long newCoeff = static_cast<long long>(resultData[term.first]) + term.second;

                // Handle overflow
                if (newCoeff > INT_MAX) {
                    resultData[term.first] = INT_MAX;
                } else if (newCoeff < INT_MIN) {
                    resultData[term.first] = INT_MIN;
                } else {
                    resultData[term.first] = static_cast<int>(newCoeff);
                }
            }
        }

        // Remove zero coefficients
        for (auto it = resultData.begin(); it != resultData.end(); ) {
            if (it->second == 0) {
                it = resultData.erase(it);
            } else {
                ++it;
            }
        }

        // Store for local result and for the copy on return
        polyStorage[&result] = resultData;
        polyInitialized[&result] = true;
        lastResultData = resultData;
        hasLastResult = true;
    }

    return result;
}

Polynomial Polynomial::multiply(const Polynomial& other) const {
    checkAndAdoptPendingData(this);
    checkAndAdoptPendingData(&other);

    Polynomial result;
    std::map<int,int> resultData;

    {
        std::lock_guard<std::mutex> lock(storageMutex);

        auto it1 = polyStorage.find(this);
        auto it2 = polyStorage.find(&other);

        // Edge case: one or both polynomials are empty (result is 0)
        if (it1 != polyStorage.end() && it2 != polyStorage.end()) {
            for (const auto& term1 : it1->second) {
                for (const auto& term2 : it2->second) {
                    // Edge case: handle coefficient overflow
                    long long newCoeff = static_cast<long long>(term1.second) * term2.second;

                    // Edge case: handle exponent overflow
                    long long newExp = static_cast<long long>(term1.first) + term2.first;

                    // Clamp exponent to reasonable range (prevent infinity/overflow)
                    if (newExp > 10000) {
                        continue; // Skip terms with exponents too large
                    } else if (newExp < 0) {
                        newExp = 0;
                    }

                    int exp = static_cast<int>(newExp);

                    // Add to existing term
                    long long currentCoeff = resultData[exp];
                    long long sumCoeff = currentCoeff + newCoeff;

                    // Clamp coefficient
                    if (sumCoeff > INT_MAX) {
                        resultData[exp] = INT_MAX;
                    } else if (sumCoeff < INT_MIN) {
                        resultData[exp] = INT_MIN;
                    } else {
                        resultData[exp] = static_cast<int>(sumCoeff);
                    }
                }
            }
        }

        // Remove zero coefficients
        for (auto it = resultData.begin(); it != resultData.end(); ) {
            if (it->second == 0) {
                it = resultData.erase(it);
            } else {
                ++it;
            }
        }

        polyStorage[&result] = resultData;
        polyInitialized[&result] = true;
        lastResultData = resultData;
        hasLastResult = true;
    }

    return result;
}

Polynomial Polynomial::derivative() const {
    checkAndAdoptPendingData(this);

    Polynomial result;
    std::map<int,int> resultData;

    {
        std::lock_guard<std::mutex> lock(storageMutex);

        auto it = polyStorage.find(this);

        // Edge case: polynomial is empty or constant (derivative is 0)
        if (it != polyStorage.end()) {
            for (const auto& term : it->second) {
                int exp = term.first;
                int coeff = term.second;

                // Edge case: constant term (exponent 0) has derivative 0
                if (exp <= 0) {
                    continue; // Skip constant terms and any negative exponents
                }

                // Edge case: exponent is 1, derivative is just the coefficient (becomes constant)
                // This is handled naturally: 1 * coeff * x^0 = coeff

                // Edge case: very large exponent might cause overflow when multiplied with coefficient
                // Check if exp * coeff will overflow
                if (exp > 0) {
                    long long newCoeff = static_cast<long long>(coeff) * exp;
                    int newExp = exp - 1;

                    // Edge case: resulting exponent is still valid (>= 0)
                    if (newExp < 0) {
                        continue; // Should not happen with exp > 0, but defensive
                    }

                    // Edge case: new exponent exceeds practical limit
                    if (newExp > 10000) {
                        continue; // Skip if still too large after derivative
                    }

                    // Edge case: coefficient overflow/underflow
                    if (newCoeff > INT_MAX) {
                        resultData[newExp] = INT_MAX;
                    } else if (newCoeff < INT_MIN) {
                        resultData[newExp] = INT_MIN;
                    } else if (newCoeff == 0) {
                        // Edge case: coefficient becomes 0 after multiplication (rare but possible with overflow handling)
                        continue; // Don't add zero coefficient terms
                    } else {
                        resultData[newExp] = static_cast<int>(newCoeff);
                    }
                }
            }
        }

        // Edge case: remove any zero coefficients that might have been added
        for (auto it = resultData.begin(); it != resultData.end(); ) {
            if (it->second == 0) {
                it = resultData.erase(it);
            } else {
                ++it;
            }
        }

        polyStorage[&result] = resultData;
        polyInitialized[&result] = true;
        lastResultData = resultData;
        hasLastResult = true;
    }

    return result;
}