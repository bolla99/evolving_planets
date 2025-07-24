//
// Created by Giovanni Bollati on 06/07/25.
//

#ifndef SURFACE_HPP
#define SURFACE_HPP

#include <vector>
#include <glm/glm.hpp>
#include "bezier.hpp"

namespace Bezier
{
    class Patch
    {
    public:
        Patch() = default;

        explicit Patch(const std::vector<std::vector<glm::vec3>>& controlPoints)
            : _controlPoints(controlPoints)
        {
            // enforce that control points form a grid
            if (_controlPoints.empty() || _controlPoints[0].empty())
            {
                throw std::invalid_argument("Control points cannot be empty.");
            }
            int numCol = static_cast<int>(_controlPoints.size());
            for (int i = 1; i < numCol; ++i)
            {
                if (_controlPoints[i].size() != _controlPoints[0].size())
                {
                    throw std::invalid_argument("All columns of control points must have the same number of points.");
                }
            }
        }

        void addColumn(const std::vector<glm::vec3>& column)
        {
            if (_controlPoints.empty())
            {
                _controlPoints.push_back(column);
                return;
            }
            if (column.size() != _controlPoints[0].size())
            {
                throw std::invalid_argument("New column must have the same number of rows as existing columns.");
            }
            _controlPoints.push_back(column);
        }

        [[nodiscard]] glm::vec3 evaluate(float u, float v) const
        {
            if (u < 0.0f || u > 1.0f || v < 0.0f || v > 1.0f)
            {
                throw std::out_of_range("u and v must be in the range [0, 1].");
            }

            if (USize() < 2 || VSize() < 2)
            {
                throw std::invalid_argument("Patch must have at least 2 control points in both dimensions.");
            }

            auto c = Curve();
            for (int i = 0; i < USize(); i++)
            {
                c.add(Curve(_controlPoints[i]).evaluate(v));
            }
            return c.evaluate(u);
        }

        [[nodiscard]] int USize() const
        {
            return static_cast<int>(_controlPoints.size());
        }
        [[nodiscard]] int VSize() const
        {
            if (_controlPoints.empty())
            {
                return 0;
            }
            return static_cast<int>(_controlPoints[0].size());

        }
    private:
        // _controlPoints[0], _controlPoints[1], ... are columns
        std::vector<std::vector<glm::vec3>> _controlPoints;
    };
}

#endif //SURFACE_HPP
