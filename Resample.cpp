// Resample.cpp
#include "stdafx.h"
#include "Resample.h"

#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.785398163397448309616
void CResampleContext::FilterSoundResample()
{
	int SrcSamples = m_SrcBufFilled - m_SrcBufUsed - m_SrcFilterLength;
	int Channels = m_DstFile.Channels();
	const float * src = m_pSrcBuf + m_SrcBufUsed;

	if (SrcSamples <= 0)
	{
		return;
	}

	for (int i = 0; i < DstBufSize; i+= Channels)
	{
		for (int ch = 0; ch < Channels; ch++)
		{
			unsigned __int32 Phase1 = m_Phase;
			double OutSample = 0.;
			for (int j = 0; ; j+= Channels)
			{
				int TableIndex = Phase1 >> 24;
				OutSample += src[j+ch] * (m_FilterBuf[TableIndex] +
										int(Phase1 & 0xFFFFFF) * m_FilterDifBuf[TableIndex]);
				unsigned __int32 Phase2 = Phase1 + m_InputPeriod;
				if (Phase2 < Phase1)
				{
					break;
				}
				Phase1 = Phase2;
			}
			m_pDstBuf[i+ch] = OutSample;
		}
		m_Phase -= m_OutputPeriod;
		while (m_Phase & 0x80000000)
		{
			src += Channels;
			m_Phase += m_InputPeriod;
			SrcSamples -= Channels;
			m_SrcBufUsed += Channels;
			if (SrcSamples < Channels)
			{
				return;
			}
		}
	}
}

BOOL CResampleContext::InitResample(CWaveFile & SrcFile, CWaveFile &DstFile,
									double FrequencyRatio, double FilterLength)
{
	m_SrcFile = SrcFile;
	m_DstFile = DstFile;
	// FrequencyRatio is out freq/ input freq. If >1, it is upsampling,
	// if < 1 it is downsampling
	// FilterLength is how many Sin periods are in the array
	double ResampleFilterSize = 40;
	double PrevVal = 0.;
	for (int i = 0; i < ResampleFilterSize; i++)
	{
		double arg = M_PI *2 * FilterLength / ResampleFilterSize * (i - ResampleFilterSize / 2);
		double Window = sin(M_PI * i / ResampleFilterSize);
		Window *= Window;   // squared sin window
		double val;
		if (arg != 0)
		{
			val = Window * sin(arg) / arg;
		}
		else
		{
			val = Window;   // window must be 1.
		}
		// val *= FilterScale;
		m_FilterBuf[i] = PrevVal;
		m_FilterDifBuf[i] = (val - PrevVal) / 0x1000000;
		PrevVal = val;
	}
	if (FrequencyRatio >= 1.)
	{
		// upsampling.
		//
		double InputPeriod = 0x100000000i64 / (FilterLength + 1. / FilterLength);
		m_InputPeriod = unsigned __int32(InputPeriod);
		m_OutputPeriod = unsigned __int32(InputPeriod / FrequencyRatio);
	}
	else
	{
		// downsampling
		double OutputPeriod = 0x100000000i64 / (FilterLength + 1. / FilterLength);
		m_OutputPeriod = unsigned __int32(OutputPeriod);
		m_InputPeriod = unsigned __int32(OutputPeriod * FrequencyRatio);
	}
	m_Phase = 0x80000000u % m_InputPeriod;

	m_pSrcBuf = new float[SrcBufSize];
	m_pDstBuf = new float[SrcBufSize];
	if (NULL == m_pSrcBuf || NULL == m_pDstBuf)
	{
		return FALSE;
	}
	memset(m_pSrcBuf, 0, SrcBufSize * sizeof * m_pSrcBuf);
	// prefill at 1/2 filter length
	m_SrcBufUsed = (0x80000000u / m_InputPeriod) * SrcFile.Channels();
	m_DstBufUsed = 0;
	return TRUE;
}

BOOL CResampleContext::OperationProc()
{
	DWORD dwOperationBegin = m_DstCopyPos;
	DWORD dwStartTime = GetTickCount();
	if (m_DstCopyPos >= m_DstEnd)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}


	DWORD LeftToRead = 0;
	DWORD LeftToWrite = 0;
	DWORD WasRead = 0;
	DWORD WasLockedToWrite = 0;
	void * pOriginalSrcBuf;
	char * pSrcBuf;
	void * pOriginalDstBuf;
	char * pDstBuf;
	do
	{
		if ((m_SrcBufFilled - m_SrcBufUsed) / m_SrcFile.Channels()
			<= m_SrcFilterLength)
		{
			for (int i = 0, j = m_SrcBufUsed; j < m_SrcBufFilled; i++, j++)
			{
				m_pSrcBuf[i] = m_pSrcBuf[j];
			}
			m_SrcBufUsed = 0;
			m_SrcBufFilled = i;
		}
		while (m_SrcBufFilled < SrcBufSize)
		{
			if (0 == LeftToRead)
			{
				DWORD SizeToRead = m_SrcEnd - m_SrcCopyPos;
				if (0 == SizeToRead)
				{
					break;
				}
				if (SizeToRead > 0x10000)
				{
					SizeToRead = 0x10000;
				}
				WasRead = m_SrcFile.m_File.GetDataBuffer( & pOriginalSrcBuf,
														SizeToRead, m_SrcCopyPos, CDirectFile::GetBufferAndPrefetchNext);
				if (0 == WasRead)
				{
					if (0 != WasLockedToWrite)
					{
						m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
														CDirectFile::ReturnBufferDirty);
					}
					return FALSE;
				}
				pSrcBuf = (char *) pOriginalSrcBuf;
				LeftToRead = WasRead;
			}

			int ToCopy = __min(SrcBufSize - m_SrcBufFilled, LeftToRead / sizeof *pSrcBuf);
			for (int i = 0, j = m_SrcBufFilled; i < ToCopy; i++, j++)
			{
				m_pSrcBuf[j] = pSrcBuf[i];
			}
			m_SrcBufFilled = j;
			pSrcBuf += i;
			LeftToRead -= i * sizeof *pSrcBuf;
			m_SrcCopyPos += i * sizeof *pSrcBuf;

			ASSERT(pSrcBuf + LeftToRead == WasRead + (char*)pOriginalSrcBuf);
			if (0 == LeftToRead)
			{
				m_SrcFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
												CDirectFile::ReturnBufferDiscard);
				WasRead = 0;
			}
		}

		FilterSoundResample();
		int DstBufSaved = 0;
		while (DstBufSaved < m_DstBufUsed)
		{
			if (0 == LeftToWrite)
			{
				DWORD SizeToWrite = m_DstEnd - m_DstCopyPos;
				WasLockedToWrite = m_DstFile.m_File.GetDataBuffer( & pOriginalDstBuf,
										SizeToWrite, m_DstCopyPos, CDirectFile::GetBufferWriteOnly);

				if (0 == WasLockedToWrite)
				{
					if (0 != WasRead)
					{
						m_SrcFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
														CDirectFile::ReturnBufferDiscard);
					}
					return FALSE;
				}
				pDstBuf = (char *) pOriginalDstBuf;
				LeftToWrite = WasLockedToWrite;

			}

			int ToCopy = __min(m_DstBufUsed - DstBufSaved, LeftToWrite / sizeof *pDstBuf);
			for (int i = 0, j = DstBufSaved; i < ToCopy; i++, j++)
			{
				pDstBuf[i] = m_pDstBuf[j];
			}
			DstBufSaved = j;
			pDstBuf += i;
			LeftToWrite -= i * sizeof *pDstBuf;
			m_DstCopyPos += i * sizeof *pDstBuf;

			ASSERT(pDstBuf + LeftToWrite == WasLockedToWrite + (char*)pOriginalDstBuf);
			if (0 == LeftToWrite)
			{
				m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
												CDirectFile::ReturnBufferDirty);
				WasLockedToWrite = 0;
			}
		}
		m_DstBufUsed = 0;

	}
	while (m_DstCopyPos < m_DstEnd
			&& timeGetTime() - dwStartTime < 200
			);
	if (0 != WasRead)
	{
		m_SrcFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
										CDirectFile::ReturnBufferDiscard);
	}
	if (0 != WasLockedToWrite)
	{
		m_DstFile.m_File.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
										CDirectFile::ReturnBufferDirty);
	}

	if (m_DstEnd > m_DstStart)
	{
		PercentCompleted = 100i64 * (m_DstCopyPos - m_DstStart) / (m_DstEnd - m_DstStart);
	}
	return TRUE;
}
