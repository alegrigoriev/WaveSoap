//#include <strcore.cpp>
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "fixalloc.h"
#include <malloc.h>

#ifndef _AFX_ENABLE_INLINES
#define _AFX_INLINE
#endif
#include "CStringW.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// static class data, special inlines

// afxChNil is left for backward compatibility
AFX_DATADEF NTCHAR afxChNilN = '\0';

// For an empty string, m_pchData will point here
// (note: avoids special case of checking for NULL m_pchData)
// empty string data (and locked)
AFX_STATIC_DATA int _afxInitData[] = { -1, 0, 0, 0 };
AFX_STATIC_DATA CStringDataN* _afxDataNilN = (CStringDataN*)&_afxInitData;
AFX_COMDAT LPCNTSTR _afxPchNilN = (LPCNTSTR)(((BYTE*)&_afxInitData)+sizeof(CStringDataN));
// special function to make afxEmptyString work even during initialization
const CStringN& AFXAPI AfxGetEmptyStringN()
{ return *(CStringN*)&_afxPchNilN; }

#define _countof(array) (sizeof(array)/sizeof(array[0]))

//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

#ifdef _AFXDLL
CStringN::CStringN()
{
	Init();
}
#endif

CStringN::CStringN(const CStringN& stringSrc)
{
	ASSERT(stringSrc.GetData()->nRefs != 0);
	if (stringSrc.GetData()->nRefs >= 0)
	{
		ASSERT(stringSrc.GetData() != _afxDataNilN);
		m_pchData = stringSrc.m_pchData;
		InterlockedIncrement(&GetData()->nRefs);
	}
	else
	{
		Init();
		*this = stringSrc.m_pchData;
	}
}

#ifndef _DEBUG

#pragma warning(disable: 4074)
#pragma init_seg(compiler)

#define ROUND(x,y) (((x)+(y-1))&~(y-1))
#define ROUND4(x) ROUND(x, 4)
AFX_STATIC CFixedAlloc _afxAlloc64N(ROUND4(65*sizeof(NTCHAR)+sizeof(CStringDataN)));
AFX_STATIC CFixedAlloc _afxAlloc128N(ROUND4(129*sizeof(NTCHAR)+sizeof(CStringDataN)));
AFX_STATIC CFixedAlloc _afxAlloc256N(ROUND4(257*sizeof(NTCHAR)+sizeof(CStringDataN)));
AFX_STATIC CFixedAlloc _afxAlloc512N(ROUND4(513*sizeof(NTCHAR)+sizeof(CStringDataN)));

#endif //!_DEBUG

void CStringN::AllocBuffer(int nLen)
// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
{
	ASSERT(nLen >= 0);
	ASSERT(nLen <= INT_MAX-1);    // max size (enough room for 1 extra)

	if (nLen == 0)
		Init();
	else
	{
		CStringDataN* pData;
#ifndef _DEBUG
		if (nLen <= 64)
		{
			pData = (CStringDataN*)_afxAlloc64N.Alloc();
			pData->nAllocLength = 64;
		}
		else if (nLen <= 128)
		{
			pData = (CStringDataN*)_afxAlloc128N.Alloc();
			pData->nAllocLength = 128;
		}
		else if (nLen <= 256)
		{
			pData = (CStringDataN*)_afxAlloc256N.Alloc();
			pData->nAllocLength = 256;
		}
		else if (nLen <= 512)
		{
			pData = (CStringDataN*)_afxAlloc512N.Alloc();
			pData->nAllocLength = 512;
		}
		else
#endif
		{
			pData = (CStringDataN*)
					new BYTE[sizeof(CStringDataN) + (nLen+1)*sizeof(NTCHAR)];
			pData->nAllocLength = nLen;
		}
		pData->nRefs = 1;
		pData->data()[nLen] = '\0';
		pData->nDataLength = nLen;
		m_pchData = pData->data();
	}
}

void FASTCALL CStringN::FreeData(CStringDataN* pData)
{
#ifndef _DEBUG
	int nLen = pData->nAllocLength;
	if (nLen == 64)
		_afxAlloc64N.Free(pData);
	else if (nLen == 128)
		_afxAlloc128N.Free(pData);
	else if (nLen == 256)
		_afxAlloc256N.Free(pData);
	else  if (nLen == 512)
		_afxAlloc512N.Free(pData);
	else
	{
		ASSERT(nLen > 512);
		delete[] (BYTE*)pData;
	}
#else
	delete[] (BYTE*)pData;
#endif
}

void CStringN::Release()
{
	if (GetData() != _afxDataNilN)
	{
		ASSERT(GetData()->nRefs != 0);
		if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			FreeData(GetData());
		Init();
	}
}

void PASCAL CStringN::Release(CStringDataN* pData)
{
	if (pData != _afxDataNilN)
	{
		ASSERT(pData->nRefs != 0);
		if (InterlockedDecrement(&pData->nRefs) <= 0)
			FreeData(pData);
	}
}

void CStringN::Empty()
{
	if (GetData()->nDataLength == 0)
		return;
	if (GetData()->nRefs >= 0)
		Release();
	else
		*this = &afxChNil;
	ASSERT(GetData()->nDataLength == 0);
	ASSERT(GetData()->nRefs < 0 || GetData()->nAllocLength == 0);
}

void CStringN::CopyBeforeWrite()
{
	if (GetData()->nRefs > 1)
	{
		CStringDataN* pData = GetData();
		Release();
		AllocBuffer(pData->nDataLength);
		memcpy(m_pchData, pData->data(), (pData->nDataLength+1)*sizeof(NTCHAR));
	}
	ASSERT(GetData()->nRefs <= 1);
}

void CStringN::AllocBeforeWrite(int nLen)
{
	if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
	{
		Release();
		AllocBuffer(nLen);
	}
	ASSERT(GetData()->nRefs <= 1);
}

CStringN::~CStringN()
//  free any attached data
{
	if (GetData() != _afxDataNilN)
	{
		if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			FreeData(GetData());
	}
}

//////////////////////////////////////////////////////////////////////////////
// Helpers for the rest of the implementation

void CStringN::AllocCopy(CStringN& dest, int nCopyLen, int nCopyIndex,
						int nExtraLen) const
{
	// will clone the data attached to this string
	// allocating 'nExtraLen' characters
	// Places results in uninitialized string 'dest'
	// Will copy the part or all of original data to start of new string

	int nNewLen = nCopyLen + nExtraLen;
	if (nNewLen == 0)
	{
		dest.Init();
	}
	else
	{
		dest.AllocBuffer(nNewLen);
		memcpy(dest.m_pchData, m_pchData+nCopyIndex, nCopyLen*sizeof(NTCHAR));
	}
}

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CStringN::CStringN(LPCNTSTR lpsz)
{
	Init();
	if (lpsz != NULL && HIWORD(lpsz) == NULL)
	{
		UINT nID = LOWORD((DWORD)lpsz);
		if (!LoadString(nID))
			TRACE1("Warning: implicit LoadString(%u) failed\n", nID);
	}
	else
	{
		int nLen = SafeStrlen(lpsz);
		if (nLen != 0)
		{
			AllocBuffer(nLen);
			memcpy(m_pchData, lpsz, nLen*sizeof(NTCHAR));
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion constructors

#ifndef _UNICODE
CStringN::CStringN(LPCSTR lpsz)
{
	Init();
	int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
	if (nSrcLen != 0)
	{
		AllocBuffer(nSrcLen);
		_mbstowcsz(m_pchData, lpsz, nSrcLen+1);
		ReleaseBuffer();
	}
}
#else //_UNICODE
CStringN::CStringN(LPCWSTR lpsz)
{
	Init();
	int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
	if (nSrcLen != 0)
	{
		AllocBuffer(nSrcLen*2);
		_wcstombsz(m_pchData, lpsz, (nSrcLen*2)+1);
		ReleaseBuffer();
	}
}
#endif //!_UNICODE

//////////////////////////////////////////////////////////////////////////////
// Diagnostic support

#ifdef _DEBUG
CDumpContext& AFXAPI operator<<(CDumpContext& dc, const CStringN& string)
{
	dc << string.m_pchData;
	return dc;
}
#endif //_DEBUG

//////////////////////////////////////////////////////////////////////////////
// Assignment operators
//  All assign a new value to the string
//      (a) first see if the buffer is big enough
//      (b) if enough room, copy on top of old buffer, set size and type
//      (c) otherwise free old string data, and create a new one
//
//  All routines return the new string (but as a 'const CStringN&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

void CStringN::AssignCopy(int nSrcLen, LPCNTSTR lpszSrcData)
{
	AllocBeforeWrite(nSrcLen);
	memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(NTCHAR));
	GetData()->nDataLength = nSrcLen;
	m_pchData[nSrcLen] = '\0';
}

const CStringN& CStringN::operator=(const CStringN& stringSrc)
{
	if (m_pchData != stringSrc.m_pchData)
	{
		if ((GetData()->nRefs < 0 && GetData() != _afxDataNilN) ||
			stringSrc.GetData()->nRefs < 0)
		{
			// actual copy necessary since one of the strings is locked
			AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
		}
		else
		{
			// can just copy references around
			Release();
			ASSERT(stringSrc.GetData() != _afxDataNilN);
			m_pchData = stringSrc.m_pchData;
			InterlockedIncrement(&GetData()->nRefs);
		}
	}
	return *this;
}

const CStringN& CStringN::operator=(LPCNTSTR lpsz)
{
	ASSERT(lpsz == NULL || AfxIsValidString(lpsz));
	AssignCopy(SafeStrlen(lpsz), lpsz);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion assignment

#ifndef _UNICODE
const CStringN& CStringN::operator=(LPCSTR lpsz)
{
	int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
	AllocBeforeWrite(nSrcLen);
	_mbstowcsz(m_pchData, lpsz, nSrcLen+1);
	ReleaseBuffer();
	return *this;
}
#else //!_UNICODE
const CStringN& CStringN::operator=(LPCWSTR lpsz)
{
	int nSrcLen = lpsz != NULL ? wcslen(lpsz) : 0;
	AllocBeforeWrite(nSrcLen*2);
	_wcstombsz(m_pchData, lpsz, (nSrcLen*2)+1);
	ReleaseBuffer();
	return *this;
}
#endif  //!_UNICODE

//////////////////////////////////////////////////////////////////////////////
// concatenation

// NOTE: "operator+" is done as friend functions for simplicity
//      There are three variants:
//          CStringN + CStringN
// and for ? = NTCHAR, LPCNTSTR
//          CStringN + ?
//          ? + CStringN

void CStringN::ConcatCopy(int nSrc1Len, LPCNTSTR lpszSrc1Data,
						int nSrc2Len, LPCNTSTR lpszSrc2Data)
{
	// -- master concatenation routine
	// Concatenate two sources
	// -- assume that 'this' is a new CStringN object

	int nNewLen = nSrc1Len + nSrc2Len;
	if (nNewLen != 0)
	{
		AllocBuffer(nNewLen);
		memcpy(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(NTCHAR));
		memcpy(m_pchData+nSrc1Len, lpszSrc2Data, nSrc2Len*sizeof(NTCHAR));
	}
}

CStringN AFXAPI operator+(const CStringN& string1, const CStringN& string2)
{
	CStringN s;
	s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData,
				string2.GetData()->nDataLength, string2.m_pchData);
	return s;
}

CStringN AFXAPI operator+(const CStringN& string, LPCNTSTR lpsz)
{
	ASSERT(lpsz == NULL || AfxIsValidString(lpsz));
	CStringN s;
	s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData,
				CStringN::SafeStrlen(lpsz), lpsz);
	return s;
}

CStringN AFXAPI operator+(LPCNTSTR lpsz, const CStringN& string)
{
	ASSERT(lpsz == NULL || AfxIsValidString(lpsz));
	CStringN s;
	s.ConcatCopy(CStringN::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength,
				string.m_pchData);
	return s;
}

//////////////////////////////////////////////////////////////////////////////
// concatenate in place

void CStringN::ConcatInPlace(int nSrcLen, LPCNTSTR lpszSrcData)
{
	//  -- the main routine for += operators

	// concatenating an empty string is a no-op!
	if (nSrcLen == 0)
		return;

	// if the buffer is too small, or we have a width mis-match, just
	//   allocate a new buffer (slow but sure)
	if (GetData()->nRefs > 1 || GetData()->nDataLength + nSrcLen > GetData()->nAllocLength)
	{
		// we have to grow the buffer, use the ConcatCopy routine
		CStringDataN* pOldData = GetData();
		ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData);
		ASSERT(pOldData != NULL);
		CStringN::Release(pOldData);
	}
	else
	{
		// fast concatenation when buffer big enough
		memcpy(m_pchData+GetData()->nDataLength, lpszSrcData, nSrcLen*sizeof(NTCHAR));
		GetData()->nDataLength += nSrcLen;
		ASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
		m_pchData[GetData()->nDataLength] = '\0';
	}
}

const CStringN& CStringN::operator+=(LPCNTSTR lpsz)
{
	ASSERT(lpsz == NULL || AfxIsValidString(lpsz));
	ConcatInPlace(SafeStrlen(lpsz), lpsz);
	return *this;
}

const CStringN& CStringN::operator+=(NTCHAR ch)
{
	ConcatInPlace(1, &ch);
	return *this;
}

const CStringN& CStringN::operator+=(const CStringN& string)
{
	ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
	return *this;
}

///////////////////////////////////////////////////////////////////////////////
// Advanced direct buffer access

LPNTSTR CStringN::GetBuffer(int nMinBufLength)
{
	ASSERT(nMinBufLength >= 0);

	if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
	{
#ifdef _DEBUG
		// give a warning in case locked string becomes unlocked
		if (GetData() != _afxDataNilN && GetData()->nRefs < 0)
			TRACE0("Warning: GetBuffer on locked CStringN creates unlocked CStringN!\n");
#endif
		// we have to grow the buffer
		CStringDataN* pOldData = GetData();
		int nOldLen = GetData()->nDataLength;   // AllocBuffer will tromp it
		if (nMinBufLength < nOldLen)
			nMinBufLength = nOldLen;
		AllocBuffer(nMinBufLength);
		memcpy(m_pchData, pOldData->data(), (nOldLen+1)*sizeof(NTCHAR));
		GetData()->nDataLength = nOldLen;
		CStringN::Release(pOldData);
	}
	ASSERT(GetData()->nRefs <= 1);

	// return a pointer to the character storage for this string
	ASSERT(m_pchData != NULL);
	return m_pchData;
}

void CStringN::ReleaseBuffer(int nNewLength)
{
	CopyBeforeWrite();  // just in case GetBuffer was not called

	if (nNewLength == -1)
		nNewLength = lstrlenN(m_pchData); // zero terminated

	ASSERT(nNewLength <= GetData()->nAllocLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
}

LPNTSTR CStringN::GetBufferSetLength(int nNewLength)
{
	ASSERT(nNewLength >= 0);

	GetBuffer(nNewLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
	return m_pchData;
}

void CStringN::FreeExtra()
{
	ASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
	if (GetData()->nDataLength != GetData()->nAllocLength)
	{
		CStringDataN* pOldData = GetData();
		AllocBuffer(GetData()->nDataLength);
		memcpy(m_pchData, pOldData->data(), pOldData->nDataLength*sizeof(NTCHAR));
		ASSERT(m_pchData[GetData()->nDataLength] == '\0');
		CStringN::Release(pOldData);
	}
	ASSERT(GetData() != NULL);
}

LPNTSTR CStringN::LockBuffer()
{
	LPNTSTR lpsz = GetBuffer(0);
	GetData()->nRefs = -1;
	return lpsz;
}

void CStringN::UnlockBuffer()
{
	ASSERT(GetData()->nRefs == -1);
	if (GetData() != _afxDataNilN)
		GetData()->nRefs = 1;
}

///////////////////////////////////////////////////////////////////////////////
// Commonly used routines (rarely used routines in STREX.CPP)

int CStringN::Find(NTCHAR ch) const
{
	return Find(ch, 0);
}

int CStringN::Find(NTCHAR ch, int nStart) const
{
	int nLength = GetData()->nDataLength;
	if (nStart >= nLength)
		return -1;

	// find first single character
	LPNTSTR lpsz = _tcschrN(m_pchData + nStart, (_TUCHAR)ch);

	// return -1 if not found and index otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CStringN::FindOneOf(LPCNTSTR lpszCharSet) const
{
	ASSERT(AfxIsValidString(lpszCharSet));
	LPNTSTR lpsz = _tcspbrkN(m_pchData, lpszCharSet);
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

void CStringN::MakeUpper()
{
	CopyBeforeWrite();
	_tcsuprN(m_pchData);
}

void CStringN::MakeLower()
{
	CopyBeforeWrite();
	_tcslwrN(m_pchData);
}

void CStringN::MakeReverse()
{
	CopyBeforeWrite();
	_tcsrevN(m_pchData);
}

void CStringN::SetAt(int nIndex, NTCHAR ch)
{
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);

	CopyBeforeWrite();
	m_pchData[nIndex] = ch;
}

#ifdef _UNICODE
void CStringN::AnsiToOem()
{
	CopyBeforeWrite();
	::AnsiToOem(m_pchData, m_pchData);
}
void CStringN::OemToAnsi()
{
	CopyBeforeWrite();
	::OemToAnsi(m_pchData, m_pchData);
}
#endif

///////////////////////////////////////////////////////////////////////////////
// CStringN conversion helpers (these use the current system locale)

///////////////////////////////////////////////////////////////////////////////

// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include <afxtempl.h>

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CStringN::CStringN(NTCHAR ch, int nLength)
{
	Init();
	if (nLength >= 1)
	{
		AllocBuffer(nLength);
#ifdef _UNICODE
		for (int i = 0; i < nLength; i++)
			m_pchData[i] = ch;
#else
		memset(m_pchData, ch, nLength);
#endif
	}
}

CStringN::CStringN(LPCNTSTR lpch, int nLength)
{
	Init();
	if (nLength != 0)
	{
		ASSERT(AfxIsValidAddress(lpch, nLength, FALSE));
		AllocBuffer(nLength);
		memcpy(m_pchData, lpch, nLength*sizeof(NTCHAR));
	}
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion constructors

#ifndef _UNICODE
CStringN::CStringN(LPCSTR lpsz, int nLength)
{
	Init();
	if (nLength != 0)
	{
		AllocBuffer(nLength);
		int n = ::MultiByteToWideChar(CP_ACP, 0, lpsz, nLength, m_pchData, nLength+1);
		ReleaseBuffer(n >= 0 ? n : -1);
	}
}
#else //_UNICODE
CStringN::CStringN(LPCWSTR lpsz, int nLength)
{
	Init();
	if (nLength != 0)
	{
		AllocBuffer(nLength*2);
		int n = ::WideCharToMultiByte(CP_ACP, 0, lpsz, nLength, m_pchData,
									(nLength*2)+1, NULL, NULL);
		ReleaseBuffer(n >= 0 ? n : -1);
	}
}
#endif //!_UNICODE

//////////////////////////////////////////////////////////////////////////////
// Assignment operators

const CStringN& CStringN::operator=(NTCHAR ch)
{
	AssignCopy(1, &ch);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////
// less common string expressions

CStringN AFXAPI operator+(const CStringN& string1, NTCHAR ch)
{
	CStringN s;
	s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData, 1, &ch);
	return s;
}

CStringN AFXAPI operator+(NTCHAR ch, const CStringN& string)
{
	CStringN s;
	s.ConcatCopy(1, &ch, string.GetData()->nDataLength, string.m_pchData);
	return s;
}

//////////////////////////////////////////////////////////////////////////////
// Advanced manipulation

int CStringN::Delete(int nIndex, int nCount /* = 1 */)
{
	if (nIndex < 0)
		nIndex = 0;
	int nNewLength = GetData()->nDataLength;
	if (nCount > 0 && nIndex < nNewLength)
	{
		CopyBeforeWrite();
		int nBytesToCopy = nNewLength - (nIndex + nCount) + 1;

		memcpy(m_pchData + nIndex,
				m_pchData + nIndex + nCount, nBytesToCopy * sizeof(NTCHAR));
		GetData()->nDataLength = nNewLength - nCount;
	}

	return nNewLength;
}

int CStringN::Insert(int nIndex, NTCHAR ch)
{
	CopyBeforeWrite();

	if (nIndex < 0)
		nIndex = 0;

	int nNewLength = GetData()->nDataLength;
	if (nIndex > nNewLength)
		nIndex = nNewLength;
	nNewLength++;

	if (GetData()->nAllocLength < nNewLength)
	{
		CStringDataN* pOldData = GetData();
		LPNTSTR pstr = m_pchData;
		AllocBuffer(nNewLength);
		memcpy(m_pchData, pstr, (pOldData->nDataLength+1)*sizeof(NTCHAR));
		CStringN::Release(pOldData);
	}

	// move existing bytes down
	memcpy(m_pchData + nIndex + 1,
			m_pchData + nIndex, (nNewLength-nIndex)*sizeof(NTCHAR));
	m_pchData[nIndex] = ch;
	GetData()->nDataLength = nNewLength;

	return nNewLength;
}

int CStringN::Insert(int nIndex, LPCNTSTR pstr)
{
	if (nIndex < 0)
		nIndex = 0;

	int nInsertLength = SafeStrlen(pstr);
	int nNewLength = GetData()->nDataLength;
	if (nInsertLength > 0)
	{
		CopyBeforeWrite();
		if (nIndex > nNewLength)
			nIndex = nNewLength;
		nNewLength += nInsertLength;

		if (GetData()->nAllocLength < nNewLength)
		{
			CStringDataN* pOldData = GetData();
			LPNTSTR pstr = m_pchData;
			AllocBuffer(nNewLength);
			memcpy(m_pchData, pstr, (pOldData->nDataLength+1)*sizeof(NTCHAR));
			CStringN::Release(pOldData);
		}

		// move existing bytes down
		memcpy(m_pchData + nIndex + nInsertLength,
				m_pchData + nIndex,
				(nNewLength-nIndex-nInsertLength+1)*sizeof(NTCHAR));
		memcpy(m_pchData + nIndex,
				pstr, nInsertLength*sizeof(NTCHAR));
		GetData()->nDataLength = nNewLength;
	}

	return nNewLength;
}

int CStringN::Replace(NTCHAR chOld, NTCHAR chNew)
{
	int nCount = 0;

	// short-circuit the nop case
	if (chOld != chNew)
	{
		// otherwise modify each character that matches in the string
		CopyBeforeWrite();
		LPNTSTR psz = m_pchData;
		LPNTSTR pszEnd = psz + GetData()->nDataLength;
		while (psz < pszEnd)
		{
			// replace instances of the specified character only
			if (*psz == chOld)
			{
				*psz = chNew;
				nCount++;
			}
			psz = _tcsincN(psz);
		}
	}
	return nCount;
}

int CStringN::Replace(LPCNTSTR lpszOld, LPCNTSTR lpszNew)
{
	// can't have empty or NULL lpszOld

	int nSourceLen = SafeStrlen(lpszOld);
	if (nSourceLen == 0)
		return 0;
	int nReplacementLen = SafeStrlen(lpszNew);

	// loop once to figure out the size of the result string
	int nCount = 0;
	LPNTSTR lpszStart = m_pchData;
	LPNTSTR lpszEnd = m_pchData + GetData()->nDataLength;
	LPNTSTR lpszTarget;
	while (lpszStart < lpszEnd)
	{
		while ((lpszTarget = _tcsstrN(lpszStart, lpszOld)) != NULL)
		{
			nCount++;
			lpszStart = lpszTarget + nSourceLen;
		}
		lpszStart += lstrlenN(lpszStart) + 1;
	}

	// if any changes were made, make them
	if (nCount > 0)
	{
		CopyBeforeWrite();

		// if the buffer is too small, just
		//   allocate a new buffer (slow but sure)
		int nOldLength = GetData()->nDataLength;
		int nNewLength =  nOldLength + (nReplacementLen-nSourceLen)*nCount;
		if (GetData()->nAllocLength < nNewLength || GetData()->nRefs > 1)
		{
			CStringDataN* pOldData = GetData();
			LPNTSTR pstr = m_pchData;
			AllocBuffer(nNewLength);
			memcpy(m_pchData, pstr, pOldData->nDataLength*sizeof(NTCHAR));
			CStringN::Release(pOldData);
		}
		// else, we just do it in-place
		lpszStart = m_pchData;
		lpszEnd = m_pchData + GetData()->nDataLength;

		// loop again to actually do the work
		while (lpszStart < lpszEnd)
		{
			while ( (lpszTarget = _tcsstrN(lpszStart, lpszOld)) != NULL)
			{
				int nBalance = nOldLength - (lpszTarget - m_pchData + nSourceLen);
				memmove(lpszTarget + nReplacementLen, lpszTarget + nSourceLen,
						nBalance * sizeof(NTCHAR));
				memcpy(lpszTarget, lpszNew, nReplacementLen*sizeof(NTCHAR));
				lpszStart = lpszTarget + nReplacementLen;
				lpszStart[nBalance] = '\0';
				nOldLength += (nReplacementLen - nSourceLen);
			}
			lpszStart += lstrlenN(lpszStart) + 1;
		}
		ASSERT(m_pchData[nNewLength] == '\0');
		GetData()->nDataLength = nNewLength;
	}

	return nCount;
}

int CStringN::Remove(NTCHAR chRemove)
{
	CopyBeforeWrite();

	LPNTSTR pstrSource = m_pchData;
	LPNTSTR pstrDest = m_pchData;
	LPNTSTR pstrEnd = m_pchData + GetData()->nDataLength;

	while (pstrSource < pstrEnd)
	{
		if (*pstrSource != chRemove)
		{
			*pstrDest = *pstrSource;
			pstrDest = _tcsincN(pstrDest);
		}
		pstrSource = _tcsincN(pstrSource);
	}
	*pstrDest = '\0';
	int nCount = pstrSource - pstrDest;
	GetData()->nDataLength -= nCount;

	return nCount;
}

//////////////////////////////////////////////////////////////////////////////
// Very simple sub-string extraction

CStringN CStringN::Mid(int nFirst) const
{
	return Mid(nFirst, GetData()->nDataLength - nFirst);
}

CStringN CStringN::Mid(int nFirst, int nCount) const
{
	// out-of-bounds requests return sensible things
	if (nFirst < 0)
		nFirst = 0;
	if (nCount < 0)
		nCount = 0;

	if (nFirst + nCount > GetData()->nDataLength)
		nCount = GetData()->nDataLength - nFirst;
	if (nFirst > GetData()->nDataLength)
		nCount = 0;

	ASSERT(nFirst >= 0);
	ASSERT(nFirst + nCount <= GetData()->nDataLength);

	// optimize case of returning entire string
	if (nFirst == 0 && nFirst + nCount == GetData()->nDataLength)
		return *this;

	CStringN dest;
	AllocCopy(dest, nCount, nFirst, 0);
	return dest;
}

CStringN CStringN::Right(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	if (nCount >= GetData()->nDataLength)
		return *this;

	CStringN dest;
	AllocCopy(dest, nCount, GetData()->nDataLength-nCount, 0);
	return dest;
}

CStringN CStringN::Left(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	if (nCount >= GetData()->nDataLength)
		return *this;

	CStringN dest;
	AllocCopy(dest, nCount, 0, 0);
	return dest;
}

// strspn equivalent
CStringN CStringN::SpanIncluding(LPCNTSTR lpszCharSet) const
{
	ASSERT(AfxIsValidString(lpszCharSet));
	return Left(_tcsspnN(m_pchData, lpszCharSet));
}

// strcspn equivalent
CStringN CStringN::SpanExcluding(LPCNTSTR lpszCharSet) const
{
	ASSERT(AfxIsValidString(lpszCharSet));
	return Left(_tcscspnN(m_pchData, lpszCharSet));
}

//////////////////////////////////////////////////////////////////////////////
// Finding

int CStringN::ReverseFind(NTCHAR ch) const
{
	// find last single character
	LPNTSTR lpsz = _tcsrchrN(m_pchData, (_NTUCHAR) ch);

	// return -1 if not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

// find a sub-string (like strstr)
int CStringN::Find(LPCNTSTR lpszSub) const
{
	return Find(lpszSub, 0);
}

int CStringN::Find(LPCNTSTR lpszSub, int nStart) const
{
	ASSERT(AfxIsValidString(lpszSub));

	int nLength = GetData()->nDataLength;
	if (nStart > nLength)
		return -1;

	// find first matching substring
	LPNTSTR lpsz = _tcsstrN(m_pchData + nStart, lpszSub);

	// return -1 for not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}


/////////////////////////////////////////////////////////////////////////////
// CStringN formatting

#define TCHAR_ARG   NTCHAR
#define WCHAR_ARG   WCHAR
#define CHAR_ARG    char

#ifdef _X86_
	#define DOUBLE_ARG  _AFX_DOUBLE
#else
	#define DOUBLE_ARG  double
#endif

#define FORCE_ANSI      0x10000
#define FORCE_UNICODE   0x20000
#define FORCE_INT64     0x40000

void CStringN::FormatV(LPCNTSTR lpszFormat, va_list argList)
{
	ASSERT(AfxIsValidString(lpszFormat));

	va_list argListSave = argList;

	// make a guess at the maximum length of the resulting string
	int nMaxLen = 0;
	for (LPCNTSTR lpsz = lpszFormat; *lpsz != '\0'; lpsz = _tcsincN(lpsz))
	{
		// handle '%' character, but watch out for '%%'
		if (*lpsz != '%' || *(lpsz = _tcsincN(lpsz)) == '%')
		{
			nMaxLen += _tclenN(lpsz);
			continue;
		}

		int nItemLen = 0;

		// handle '%' character with format
		int nWidth = 0;
		for (; *lpsz != '\0'; lpsz = _tcsincN(lpsz))
		{
			// check for valid flags
			if (*lpsz == '#')
				nMaxLen += 2;   // for '0x'
			else if (*lpsz == '*')
				nWidth = va_arg(argList, int);
			else if (*lpsz == '-' || *lpsz == '+' || *lpsz == '0' ||
					*lpsz == ' ')
				;
			else // hit non-flag character
				break;
		}
		// get width and skip it
		if (nWidth == 0)
		{
			// width indicated by
			nWidth = _ttoiN(lpsz);
			for (; *lpsz != '\0' && _istdigitN(*lpsz); lpsz = _tcsincN(lpsz))
				;
		}
		ASSERT(nWidth >= 0);

		int nPrecision = 0;
		if (*lpsz == '.')
		{
			// skip past '.' separator (width.precision)
			lpsz = _tcsincN(lpsz);

			// get precision and skip it
			if (*lpsz == '*')
			{
				nPrecision = va_arg(argList, int);
				lpsz = _tcsincN(lpsz);
			}
			else
			{
				nPrecision = _ttoiN(lpsz);
				for (; *lpsz != '\0' && _istdigitN(*lpsz); lpsz = _tcsincN(lpsz))
					;
			}
			ASSERT(nPrecision >= 0);
		}

		// should be on type modifier or specifier
		int nModifier = 0;
		if (_tcsncmpN(lpsz, _nT("I64"), 3) == 0)
		{
			lpsz += 3;
			nModifier = FORCE_INT64;
#if !defined(_X86_) && !defined(_ALPHA_)
			// __int64 is only available on X86 and ALPHA platforms
			ASSERT(FALSE);
#endif
		}
		else
		{
			switch (*lpsz)
			{
				// modifiers that affect size
			case 'h':
				nModifier = FORCE_ANSI;
				lpsz = _tcsincN(lpsz);
				break;
			case 'l':
				nModifier = FORCE_UNICODE;
				lpsz = _tcsincN(lpsz);
				break;

				// modifiers that do not affect size
			case 'F':
			case 'N':
			case 'L':
				lpsz = _tcsincN(lpsz);
				break;
			}
		}

		// now should be on specifier
		switch (*lpsz | nModifier)
		{
			// single characters
		case 'c':
		case 'C':
			nItemLen = 2;
			va_arg(argList, TCHAR_ARG);
			break;
		case 'c'|FORCE_ANSI:
		case 'C'|FORCE_ANSI:
			nItemLen = 2;
			va_arg(argList, CHAR_ARG);
			break;
		case 'c'|FORCE_UNICODE:
		case 'C'|FORCE_UNICODE:
			nItemLen = 2;
			va_arg(argList, WCHAR_ARG);
			break;

			// strings
		case 's':
		{
			LPCNTSTR pstrNextArg = va_arg(argList, LPCNTSTR);
			if (pstrNextArg == NULL)
				nItemLen = 6;  // "(null)"
			else
			{
				nItemLen = lstrlenN(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
		}
			break;

		case 'S':
		{
#ifndef _UNICODE
			LPWSTR pstrNextArg = va_arg(argList, LPWSTR);
			if (pstrNextArg == NULL)
				nItemLen = 6;  // "(null)"
			else
			{
				nItemLen = wcslen(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
#else
			LPCSTR pstrNextArg = va_arg(argList, LPCSTR);
			if (pstrNextArg == NULL)
				nItemLen = 6; // "(null)"
			else
			{
				nItemLen = lstrlenA(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
#endif
		}
			break;

		case 's'|FORCE_ANSI:
		case 'S'|FORCE_ANSI:
		{
			LPCSTR pstrNextArg = va_arg(argList, LPCSTR);
			if (pstrNextArg == NULL)
				nItemLen = 6; // "(null)"
			else
			{
				nItemLen = lstrlenA(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
		}
			break;

		case 's'|FORCE_UNICODE:
		case 'S'|FORCE_UNICODE:
		{
			LPWSTR pstrNextArg = va_arg(argList, LPWSTR);
			if (pstrNextArg == NULL)
				nItemLen = 6; // "(null)"
			else
			{
				nItemLen = wcslen(pstrNextArg);
				nItemLen = max(1, nItemLen);
			}
		}
			break;
		}

		// adjust nItemLen for strings
		if (nItemLen != 0)
		{
			if (nPrecision != 0)
				nItemLen = min(nItemLen, nPrecision);
			nItemLen = max(nItemLen, nWidth);
		}
		else
		{
			switch (*lpsz)
			{
				// integers
			case 'd':
			case 'i':
			case 'u':
			case 'x':
			case 'X':
			case 'o':
				if (nModifier & FORCE_INT64)
					va_arg(argList, __int64);
				else
					va_arg(argList, int);
				nItemLen = 32;
				nItemLen = max(nItemLen, nWidth+nPrecision);
				break;

			case 'e':
			case 'g':
			case 'G':
				va_arg(argList, DOUBLE_ARG);
				nItemLen = 128;
				nItemLen = max(nItemLen, nWidth+nPrecision);
				break;

			case 'f':
			{
				double f;
				LPNTSTR pszTemp;

				// 312 == strlen("-1+(309 zeroes).")
				// 309 zeroes == max precision of a double
				// 6 == adjustment in case precision is not specified,
				//   which means that the precision defaults to 6
				pszTemp = (LPNTSTR)_alloca(max(nWidth, 312+nPrecision+6));

				f = va_arg(argList, double);
				_stprintfN( pszTemp, _nT( "%*.*f" ), nWidth, nPrecision+6, f );
				nItemLen = _tcslenN(pszTemp);
			}
				break;

			case 'p':
				va_arg(argList, void*);
				nItemLen = 32;
				nItemLen = max(nItemLen, nWidth+nPrecision);
				break;

				// no output
			case 'n':
				va_arg(argList, int*);
				break;

			default:
				ASSERT(FALSE);  // unknown formatting option
			}
		}

		// adjust nMaxLen for output nItemLen
		nMaxLen += nItemLen;
	}

	GetBuffer(nMaxLen);
	VERIFY(_vstprintfN(m_pchData, lpszFormat, argListSave) <= GetAllocLength());
	ReleaseBuffer();

	va_end(argListSave);
}

// formatting (using wsprintf style formatting)
void AFX_CDECL CStringN::Format(LPCNTSTR lpszFormat, ...)
{
	ASSERT(AfxIsValidString(lpszFormat));

	va_list argList;
	va_start(argList, lpszFormat);
	FormatV(lpszFormat, argList);
	va_end(argList);
}

void AFX_CDECL CStringN::Format(UINT nFormatID, ...)
{
	CStringN strFormat;
	VERIFY(strFormat.LoadString(nFormatID) != 0);

	va_list argList;
	va_start(argList, nFormatID);
	FormatV(strFormat, argList);
	va_end(argList);
}

// formatting (using FormatMessage style formatting)
void AFX_CDECL CStringN::FormatMessage(LPCNTSTR lpszFormat, ...)
{
	// format message into temporary buffer lpszTemp
	va_list argList;
	va_start(argList, lpszFormat);
	LPNTSTR lpszTemp;

	if (::FormatMessageN(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
						lpszFormat, 0, 0, (LPNTSTR)&lpszTemp, 0, &argList) == 0 ||
		lpszTemp == NULL)
	{
		AfxThrowMemoryException();
	}

	// assign lpszTemp into the resulting string and free the temporary
	*this = lpszTemp;
	LocalFree(lpszTemp);
	va_end(argList);
}

void AFX_CDECL CStringN::FormatMessage(UINT nFormatID, ...)
{
	// get format string from string table
	CStringN strFormat;
	VERIFY(strFormat.LoadString(nFormatID) != 0);

	// format message into temporary buffer lpszTemp
	va_list argList;
	va_start(argList, nFormatID);
	LPNTSTR lpszTemp;
	if (::FormatMessageN(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
						strFormat, 0, 0, (LPNTSTR)&lpszTemp, 0, &argList) == 0 ||
		lpszTemp == NULL)
	{
		AfxThrowMemoryException();
	}

	// assign lpszTemp into the resulting string and free lpszTemp
	*this = lpszTemp;
	LocalFree(lpszTemp);
	va_end(argList);
}

void CStringN::TrimRight(LPCNTSTR lpszTargetList)
{
	// find beginning of trailing matches
	// by starting at beginning (DBCS aware)

	CopyBeforeWrite();
	LPNTSTR lpsz = m_pchData;
	LPNTSTR lpszLast = NULL;

	while (*lpsz != '\0')
	{
		if (_tcschrN(lpszTargetList, *lpsz) != NULL)
		{
			if (lpszLast == NULL)
				lpszLast = lpsz;
		}
		else
			lpszLast = NULL;
		lpsz = _tcsincN(lpsz);
	}

	if (lpszLast != NULL)
	{
		// truncate at left-most matching character
		*lpszLast = '\0';
		GetData()->nDataLength = lpszLast - m_pchData;
	}
}

void CStringN::TrimRight(NTCHAR chTarget)
{
	// find beginning of trailing matches
	// by starting at beginning (DBCS aware)

	CopyBeforeWrite();
	LPNTSTR lpsz = m_pchData;
	LPNTSTR lpszLast = NULL;

	while (*lpsz != '\0')
	{
		if (*lpsz == chTarget)
		{
			if (lpszLast == NULL)
				lpszLast = lpsz;
		}
		else
			lpszLast = NULL;
		lpsz = _tcsincN(lpsz);
	}

	if (lpszLast != NULL)
	{
		// truncate at left-most matching character
		*lpszLast = '\0';
		GetData()->nDataLength = lpszLast - m_pchData;
	}
}

void CStringN::TrimRight()
{
	// find beginning of trailing spaces by starting at beginning (DBCS aware)

	CopyBeforeWrite();
	LPNTSTR lpsz = m_pchData;
	LPNTSTR lpszLast = NULL;

	while (*lpsz != '\0')
	{
		if (_istspaceN(*lpsz))
		{
			if (lpszLast == NULL)
				lpszLast = lpsz;
		}
		else
			lpszLast = NULL;
		lpsz = _tcsincN(lpsz);
	}

	if (lpszLast != NULL)
	{
		// truncate at trailing space start
		*lpszLast = '\0';
		GetData()->nDataLength = lpszLast - m_pchData;
	}
}

void CStringN::TrimLeft(LPCNTSTR lpszTargets)
{
	// if we're not trimming anything, we're not doing any work
	if (SafeStrlen(lpszTargets) == 0)
		return;

	CopyBeforeWrite();
	LPCNTSTR lpsz = m_pchData;

	while (*lpsz != '\0')
	{
		if (_tcschrN(lpszTargets, *lpsz) == NULL)
			break;
		lpsz = _tcsincN(lpsz);
	}

	if (lpsz != m_pchData)
	{
		// fix up data and length
		int nDataLength = GetData()->nDataLength - (lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(NTCHAR));
		GetData()->nDataLength = nDataLength;
	}
}

void CStringN::TrimLeft(NTCHAR chTarget)
{
	// find first non-matching character

	CopyBeforeWrite();
	LPCNTSTR lpsz = m_pchData;

	while (chTarget == *lpsz)
		lpsz = _tcsincN(lpsz);

	if (lpsz != m_pchData)
	{
		// fix up data and length
		int nDataLength = GetData()->nDataLength - (lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(NTCHAR));
		GetData()->nDataLength = nDataLength;
	}
}

void CStringN::TrimLeft()
{
	// find first non-space character

	CopyBeforeWrite();
	LPCNTSTR lpsz = m_pchData;

	while (_istspaceN(*lpsz))
		lpsz = _tcsincN(lpsz);

	if (lpsz != m_pchData)
	{
		// fix up data and length
		int nDataLength = GetData()->nDataLength - (lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(NTCHAR));
		GetData()->nDataLength = nDataLength;
	}
}

///////////////////////////////////////////////////////////////////////////////
// CStringN support for template collections

#if _MSC_VER >= 1100
template<> void AFXAPI ConstructElements<CStringN> (CStringN* pElements, int nCount)
#else
void AFXAPI ConstructElements(CStringN* pElements, int nCount)
#endif
{
	ASSERT(nCount == 0 ||
			AfxIsValidAddress(pElements, nCount * sizeof(CStringN)));

	for (; nCount--; ++pElements)
		memcpy(pElements, &afxEmptyStringN, sizeof(*pElements));
}

#if _MSC_VER >= 1100
template<> void AFXAPI DestructElements<CStringN> (CStringN* pElements, int nCount)
#else
void AFXAPI DestructElements(CStringN* pElements, int nCount)
#endif
{
	ASSERT(nCount == 0 ||
			AfxIsValidAddress(pElements, nCount * sizeof(CStringN)));

	for (; nCount--; ++pElements)
		pElements->~CStringN();
}

#if _MSC_VER >= 1100
template<> void AFXAPI CopyElements<CStringN> (CStringN* pDest, const CStringN* pSrc, int nCount)
#else
void AFXAPI CopyElements(CStringN* pDest, const CStringN* pSrc, int nCount)
#endif
{
	ASSERT(nCount == 0 ||
			AfxIsValidAddress(pDest, nCount * sizeof(CStringN)));
	ASSERT(nCount == 0 ||
			AfxIsValidAddress(pSrc, nCount * sizeof(CStringN)));

	for (; nCount--; ++pDest, ++pSrc)
		*pDest = *pSrc;
}

///////////////////////////////////////////////////////////////////////////////
#ifndef _UNICODE
#define CHAR_FUDGE 1    // one NTCHAR unused is good enough
#else
#define CHAR_FUDGE 2    // two BYTES unused for case of DBC last char
#endif
//WINSTR.CPP
int AFXAPI AfxLoadStringN(UINT nID, LPNTSTR lpszBuf, UINT nMaxBuf);
BOOL CStringN::LoadString(UINT nID)
{
	// try fixed buffer first (to avoid wasting space in the heap)
	NTCHAR szTemp[256];
	int nLen = AfxLoadStringN(nID, szTemp, _countof(szTemp));
	if (_countof(szTemp) - nLen > CHAR_FUDGE)
	{
		*this = szTemp;
		return nLen > 0;
	}

	// try buffer size of 512, then larger size until entire string is retrieved
	int nSize = 256;
	do
	{
		nSize += 256;
		nLen = AfxLoadStringN(nID, GetBuffer(nSize-1), nSize);
	} while (nSize - nLen <= CHAR_FUDGE);
	ReleaseBuffer();

	return nLen > 0;
}

#ifndef _AFXDLL
int AFXAPI AfxLoadStringN(UINT nID, LPNTSTR lpszBuf, UINT nMaxBuf)
{
	ASSERT(AfxIsValidAddress(lpszBuf, nMaxBuf*sizeof(NTCHAR)));
#ifdef UNICODE
#ifdef _DEBUG
	// LoadString without annoying warning from the Debug kernel if the
	//  segment containing the string is not present
	if (::FindResourceA(AfxGetResourceHandle(),
						MAKEINTRESOURCEA((nID>>4)+1), MAKEINTRESOURCEA(RT_STRING)) == NULL)
	{
		lpszBuf[0] = '\0';
		return 0; // not found
	}
#endif //_DEBUG
	int nLen = ::LoadStringA(AfxGetResourceHandle(), nID, lpszBuf, nMaxBuf);
	if (nLen == 0)
		lpszBuf[0] = '\0';
	return nLen;
#else
	// LoadString without annoying warning from the Debug kernel if the
	//  segment containing the string is not present
#ifdef _DEBUG
	if (::FindResourceW(AfxGetResourceHandle(),
						MAKEINTRESOURCEW((nID>>4)+1), MAKEINTRESOURCEW(RT_STRING)) == NULL)
	{
		lpszBuf[0] = '\0';
		return 0; // not found
	}
#endif //_DEBUG
	int nLen = ::LoadStringW(AfxGetResourceHandle(), nID, lpszBuf, nMaxBuf);
	if (nLen == 0)
		lpszBuf[0] = '\0';
	return nLen;
#endif
}
#endif

///////////////////////////////////////////////////////////////////////////////
// OLE BSTR support

BSTR CStringN::AllocSysString() const
{
#if defined(_UNICODE) || defined(OLE2ANSI)
	int nLen = MultiByteToWideChar(CP_ACP, 0, m_pchData,
									GetData()->nDataLength, NULL, NULL);
	BSTR bstr = ::SysAllocStringLen(NULL, nLen);
	if (bstr == NULL)
		AfxThrowMemoryException();
	MultiByteToWideChar(CP_ACP, 0, m_pchData, GetData()->nDataLength,
						bstr, nLen);
#else
	BSTR bstr = ::SysAllocStringLen(m_pchData, GetData()->nDataLength);
	if (bstr == NULL)
		AfxThrowMemoryException();
#endif

	return bstr;
}

BSTR CStringN::SetSysString(BSTR* pbstr) const
{
	ASSERT(AfxIsValidAddress(pbstr, sizeof(BSTR)));

#if ! defined(_UNICODE) || defined(OLE2ANSI)
	if (!::SysReAllocStringLen(pbstr, m_pchData, GetData()->nDataLength))
		AfxThrowMemoryException();
#else
	int nLen = MultiByteToWideChar(CP_ACP, 0, m_pchData,
									GetData()->nDataLength, NULL, NULL);
	if (!::SysReAllocStringLen(pbstr, NULL, nLen))
		AfxThrowMemoryException();
	MultiByteToWideChar(CP_ACP, 0, m_pchData, GetData()->nDataLength,
						*pbstr, nLen);
#endif

	ASSERT(*pbstr != NULL);
	return *pbstr;
}

