#pragma once

class CGdiObjectSave
{
public:

	CGdiObjectSave(CDC * pDC, CGdiObject * pObjectToSave)
		: m_pDC(pDC), m_pSavedObject(pObjectToSave)
	{
	}

	void operator=(CGdiObject * pObj)
	{
		ASSERT(NULL == m_pSavedObject);
		m_pSavedObject = pObj;
	}

	~CGdiObjectSave()
	{
		if (NULL != m_pSavedObject)
		{
			m_pDC->SelectObject(m_pSavedObject);
		}
	}
private:
	CDC * const m_pDC;
	CGdiObject * m_pSavedObject;

	CGdiObjectSave(CGdiObjectSave const &);
	CGdiObjectSave & operator =(CGdiObjectSave const &);
};
