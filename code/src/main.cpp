#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <iostream>
#include "diffusionterrain.h"


int main(void) {

	// initialization of glfw, glfw and the OpenGL context
	if (!glfwInit())
	{
		std::cout << "GLFW failed to initialize" << std::endl;
		return 1;
	}

	// a window is necessary to obtain a context, even invisible
	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(10,10, "Invisible", NULL, NULL);
	
	glfwMakeContextCurrent(window);

	glewInit();
	
	GLenum err = glGetError();
	if (err != GL_NO_ERROR)
	{
		std::cout << "GLEW: failed to initialize OpenGL : " << err << std::endl;
		glfwTerminate();
		return 1;
	}

	// load the different maps
	ScalarField2D alpha("../data/004_mask.pgm"); // locations of fixed constraints (Dirichlet) /!\ 0 = fixed constraint, 1 = laplacian
	alpha.NormalizeField();
	ScalarField2D altitudes("../data/004_alt.pgm"); // values of fixed constraints (Dirichlet) where alpha = 0 (ignored on other locations)
	altitudes.NormalizeField();
	ScalarField2D laplacian("../data/004_lap.pgm"); // this contains the Laplacian, that can be calculated from the divergence of the gradient field
	laplacian.AffineTransform(1.0f,-0.5f); // center to 0
	laplacian.AffineTransform(0.03f); // adjust the strength of the Lapacian

	// solve and export
	SimpleGeometricMultigridFloat diffusion(alpha, altitudes, laplacian);
	// initialize the opengl shaders
	diffusion.InitGL();
	// execute the solver
	diffusion.Solve();
	// get the result and export it
	ScalarField2D result = diffusion.GetResult();
	result.SavePGM("../results/result.pgm");
	glfwTerminate();
	return 0;
}