# evolving_planets
### abstract
The project consists in using an evolutionary algorithm for the procedural generation of arbitrarily shaped planets, so that they are difficult to land on.
Each planet is a closed, periodic, B-Spline surface.
This formalism involves a control polyhedron that determines the surface: any change to this polyhedron produces a local change to the shape, while automatically preserving C2 continuity, i.e. differentiable twice.
A randomly generated collection of starting planets is provided as input to a differential evolution algorithm, which will maximize their landing difficulty.
The landing difficulty depends on the gravitational field, which is determined by the planet shape, assuming homogeneous density.
The purpose of the project is to generate the assets for a game in which the player must land on a planet: the work falls within the scope of procedural content generation for video games.
The evolutionary algorithms class is chosen because it offers a non-deterministic but oriented generation of content; the algorithm is meant to be executed offline, during the development stage; the generated assets would then be included in the game before shipment.
### additional notes
The project includes the creation of a basic renderer abstraction and a metal implementation.
