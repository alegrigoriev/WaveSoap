#if !defined(_INC_LDOUBLE)
#define _INC_LDOUBLE
#pragma pack(push,2)

class	ldouble {
public:
	ldouble(double);
	ldouble() {}
	friend ldouble __stdcall operator +(const ldouble &, const ldouble &);
	friend ldouble __stdcall operator +(double, const ldouble &);
	friend ldouble __stdcall operator +(const ldouble &, double);
	friend ldouble __stdcall operator -(const ldouble &, const ldouble &);
	friend ldouble __stdcall operator -(double, const ldouble &);
	friend ldouble __stdcall operator -(const ldouble &, double);
	friend ldouble __stdcall operator *(const ldouble &, const ldouble &);
	friend ldouble __stdcall operator *(double, const ldouble &);
	friend ldouble __stdcall operator *(const ldouble &, double);
	friend ldouble __stdcall operator /(const ldouble &, const ldouble &);
	friend ldouble __stdcall operator /(double, const ldouble &);
	friend ldouble __stdcall operator /(const ldouble &, double);
	ldouble & operator += (const ldouble &);
	ldouble & operator += (double);
	ldouble & operator -= (const ldouble &);
	ldouble & operator -= (double);
	ldouble & operator /= (const ldouble &);
	ldouble & operator /= (double);
	ldouble & operator *= (const ldouble &);
	ldouble & operator *= (double);
	ldouble operator+() const;
	ldouble operator-() const;
//double & operator =(double &) const;
	operator double() const;
	friend int __stdcall operator ==(const ldouble &, const ldouble &);
	friend int __stdcall operator ==(double, const ldouble &);
	friend int __stdcall operator ==(const ldouble &, double);
	friend int __stdcall operator !=(const ldouble &, const ldouble &);
	friend int __stdcall operator !=(double, const ldouble &);
	friend int __stdcall operator !=(const ldouble &, double);
	friend int __stdcall operator >(const ldouble &, const ldouble &);
	friend int __stdcall operator >(double, const ldouble &);
	friend int __stdcall operator >(const ldouble &, double);
	friend int __stdcall operator <(const ldouble &, const ldouble &);
	friend int __stdcall operator <(double, const ldouble &);
	friend int __stdcall operator <(const ldouble &, double);
	friend int __stdcall operator >=(const ldouble &, const ldouble &);
	friend int __stdcall operator >=(double, const ldouble &);
	friend int __stdcall operator >=(const ldouble &, double);
	friend int __stdcall operator <=(const ldouble &, const ldouble &);
	friend int __stdcall operator <=(double, const ldouble &);
	friend int __stdcall operator <=(const ldouble &, double);
private:
	char	data[10];
};

ldouble __stdcall sqrt(const ldouble &);
#pragma pack(pop)
#endif
