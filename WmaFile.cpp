// WmaFile.cpp
#include "stdafx.h"
#include "WmaFile.h"
#include <wmsysprf.h>

HRESULT STDMETHODCALLTYPE CDirectFileStream::Read(
												/* [length_is][size_is][out] */ void __RPC_FAR *pv,
												/* [in] */ ULONG cb,
												/* [out] */ ULONG __RPC_FAR *pcbRead)
{
	TRACE("CDirectFileStream::Read %d bytes\n", cb);
	LONG lRead = m_File.Read(pv, cb);
	if (lRead != -1)
	{
		if (NULL != pcbRead)
		{
			*pcbRead = lRead;
		}
		return S_OK;
	}
	else
	{
		return S_FALSE;
	}
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::Write(
													/* [size_is][in] */ const void __RPC_FAR *pv,
													/* [in] */ ULONG cb,
													/* [out] */ ULONG __RPC_FAR *pcbWritten)
{
	TRACE("CDirectFileStream::Write %d bytes\n", cb);
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
	TRACE("CDirectFileStream::Seek to %08X%08X, %d\n",
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
	TRACE("CDirectFileStream::CopyTo %d bytes\n", cb);
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
		if (Written != Locked)
		{
			break;
		}
		cb.QuadPart -= Locked;
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::Commit(
													/* [in] */ DWORD grfCommitFlags)
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
														/* [in] */ ULARGE_INTEGER libOffset,
														/* [in] */ ULARGE_INTEGER cb,
														/* [in] */ DWORD dwLockType)
{
	TRACE("CDirectFileStream::LockRegion\n");
	return STG_E_INVALIDFUNCTION;
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::UnlockRegion(
														/* [in] */ ULARGE_INTEGER libOffset,
														/* [in] */ ULARGE_INTEGER cb,
														/* [in] */ DWORD dwLockType)
{
	TRACE("CDirectFileStream::UnlockRegion\n");
	return STG_E_INVALIDFUNCTION;
}

HRESULT STDMETHODCALLTYPE CDirectFileStream::Stat(
												/* [out] */ STATSTG __RPC_FAR *pstatstg,
												/* [in] */ DWORD grfStatFlag)
{
	TRACE("CDirectFileStream::Stat flag=%x\n", grfStatFlag);
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
													/* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm)
{
	TRACE("CDirectFileStream::Clone\n");
	return STG_E_INVALIDFUNCTION;
}

void CDirectFileStream::Close()
{
	m_File.Close(0);
}


CWmaDecoder::CWmaDecoder()
	: m_Reader(NULL),
	m_pAdvReader(NULL),
	m_pwfx(NULL),
	m_pSrcWf(NULL),
	ReaderStatus(WMT_ERROR),
//m_DstPosStart(0),
//m_DstCopyPos(0),
	m_CurrentStreamTime(0),
	m_bNeedNextSample(false),
	m_BufferLengthTime(0),
	m_Bitrate(1),
	m_dwAudioOutputNum(0)
{
}
CWmaDecoder::~CWmaDecoder()
{
	if (NULL != m_pAdvReader)
	{
		m_pAdvReader->Release();
		m_pAdvReader = NULL;
	}
	if (NULL != m_Reader)
	{
		m_Reader->Release();
		m_Reader = NULL;
	}
	if (NULL != m_pwfx)
	{
		delete[] (char*) m_pwfx;
	}
	if (NULL != m_pSrcWf)
	{
		delete[] (char*) m_pSrcWf;
	}
}

HRESULT STDMETHODCALLTYPE CWmaDecoder::OnStatus( /* [in] */ WMT_STATUS Status,
															/* [in] */ HRESULT hr,
															/* [in] */ WMT_ATTR_DATATYPE dwType,
															/* [in] */ BYTE __RPC_FAR *pValue,
															/* [in] */ void __RPC_FAR *pvContext )
{
	TRACE("CWmaDecoder::OnStatus Status=%X, hr=%08X, DataType=%X, pValue=%X\n",
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
	case WMT_STOPPED:
	case WMT_STARTED:
		ReaderStatus = Status;
		break;
	case WMT_ERROR:
		ReaderStatus = Status;
		break;
	case WMT_END_OF_FILE:
		ReaderStatus = Status;
		if (m_Reader)
		{
			m_Reader->Stop();
		}
		else
		{
			ReaderStatus = WMT_STOPPED;
		}
		break;
	}
	if (WMT_END_OF_STREAMING != Status)
	{
		ReaderStatus = Status;
	}
	if (WMT_END_OF_FILE == Status)
	{
	}
	m_SignalEvent.SetEvent();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CWmaDecoder::OnSample( /* [in] */ DWORD dwOutputNum,
															/* [in] */ QWORD cnsSampleTime,
															/* [in] */ QWORD cnsSampleDuration,
															/* [in] */ DWORD dwFlags,
															/* [in] */ INSSBuffer __RPC_FAR *pSample,
															/* [in] */ void __RPC_FAR *pvContext )
{
	//
	// Make sure its Audio sample
	//
	if (m_dwAudioOutputNum != dwOutputNum)
	{
		return S_OK;
	}

	if (ReaderStatus != WMT_STARTED)
	{
		TRACE("CWmaDecoder::OnSample: ReaderStatus(%d) != WMT_STARTED\n", ReaderStatus);
		return S_OK;
	}

	HRESULT hr = S_OK;
	BYTE *pData = NULL;
	DWORD cbData = 0;

	hr = pSample->GetBufferAndLength( &pData, &cbData);

	if (0) TRACE("CWmaDecoder::OnSample, time=%d ms, %d bytes, hr=%X\n",
				DWORD(cnsSampleTime/10000), cbData, hr);
	if( FAILED( hr ) )
	{
		return hr;
	}
	m_DstFile.CDirectFile::Write(pData, cbData);
	// update current number of samples
	int nSampleSize = m_DstFile.SampleSize();
	DWORD DstCopyPos = m_DstFile.CDirectFile::Seek(0, FILE_CURRENT);
	LPMMCKINFO pck = m_DstFile.GetDataChunk();
	DWORD DstCopySample = (DstCopyPos - pck->dwDataOffset)
						/ nSampleSize;
	if (DstCopySample > m_CurrentSamples)
	{
		// calculate new length
		long TotalSamples = MulDiv(DstCopySample, SrcLength(), SrcPos());
		if (TotalSamples > 0x7FFFFFFF / nSampleSize)
		{
			TotalSamples = 0x7FFFFFFF / nSampleSize;
		}
		if (TotalSamples < m_CurrentSamples)
		{
			TotalSamples = m_CurrentSamples;
		}
		DWORD datasize = TotalSamples * nSampleSize;
		m_DstFile.SetFileLength(datasize + pck->dwDataOffset);
		m_CurrentSamples = TotalSamples;
		// update data chunk length
		pck->cksize = datasize;
		pck->dwFlags |= MMIO_DIRTY;
	}
	// modify positions after file length modified,
	m_DstCopyPos = DstCopyPos;  // to avoid race condition
	m_DstCopySample = DstCopySample;

	// ask for next buffer
	m_CurrentStreamTime = cnsSampleTime + cnsSampleDuration;
	m_bNeedNextSample = true;
	m_SignalEvent.SetEvent();
	return S_OK;
}

void CWmaDecoder::DeliverNextSample()
{
	if ( ! m_bNeedNextSample)
	{
		return;
	}
	m_bNeedNextSample = false;
	if (m_pAdvReader)
	{
		m_pAdvReader->DeliverTime(m_CurrentStreamTime + m_BufferLengthTime);
	}
	else
	{
		TRACE("NULL == m_pAdvReader, m_Reader = %X\n", m_Reader);
	}
}

BOOL CWmaDecoder::Init()
{
	HRESULT hr;
	hr = WMCreateReader( NULL, WMT_RIGHT_PLAYBACK , &m_Reader );
	if( FAILED( hr ) )
	{
		return FALSE;
	}
	hr = m_Reader->QueryInterface(IID_IWMReaderAdvanced2, ( VOID ** )& m_pAdvReader);
	if( FAILED( hr ) )
	{
		m_Reader->Release();
		m_Reader = NULL;
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
	WaitForSingleObject(m_SignalEvent, 5000);
	if (ReaderStatus != WMT_OPENED)
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
	IWMOutputMediaProps* pProps = NULL ;

	WM_MEDIA_TYPE* pMedia = NULL ;
	ULONG cbType = 0 ;
	for(DWORD i = 0 ; i < cOutputs ; i++ )
	{
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

		pProps->Release();
		pProps = NULL;

		if (pMedia->majortype == WMMEDIATYPE_Audio)
		{
			break;
		}

		delete[] pMedia;
		pMedia = NULL;
		cbType = 0;
	}
	if (NULL != pProps)
	{
		pProps->Release();
		pProps = NULL;
	}
	if( i == cOutputs || NULL == pMedia)
	{
		//
		// Couldnt find any Audio output number in the file
		//
		hr = E_UNEXPECTED;

	}
	else
	{
		m_dwAudioOutputNum = i;

		WAVEFORMATEX* pwfx = ( WAVEFORMATEX * )pMedia->pbFormat;

		m_pwfx = (WAVEFORMATEX *)new char[sizeof( WAVEFORMATEX ) + pwfx->cbSize];
		if (m_pwfx)
		{
			DWORD MaxSampleSize = 32768;
			memcpy(m_pwfx, pwfx, sizeof( WAVEFORMATEX ) + pwfx->cbSize );
			hr = m_pAdvReader->GetMaxOutputSampleSize(m_dwAudioOutputNum, & MaxSampleSize);
			TRACE("m_pAdvReader->GetMaxOutputSampleSize=%d\n", MaxSampleSize);
			if (FAILED(hr))
			{
				MaxSampleSize = 32768;
			}
			m_BufferLengthTime = MulDiv(MaxSampleSize-4, 10000000, pwfx->nAvgBytesPerSec);
		}
	}
	delete[] ( BYTE* )pMedia;
	pMedia = NULL;
	if (FAILED(hr))
	{
		return hr;
	}
	// if more than one stream, call SetStreamsSelected,
	// to have only 1 stream decompressed
	IWMHeaderInfo * pHeaderInfo = NULL;
	hr = m_Reader->QueryInterface(IID_IWMHeaderInfo, ( VOID ** )& pHeaderInfo);
	if (SUCCEEDED(hr))
	{
		WORD stream = m_dwAudioOutputNum;
		TRACE("IWMHeaderInfo interface acquired\n");
		QWORD StreamLength = 0;
		WORD SizeofStreamLength = sizeof StreamLength;
		WMT_ATTR_DATATYPE type = WMT_TYPE_QWORD;
		hr = pHeaderInfo->GetAttributeByName( & stream, g_wszWMDuration,
											& type, (BYTE *) & StreamLength, & SizeofStreamLength);
		if (SUCCEEDED(hr))
		{
			TRACE("Stream Length = %08X%08X (%d seconds), size=%d\n",
				DWORD(StreamLength >> 32), DWORD(StreamLength), DWORD(StreamLength / 10000000), SizeofStreamLength);
			m_StreamDuration = StreamLength;
		}
		pHeaderInfo->Release();
		pHeaderInfo = NULL;
	}
	else
	{
		m_StreamDuration = 10000 * 1000 * 60i64;    // 1 minute
	}

	m_CurrentSamples = m_StreamDuration * m_pwfx->nSamplesPerSec / 10000000;
	TRACE("m_CurrentSamples = %d (%d seconds)\n", m_CurrentSamples,
		m_CurrentSamples / m_pwfx->nSamplesPerSec);

	IWMProfile * pProfile = NULL;
	hr = m_Reader->QueryInterface(IID_IWMProfile, (void **) &pProfile);
	if (SUCCEEDED(hr))
	{
		IWMStreamConfig * pStreamConfig = NULL;
		hr = pProfile->GetStream(m_dwAudioOutputNum, & pStreamConfig);
		if (SUCCEEDED(hr))
		{
			if (FAILED(pStreamConfig->GetBitrate( & m_Bitrate)))
			{
				m_Bitrate = 1;
			}
			IWMMediaProps * pStreamProps = NULL;
			if (SUCCEEDED(pStreamConfig->QueryInterface(IID_IWMMediaProps, (void**) &pStreamProps)))
			{
				DWORD size = 0;
				pStreamProps->GetMediaType(NULL, & size);
				WM_MEDIA_TYPE * pType = (WM_MEDIA_TYPE *)new  char[size];
				if (pType)
				{
					pStreamProps->GetMediaType(pType, & size);
					WAVEFORMATEX * pWf = NULL;
					pWf = (WAVEFORMATEX *) pType->pbFormat;
					if (NULL != pWf)
					{
						int size= sizeof WAVEFORMATEX + pWf->cbSize;
						m_pSrcWf = (WAVEFORMATEX *) new char[size];
						if (m_pSrcWf)
						{
							memcpy(m_pSrcWf, pWf, size);
						}
					}
					delete[] (char*) pType;
				}
				pStreamProps->Release();
				pStreamProps = NULL;
			}
		}
		if (NULL != pStreamConfig)
		{
			pStreamConfig->Release();
			pStreamConfig = NULL;
		}
	}
	if (NULL != pProfile)
	{
		pProfile->Release();
		pProfile = NULL;
	}

	if (cOutputs > 1)
	{
	}

	if (NULL != m_pAdvReader)
	{
		m_pAdvReader->SetUserProvidedClock(TRUE);   // use our clock for fast decompression
	}
	return S_OK;
}

HRESULT CWmaDecoder::Start()
{
	TRACE("CWmaDecoder::Start()\n");
	m_SignalEvent.ResetEvent();
	if (NULL != m_Reader)
	{
		HRESULT hr = m_Reader->Start(0, 0, 1.0, NULL);
		TRACE("Immediately after Start: ReaderStatus=%X\n", ReaderStatus);
		WaitForSingleObject(m_SignalEvent, 5000);
		if (ReaderStatus != WMT_STARTED)
		{
			m_Reader->Stop();
			return S_FALSE;
		}
		if(SUCCEEDED(hr)
			&& NULL != m_pAdvReader)
		{
			HRESULT hr1 = m_pAdvReader->DeliverTime(m_BufferLengthTime);
			TRACE("m_pAdvReader->DeliverTime returned %X\n", hr1);
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
	TRACE("CWmaDecoder::Stop()\n");
	if (NULL != m_Reader)
	{
		return m_Reader->Stop();
	}
	else
	{
		return S_OK;
	}
}

WmaEncoder::WmaEncoder()
	:m_pWriter(NULL),
	m_pProfile(NULL),
	m_pProfileManager(NULL),
	m_pHeaderInfo(NULL),
	m_pStreamConfig(NULL),
	m_pBuffer(NULL),
	m_pFileSink(NULL)
{
}

void WmaEncoder::DeInit()
{
	if (NULL != m_pWriter)
	{
		m_pWriter->Flush();
		m_pWriter->EndWriting();
	}
	if (NULL != m_pFileSink)
	{
		m_pFileSink->Release();
		m_pFileSink = NULL;
	}
	if (NULL != m_pBuffer)
	{
		m_pBuffer->Release();
		m_pBuffer = NULL;
	}
	if (NULL != m_pStreamConfig)
	{
		m_pStreamConfig->Release();
		m_pStreamConfig = NULL;
	}
	if (NULL != m_pProfile)
	{
		m_pProfile->Release();
		m_pProfile = NULL;
	}
	if (NULL != m_pProfileManager)
	{
		m_pProfileManager->Release();
		m_pProfileManager = NULL;
	}
	if (NULL != m_pHeaderInfo)
	{
		m_pHeaderInfo->Release();
		m_pHeaderInfo = NULL;
	}
	if (NULL != m_pWriterAdvanced)
	{
		m_pWriterAdvanced->Release();
		m_pWriterAdvanced = NULL;
	}
	if (NULL != m_pWriter)
	{
		m_pWriter->Release();
		m_pWriter = NULL;
	}
}

WmaEncoder::~WmaEncoder()
{
	DeInit();
}

BOOL WmaEncoder::OpenWrite(LPCTSTR FileName)
{
#ifndef _UNICODE
	WCHAR Name[MAX_PATH+1] = {0};
	if (0 == ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, FileName, -1,
									Name, MAX_PATH))
	{
		return FALSE;
	}
	if ( ! SUCCEEDED(m_pFileSink->Open(Name)))
	{
		return FALSE;
	}
#else
	if ( ! SUCCEEDED(m_pFileSink->Open(FileName)))
	{
		return FALSE;
	}
#endif
	m_pWriterAdvanced->AddSink(m_pFileSink);
	m_pWriter->BeginWriting();
	HRESULT hr = m_pWriter->AllocateSample(0x10000, & m_pBuffer);
	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL WmaEncoder::Init()
{
	if (S_OK != WMCreateWriter(NULL, & m_pWriter))
	{
		return FALSE;
	}

	HRESULT hr = m_pWriter->QueryInterface(IID_IWMWriterAdvanced, ( VOID ** )& m_pWriterAdvanced);

	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}

	//IWMHeaderInfo * pHeaderInfo = NULL;
	hr = m_pWriter->QueryInterface(IID_IWMHeaderInfo, ( VOID ** )& m_pHeaderInfo);
	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}
#if 0
	hr = pHeaderInfo->QueryInterface(IID_IWMHeaderInfo2, ( VOID ** )& m_pHeaderInfo);
	pHeaderInfo->Release();
	pHeaderInfo = NULL;
	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}
#endif

	//IWMWriterFileSink * pSink;
	hr = WMCreateWriterFileSink( & m_pFileSink);
	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}

#if 0
	hr = pSink->QueryInterface(IID_IWMWriterFileSink2, ( VOID ** )& m_pFileSink);
	pSink->Release();
	pSink = NULL;
	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}
#endif
	hr = WMCreateProfileManager( & m_pProfileManager);
	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}
#if 0//def _DEBUG
	IWMProfileManager2 * pProfManager = NULL;
	hr = m_pProfileManager->QueryInterface(IID_IWMProfileManager2, ( VOID ** )& pProfManager);
	if (SUCCEEDED(hr))
	{
		pProfManager->SetSystemProfileVersion(WMT_VER_7_0);
		DWORD count = 0;
		pProfManager->GetSystemProfileCount( & count);
		TRACE("Number of system profiles = %d\n", count);
		for (int i = 0; i < count; i++)
		{
			IWMProfile * pSysProfile = NULL;
			hr = pProfManager->LoadSystemProfile(i, & pSysProfile);
			if (SUCCEEDED(hr))
			{
				DWORD StreamCount = 0;
				pSysProfile->GetStreamCount( & StreamCount);
				TRACE("Profile %d, num of streams = %d\n", i, StreamCount);
				for (int j = 0; j < StreamCount; j++)
				{
					IWMStreamConfig * pStream = NULL;
					hr = pSysProfile->GetStream(j, & pStream);
					if (SUCCEEDED(hr))
					{
						GUID type;
						DWORD bitrate;
						pStream->GetStreamType( & type);
						pStream->GetBitrate( & bitrate);
						if (0 == memcmp( & type, & WMMEDIATYPE_Audio, sizeof type))
						{
							TRACE("Audio stream %d, bitrate=%d\n", j, bitrate);
						}
						else if (0 == memcmp( & type, & WMMEDIATYPE_Video, sizeof type))
						{
							TRACE("Video stream %d, bitrate=%d\n", j, bitrate);
						}
						pStream->Release();
					}
				}

				pSysProfile->Release();
			}
		}
		pProfManager->Release();
	}
#endif

	hr = m_pProfileManager->LoadProfileByID(WMProfile_V70_128Audio, & m_pProfile);
	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}

	hr = m_pProfile->GetStreamByNumber(1, & m_pStreamConfig);
	if ( ! SUCCEEDED(hr))
	{
		return FALSE;
	}
#if 1
	DWORD SourceBitrate = 0;
	m_pWriter->SetProfile(m_pProfile);
	m_pStreamConfig->GetBitrate( & SourceBitrate);
	TRACE("Stream Bitrate = %d\n", SourceBitrate);

	IWMMediaProps * pProps = NULL;
	hr = m_pStreamConfig->QueryInterface(IID_IWMMediaProps, (void**) & pProps);
	if ( ! SUCCEEDED(hr))
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
	WAVEFORMATEX * pwfx = (WAVEFORMATEX *) pType->pbFormat;
	TRACE("MediaType: wFormatTag=%x, BytesPerSec = %d\n",
		pwfx->wFormatTag, pwfx->nAvgBytesPerSec);

	hr = m_pStreamConfig->SetBitrate(96024);
	hr = pProps -> GetMediaType(pType, & cbType);
	pwfx = (WAVEFORMATEX *) pType->pbFormat;
	TRACE("new MediaType: wFormatTag=%x, BytesPerSec = %d\n",
		pwfx->wFormatTag, pwfx->nAvgBytesPerSec);


	m_pStreamConfig->GetBitrate( & SourceBitrate);
	TRACE("Stream Bitrate = %d\n", SourceBitrate);

	pwfx->nAvgBytesPerSec = SourceBitrate / 8;

	pProps->SetMediaType(pType);

	m_pProfile->SetName(L"New Profile");
	m_pProfile->SetDescription(L"New Profile");
	hr = m_pProfile->ReconfigStream(m_pStreamConfig);
	TRACE("ReconfigBitrate returned %X\n", hr);
	delete[] pBuf;

	pProps->Release();
	pProps = NULL;

#endif

	return TRUE;
}

void WmaEncoder::SetArtist(LPCTSTR szArtist)
{
#ifdef _UNICODE
	m_pHeaderInfo->SetAttribute(1, g_wszWMAuthor, WMT_TYPE_STRING,
								(LPBYTE)szArtist, (wcslen(szArtist) + 1) * sizeof(TCHAR));
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
								(LPBYTE)szAlbum, (wcslen(szAlbum) + 1) * sizeof(TCHAR));
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
								(LPBYTE)szGenre, (wcslen(szGenre) + 1) * sizeof(TCHAR));
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
BOOL WmaEncoder::SetBitrate(int Bitrate);

BOOL WmaEncoder::Write(void * Buf, size_t size)
{
	PBYTE pSrcBuf = PBYTE(Buf);
	if (0 == size)
	{
		pSrcBuf = NULL;
	}
	do
	{

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

		if (NULL == pSrcBuf
			|| BufLength == MaxLength
			|| 0 != BufLength)
		{
			QWORD WriterTime = 0;
			m_pWriterAdvanced->GetWriterTime( & WriterTime);

			if (! SUCCEEDED(m_pWriter->WriteSample(0, WriterTime, 0, m_pBuffer)))
			{
				return FALSE;
			}
			m_pBuffer->SetLength(0);
		}
	} while(size != 0);

	return TRUE;
}
