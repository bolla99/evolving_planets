#include <vector>

#include "Planet.hpp"
#include <Mesh.hpp>
#include <GravityAdapter.hpp>
#include <PlanetsPopulation.hpp>

template <typename T>
class IEvolutionaryAlgorithm
{
public:
    virtual ~IEvolutionaryAlgorithm() = default;

    
    // return true if it should be called again, false when initialized is done 
    virtual bool initialize() = 0;
    virtual void loop();

    virtual void mutation() = 0;
    virtual void crossover() = 0;
    virtual void selection() = 0;
    virtual float fitness(const T& individual) = 0;

    virtual bool shouldTerminate() const = 0;

    virtual float getDiversity(int i) const = 0;
    virtual std::vector<float> getDiversityValues() const = 0;
    virtual float getMeanDiversity() const = 0;
    virtual std::vector<std::vector<float>> getFitnessValues() const = 0;
    virtual std::vector<float> getMeanFitness() const = 0;
    virtual float getLastMeanFitness() const = 0;
    virtual std::vector<float> getLastFitnessValues() const = 0;
    virtual std::vector<float> getMeanErrors() const = 0;

    virtual int currentIndividualBeingInitialized() const = 0;
    virtual bool hasInitialized() const = 0;

    std::vector<std::shared_ptr<T>> population;
    std::vector<std::shared_ptr<T>> nextGeneration;
    std::vector<std::vector<float>> fitnessValues;
    std::vector<float> diversityValues;

    int epoch = 0;
    std::vector<float> meanFitness;
    float meanDiversity = 0.0f;
    float meanError = 0.0f;
    
    virtual std::string log() const = 0;
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
        float autointersectionStep = 0.01f,
        bool immigrationReplaceWithMutatedSphere = false,
        int nImmigrationMutations = 30,
        float diversityLimit = 0.1f,
        float fitnessThreshold = 0.001f,
        int epochWithNoImprovement = 20,
        int maxIterations = 1000,
        bool adaptiveMutationRate = false,
        int fitnessType = 0,
        float distanceFromSurface = 0.0f,
        int immigrationType = 0
        );

    PlanetGA& nParallels(int nParallels) { _nParallels = nParallels; return *this; }
    PlanetGA& nMeridians(int nMeridians) { _nMeridians = nMeridians; return *this; }
    PlanetGA& radius(float radius) { _radius = radius; return *this; }
    int nParallels() { return _nParallels; }
    int nMeridians() { return _nMeridians; }
    float radius() { return _radius; }
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
    PlanetGA& immigrationReplaceWithMutatedSphere(bool immigrationReplaceWithMutatedSphere) { _immigrationReplaceWithMutatedSphere = immigrationReplaceWithMutatedSphere; return *this; }
    PlanetGA& nImmigrationMutations(int nImmigrationMutations) { _nImmigrationMutations = nImmigrationMutations; return *this; }
    PlanetGA& diversityLimit(float diversityLimit) { _diversityLimit = diversityLimit; return *this; }
    PlanetGA& fitnessThreshold(float fitnessThreshold) { _fitnessThreshold = fitnessThreshold; return *this; }
    PlanetGA& epochWithNoImprovement(int epochWithNoImprovement) { _epochWithNoImprovement = epochWithNoImprovement; return *this; }
    PlanetGA& maxIterations(int maxIterations) { _maxIterations = maxIterations; return *this; }
    PlanetGA& adaptiveMutationRate(bool adaptiveMutationRate) { _adaptiveMutationRate = adaptiveMutationRate; return *this; }
    PlanetGA& fitnessType(int fitnessType) { _fitnessType = fitnessType; return *this; }
    PlanetGA& distanceFromSurface(float distanceFromSurface) { _distanceFromSurface = distanceFromSurface; return *this; }
    PlanetGA& immigrationType(int immigrationType) { _immigrationType = immigrationType; return *this; }

    void loop() override;
    bool initialize() override;
    void mutation() override;
    void crossover() override;
    void selection() override;
    float fitness(const Planet& individual) override;

    bool shouldTerminate() const override;

    float getDiversity(int i) const override;
    std::vector<float> getDiversityValues() const override;
    float getMeanDiversity() const override;
    std::vector<std::vector<float>> getFitnessValues() const override;
    std::vector<float> getMeanFitness() const override;
    float getLastMeanFitness() const override;
    std::vector<float> getLastFitnessValues() const override;
    std::vector<float> getMeanErrors() const override;


    int currentIndividualBeingInitialized() const override;
    bool hasInitialized() const override;
    
    bool initByPopulation(const PlanetsPopulation& p);
    
    std::string log() const override;

private:
    int _size;
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
    
    int _immigrationType;
    bool _immigrationReplaceWithMutatedSphere;
    int _nImmigrationMutations;

    int _fitnessType;
    float _distanceFromSurface;

    float _diversityLimit;
    float _fitnessThreshold;
    int  _epochWithNoImprovement;
    int _currentEpochsWithoutImprovement = 0;
    int _maxIterations;

    int _currentIndividualBeingInitialized = 0;
    bool _initialized = false;

    bool _adaptiveMutationRate;
    
    void updateFitness();
    void updateDiversity();
    void updateError();
    void updateTerminationData();
    void handleImmigration();
};


