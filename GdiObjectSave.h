// GdiObjectSave.h
#pragma once

template<typename T=CGdiObject>
class CGdiObjectSaveT
{
public:

	CGdiObjectSaveT(CDC * pDC, T * pObjectToSave)
		: m_pDC(pDC), m_pSavedObject(pObjectToSave)
	{
	}

	CGdiObjectSaveT(CDC & DC, T * pObjectToSave)
		: m_pDC( & DC), m_pSavedObject(pObjectToSave)
	{
	}

	void operator=(T * pObj)
	{
		ASSERT(NULL == m_pSavedObject);
		m_pSavedObject = pObj;
	}
	operator T * () const
	{
		return m_pSavedObject;
	}
	~CGdiObjectSaveT()
	{
		if (NULL != m_pSavedObject)
		{
			m_pDC->SelectObject(m_pSavedObject);
		}
	}
private:
	CDC * const m_pDC;
	T * m_pSavedObject;

	CGdiObjectSaveT(CGdiObjectSaveT const &);
	CGdiObjectSaveT & operator =(CGdiObjectSaveT const &);
};

typedef CGdiObjectSaveT<> CGdiObjectSave;

class CPushDcPalette
{
public:
	CPushDcPalette(CDC * pDC, CPalette * pPalette, BOOL bForceBackground = FALSE)
		: m_pDC(pDC), m_pSavedPalette(NULL)
	{
		if (NULL != pPalette)
		{
			m_pSavedPalette = pDC->SelectPalette(pPalette, bForceBackground);
		}
	}

	CPushDcPalette(CDC & DC, CPalette * pPalette, BOOL bForceBackground = FALSE)
		: m_pDC( & DC), m_pSavedPalette(NULL)
	{
		if (NULL != pPalette)
		{
			m_pSavedPalette = DC.SelectPalette(pPalette, bForceBackground);
		}
	}

	void PushPalette(CPalette * pPalette, BOOL bForceBackground = FALSE)
	{
		m_pSavedPalette = m_pDC->SelectPalette(pPalette, bForceBackground);
	}

	~CPushDcPalette()
	{
		if (NULL != m_pSavedPalette)
		{
			m_pDC->SelectPalette(m_pSavedPalette, FALSE);
		}
	}
private:
	CDC * const m_pDC;
	CPalette * m_pSavedPalette;
};
