#include <genetic.hpp>

template <typename T>
void IEvolutionaryAlgorithm<T>::loop() {

    meanFitness = 0.0f;
    meanError = 0.0f;
    std::cout << "Epoch: " << epoch << std::endl;
    // Implement the main loop of the genetic algorithm
    mutation();
    crossover();
    selection();
    epoch++;
    for (int i = 0; i < population.size(); i++)
    {
        meanFitness += fitnessValues[i];
    }
    meanFitness /= static_cast<float>(population.size());

    for (int i = 0; i < population.size(); i++)
    {
        auto diff = fitnessValues[i] - meanFitness;
        meanError += diff;
    }
    meanError /= static_cast<float>(population.size());
    lastMeanFitness = meanFitness;
    std::cout << "mean fitness: " << meanFitness << std::endl;
    std::cout << std::endl;
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
    float autointersectionStep
    ) : _nParallels(nParallels),
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
        _autointersectionStep(autointersectionStep)
{
    std::cout << "initializing population" << std::endl;
    population = std::vector<std::shared_ptr<Planet>>(size);
    nextGeneration = std::vector<std::shared_ptr<Planet>>(size);
    fitnessValues = std::vector(size, 0.0f);
    for (int i = 0; i < size; i++)
    {
        std::cout << "initializing individual " << i << " ";
        population[i] = Planet::sphere(nParallels, nMeridians, radius);
        //std::cout << "initializing individual " << i << std::endl;
        for (int j = 0; j < nInitMutations; j++)
            population[i]->mutate(mutationMinDistance, mutationMaxDistance, autointersectionStep);

        //population[i]->laplacianSmoothing(0.1f);
        population[i]->polesSmoothing();

        fitnessValues[i] = PlanetGA::fitness(*population[i]);
        std::cout << "with fitness: " << fitnessValues[i] << std::endl;
    }
    auto meanFitness = 0.0f;
    for (int i = 0; i < fitnessValues.size(); i++) meanFitness += fitnessValues[i];
    std::cout << "initialization done with mean fitness: " << meanFitness / static_cast<float>(fitnessValues.size()) << std::endl;
}

void PlanetGA::loop()
{
    if (_immigration)
    {
        for (int i = 0; i < _immigrationNewIndividuals; i++) {
            int id = rand() % population.size();
            population[id] = Planet::sphere(_nParallels, _nMeridians, _radius);
            for (int j = 0; j < _nInitMutations; j++)
                population[id]->mutate(_mutationMinDistance, _mutationMaxDistance, _autointersectionStep);
        }
    }
    IEvolutionaryAlgorithm::loop();
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
                planetR1->polesSmoothing();
                successfulMutations++;
                nextGeneration[i] = planetR1;
                break;
            }
        }
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

        // three attempts
        for (int j = 0; j < _crossoverAttempts; j++) {
            bool crossoverResult = false;
            switch (_crossoverType)
            {
            case Continuous: crossoverResult = population[i]->continuousCrossover(
                *nextGeneration[i],
                *child,
                0.5f,
                _autointersectionStep
                ); break;
            case Uniform: crossoverResult = population[i]->uniformCrossover(
                *nextGeneration[i],
                *child,
                0.5f,
                _autointersectionStep
                ); break;
            case ParallelWiseUniform: crossoverResult = population[i]->parallelWiseUniformCrossover(
                *nextGeneration[i],
                *child,
                0.5f,
                _autointersectionStep
                ); break;
            default: throw std::runtime_error("Unknown crossover type");
            }

            if (crossoverResult)
            {
                successfulCrossovers++;
                nextGeneration[i] = child;
                nextGeneration[i]->polesSmoothing();
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
                    fallbackCrossovers++;
                    nextGeneration[i] = child;
                    nextGeneration[i]->polesSmoothing();
                    break;
                }
            }
        }
    }
    /*
    std::cout << "crossover done with success: " << 100.0f * static_cast<float>(successfulCrossovers) / static_cast<float>(population.size()) << "%" << std::endl;
    std::cout << "crossover fallback done with success: " << 100.0f * static_cast<float>(fallbackCrossovers) / static_cast<float>(population.size()) << "%" << std::endl;
     */
    std::cout << "total crossover success: " << 100.0f * static_cast<float>(successfulCrossovers + fallbackCrossovers) / static_cast<float>(population.size()) << "%" << std::endl;
}

void PlanetGA::selection()
{
    for (int i = 0; i < population.size(); i++)
    {
        auto xFitness = fitness(*population[i]);
        float xFitnessWithDiversity = xFitness;
        float diversity = 0.0f;
        for (int j = 0; j < population.size(); j++) {
            diversity += population[i]->diversity(*population[j]);
        }
        diversity /= static_cast<float>(population.size());
        xFitnessWithDiversity += -_diversityCoefficient * diversity;

        auto uFitness = fitness(*nextGeneration[i]);
        float uFitnessWithDiversity = uFitness;
        diversity = 0.0f;
        for (int j = 0; j < population.size(); j++) {
            diversity += nextGeneration[i]->diversity(*nextGeneration[j]);
        }
        diversity /= static_cast<float>(population.size());
        uFitnessWithDiversity += -_diversityCoefficient * diversity;

        if (xFitnessWithDiversity > uFitnessWithDiversity)
        {
            population[i] = nextGeneration[i];
            fitnessValues[i] = uFitness;
        }
        else
        {
            fitnessValues[i] = xFitness;
        }
    }
    //std::cout << "selection done" << std::endl;
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
    auto gravities = gc.getGravitiesGPU(positions);
    auto d = 0;
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
    //std::cout << "fitness value: " << fitnessValue << std::endl;
    //std::cout << "positions size: " << positions.size() << std::endl;
    if (std::isnan(fitnessValue / static_cast<float>(d)))
        throw std::runtime_error("NaN fitness value");
    return fitnessValue / static_cast<float>(d);
}

