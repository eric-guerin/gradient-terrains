# Gradient Terrain Authoring
Code source of the paper "Gradient Terrain Authoring" - EG 2022

This repository gathers a minimal setup for processing a terrain authored by its gradient and Dirichlet constraints. It is written in C++ and has a minimal dependancy setup: OpenGL and GLFW. The diffusion process is performed in a Compute Shader program. 

## Build

### Windows

Simply open the solution using Visual Studio 2019 in the visual subdirectory, build and run.

### Linux - TODO

## Usage

By default the application will import three maps in the data subdirectory, representing the alpha map (is this pixel a Dirichlet point or a Poisson one?),
the altitude constraint (only where Dirichlet conditions have been placed) and the Laplacian map (only where Poisson equation occurs). The image format is PGM.

## Output

The result is put in the result subdirectory using the defaut name results.pgm.

