#pragma once

struct LocaleParameters
{
	TCHAR m_TimeSeparator;
	TCHAR m_DecimalPoint;
	TCHAR m_ThousandSeparator;

	LocaleParameters()
		: m_TimeSeparator(':'),
		m_DecimalPoint('.'),
		m_ThousandSeparator(',')
	{
	}
	void LoadLocaleParameters()
	{
		TCHAR TimeSeparator[] = _T(": ");
		TCHAR DecimalPoint[] = _T(". ");
		TCHAR ThousandSeparator[] = _T(", ");
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, DecimalPoint, 2);
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STIME, TimeSeparator, 2);
		GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, ThousandSeparator, 2);
		m_TimeSeparator = TimeSeparator[0];
		m_DecimalPoint = DecimalPoint[0];
		m_ThousandSeparator = ThousandSeparator[0];
	}
};

CString LtoaCS(long num);
