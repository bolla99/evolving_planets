#include <genetic.hpp>
#include <assert.h>
#include <ctime>

template <typename T>
void IEvolutionaryAlgorithm<T>::loop() {
    std::cout << "Epoch: " << epoch << std::endl;
    // Implement the main loop of the genetic algorithm
    mutation();
    crossover();
    selection();
}


PlanetGA::PlanetGA(
    int size,
    int nParallels,
    int nMeridians,
    float radius,
    int nInitMutations,
    bool immigration,
    int immigrationNewIndividuals,
    float mutationScale,
    float mutationMinDistance,
    float mutationMaxDistance,
    int mutationAttempts,
    CrossoverType crossoverType,
    int crossoverAttempts,
    bool crossoverFallbackToContinuous,
    int crossoverFallbackAttempts,
    float diversityCoefficient,
    int gravityComputationSampleSize,
    int gravityComputationTubesResolution,
    float autointersectionStep,
    bool immigrationReplaceWithMutatedSphere,
    int nImmigrationMutations,
    float diversityLimit,
    float fitnessThreshold,
    int epochWithNoImprovement,
    int maxIterations,
    bool adaptiveMutationRate,
    int fitnessType,
    float distanceFromSurface,
    int immigrationType
    ) : _size(size),
        _nParallels(nParallels),
        _nMeridians(nMeridians),
        _radius(radius),
        _nInitMutations(nInitMutations),
        _immigration(immigration),
        _immigrationNewIndividuals(immigrationNewIndividuals),
        _mutationScale(mutationScale),
        _mutationMinDistance(mutationMinDistance),
        _mutationMaxDistance(mutationMaxDistance),
        _mutationAttempts(mutationAttempts),
        _crossoverType(crossoverType),
        _crossoverAttempts(crossoverAttempts),
        _crossoverFallbackToContinuous(crossoverFallbackToContinuous),
        _crossoverFallbackAttempts(crossoverFallbackAttempts),
        _diversityCoefficient(diversityCoefficient),
        _gravityComputationSampleSize(gravityComputationSampleSize),
        _gravityComputationTubesResolution(gravityComputationTubesResolution),
        _autointersectionStep(autointersectionStep),
        _immigrationReplaceWithMutatedSphere(immigrationReplaceWithMutatedSphere),
        _nImmigrationMutations(nImmigrationMutations),
        _diversityLimit(diversityLimit),
        _fitnessThreshold(fitnessThreshold),
        _epochWithNoImprovement(epochWithNoImprovement),
        _maxIterations(maxIterations),
        _adaptiveMutationRate(adaptiveMutationRate),
        _fitnessType(fitnessType),
        _distanceFromSurface(distanceFromSurface),
        _immigrationType(immigrationType)
{
    // initialize population and next generation vectors
    population = std::vector<std::shared_ptr<Planet>>(_size);
    for (int i = 0; i < _size; i++) { population[i] = Planet::sphere(_nParallels, _nMeridians, _radius); }
    nextGeneration = std::vector<std::shared_ptr<Planet>>(_size);
    
    // init empty fitness values
    fitnessValues = std::vector<std::vector<float>>();
    meanFitness = std::vector<float>();
}

bool PlanetGA::initialize()
{
    if (_initialized) { return false; }
    
    for (int j = 0; j < _nInitMutations; j++)
    {
        population[_currentIndividualBeingInitialized]->mutate(_mutationMinDistance, _mutationMaxDistance, _autointersectionStep);
    }
    if (_currentIndividualBeingInitialized == population.size() - 1) {
        updateFitness();
        updateDiversity();
        
        _initialized = true;
        return false;
    }
    _currentIndividualBeingInitialized++;
    return true;
}

// immigration -> super.loop -> update analysis data -> update termination data
void PlanetGA::loop()
{
    handleImmigration();
    IEvolutionaryAlgorithm::loop();

    updateDiversity();
    updateFitness();
    updateTerminationData();
    updateError();
    epoch++;
}


void PlanetGA::mutation()
{
    auto successfulMutations = 0;
    for (int i = 0; i < population.size(); i++)
    {
        for (int j = 0; j < _mutationAttempts; j++) {
            // random three individuals
            // the base planet must not be the same position of the target mutator position
            int r1 = rand() % population.size();
            while (r1 == i) r1 = rand() % population.size();
            int r2 = rand() % population.size();
            while (r1 == r2) r2 = rand() % population.size();
            int r3 = rand() % population.size();
            while (r1 == r3 || r2 == r3) r3 = rand() % population.size();

            // COPY population[r1] before mutation
            auto planetR1 = std::make_shared<Planet>(*population[r1]);

            if (planetR1->differentialMutate(
                *population[r2],
                *population[r3],
                _mutationScale, // scale factor
                _autointersectionStep // auto intersection step
            ))
            {
                //planetR1->polesSmoothing();
                successfulMutations++;
                nextGeneration[i] = planetR1;
                break;
            }
        }
        // if failed -> put next generation placeholder to avoid nullptr on first epoch if every mutation fails
        nextGeneration[i] = Planet::sphere(_nParallels, _nMeridians);
    }
    std::cout << "mutation done with success: " << 100.0f * static_cast<float>(successfulMutations) / static_cast<float>(population.size()) << "%" << std::endl;
}

void PlanetGA::crossover()
{
    auto successfulCrossovers = 0;
    auto fallbackCrossovers = 0;

    for (int i = 0; i < population.size(); i++)
    {
        auto success = false;
        auto child = std::make_shared<Planet>(*population[i]);

        auto step = 0.5f / static_cast<float>(_crossoverAttempts);
        for (int j = 0; j < _crossoverAttempts; j++) {
            bool crossoverResult = false;
            crossoverResult = population[i]->uniformCrossover(
                *nextGeneration[i],
                *child,
                0.5f - step * static_cast<float>(j),
                _autointersectionStep
                );

            if (crossoverResult)
            {
                successfulCrossovers++;
                nextGeneration[i] = child;
                success = true;
                break;
            }
        }

        if (!success && _crossoverFallbackToContinuous)
        {
            for (int j = 0; j < _crossoverFallbackAttempts; j++) {
                if (population[i]->continuousCrossover(
                    *nextGeneration[i],
                    *child,
                    0.5f,
                    _autointersectionStep
                ))
                {
                    success = true;
                    fallbackCrossovers++;
                    nextGeneration[i] = child;
                    break;
                }
            }
        }
        if (!success)
        {
            for (int k = 0; k < _nInitMutations; k++)
            {
                nextGeneration[i]->mutate(_mutationMinDistance, _mutationMaxDistance, _autointersectionStep);
            }
        }
    }
    std::cout << "crossover done with success: " << 100.0f * static_cast<float>(successfulCrossovers) / static_cast<float>(population.size()) << "%" << std::endl;
    std::cout << "crossover fallback done with success: " << 100.0f * static_cast<float>(fallbackCrossovers) / static_cast<float>(population.size()) << "%" << std::endl;
    std::cout << "total crossover success: " << 100.0f * static_cast<float>(successfulCrossovers + fallbackCrossovers) / static_cast<float>(population.size()) << "%" << std::endl;
}

void PlanetGA::selection()
{
    auto diversities = Planet::minDiversities(population);
    auto nextGenerationDiversities = Planet::minDiversities(population);
    for (int i = 0; i < population.size(); i++)
    {
        auto xFitness = fitness(*population[i]);
        float xFitnessWithDiversity = xFitness - _diversityCoefficient * diversities[i];

        auto uFitness = fitness(*nextGeneration[i]);
        float uFitnessWithDiversity = uFitness - _diversityCoefficient * nextGenerationDiversities[i];

        if (xFitnessWithDiversity > uFitnessWithDiversity)
        {
            population[i] = nextGeneration[i];
            //fitnessValues[i] = uFitness;
        }
        else
        {
            //fitnessValues[i] = xFitness;
        }
    }
}

float PlanetGA::fitness(const Planet& individual)
{
    // get positions
    auto sampleSize = _gravityComputationSampleSize;

    auto step = 1.0f / static_cast<float>(sampleSize);
    auto pairs = Planet::pairs(step, step);
    auto positions = individual.positions(pairs);
    auto normals = individual.normals(pairs);
    // compute gravity values
    auto mesh = Mesh::fromPlanet(individual, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), step);
    auto gc = GravityAdapter::GravityComputer(*mesh, _gravityComputationTubesResolution);
    auto fitnessValue = 0.0f;
    auto d = 0;
    auto centerOfMass = gc.massCenter();
    if (_fitnessType == 0)
    {
        auto gravities = gc.getGravitiesGPU(positions);
        for (int i = 0; i < positions.size(); i++)
        {
            auto newFitness = glm::dot(glm::normalize(-normals[i]), glm::normalize(gravities[i]));
            if (std::isnan(newFitness))
            {
                continue;
            }
            d++;
            fitnessValue += newFitness;
        }
    }
    else if (_fitnessType == 1)
    {
        for (int i = 0; i < positions.size(); i++)
        {
            positions[i] -= _distanceFromSurface * (centerOfMass - positions[i]);
        }
        auto gravities = gc.getGravitiesGPU(positions);
        for (int i = 0; i < positions.size(); i++)
        {
            auto newFitness = glm::dot(glm::normalize(centerOfMass - positions[i]), glm::normalize(gravities[i]));
            if (std::isnan(newFitness))
            {
                continue;
            }
            d++;
            fitnessValue += newFitness;
        }

    }
    //std::cout << "fitness value: " << fitnessValue << std::endl;
    //std::cout << "positions size: " << positions.size() << std::endl;
    if (std::isnan(fitnessValue / static_cast<float>(d)))
        throw std::runtime_error("NaN fitness value");
    return fitnessValue / static_cast<float>(d);
}

bool PlanetGA::shouldTerminate() const
{
    if (epoch > _maxIterations) return true;
    if (meanDiversity < _diversityLimit) return true;
    if (_currentEpochsWithoutImprovement >= _epochWithNoImprovement) return true;
    return false;
}

float PlanetGA::getDiversity(int i) const
{
    if (i < 0 || i >= population.size()) { throw std::runtime_error("Invalid index"); }
    else { return diversityValues[i];}
}
std::vector<float> PlanetGA::getDiversityValues() const
{
    return diversityValues;
}
float PlanetGA::getMeanDiversity() const
{
    return meanDiversity;
}
std::vector<std::vector<float>> PlanetGA::getFitnessValues() const
{
    return fitnessValues;
}
std::vector<float> PlanetGA::getMeanFitness() const
{
    return meanFitness;
}

int PlanetGA::currentIndividualBeingInitialized() const
{
    return _currentIndividualBeingInitialized;
}
bool PlanetGA::hasInitialized() const
{
    return _initialized;
}

// to be called after construction in place of initialize() cycle by feeding
// with a PlanetsPopulation object
bool PlanetGA::initByPopulation(const PlanetsPopulation& p) {
    assert(!_initialized);
    _size = static_cast<int>(p.planets().size());
    _nParallels = p.vSize();
    _nMeridians = p.uSize();
    _radius = p.radius();
    population = std::vector<std::shared_ptr<Planet>>(_size);
    for (int i = 0; i < _size; i++) {
        population[i] = std::make_shared<Planet>(p.planets()[i]);
    }
    // update next generation size
    nextGeneration = std::vector<std::shared_ptr<Planet>>(_size);
    updateFitness();
    updateDiversity();
    _initialized = true;
    _currentIndividualBeingInitialized = _size;
    return true;
}

// update fitness values
void PlanetGA::updateFitness() {
    // compute new fitness values and new mean fitness
    auto newFitnessValues = std::vector<float>(population.size());
    auto newMeanFitness = 0.0f;
    for (int i = 0; i < population.size(); i++) {
        auto value = fitness(*population[i]);
        newFitnessValues[i] = value;
        newMeanFitness += newFitnessValues[i];
    }
    newMeanFitness /= population.size();
    
    // push back values for past epoch
    fitnessValues.push_back(newFitnessValues);
    meanFitness.push_back(newMeanFitness);
}

// update diversity values and mean diversity
void PlanetGA::updateDiversity() {
    diversityValues = Planet::minDiversities(population);
    for (int i = 0; i < diversityValues.size(); i++) meanDiversity += diversityValues[i];
    meanDiversity /= static_cast<float>(diversityValues.size());
}

float PlanetGA::getLastMeanFitness() const {
    return meanFitness[meanFitness.size() - 1];
}

std::vector<float> PlanetGA::getLastFitnessValues() const {
    return fitnessValues[fitnessValues.size() - 1];
}
   
void PlanetGA::updateError() {
    // update error value
    for (int i = 0; i < population.size(); i++)
    {
        auto diff = fitnessValues[epoch - 1][i] - meanFitness[epoch - 1];
        meanError += diff;
    }
    meanError /= static_cast<float>(population.size());
}

void PlanetGA::updateTerminationData() {
    // update termination data
    assert(epoch == meanFitness.size() - 1);
    if (epoch >= 1) {
        if (abs(meanFitness[epoch] - meanFitness[epoch - 1]) < _fitnessThreshold)
            _currentEpochsWithoutImprovement++;
        else _currentEpochsWithoutImprovement = 0;
    }
}

void PlanetGA::handleImmigration() {
    if (!_immigration) return;
    
    if (_immigrationType == 0) {
        for (int i = 0; i < _immigrationNewIndividuals; i++) {
            int id = rand() % population.size();
            if (_immigrationReplaceWithMutatedSphere)
                population[id] = Planet::sphere(_nParallels, _nMeridians, _radius);
            for (int j = 0; j < _nInitMutations; j++)
                population[id]->mutate(_mutationMinDistance, _mutationMaxDistance, _autointersectionStep);
        }
        return;
    }
    
    if (_immigrationType == 1) {
        
        // third option -> mutate already existing planets, like the previous one, but choose the one with the highest
        // similarity with respect to the rest of the population
        auto indices = std::vector<int>(population.size());
        for (int i = 0; i < population.size(); i++) indices[i] = i;
        auto sortedIndices = std::ranges::partial_sort(
                                                       indices,
                                                       indices.begin() + _immigrationNewIndividuals,
                                                       [this](int a, int b) { return diversityValues[a] < diversityValues[b]; }
                                                       );
        // mutate the _immigrationNewIndividuals individuals with minor diversity
        for (int i = 0; i < _immigrationNewIndividuals; i++)
        {
            if (_immigrationReplaceWithMutatedSphere)
                population[indices[i]] = Planet::sphere(_nParallels, _nMeridians, _radius);
            
            // perform _nInitMutations
            for (int k = 0; k < _nImmigrationMutations; k++)
            {
                population[indices[i]]->mutate(_mutationMinDistance, _mutationMaxDistance, _autointersectionStep);
            }
        }
        return;
    }
}

std::vector<float> PlanetGA::getMeanErrors() const {
    auto errors = std::vector<float>(fitnessValues.size());
    for (int i = 0; i < fitnessValues.size(); i++) {
        for(int j = 0; j < population.size(); j++) {
            errors[i] += fabs((fitnessValues[i][j] - meanFitness[i]));
        }
        errors[i] /= population.size();
    }
    return errors;
}

float PlanetGA::getLastMeanError() const {
    auto size = fitnessValues.size();
    float error = 0.0f;
    for(int j = 0; j < population.size(); j++) {
        error += fabs((fitnessValues[size - 1][j] - meanFitness[size - 1]));
    }
    error /= population.size();
    return error;
}


std::string PlanetGA::log() const {
    std::string s = "";
    auto date = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    s += "algorithm executed at: " + std::string(std::ctime(&date)) + "\n";
    s += "PARAMETERS:\n\n";
    s += "size: " + std::to_string(_size) + "\n";
    s += "nParallels: " + std::to_string(_nParallels) + "\n";
    s += "nMeridians: " + std::to_string(_nMeridians) + "\n";
    s += "radius: " + std::to_string(_radius) + "\n";
    s += "nInitMutations: " + std::to_string(_nInitMutations) + "\n";
    s += "immigration: " + std::to_string(_immigration) + "\n";
    s += "immigration new individuals: " + std::to_string(_immigrationNewIndividuals) + "\n";
    s += "mutation scale: " + std::to_string(_mutationScale) + "\n";
    s += "mutation min distance: " + std::to_string(_mutationMinDistance) + "\n";
    s += "mutation max distance: " + std::to_string(_mutationMaxDistance) + "\n";
    s += "mutation attempts: " + std::to_string(_mutationAttempts) + "\n";
    s += "crossover attempts: " + std::to_string(_crossoverAttempts) + "\n";
    s += "crossover fallback to continuous: " + std::to_string(_crossoverFallbackToContinuous) + "\n";
    s += "crossover fallback attempts: " + std::to_string(_crossoverFallbackAttempts) + "\n";
    s += "diversity coefficient: " + std::to_string(_diversityCoefficient) + "\n";
    s += "gravity computation sample size: " + std::to_string(_gravityComputationSampleSize) + "\n";
    s += "gravity computation tubes resolution: " + std::to_string(_gravityComputationTubesResolution) + "\n";
    s += "auto intersection step: " + std::to_string(_autointersectionStep) + "\n";
    
    s += "immigration type: " + std::to_string(_immigrationType) + "\n";
    s += "immigration replace with mutated sphere: " + std::to_string(_immigrationReplaceWithMutatedSphere) + "\n";
    s += "n immigration mutations: " + std::to_string(_nImmigrationMutations) + "\n";

    s += "fitness type: " + std::to_string(_fitnessType) + "\n";
    s += "distance form surface: " + std::to_string(_distanceFromSurface) + "\n";

    s += "diversity limit: " + std::to_string(_diversityLimit) + "\n";
    s += "fitness threshold: " + std::to_string(_fitnessThreshold) + "\n";
    s += "epoch with no improvement: " + std::to_string(_epochWithNoImprovement) + "\n";
    s += "max iterations: " + std::to_string(_maxIterations) + "\n";

    s += "adaptive mutation rate: " + std::to_string(_adaptiveMutationRate) + "\n";
    
    s += "\nEPOCHS: " + std::to_string(fitnessValues.size()) + "\n";
    s += "\n\nFITNESS VALUES:\n\n";
    for(int i = 0; i < fitnessValues.size(); i++) {
        for (int j = 0; j < fitnessValues[i].size(); j++) {
            s += std::to_string(fitnessValues[i][j]) + " ";
        }
        s += "\n";
    }
    s += "\n\nMEAN FITNESS:\n\n";
    for (int i = 0; i < meanFitness.size(); i++) {
        s += std::to_string(meanFitness[i]) + "\n";
    }
    
    return s;
}





