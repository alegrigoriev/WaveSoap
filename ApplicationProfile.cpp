#include <stdafx.h>
#include "ApplicationProfile.h"
#include <float.h>

CApplicationProfileItem::CApplicationProfileItem(LPCTSTR szSection, LPCTSTR szName)
	: Section(szSection), Name(szName)
{
}

CApplicationProfileItemStr::CApplicationProfileItemStr(LPCTSTR szSection, LPCTSTR szName,
														CString& StringReference, LPCTSTR Default)
	: CApplicationProfileItem(szSection, szName), StrRef(StringReference), m_Default(Default)
{
	// read value
	ReadData();
	InitialData = StrRef;
}

void CApplicationProfileItemStr::ReadData()
{
	StrRef = AfxGetApp()->GetProfileString(Section, Name, m_Default);
}

void CApplicationProfileItemStr::WriteData(BOOL bForceWrite)
{
	if (bForceWrite || StrRef != InitialData)
	{
		AfxGetApp()->WriteProfileString(Section, Name, StrRef);
	}
}

void CApplicationProfileItemStr::ResetToInitial()
{
	StrRef = InitialData;
}

void CApplicationProfileItemStr::ResetToDefault()
{
	StrRef = m_Default;
}

CApplicationProfileItemLong::CApplicationProfileItemLong(LPCTSTR szSection, LPCTSTR szName,
														LONG& LongReference,
														LONG Default, LONG MinVal, LONG MaxVal)
	: CApplicationProfileItem(szSection, szName),
	LongRef(LongReference), m_Default(Default), m_MinVal(MinVal), m_MaxVal(MaxVal)
{
	// read value
	ReadData();
	InitialData = LongRef;
}

void CApplicationProfileItemLong::ReadData()
{
	LongRef = AfxGetApp()->GetProfileInt(Section, Name, m_Default);
}

void CApplicationProfileItemLong::WriteData(BOOL bForceWrite)
{
	if (bForceWrite || LongRef != InitialData)
	{
		AfxGetApp()->WriteProfileInt(Section, Name, LongRef);
	}
}

void CApplicationProfileItemLong::ResetToInitial()
{
	LongRef = InitialData;
}

void CApplicationProfileItemLong::ResetToDefault()
{
	LongRef = m_Default;
}

CApplicationProfileItemInt::CApplicationProfileItemInt(LPCTSTR szSection, LPCTSTR szName,
														int & Reference,
														int Default, int MinVal, int MaxVal)
	: CApplicationProfileItem(szSection, szName),
	Ref(Reference), m_Default(Default), m_MinVal(MinVal), m_MaxVal(MaxVal)
{
	// read value
	ReadData();
	InitialData = Ref;
}

void CApplicationProfileItemInt::ReadData()
{
	Ref = AfxGetApp()->GetProfileInt(Section, Name, m_Default);
}

void CApplicationProfileItemInt::WriteData(BOOL bForceWrite)
{
	if (bForceWrite || Ref != InitialData)
	{
		AfxGetApp()->WriteProfileInt(Section, Name, Ref);
	}
}

void CApplicationProfileItemInt::ResetToInitial()
{
	Ref = InitialData;
}

void CApplicationProfileItemInt::ResetToDefault()
{
	Ref = m_Default;
}

CApplicationProfileItemUlong::CApplicationProfileItemUlong(LPCTSTR szSection, LPCTSTR szName,
															ULONG& LongReference,
															ULONG Default, ULONG MinVal, ULONG MaxVal)
	: CApplicationProfileItem(szSection, szName),
	LongRef(LongReference), m_Default(Default), m_MinVal(MinVal), m_MaxVal(MaxVal)
{
	// read value
	ReadData();
	InitialData = LongRef;
}

void CApplicationProfileItemUlong::ReadData()
{
	LongRef = AfxGetApp()->GetProfileInt(Section, Name, m_Default);
}

void CApplicationProfileItemUlong::WriteData(BOOL bForceWrite)
{
	if (bForceWrite || LongRef != InitialData)
	{
		AfxGetApp()->WriteProfileInt(Section, Name, LongRef);
	}
}

void CApplicationProfileItemUlong::ResetToInitial()
{
	LongRef = InitialData;
}

void CApplicationProfileItemUlong::ResetToDefault()
{
	LongRef = m_Default;
}

CApplicationProfileItemDouble::CApplicationProfileItemDouble(LPCTSTR szSection, LPCTSTR szName,
															double& DoubleReference,
															double Default, double MinVal, double MaxVal)
	: CApplicationProfileItem(szSection, szName),
	DoubleRef(DoubleReference), m_Default(Default), m_MinVal(MinVal), m_MaxVal(MaxVal)
{
	// read value
	ReadData();
	InitialData = DoubleRef;
}

void CApplicationProfileItemDouble::ReadData()
{
	CString s = AfxGetApp()->GetProfileString(Section, Name, "");
	double val;
	TCHAR * endptr;

	if (s.IsEmpty()
		|| (val = _tcstod(s, & endptr), 0 != *endptr)
		|| _isnan(val)
		|| ! _finite(val))
	{
		val = m_Default;
	}

	if (m_MinVal != m_MaxVal)
	{
		if (val < m_MinVal) val = m_MinVal;
		if (val > m_MaxVal) val = m_MaxVal;
	}
	DoubleRef = val;
}

void CApplicationProfileItemDouble::WriteData(BOOL bForceWrite)
{
	if (bForceWrite || DoubleRef != InitialData)
	{
		CString s;
		s.Format("%g", DoubleRef);
		AfxGetApp()->WriteProfileString(Section, Name, s);
	}
}

void CApplicationProfileItemDouble::ResetToInitial()
{
	DoubleRef = InitialData;
}

void CApplicationProfileItemDouble::ResetToDefault()
{
	DoubleRef = m_Default;
}

CApplicationProfileItemFloat::CApplicationProfileItemFloat(LPCTSTR szSection, LPCTSTR szName,
															float& FloatReference,
															double Default, double MinVal, double MaxVal)
	: CApplicationProfileItemDouble(szSection, szName, IntermediateValue,
									Default, MinVal, MaxVal),
	FloatRef(FloatReference)
{
	// read value
	ReadData();
	FloatReference = IntermediateValue;
}

void CApplicationProfileItemFloat::ReadData()
{
	CApplicationProfileItemDouble::ReadData();
	FloatRef = IntermediateValue;
}

void CApplicationProfileItemFloat::WriteData(BOOL bForceWrite)
{
	IntermediateValue = FloatRef;
	CApplicationProfileItemDouble::WriteData(bForceWrite);
}

void CApplicationProfileItemFloat::ResetToInitial()
{
	FloatRef = InitialData;
}

void CApplicationProfileItemFloat::ResetToDefault()
{
	CApplicationProfileItemDouble::ResetToDefault();
	FloatRef = IntermediateValue;
}

CApplicationProfile::CApplicationProfile()
	:pItems(NULL)
{
}

CApplicationProfile::~CApplicationProfile()
{
	CApplicationProfileItem * pTmp;
	while (pItems != NULL)
	{
		pTmp = pItems;
		pItems = pTmp->Next;
		delete pTmp;
	}
}

BOOL CApplicationProfile::AddItem(LPCTSTR szSection, LPCTSTR szName, CString & str,
								LPCTSTR szDefault,int MaxLen)
{
	CApplicationProfileItem * pTmp;
	RemoveItem(szSection, szName);
	pTmp = new CApplicationProfileItemStr(szSection, szName, str, szDefault);
	pTmp->Next = pItems;
	pItems = pTmp;
	return TRUE;
}

BOOL CApplicationProfile::AddItem(LPCTSTR szSection, LPCTSTR szName, LONG & val,
								LONG nDefault, LONG nMin, LONG nMax)
{
	CApplicationProfileItem * pTmp;
	RemoveItem(szSection, szName);
	pTmp = new CApplicationProfileItemLong(szSection, szName, val, nDefault, nMin, nMax);
	pTmp->Next = pItems;
	pItems = pTmp;
	return TRUE;
}

BOOL CApplicationProfile::AddItem(LPCTSTR szSection, LPCTSTR szName, int & val,
								int nDefault, int nMin, int nMax)
{
	CApplicationProfileItem * pTmp;
	RemoveItem(szSection, szName);
	pTmp = new CApplicationProfileItemInt(szSection, szName, val, nDefault, nMin, nMax);
	pTmp->Next = pItems;
	pItems = pTmp;
	return TRUE;
}

BOOL CApplicationProfile::AddItem(LPCTSTR szSection, LPCTSTR szName, ULONG & val,
								ULONG nDefault, ULONG nMin, ULONG nMax)
{
	CApplicationProfileItem * pTmp;
	RemoveItem(szSection, szName);
	pTmp = new CApplicationProfileItemUlong(szSection, szName, val, nDefault, nMin, nMax);
	pTmp->Next = pItems;
	pItems = pTmp;
	return TRUE;
}

BOOL CApplicationProfile::AddBoolItem(LPCTSTR szSection, LPCTSTR szName, int & val,
									int nDefault)
{
	CApplicationProfileItem * pTmp;
	RemoveItem(szSection, szName);
	pTmp = new CApplicationProfileItemInt(szSection, szName, val, nDefault, 0, 1);
	pTmp->Next = pItems;
	pItems = pTmp;
	return TRUE;
}

BOOL CApplicationProfile::AddItem(LPCTSTR szSection, LPCTSTR szName, double & val,
								double nDefault, double nMin, double nMax)
{
	CApplicationProfileItem * pTmp;
	RemoveItem(szSection, szName);
	pTmp = new CApplicationProfileItemDouble(szSection, szName, val, nDefault, nMin, nMax);
	pTmp->Next = pItems;
	pItems = pTmp;
	return TRUE;
}

BOOL CApplicationProfile::AddItem(LPCTSTR szSection, LPCTSTR szName, float & val,
								double nDefault, double nMin, double nMax)
{
	CApplicationProfileItem * pTmp;
	RemoveItem(szSection, szName);
	pTmp = new CApplicationProfileItemFloat(szSection, szName, val, nDefault, nMin, nMax);
	pTmp->Next = pItems;
	pItems = pTmp;
	return TRUE;
}

BOOL CApplicationProfile::RemoveItem(LPCTSTR szSection, LPCTSTR szName)
{
	CApplicationProfileItem * pTmp = pItems;
	CApplicationProfileItem * pPrev = NULL;
	while(pTmp != NULL)
	{
		if (0 == pTmp->Name.CompareNoCase(szName)
			&& 0 == pTmp->Section.CompareNoCase(szSection))
		{
			if (pPrev)
			{
				pPrev->Next = pTmp->Next;
			}
			else
			{
				pItems = pTmp->Next;
			}
			delete pTmp;
			return TRUE;
		}
		pPrev = pTmp;
		pTmp = pTmp->Next;
	}
	return FALSE;
}

BOOL CApplicationProfile::RemoveSection(LPCTSTR szSection)
{
	CApplicationProfileItem * pTmp = pItems;
	CApplicationProfileItem * pPrev = NULL;
	while(pTmp != NULL)
	{
		if (0 == pTmp->Section.CompareNoCase(szSection))
		{
			if (pPrev)
			{
				pPrev->Next = pTmp->Next;
				delete pTmp;
				pTmp = pPrev->Next;
			}
			else
			{
				pItems = pTmp->Next;
				delete pTmp;
				pTmp = pItems;
			}
			continue;
		}
		pPrev = pTmp;
		pTmp = pTmp->Next;
	}
	return TRUE;
}

BOOL CApplicationProfile::FlushItem(LPCTSTR szSection, LPCTSTR szName)
{
	CApplicationProfileItem * pTmp = pItems;
	while(pTmp != NULL)
	{
		if (0 == pTmp->Name.CompareNoCase(szName)
			&& 0 == pTmp->Section.CompareNoCase(szSection))
		{
			pTmp->WriteData();
			return TRUE;
		}
		pTmp = pTmp->Next;
	}
	return FALSE;
}

BOOL CApplicationProfile::FlushSection(LPCTSTR szSection)
{
	CApplicationProfileItem * pTmp = pItems;
	while(pTmp != NULL)
	{
		if (0 == pTmp->Section.CompareNoCase(szSection))
		{
			pTmp->WriteData();
		}
		pTmp = pTmp->Next;
	}
	return TRUE;
}

BOOL CApplicationProfile::UnloadItem(LPCTSTR szSection, LPCTSTR szName)
{
	return FlushItem(szSection, szName) && RemoveItem(szSection, szName);
}

BOOL CApplicationProfile::UnloadSection(LPCTSTR szSection)
{
	return FlushSection(szSection) && RemoveSection(szSection);
}

BOOL CApplicationProfile::ResetItemToDefault(LPCTSTR szSection, LPCTSTR szName)
{
	CApplicationProfileItem * pTmp = pItems;
	while(pTmp != NULL)
	{
		if (0 == pTmp->Name.CompareNoCase(szName)
			&& 0 == pTmp->Section.CompareNoCase(szSection))
		{
			pTmp->ResetToDefault();
			return TRUE;
		}
		pTmp = pTmp->Next;
	}
	return FALSE;
}

BOOL CApplicationProfile::ResetSectionToDefault(LPCTSTR szSection)
{
	CApplicationProfileItem * pTmp = pItems;
	while(pTmp != NULL)
	{
		if (0 == pTmp->Section.CompareNoCase(szSection))
		{
			pTmp->ResetToDefault();
		}
		pTmp = pTmp->Next;
	}
	return TRUE;
}

BOOL CApplicationProfile::RevertItemToInitial(LPCTSTR szSection, LPCTSTR szName)
{
	CApplicationProfileItem * pTmp = pItems;
	while(pTmp != NULL)
	{
		if (0 == pTmp->Name.CompareNoCase(szName)
			&& 0 == pTmp->Section.CompareNoCase(szSection))
		{
			pTmp->ResetToInitial();
			return TRUE;
		}
		pTmp = pTmp->Next;
	}
	return FALSE;
}

BOOL CApplicationProfile::RevertSectionToInitial(LPCTSTR szSection)
{
	CApplicationProfileItem * pTmp = pItems;
	while(pTmp != NULL)
	{
		if (0 == pTmp->Section.CompareNoCase(szSection))
		{
			pTmp->ResetToInitial();
		}
		pTmp = pTmp->Next;
	}
	return TRUE;
}
		// saves the section in INI file. If section name is empty,
		// saves all of them
BOOL CApplicationProfile::ExportSection(LPCTSTR szSection, LPCTSTR szFilename)
{
	CWinApp * pApp = AfxGetApp();
	if (NULL == szFilename || 0 == szFilename[0])
	{
		return FALSE;
	}
	LPCTSTR OldRegistryKeyName = pApp->m_pszRegistryKey;
	LPCTSTR OldProfileName = pApp->m_pszProfileName;
	pApp->m_pszRegistryKey = NULL;
	pApp->m_pszProfileName = szFilename;
	try
	{
		CApplicationProfileItem * pTmp = pItems;
		while(pTmp != NULL)
		{
			if (NULL == szSection
				|| 0 == szSection[0]
				|| 0 == pTmp->Section.CompareNoCase(szSection))
			{
				pTmp->WriteData(TRUE);  // force to write
			}
			pTmp = pTmp->Next;
		}
	}
	catch(...)
	{
		pApp->m_pszProfileName = OldProfileName;
		pApp->m_pszRegistryKey = OldRegistryKeyName;
		throw;
	}
	pApp->m_pszProfileName = OldProfileName;
	pApp->m_pszRegistryKey = OldRegistryKeyName;
	return TRUE;
}

// restores the section from INI file. If section name is empty,
// restores all of them.
BOOL CApplicationProfile::ImportSection(LPCTSTR szSection, LPCTSTR szFilename)
{
	CWinApp * pApp = AfxGetApp();
	if (NULL == szFilename || 0 == szFilename[0])
	{
		return FALSE;
	}
	LPCTSTR OldRegistryKeyName = pApp->m_pszRegistryKey;
	LPCTSTR OldProfileName = pApp->m_pszProfileName;
	pApp->m_pszRegistryKey = NULL;
	pApp->m_pszProfileName = szFilename;
	try
	{
		CApplicationProfileItem * pTmp = pItems;
		while(pTmp != NULL)
		{
			if (NULL == szSection
				|| 0 == szSection[0]
				|| 0 == pTmp->Section.CompareNoCase(szSection))
			{
				pTmp->ReadData();
			}
			pTmp = pTmp->Next;
		}
	}
	catch(...)
	{
		pApp->m_pszProfileName = OldProfileName;
		pApp->m_pszRegistryKey = OldRegistryKeyName;
		throw;
	}
	pApp->m_pszProfileName = OldProfileName;
	pApp->m_pszRegistryKey = OldRegistryKeyName;
	return TRUE;
}
