# evolving_planets
This project aims to represent planets with the b-spline formalism so that they can become genotypes of a genetic algorithm.
The purpose is to create a collection of planets of different shapes; in particular, the project is meant to be an extension of this project: https://github.com/bolla99/gravity_field_sampling, which consists in a method for computing gravity induced by an arbitrarily shaped planet.
The genetic algorithm will optimise the non-triviality of the planets, i.e. the mean divergence of gravity with respect to the surface normal; the motivation is producing planets that are difficult to land on. 
In addition, this project includes the creation of a basic renderer abstraction and a metal implementation; in the future the essential systems (mainly physics) of a minimal game engine will be included, in order to generate a working game (Cookie Lander) from this project.
