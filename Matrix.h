#pragma once
// matrix math
#include <stddef.h>
#include <crtdbg.h>

namespace MatrixMath
{
typedef double DataType;
class Vector
{
public:
	Vector() : data(NULL), order(0)
	{
	}

	Vector(const Vector & v);
	Vector(int init_order);
	~Vector()
	{
		delete[] data;
	}
	DataType & operator[](ptrdiff_t i)
	{
		_ASSERT(i >= 0 && i < order);
		return data[i];
	}
	DataType const & operator[](ptrdiff_t i) const
	{
		_ASSERT(i >= 0 && i < order);
		return data[i];
	}

	int Order() const
	{
		return order;
	}

	void Allocate(int init_order);
	void Free();
	void Fill(DataType d);

	Vector & operator =(Vector const & v);
	void Multiply(class Matrix const & a, Vector const & b);

private:
	DataType * data;
	int order;
};

class Matrix
{
public:
	Matrix() : data(NULL), rows(NULL), width(0), height(0)
	{
	}

	Matrix(const Matrix & m);   // creates separate copy
	Matrix(Matrix & m, int col, int row, int w, int h);     // create an alias - section

	Matrix(int init_width, int init_height);
	~Matrix()
	{
		delete[] data;
		delete[] rows;
	}
#if _DEBUG
	class MatrixRow
	{
	public:
		MatrixRow(int w, DataType * d)
			: data(d), width(w)
		{
		}

		DataType & operator[](ptrdiff_t i) const
		{
			_ASSERT(i >= 0 && i < width);
			return data[i];
		}
		operator DataType *() const
		{
			return data;
		}
		// copy constructor and assignment are trivial
	private:
		DataType *data;
		int width;
	};
	class constMatrixRow
	{
	public:
		constMatrixRow(int w, DataType const * d)
			: data(d), width(w)
		{
		}

		DataType const & operator[](ptrdiff_t i) const
		{
			_ASSERT(i >= 0 && i < width);
			return data[i];
		}
		operator DataType const *() const
		{
			return data;
		}
		// copy constructor and assignment are trivial
	private:
		DataType const *data;
		int width;
	};
	MatrixRow operator[](ptrdiff_t i)
	{
		_ASSERT(i >= 0 && i < height);
		return MatrixRow(width, rows[i]);
	}

	constMatrixRow operator[](ptrdiff_t i) const
	{
		_ASSERT(i >= 0 && i < height);
		return constMatrixRow(width, rows[i]);
	}

#else
	typedef DataType *MatrixRow;
	typedef DataType const * constMatrixRow;
	MatrixRow operator[](ptrdiff_t i)
	{
		_ASSERT(i >= 0 && i < height);
		return rows[i];
	}

	constMatrixRow operator[](ptrdiff_t i) const
	{
		_ASSERT(i >= 0 && i < height);
		return rows[i];
	}
#endif
	int Width() const
	{
		return width;
	}

	int Height() const
	{
		return height;
	}

	void Allocate(int init_width, int init_height);
	void Free();
	void Fill(DataType d);

	Matrix & operator =(Matrix const & v);

	void Product(Matrix const & a, Matrix const & b);
	void Multiply(Matrix const & a, DataType d);
	void Add(Matrix const & a, Matrix const & b);
	void Subtract(Matrix const & a, Matrix const & b);
	void Inverse();

private:
	DataType * data;
	DataType ** rows;
	int width;
	int height;
};

void LinearEquationSolution(Matrix &m, Vector &v, Vector &sol);

}
