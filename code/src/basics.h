#pragma once
#include "vec.h"
#include <time.h>

#include <iostream>
#include <fstream>
#include <string>

#include <vector>
#include <sstream>

// ScalarField2D. Represents a 2D field (nx * ny) of scalar values. Can represent a heightfield.
class ScalarField2D
{
protected:
	int nx, ny;
	std::vector<float> values;

public:
	/*
	\brief Default Constructor
	*/
	inline ScalarField2D() : nx(0), ny(0)
	{
		// Empty
	}

	/*
	\brief Constructor
	\param nx size in x axis
	\param ny size in z axis
	\param bbox bounding box of the domain in world coordinates
	*/
	inline ScalarField2D(int nx, int ny) : nx(nx), ny(ny)
	{
		values.resize(size_t(nx * ny));
	}

	/*
	\brief Constructor
	\param name name of the file to read (format ASCII PGM required)
	*/
	inline ScalarField2D(std::string name)
	{
		std::string line;
		std::ifstream pgmfile (name);
		if (!pgmfile.is_open()) 
			return;
		std::getline(pgmfile,line);
		if (line != "P2") 
			return;	
		std::getline(pgmfile, line);
		while (line[0]=='#')
			std::getline(pgmfile, line);
		std::istringstream ss(line);
		ss>>nx>>ny;
		int max;
		pgmfile>>max;
		values.resize(size_t(nx * ny));
		for (int i=0;i<ny;i++) {
			for (int j=0;j<nx;j++) {
				int val;
				pgmfile>>val;
				Set(i,j,float(val)/float(max));
			}
		}		
	}


	/*
	\brief Constructor
	\param nx size in x axis
	\param ny size in y axis
	\param bbox bounding box of the domain
	\param value default value of the field
	*/
	inline ScalarField2D(int nx, int ny, float value) : nx(nx), ny(ny)
	{
		values.resize(size_t(nx * ny));
		Fill(value);
	}

	/*
	\brief copy constructor
	\param field Scalarfield2D to copy
	*/
	inline ScalarField2D(const ScalarField2D& field) : ScalarField2D(field.nx, field.ny)
	{
		for (unsigned int i = 0; i < values.size(); i++)
			values[i] = field.values[i];
	}

	/*
	\brief Destructor
	*/
	inline ~ScalarField2D()
	{
	}

	/*
	\brief Export in PGM format (ASCII)
	\param filename name of the file
	*/
	inline void SavePGM(std::string filename) {
		std::ofstream pgmfile(filename);
		pgmfile << "P2" << std::endl;
		pgmfile << nx << " "<<ny << std::endl;
		pgmfile << "65535" << std::endl;
		float min = Min();
		float max = Max();
		
		for (int i = 0; i < ny; i++) {
			for (int j = 0; j < nx; j++) {
				float val = Get(i, j);
				int ival = int((val - min) / (max - min) * 65535.0f);
				pgmfile << ival << std::endl;
			}
		}
	}

	/*
	\brief Normalize this field
	*/
	inline void NormalizeField()
	{
		float min = Min();
		float max = Max();
		for (int i = 0; i < ny * nx; i++)
			values[i] = (values[i] - min) / (max - min);
	}

	/*
	\brief Affine transform this field with the formula v' = a*v+b
	*/
	inline void AffineTransform(float a,float b=0.f)
	{
		for (int i = 0; i < ny * nx; i++)
			values[i] = a*values[i]+b;
	}

	/*
	\brief Return the normalized version of this field
	*/
	inline ScalarField2D Normalized() const
	{
		ScalarField2D ret(*this);
		float min = Min();
		float max = Max();
		for (int i = 0; i < ny * nx; i++)
			ret.values[i] = (ret.values[i] - min) / (max - min);
		return ret;
	}

	/*!
	\brief Utility.
	*/
	inline void ToIndex2D(int index, int& i, int& j) const
	{
		i = index / nx;
		j = index % nx;
	}

	/*!
	\brief Utility.
	*/
	inline Vector2i ToIndex2D(int index) const
	{
		return Vector2i(index / nx, index % nx);
	}

	/*!
	\brief Utility.
	*/
	inline int ToIndex1D(const Vector2i& v) const
	{
		return v.x * nx + v.y;
	}

	/*!
	\brief Utility.
	*/
	inline int ToIndex1D(int i, int j) const
	{
		return i * nx + j;
	}


	/*!
	\brief Returns the value of the field at a given coordinate.
	*/
	inline float Get(int row, int column) const
	{
		int index = ToIndex1D(row, column);
		return values[index];
	}

	/*!
	\brief Returns the value of the field at a given coordinate.
	*/
	inline float Get(int index) const
	{
		return values[index];
	}

	/*!
	\brief Returns the value of the field at a given coordinate.
	*/
	inline float Get(const Vector2i& v) const
	{
		int index = ToIndex1D(v);
		return values[index];
	}

	/*!
	\brief Todo
	*/
	void Add(int i, int j, float v)
	{
		values[ToIndex1D(i, j)] += v;
	}

	/*!
	\brief Todo
	*/
	void Remove(int i, int j, float v)
	{
		values[ToIndex1D(i, j)] -= v;
	}

	/*!
	\brief Todo
	*/
	void Add(const ScalarField2D& field)
	{
		for (int i = 0; i < values.size(); i++)
			values[i] += field.values[i];
	}

	/*!
	\brief Todo
	*/
	void Remove(const ScalarField2D& field)
	{
		for (int i = 0; i < values.size(); i++)
			values[i] -= field.values[i];
	}

	/*!
	\brief Fill all the field with a given value.
	*/
	inline void Fill(float v)
	{
		std::fill(values.begin(), values.end(), v);
	}

	/*!
	\brief Return the data in the field.
	\param c Index.
	*/
	inline float& operator[](int c)
	{
		return values[c];
	}

	/*!
	\brief Set a given value at a given coordinate.
	*/
	inline void Set(int row, int column, float v)
	{
		values[ToIndex1D(row, column)] = v;
	}

	/*!
	\brief Set a given value at a given coordinate.
	*/
	inline void Set(const Vector2i& coord, float v)
	{
		values[ToIndex1D(coord)] = v;
	}

	/*!
	\brief Set a given value at a given coordinate.
	*/
	inline void Set(int index, float v)
	{
		values[index] = v;
	}

	/*!
	\brief Compute the maximum of the field.
	*/
	inline float Max() const
	{
		if (values.size() == 0)
			return 0.0f;
		float max = values[0];
		for (int i = 1; i < values.size(); i++)
		{
			if (values[i] > max)
				max = values[i];
		}
		return max;
	}

	/*!
	\brief Compute the minimum of the field.
	*/
	inline float Min() const
	{
		if (values.size() == 0)
			return 0.0f;
		float min = values[0];
		for (int i = 1; i < values.size(); i++)
		{
			if (values[i] < min)
				min = values[i];
		}
		return min;
	}

	/*!
	\brief Compute the average value of the scalarfield.
	*/
	inline float Average() const
	{
		float sum = 0.0f;
		for (int i = 0; i < values.size(); i++)
			sum += values[i];
		return sum / values.size();
	}

	/*!
	\brief Returns the size of the array.
	*/
	inline Vector2i Size() const
	{
		return Vector2i(nx, ny);
	}

	/*!
	\brief Returns the size of x-axis of the array.
	*/
	inline int SizeX() const
	{
		return nx;
	}

	/*!
	\brief Returns the size of y-axis of the array.
	*/
	inline int SizeY() const
	{
		return ny;
	}

	/*!
	\brief Compute the memory used by the field.
	*/
	inline int Memory() const
	{
		return sizeof(ScalarField2D) + sizeof(float) * int(values.size());
	}
};
