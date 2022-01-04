#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>

#include <climits>

#include "gpu-shader.h"


// Load the text file
static std::string read(const char* filename)
{
  std::stringbuf source;
  std::ifstream in(filename);
  if (in.good() == false)
    printf("[error] loading program '%s'...\n", filename);
  else
    printf("loading program '%s'...\n", filename);

  in.get(source, 0);       
  return source.str();
}

// insert definitions
static std::string prepare_source(std::string file, const std::string& definitions)
{
  if (file.empty())
    return std::string();

  // #version must stay on the first line
  std::string source;

  // get #version line
  std::string version;
  size_t b = file.find("#version");
  if (b != std::string::npos)
  {
    size_t e = file.find('\n', b);
    if (e != std::string::npos)
    {
      version = file.substr(0, e + 1);
      file.erase(0, e + 1);

      if (file.find("#version") != std::string::npos)
      {
        printf("[error] found several #version directives. failed.\n");
        return std::string();
      }
    }
  }
  else
  {
    printf("[error] no #version directive found. failed.\n");
    return std::string();
  }

  // build the full source code
  if (definitions.empty() == false)
  {
    source.append(version);                         
    source.append(definitions).append("\n");        
    source.append(file);                            
  }
  else
  {
    source.append(version);                         
    source.assign(file);                            
  }

  return source;
}


static
const char* shader_string(const GLenum type)
{
  switch (type)
  {
  case GL_VERTEX_SHADER: return "vertex shader";
  case GL_FRAGMENT_SHADER: return "fragment shader";
  case GL_GEOMETRY_SHADER: return "geometry shader";
#ifdef GL_VERSION_4_0
  case GL_TESS_CONTROL_SHADER: return "control shader";
  case GL_TESS_EVALUATION_SHADER: return "evaluation shader";
#endif
#ifdef GL_VERSION_4_3
  case GL_COMPUTE_SHADER: return "compute shader";
#endif
  default: return "shader";
  }
}

static
const char* shader_keys[] =
{
  "VERTEX_SHADER",
  "FRAGMENT_SHADER",
  "GEOMETRY_SHADER",
  "TESSELATION_CONTROL",
  "EVALUATION_CONTROL",
  "COMPUTE_SHADER"
};
const int shader_keys_max = 6;

static
GLenum shader_types[] =
{
  GL_VERTEX_SHADER,
  GL_FRAGMENT_SHADER,
  GL_GEOMETRY_SHADER,
#ifdef GL_VERSION_4_0
  GL_TESS_CONTROL_SHADER,
  GL_TESS_EVALUATION_SHADER,
#else
  0,
  0,
#endif
#ifdef GL_VERSION_4_3
  GL_COMPUTE_SHADER
#else
  0
#endif
};

static
GLuint compile_shader(const GLuint program, const GLenum shader_type, const std::string& source)
{
  if (source.size() == 0 || shader_type == 0)
    return 0;

  GLuint shader = glCreateShader(shader_type);
  glAttachShader(program, shader);

  const char* sources = source.c_str();
  glShaderSource(shader, 1, &sources, NULL);
  glCompileShader(shader);

  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  return (status == GL_TRUE) ? shader : 0;
}


int reload_program(GLuint program, const char* filename, const char* definitions)
{
  if (program == 0)
    return -1;

  // remove shaders attached to the program
  int shaders_max = 0;
  glGetProgramiv(program, GL_ATTACHED_SHADERS, &shaders_max);
  if (shaders_max > 0)
  {
    std::vector<GLuint> shaders(shaders_max, 0);
    glGetAttachedShaders(program, shaders_max, NULL, &shaders.front());
    for (int i = 0; i < shaders_max; i++)
    {
      glDetachShader(program, shaders[i]);
      glDeleteShader(shaders[i]);
    }
  }

#ifdef GL_VERSION_4_3
  glObjectLabel(GL_PROGRAM, program, -1, filename);
#endif

  // prpare sources
  std::string common_source = read(filename);
  for (int i = 0; i < shader_keys_max; i++)
  {
    if (common_source.find(shader_keys[i]) != std::string::npos)
    {
      // create and compile shaders detected in the source file
      std::string source = prepare_source(common_source, std::string(definitions).append("#define ").append(shader_keys[i]).append("\n"));
      GLuint shader = compile_shader(program, shader_types[i], source);
      if (shader == 0)
        printf("[error] compiling %s...\n%s\n", shader_string(shader_types[i]), definitions);
    }
  }

  // link shaders
  glLinkProgram(program);

  // check errors
  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status == GL_FALSE)
  {
    printf("[error] linking program %u '%s'...\n", program, filename);
    return -1;
  }

  // activate the gl object
  glUseProgram(program);
  return 0;
}

GLuint read_program(const char* filename, const char* definitions)
{
  std::cout<<"Read program"<<std::endl;
  GLuint program = glCreateProgram();
  reload_program(program, filename, definitions);
  program_print_errors(program);
  return program;
}

int release_program(const GLuint program)
{
  if (program == 0)
    return -1;

  // get the shaders
  int shaders_max = 0;
  glGetProgramiv(program, GL_ATTACHED_SHADERS, &shaders_max);

  if (shaders_max > 0)
  {
    std::vector<GLuint> shaders(shaders_max, 0);
    glGetAttachedShaders(program, shaders_max, NULL, &shaders.front());
    for (int i = 0; i < shaders_max; i++)
    {
      glDetachShader(program, shaders[i]);
      glDeleteShader(shaders[i]);
    }
  }

  glDeleteProgram(program);
  return 0;
}


// format compilation errors

static
void print_line(std::string& errors, const char* source, const int begin_id, const int line_id)
{
  int line = 0;
  char last = '\n';
  for (unsigned int i = 0; source[i] != 0; i++)
  {
    if (line > line_id)
      break;

    if (last == '\n')
    {
      line++;
      if (line >= begin_id && line <= line_id)
      {
        errors.append("  ");
        errors.push_back('0' + (line / 1000) % 10);
        errors.push_back('0' + (line / 100) % 10);
        errors.push_back('0' + (line / 10) % 10);
        errors.push_back('0' + (line / 1) % 10);
        errors.append("  ");
      }
    }

    if (line >= begin_id && line <= line_id)
    {
      if (source[i] == '\t')
        errors.append("    ");
      else
        errors.push_back(source[i]);
    }
    last = source[i];
  }
}
#ifdef _WIN64
#define Sscanf sscanf_s
#elif defined(__unix__)
#define Sscanf sscanf
#endif


static
int print_errors(std::string& errors, const char* log, const char* source)
{
  printf("[error log]\n%s\n", log);

  int first_error = INT_MAX;
  int last_string = -1;
  int last_line = -1;
  for (int i = 0; log[i] != 0; i++)
  {
    // get the line associated with the error
    int string_id = 0, line_id = 0, position = 0;
    if (Sscanf(&log[i], "%d ( %d ) : %n", &string_id, &line_id, &position) == 2        // nvidia syntax
      || Sscanf(&log[i], "%d : %d (%*d) : %n", &string_id, &line_id, &position) == 2  // mesa syntax
      || Sscanf(&log[i], "ERROR : %d : %d : %n", &string_id, &line_id, &position) == 2  // ati syntax
      || Sscanf(&log[i], "WARNING : %d : %d : %n", &string_id, &line_id, &position) == 2)  // ati syntax
    {
      if (string_id != last_string || line_id != last_line)
      {
        // keep the first error
        first_error = std::min(first_error, line_id);

        // extract the source code
        errors.append("\n");
        print_line(errors, source, last_line + 1, line_id);
        errors.append("\n");
      }
    }
    // display the associated error
    for (i += position; log[i] != 0; i++)
    {
      errors.push_back(log[i]);
      if (log[i] == '\n')
        break;
    }

    last_string = string_id;
    last_line = line_id;
  }
  errors.append("\n");
  print_line(errors, source, last_line + 1, 1000);
  errors.append("\n");

  return first_error;
}

int program_format_errors(const GLuint program, std::string& errors)
{
  errors.clear();

  if (program == 0)
  {
    errors.append("[error] no program...\n");
    return -1;
  }

  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  if (status == GL_TRUE)
    return 0;

  int first_error = INT_MAX;
  // get the shaders
  int shaders_max = 0;
  glGetProgramiv(program, GL_ATTACHED_SHADERS, &shaders_max);
  if (shaders_max == 0)
  {
    errors.append("[error] no shaders...\n");
    return 0;
  }

  std::vector<GLuint> shaders(shaders_max, 0);
  glGetAttachedShaders(program, shaders_max, NULL, &shaders.front());
  for (int i = 0; i < shaders_max; i++)
  {
    GLint value;
    glGetShaderiv(shaders[i], GL_COMPILE_STATUS, &value);
    if (value == GL_FALSE)
    {
      // get errors
      glGetShaderiv(shaders[i], GL_INFO_LOG_LENGTH, &value);
      std::vector<char>log(value + 1, 0);
      glGetShaderInfoLog(shaders[i], (GLsizei)log.size(), NULL, &log.front());

      // get source code
      glGetShaderiv(shaders[i], GL_SHADER_SOURCE_LENGTH, &value);
      std::vector<char> source(value + 1, 0);
      glGetShaderSource(shaders[i], (GLsizei)source.size(), NULL, &source.front());

      glGetShaderiv(shaders[i], GL_SHADER_TYPE, &value);
      errors.append("[error] compiling ").append(shader_string(value)).append("...\n");

      // format errors
      int last_error = print_errors(errors, &log.front(), &source.front());
      first_error = std::min(first_error, last_error);
    }
  }

  // get link errors
  {
    GLint value = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &value);

    std::vector<char>log(value + 1, 0);
    glGetProgramInfoLog(program, (GLsizei)log.size(), NULL, &log.front());

    errors.append("[error] linking program...\n").append(log.begin(), log.end());
  }

  return first_error;
}

int program_print_errors(const GLuint program)
{
  std::string errors;
  int code = program_format_errors(program, errors);
  if (errors.size() > 0)
    printf("%s\n", errors.c_str());
  return code;
}
