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
