#version 430
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object : enable

# ifdef COMPUTE_SHADER

uniform int GridSizeX;
uniform int GridSizeY;

layout(std430, binding=1) buffer Alpha {
    float alpha[];
};

layout(std430, binding=2) buffer Altitude {
    float altitude[];
};

layout(std430, binding=3) buffer BufferA {
    float bufferA[];
};

layout(std430, binding=4) buffer BufferB {
    float bufferB[];
};

layout(std430, binding=5) buffer BufferLaplacian {
    float laplacian[];
};

layout(local_size_x = WORK_GROUP_SIZE_X,  local_size_y = WORK_GROUP_SIZE_Y, local_size_z = 1) in;

int GetOffset(int i, int j)
{
    return i * GridSizeY + j;
}

float A(int i, int j, inout int cpt)
{
    if (i < 0) return 0;
    if (j < 0) return 0;
    if (i >= GridSizeX) return 0;
    if (j >= GridSizeY) return 0;
	cpt++;
    return bufferA[GetOffset(i, j)];
}

float Aspecial(int i, int j, int di, int dj, inout int cpt)
{
	int ii = i+di;
	int jj = j+dj;
    if (ii < 0) return 0;
    if (jj < 0) return 0;
    if (ii >= GridSizeX) return 0;
    if (jj >= GridSizeY) return 0;
	int idx = GetOffset(ii, jj);
	
	cpt++;
    return bufferA[GetOffset(ii, jj)];
}


void main()
{
    int i = int(gl_GlobalInvocationID.x);
    int j = int(gl_GlobalInvocationID.y);

	if (i < 0) return;
    if (j < 0) return;
    if (i >= GridSizeX) return;
    if (j >= GridSizeY) return;
    
	int idx = GetOffset(i,j);
	
	float a = alpha[idx]; // fetch alpha
	
	float lap = .0;
	int cpt = 0;
	if (a>0.) { // Laplace component is the average of neighbors, only if alpha is not null
		lap = Aspecial(i,j,-1,0,cpt)+Aspecial(i,j,1,0,cpt)+Aspecial(i,j,0,1,cpt)+Aspecial(i,j,0,-1,cpt);
		float c = cpt;
		lap = lap/c-laplacian[idx];
	}
	
	bufferB[idx] = a*lap+(1.0-a)*altitude[idx]; // final combination
}

#endif