#include "StdAfx.h"
#include "TimeToStr.h"

CString TimeToHhMmSs(unsigned TimeMs, int Flags)
{
	int hh = TimeMs / 3600000;
	TimeMs -= hh * 3600000;
	int mm = TimeMs / 60000;
	TimeMs -= mm * 60000;
	int ss = TimeMs / 1000;
	TimeMs -= ss * 1000;
	int ms = TimeMs;
	CString s;
	TCHAR TimeSeparator = GetApp()->m_TimeSeparator;
	TCHAR DecimalPoint = GetApp()->m_DecimalPoint;

	TCHAR StrMs[16];

	if (Flags & TimeToHhMmSs_Frames75)
	{
		_stprintf(StrMs, _T("%02df"), ms);
	}
	else
	{
		_stprintf(StrMs, _T("%03d"), ms);
	}

	if (Flags & (TimeToHhMmSs_NeedsMs | TimeToHhMmSs_Frames75))
	{
		if (hh != 0 || (Flags & TimeToHhMmSs_NeedsHhMm))
		{
			s.Format(_T("%d%c%02d%c%02d%c%s"),
					hh, TimeSeparator,
					mm, TimeSeparator,
					ss, DecimalPoint,
					StrMs);
		}
		else if (mm != 0)
		{
			s.Format(_T("%d%c%02d%c%s"),
					mm, TimeSeparator,
					ss, DecimalPoint,
					StrMs);
		}
		else
		{
			s.Format(_T("%d%c%s"),
					ss, DecimalPoint,
					StrMs);
		}
	}
	else
	{
		if (hh != 0
			|| 0 != (Flags & TimeToHhMmSs_NeedsHhMm))
		{
			s.Format(_T("%d%c%02d%c%02d"),
					hh, TimeSeparator,
					mm, TimeSeparator,
					ss);
		}
		else
		{
			s.Format(_T("%d%c%02d"),
					mm, TimeSeparator,
					ss);
		}
	}
	return s;
}

CString SampleToString(SAMPLE_INDEX Sample, int nSamplesPerSec, int Flags)
{
	switch (Flags & SampleToString_Mask)
	{
	case SampleToString_Sample:
		return LtoaCS(Sample);
		break;
	case SampleToString_Seconds:
	{
		CString s;
		unsigned ms = unsigned(Sample * 1000. / nSamplesPerSec);
		int sec = ms / 1000;
		ms = ms % 1000;
		TCHAR * pFormat = _T("%s%c0");
		if (Flags & TimeToHhMmSs_NeedsMs)
		{
			pFormat = _T("%s%c%03d");
		}
		s.Format(pFormat, LtoaCS(sec), GetApp()->m_DecimalPoint, ms);
		return s;
	}
		break;

	default:
	case SampleToString_HhMmSs:
		return TimeToHhMmSs(unsigned(Sample * 1000. / nSamplesPerSec), Flags);
		break;

	case SampleToString_HhMmSsFf:
	{
		unsigned Seconds = Sample / nSamplesPerSec;
		unsigned Frames = (Sample % nSamplesPerSec) / (nSamplesPerSec / 75);
		return TimeToHhMmSs(Seconds * 1000 + Frames, Flags | TimeToHhMmSs_Frames75);
	}
		break;
	}
}

