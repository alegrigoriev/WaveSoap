// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// WmaFile.cpp
#include "stdafx.h"
#include "WmaFile.h"
#include <wmsysprf.h>
#include "KInterlocked.h"
#define TRACE_WMA_DECODER 0

HRESULT STDMETHODCALLTYPE CDirectFileStream::Read(
												/* [length_is][size_is][out] */ void __RPC_FAR *pv,
												/* [in] */ ULONG cb,
												/* [out] */ ULONG __RPC_FAR *pcbRead)
{
	if (TRACE_WMA_DECODER) TRACE(_T("CDirectFileStream::Read %d bytes at pos %d, length=%d\n"),
								cb, (DWORD)m_File.Seek(0, FILE_CURRENT), (DWORD)m_File.GetLength());

	LONG lRead = m_File.Read(pv, cb);

	if (lRead != -1)
	{
		if (NULL != pcbRead)
		{
			*pcbRead = lRead;
		}
		//TRACE(_T("Read %d bytes, buffer CRC = %X\n"), lRead, CalcCrc32(PUCHAR(pv), lRead));
		return S_OK;
	}
	else
	{
		TRACE(_T("CDirectFileStream::Read failed, read %d bytes, requested %d\n"),
			lRead, cb);
		return S_FALSE;
	}
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::Write(
													/* [size_is][in] */ const void __RPC_FAR *pv,
													/* [in] */ ULONG cb,
													/* [out] */ ULONG __RPC_FAR *pcbWritten)
{
	if (TRACE_WMA_DECODER) TRACE(_T("CDirectFileStream::Write %d bytes\n"), cb);
	LONG lWritten = m_File.Write(pv, cb);
	if (-1 != lWritten)
	{
		if (NULL != pcbWritten)
		{
			*pcbWritten = lWritten;
		}
		return S_OK;
	}
	else
	{
		return STG_E_MEDIUMFULL;
	}
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::Seek(
												/* [in] */ LARGE_INTEGER dlibMove,
												/* [in] */ DWORD dwOrigin,
												/* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition)
{
	if (0 || TRACE_WMA_DECODER) TRACE(_T("CDirectFileStream::Seek to %08X%08X, %d\n"),
									dlibMove.HighPart, dlibMove.LowPart, dwOrigin);
	int origin;
	switch (dwOrigin)
	{
	case STREAM_SEEK_SET:
		origin = FILE_BEGIN;
		break;
	case STREAM_SEEK_CUR:
		origin = FILE_CURRENT;
		break;
	case STREAM_SEEK_END:
		origin = FILE_END;
		break;
	default:
		return STG_E_INVALIDFUNCTION;
	}
	LONGLONG pos = m_File.Seek(dlibMove.QuadPart, origin);
	if (-1i64 == pos)
	{
		return STG_E_INVALIDPOINTER;
	}
	if (NULL != plibNewPosition)
	{
		plibNewPosition->QuadPart = pos;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::SetSize(
													/* [in] */ ULARGE_INTEGER libNewSize)
{
	if (m_File.SetFileLength(libNewSize.QuadPart))
	{
		return S_OK;
	}
	else
	{
		return STG_E_MEDIUMFULL;
	}
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::CopyTo(
													/* [unique][in] */ IStream __RPC_FAR *pstm,
													/* [in] */ ULARGE_INTEGER cb,
													/* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
													/* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten)
{
	// copy cb bytes from this stream to *pstrm
	if (TRACE_WMA_DECODER) TRACE(_T("CDirectFileStream::CopyTo %d bytes\n"), cb);
	LONGLONG FilePos = m_File.Seek(0, FILE_CURRENT);
	LONGLONG FileLength = m_File.GetLength();
	if (NULL != pcbRead)
	{
		pcbRead->QuadPart = 0;
	}
	if (NULL != pcbWritten)
	{
		pcbWritten->QuadPart = 0;
	}
	while (cb.QuadPart > 0)
	{
		LONGLONG ToLock = cb.QuadPart;
		LONGLONG RestOfTheFile = FileLength - FilePos;
		if (ToLock > RestOfTheFile)
		{
			ToLock = RestOfTheFile;
		}
		if (0 == ToLock)
		{
			break;
		}
		void * pBuf;
		long Locked = m_File.GetDataBuffer(& pBuf, ToLock, FilePos, 0);
		if (0 == Locked)
		{
			break;
		}
		if (Locked < 0)
		{
			return STG_E_MEDIUMFULL;
		}
		FilePos += Locked;
		m_File.Seek(Locked, FILE_CURRENT);

		if (NULL != pcbRead)
		{
			pcbRead->QuadPart += Locked;
		}
		ULONG Written = 0;
		HRESULT hr = pstm->Write(pBuf, Locked, & Written);
		m_File.ReturnDataBuffer(pBuf, Locked, CDirectFile::ReturnBufferDiscard);

		if( FAILED( hr ) )
		{
			return hr ;
		}
		if (NULL != pcbWritten)
		{
			pcbWritten->QuadPart += Written;
		}
		if (Written != ULONG(Locked))
		{
			break;
		}
		cb.QuadPart -= Locked;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::Commit(
													/* [in] */ DWORD /*grfCommitFlags*/)
{
	m_File.Commit();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::Revert( void)
{
	// not really supported
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::LockRegion(
														/* [in] */ ULARGE_INTEGER /*libOffset*/,
														/* [in] */ ULARGE_INTEGER /*cb*/,
														/* [in] */ DWORD /*dwLockType*/)
{
	if (TRACE_WMA_DECODER) TRACE(_T("CDirectFileStream::LockRegion\n"));
	return STG_E_INVALIDFUNCTION;
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::UnlockRegion(
														/* [in] */ ULARGE_INTEGER /*libOffset*/,
														/* [in] */ ULARGE_INTEGER /*cb*/,
														/* [in] */ DWORD /*dwLockType*/)
{
	if (TRACE_WMA_DECODER) TRACE(_T("CDirectFileStream::UnlockRegion\n"));
	return STG_E_INVALIDFUNCTION;
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::Stat(
												/* [out] */ STATSTG __RPC_FAR *pstatstg,
												/* [in] */ DWORD grfStatFlag)
{
	if (TRACE_WMA_DECODER) TRACE(_T("CDirectFileStream::Stat flag=%x\n"), grfStatFlag);
	if (! m_File.IsOpen())
	{
		return STG_E_ACCESSDENIED;
	}
	switch (grfStatFlag)
	{
	case STATFLAG_NONAME:
		break;
	case STATFLAG_DEFAULT:
		break;
	default:
		return STG_E_INVALIDFLAG;
	}
	if (NULL == pstatstg)
	{
		return STG_E_INVALIDPOINTER;
	}
	pstatstg->type = STGTY_STREAM;
	pstatstg->cbSize.QuadPart = m_File.GetLength();
	pstatstg->mtime = m_File.GetFileInformation().ftLastWriteTime;
	pstatstg->ctime = m_File.GetFileInformation().ftCreationTime;
	pstatstg->atime = m_File.GetFileInformation().ftLastAccessTime;
	pstatstg->grfLocksSupported = 0;
	pstatstg->grfMode = 0;
	pstatstg->clsid = CLSID_NULL;
	pstatstg->grfStateBits = 0;
	pstatstg->reserved = 0;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::Clone(
													/* [out] */ IStream __RPC_FAR *__RPC_FAR * /*ppstm*/)
{
	if (TRACE_WMA_DECODER) TRACE(_T("CDirectFileStream::Clone\n"));
	return STG_E_INVALIDFUNCTION;
}

void CDirectFileStream::Close()
{
	m_File.Close(0);
}


CWmaDecoder::CWmaDecoder()
	: //ReaderStatus(WMT_ERROR),
	m_CurrentStreamTime(0),
	m_bNeedNextSample(false),
	m_BufferLengthTime(0),
	m_Bitrate(1),
	m_bOpened(false),
	m_bStarted(false),
	m_dwAudioOutputNum(0)
{
}

CWmaDecoder::~CWmaDecoder()
{
}

void CWmaDecoder::DeInit()
{
	m_pAdvReader.Release();
	m_Reader.Release();
}

HRESULT STDMETHODCALLTYPE CWmaDecoder::OnStatus( /* [in] */ WMT_STATUS Status,
															/* [in] */ HRESULT hr,
															/* [in] */ WMT_ATTR_DATATYPE dwType,
															/* [in] */ BYTE __RPC_FAR *pValue,
															/* [in] */ void __RPC_FAR * /*pvContext*/ )
{
	if (TRACE_WMA_DECODER) TRACE(_T("CWmaDecoder::OnStatus Status=%X, hr=%08X, DataType=%X, pValue=%X\n"),
								Status, hr, dwType, pValue);
	// The following values are used:
	// WMT_OPENED
	// WMT_STOPPED
	// WMT_STARTED
	// WMT_END_OF_FILE
	// WMT_ERROR
	switch (Status)
	{
	case WMT_OPENED:
		m_bOpened = true;
		m_OpenedEvent.SetEvent();
		break;
	case WMT_STARTED:
		m_bStarted = true;
		m_StartedEvent.SetEvent();
		break;
	case WMT_STOPPED:
		m_bStarted = false;
		m_StartedEvent.SetEvent();
		break;
	case WMT_ERROR:
		m_bStarted = false;
		m_StartedEvent.SetEvent();
		break;
	case WMT_END_OF_FILE:
		m_bStarted = false;
		m_StartedEvent.SetEvent();
		m_OpenedEvent.SetEvent();
		if (m_Reader)
		{
			m_Reader->Stop();
		}
		break;
	}
	if (WMT_END_OF_STREAMING == Status)
	{
		m_bStarted = false;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CWmaDecoder::OnSample( /* [in] */ DWORD dwOutputNum,
															/* [in] */ QWORD cnsSampleTime,
															/* [in] */ QWORD cnsSampleDuration,
															/* [in] */ DWORD /*dwFlags*/,
															/* [in] */ INSSBuffer __RPC_FAR *pSample,
															/* [in] */ void __RPC_FAR * /*pvContext*/ )
{
	//
	// Make sure its Audio sample
	//
	if (m_dwAudioOutputNum != dwOutputNum)
	{
		if (TRACE_WMA_DECODER) TRACE(_T("CWmaDecoder::OnSample m_dwAudioOutputNum != dwOutputNum\n"));
		return S_OK;
	}

	if (! IsStarted())
	{
		if (TRACE_WMA_DECODER) TRACE(_T("CWmaDecoder::OnSample: ! IsStarted()\n"));
		return S_OK;
	}

	HRESULT hr = S_OK;
	BYTE *pData = NULL;
	DWORD cbData = 0;

	hr = pSample->GetBufferAndLength( &pData, &cbData);

	if (TRACE_WMA_DECODER) TRACE(_T("CWmaDecoder::OnSample, time=%d ms, %d bytes, hr=%X\n"),
								DWORD(cnsSampleTime/10000), cbData, hr);
	if( FAILED( hr ) )
	{
		return hr;
	}
	m_DstFile.CDirectFile::Write(pData, cbData);

	// update current number of samples
	SAMPLE_POSITION DstCopyPos = (SAMPLE_POSITION)m_DstFile.CDirectFile::Seek(0, FILE_CURRENT);
	SAMPLE_INDEX DstCopySample = m_DstFile.PositionToSample(DstCopyPos);

	if (DstCopySample > m_CurrentSamples)
	{
		// calculate new length
		NUMBER_OF_SAMPLES MaxNumberOfSamples = 0x7FFFFFFF / m_DstFile.SampleSize();
		LONGLONG TotalSamplesEstimated = ULONG(double(DstCopySample) * SrcLength() / SrcPos());

		if (TotalSamplesEstimated > MaxNumberOfSamples)
		{
			TotalSamplesEstimated = MaxNumberOfSamples;
		}
		if (NUMBER_OF_SAMPLES(TotalSamplesEstimated) < m_CurrentSamples)
		{
			TotalSamplesEstimated = m_CurrentSamples;
		}

		m_CurrentSamples = NUMBER_OF_SAMPLES(TotalSamplesEstimated);

		m_DstFile.SetFileLengthSamples(m_CurrentSamples);
	}
	// modify positions after file length modified,
	m_DstPos = DstCopyPos;  // to avoid race condition
	m_DstCopySample = DstCopySample;

	// ask for next buffer
	m_CurrentStreamTime = cnsSampleTime + cnsSampleDuration;

	m_bNeedNextSample = true;
	m_SampleEvent.SetEvent();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CWmaDecoder::OnTime(
											/* [in] */ QWORD cnsCurrentTime,
											/* [in] */ void __RPC_FAR * /*pvContext*/)
{
	if (0 || TRACE_WMA_DECODER) TRACE(_T("IWMReaderCallbackAdvancedOnTime(%I64d)\n"), cnsCurrentTime);
	// ask for next buffer
	m_CurrentStreamTime = cnsCurrentTime;

	m_bNeedNextSample = true;
	m_SampleEvent.SetEvent();
	return S_OK;
}

void CWmaDecoder::DeliverNextSample(DWORD timeout)
{
	if (m_bNeedNextSample)
	{
		m_bNeedNextSample = false;
		if (m_pAdvReader)
		{
			QWORD NextSampleTime = m_CurrentStreamTime + m_BufferLengthTime;
			if (TRACE_WMA_DECODER) TRACE(_T("CWmaDecoder::DeliverNextSample:  m_CurrentStreamTime=%I64d, NextTime=%I64d\n"),
										m_CurrentStreamTime, NextSampleTime);

			m_pAdvReader->DeliverTime(NextSampleTime);
		}
		else
		{
			if (TRACE_WMA_DECODER) TRACE(_T("NULL == m_pAdvReader, m_Reader = %X\n"), m_Reader);
		}
	}
	else
	{
		if (TRACE_WMA_DECODER) TRACE(_T("CWmaDecoder::DeliverNextSample:  ! m_bNeedNextSample\n"));
	}
	if (0 != timeout)
	{
		WaitForSingleObject(m_SampleEvent, timeout);
	}
}

BOOL CWmaDecoder::Init()
{
	HRESULT hr;
	hr = WMCreateReader( NULL, 0 , &m_Reader );
	if( FAILED( hr ) )
	{
		return FALSE;
	}

	m_pAdvReader = m_Reader;
	if( ! m_pAdvReader)
	{
		m_Reader.Release();
		return FALSE;
	}

	m_CurrentStreamTime = 0;
	m_bNeedNextSample = true;
	return TRUE;
}

HRESULT CWmaDecoder::Open(LPCTSTR szFilename)
{
	CDirectFile file;
	if ( ! file.Open(szFilename, CDirectFile::OpenReadOnly))
	{
		return S_FALSE;
	}
	return Open(file);
}

HRESULT CWmaDecoder::Open(CDirectFile & file)
{
	HRESULT hr;
	if ( ! file.IsOpen())
	{
		return S_FALSE;
	}
	m_InputStream.SetFile(file);

	hr = m_pAdvReader->OpenStream( & m_InputStream, /* (IWMReaderCallback *) */this, NULL);
	if (FAILED(hr))
	{
		m_InputStream.Close();
		return hr;
	}
	WaitForSingleObject(m_OpenedEvent, 5000);

	if ( ! IsOpened())
	{
		m_Reader->Close();
		m_InputStream.Close();
		return S_FALSE;
	}
	// select an audio stream
	DWORD cOutputs = 0 ;
	hr = m_Reader->GetOutputCount( & cOutputs);
	if (FAILED(hr) || cOutputs > 63)
	{
		m_Reader->Close();
		m_InputStream.Close();
		return hr;
	}
	// query profile (stream properties)

	WM_MEDIA_TYPE* pMedia = NULL ;
	ULONG cbType = 0 ;

	for(DWORD i = 0 ; i < cOutputs ; i++ )
	{
		CComPtr<IWMOutputMediaProps> pProps;

		hr = m_Reader->GetOutputProps( i, &pProps );

		if (FAILED(hr))
		{
			break;
		}

		hr = pProps->GetMediaType(NULL, &cbType) ;

		if (FAILED(hr))
		{
			break;
		}

		pMedia = (WM_MEDIA_TYPE*) new BYTE[cbType];

		if (pMedia == NULL)
		{
			hr = HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY);

			break;
		}

		hr = pProps->GetMediaType(pMedia, &cbType);

		if (FAILED(hr))
		{
			break;
		}

		if (pMedia->majortype == WMMEDIATYPE_Audio)
		{
			break;
		}

		delete[] pMedia;
		pMedia = NULL;
		cbType = 0;
	}

	if( i >= cOutputs || NULL == pMedia)
	{
		//
		// Couldnt find any Audio output number in the file
		//
		hr = E_UNEXPECTED;

	}
	else
	{
#if 0
		WCHAR str1[100];
		WCHAR str2[100];
		StringFromGUID2(pMedia->majortype, str1, sizeof str1 / sizeof str1[0]);
		StringFromGUID2(pMedia->subtype, str2, sizeof str2 / sizeof str2[0]);
		if (TRACE_WMA_DECODER) TRACE(_T("Media type = %S, subtype = %S\n"), str1, str2);
// WMA: Media type = {73647561-0000-0010-8000-00AA00389B71}, sybtype = {00000001-0000-0010-8000-00AA00389B71}
// MP3: Media type = {73647561-0000-0010-8000-00AA00389B71}, sybtype = {00000001-0000-0010-8000-00AA00389B71}
#endif
		m_dwAudioOutputNum = i;

		m_DstWf = ( WAVEFORMATEX * )pMedia->pbFormat;

		DWORD MaxSampleSize = 32768;

		hr = m_pAdvReader->GetMaxOutputSampleSize(m_dwAudioOutputNum, & MaxSampleSize);
		if (TRACE_WMA_DECODER) TRACE(_T("m_pAdvReader->GetMaxOutputSampleSize=%d\n"), MaxSampleSize);
		if (FAILED(hr))
		{
			MaxSampleSize = 32768;
		}
		m_BufferLengthTime = MulDiv(MaxSampleSize, 10000000, m_DstWf.BytesPerSec());
		m_BufferLengthTime -= m_BufferLengthTime % 10000;   // round to miliseconds

		if (m_DstWf.FormatTag() != WAVE_FORMAT_PCM)
		{
			hr = E_UNEXPECTED;
		}
	}

	delete[] ( BYTE* )pMedia;
	pMedia = NULL;
	if (FAILED(hr))
	{
		return hr;
	}

	if (CComQIPtr<IWMHeaderInfo> pHeaderInfo = m_Reader)
	{
		WORD stream = WORD(m_dwAudioOutputNum);
		if (TRACE_WMA_DECODER) TRACE(_T("IWMHeaderInfo interface acquired\n"));

		QWORD StreamLength = 0;
		WORD SizeofStreamLength = sizeof StreamLength;
		WMT_ATTR_DATATYPE type = WMT_TYPE_QWORD;

		hr = pHeaderInfo->GetAttributeByName( & stream, g_wszWMDuration,
											& type, (BYTE *) & StreamLength, & SizeofStreamLength);

		if (SUCCEEDED(hr))
		{
			if (TRACE_WMA_DECODER) TRACE(_T("Stream Length = %08X%08X (%d seconds), size=%d\n"),
				DWORD(StreamLength >> 32), DWORD(StreamLength & 0xFFFFFFFF), DWORD(StreamLength / 10000000), SizeofStreamLength);
			m_StreamDuration = StreamLength;
		}
	}
	else
	{
		m_StreamDuration = 10000 * 1000 * 60i64;    // 1 minute
	}

	m_CurrentSamples = ULONG(m_StreamDuration * m_DstWf.SampleRate() / 10000000);
	if (TRACE_WMA_DECODER) TRACE(_T("m_CurrentSamples = %d (%d seconds)\n"), m_CurrentSamples,
								m_CurrentSamples / m_DstWf.SampleRate());

	if (CComQIPtr<IWMProfile> pProfile = m_Reader)
	{
		DWORD nStreamCount = 0;
		pProfile->GetStreamCount( & nStreamCount);

		for (unsigned iStream = 0; iStream < nStreamCount; iStream++)
		{
			CComPtr<IWMStreamConfig> pStreamConfig;
			hr = pProfile->GetStream(iStream, & pStreamConfig);

			if (SUCCEEDED(hr))
			{
				WORD iStreamNumber;
				pStreamConfig->GetStreamNumber( & iStreamNumber);

				if (CComQIPtr<IWMMediaProps> pStreamProps = pStreamConfig)
				{
					if (FAILED(pStreamConfig->GetBitrate( & m_Bitrate)))
					{
						m_Bitrate = 1;
					}
					DWORD size = 0;
					pStreamProps->GetMediaType(NULL, & size);

					WM_MEDIA_TYPE * pType = (WM_MEDIA_TYPE *)new  char[size];
					if (pType)
					{
						pStreamProps->GetMediaType(pType, & size);
						WMT_STREAM_SELECTION StreamSelection = WMT_ON;
						if (pType->majortype != WMMEDIATYPE_Audio)
						{
							StreamSelection = WMT_OFF;
						}

						m_pAdvReader->SetStreamsSelected(1, & iStreamNumber, & StreamSelection);

						m_SrcWf = (WAVEFORMATEX *) pType->pbFormat;
						delete[] (char*) pType;
					}
				}
			}
		}
	}

	if (m_pAdvReader)
	{
		m_pAdvReader->SetUserProvidedClock(TRUE);   // use our clock for fast decompression
	}
	return S_OK;
}

HRESULT CWmaDecoder::Start()
{
	if (1 || TRACE_WMA_DECODER) TRACE(_T("CWmaDecoder::Start()\n"));

	if (m_Reader)
	{
		HRESULT hr = m_Reader->Start(0, 0, 1.0, NULL);
		if (1 || TRACE_WMA_DECODER) TRACE(_T("m_Reader->Start=%X, Immediately after Start: IsStarted()=%X\n"),
										hr, IsStarted());
		WaitForSingleObject(m_StartedEvent, 5000);
		if ( ! IsStarted())
		{
			m_Reader->Stop();
			return S_FALSE;
		}
		if(SUCCEEDED(hr)
			&& m_pAdvReader)
		{
			HRESULT hr1 = m_pAdvReader->DeliverTime(m_BufferLengthTime);
			if (1 || TRACE_WMA_DECODER) TRACE(_T("m_pAdvReader->DeliverTime returned %X\n"), hr1);
		}
		return hr;
	}
	else
	{
		return S_FALSE;
	}
}

HRESULT CWmaDecoder::Stop()
{
	if (1 || TRACE_WMA_DECODER) TRACE(_T("CWmaDecoder::Stop()\n"));
	if (m_Reader
		&& m_bStarted)
	{
		return m_Reader->Stop();
	}
	else
	{
		return S_OK;
	}
}

void CWmaDecoder::SetDstFile(CWaveFile & file)
{
	m_DstFile = file;
	m_DstPos = file.SampleToPosition(0);
	m_DstCopySample = 0;
	m_DstFile.CDirectFile::Seek(m_DstPos, FILE_BEGIN);
}

WmaEncoder::WmaEncoder()
	:m_SampleTime100ns(0)
{
}

void WmaEncoder::DeInit()
{
	TRACE("WmaEncoder::DeInit\n");
	if (m_pWriter)
	{
		Write(NULL, 0);
		m_pWriter->Flush();
		m_pWriter->EndWriting();
	}
	m_pWriter.Release();
	m_pWriterAdvanced.Release();
	m_pProfileManager.Release();
	m_pHeaderInfo.Release();
	m_pBuffer.Release();
}

WmaEncoder::~WmaEncoder()
{
}

BOOL WmaEncoder::OpenWrite(CDirectFile & File)
{
	m_FileWriter.Open(File);
	HRESULT hr;

	hr = m_pWriterAdvanced->AddSink( & m_FileWriter);

#if 0

	m_pHeaderInfo = m_pWriter;
	if ( ! m_pHeaderInfo)
	{
		return FALSE;
	}

	m_pHeaderInfo->SetAttribute(0, g_wszWMTitle, WMT_TYPE_STRING, (BYTE const *)L"Title", sizeof L"Title");
	m_pHeaderInfo->SetAttribute(0, g_wszWMAuthor, WMT_TYPE_STRING, (BYTE const *)L"Author", sizeof L"Author");
	m_pHeaderInfo.Release();
#endif
	// open input properties
	CComPtr<IWMInputMediaProps> pMediaProps;
	hr = m_pWriter->GetInputProps(0, & pMediaProps);

	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}

	char * buf = NULL;
	DWORD size = 0;
	pMediaProps->GetMediaType(NULL, & size);
	buf = new char[size];

	WM_MEDIA_TYPE * pType = (WM_MEDIA_TYPE *) buf;
	hr = pMediaProps->GetMediaType(pType, & size);

	if (SUCCEEDED(hr))
	{
		//WAVEFORMATEX * pwfx = (WAVEFORMATEX *)pType->pbFormat;

		pType->pbFormat = (PBYTE) & m_SrcWfx;
		pType->cbFormat = sizeof m_SrcWfx;
		hr = m_pWriter->SetInputProps(0, pMediaProps);
	}

	delete[] buf;

	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}

	m_SampleTime100ns = 0;

	hr = m_pWriter->BeginWriting();

	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}

#ifdef _DEBUG
	DWORD NumSinks;
	m_pWriterAdvanced->GetSinkCount( & NumSinks);
	if (TRACE_WMA_DECODER) TRACE(_T("Writer sink count=%d\n"), NumSinks);
#endif
	return TRUE;
}

#ifdef _DEBUG
void PrintProfile(IWMProfileManager * pProfileManager, REFGUID guid)
{
	CComPtr<IWMProfile> pProfile;

	HRESULT hr;
	hr = pProfileManager->LoadProfileByID(guid, & pProfile);
	if ( ! SUCCEEDED(hr))
	{
		return;
	}

	CComPtr<IWMStreamConfig> pStreamConfig;
	hr = pProfile->GetStreamByNumber(1, & pStreamConfig);

	if (SUCCEEDED(hr))
	{
		CComQIPtr<IWMMediaProps> pProps = pStreamConfig;

		DWORD bitrate;
		pStreamConfig->GetBitrate( & bitrate);

		if (pProps)
		{
			DWORD cbType;
			//   Make the first call to establish the size of buffer needed.
			pProps -> GetMediaType(NULL, &cbType);

			//   Now create a buffer of the appropriate size
			BYTE *pBuf = new BYTE[cbType];
			//   Create an appropriate structure pointer to the buffer.
			WM_MEDIA_TYPE *pType = (WM_MEDIA_TYPE*) pBuf;

			//   Call the method again to extract the information.
			hr = pProps -> GetMediaType(pType, & cbType);

			WAVEFORMATEX * pWfx = (WAVEFORMATEX *) pType->pbFormat;
			DWORD * pExt = (DWORD *) (pType->pbFormat + sizeof WAVEFORMATEX);
			if (TRACE_WMA_DECODER) TRACE(_T("Format: %d, BytesPerSec: %d, BlockAlign: %d, cbSize: %d, stream bitrate=%d\n"),
										pWfx->wFormatTag, pWfx->nAvgBytesPerSec,
										pWfx->nBlockAlign, pWfx->cbSize, bitrate);
			if (TRACE_WMA_DECODER) TRACE(_T("FormatExtension: %08X, %08X, %08X\n"), pExt[0], pExt[1], pExt[2]);

			delete[] pBuf;

		}
	}

}
#endif

BOOL WmaEncoder::Init()
{
	HRESULT hr;
	if (S_OK != WMCreateWriter(NULL, & m_pWriter))
	{
		return FALSE;
	}

	m_pWriterAdvanced = m_pWriter;

	if ( ! m_pWriterAdvanced)
	{
		return FALSE;
	}

	hr = WMCreateProfileManager( & m_pProfileManager);
	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}

#if 0//def _DEBUG
	PrintProfile(m_pProfileManager, WMProfile_V40_64Audio);
	PrintProfile(m_pProfileManager, WMProfile_V40_96Audio);
	PrintProfile(m_pProfileManager, WMProfile_V40_128Audio);
	PrintProfile(m_pProfileManager, WMProfile_V70_64AudioISDN);
	PrintProfile(m_pProfileManager, WMProfile_V70_64Audio);
	PrintProfile(m_pProfileManager, WMProfile_V70_96Audio);
	PrintProfile(m_pProfileManager, WMProfile_V70_128Audio);
#endif

	return TRUE;
}

void WmaEncoder::SetArtist(LPCTSTR szArtist)
{
#ifdef _UNICODE
	m_pHeaderInfo->SetAttribute(1, g_wszWMAuthor, WMT_TYPE_STRING,
								(LPBYTE)szArtist, WORD((wcslen(szArtist) + 1) * sizeof(TCHAR)));
#else
	WCHAR Artist[MAX_PATH+1] = {0};
	int nChars = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szArtist, -1,
										Artist, MAX_PATH);
	if (0 == nChars)
	{
		return;
	}
	m_pHeaderInfo->SetAttribute(1, g_wszWMAuthor, WMT_TYPE_STRING,
								(LPBYTE)Artist, (nChars + 1) * sizeof(TCHAR));
#endif
}

void WmaEncoder::SetAlbum(LPCTSTR szAlbum)
{
#ifdef _UNICODE
	m_pHeaderInfo->SetAttribute(1, g_wszWMAlbumTitle, WMT_TYPE_STRING,
								(LPBYTE)szAlbum, WORD((wcslen(szAlbum) + 1) * sizeof(TCHAR)));
#else
	WCHAR Album[MAX_PATH+1] = {0};
	int nChars = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szAlbum, -1,
										Album, MAX_PATH);
	if (0 == nChars)
	{
		return;
	}
	m_pHeaderInfo->SetAttribute(1, g_wszWMAlbumTitle, WMT_TYPE_STRING,
								(LPBYTE)Album, (nChars + 1) * sizeof(TCHAR));
#endif
}

void WmaEncoder::SetGenre(LPCTSTR szGenre)
{
#ifdef _UNICODE
	m_pHeaderInfo->SetAttribute(1, g_wszWMGenre, WMT_TYPE_STRING,
								(LPBYTE)szGenre, WORD((wcslen(szGenre) + 1) * sizeof(TCHAR)));
#else
	WCHAR Genre[MAX_PATH+1] = {0};
	int nChars = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szGenre, -1,
										Genre, MAX_PATH);
	if (0 == nChars)
	{
		return;
	}
	m_pHeaderInfo->SetAttribute(1, g_wszWMGenre, WMT_TYPE_STRING,
								(LPBYTE)Genre, (nChars + 1) * sizeof(TCHAR));
#endif
}

void WmaEncoder::SetSourceWaveFormat(WAVEFORMATEX const * pSrcWfx)
{
	m_SrcWfx = pSrcWfx;
}

BOOL WmaEncoder::SetDestinationFormat(WAVEFORMATEX const * pDstWfx)
{
// pDstWfx points to the destination format (but with wFormatTag = WAVE_FORMAT_PCM
// the function must replace wFormatTag
	CComPtr<IWMProfile> pProfile;

	HRESULT hr = m_pProfileManager->LoadProfileByID(WMProfile_V70_128Audio, & pProfile);
	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}

	CComPtr<IWMStreamConfig> pStreamConfig;
	hr = pProfile->GetStreamByNumber(1, & pStreamConfig);

	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}

	CComQIPtr<IWMMediaProps> pProps = pStreamConfig;

	if ( ! pProps)
	{
		return FALSE;
	}

	DWORD cbType;
	//   Make the first call to establish the size of buffer needed.
	pProps -> GetMediaType(NULL, &cbType);

	//   Now create a buffer of the appropriate size
	BYTE *pBuf = new BYTE[cbType];
	//   Create an appropriate structure pointer to the buffer.
	WM_MEDIA_TYPE *pType = (WM_MEDIA_TYPE*) pBuf;

	//   Call the method again to extract the information.
	hr = pProps -> GetMediaType(pType, & cbType);

#ifdef _DEBUG
	WAVEFORMATEX * pWfx = (WAVEFORMATEX *) pType->pbFormat;
	DWORD * pExt = (DWORD *) (pType->pbFormat + sizeof WAVEFORMATEX);
	if (TRACE_WMA_DECODER) TRACE(_T("Format: %d, BytesPerSec: %d, BlockAlign: %d, cbSize: %d\n"),
								pWfx->wFormatTag, pWfx->nAvgBytesPerSec,
								pWfx->nBlockAlign, pWfx->cbSize);
	if (TRACE_WMA_DECODER) TRACE(_T("FormatExtension: %08X, %08X, %08X\n"), pExt[0], pExt[1], pExt[2]);

#endif
	pType->pbFormat = PBYTE(pDstWfx);
	pType->cbFormat = sizeof (WAVEFORMATEX) + pDstWfx->cbSize;

	if (TRACE_WMA_DECODER) TRACE(_T("MediaType: wFormatTag=%d, BytesPerSec = %d\n"),
								pDstWfx->wFormatTag, pDstWfx->nAvgBytesPerSec);

	hr = pStreamConfig->SetBitrate(pDstWfx->nAvgBytesPerSec * 8);
	hr = pProps->SetMediaType(pType);

	hr = pProfile->ReconfigStream(pStreamConfig);
	if (TRACE_WMA_DECODER) TRACE(_T("ReconfigStream returned %X\n"), hr);
	hr = m_pWriter->SetProfile(pProfile);
	delete[] pBuf;

	return SUCCEEDED(hr);
}

BOOL WmaEncoder::Write(void const * Buf, size_t size)
{
	UCHAR const * pSrcBuf = static_cast<UCHAR const *>(Buf);

	if (0 == size)
	{
		pSrcBuf = NULL;
	}
	do
	{

		if ( ! m_pBuffer)
		{
			HRESULT hr = m_pWriter->AllocateSample(m_SrcWfx.BytesPerSec(), & m_pBuffer);
			if (! SUCCEEDED(hr))
			{
				return FALSE;
			}
			m_pBuffer->SetLength(0);
		}

		DWORD BufLength = 0;
		DWORD MaxLength = 0;
		BYTE * pBuf;
		m_pBuffer->GetBufferAndLength( & pBuf, & BufLength);
		m_pBuffer->GetMaxLength( & MaxLength);

		if (pSrcBuf != NULL)
		{
			DWORD ToCopy = MaxLength - BufLength;
			if (ToCopy > size)
			{
				ToCopy = size;
			}
			memcpy(pBuf + BufLength, pSrcBuf, ToCopy);

			BufLength += ToCopy;
			size -= ToCopy;
			pSrcBuf += ToCopy;

			m_pBuffer->SetLength(BufLength);
		}

		if ((NULL == pSrcBuf && 0 != BufLength)
			|| BufLength == MaxLength
			)
		{
			QWORD ActualWriterTime;
			m_pWriterAdvanced->GetWriterTime( & ActualWriterTime);

			if (TRACE_WMA_DECODER) TRACE(_T("Writing src buf %p, time=%d ms, ActualWriterTime=%d ms\n"),
										m_pBuffer, DWORD(m_SampleTime100ns / 10000), DWORD(ActualWriterTime / 10000));

			if (! SUCCEEDED(m_pWriter->WriteSample(0, m_SampleTime100ns, 0, m_pBuffer)))
			{
				return FALSE;
			}
			//Sleep(50);
			m_SampleTime100ns += (1000*1000*10Ui64) * BufLength / m_SrcWfx.BytesPerSec();

			m_pBuffer.Release();
		}
	} while(size != 0);

	return TRUE;
}

HRESULT STDMETHODCALLTYPE FileWriter::OnHeader(
												/* [in] */ INSSBuffer __RPC_FAR *pHeader)
{
	if (TRACE_WMA_DECODER) TRACE(_T("FileWriter::OnHeader\n"));
	m_DstFile.Seek(0, FILE_BEGIN);
	return OnDataUnit(pHeader);
}

HRESULT STDMETHODCALLTYPE FileWriter::IsRealTime(
												/* [out] */ BOOL __RPC_FAR *pfRealTime)
{
	if (NULL != pfRealTime)
	{
		* pfRealTime = FALSE;
		return S_OK;
	}
	else
	{
		return E_POINTER;
	}
}

namespace
{
class NSSBuffer : public INSSBuffer
{

	LONG_volatile RefCount;
	DWORD BufLength;
	DWORD MaxLength;
	BYTE * pBuf;

public:
	NSSBuffer(DWORD Length)
		: RefCount(1),
		BufLength(Length),
		MaxLength(Length),
		pBuf(new BYTE[Length])
	{
	}
private:
	//
	//Methods of IUnknown
	//
	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid,
											void __RPC_FAR *__RPC_FAR *ppvObject )
	{
		if ( riid == IID_INSSBuffer )
		{
			AddRef();
			*ppvObject = ( INSSBuffer* )this;
		}
		else
		{
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef( void )
	{
		return ++RefCount;
	}

	ULONG STDMETHODCALLTYPE Release( void )
	{
		LONG Ref = --RefCount;
		if (0 == Ref)
		{
			delete this;
		}
		return Ref;
	}
	virtual HRESULT STDMETHODCALLTYPE GetLength(
												/* [out] */ DWORD __RPC_FAR *pdwLength)
	{
		if (NULL == pdwLength)
		{
			return E_POINTER;
		}
		* pdwLength = BufLength;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE SetLength(
												/* [in] */ DWORD dwLength)
	{
		if (dwLength > MaxLength)
		{
			return E_INVALIDARG;
		}
		BufLength = dwLength;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetMaxLength(
													/* [out] */ DWORD __RPC_FAR *pdwLength)
	{
		if (NULL == pdwLength)
		{
			return E_POINTER;
		}
		* pdwLength = MaxLength;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetBuffer(
												/* [out] */ BYTE __RPC_FAR *__RPC_FAR *ppdwBuffer)
	{
		if (NULL == ppdwBuffer)
		{
			return E_POINTER;
		}
		* ppdwBuffer = pBuf;
		return S_OK;
	}

	virtual HRESULT STDMETHODCALLTYPE GetBufferAndLength(
														/* [out] */ BYTE __RPC_FAR *__RPC_FAR *ppdwBuffer,
														/* [out] */ DWORD __RPC_FAR *pdwLength)
	{
		if (NULL == ppdwBuffer
			|| NULL == pdwLength)
		{
			return E_POINTER;
		}
		* pdwLength = BufLength;
		* ppdwBuffer = pBuf;
		return S_OK;
	}
	~NSSBuffer()
	{
		delete[] pBuf;
	}
};
}

HRESULT STDMETHODCALLTYPE FileWriter::AllocateDataUnit(
														/* [in] */ DWORD cbDataUnit,
														/* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppDataUnit)
{
	if (NULL == ppDataUnit)
	{
		return E_POINTER;
	}
	* ppDataUnit = new NSSBuffer(cbDataUnit);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE FileWriter::OnDataUnit(
												/* [in] */ INSSBuffer __RPC_FAR *pDataUnit)
{
	PBYTE pData;
	DWORD Length;
	if (NULL == pDataUnit)
	{
		return E_POINTER;
	}

	pDataUnit->GetBufferAndLength( & pData, & Length);
	if (TRACE_WMA_DECODER) TRACE(_T("FileWriter::OnDataUnit buf = %p, Length=%d\n"),
								pData, Length);
	if (NULL == pData)
	{
		return E_POINTER;
	}

	if (Length == (DWORD) m_DstFile.Write(pData, Length))
	{
		ULONGLONG CurPos = m_DstFile.Seek(0, FILE_CURRENT);
		if (CurPos > m_WrittenLength)
		{
			m_WrittenLength = CurPos;
		}
		//pDataUnit->SetLength(0); //???

		return S_OK;
	}
	else
	{
		return E_FAIL;
	}
}

HRESULT STDMETHODCALLTYPE FileWriter::OnEndWriting( void)
{
	if (TRACE_WMA_DECODER) TRACE(_T("OnEndWriting\n"));
	return S_OK;
}

#if USE_READER_CALLBACK_ADVANCED
HRESULT STDMETHODCALLTYPE CWmaDecoder::AllocateForStream(
														/* [in] */ WORD /*wStreamNum*/,
														/* [in] */ DWORD cbBuffer,
														/* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
														/* [in] */ void __RPC_FAR * /*pvContext*/)
{
	if (NULL == ppBuffer)
	{
		return E_POINTER;
	}
	* ppBuffer = new NSSBuffer(cbBuffer);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CWmaDecoder::AllocateForOutput(
														/* [in] */ DWORD /*dwOutputNum*/,
														/* [in] */ DWORD cbBuffer,
														/* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppBuffer,
														/* [in] */ void __RPC_FAR * /*pvContext*/)
{
	if (NULL == ppBuffer)
	{
		return E_POINTER;
	}
	* ppBuffer = new NSSBuffer(cbBuffer);
	return S_OK;
}

#endif
