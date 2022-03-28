# Gradient Terrain Authoring
Code source of the paper "Gradient Terrain Authoring" - EG 2022 - [pdf](https://hal.archives-ouvertes.fr/hal-03577171/document)

This repository gathers a minimal setup for processing a terrain authored by its gradient and Dirichlet constraints. It is written in C++ and has a minimal dependancy setup: OpenGL and GLFW. The diffusion process is performed in a Compute Shader program. An example is given that reproduces the scene from the teaser (a canyon).

This implementation has been labelled by the GRSI (Graphics Replicability Stamp Initiative), see [here](http://www.replicabilitystamp.org/#https-github-com-eric-guerin-gradient-terrains) for more details.

## Build

### Windows

You may have to install the C++ development target installed in Visual Studio if it's not already done. Simply open the solution using Visual Studio 2019 in the visual subdirectory, build and run. On Visual studio 2022, just accept the update, build, and run.

### Linux
See linux/README.md

## Usage

By default the application will import three maps in the data subdirectory, representing the alpha map (is this pixel a Dirichlet point or a Poisson one?),
the altitude constraint (only where Dirichlet conditions have been placed) and the Laplacian map (only where Poisson equation occurs). The image format is PGM.

## Output

The result is put in the results subdirectory using the defaut name result.pgm. Note that this file is already present in the repository, you will have to delete it before execution to be sure the program has correctly been executed.

## Paper

If you use this code, please cite the paper it is drawn from:
<pre>
  @article {guerin2022,
      author = {Eric Guérin and Adrien Peytavie and Simon Masnou 
        and Julie Digne and Basile Sauvage and James Gain and Eric Galin},
      title = {Gradient Terrain Authoring},
      journal = {Computer Graphics Forum 
                 (proceedings of Eurographics 2022)},
      volume = {41},
      number = {2},
      year = {2022},
  }
</pre>

## Video

[![Gradient Terrain Authoring Video](http://img.youtube.com/vi/tuXs71n2C9I/0.jpg)](http://www.youtube.com/watch?v=tuXs71n2C9I "Gradient Terrain Authoring")
