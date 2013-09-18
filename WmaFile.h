// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#ifndef __WMAFILE_H_INCLUDED
#define __WMAFILE_H_INCLUDED
#include <wtypes.h>
#include <wmsdk.h>
#include <atlsync.h>

#pragma once

class CDirectFileStream: public IStream
{
	//
	//Methods of IUnknown
	//
	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid,
											void __RPC_FAR *__RPC_FAR *ppvObject )
	{
		if ( riid == IID_IUnknown)
		{
			*ppvObject = static_cast<IUnknown*>(this);
		}
		else if ( riid == IID_IStream )
		{
			*ppvObject = static_cast<IStream*>(this);
		}
		else
		{
			*ppvObject = NULL;
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef( void ) { return 2; }

	ULONG STDMETHODCALLTYPE Release( void ) { return 1; }

public:
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read(
														/* [length_is][size_is][out] */ void __RPC_FAR *pv,
														/* [in] */ ULONG cb,
														/* [out] */ ULONG __RPC_FAR *pcbRead);

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write(
														/* [size_is][in] */ const void __RPC_FAR *pv,
														/* [in] */ ULONG cb,
														/* [out] */ ULONG __RPC_FAR *pcbWritten);
	virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek(
														/* [in] */ LARGE_INTEGER dlibMove,
														/* [in] */ DWORD dwOrigin,
														/* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition);

	virtual HRESULT STDMETHODCALLTYPE SetSize(
											/* [in] */ ULARGE_INTEGER libNewSize);

	virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo(
															/* [unique][in] */ IStream __RPC_FAR *pstm,
															/* [in] */ ULARGE_INTEGER cb,
															/* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
															/* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten);

	virtual HRESULT STDMETHODCALLTYPE Commit(
											/* [in] */ DWORD grfCommitFlags);

	virtual HRESULT STDMETHODCALLTYPE Revert( void);

	virtual HRESULT STDMETHODCALLTYPE LockRegion(
												/* [in] */ ULARGE_INTEGER libOffset,
												/* [in] */ ULARGE_INTEGER cb,
												/* [in] */ DWORD dwLockType);

	virtual HRESULT STDMETHODCALLTYPE UnlockRegion(
													/* [in] */ ULARGE_INTEGER libOffset,
													/* [in] */ ULARGE_INTEGER cb,
													/* [in] */ DWORD dwLockType);

	virtual HRESULT STDMETHODCALLTYPE Stat(
											/* [out] */ STATSTG __RPC_FAR *pstatstg,
											/* [in] */ DWORD grfStatFlag);

	virtual HRESULT STDMETHODCALLTYPE Clone(
											/* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm);

	// CWmaRead methods:
	BOOL Open(LPCTSTR szFilename, DWORD Flags);
	void Close();
	void SetFile(CDirectFile & file)
	{
		m_File = file;
	}
	DWORD GetLength()
	{
		return (DWORD)m_File.GetLength();
	}
	DWORD GetPos()
	{
		return (DWORD)m_File.Seek(0, FILE_CURRENT);
	}

private:
	CDirectFile m_File;
};
#define USE_READER_CALLBACK_ADVANCED 1
class CWmaDecoder

{
public:

	CWmaDecoder();
	~CWmaDecoder();
	//
	//Methods of IUnknown
	//
	HRESULT Open(LPCTSTR szFilename);
	HRESULT Open(CDirectFile & file);

	BOOL Init();
	void DeInit();

	HRESULT Start();
	HRESULT Stop();

	MEDIA_FILE_SIZE SrcLength()
	{
		return m_InputStream.GetLength();
	}
	MEDIA_FILE_POSITION SrcPos()
	{
		return m_InputStream.GetPos();
	}

	bool IsOpened() const
	{
		return m_bOpened;
	}
	bool IsStarted() const
	{
		return m_bStarted;
	}

	bool DeliverNextSample();
	void SetDstFile(CWaveFile & file);
	CWaveFormat const & GetSrcFormat() const
	{
		return m_SrcWf;
	}
	CWaveFormat const & GetDstFormat() const
	{
		return m_DstWf;
	}
	DWORD GetBitRate() const
	{
		return m_Bitrate;
	}
	long GetCurrentSample() const
	{
		return m_DstCopySample;
	}
	ULONG GetTotalSamples() const
	{
		return m_CurrentSamples;
	}

protected:
	BOOL volatile m_bNeedNextSample;
	CWaveFile m_DstFile;

	SAMPLE_POSITION m_DstPos;
	SAMPLE_INDEX m_DstCopySample;

	CComPtr<IWMSyncReader> m_Reader;

	CDirectFileStream m_InputStream;

	bool m_bOpened;
	bool m_bStarted;

	ATL::CEvent m_OpenedEvent;
	ATL::CEvent m_StartedEvent;
	ATL::CEvent m_SampleEvent;

	//WMT_STATUS ReaderStatus;
	QWORD m_CurrentStreamTime;
	QWORD m_BufferLengthTime; //32kbytes in 100ns units
	QWORD m_StreamDuration;

	NUMBER_OF_SAMPLES m_CurrentSamples;
	DWORD m_dwAudioOutputNum;
	CWaveFormat m_DstWf;
	CWaveFormat m_SrcWf;
	DWORD m_Bitrate;
};

class FileWriter : public IWMWriterSink
{

	//
	//Methods of IUnknown
	//
	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid,
											void __RPC_FAR *__RPC_FAR *ppvObject )
	{
		if ( riid == IID_IUnknown)
		{
			*ppvObject = static_cast<IUnknown*>(this);
		}
		else if ( riid == IID_IWMWriterSink )
		{
			*ppvObject = static_cast<IWMWriterSink*>(this);
		}
		else
		{
			*ppvObject = NULL;
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef( void )
	{
		return 2;
	}

	ULONG STDMETHODCALLTYPE Release( void )
	{
		return 1;
	}
public:
	virtual HRESULT STDMETHODCALLTYPE OnHeader(
												/* [in] */ INSSBuffer __RPC_FAR *pHeader);

	virtual HRESULT STDMETHODCALLTYPE IsRealTime(
												/* [out] */ BOOL __RPC_FAR *pfRealTime);

	virtual HRESULT STDMETHODCALLTYPE AllocateDataUnit(
														/* [in] */ DWORD cbDataUnit,
														/* [out] */ INSSBuffer __RPC_FAR *__RPC_FAR *ppDataUnit);

	virtual HRESULT STDMETHODCALLTYPE OnDataUnit(
												/* [in] */ INSSBuffer __RPC_FAR *pDataUnit);

	virtual HRESULT STDMETHODCALLTYPE OnEndWriting( void);

	FileWriter() : m_WrittenLength(0) {}
	~FileWriter() {}
	CDirectFile m_DstFile;
	ULONGLONG m_WrittenLength;

	void Open(CDirectFile & File)
	{
		m_DstFile = File;
		m_WrittenLength = 0;
	}
	ULONGLONG GetWrittenLength()
	{
		return m_WrittenLength;
	}
};

class WmaEncoder
{
public:
	WmaEncoder();
	~WmaEncoder();
	BOOL OpenWrite(CDirectFile & File);
	ULONGLONG GetWrittenLength()
	{
		return m_FileWriter.GetWrittenLength();
	}

	BOOL Init();
	void DeInit();
	void SetArtist(LPCTSTR szArtist);
	void SetAlbum(LPCTSTR szAlbum);
	void SetGenre(LPCTSTR szGenre);
	BOOL SetDestinationFormat(WAVEFORMATEX const * pDstWfx);
	BOOL Write(void const * Buf, unsigned size);
	void SetSourceWaveFormat(WAVEFORMATEX const * pSrcWfx);
	WAVEFORMATEX const * GetSourceWaveFormat() const
	{
		return m_SrcWfx;
	}
private:
	CWaveFormat m_SrcWfx;
	CComPtr<IWMWriter> m_pWriter;
	CComQIPtr<IWMWriterAdvanced> m_pWriterAdvanced;
	CComPtr<IWMProfileManager> m_pProfileManager;

	FileWriter m_FileWriter;
	QWORD m_SampleTime100ns;
	CComQIPtr<IWMHeaderInfo> m_pHeaderInfo;
	CComPtr<INSSBuffer> m_pBuffer;
};

#endif
