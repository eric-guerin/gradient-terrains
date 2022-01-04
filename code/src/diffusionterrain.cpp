#include "gpu-shader.h"
#include "diffusionterrain.h"
#include <cstring>
#include <cstdio>

using namespace std;

/////////////////////////////// SimpleGeometricMultigrid - float version


SimpleGeometricMultigridFloat::SimpleGeometricMultigridFloat(const ScalarField2D& alph, const ScalarField2D& alt, const ScalarField2D& lap)
  : ScalarField2D(alt) {
  bufferElems = nx * ny;
  int s = nx;

  mgsize = 1;
  minsize = 9;
  nrec = 0;
  trec = .0;
  while (s > minsize) {
    mgsize++;
    s = s / 2 + 1;
  }
  cout << "# of resolutions " << mgsize << endl;
  altitude = new ScalarField2D[mgsize];
  alpha = new ScalarField2D[mgsize];

  bufferA = new ScalarField2D[mgsize];
  bufferB = new ScalarField2D[mgsize];
  laplacian = new ScalarField2D[mgsize];
  s = nx / 2 + 1;
  altitude[0] = ScalarField2D(alph);
  alpha[0] = ScalarField2D(alph);

  bufferA[0] = ScalarField2D(alph);
  bufferB[0] = ScalarField2D(alph);
  laplacian[0] = ScalarField2D(alph);

  for (int i = 0; i < nx; i++) {
    for (int j = 0; j < ny; j++) {
      altitude[0][i * ny + j] = alt.Get(i, j);
      alpha[0][i * ny + j] = alph.Get(i, j);
      laplacian[0][i * ny + j] = lap.Get(i, j);
    }
  }

  int olds = nx;
  // geometric multigrid -> find the coarse system depending on the geometric fine system
  for (int r = 1; r < mgsize; r++) {
    altitude[r] = ScalarField2D(s,s);
    alpha[r] = ScalarField2D(s,s); // laplacian coef

    bufferA[r] = ScalarField2D(s,s);
    bufferB[r] = ScalarField2D(s,s);
    laplacian[r] = ScalarField2D(s,s);
    for (int i = 0; i < s; i++)
    {
      for (int j = 0; j < s; j++)
      {
        int ii, jj;
        int iimin = -1;
        int jjmin = -1;
        int iimax = 1;
        int jjmax = 1;
        if (i == 0)
          iimin = 0;
        else if (i == s - 1)
          iimax = 0;
        if (j == 0)
          jjmin = 0;
        else if (j == s - 1)
          jjmax = 0;
        bool fixed = false;
        float maltitude = .0;
        float nfixed = .0;
        float sumlap = .0;
        float nlap = .0;
        float newalpha = 0.;
        float sumcoef = .0;
        for (ii = iimin; ii <= iimax; ii++)
        {
          for (jj = jjmin; jj <= jjmax; jj++)
          {
            int iii = 2 * i + ii;
            int jjj = 2 * j + jj;
            float coef = 1.0f / float((1 << abs(ii)) * (1 << abs(jj)));
            newalpha += coef * alpha[r - 1][iii * olds + jjj];
            sumcoef += coef;
            if (alpha[r - 1][iii * olds + jjj] < 1.0f) // there is a fixed altitude constraint
            {
              fixed = true;

              maltitude += coef * (1.0f - alpha[r - 1][iii * olds + jjj]) * altitude[r - 1][iii * olds + jjj];
              nfixed += coef * (1.0f - alpha[r - 1][iii * olds + jjj]);
            }

            if (alpha[r - 1][iii * olds + jjj] > .0) // there is a laplacian constraint here
            {
              sumlap += coef * alpha[r - 1][iii * olds + jjj] * laplacian[r - 1][iii * olds + jjj]; // not an average, a geometric-weighted sum
              nlap += coef * alpha[r - 1][iii * olds + jjj];
            }
          }
        }

        if (fixed) {
          alpha[r][i * s + j] = 0.;

          altitude[r][i * s + j] = maltitude / nfixed; // geometric-weighted average if several cells were concerned
        }
        else { // only laplacian

          alpha[r][i * s + j] = 1.0;
          laplacian[r][i * s + j] = sumlap;// / nlap * 4.0;
        }
      }
    }
    s = s / 2 + 1;
    olds = olds / 2 + 1;
  }
}

SimpleGeometricMultigridFloat::~SimpleGeometricMultigridFloat()
{
  glDeleteBuffers(mgsize, glbufferAlpha);
  glDeleteBuffers(mgsize, glbufferAltitude);
  glDeleteBuffers(mgsize, glbufferA);
  glDeleteBuffers(mgsize, glbufferB);
  glDeleteBuffers(mgsize, glbufferLaplacian);
}

void SimpleGeometricMultigridFloat::InitGL()
{
  // load shader
  std::string definitions = "";
  definitions += "#define WORK_GROUP_SIZE_X " + std::to_string(WORK_GROUP_SIZE_X) + "\n";
  definitions += "#define WORK_GROUP_SIZE_Y " + std::to_string(WORK_GROUP_SIZE_Y) + "\n";
  char * chaine = const_cast<char*>(definitions.c_str());
  shaderStepAtoB = read_program("../shader/mgstepfloat.glsl", chaine);
  std::cerr << "Compute shader loaded!" << std::endl;

  // create buffers
  int GPUsize = 0;
  glbufferAlpha = new GLuint[mgsize];
  glGenBuffers(mgsize, glbufferAlpha);
  int s = nx;
  for (int r = 0; r < mgsize; r++) {
    int nelems = s * s;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, glbufferAlpha[r]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, nelems * sizeof(float), (const void*)(&(alpha[r][0])), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    GPUsize += nelems * sizeof(float);
    s = s / 2 + 1;
  }

  glbufferAltitude = new GLuint[mgsize];
  glGenBuffers(mgsize, glbufferAltitude);
  s = nx;
  for (int r = 0; r < mgsize; r++) {
    int nelems = s * s;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, glbufferAltitude[r]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, nelems * sizeof(float), (const void*)(&(altitude[r][0])), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    GPUsize += nelems * sizeof(float);
    s = s / 2 + 1;
  }



  glbufferA = new GLuint[mgsize];
  glGenBuffers(mgsize, glbufferA);
  s = nx;
  for (int r = 0; r < mgsize; r++) {
    int nelems = s * s;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, glbufferA[r]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, nelems * sizeof(float), (const void*)(&(bufferA[r][0])), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    GPUsize += nelems * sizeof(float);
    s = s / 2 + 1;
  }

  glbufferB = new GLuint[mgsize];
  glGenBuffers(mgsize, glbufferB);
  s = nx;
  for (int r = 0; r < mgsize; r++) {
    int nelems = s * s;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, glbufferB[r]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, nelems * sizeof(float), (const void*)(&(bufferB[r][0])), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    GPUsize += nelems * sizeof(float);
    s = s / 2 + 1;
  }

  glbufferLaplacian = new GLuint[mgsize];
  glGenBuffers(mgsize, glbufferLaplacian);
  s = nx;
  for (int r = 0; r < mgsize; r++) {
    int nelems = s * s;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, glbufferLaplacian[r]);
    glBufferData(GL_SHADER_STORAGE_BUFFER, nelems * sizeof(float), (const void*)(&(laplacian[r][0])), GL_STATIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    GPUsize += nelems * sizeof(float);
    s = s / 2 + 1;
  }
  cout << "GPU buffers size en bytes " << GPUsize << endl;

}

void SimpleGeometricMultigridFloat::Solve() {
  VCycle(0);
  nrec++;
}


void SimpleGeometricMultigridFloat::VCycle(int level) {
  int s = nx;
  for (int i = 0; i < level; i++) {
    s = s / 2 + 1;
  }
  int nelem = s * s;

  int nit = 50 + (10 * (mgsize - level));
  cout << "level " << level << " " << nit << " iterations" << endl;

  if (level == mgsize - 1) {

    // First : smooth (Jacobi iterations should not need too much of these)
    for (int step = 0; step < nit; step++) {

      glUseProgram(shaderStepAtoB);

      glProgramUniform1i(shaderStepAtoB, glGetUniformLocation(shaderStepAtoB, "GridSizeX"), s); // attention X,Y
      glProgramUniform1i(shaderStepAtoB, glGetUniformLocation(shaderStepAtoB, "GridSizeY"), s);

      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, glbufferAlpha[level]);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, glbufferAltitude[level]);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, glbufferA[level]);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, glbufferB[level]);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, glbufferLaplacian[level]);


      glDispatchCompute((s / WORK_GROUP_SIZE_X) + 1, (s / WORK_GROUP_SIZE_Y) + 1, 1); // attention X,Y
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, 0);
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, 0);

      glUseProgram(0);
      std::swap(glbufferA[level], glbufferB[level]);
    }
    // get the iteration result in bufferA (swap has just been performed before stopping the loop)

    glGetNamedBufferSubData(glbufferA[level], 0, sizeof(float) * nelem, &(bufferA[level][0]));


    return;
  }


  // we do not need to compute the residual here, because it's not yet a complete multi-grid method, i.e. we do not perform the next
  // iteration on the residual/error but on the restriction (bilinear) of the result

  // restriction operator -> bilinear interpolation using geometric weights

  // solve the next level - reccursive call //////////////////////////////////////////////////////////
  VCycle(level + 1);

  // prolongation operator : computes the fine (level) interpolation wrt the coarse level result (level+1)
  for (int i = 0; i < s; i++) {
    for (int j = 0; j < s; j++) {
      float val = .0;
      if (i % 2 == 0 && j % 2 == 0) { // both even row and column
        val = bufferA[level + 1][(i / 2) * (s / 2 + 1) + j / 2];
      }
      else if (i % 2 == 0 && j % 2 == 1) { // even column and odd row
        val = 0.5f * bufferA[level + 1][(i / 2) * (s / 2 + 1) + j / 2] + 0.5f * bufferA[level + 1][(i / 2) * (s / 2 + 1) + j / 2 + 1];
      }
      else if (i % 2 == 1 && j % 2 == 0) { // even row and odd column
        val = 0.5f * bufferA[level + 1][(i / 2) * (s / 2 + 1) + j / 2] + 0.5f * bufferA[level + 1][(i / 2 + 1) * (s / 2 + 1) + j / 2];
      }
      else { // odd column and row
        val = 0.25f * bufferA[level + 1][(i / 2) * (s / 2 + 1) + j / 2] + 0.25f * bufferA[level + 1][(i / 2) * (s / 2 + 1) + j / 2 + 1]
          + 0.25f * bufferA[level + 1][(i / 2 + 1) * (s / 2 + 1) + j / 2] + 0.25f * bufferA[level + 1][(i / 2 + 1) * (s / 2 + 1) + j / 2 + 1];
      }
      bufferA[level][i * s + j] = val;
    }
  }

  // last step : iterate to refine the result on the current level
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, glbufferA[level]);
  glBufferData(GL_SHADER_STORAGE_BUFFER, nelem * sizeof(float), (const void*)(&(bufferA[level][0])), GL_DYNAMIC_DRAW);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  // First : smooth (Jacobi iterations should not need too much of these)
  for (int step = 0; step < nit; step++) {


    glUseProgram(shaderStepAtoB);

    glProgramUniform1i(shaderStepAtoB, glGetUniformLocation(shaderStepAtoB, "GridSizeX"), s); // attention X,Y
    glProgramUniform1i(shaderStepAtoB, glGetUniformLocation(shaderStepAtoB, "GridSizeY"), s);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, glbufferAlpha[level]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, glbufferAltitude[level]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, glbufferA[level]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, glbufferB[level]);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, glbufferLaplacian[level]);



    glDispatchCompute((s / WORK_GROUP_SIZE_X) + 1, (s / WORK_GROUP_SIZE_Y) + 1, 1); // attention X,Y
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, 0);


    glUseProgram(0);

    std::swap(glbufferA[level], glbufferB[level]);
  }
  // get the iteration result in bufferA (swap has just been performed before stopping the loop)
  glGetNamedBufferSubData(glbufferA[level], 0, sizeof(float)* nelem, &(bufferA[level][0]));
}



ScalarField2D SimpleGeometricMultigridFloat::GetResult() {

  glUseProgram(shaderStepAtoB);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, glbufferA[0]); // note we have the most recent buffer here due to swap!
  void* ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, bufferElems * sizeof(float), GL_MAP_READ_BIT);
  memcpy(&(bufferA[0][0]), ptr, bufferElems * sizeof(float));
  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  glUseProgram(0);

  ScalarField2D result(nx, ny);
  ScalarField2D farray(bufferA[0]);
  for (int i = 0; i < nx; i++) {
    for (int j = 0; j < ny; j++) {
      result.Set(i, j,farray[i * ny + j]);
    }
  }
  return result;
}
