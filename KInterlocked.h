// KInterlocked.h
#pragma once
#pragma warning(disable:4197)

template <typename T>
inline T InterlockedIncrementT(T * src)
{
	C_ASSERT(sizeof(LONG) == sizeof(T));
	return static_cast<T>(InterlockedIncrement(reinterpret_cast<LONG volatile*>(src)));
}

inline LONG InterlockedIncrementT(volatile LONG * src)
{
	return InterlockedIncrement(src);
}

template <typename T>
inline T InterlockedDecrementT(T * src)
{
	C_ASSERT(sizeof(LONG) == sizeof(T));
	return static_cast<T>(InterlockedDecrement(reinterpret_cast<LONG volatile*>(src)));
}

inline LONG InterlockedDecrementT(volatile LONG * src)
{
	return InterlockedDecrement(src);
}

template <typename T, typename T1>
inline T InterlockedExchangeT(T * src, T1 exchange)
{
	LONG tmp;
	return reinterpret_cast<T &>
			(tmp = InterlockedExchange(
										const_cast<LONG *>(& reinterpret_cast<LONG volatile &>(*src)),
										reinterpret_cast<LONG volatile &>(exchange)));
}

template <typename T>
inline T InterlockedExchangeAddT(T * src, LONG addend)
{
	return static_cast<T>
			(InterlockedExchangeAdd(
									const_cast<LONG *>(& reinterpret_cast<LONG volatile &>(*src)),
									addend));
}

template <typename T>
inline T InterlockedAddT(T * src, LONG addend)
{
	return static_cast<T>
			(addend + InterlockedExchangeAdd(
											const_cast<LONG *>(& reinterpret_cast<LONG volatile &>(*src)),
											addend));
}

template <typename T>
inline T InterlockedSubtractT(T * src, LONG addend)
{
	return static_cast<T>
			(InterlockedExchangeAdd(
									const_cast<LONG *>(& reinterpret_cast<LONG volatile &>(*src)),
									-addend) - addend);
}

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

template <typename T>
inline T InterlockedCompareExchangePointerT(T * src, PVOID exchange, PVOID compare)
{
	return reinterpret_cast<T>
			(InterlockedCompareExchangePointer(& reinterpret_cast<PVOID volatile&>(*src),
												exchange,
												compare));
}

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
static T InterlockedOrT(T * src, LONG operand)
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
static T InterlockedAndT(T * src, LONG operand)
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
static T InterlockedXorT(T * src, LONG operand)
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
		return InterlockedIncrementT<typename T volatile>( & num);
	}

	T operator ++(int)
	{
		return InterlockedIncrementT<typename T volatile>( & num) - 1;
	}

	T operator --()
	{
		return InterlockedDecrementT<typename T volatile>( & num);
	}

	T operator --(int)
	{
		return InterlockedDecrementT<typename T volatile>( & num) + 1;
	}

	T operator +=(T op)
	{
		return InterlockedAddT<typename T volatile>(& num, op);
	}
	T operator -=(T op)
	{
		return InterlockedSubtractT<typename T volatile>(& num, op);
	}

	T operator |=(T op)
	{
		return InterlockedOrT<typename T volatile>(& num, op) | op;
	}

	T Exchange_Or(T op)
	{
		return InterlockedOrT<typename T volatile>(& num, op);
	}

	T operator &=(T op)
	{
		return InterlockedAndT<typename T volatile>(& num, op) & op;
	}

	T Exchange_And(T op)
	{
		return InterlockedAndT<typename T volatile>(& num, op);
	}

	T operator ^=(T op)
	{
		return InterlockedXorT<typename T volatile>(& num, op) ^ op;
	}

	T Exchange(T src)
	{
		return InterlockedExchangeT<typename T volatile>( & num, src);
	}
	T ExchangeAdd(T src)
	{
		return InterlockedExchangeAddT<typename T volatile>( & num, src);
	}
	T CompareExchange(T src, T Comperand)
	{
		return InterlockedCompareExchangeT<typename T volatile>( & num, src, Comperand);
	}

	T IncrementModulo(T modulo)
	{
		return InterlockedIncrementModulo<typename T volatile>( & num, modulo);
	}

	T AddModulo(T Addend, T modulo)
	{
		return InterlockedAddModulo<typename T volatile>( & num, Addend, modulo);
	}
};

typedef NUM_volatile<LONG> LONG_volatile;
typedef NUM_volatile<ULONG> ULONG_volatile;

