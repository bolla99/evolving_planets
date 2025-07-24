//
// Created by Giovanni Bollati on 04/07/25.
//

#ifndef BEZIER_HPP
#define BEZIER_HPP

#include <glm/glm.hpp>
#include <cmath>
#include <vector>
#include <stdexcept>

namespace Bezier
{
    inline glm::vec3 lerp(const glm::vec3& a, const glm::vec3& b, float t)
    {
        return a + t * (b - a);
    }

    inline int fact(int n)
    {
        if (n < 0 || n > 12) // 12! is the largest factorial that fits in an int
        {
            throw std::out_of_range("Factorial is not defined for negative numbers or numbers greater than 12.");
        }
        int result = 1;
        while (n > 1)
        {
            result *= n;
            --n;
        }
        return result;
    }

    inline float binomial(int n, int k)
    {
        if (k < 0 || n < 0)
        {
            throw std::invalid_argument("n and k must be non-negative integers.");
        }
        if (k > n)
        {
            throw std::invalid_argument("k cannot be greater than n in binomial coefficient calculation.");
        }
        return static_cast<float>(fact(n)) / static_cast<float>((fact(k) * fact(n - k)));
    }

    inline float bernstein(int n, int i, float t)
    {
        return binomial(n, i) * static_cast<float>(pow(t, i)) * static_cast<float>(pow(1 - t, n - i));
    }

    class Curve
    {
    public:
        Curve() = default;
        explicit Curve(const std::vector<glm::vec3>& points) : _points(points) {}

        // factories
        static Curve quadratic(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2)
        {
            Curve curve;
            curve._points = {p0, p1, p2};
            return curve;
        }
        static Curve cubic(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3)
        {
            Curve curve;
            curve._points = {p0, p1, p2, p3};
            return curve;
        }

        Curve& add(const glm::vec3& point)
        {
            _points.push_back(point);
            return *this;
        }

        [[nodiscard]] int degree() const { return static_cast<int>(_points.size()) - 1; }
        [[nodiscard]] glm::vec3 first() const
        {
            if (_points.empty())
            {
                throw std::out_of_range("Curve has no points.");
            }
            return _points.front();
        }
        [[nodiscard]] glm::vec3 last() const
        {
            if (_points.empty())
            {
                throw std::out_of_range("Curve has no points.");
            }
            return _points.back();
        }

        glm::vec3 operator[](int i) const
        {
            if (i < 0 || i >= _points.size())
            {
                throw std::out_of_range("Index out of range in Curve::operator[]");
            }
            return _points[i];
        }

        [[nodiscard]] glm::vec3 evaluate(float t) const
        {
            auto result = glm::vec3(0.0f);
            for (int i = 0; i < _points.size(); i++)
            {
                result += _points[i] * bernstein(static_cast<int>(_points.size()) - 1, i, t);
            }
            return result;
        }

        // b[t0, t1, t2, ...], size(t) == degree
        [[nodiscard]] static glm::vec3 blossom(std::vector<float> t, const Curve& curve)
        {
            if (curve.degree() < 0)
            {
                throw std::invalid_argument("Curve must have at least one point.");
            };
            if (curve.degree() == 0) return curve.first();

            // degree > 0
            if (t.size() != curve.degree())
            {
                throw std::invalid_argument("The size of t must match the degree of the curve.");
            }
            auto newCurve = Curve();
            for (int i = 0; i < curve.degree(); i++)
            {
                auto a = curve[i];
                auto b = curve[i + 1];
                newCurve.add(lerp(a, b, t[0]));
            }
            return blossom(std::vector(t.begin() + 1, t.end()), newCurve);
        }

    private:
        std::vector<glm::vec3> _points;
    };
}

#endif
