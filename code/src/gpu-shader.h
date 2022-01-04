#ifndef _SHADER_UTILS_
#define _SHADER_UTILS_

#include <GL/glew.h>
#include <string>

// Shader API
GLuint read_program(const char* filename, const char* definitions = "");
int release_program(const GLuint program);
int reload_program(const GLuint program, const char* filename, const char* definitions = "");
int program_format_errors(const GLuint program, std::string& errors);
int program_print_errors(const GLuint program);

#endif // !_SHADER_UTILS:_
