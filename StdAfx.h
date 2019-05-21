// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__FFA16C46_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_STDAFX_H__FFA16C46_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#define NOMINMAX

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#define _USE_MATH_DEFINES   // for M_PI definition
#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

template<typename T> inline void memzero(T & obj)
{
	memset( & obj, 0, sizeof obj);
}

#define countof(a) (sizeof (a) / sizeof (a[0]))

#include <MultiString.inl>

inline LONGLONG MulDiv64(
						_In_ LONGLONG nNumber,
						_In_ LONGLONG nNumerator,
						_In_ LONGLONG nDenominator
						)
{
	return (nNumber * nNumerator) / nDenominator;
}

inline ULONG UMulDiv(
					_In_ ULONG nNumber,
					_In_ ULONG nNumerator,
					_In_ ULONG nDenominator
					)
{
	return (ULONG)(UInt32x32To64(nNumber, nNumerator) / nDenominator);
}

template<int alignment> class CCRTAllocatorAligned
{
public:
	_Ret_maybenull_ _Post_writable_byte_size_(nBytes) _ATL_DECLSPEC_ALLOCATOR static void* Reallocate(
		_In_ void* p,
		_In_ size_t nBytes) throw()
	{
		return _aligned_realloc(p, nBytes, alignment);
	}

	_Ret_maybenull_ _Post_writable_byte_size_(nBytes) _ATL_DECLSPEC_ALLOCATOR static void* Allocate(_In_ size_t nBytes) throw()
	{
		return _aligned_malloc(nBytes, alignment);
	}

	static void Free(_In_ void* p) throw()
	{
		_aligned_free(p);
	}
};

typedef CCRTAllocatorAligned<16> AlignedAllocator;

#define EnableDlgItem(id, Enable) \
	::EnableWindow(GetDlgItem(id)->GetSafeHwnd(), Enable)

#ifdef NOMINMAX
#include <algorithm>
using std::min;
using std::max;
#endif
#define UWM_NOTIFY_VIEWS    (WM_APP+1)  // notify siblings in the child frame for view changes. Only those changes that don't reflect document updates
#define UWM_RESET_LAST_STATUS_MESSAGE    (WM_APP+2)
#define UWM_UPDATE_DOCUMENT_ON_IDLE    (WM_APP+3)
#include "WaveSoapFront.h"
#include <afxdlgs.h>
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__FFA16C46_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
