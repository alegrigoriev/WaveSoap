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
#include "CStringW.h"
#ifdef AFX_CORE1_SEG
#pragma code_seg(AFX_CORE1_SEG)
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// static class data, special inlines

// afxChNil is left for backward compatibility
AFX_DATADEF TCHAR afxChNil = '\0';

// For an empty string, m_pchData will point here
// (note: avoids special case of checking for NULL m_pchData)
// empty string data (and locked)
AFX_STATIC_DATA int _afxInitData[] = { -1, 0, 0, 0 };
AFX_STATIC_DATA CStringData* _afxDataNil = (CStringData*)&_afxInitData;
AFX_COMDAT LPCTSTR _afxPchNil = (LPCTSTR)(((BYTE*)&_afxInitData)+sizeof(CStringData));
// special function to make afxEmptyString work even during initialization
const CString& AFXAPI AfxGetEmptyString()
{ return *(CString*)&_afxPchNil; }

//////////////////////////////////////////////////////////////////////////////
// Construction/Destruction

#ifdef _AFXDLL
CString::CString()
{
	Init();
}
#endif

CString::CString(const CString& stringSrc)
{
	ASSERT(stringSrc.GetData()->nRefs != 0);
	if (stringSrc.GetData()->nRefs >= 0)
	{
		ASSERT(stringSrc.GetData() != _afxDataNil);
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
AFX_STATIC CFixedAlloc _afxAlloc64(ROUND4(65*sizeof(TCHAR)+sizeof(CStringData)));
AFX_STATIC CFixedAlloc _afxAlloc128(ROUND4(129*sizeof(TCHAR)+sizeof(CStringData)));
AFX_STATIC CFixedAlloc _afxAlloc256(ROUND4(257*sizeof(TCHAR)+sizeof(CStringData)));
AFX_STATIC CFixedAlloc _afxAlloc512(ROUND4(513*sizeof(TCHAR)+sizeof(CStringData)));

#endif //!_DEBUG

void CString::AllocBuffer(int nLen)
// always allocate one extra character for '\0' termination
// assumes [optimistically] that data length will equal allocation length
{
	ASSERT(nLen >= 0);
	ASSERT(nLen <= INT_MAX-1);    // max size (enough room for 1 extra)

	if (nLen == 0)
		Init();
	else
	{
		CStringData* pData;
#ifndef _DEBUG
		if (nLen <= 64)
		{
			pData = (CStringData*)_afxAlloc64.Alloc();
			pData->nAllocLength = 64;
		}
		else if (nLen <= 128)
		{
			pData = (CStringData*)_afxAlloc128.Alloc();
			pData->nAllocLength = 128;
		}
		else if (nLen <= 256)
		{
			pData = (CStringData*)_afxAlloc256.Alloc();
			pData->nAllocLength = 256;
		}
		else if (nLen <= 512)
		{
			pData = (CStringData*)_afxAlloc512.Alloc();
			pData->nAllocLength = 512;
		}
		else
#endif
		{
			pData = (CStringData*)
					new BYTE[sizeof(CStringData) + (nLen+1)*sizeof(TCHAR)];
			pData->nAllocLength = nLen;
		}
		pData->nRefs = 1;
		pData->data()[nLen] = '\0';
		pData->nDataLength = nLen;
		m_pchData = pData->data();
	}
}

void FASTCALL CString::FreeData(CStringData* pData)
{
#ifndef _DEBUG
	int nLen = pData->nAllocLength;
	if (nLen == 64)
		_afxAlloc64.Free(pData);
	else if (nLen == 128)
		_afxAlloc128.Free(pData);
	else if (nLen == 256)
		_afxAlloc256.Free(pData);
	else  if (nLen == 512)
		_afxAlloc512.Free(pData);
	else
	{
		ASSERT(nLen > 512);
		delete[] (BYTE*)pData;
	}
#else
	delete[] (BYTE*)pData;
#endif
}

void CString::Release()
{
	if (GetData() != _afxDataNil)
	{
		ASSERT(GetData()->nRefs != 0);
		if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			FreeData(GetData());
		Init();
	}
}

void PASCAL CString::Release(CStringData* pData)
{
	if (pData != _afxDataNil)
	{
		ASSERT(pData->nRefs != 0);
		if (InterlockedDecrement(&pData->nRefs) <= 0)
			FreeData(pData);
	}
}

void CString::Empty()
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

void CString::CopyBeforeWrite()
{
	if (GetData()->nRefs > 1)
	{
		CStringData* pData = GetData();
		Release();
		AllocBuffer(pData->nDataLength);
		memcpy(m_pchData, pData->data(), (pData->nDataLength+1)*sizeof(TCHAR));
	}
	ASSERT(GetData()->nRefs <= 1);
}

void CString::AllocBeforeWrite(int nLen)
{
	if (GetData()->nRefs > 1 || nLen > GetData()->nAllocLength)
	{
		Release();
		AllocBuffer(nLen);
	}
	ASSERT(GetData()->nRefs <= 1);
}

CString::~CString()
//  free any attached data
{
	if (GetData() != _afxDataNil)
	{
		if (InterlockedDecrement(&GetData()->nRefs) <= 0)
			FreeData(GetData());
	}
}

//////////////////////////////////////////////////////////////////////////////
// Helpers for the rest of the implementation

void CString::AllocCopy(CString& dest, int nCopyLen, int nCopyIndex,
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
		memcpy(dest.m_pchData, m_pchData+nCopyIndex, nCopyLen*sizeof(TCHAR));
	}
}

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CString::CString(LPCTSTR lpsz)
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
			memcpy(m_pchData, lpsz, nLen*sizeof(TCHAR));
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion constructors

#ifdef _UNICODE
CString::CString(LPCSTR lpsz)
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
CString::CString(LPCWSTR lpsz)
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
CDumpContext& AFXAPI operator<<(CDumpContext& dc, const CString& string)
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
//  All routines return the new string (but as a 'const CString&' so that
//      assigning it again will cause a copy, eg: s1 = s2 = "hi there".
//

void CString::AssignCopy(int nSrcLen, LPCTSTR lpszSrcData)
{
	AllocBeforeWrite(nSrcLen);
	memcpy(m_pchData, lpszSrcData, nSrcLen*sizeof(TCHAR));
	GetData()->nDataLength = nSrcLen;
	m_pchData[nSrcLen] = '\0';
}

const CString& CString::operator=(const CString& stringSrc)
{
	if (m_pchData != stringSrc.m_pchData)
	{
		if ((GetData()->nRefs < 0 && GetData() != _afxDataNil) ||
			stringSrc.GetData()->nRefs < 0)
		{
			// actual copy necessary since one of the strings is locked
			AssignCopy(stringSrc.GetData()->nDataLength, stringSrc.m_pchData);
		}
		else
		{
			// can just copy references around
			Release();
			ASSERT(stringSrc.GetData() != _afxDataNil);
			m_pchData = stringSrc.m_pchData;
			InterlockedIncrement(&GetData()->nRefs);
		}
	}
	return *this;
}

const CString& CString::operator=(LPCTSTR lpsz)
{
	ASSERT(lpsz == NULL || AfxIsValidString(lpsz));
	AssignCopy(SafeStrlen(lpsz), lpsz);
	return *this;
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion assignment

#ifdef _UNICODE
const CString& CString::operator=(LPCSTR lpsz)
{
	int nSrcLen = lpsz != NULL ? lstrlenA(lpsz) : 0;
	AllocBeforeWrite(nSrcLen);
	_mbstowcsz(m_pchData, lpsz, nSrcLen+1);
	ReleaseBuffer();
	return *this;
}
#else //!_UNICODE
const CString& CString::operator=(LPCWSTR lpsz)
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
//          CString + CString
// and for ? = TCHAR, LPCTSTR
//          CString + ?
//          ? + CString

void CString::ConcatCopy(int nSrc1Len, LPCTSTR lpszSrc1Data,
						int nSrc2Len, LPCTSTR lpszSrc2Data)
{
	// -- master concatenation routine
	// Concatenate two sources
	// -- assume that 'this' is a new CString object

	int nNewLen = nSrc1Len + nSrc2Len;
	if (nNewLen != 0)
	{
		AllocBuffer(nNewLen);
		memcpy(m_pchData, lpszSrc1Data, nSrc1Len*sizeof(TCHAR));
		memcpy(m_pchData+nSrc1Len, lpszSrc2Data, nSrc2Len*sizeof(TCHAR));
	}
}

CString AFXAPI operator+(const CString& string1, const CString& string2)
{
	CString s;
	s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData,
				string2.GetData()->nDataLength, string2.m_pchData);
	return s;
}

CString AFXAPI operator+(const CString& string, LPCTSTR lpsz)
{
	ASSERT(lpsz == NULL || AfxIsValidString(lpsz));
	CString s;
	s.ConcatCopy(string.GetData()->nDataLength, string.m_pchData,
				CString::SafeStrlen(lpsz), lpsz);
	return s;
}

CString AFXAPI operator+(LPCTSTR lpsz, const CString& string)
{
	ASSERT(lpsz == NULL || AfxIsValidString(lpsz));
	CString s;
	s.ConcatCopy(CString::SafeStrlen(lpsz), lpsz, string.GetData()->nDataLength,
				string.m_pchData);
	return s;
}

//////////////////////////////////////////////////////////////////////////////
// concatenate in place

void CString::ConcatInPlace(int nSrcLen, LPCTSTR lpszSrcData)
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
		CStringData* pOldData = GetData();
		ConcatCopy(GetData()->nDataLength, m_pchData, nSrcLen, lpszSrcData);
		ASSERT(pOldData != NULL);
		CString::Release(pOldData);
	}
	else
	{
		// fast concatenation when buffer big enough
		memcpy(m_pchData+GetData()->nDataLength, lpszSrcData, nSrcLen*sizeof(TCHAR));
		GetData()->nDataLength += nSrcLen;
		ASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
		m_pchData[GetData()->nDataLength] = '\0';
	}
}

const CString& CString::operator+=(LPCTSTR lpsz)
{
	ASSERT(lpsz == NULL || AfxIsValidString(lpsz));
	ConcatInPlace(SafeStrlen(lpsz), lpsz);
	return *this;
}

const CString& CString::operator+=(TCHAR ch)
{
	ConcatInPlace(1, &ch);
	return *this;
}

const CString& CString::operator+=(const CString& string)
{
	ConcatInPlace(string.GetData()->nDataLength, string.m_pchData);
	return *this;
}

///////////////////////////////////////////////////////////////////////////////
// Advanced direct buffer access

LPTSTR CString::GetBuffer(int nMinBufLength)
{
	ASSERT(nMinBufLength >= 0);

	if (GetData()->nRefs > 1 || nMinBufLength > GetData()->nAllocLength)
	{
#ifdef _DEBUG
		// give a warning in case locked string becomes unlocked
		if (GetData() != _afxDataNil && GetData()->nRefs < 0)
			TRACE0("Warning: GetBuffer on locked CString creates unlocked CString!\n");
#endif
		// we have to grow the buffer
		CStringData* pOldData = GetData();
		int nOldLen = GetData()->nDataLength;   // AllocBuffer will tromp it
		if (nMinBufLength < nOldLen)
			nMinBufLength = nOldLen;
		AllocBuffer(nMinBufLength);
		memcpy(m_pchData, pOldData->data(), (nOldLen+1)*sizeof(TCHAR));
		GetData()->nDataLength = nOldLen;
		CString::Release(pOldData);
	}
	ASSERT(GetData()->nRefs <= 1);

	// return a pointer to the character storage for this string
	ASSERT(m_pchData != NULL);
	return m_pchData;
}

void CString::ReleaseBuffer(int nNewLength)
{
	CopyBeforeWrite();  // just in case GetBuffer was not called

	if (nNewLength == -1)
		nNewLength = lstrlen(m_pchData); // zero terminated

	ASSERT(nNewLength <= GetData()->nAllocLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
}

LPTSTR CString::GetBufferSetLength(int nNewLength)
{
	ASSERT(nNewLength >= 0);

	GetBuffer(nNewLength);
	GetData()->nDataLength = nNewLength;
	m_pchData[nNewLength] = '\0';
	return m_pchData;
}

void CString::FreeExtra()
{
	ASSERT(GetData()->nDataLength <= GetData()->nAllocLength);
	if (GetData()->nDataLength != GetData()->nAllocLength)
	{
		CStringData* pOldData = GetData();
		AllocBuffer(GetData()->nDataLength);
		memcpy(m_pchData, pOldData->data(), pOldData->nDataLength*sizeof(TCHAR));
		ASSERT(m_pchData[GetData()->nDataLength] == '\0');
		CString::Release(pOldData);
	}
	ASSERT(GetData() != NULL);
}

LPTSTR CString::LockBuffer()
{
	LPTSTR lpsz = GetBuffer(0);
	GetData()->nRefs = -1;
	return lpsz;
}

void CString::UnlockBuffer()
{
	ASSERT(GetData()->nRefs == -1);
	if (GetData() != _afxDataNil)
		GetData()->nRefs = 1;
}

///////////////////////////////////////////////////////////////////////////////
// Commonly used routines (rarely used routines in STREX.CPP)

int CString::Find(TCHAR ch) const
{
	return Find(ch, 0);
}

int CString::Find(TCHAR ch, int nStart) const
{
	int nLength = GetData()->nDataLength;
	if (nStart >= nLength)
		return -1;

	// find first single character
	LPTSTR lpsz = _tcschr(m_pchData + nStart, (_TUCHAR)ch);

	// return -1 if not found and index otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

int CString::FindOneOf(LPCTSTR lpszCharSet) const
{
	ASSERT(AfxIsValidString(lpszCharSet));
	LPTSTR lpsz = _tcspbrk(m_pchData, lpszCharSet);
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

void CString::MakeUpper()
{
	CopyBeforeWrite();
	_tcsupr(m_pchData);
}

void CString::MakeLower()
{
	CopyBeforeWrite();
	_tcslwr(m_pchData);
}

void CString::MakeReverse()
{
	CopyBeforeWrite();
	_tcsrev(m_pchData);
}

void CString::SetAt(int nIndex, TCHAR ch)
{
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);

	CopyBeforeWrite();
	m_pchData[nIndex] = ch;
}

#ifndef _UNICODE
void CString::AnsiToOem()
{
	CopyBeforeWrite();
	::AnsiToOem(m_pchData, m_pchData);
}
void CString::OemToAnsi()
{
	CopyBeforeWrite();
	::OemToAnsi(m_pchData, m_pchData);
}
#endif

///////////////////////////////////////////////////////////////////////////////
// CString conversion helpers (these use the current system locale)

int AFX_CDECL _wcstombsz(char* mbstr, const wchar_t* wcstr, size_t count)
{
	if (count == 0 && mbstr != NULL)
		return 0;

	int result = ::WideCharToMultiByte(CP_ACP, 0, wcstr, -1,
										mbstr, count, NULL, NULL);
	ASSERT(mbstr == NULL || result <= (int)count);
	if (result > 0)
		mbstr[result-1] = 0;
	return result;
}

int AFX_CDECL _mbstowcsz(wchar_t* wcstr, const char* mbstr, size_t count)
{
	if (count == 0 && wcstr != NULL)
		return 0;

	int result = ::MultiByteToWideChar(CP_ACP, 0, mbstr, -1,
										wcstr, count);
	ASSERT(wcstr == NULL || result <= (int)count);
	if (result > 0)
		wcstr[result-1] = 0;
	return result;
}

LPWSTR AFXAPI AfxA2WHelper(LPWSTR lpw, LPCSTR lpa, int nChars)
{
	if (lpa == NULL)
		return NULL;
	ASSERT(lpw != NULL);
	// verify that no illegal character present
	// since lpw was allocated based on the size of lpa
	// don't worry about the number of chars
	lpw[0] = '\0';
	VERIFY(MultiByteToWideChar(CP_ACP, 0, lpa, -1, lpw, nChars));
	return lpw;
}

LPSTR AFXAPI AfxW2AHelper(LPSTR lpa, LPCWSTR lpw, int nChars)
{
	if (lpw == NULL)
		return NULL;
	ASSERT(lpa != NULL);
	// verify that no illegal character present
	// since lpa was allocated based on the size of lpw
	// don't worry about the number of chars
	lpa[0] = '\0';
	VERIFY(WideCharToMultiByte(CP_ACP, 0, lpw, -1, lpa, nChars, NULL, NULL));
	return lpa;
}

///////////////////////////////////////////////////////////////////////////////

//#include <strex.cpp>
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

#ifdef AFX_AUX_SEG
#pragma code_seg(AFX_AUX_SEG)
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

//////////////////////////////////////////////////////////////////////////////
// More sophisticated construction

CString::CString(TCHAR ch, int nLength)
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

CString::CString(LPCTSTR lpch, int nLength)
{
	Init();
	if (nLength != 0)
	{
		ASSERT(AfxIsValidAddress(lpch, nLength, FALSE));
		AllocBuffer(nLength);
		memcpy(m_pchData, lpch, nLength*sizeof(TCHAR));
	}
}

/////////////////////////////////////////////////////////////////////////////
// Special conversion constructors

#ifdef _UNICODE
CString::CString(LPCSTR lpsz, int nLength)
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
CString::CString(LPCWSTR lpsz, int nLength)
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

const CString& CString::operator=(TCHAR ch)
{
	AssignCopy(1, &ch);
	return *this;
}

//////////////////////////////////////////////////////////////////////////////
// less common string expressions

CString AFXAPI operator+(const CString& string1, TCHAR ch)
{
	CString s;
	s.ConcatCopy(string1.GetData()->nDataLength, string1.m_pchData, 1, &ch);
	return s;
}

CString AFXAPI operator+(TCHAR ch, const CString& string)
{
	CString s;
	s.ConcatCopy(1, &ch, string.GetData()->nDataLength, string.m_pchData);
	return s;
}

//////////////////////////////////////////////////////////////////////////////
// Advanced manipulation

int CString::Delete(int nIndex, int nCount /* = 1 */)
{
	if (nIndex < 0)
		nIndex = 0;
	int nNewLength = GetData()->nDataLength;
	if (nCount > 0 && nIndex < nNewLength)
	{
		CopyBeforeWrite();
		int nBytesToCopy = nNewLength - (nIndex + nCount) + 1;

		memcpy(m_pchData + nIndex,
				m_pchData + nIndex + nCount, nBytesToCopy * sizeof(TCHAR));
		GetData()->nDataLength = nNewLength - nCount;
	}

	return nNewLength;
}

int CString::Insert(int nIndex, TCHAR ch)
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
		CStringData* pOldData = GetData();
		LPTSTR pstr = m_pchData;
		AllocBuffer(nNewLength);
		memcpy(m_pchData, pstr, (pOldData->nDataLength+1)*sizeof(TCHAR));
		CString::Release(pOldData);
	}

	// move existing bytes down
	memcpy(m_pchData + nIndex + 1,
			m_pchData + nIndex, (nNewLength-nIndex)*sizeof(TCHAR));
	m_pchData[nIndex] = ch;
	GetData()->nDataLength = nNewLength;

	return nNewLength;
}

int CString::Insert(int nIndex, LPCTSTR pstr)
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
			CStringData* pOldData = GetData();
			LPTSTR pstr = m_pchData;
			AllocBuffer(nNewLength);
			memcpy(m_pchData, pstr, (pOldData->nDataLength+1)*sizeof(TCHAR));
			CString::Release(pOldData);
		}

		// move existing bytes down
		memcpy(m_pchData + nIndex + nInsertLength,
				m_pchData + nIndex,
				(nNewLength-nIndex-nInsertLength+1)*sizeof(TCHAR));
		memcpy(m_pchData + nIndex,
				pstr, nInsertLength*sizeof(TCHAR));
		GetData()->nDataLength = nNewLength;
	}

	return nNewLength;
}

int CString::Replace(TCHAR chOld, TCHAR chNew)
{
	int nCount = 0;

	// short-circuit the nop case
	if (chOld != chNew)
	{
		// otherwise modify each character that matches in the string
		CopyBeforeWrite();
		LPTSTR psz = m_pchData;
		LPTSTR pszEnd = psz + GetData()->nDataLength;
		while (psz < pszEnd)
		{
			// replace instances of the specified character only
			if (*psz == chOld)
			{
				*psz = chNew;
				nCount++;
			}
			psz = _tcsinc(psz);
		}
	}
	return nCount;
}

int CString::Replace(LPCTSTR lpszOld, LPCTSTR lpszNew)
{
	// can't have empty or NULL lpszOld

	int nSourceLen = SafeStrlen(lpszOld);
	if (nSourceLen == 0)
		return 0;
	int nReplacementLen = SafeStrlen(lpszNew);

	// loop once to figure out the size of the result string
	int nCount = 0;
	LPTSTR lpszStart = m_pchData;
	LPTSTR lpszEnd = m_pchData + GetData()->nDataLength;
	LPTSTR lpszTarget;
	while (lpszStart < lpszEnd)
	{
		while ((lpszTarget = _tcsstr(lpszStart, lpszOld)) != NULL)
		{
			nCount++;
			lpszStart = lpszTarget + nSourceLen;
		}
		lpszStart += lstrlen(lpszStart) + 1;
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
			CStringData* pOldData = GetData();
			LPTSTR pstr = m_pchData;
			AllocBuffer(nNewLength);
			memcpy(m_pchData, pstr, pOldData->nDataLength*sizeof(TCHAR));
			CString::Release(pOldData);
		}
		// else, we just do it in-place
		lpszStart = m_pchData;
		lpszEnd = m_pchData + GetData()->nDataLength;

		// loop again to actually do the work
		while (lpszStart < lpszEnd)
		{
			while ( (lpszTarget = _tcsstr(lpszStart, lpszOld)) != NULL)
			{
				int nBalance = nOldLength - (lpszTarget - m_pchData + nSourceLen);
				memmove(lpszTarget + nReplacementLen, lpszTarget + nSourceLen,
						nBalance * sizeof(TCHAR));
				memcpy(lpszTarget, lpszNew, nReplacementLen*sizeof(TCHAR));
				lpszStart = lpszTarget + nReplacementLen;
				lpszStart[nBalance] = '\0';
				nOldLength += (nReplacementLen - nSourceLen);
			}
			lpszStart += lstrlen(lpszStart) + 1;
		}
		ASSERT(m_pchData[nNewLength] == '\0');
		GetData()->nDataLength = nNewLength;
	}

	return nCount;
}

int CString::Remove(TCHAR chRemove)
{
	CopyBeforeWrite();

	LPTSTR pstrSource = m_pchData;
	LPTSTR pstrDest = m_pchData;
	LPTSTR pstrEnd = m_pchData + GetData()->nDataLength;

	while (pstrSource < pstrEnd)
	{
		if (*pstrSource != chRemove)
		{
			*pstrDest = *pstrSource;
			pstrDest = _tcsinc(pstrDest);
		}
		pstrSource = _tcsinc(pstrSource);
	}
	*pstrDest = '\0';
	int nCount = pstrSource - pstrDest;
	GetData()->nDataLength -= nCount;

	return nCount;
}

//////////////////////////////////////////////////////////////////////////////
// Very simple sub-string extraction

CString CString::Mid(int nFirst) const
{
	return Mid(nFirst, GetData()->nDataLength - nFirst);
}

CString CString::Mid(int nFirst, int nCount) const
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

	CString dest;
	AllocCopy(dest, nCount, nFirst, 0);
	return dest;
}

CString CString::Right(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	if (nCount >= GetData()->nDataLength)
		return *this;

	CString dest;
	AllocCopy(dest, nCount, GetData()->nDataLength-nCount, 0);
	return dest;
}

CString CString::Left(int nCount) const
{
	if (nCount < 0)
		nCount = 0;
	if (nCount >= GetData()->nDataLength)
		return *this;

	CString dest;
	AllocCopy(dest, nCount, 0, 0);
	return dest;
}

// strspn equivalent
CString CString::SpanIncluding(LPCTSTR lpszCharSet) const
{
	ASSERT(AfxIsValidString(lpszCharSet));
	return Left(_tcsspn(m_pchData, lpszCharSet));
}

// strcspn equivalent
CString CString::SpanExcluding(LPCTSTR lpszCharSet) const
{
	ASSERT(AfxIsValidString(lpszCharSet));
	return Left(_tcscspn(m_pchData, lpszCharSet));
}

//////////////////////////////////////////////////////////////////////////////
// Finding

int CString::ReverseFind(TCHAR ch) const
{
	// find last single character
	LPTSTR lpsz = _tcsrchr(m_pchData, (_TUCHAR) ch);

	// return -1 if not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}

// find a sub-string (like strstr)
int CString::Find(LPCTSTR lpszSub) const
{
	return Find(lpszSub, 0);
}

int CString::Find(LPCTSTR lpszSub, int nStart) const
{
	ASSERT(AfxIsValidString(lpszSub));

	int nLength = GetData()->nDataLength;
	if (nStart > nLength)
		return -1;

	// find first matching substring
	LPTSTR lpsz = _tcsstr(m_pchData + nStart, lpszSub);

	// return -1 for not found, distance from beginning otherwise
	return (lpsz == NULL) ? -1 : (int)(lpsz - m_pchData);
}


/////////////////////////////////////////////////////////////////////////////
// CString formatting

#define TCHAR_ARG   TCHAR
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

void CString::FormatV(LPCTSTR lpszFormat, va_list argList)
{
	ASSERT(AfxIsValidString(lpszFormat));

	va_list argListSave = argList;

	// make a guess at the maximum length of the resulting string
	int nMaxLen = 0;
	for (LPCTSTR lpsz = lpszFormat; *lpsz != '\0'; lpsz = _tcsinc(lpsz))
	{
		// handle '%' character, but watch out for '%%'
		if (*lpsz != '%' || *(lpsz = _tcsinc(lpsz)) == '%')
		{
			nMaxLen += _tclen(lpsz);
			continue;
		}

		int nItemLen = 0;

		// handle '%' character with format
		int nWidth = 0;
		for (; *lpsz != '\0'; lpsz = _tcsinc(lpsz))
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
			nWidth = _ttoi(lpsz);
			for (; *lpsz != '\0' && _istdigit(*lpsz); lpsz = _tcsinc(lpsz))
				;
		}
		ASSERT(nWidth >= 0);

		int nPrecision = 0;
		if (*lpsz == '.')
		{
			// skip past '.' separator (width.precision)
			lpsz = _tcsinc(lpsz);

			// get precision and skip it
			if (*lpsz == '*')
			{
				nPrecision = va_arg(argList, int);
				lpsz = _tcsinc(lpsz);
			}
			else
			{
				nPrecision = _ttoi(lpsz);
				for (; *lpsz != '\0' && _istdigit(*lpsz); lpsz = _tcsinc(lpsz))
					;
			}
			ASSERT(nPrecision >= 0);
		}

		// should be on type modifier or specifier
		int nModifier = 0;
		if (_tcsncmp(lpsz, _T("I64"), 3) == 0)
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
				lpsz = _tcsinc(lpsz);
				break;
			case 'l':
				nModifier = FORCE_UNICODE;
				lpsz = _tcsinc(lpsz);
				break;

				// modifiers that do not affect size
			case 'F':
			case 'N':
			case 'L':
				lpsz = _tcsinc(lpsz);
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
			LPCTSTR pstrNextArg = va_arg(argList, LPCTSTR);
			if (pstrNextArg == NULL)
				nItemLen = 6;  // "(null)"
			else
			{
				nItemLen = lstrlen(pstrNextArg);
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
				LPTSTR pszTemp;

				// 312 == strlen("-1+(309 zeroes).")
				// 309 zeroes == max precision of a double
				// 6 == adjustment in case precision is not specified,
				//   which means that the precision defaults to 6
				pszTemp = (LPTSTR)_alloca(max(nWidth, 312+nPrecision+6));

				f = va_arg(argList, double);
				_stprintf( pszTemp, _T( "%*.*f" ), nWidth, nPrecision+6, f );
				nItemLen = _tcslen(pszTemp);
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
	VERIFY(_vstprintf(m_pchData, lpszFormat, argListSave) <= GetAllocLength());
	ReleaseBuffer();

	va_end(argListSave);
}

// formatting (using wsprintf style formatting)
void AFX_CDECL CString::Format(LPCTSTR lpszFormat, ...)
{
	ASSERT(AfxIsValidString(lpszFormat));

	va_list argList;
	va_start(argList, lpszFormat);
	FormatV(lpszFormat, argList);
	va_end(argList);
}

void AFX_CDECL CString::Format(UINT nFormatID, ...)
{
	CString strFormat;
	VERIFY(strFormat.LoadString(nFormatID) != 0);

	va_list argList;
	va_start(argList, nFormatID);
	FormatV(strFormat, argList);
	va_end(argList);
}

// formatting (using FormatMessage style formatting)
void AFX_CDECL CString::FormatMessage(LPCTSTR lpszFormat, ...)
{
	// format message into temporary buffer lpszTemp
	va_list argList;
	va_start(argList, lpszFormat);
	LPTSTR lpszTemp;

	if (::FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
						lpszFormat, 0, 0, (LPTSTR)&lpszTemp, 0, &argList) == 0 ||
		lpszTemp == NULL)
	{
		AfxThrowMemoryException();
	}

	// assign lpszTemp into the resulting string and free the temporary
	*this = lpszTemp;
	LocalFree(lpszTemp);
	va_end(argList);
}

void AFX_CDECL CString::FormatMessage(UINT nFormatID, ...)
{
	// get format string from string table
	CString strFormat;
	VERIFY(strFormat.LoadString(nFormatID) != 0);

	// format message into temporary buffer lpszTemp
	va_list argList;
	va_start(argList, nFormatID);
	LPTSTR lpszTemp;
	if (::FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
						strFormat, 0, 0, (LPTSTR)&lpszTemp, 0, &argList) == 0 ||
		lpszTemp == NULL)
	{
		AfxThrowMemoryException();
	}

	// assign lpszTemp into the resulting string and free lpszTemp
	*this = lpszTemp;
	LocalFree(lpszTemp);
	va_end(argList);
}

void CString::TrimRight(LPCTSTR lpszTargetList)
{
	// find beginning of trailing matches
	// by starting at beginning (DBCS aware)

	CopyBeforeWrite();
	LPTSTR lpsz = m_pchData;
	LPTSTR lpszLast = NULL;

	while (*lpsz != '\0')
	{
		if (_tcschr(lpszTargetList, *lpsz) != NULL)
		{
			if (lpszLast == NULL)
				lpszLast = lpsz;
		}
		else
			lpszLast = NULL;
		lpsz = _tcsinc(lpsz);
	}

	if (lpszLast != NULL)
	{
		// truncate at left-most matching character
		*lpszLast = '\0';
		GetData()->nDataLength = lpszLast - m_pchData;
	}
}

void CString::TrimRight(TCHAR chTarget)
{
	// find beginning of trailing matches
	// by starting at beginning (DBCS aware)

	CopyBeforeWrite();
	LPTSTR lpsz = m_pchData;
	LPTSTR lpszLast = NULL;

	while (*lpsz != '\0')
	{
		if (*lpsz == chTarget)
		{
			if (lpszLast == NULL)
				lpszLast = lpsz;
		}
		else
			lpszLast = NULL;
		lpsz = _tcsinc(lpsz);
	}

	if (lpszLast != NULL)
	{
		// truncate at left-most matching character
		*lpszLast = '\0';
		GetData()->nDataLength = lpszLast - m_pchData;
	}
}

void CString::TrimRight()
{
	// find beginning of trailing spaces by starting at beginning (DBCS aware)

	CopyBeforeWrite();
	LPTSTR lpsz = m_pchData;
	LPTSTR lpszLast = NULL;

	while (*lpsz != '\0')
	{
		if (_istspace(*lpsz))
		{
			if (lpszLast == NULL)
				lpszLast = lpsz;
		}
		else
			lpszLast = NULL;
		lpsz = _tcsinc(lpsz);
	}

	if (lpszLast != NULL)
	{
		// truncate at trailing space start
		*lpszLast = '\0';
		GetData()->nDataLength = lpszLast - m_pchData;
	}
}

void CString::TrimLeft(LPCTSTR lpszTargets)
{
	// if we're not trimming anything, we're not doing any work
	if (SafeStrlen(lpszTargets) == 0)
		return;

	CopyBeforeWrite();
	LPCTSTR lpsz = m_pchData;

	while (*lpsz != '\0')
	{
		if (_tcschr(lpszTargets, *lpsz) == NULL)
			break;
		lpsz = _tcsinc(lpsz);
	}

	if (lpsz != m_pchData)
	{
		// fix up data and length
		int nDataLength = GetData()->nDataLength - (lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(TCHAR));
		GetData()->nDataLength = nDataLength;
	}
}

void CString::TrimLeft(TCHAR chTarget)
{
	// find first non-matching character

	CopyBeforeWrite();
	LPCTSTR lpsz = m_pchData;

	while (chTarget == *lpsz)
		lpsz = _tcsinc(lpsz);

	if (lpsz != m_pchData)
	{
		// fix up data and length
		int nDataLength = GetData()->nDataLength - (lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(TCHAR));
		GetData()->nDataLength = nDataLength;
	}
}

void CString::TrimLeft()
{
	// find first non-space character

	CopyBeforeWrite();
	LPCTSTR lpsz = m_pchData;

	while (_istspace(*lpsz))
		lpsz = _tcsinc(lpsz);

	if (lpsz != m_pchData)
	{
		// fix up data and length
		int nDataLength = GetData()->nDataLength - (lpsz - m_pchData);
		memmove(m_pchData, lpsz, (nDataLength+1)*sizeof(TCHAR));
		GetData()->nDataLength = nDataLength;
	}
}

///////////////////////////////////////////////////////////////////////////////
// CString support for template collections

#if _MSC_VER >= 1100
template<> void AFXAPI ConstructElements<CString> (CString* pElements, int nCount)
#else
void AFXAPI ConstructElements(CString* pElements, int nCount)
#endif
{
	ASSERT(nCount == 0 ||
			AfxIsValidAddress(pElements, nCount * sizeof(CString)));

	for (; nCount--; ++pElements)
		memcpy(pElements, &afxEmptyString, sizeof(*pElements));
}

#if _MSC_VER >= 1100
template<> void AFXAPI DestructElements<CString> (CString* pElements, int nCount)
#else
void AFXAPI DestructElements(CString* pElements, int nCount)
#endif
{
	ASSERT(nCount == 0 ||
			AfxIsValidAddress(pElements, nCount * sizeof(CString)));

	for (; nCount--; ++pElements)
		pElements->~CString();
}

#if _MSC_VER >= 1100
template<> void AFXAPI CopyElements<CString> (CString* pDest, const CString* pSrc, int nCount)
#else
void AFXAPI CopyElements(CString* pDest, const CString* pSrc, int nCount)
#endif
{
	ASSERT(nCount == 0 ||
			AfxIsValidAddress(pDest, nCount * sizeof(CString)));
	ASSERT(nCount == 0 ||
			AfxIsValidAddress(pSrc, nCount * sizeof(CString)));

	for (; nCount--; ++pDest, ++pSrc)
		*pDest = *pSrc;
}

#ifndef OLE2ANSI
#if _MSC_VER >= 1100
template<> UINT AFXAPI HashKey<LPCWSTR> (LPCWSTR key)
#else
UINT AFXAPI HashKey(LPCWSTR key)
#endif
{
	UINT nHash = 0;
	while (*key)
		nHash = (nHash<<5) + nHash + *key++;
	return nHash;
}
#endif

#if _MSC_VER >= 1100
template<> UINT AFXAPI HashKey<LPCSTR> (LPCSTR key)
#else
UINT AFXAPI HashKey(LPCSTR key)
#endif
{
	UINT nHash = 0;
	while (*key)
		nHash = (nHash<<5) + nHash + *key++;
	return nHash;
}

///////////////////////////////////////////////////////////////////////////////
#ifdef _UNICODE
#define CHAR_FUDGE 1    // one TCHAR unused is good enough
#else
#define CHAR_FUDGE 2    // two BYTES unused for case of DBC last char
#endif
//WINSTR.CPP
BOOL CString::LoadString(UINT nID)
{
	// try fixed buffer first (to avoid wasting space in the heap)
	TCHAR szTemp[256];
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
int AFXAPI AfxLoadStringN(UINT nID, LPTSTR lpszBuf, UINT nMaxBuf)
{
	ASSERT(AfxIsValidAddress(lpszBuf, nMaxBuf*sizeof(TCHAR)));
#ifdef _DEBUG
	// LoadString without annoying warning from the Debug kernel if the
	//  segment containing the string is not present
#ifdef UNICODE
	if (::FindResourceA(AfxGetResourceHandle(),
						MAKEINTRESOURCE((nID>>4)+1), RT_STRING) == NULL)
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
#ifdef UNICODE
	if (::FindResourceW(AfxGetResourceHandle(),
						MAKEINTRESOURCE((nID>>4)+1), RT_STRING) == NULL)
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

BSTR CString::AllocSysString() const
{
#if defined(_UNICODE) || defined(OLE2ANSI)
	BSTR bstr = ::SysAllocStringLen(m_pchData, GetData()->nDataLength);
	if (bstr == NULL)
		AfxThrowMemoryException();
#else
	int nLen = MultiByteToWideChar(CP_ACP, 0, m_pchData,
									GetData()->nDataLength, NULL, NULL);
	BSTR bstr = ::SysAllocStringLen(NULL, nLen);
	if (bstr == NULL)
		AfxThrowMemoryException();
	MultiByteToWideChar(CP_ACP, 0, m_pchData, GetData()->nDataLength,
						bstr, nLen);
#endif

	return bstr;
}

BSTR CString::SetSysString(BSTR* pbstr) const
{
	ASSERT(AfxIsValidAddress(pbstr, sizeof(BSTR)));

#if defined(_UNICODE) || defined(OLE2ANSI)
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

