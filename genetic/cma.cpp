#include <genetic.hpp>

CMA::CMA(int size)
{
    std::cout << "initializing population" << std::endl;
    population = std::vector<std::shared_ptr<Planet>>(size);
    nextGeneration = std::vector<std::shared_ptr<Planet>>(size * 2);
    fitnessValues = std::vector(size, 0.0f);
    for (int i = 0; i < size; i++)
    {
        std::cout << "initializing individual " << i << std::endl;
        population[i] = Planet::sphere(14, 14, 2.0f);
        //for (int j = 0; j < 15; j++)
            //population[i]->mutate(0.3f, 1.0f, 0.02f);
    }
}

void CMA::mutation()
{
    int successfulMutations = 0;
    for (int i = 0; i < population.size(); i++)
    {
        for (int j = 0; j < 10; j++)
        {
            if (population[i]->mutate(0.2f, 0.7f, 0.01f))
            {
                successfulMutations++;
                break;
            }
        }
    }
    std::cout << "mutation done; successful mutations: " << successfulMutations << std::endl;
}
void CMA::crossover()
{
    int successfulCrossovers = 0;
    for (int i = 0; i < nextGeneration.size(); i++)
    {
        int p1 = rand() % population.size();
        int p2 = rand() % population.size();
        auto child = std::make_shared<Planet>(*population[p1]);
        if (population[p1]->uniformCrossover(*population[p2], *child, 0.5f, 0.01f))
        {
            successfulCrossovers++;
        }
        nextGeneration[i] = child;
    }
    std::cout << "successful crossovers: " << successfulCrossovers << std::endl;
}

void CMA::selection()
{
    for (int i = 0; i < nextGeneration.size(); i++)
    {
        fitnessValues[i] = fitness(*nextGeneration[i]);
    }

    float totalFitness = 0.0f;
    for (auto f : fitnessValues) totalFitness += f;

    for (int i = 0; i < population.size(); i++)
    {
        float pick = static_cast<float>(rand()) / RAND_MAX * totalFitness;
        float cumulative = 0.0f;
        int selected = 0;
        for (int j = 0; j < fitnessValues.size(); ++j) {
            cumulative += fitnessValues[j];
            if (cumulative >= pick) {
                selected = j;
                break;
            }
        }
        population[i] = nextGeneration[selected];
    }
}

float CMA::fitness(const Planet& individual)
{
    // get positions
    auto sampleSize = 32;

    auto step = 1.0f / static_cast<float>(sampleSize);
    auto pairs = Planet::pairs(step, step);
    auto positions = individual.positions(pairs);
    auto normals = individual.normals(pairs);
    // compute gravity values
    auto mesh = Mesh::fromPlanet(individual, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), step);
    auto gc = GravityAdapter::GravityComputer(*mesh, 32);
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
    return fitnessValue / d;
}