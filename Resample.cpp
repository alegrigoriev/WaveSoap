// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// Resample.cpp
#include "stdafx.h"
#include "WaveSoapFrontDoc.h"
#include "resource.h"
#include "Resample.h"
#include "MessageBoxSynch.h"

#define _USE_MATH_DEFINES   // for M_PI definition
#include <math.h>

CResampleContext::CResampleContext(CWaveSoapFrontDoc * pDoc,
									UINT StatusStringId, UINT OperationNameId,
									CWaveFile & SrcFile, CWaveFile &DstFile,
									double FrequencyRatio, double FilterLength)
	: BaseClass(pDoc, 0, StatusStringId, OperationNameId)
{
	InitSource(SrcFile, 0, LAST_SAMPLE, ALL_CHANNELS);
	InitDestination(DstFile, 0, LAST_SAMPLE, ALL_CHANNELS, FALSE, 0, 0);

	m_Resample.InitResample(FrequencyRatio, FilterLength, SrcFile.Channels());
}

BOOL CResampleContext::OperationProc()
{
	//SAMPLE_POSITION dwOperationBegin = m_DstPos;
	DWORD dwStartTime = GetTickCount();

	if (m_Flags & OperationContextStopRequested)
	{
		m_Flags |= OperationContextStop;
		return TRUE;
	}
	if (m_DstPos >= m_DstEnd)
	{
		m_Flags |= OperationContextFinished;
		return TRUE;
	}

	DWORD LeftToRead = 0;
	DWORD LeftToWrite = 0;
	DWORD WasRead = 0;
	DWORD WasLockedToWrite = 0;
	void * pOriginalSrcBuf = NULL;
	char * pSrcBuf = NULL;
	void * pOriginalDstBuf = NULL;
	char * pDstBuf = NULL;

	do
	{
		if (0 == LeftToRead)
		{
			MEDIA_FILE_SIZE SizeToRead = m_SrcEnd - m_SrcPos;
			if (SizeToRead > CDirectFile::CacheBufferSize())
			{
				SizeToRead = CDirectFile::CacheBufferSize();
			}
			if (SizeToRead != 0)
			{
				WasRead = m_SrcFile.GetDataBuffer( & pOriginalSrcBuf,
													SizeToRead, m_SrcPos, CDirectFile::GetBufferAndPrefetchNext);
				if (0 == WasRead)
				{
					m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
												CDirectFile::ReturnBufferDirty);

					return FALSE;
				}
				pSrcBuf = (char *) pOriginalSrcBuf;
				LeftToRead = WasRead;
			}
			else
			{
				pSrcBuf = NULL;
				LeftToRead = 0;
				WasRead = 0;
			}
		}

		if (0 == LeftToWrite)
		{
			MEDIA_FILE_SIZE SizeToWrite = m_DstEnd - m_DstPos;
			if (0 == SizeToWrite)
			{
				break;  // all data written
			}
			WasLockedToWrite = m_DstFile.GetDataBuffer( & pOriginalDstBuf,
														SizeToWrite, m_DstPos, CDirectFile::GetBufferWriteOnly);

			if (0 == WasLockedToWrite)
			{
				m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
											CDirectFile::ReturnBufferDiscard);

				return FALSE;
			}
			pDstBuf = (char *) pOriginalDstBuf;
			LeftToWrite = WasLockedToWrite;

		}

		size_t SrcBufUsed = 0;

		size_t DstBufUsed = m_Resample.ProcessSound(pSrcBuf, pDstBuf, LeftToRead,
													LeftToWrite, & SrcBufUsed);

		if (0) TRACE("ResampleContext: SrcPos=%d (0x%X), DstPos=%d (0x%X), src: %d bytes, dst: %d bytes\n",
					m_SrcPos, m_SrcPos, m_DstPos, m_DstPos,
					SrcBufUsed, DstBufUsed);
		LeftToRead -= SrcBufUsed;
		m_SrcPos += SrcBufUsed;
		pSrcBuf += SrcBufUsed;

		if (0 == LeftToRead)
		{
			m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
										CDirectFile::ReturnBufferDiscard);
			WasRead = 0;
		}

		LeftToWrite -= DstBufUsed;
		m_DstPos += DstBufUsed;
		pDstBuf += DstBufUsed;

		if (0 == LeftToWrite)
		{
			m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
										CDirectFile::ReturnBufferDirty);
			WasLockedToWrite = 0;
		}

	}
	while (m_DstPos < m_DstEnd
			&& timeGetTime() - dwStartTime < 200
			);

	m_SrcFile.ReturnDataBuffer(pOriginalSrcBuf, WasRead,
								CDirectFile::ReturnBufferDiscard);

	m_DstFile.ReturnDataBuffer(pOriginalDstBuf, WasLockedToWrite,
								CDirectFile::ReturnBufferDirty);

	return TRUE;
}

void CResampleContext::DeInit()
{
	BaseClass::DeInit();
	// if overflow happened, ask the user if it should continue
	if ( ! m_Resample.WasClipped())
	{
		return;
	}

	class QueryOverflow : public MainThreadCall
	{
	public:
		QueryOverflow(CResampleContext * pContext)
			: m_pContext(pContext)
		{
		}
	protected:
		virtual LRESULT Exec()
		{
			// bring document frame to the top, then return
			CDocumentPopup pop(m_pContext->m_pDocument);

			CString s;
			s.Format(IDS_SOUND_CLIPPED, m_pContext->m_pDocument->GetTitle(),
					int(m_pContext->m_Resample.GetMaxClipped() * (100. / 32678)));

			CString s1;
			s1.LoadString(IDS_CONTINUE_QUESTION);

			s += s1;
			LRESULT result = AfxMessageBox(s, MB_YESNO | MB_ICONEXCLAMATION);
			return result;
		}
		CResampleContext * const m_pContext;
	} call(this);

	if (IDNO == call.Call())
	{
		m_Flags |= OperationContextStop;
	}
}
