#ifndef __CSTRINGW_H_INCLUDED
#define __CSTRINGW_H_INCLUDED
#pragma once

#ifdef _UNICODE
typedef class CString CStringW;
typedef struct CStringData CStringDataW;

#define CStringN CStringA
#define CStringDataN CStringDataA
#define _nT(s) s

typedef char NTCHAR;
typedef unsigned char _NTUCHAR;

#define lstrcmpN  lstrcmpA
#define lstrcmpiN  lstrcmpiA
#define lstrcpynN  lstrcpynA
#define lstrcpyN  lstrcpyA
#define lstrcatN  lstrcatA
#define lstrlenN  lstrlenA

#define _tcslenN     strlen
#define _tcscmpN    strcmp
#define _tcsicmpN   _stricmp
#define _tcscollN    strcoll
#define _tcsicollN   _stricoll
#define _tcschrN      strchr
#define _tcspbrkN     strpbrk
#define _tcsuprN      _strupr
#define _tcslwrN      _strlwr
#define _tcsrevN      _strrev
//#define _tcsincN      _strinc
#define _tcsstrN      strstr
#define _tcsspnN      strspn
#define _tcscspnN     strcspn
#define _tcsrchrN     strrchr
#define _ttoiN        atoi
#define _istdigitN    isdigit
#define FormatMessageN FormatMessageA
#define _istspaceN    isspace
#define _vstprintfN  vsprintf
#define _stprintfN   sprintf
#define _tcsncmpN    strncmp
//#define _tclen
#define _tcsincN(_pc)    ((_pc)+1)
__inline size_t __cdecl _tclenN(const char *_cpc) { return (_cpc,1); }
//__inline void __cdecl _tccpy(wchar_t *_pc1, const wchar_t *_cpc2) { *_pc1 = (wchar_t)*_cpc2; }
//__inline int __cdecl _tccmp(const wchar_t *_cpc1, const wchar_t *_cpc2) { return (int) ((*_cpc1)-(*_cpc2)); }
#else
typedef class CString CStringA;
typedef struct CStringData CStringDataA;

#define CStringN CStringW
#define CStringDataN CStringDataW
#define _nT(s) L##s

typedef WCHAR NTCHAR;
typedef WCHAR _NTUCHAR;

#define lstrcmpN  lstrcmpW
#define lstrcmpiN  lstrcmpiW
#define lstrcpynN  lstrcpynW
#define lstrcpyN  lstrcpyW
#define lstrcatN  lstrcatW
#define lstrlenN  lstrlenW

#define _tcscmpN wcscmp
#define _tcsicmpN _wcsicmp
#define _tcscollN    wcscoll
#define _tcsicollN   _wcsicoll
#define _tcschrN      wcschr
#define _tcspbrkN     wcspbrk
#define _tcsuprN      _wcsupr
#define _tcslwrN      _wcslwr
#define _tcsrevN      _wcsrev
#define _tcslenN      wcslen
//#define _tcsincN      _wcsinc
#define _tcsstrN      wcsstr
#define _tcsspnN      wcsspn
#define _tcscspnN     wcscspn
#define _tcsrchrN     wcsrchr
#define _ttoiN         _wtoi
#define _istdigitN    iswdigit
#define FormatMessageN FormatMessageW
#define _istspaceN    iswspace
#define _vstprintfN  vswprintf
#define _stprintfN   swprintf
#define _tcsncmpN    wcsncmp
//#define _tclen
__inline size_t __cdecl _tclenN(const wchar_t *_cpc) { return (_cpc,1); }
#define _tcsincN(_pc)    ((_pc)+1)
//__inline void __cdecl _tccpy(wchar_t *_pc1, const wchar_t *_cpc2) { *_pc1 = (wchar_t)*_cpc2; }
//__inline int __cdecl _tccmp(const wchar_t *_cpc1, const wchar_t *_cpc2) { return (int) ((*_cpc1)-(*_cpc2)); }
#endif
typedef NTCHAR * LPNTCHAR;
typedef NTCHAR const * LPCNTCHAR;
typedef LPNTCHAR LPNTSTR;
typedef LPCNTCHAR LPCNTSTR;
// CStringN

struct CStringDataN
{
	long nRefs;             // reference count
	int nDataLength;        // length of data (including terminator)
	int nAllocLength;       // length of allocation
	// NTCHAR data[nAllocLength]

	NTCHAR* data()           // NTCHAR* to managed data
	{ return (NTCHAR*)(this+1); }
};

class CStringN
{
public:
// Constructors

	// constructs empty CStringN
	CStringN();
	// copy constructor
	CStringN(const CStringN& stringSrc);
	// from a single character
	CStringN(NTCHAR ch, int nRepeat = 1);
	// from an ANSI string (converts to NTCHAR)
	CStringN(LPCSTR lpsz);
	// from a UNICODE string (converts to NTCHAR)
	CStringN(LPCWSTR lpsz);
	// subset of characters from an ANSI string (converts to NTCHAR)
	CStringN(LPCSTR lpch, int nLength);
	// subset of characters from a UNICODE string (converts to NTCHAR)
	CStringN(LPCWSTR lpch, int nLength);
	// from unsigned characters
	CStringN(const unsigned char* psz);

// Attributes & Operations

	// get data length
	int GetLength() const;
	// TRUE if zero length
	BOOL IsEmpty() const;
	// clear contents to empty
	void Empty();

	// return single character at zero-based index
	NTCHAR GetAt(int nIndex) const;
	// return single character at zero-based index
	NTCHAR operator[](int nIndex) const;
	// set a single character at zero-based index
	void SetAt(int nIndex, NTCHAR ch);
	// return pointer to const string
	operator LPCNTSTR() const;

	// overloaded assignment

	// ref-counted copy from another CStringN
	const CStringN& operator=(const CStringN& stringSrc);
	// set string content to single character
	const CStringN& operator=(NTCHAR ch);
#ifndef _UNICODE
	const CStringN& operator=(char ch);
#endif
	// copy string content from ANSI string (converts to NTCHAR)
	const CStringN& operator=(LPCSTR lpsz);
	// copy string content from UNICODE string (converts to NTCHAR)
	const CStringN& operator=(LPCWSTR lpsz);
	// copy string content from unsigned chars
	const CStringN& operator=(const unsigned char* psz);

	// string concatenation

	// concatenate from another CStringN
	const CStringN& operator+=(const CStringN& string);

	// concatenate a single character
	const CStringN& operator+=(NTCHAR ch);
#ifndef _UNICODE
	// concatenate an ANSI character after converting it to NTCHAR
	const CStringN& operator+=(char ch);
#endif
	// concatenate a UNICODE character after converting it to NTCHAR
	const CStringN& operator+=(LPCNTSTR lpsz);

	friend CStringN AFXAPI operator+(const CStringN& string1,
									const CStringN& string2);
	friend CStringN AFXAPI operator+(const CStringN& string, NTCHAR ch);
	friend CStringN AFXAPI operator+(NTCHAR ch, const CStringN& string);
#ifndef _UNICODE
	friend CStringN AFXAPI operator+(const CStringN& string, char ch);
	friend CStringN AFXAPI operator+(char ch, const CStringN& string);
#endif
	friend CStringN AFXAPI operator+(const CStringN& string, LPCNTSTR lpsz);
	friend CStringN AFXAPI operator+(LPCNTSTR lpsz, const CStringN& string);

	// string comparison

	// straight character comparison
	int Compare(LPCNTSTR lpsz) const;
	// compare ignoring case
	int CompareNoCase(LPCNTSTR lpsz) const;
	// NLS aware comparison, case sensitive
	int Collate(LPCNTSTR lpsz) const;
	// NLS aware comparison, case insensitive
	int CollateNoCase(LPCNTSTR lpsz) const;

	// simple sub-string extraction

	// return nCount characters starting at zero-based nFirst
	CStringN Mid(int nFirst, int nCount) const;
	// return all characters starting at zero-based nFirst
	CStringN Mid(int nFirst) const;
	// return first nCount characters in string
	CStringN Left(int nCount) const;
	// return nCount characters from end of string
	CStringN Right(int nCount) const;

	//  characters from beginning that are also in passed string
	CStringN SpanIncluding(LPCNTSTR lpszCharSet) const;
	// characters from beginning that are not also in passed string
	CStringN SpanExcluding(LPCNTSTR lpszCharSet) const;

	// upper/lower/reverse conversion

	// NLS aware conversion to uppercase
	void MakeUpper();
	// NLS aware conversion to lowercase
	void MakeLower();
	// reverse string right-to-left
	void MakeReverse();

	// trimming whitespace (either side)

	// remove whitespace starting from right edge
	void TrimRight();
	// remove whitespace starting from left side
	void TrimLeft();

	// trimming anything (either side)

	// remove continuous occurrences of chTarget starting from right
	void TrimRight(NTCHAR chTarget);
	// remove continuous occcurrences of characters in passed string,
	// starting from right
	void TrimRight(LPCNTSTR lpszTargets);
	// remove continuous occurrences of chTarget starting from left
	void TrimLeft(NTCHAR chTarget);
	// remove continuous occcurrences of characters in
	// passed string, starting from left
	void TrimLeft(LPCNTSTR lpszTargets);

	// advanced manipulation

	// replace occurrences of chOld with chNew
	int Replace(NTCHAR chOld, NTCHAR chNew);
	// replace occurrences of substring lpszOld with lpszNew;
	// empty lpszNew removes instances of lpszOld
	int Replace(LPCNTSTR lpszOld, LPCNTSTR lpszNew);
	// remove occurrences of chRemove
	int Remove(NTCHAR chRemove);
	// insert character at zero-based index; concatenates
	// if index is past end of string
	int Insert(int nIndex, NTCHAR ch);
	// insert substring at zero-based index; concatenates
	// if index is past end of string
	int Insert(int nIndex, LPCNTSTR pstr);
	// delete nCount characters starting at zero-based index
	int Delete(int nIndex, int nCount = 1);

	// searching

	// find character starting at left, -1 if not found
	int Find(NTCHAR ch) const;
	// find character starting at right
	int ReverseFind(NTCHAR ch) const;
	// find character starting at zero-based index and going right
	int Find(NTCHAR ch, int nStart) const;
	// find first instance of any character in passed string
	int FindOneOf(LPCNTSTR lpszCharSet) const;
	// find first instance of substring
	int Find(LPCNTSTR lpszSub) const;
	// find first instance of substring starting at zero-based index
	int Find(LPCNTSTR lpszSub, int nStart) const;

	// simple formatting

	// printf-like formatting using passed string
	void AFX_CDECL Format(LPCNTSTR lpszFormat, ...);
	// printf-like formatting using referenced string resource
	void AFX_CDECL Format(UINT nFormatID, ...);
	// printf-like formatting using variable arguments parameter
	void FormatV(LPCNTSTR lpszFormat, va_list argList);

	// formatting for localization (uses FormatMessage API)

	// format using FormatMessage API on passed string
	void AFX_CDECL FormatMessage(LPCNTSTR lpszFormat, ...);
	// format using FormatMessage API on referenced string resource
	void AFX_CDECL FormatMessage(UINT nFormatID, ...);

	// input and output
#ifdef _DEBUG
	friend CDumpContext& AFXAPI operator<<(CDumpContext& dc,
											const CStringN& string);
#endif
	friend CArchive& AFXAPI operator<<(CArchive& ar, const CStringN& string);
	friend CArchive& AFXAPI operator>>(CArchive& ar, CStringN& string);

	// load from string resource
	BOOL LoadString(UINT nID);

#ifdef _UNICODE
	// ANSI <-> OEM support (convert string in place)

	// convert string from ANSI to OEM in-place
	void AnsiToOem();
	// convert string from OEM to ANSI in-place
	void OemToAnsi();
#endif

#ifndef _AFX_NO_BSTR_SUPPORT
	// OLE BSTR support (use for OLE automation)

	// return a BSTR initialized with this CStringN's data
	BSTR AllocSysString() const;
	// reallocates the passed BSTR, copies content of this CStringN to it
	BSTR SetSysString(BSTR* pbstr) const;
#endif

	// Access to string implementation buffer as "C" character array

	// get pointer to modifiable buffer at least as long as nMinBufLength
	LPNTSTR GetBuffer(int nMinBufLength);
	// release buffer, setting length to nNewLength (or to first nul if -1)
	void ReleaseBuffer(int nNewLength = -1);
	// get pointer to modifiable buffer exactly as long as nNewLength
	LPNTSTR GetBufferSetLength(int nNewLength);
	// release memory allocated to but unused by string
	void FreeExtra();

	// Use LockBuffer/UnlockBuffer to turn refcounting off

	// turn refcounting back on
	LPNTSTR LockBuffer();
	// turn refcounting off
	void UnlockBuffer();

// Implementation
public:
	~CStringN();
	int GetAllocLength() const;

protected:
	LPNTSTR m_pchData;   // pointer to ref counted string data

	// implementation helpers
	CStringDataN* GetData() const;
	void Init();
	void AllocCopy(CStringN& dest, int nCopyLen, int nCopyIndex, int nExtraLen) const;
	void AllocBuffer(int nLen);
	void AssignCopy(int nSrcLen, LPCNTSTR lpszSrcData);
	void ConcatCopy(int nSrc1Len, LPCNTSTR lpszSrc1Data, int nSrc2Len, LPCNTSTR lpszSrc2Data);
	void ConcatInPlace(int nSrcLen, LPCNTSTR lpszSrcData);
	void CopyBeforeWrite();
	void AllocBeforeWrite(int nLen);
	void Release();
	static void PASCAL Release(CStringDataN* pData);
	static int PASCAL SafeStrlen(LPCNTSTR lpsz);
	static void FASTCALL FreeData(CStringDataN* pData);
};

// Compare helpers
bool AFXAPI operator==(const CStringN& s1, const CStringN& s2);
bool AFXAPI operator==(const CStringN& s1, LPCNTSTR s2);
bool AFXAPI operator==(LPCNTSTR s1, const CStringN& s2);
bool AFXAPI operator!=(const CStringN& s1, const CStringN& s2);
bool AFXAPI operator!=(const CStringN& s1, LPCNTSTR s2);
bool AFXAPI operator!=(LPCNTSTR s1, const CStringN& s2);
bool AFXAPI operator<(const CStringN& s1, const CStringN& s2);
bool AFXAPI operator<(const CStringN& s1, LPCNTSTR s2);
bool AFXAPI operator<(LPCNTSTR s1, const CStringN& s2);
bool AFXAPI operator>(const CStringN& s1, const CStringN& s2);
bool AFXAPI operator>(const CStringN& s1, LPCNTSTR s2);
bool AFXAPI operator>(LPCNTSTR s1, const CStringN& s2);
bool AFXAPI operator<=(const CStringN& s1, const CStringN& s2);
bool AFXAPI operator<=(const CStringN& s1, LPCNTSTR s2);
bool AFXAPI operator<=(LPCNTSTR s1, const CStringN& s2);
bool AFXAPI operator>=(const CStringN& s1, const CStringN& s2);
bool AFXAPI operator>=(const CStringN& s1, LPCNTSTR s2);
bool AFXAPI operator>=(LPCNTSTR s1, const CStringN& s2);

#ifdef _AFXDLL
const CStringN& AFXAPI AfxGetEmptyString();
#define afxEmptyStringN AfxGetEmptyString()
#else
extern LPCNTSTR _afxPchNilN;
#define afxEmptyStringN ((CStringN&)*(CStringN*)&_afxPchNilN)
#endif

#ifdef _AFX_INLINE

_AFX_INLINE CStringDataN* CStringN::GetData() const
{ ASSERT(m_pchData != NULL); return ((CStringDataN*)m_pchData)-1; }
_AFX_INLINE void CStringN::Init()
{ m_pchData = afxEmptyStringN.m_pchData; }
#ifndef _AFXDLL
_AFX_INLINE CStringN::CStringN()
{ m_pchData = afxEmptyStringN.m_pchData; }
#endif
_AFX_INLINE CStringN::CStringN(const unsigned char* lpsz)
{ Init(); *this = (LPCSTR)lpsz; }
_AFX_INLINE const CStringN& CStringN::operator=(const unsigned char* lpsz)
{ *this = (LPCSTR)lpsz; return *this; }
#ifndef _UNICODE
_AFX_INLINE const CStringN& CStringN::operator+=(char ch)
{ *this += (NTCHAR)ch; return *this; }
_AFX_INLINE const CStringN& CStringN::operator=(char ch)
{ *this = (NTCHAR)ch; return *this; }
_AFX_INLINE CStringN AFXAPI operator+(const CStringN& string, char ch)
{ return string + (NTCHAR)ch; }
_AFX_INLINE CStringN AFXAPI operator+(char ch, const CStringN& string)
{ return (NTCHAR)ch + string; }
#endif

_AFX_INLINE int CStringN::GetLength() const
{ return GetData()->nDataLength; }
_AFX_INLINE int CStringN::GetAllocLength() const
{ return GetData()->nAllocLength; }
_AFX_INLINE BOOL CStringN::IsEmpty() const
{ return GetData()->nDataLength == 0; }
_AFX_INLINE CStringN::operator LPCNTSTR() const
{ return m_pchData; }
_AFX_INLINE int PASCAL CStringN::SafeStrlen(LPCNTSTR lpsz)
{ return (lpsz == NULL) ? 0 : lstrlenN(lpsz); }

// CStringN support (windows specific)
_AFX_INLINE int CStringN::Compare(LPCNTSTR lpsz) const
{ ASSERT(AfxIsValidString(lpsz)); return _tcscmpN(m_pchData, lpsz); }    // MBCS/Unicode aware
_AFX_INLINE int CStringN::CompareNoCase(LPCNTSTR lpsz) const
{ ASSERT(AfxIsValidString(lpsz)); return _tcsicmpN(m_pchData, lpsz); }   // MBCS/Unicode aware
// CStringN::Collate is often slower than Compare but is MBSC/Unicode
//  aware as well as locale-sensitive with respect to sort order.
_AFX_INLINE int CStringN::Collate(LPCNTSTR lpsz) const
{ ASSERT(AfxIsValidString(lpsz)); return _tcscollN(m_pchData, lpsz); }   // locale sensitive
_AFX_INLINE int CStringN::CollateNoCase(LPCNTSTR lpsz) const
{ ASSERT(AfxIsValidString(lpsz)); return _tcsicollN(m_pchData, lpsz); }   // locale sensitive

_AFX_INLINE NTCHAR CStringN::GetAt(int nIndex) const
{
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}
_AFX_INLINE NTCHAR CStringN::operator[](int nIndex) const
{
	// same as GetAt
	ASSERT(nIndex >= 0);
	ASSERT(nIndex < GetData()->nDataLength);
	return m_pchData[nIndex];
}
_AFX_INLINE bool AFXAPI operator==(const CStringN& s1, const CStringN& s2)
{ return s1.Compare(s2) == 0; }
_AFX_INLINE bool AFXAPI operator==(const CStringN& s1, LPCNTSTR s2)
{ return s1.Compare(s2) == 0; }
_AFX_INLINE bool AFXAPI operator==(LPCNTSTR s1, const CStringN& s2)
{ return s2.Compare(s1) == 0; }
_AFX_INLINE bool AFXAPI operator!=(const CStringN& s1, const CStringN& s2)
{ return s1.Compare(s2) != 0; }
_AFX_INLINE bool AFXAPI operator!=(const CStringN& s1, LPCNTSTR s2)
{ return s1.Compare(s2) != 0; }
_AFX_INLINE bool AFXAPI operator!=(LPCNTSTR s1, const CStringN& s2)
{ return s2.Compare(s1) != 0; }
_AFX_INLINE bool AFXAPI operator<(const CStringN& s1, const CStringN& s2)
{ return s1.Compare(s2) < 0; }
_AFX_INLINE bool AFXAPI operator<(const CStringN& s1, LPCNTSTR s2)
{ return s1.Compare(s2) < 0; }
_AFX_INLINE bool AFXAPI operator<(LPCNTSTR s1, const CStringN& s2)
{ return s2.Compare(s1) > 0; }
_AFX_INLINE bool AFXAPI operator>(const CStringN& s1, const CStringN& s2)
{ return s1.Compare(s2) > 0; }
_AFX_INLINE bool AFXAPI operator>(const CStringN& s1, LPCNTSTR s2)
{ return s1.Compare(s2) > 0; }
_AFX_INLINE bool AFXAPI operator>(LPCNTSTR s1, const CStringN& s2)
{ return s2.Compare(s1) < 0; }
_AFX_INLINE bool AFXAPI operator<=(const CStringN& s1, const CStringN& s2)
{ return s1.Compare(s2) <= 0; }
_AFX_INLINE bool AFXAPI operator<=(const CStringN& s1, LPCNTSTR s2)
{ return s1.Compare(s2) <= 0; }
_AFX_INLINE bool AFXAPI operator<=(LPCNTSTR s1, const CStringN& s2)
{ return s2.Compare(s1) >= 0; }
_AFX_INLINE bool AFXAPI operator>=(const CStringN& s1, const CStringN& s2)
{ return s1.Compare(s2) >= 0; }
_AFX_INLINE bool AFXAPI operator>=(const CStringN& s1, LPCNTSTR s2)
{ return s1.Compare(s2) >= 0; }
_AFX_INLINE bool AFXAPI operator>=(LPCNTSTR s1, const CStringN& s2)
{ return s2.Compare(s1) <= 0; }
#endif // #ifdef _AFX_INLINE

#endif
