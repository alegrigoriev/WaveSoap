// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__FFA16C46_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_STDAFX_H__FFA16C46_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include "resource.h"
#include "mmsystem.h"
#include "SimpleCriticalSection.h"

#include "WaveFile.h"
#include "waveproc.h"
#include "WaveSoapFront.h"
#include "WaveSoapFrontDoc.h"
#include "WaveSoapFrontView.h"
#include "WaveFftView.h"

template<typename T> inline void memzero(T & obj)
{
	memset( & obj, 0, sizeof obj);
}
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__FFA16C46_2FA7_11D4_9ADD_00C0F0583C4B__INCLUDED_)
