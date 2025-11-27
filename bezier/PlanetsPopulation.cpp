#include <PlanetsPopulation.hpp>

PlanetsPopulation::PlanetsPopulation(
                                     const std::vector<std::shared_ptr<Planet>>& planets,
                                     float radius, int uSize, int vSize
                                     ) :
    _uSize(uSize), _vSize(vSize)
{
    for (int i = 0; i < planets.size(); i++) {
        _planets.push_back(*planets[i]);
    }
}

const std::vector<Planet>& PlanetsPopulation::planets() const {
    return _planets;
}

float PlanetsPopulation::radius() const {
    return _radius;
}
int PlanetsPopulation::uSize() const {
    return _uSize;
}
int PlanetsPopulation::vSize() const {
    return _vSize;
}
