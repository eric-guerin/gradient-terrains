#pragma once
#include <GL/glew.h>
#include "basics.h"

class SimpleGeometricMultigridFloat : public ScalarField2D {
public:
    SimpleGeometricMultigridFloat(const ScalarField2D& alpha,
        const ScalarField2D& altitude, const ScalarField2D& laplacian);
    ~SimpleGeometricMultigridFloat();
    void InitGL();
    void Solve();
    void VCycle(int);
    ScalarField2D GetResult();
    ScalarField2D* alpha;         //!< alpha coefficient
    ScalarField2D* altitude;      //!< altitude constraint 
    ScalarField2D* bufferA;       //!< First buffer
    ScalarField2D* bufferB;       //!< Second buffer
    ScalarField2D* laplacian;     //!< Laplacian field
    int mgsize;
    int minsize;
    int nrec;
    double trec;
protected:
    static const unsigned int WORK_GROUP_SIZE_X = 32;
    static const unsigned int WORK_GROUP_SIZE_Y = 32;
    unsigned int  bufferElems;
    GLuint shaderStepAtoB;
    GLuint* glbufferAlpha;
    GLuint* glbufferAltitude;
    GLuint* glbufferA;
    GLuint* glbufferB;
    GLuint* glbufferLaplacian;
};
