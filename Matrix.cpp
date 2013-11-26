#include "StdAfx.h"
#include "Matrix.h"
//#include <new>

namespace MatrixMath
{

Vector::Vector(const Vector & v)
	: data(NULL), order(0)
{
	*this = v;
}

Vector::Vector(int init_order)
	: data(NULL), order(0)
{
	Allocate(init_order);
	Fill(0.);
}

void Vector::Allocate(int init_order)
{
	_ASSERT(data == NULL);
	data = new DataType[init_order];
	order = init_order;
}

void Vector::Free()
{
	delete[] data;
	data = NULL;
	order = 0;
}

void Vector::Fill(DataType d)
{
	for (int i = 0; i < order; i++)
	{
		data[i] = d;
	}
}

Vector & Vector::operator =(Vector const & v)
{
	Free();
	if (v.Order() == 0)
	{
		return *this;
	}
	Allocate(v.Order());

	for (int i = 0; i < order; i++)
	{
		data[i] = v[i];
	}
	return *this;
}

Matrix::Matrix(const Matrix & v)
	: data(NULL), rows(NULL), width(0), height(0)
{
	*this = v;
}

Matrix::Matrix(int init_width, int init_height)
	: data(NULL), rows(NULL), width(0), height(0)
{
	Allocate(init_width, init_height);
}

Matrix::Matrix(Matrix & m, int col, int row, int w, int h)     // create an alias - section
	: data(NULL), rows(NULL), width(0), height(0)
{
	_ASSERT(row + h <= m.Height());
	_ASSERT(col + w <= m.Width());
	rows = new DataType*[h];
	width = w;
	height = h;
	for (int i = 0; i < height; i++)
	{
		rows[i] = &m[i + row][col];
	}
	// data stays NULL because we don't own the array
}

void Matrix::Allocate(int init_width, int init_height)
{
	_ASSERT(data == NULL);
	_ASSERT(rows == NULL);
	data = new DataType[init_width * init_height];

	try
	{
		rows = new DataType*[init_height];
		width = init_width;
		height = init_height;

		for (int j = 0; j < height; j++)
		{
			rows[j] = data + j * width;
		}
	}
	catch (std::bad_alloc&)
	{
		delete[] data;
		data = NULL;
		throw;
	}
}

void Matrix::Free()
{
	delete[] data;
	data = NULL;

	delete[] rows;
	rows = NULL;

	width = 0;
	height = 0;
}

void Matrix::Fill(DataType d)
{
	for (int i = 0; i < height; i++)
	{
		DataType * row = rows[i];
		for (int j = 0; j < width; j++)
		{
			row[j] = d;
		}
	}
}

Matrix & Matrix::operator =(Matrix const & m)
{
	// the destination can be a section of the desired dimensions
	if (m.Width() != width
		|| m.Height() != height)
	{
		_ASSERT(rows == NULL || data != NULL);  // don't reallocate matrix section
		Free();
		Allocate(m.Width(), m.Height());
	}

	for (int i = 0; i < height; i++)
	{
		DataType * row = rows[i];
		DataType const * src_row = m[i];
		for (int j = 0; j < width; j++)
		{
			row[j] = src_row[j];
		}
	}
	return *this;
}

void Matrix::Product(Matrix const & a, Matrix const & b)
{
	_ASSERT(a.Width() == b.Height());

	if (height != a.Height()
		|| width != b.Width())
	{
		_ASSERT(rows == NULL || data != NULL);  // don't reallocate matrix section
		Free();
		Allocate(b.Width(), a.Height());
	}

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			DataType s = 0.;
			for (int n = 0; n < a.Width(); n++)
			{
				s += a[y][n] * b[n][x];
			}
			rows[y][x] = s;
		}
	}
}

void Vector::Multiply(Matrix const & a, Vector const & b)
{
	// The vector represents a column
	_ASSERT(b.Order() == a.Width());

	if (order != a.Height())
	{
		Free();
		Allocate(a.Height());
	}

	for (int y = 0; y < order; y++)
	{
		DataType s = 0.;
		for (int n = 0; n < a.Width(); n++)
		{
			s += a[y][n] * b[n];
		}

		(*this)[y] = s;
	}
}

void Matrix::Multiply(Matrix const & a, DataType d)
{
	if (Width() != a.Width() || Height() != a.Height())
	{
		_ASSERT(rows == NULL || data != NULL);  // don't reallocate matrix section
		Free();
		Allocate(a.Width(), a.Height());
	}

	for (int i = 0; i < height; i++)
	{
		DataType * row = rows[i];
		for (int j = 0; j < width; j++)
		{
			row[j] = a[i][j] * d;
		}
	}
}

void Matrix::Add(Matrix const & a, Matrix const & b)
{
	_ASSERT(a.Width() == b.Width());
	_ASSERT(a.Height() == b.Height());

	if (Width() != a.Width() || Height() != a.Height())
	{
		_ASSERT(rows == NULL || data != NULL);  // don't reallocate matrix section
		Free();
		Allocate(a.Width(), a.Height());
	}

	for (int y = 0; y < height; y++)
	{
		DataType * dst = rows[y];
		constMatrixRow row_a = a[y];
		constMatrixRow row_b = b[y];

		for (int x = 0; x < width; x++)
		{
			dst[x] = row_a[x] + row_b[x];
		}
	}
}

void Matrix::Subtract(Matrix const & a, Matrix const & b)
{
	_ASSERT(a.Width() == b.Width());
	_ASSERT(a.Height() == b.Height());

	if (Width() != a.Width() || Height() != a.Height())
	{
		_ASSERT(rows == NULL || data != NULL);  // don't reallocate matrix section
		Free();
		Allocate(a.Width(), a.Height());
	}

	for (int y = 0; y < height; y++)
	{
		DataType * dst = rows[y];
		constMatrixRow row_a = a[y];
		constMatrixRow row_b = b[y];

		for (int x = 0; x < width; x++)
		{
			dst[x] = row_a[x] - row_b[x];
		}
	}
}

void Matrix::Inverse()
{
	_ASSERT(Width() == Height());

	if (Width() == 1)
	{
		rows[0][0] = 1./rows[0][0];
		return;
	}
	if (Width() == 2)
	{
		DataType d = rows[0][0] * rows[1][1] - rows[1][0] * rows[0][1];
		DataType tmp = rows[0][0];
		rows[0][0] = rows[1][1] / d;
		rows[1][1] = tmp / d;

		rows[1][0] = -rows[1][0] / d;
		rows[0][1] = -rows[0][1] / d;

		return;
	}

	Matrix A_inv(*this, 0, 0, width / 2, height / 2);
	A_inv.Inverse();

	Matrix D(*this, width / 2, height / 2, width - width / 2, height - height / 2);

	Matrix B(*this, width / 2, 0, width - width / 2, height / 2);

	Matrix C(*this, 0, height / 2, width / 2, height - height / 2);

	Matrix CAinv;
	CAinv.Product(C, A_inv);

	Matrix AinvB;
	AinvB.Product(A_inv, B);

	Matrix CAinvB;
	CAinvB.Product(CAinv, B);

	D.Subtract(D, CAinvB);
	D.Inverse();

	C.Product(D, CAinv);
	C.Multiply(C, -1.);

	B.Product(AinvB, D);

	Matrix AinvBDCAinv;
	AinvBDCAinv.Product(B, CAinv);
	A_inv.Add(A_inv, AinvBDCAinv);

	B.Multiply(B, -1.);
}

void LinearEquationSolution(Matrix &m, Vector &v, Vector &sol)
{
	// the arguments are destroyed during solution
	_ASSERT(m.Width() == m.Height());
	_ASSERT(v.Order() == m.Height());
	if (sol.Order() != m.Width())
	{
		sol.Free();
		sol.Allocate(m.Width());
	}

	// first the matrix is made to upper triangular
	for (int i = 0; i < m.Height() - 1; i++)
	{
		// select row with biggest left element, swap with current row
		Matrix::MatrixRow curr_row = m[i];
		DataType max_first = abs(curr_row[i]);
		int max_i = i;
		for (int j = i + 1; j < m.Height(); j++)
		{
			if (max_first < abs(m[j][i]))
			{
				max_first = abs(m[j][i]);
				max_i = j;
			}
		}

		// swap with current row
		if (i != max_i)
		{
			Matrix::MatrixRow max_row = m[max_i];
			DataType tmp = v[i];
			v[i] = v[max_i];
			v[max_i] = tmp;

			for (int j = i; j < m.Width(); j++)
			{
				tmp = curr_row[j];
				curr_row[j] = max_row[j];
				max_row[j] = tmp;
			}
		}
		// eliminate the column
		for (int j = i + 1; j < m.Height(); j++)
		{
			Matrix::MatrixRow row = m[j];
			DataType a = row[i] / curr_row[i];
			row[i] = 0.;
			for (int k = i + 1; k < m.Width(); k++)
			{
				row[k] -= curr_row[k] * a;
			}

			v[j] -= v[i] * a;
		}
	}

	for (int i = m.Height() - 1; i >= 0; i--)
	{
		sol[i] = v[i] / m[i][i];
		for (int j = 0; j < i; j++)
		{
			v[j] -= sol[i] * m[j][i];
		}
	}
}

}
