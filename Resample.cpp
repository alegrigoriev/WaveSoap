// Resample.cpp
#include "stdafx.h"
#include "Resample.h"

#define M_PI        3.14159265358979323846
#define M_PI_2      1.57079632679489661923
#define M_PI_4      0.785398163397448309616
void CResampleContext::FilterSoundResample()
{
	int Channels = m_DstFile.Channels();
	int SrcSamples = m_SrcBufFilled - m_SrcBufUsed - Channels * m_SrcFilterLength;
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
				int TableIndex = Phase1 >> ResampleIndexShift;
				double PhaseFraction = int(Phase1 & ~(0xFFFFFFFF << ResampleIndexShift));
				ASSERT(src + j + ch < m_pSrcBuf + SrcBufSize);
				OutSample += src[j+ch] * (m_FilterBuf[TableIndex] +
										PhaseFraction * (m_FilterDifBuf[TableIndex]
											+ PhaseFraction * m_FilterDif2Buf[TableIndex]));
				unsigned __int32 Phase2 = Phase1 + m_InputPeriod;
				if (Phase2 < Phase1)
				{
					break;
				}
				Phase1 = Phase2;
			}
			m_pDstBuf[i+ch] = OutSample;
		}
		m_DstBufUsed += Channels;
		m_Phase -= m_OutputPeriod;
		while (m_Phase & 0x80000000)
		{
			src += Channels;
			m_Phase += m_InputPeriod;
			SrcSamples -= Channels;
			m_SrcBufUsed += Channels;
		}
		if (SrcSamples < Channels)
		{
			return;
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
	double PrevVal = 0.;
	for (int i = 0; i < ResampleFilterSize; i++)
	{
		double arg = M_PI * FilterLength / ResampleFilterSize * (i + 1 - ResampleFilterSize / 2);
		double Window = sin(M_PI * (i +1) / ResampleFilterSize);
		Window *= Window;   // squared sin window

		double arg1 = M_PI * FilterLength / ResampleFilterSize * (i + 0.5 - ResampleFilterSize / 2);
		double Window1 = sin(M_PI * (i +0.5) / ResampleFilterSize);
		Window1 *= Window1;   // squared sin window
		double val;
		if (arg != 0)
		{
			val = Window * sin(arg) / arg;
		}
		else
		{
			val = Window;   // window must be 1.
		}
		double val1 = Window1 * sin(arg1) / arg1;
		// val *= FilterScale;
		m_FilterBuf[i] = PrevVal;
		//TRACE("Resample filter[%03d]=%f\n", i, m_FilterBuf[i]);
		double dif1 = val1 - PrevVal;
		double dif2 = val - val1;
		double sqrdif = (dif2 - dif1) * 4. / 3.;
		m_FilterDifBuf[i] = (val - PrevVal - sqrdif) / (1 << ResampleIndexShift);
		m_FilterDif2Buf[i] = sqrdif / (1 << ResampleIndexShift) / (1 << ResampleIndexShift);
		PrevVal = val;
	}
	if (FrequencyRatio >= 1.)
	{
		// upsampling.
		//
		double InputPeriod = 0x100000000i64 / (FilterLength * (1. + 1. / FilterLength));
		m_InputPeriod = unsigned __int32(InputPeriod);
		m_OutputPeriod = unsigned __int32(InputPeriod / FrequencyRatio);
	}
	else
	{
		// downsampling
		double OutputPeriod = 0x100000000i64 / (FilterLength * (1. + 1. / FilterLength));
		m_OutputPeriod = unsigned __int32(OutputPeriod);
		m_InputPeriod = unsigned __int32(OutputPeriod * FrequencyRatio);
	}
	//TRACE("InputPeriod=%08x, OutputPeriod=%08x\n", m_InputPeriod, m_OutputPeriod);

	m_SrcFilterLength = 0x100000000i64 / m_InputPeriod;
	m_Phase = 0x80000000u % m_InputPeriod;

	m_pSrcBuf = new float[SrcBufSize];
	m_pDstBuf = new float[DstBufSize];
	if (NULL == m_pSrcBuf || NULL == m_pDstBuf)
	{
		return FALSE;
	}
	memset(m_pSrcBuf, 0, SrcBufSize * sizeof * m_pSrcBuf);
	// prefill at 1/2 filter length
	m_SrcBufFilled = (0x80000000u / m_InputPeriod) * SrcFile.Channels();
	m_SrcBufUsed = 0;
	m_DstBufUsed = 0;
	WAVEFORMATEX * pWf = m_SrcFile.GetWaveFormat();
	MMCKINFO * pCk = m_SrcFile.GetDataChunk();
	m_SrcStart = pCk->dwDataOffset;
	m_SrcCopyPos = m_SrcStart;
	m_SrcEnd = m_SrcStart + pCk->cksize;

	pCk = m_DstFile.GetDataChunk();
	m_DstStart = pCk->dwDataOffset;
	m_DstCopyPos = m_DstStart;
	m_DstEnd = m_DstStart + pCk->cksize;

	return TRUE;
}

BOOL CResampleContext::OperationProc()
{
	DWORD dwOperationBegin = m_DstCopyPos;
	DWORD dwStartTime = GetTickCount();
	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
		return TRUE;
	}
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
	__int16 * pSrcBuf;
	void * pOriginalDstBuf = NULL;
	__int16 * pDstBuf;
	do
	{
		if ((m_SrcBufFilled - m_SrcBufUsed) / m_SrcFile.Channels()
			<= m_SrcFilterLength * 2)
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
					// fill the rest of the input buffer with zeros
					for (int j = m_SrcBufFilled; j < SrcBufSize; j++)
					{
						m_pSrcBuf[j] = 0.;
					}
					m_SrcBufFilled = j;
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
				pSrcBuf = (__int16 *) pOriginalSrcBuf;
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

			ASSERT((char*)pSrcBuf + LeftToRead == WasRead + (char*)pOriginalSrcBuf);
			if (0 == LeftToRead)
			{
				m_SrcFile.m_File.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
												CDirectFile::ReturnBufferDiscard);
				WasRead = 0;
			}
		}

		m_DstBufUsed = 0;
		FilterSoundResample();
		int DstBufSaved = 0;
		while (DstBufSaved < m_DstBufUsed)
		{
			if (0 == LeftToWrite)
			{
				DWORD SizeToWrite = m_DstEnd - m_DstCopyPos;
				if (0 == SizeToWrite)
				{
					break;  // all data written
				}
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
				pDstBuf = (__int16 *) pOriginalDstBuf;
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

			ASSERT((char *)pDstBuf + LeftToWrite == WasLockedToWrite + (char*)pOriginalDstBuf);
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

void CResampleContext::PostRetire(BOOL bChildContext)
{
	if (m_Flags & OperationContextFinished)
	{
		pDocument->m_WavFile = m_DstFile;
		// since we replaced the file, it's no more direct
		if (NULL != m_pUndoContext)
		{
			m_pUndoContext->m_bOldDirectMode = pDocument->m_bDirectMode;
			m_pUndoContext->m_OldAllocatedWavePeakSize =
				pDocument->m_AllocatedWavePeakSize;
			m_pUndoContext->m_OldWavePeakSize = pDocument->m_WavePeakSize;
			pDocument->m_WavePeakSize = 0;
			m_pUndoContext->m_OldPeakDataGranularity =
				pDocument->m_PeakDataGranularity;
			m_pUndoContext->m_pOldPeaks = pDocument->m_pPeaks;
			// detach peaks
			pDocument->m_pPeaks = NULL;

			pDocument->AddUndoRedo(m_pUndoContext);
			m_pUndoContext = NULL;
		}
		if (pDocument->m_bDirectMode)
		{
			pDocument->m_bDirectMode = false;
			pDocument->UpdateFrameTitles();        // will cause name change in views
		}
		pDocument->SetModifiedFlag();
		long nSamples = pDocument->WaveFileSamples();
		pDocument->SoundChanged(pDocument->WaveFileID(),
								0, nSamples, nSamples, UpdateSoundDontRescanPeaks);
		pDocument->BuildPeakInfo();
		//pDocument->UpdateAllViews(NULL);
	}
	else
	{
		// we don't replace the file, nothing changed
		if (NULL != m_pUndoContext)
		{
			delete m_pUndoContext;
			m_pUndoContext = NULL;
		}
	}
	CCopyContext::PostRetire(bChildContext);
}
