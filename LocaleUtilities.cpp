#include "StdAfx.h"
#include "LocaleUtilities.h"

// long to string, thousands separated by commas
CString LtoaCS(long num)
{
	TCHAR s[20];
	TCHAR s1[30];
	TCHAR * p = s;
	TCHAR * p1 = s1;
	TCHAR ThSep = GetApp()->m_ThousandSeparator;
	_ltot_s(num, s, 20, 10);
	if (0 == ThSep)
	{
		return s;
	}
	if ('-' == p[0])
	{
		p1[0] = '-';
		p++;
		p1++;
	}
	unsigned len = (unsigned)_tcslen(p);
	unsigned first = len % 3;

	if (0 == first && len > 0)
	{
		first = 3;
	}
	memcpy(p1, p, first*sizeof * p1);
	p1 += first;
	p += first;
	len -= first;
	while (p[0])
	{
		p1[0] = ThSep;
		p1[1] = p[0];
		p1[2] = p[1];
		p1[3] = p[2];
		p1 += 4;
		p += 3;
	}
	*p1 = 0;
	return CString(s1);
}

