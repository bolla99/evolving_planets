#include <vector>

#include "Planet.hpp"
#include <Mesh.hpp>
#include <GravityAdapter.hpp>

template <typename T>
class IEvolutionaryAlgorithm
{
public:
    virtual ~IEvolutionaryAlgorithm() = default;

    virtual void loop();


    std::vector<std::shared_ptr<T>> population;
    std::vector<std::shared_ptr<T>> nextGeneration;

    std::vector<float> fitnessValues;

    virtual void mutation() = 0;
    virtual void crossover() = 0;
    virtual void selection() = 0;
    virtual float fitness(const T& individual) = 0;

    int epoch = 0;
    float meanFitness = 0.0f;
    float lastMeanFitness = 0.0f;
    float meanError = 0.0f;
};

enum CrossoverType
{
    Continuous,
    Uniform,
    ParallelWiseUniform
};
class PlanetGA : public IEvolutionaryAlgorithm<Planet>
{
public:
    explicit PlanetGA(
        int size,
        int nParallels = 14,
        int nMeridians = 10,
        float radius = 2.0f,
        int nInitMutations = 30,
        bool immigration = true,
        int immigrationNewIndividuals = 3,
        float mutationScale = 0.2f,
        float mutationMinDistance = 0.3f,
        float mutationMaxDistance = 1.3f,
        int mutationAttempts = 3,
        CrossoverType crossoverType = CrossoverType::ParallelWiseUniform,
        int crossoverAttempts = 3,
        bool crossoverFallbackToContinuous = true,
        int crossoverFallbackAttempts = 3,
        float diversityCoefficient = 0.3f,
        int gravityComputationSampleSize = 32,
        int gravityComputationTubesResolution = 32,
        float autointersectionStep = 0.01f
        );

    PlanetGA& nParallels(int nParallels) { _nParallels = nParallels; return *this; }
    PlanetGA& nMeridians(int nMeridians) { _nMeridians = nMeridians; return *this; }
    PlanetGA& radius(float radius) { _radius = radius; return *this; }
    PlanetGA& nInitMutations(int nInitMutations) { _nInitMutations = nInitMutations; return *this; }
    PlanetGA& immigration(bool immigration) { _immigration = immigration; return *this; }
    PlanetGA& immigrationNewIndividuals(int immigrationNewIndividuals) { _immigrationNewIndividuals = immigrationNewIndividuals; return *this; }
    PlanetGA& mutationScale(float mutationScale) { _mutationScale = mutationScale; return *this; }
    PlanetGA& mutationMinDistance(float mutationMinDistance) { _mutationMinDistance = mutationMinDistance; return *this; }
    PlanetGA& mutationMaxDistance(float mutationMaxDistance) { _mutationMaxDistance = mutationMaxDistance; return *this; }
    PlanetGA& mutationAttempts(int mutationAttempts) { _mutationAttempts = mutationAttempts; return *this; }
    PlanetGA& crossoverType(CrossoverType crossoverType) { _crossoverType = crossoverType; return *this; }
    PlanetGA& crossoverAttempts(int crossoverAttempts) { _crossoverAttempts = crossoverAttempts; return *this; }
    PlanetGA& crossoverFallbackToContinuous(bool crossoverFallbackToContinuous) { _crossoverFallbackToContinuous = crossoverFallbackToContinuous; return *this; }
    PlanetGA& crossoverFallbackAttempts(int crossoverFallbackAttempts) { _crossoverFallbackAttempts = crossoverFallbackAttempts; return *this; }
    PlanetGA& diversityCoefficient(float diversityCoefficient) { _diversityCoefficient = diversityCoefficient; return *this; }
    PlanetGA& gravityComputationSampleSize(int gravityComputationSampleSize) { _gravityComputationSampleSize = gravityComputationSampleSize; return *this; }
    PlanetGA& gravityComputationTubesResolution(int gravityComputationTubesResolution) { _gravityComputationTubesResolution = gravityComputationTubesResolution; return *this; }
    PlanetGA& autointersectionStep(float autointersectionStep) { _autointersectionStep = autointersectionStep; return *this; }

    void loop() override;
    void mutation() override;
    void crossover() override;
    void selection() override;
    float fitness(const Planet& individual) override;

private:
    int _nParallels;
    int _nMeridians;
    float _radius;
    int _nInitMutations;
    bool _immigration;
    int _immigrationNewIndividuals;
    float _mutationScale;
    float _mutationMinDistance;
    float _mutationMaxDistance;
    int _mutationAttempts;
    CrossoverType _crossoverType;
    int _crossoverAttempts;
    bool _crossoverFallbackToContinuous;
    int _crossoverFallbackAttempts;
    float _diversityCoefficient;
    int _gravityComputationSampleSize;
    int _gravityComputationTubesResolution;
    float _autointersectionStep;
};

class CMA : public IEvolutionaryAlgorithm<Planet>
{
public:
    explicit CMA(int size);

    void mutation() override;
    void crossover() override;
    void selection() override;
    float fitness(const Planet& individual) override;
};
