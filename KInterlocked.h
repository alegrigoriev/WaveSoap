// KInterlocked.h
#pragma once
#pragma warning(disable:4197)

template <typename T>
inline T InterlockedIncrementT(T * src)
{
	C_ASSERT(sizeof(LONG) == sizeof(T));
	return static_cast<T>(InterlockedIncrement(reinterpret_cast<LONG volatile*>(src)));
}

inline LONG InterlockedIncrementT(LONG * src)
{
	return InterlockedIncrement(src);
}

#define InterlockedIncrement InterlockedIncrementT

template <typename T>
inline T InterlockedDecrementT(T * src)
{
	C_ASSERT(sizeof(LONG) == sizeof(T));
	return static_cast<T>(InterlockedDecrement(reinterpret_cast<LONG volatile*>(src)));
}

inline LONG InterlockedDecrementT(LONG * src)
{
	return InterlockedDecrement(src);
}

#define InterlockedDecrement InterlockedDecrementT

template <typename T, typename T1>
inline T InterlockedExchangeT(T * src, T1 exchange)
{
	LONG tmp;
	return reinterpret_cast<T &>
			(tmp = InterlockedExchange(
										const_cast<LONG *>(& reinterpret_cast<LONG volatile &>(*src)),
										reinterpret_cast<LONG volatile &>(exchange)));
}

#define InterlockedExchange InterlockedExchangeT

template <typename T>
inline T InterlockedExchangeAddT(T * src, LONG addend)
{
	return static_cast<T>
			(InterlockedExchangeAdd(
									const_cast<LONG *>(& reinterpret_cast<LONG volatile &>(*src)),
									addend));
}

template <typename T>
inline T InterlockedAdd(T * src, LONG addend)
{
	return static_cast<T>
			(addend + InterlockedExchangeAdd(
											const_cast<LONG *>(& reinterpret_cast<LONG volatile &>(*src)),
											addend));
}

template <typename T>
inline T InterlockedSubtract(T * src, LONG addend)
{
	return static_cast<T>
			(InterlockedExchangeAdd(
									const_cast<LONG *>(& reinterpret_cast<LONG volatile &>(*src)),
									-addend) - addend);
}

#define InterlockedExchangeAdd InterlockedExchangeAddT

template <typename T, typename T1, typename T2>
inline T InterlockedCompareExchangeT(T * src, T1 exchange, T2 compare)
{
	LONG tmp;
	return reinterpret_cast<T&>
			(tmp = InterlockedCompareExchange(
											const_cast<LONG *>(& reinterpret_cast<LONG volatile &>(*src)),
											reinterpret_cast<LONG volatile &>(exchange),
											reinterpret_cast<LONG volatile &>(compare)));
}

#define InterlockedCompareExchange InterlockedCompareExchangeT

template <typename T>
inline T InterlockedCompareExchangePointerT(T * src, PVOID exchange, PVOID compare)
{
	return reinterpret_cast<T>
			(InterlockedCompareExchangePointer(& reinterpret_cast<PVOID volatile&>(*src),
												exchange,
												compare));
}
#undef InterlockedCompareExchangePointer
#define InterlockedCompareExchangePointer InterlockedCompareExchangePointerT

#define StallExecutionMicrosec KeStallExecutionProcessor

// the function returns previous contents of the memory location being modified
template <typename T>
static T InterlockedIncrementModulo(T * src, LONG modulo)
{
	T tmp;
	do
	{
		tmp = *src;
	}
	while (tmp != InterlockedCompareExchangeT<T>(src, T(tmp + 1) % T(modulo), tmp));
	return tmp;
}

// the function returns previous contents of the memory location being modified
template <typename T>
static T InterlockedAddModulo(T * src, LONG Addend, LONG modulo)
{
	T tmp;
	do
	{
		tmp = *src;
	}
	while (tmp != InterlockedCompareExchangeT<T>(src, T(tmp + Addend) % T(modulo), tmp));
	return tmp;
}

// the function returns previous contents of the memory location being modified
template <typename T>
static T InterlockedOr(T * src, LONG operand)
{
	T tmp;
	do
	{
		tmp = *src;
	}
	while (tmp != InterlockedCompareExchangeT<T>(src, tmp | operand, tmp));
	return tmp;
}

template <typename T>
// the function returns previous contents of the memory location being modified
static T InterlockedAnd(T * src, LONG operand)
{
	T tmp;
	do
	{
		tmp = *src;
	}
	while (tmp != InterlockedCompareExchangeT<T>(src, tmp & operand, tmp));
	return tmp;
}

// the function returns previous contents of the memory location being modified
template <typename T>
static T InterlockedXor(T * src, LONG operand)
{
	T tmp;
	do
	{
		tmp = *src;
	}
	while (tmp != InterlockedCompareExchangeT<T>(src, tmp ^ operand, tmp));
	return tmp;
}

template<typename T>
struct NUM_volatile
{
	T volatile num;

	NUM_volatile(T init=0)
		: num(init)
	{
	}
	NUM_volatile & operator =(T src)
	{
		num = src;
		return *this;
	}
	operator T() const
	{
		return num;
	}

	T operator ++()
	{
		return InterlockedIncrementT<T volatile>( & num);
	}

	T operator ++(int)
	{
		return InterlockedIncrementT<T volatile>( & num) - 1;
	}

	T operator --()
	{
		return InterlockedDecrementT<T volatile>( & num);
	}

	T operator --(int)
	{
		return InterlockedDecrementT<T volatile>( & num) + 1;
	}

	T operator +=(T op)
	{
		return InterlockedAdd<T volatile>(& num, op);
	}
	T operator -=(T op)
	{
		return InterlockedSubtract<T volatile>(& num, op);
	}

	T operator |=(T op)
	{
		return InterlockedOr<T volatile>(& num, op) | op;
	}

	T Exchange_Or(T op)
	{
		return InterlockedOr<T volatile>(& num, op);
	}

	T operator &=(T op)
	{
		return InterlockedAnd<T volatile>(& num, op) & op;
	}

	T Exchange_And(T op)
	{
		return InterlockedAnd<T volatile>(& num, op);
	}

	T operator ^=(T op)
	{
		return InterlockedXor<T volatile>(& num, op) ^ op;
	}

	T Exchange(T src)
	{
		return InterlockedExchangeT<T volatile>( & num, src);
	}
	T ExchangeAdd(T src)
	{
		return InterlockedExchangeAddT<T volatile>( & num, src);
	}
	T CompareExchange(T src, T Comperand)
	{
		return InterlockedCompareExchangeT<T volatile>( & num, src, Comperand);
	}

	T IncrementModulo(T modulo)
	{
		return InterlockedIncrementModulo<T volatile>( & num, modulo);
	}

	T AddModulo(T Addend, T modulo)
	{
		return InterlockedAddModulo<T volatile>( & num, Addend, modulo);
	}
};

typedef NUM_volatile<LONG> LONG_volatile;
typedef NUM_volatile<ULONG> ULONG_volatile;

