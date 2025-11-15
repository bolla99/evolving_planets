//
// Created by Giovanni Bollati on 07/07/25.
//

#ifndef PERIODICBSPLINE_HPP
#define PERIODICBSPLINE_HPP

#include <vector>
#include <glm/glm.hpp>
#include <iostream>

/**
 * This class represents a B-spline curve.
 * The knot vector is automatically set to 0, 1, ..., n + degree, where n is
 * the number of control points.
 * The class is design
 */
class BSpline {

public:
    BSpline(
        const std::vector<glm::vec3>& controlPoints,
        const std::vector<int>& knots,
        int degree
        ) :
    _controlPoints(controlPoints),
    _knots(knots),
    _degree(degree)
    {
        if (_controlPoints.size() < _degree + 1) {
            throw std::invalid_argument("Not enough control points for the specified degree.");
        }
    }

    static BSpline periodic(std::vector<glm::vec3> controlPoints, int degree)
    {
        auto cp = std::move(controlPoints);
        for (int i = 0; i < degree; ++i)
        {
            cp.push_back(cp[i]); // Repeat first degree points to make it periodic
        }
        return {cp , generateKnots(static_cast<int>(cp.size()), degree, false), degree};
    }

    [[nodiscard]] glm::vec3 evaluate(float t) const
    {
        if (t < 0.0f || t > 1.0f) {
            throw std::out_of_range("Parameter t must be in the range [0, 1].");
        }

        auto span = BSpline::span(t, _knots, _degree);
        auto basis = BSpline::basis(span, t, _knots, _degree);

        // Evaluate the B-spline
        glm::vec3 result(0.0f);
        for (int i = 0; i <= _degree; ++i) {
            result += basis[i] * _controlPoints[span - _degree + 1 + i];
        }

        // debug
        /*
        std::cout << "Evaluating B-spline at t = " << t << ": ";
        std::cout << "U: " << toU(t, _knots, _degree) << ", ";
        std::cout << "Span: " << span << ", ";
        for (const auto& b : basis) {
            std::cout << b << " ";
        }
        std::cout << "Result: " << result.x << ", " << result.y << ", " << result.z << std::endl;
        */

        return result;
    }

    [[nodiscard]] int degree() const { return _degree; }
    [[nodiscard]] int nControlPoints() const { return static_cast<int>(_controlPoints.size()); }

    // STATIC METHOD

    static std::vector<int>  generateKnots(size_t controlPointSize, int degree, int outerRepetitions)
    {
        if (controlPointSize < degree + 1) {
            throw std::invalid_argument("Not enough control points for the specified degree.");
        }
        std::vector<int> knots(controlPointSize + degree -1);
        int nextKnot = 0;
        if (outerRepetitions > 0)
        {
            for (int i = 0; i < outerRepetitions; ++i) {
                knots[i] = 0; // First degree knots
            }
            nextKnot += 1;
            for (int i = outerRepetitions; i < (knots.size() - outerRepetitions); ++i) {
                knots[i] = nextKnot++;
            }
            for (int i = static_cast<int>(knots.size()) - outerRepetitions; i < knots.size(); i++)
            {
                knots[i] = nextKnot;
            }
        }
        else
        {
            for (int i = 0; i < knots.size(); ++i) {
                knots[i] = nextKnot++;
            }
        }
        /*
        std::cout << "Generated knots: ";
        for (const auto& knot : knots) {
            std::cout << knot << " ";
        }
        std::cout << std::endl;
        */
        return knots;
    }
    static std::vector<float> basis(int span, float t, const std::vector<int>& knots, int degree)
    {
        auto u = BSpline::toU(t, knots, degree);
        auto basis = std::vector<float>(degree + 1);
        auto oldBasis = std::vector<float>(degree + 1);

        // initialize values for n = 0
        basis[degree] = 1.0f;
        oldBasis[degree] = 1.0f;
        for (int i = 0; i < degree; ++i) {
            basis[i] = 0.0f;
            oldBasis[i] = 0.0f;
        }
        // recursion level; the 0 step is already done as initialization ( 0, 0, 0, 1 )
        for (int i = 1; i <= degree; i++)
        {
            // basis index
            for (int j = 0; j <= degree; j++)
            {
                // b: index of the basis function on a global numbering
                // while j is relative numbering, b ins on global numbering
                // use b instead of j whenever you must refer to the j-th basis function
                auto b = j + span - degree + 1;
                
                float w1 = 0.0f; // weight for N_j at previous level
                float w2 = 0.0f; // weight for N_j+1 at previous level

                // no left member for the leftmost basis
                if (j > 0)
                {
                    auto d1 = knots[b + i - 1] - knots[b - 1];
                    if (d1 != 0) // if d1 = 0, then w1 must be 0
                        w1 = (u - static_cast<float>(knots[b-1])) / static_cast<float>(d1);
                }
                // no right member for the rightmost basis
                if (j < degree) {
                    auto d2 = knots[b + i] - knots[b];
                    if (d2 != 0)
                        w2 = (static_cast<float>(knots[i+b]) - u) / static_cast<float>(d2);
                }

                basis[j] = w1 * oldBasis[j];
                if (j < degree)
                    basis[j] += w2 * oldBasis[j+1];
            }
            // update oldBasis
            oldBasis = basis;
        }
        return basis;
    }
    static std::vector<float> d1Basis(int span, float t, const std::vector<int>& knots, int degree)
    {
        auto u = BSpline::toU(t, knots, degree);
        auto basis = std::vector<float>(degree + 1);
        auto oldBasis = std::vector<float>(degree + 1);

        // initialize values for n = 0
        basis[degree] = 1.0f;
        oldBasis[degree] = 1.0f;
        for (int i = 0; i < degree; ++i) {
            basis[i] = 0.0f;
            oldBasis[i] = 0.0f;
        }
        // recursion level; the 0 step is already done as initialization ( 0, 0, 0, 1 )
        // last recursion level is skipped, because first derivative
        // is computed by the level = degree - 1 level basis functions
        // after basis functions are computed till the level degree - 1, first
        // derivative is computed as the last step.
        for (int i = 1; i < degree; i++)
        {
            // basis index
            for (int j = 0; j <= degree; j++)
            {
                auto b = j + span - degree + 1;
                float w1 = 0.0f;
                float w2 = 0.0f;

                // no left member for the leftmost basis
                if (j > 0)
                {
                    auto d1 = knots[b + i - 1] - knots[b - 1];
                    if (d1 != 0)
                        w1 = (u - static_cast<float>(knots[b-1])) / static_cast<float>(d1);
                }
                
                // no right member for the rightmost basis
                if (j < degree) {
                    auto d2 = knots[b + i] - knots[b];
                    if (d2 != 0)
                        w2 = (static_cast<float>(knots[i+b]) - u) / static_cast<float>(d2);
                }

                basis[j] = w1 * oldBasis[j];
                if (j < degree) basis[j] += w2 * oldBasis[j+1];
            }
            // update oldBasis
            oldBasis = basis;
        }
        // compute first derivative
        for (int j = 0; j <= degree; j++) {
            
            auto b = j + span - degree + 1;
            float w1 = 0.0f;
            float w2 = 0.0f;
        
            // no left member for the leftmost basis
            if (j > 0)
            {
                auto d1 = knots[b + degree - 1] - knots[b - 1];
                if (d1 != 0)
                {
                    w1 = static_cast<float>(degree) / static_cast<float>(d1);
                }
            }
            // no right member for the rightmost basis
            if (j < degree) {
                auto d2 = knots[b + degree] - knots[b];
                if (d2 != 0)
                {
                    w2 = - static_cast<float>(degree) / static_cast<float>(d2);
                }
            }
            basis[j] = w1 * oldBasis[j];
            if (j < degree) basis[j] += w2 * oldBasis[j+1];
        }
        return basis;
    }
    
    static std::vector<float> d2Basis(int span, float t, const std::vector<int>& knots, int degree)
    {
        auto u = BSpline::toU(t, knots, degree);
        auto basis = std::vector<float>(degree + 1);
        auto oldBasis = std::vector<float>(degree + 1);

        // initialize values for the base recursion step
        basis[degree] = 1.0f;
        oldBasis[degree] = 1.0f;
        for (int i = 0; i < degree; ++i) {
            basis[i] = 0.0f;
            oldBasis[i] = 0.0f;
        }
        // recursion level; the 0 step is already done as initialization ( 0, 0, 0, 1 )
        // the recursion level = degree and degree - 1 are skipped
        // because second derivative is computed
        // using the elements of degree - 2 recursion level
        for (int i = 1; i <= degree - 2; i++)
        {
            // basis index
            for (int j = 0; j <= degree; j++)
            {
                // local index -> gloabal index
                auto b = j + span - degree + 1;
                
                float w1 = 0.0f;
                float w2 = 0.0f;

                if (j > 0)
                {
                    auto d1 = knots[b + i - 1] - knots[b - 1];
                    if (d1 != 0)
                        w1 = (u - static_cast<float>(knots[b-1])) / static_cast<float>(d1);
                }

                if (j < degree) {
                    auto d2 = knots[b + i] - knots[b];
                    if (d2 != 0)
                        w2 = (static_cast<float>(knots[i+b]) - u) / static_cast<float>(d2);
                }

                basis[j] = w1 * oldBasis[j];
                if (j < degree) basis[j] += w2 * oldBasis[j+1];
            }
            // update oldBasis
            oldBasis = basis;
        }
        // compute second derivative
        for (int j = 0; j <= degree; j++) {
            
            auto b = j + span - degree + 1;

            auto A = 0.0f;
            auto B = 0.0f;
            auto C = 0.0f;
            
            if (j > 0) {
                auto dA = (knots[b + degree - 1] - knots[b - 1]) * (knots[b + degree - 2] - knots[b - 1]);
                if (dA != 0)
                    A = static_cast<float>(degree * (degree - 1)) / static_cast<float>(dA);
            }
            
            if (j > 0 and j < degree) {
                auto dB1 = (knots[b + degree - 1] - knots[b]) * (knots[b + degree - 1] - knots[b - 1]);
                auto dB2 = (knots[b + degree] - knots[b]) * (knots[b + degree - 2] - knots[b]);
                if (dB1 != 0)
                    B += static_cast<float>(-degree * (degree - 1)) / static_cast<float>(dB1);
                if (dB2 != 0)
                    B += static_cast<float>(-degree * (degree - 1)) / static_cast<float>(dB2);
            }
            
            if (j < degree) {
                auto dC = (knots[b + degree] - knots[b]) * (knots[b + degree - 1] - knots[b]);
                if (dC != 0)
                    C = static_cast<float>(degree * (degree - 1)) / static_cast<float>(dC);
            }
            
            basis[j] = A * oldBasis[j];
            if (j + 1 < basis.size()) basis[j] += B * oldBasis[j + 1];
            if (j + 2 < basis.size()) basis[j] += C * oldBasis[j + 2];
        }
        return basis;
    }

    // return the knots index of the knot value that precedes the u value corresponding
    // to the t value provided
    static int span(float t, const std::vector<int>& knots, int degree)
    {
        auto u = toU(t, knots, degree);
        int span = degree - 1; // Start with the first span
        while (u >= static_cast<float>(knots[span + 1]) and span < static_cast<int>(knots.size()) - degree - 1) {
            ++span;
        }
        return span;
    }

    static std::vector<glm::vec3> generateCircleControlPoints(float r, glm::vec3 center, int numPoints)
    {
        auto controlPoints = std::vector<glm::vec3>();
        for (int i = 0; i < numPoints; ++i)
        {
            float angle = static_cast<float>(i) * 2.0f * 3.14159265358979323846f / static_cast<float>(numPoints);
            float x = center.x + r * cos(angle);
            float y = center.y + r * sin(angle);
            float z = center.z; // Assuming a 2D circle in the XY plane
            controlPoints.push_back(glm::vec3(x, y, z));
        }
        return controlPoints;
    }

    static BSpline circle(float r, glm::vec3 center, int numPoints)
    {
        auto controlPoints = generateCircleControlPoints(r, center, numPoints);
        return BSpline::periodic(controlPoints, 3);
    }

    [[nodiscard]] const std::vector<glm::vec3>& controlPoints() const { return _controlPoints; }
    [[nodiscard]] const std::vector<int>& knots() const { return _knots; }

private:
    std::vector<glm::vec3> _controlPoints;
    std::vector<int> _knots;
    int _degree = 0;

    // return the u value by setting (0, 1) -> (u[degree-1], u[size-degree])
    static float toU(float t, const std::vector<int>& knots, int degree)
    {
        return (1.0f - t) * static_cast<float>(knots[degree - 1]) + t * static_cast<float>(knots[knots.size() - degree]);
    }
};

#endif //PERIODICBSPLINE_HPP
