#pragma once

class CoInitHelper
{
public:
	HRESULT InitializeCom(DWORD dwCoInit)
	{
		if (0 == m_InitCount)
		{
			TRACE("CoInitializeEx(%X) for thread %X\n", dwCoInit, GetCurrentThreadId());

			HRESULT hr = CoInitializeEx(NULL, dwCoInit);
			if (SUCCEEDED(hr))
			{
				m_InitCount = 1;
				m_Thread = GetCurrentThreadId();
				m_Mode = dwCoInit;
			}
			else
			{
				TRACE("CoInitializeEx failed, hr=%X\n", hr);
			}
			return hr;
		}
		else
		{
			ASSERT(0 == ((dwCoInit ^ m_Mode) & (COINIT_APARTMENTTHREADED | COINIT_MULTITHREADED)));
			ASSERT(GetCurrentThreadId() == m_Thread);
			m_InitCount++;
			return S_FALSE;
		}
	}

	void UninitializeCom()
	{
		ASSERT(GetCurrentThreadId() == m_Thread);
		ASSERT(m_InitCount > 0);
		if (0 == --m_InitCount)
		{
			TRACE("CoUninitializeEx for thread %X\n", GetCurrentThreadId());
			CoUninitialize();
			m_Thread = 0;
			m_Mode = 0;
		}
	}
	CoInitHelper()
		: m_InitCount(0),
		m_Mode(0), m_Thread(0)
	{
	}
	CoInitHelper(DWORD dwCoInit)
		: m_InitCount(0),
		m_Mode(0), m_Thread(0)
	{
		InitializeCom(dwCoInit);
	}
	~CoInitHelper()
	{
		while (m_InitCount > 0)
		{
			UninitializeCom();
		}
	}
private:
	LONG m_InitCount;
	DWORD m_Mode;
	DWORD m_Thread;
};
