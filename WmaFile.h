#ifndef __WMAFILE_H_INCLUDED
#define __WMAFILE_H_INCLUDED
#include <wtypes.h>
#include <wmsdk.h>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CDirectFileStream: public IStream
{
	//
	//Methods of IUnknown
	//
	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid,
											void __RPC_FAR *__RPC_FAR *ppvObject )
	{
		if ( riid == IID_IStream )
		{
			*ppvObject = ( IStream* )this;
		}
		else
		{
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef( void ) { return 1; }

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
class CWmaDecoder : public IWMReaderCallback//Advanced
{
public:

	CWmaDecoder();
	~CWmaDecoder();
	//
	//Methods of IUnknown
	//
	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid,
											void __RPC_FAR *__RPC_FAR *ppvObject )
	{
		if ( riid == IID_IWMReaderCallback )
		{
			*ppvObject = ( IWMReaderCallback* )this;
		}
		//else if ( riid == IID_IWMReaderCallbackAdvanced )
		//{
		//*ppvObject = ( IWMReaderCallbackAdvanced* )this;
		//}
		else if ( riid == IID_IWMStatusCallback )
		{
			*ppvObject = ( IWMStatusCallback* )this;
		}
		else
		{
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef( void ) { return 1; }

	ULONG STDMETHODCALLTYPE Release( void ) { return 1; }
	//
	//Methods of IWMReaderCallback
	//
	HRESULT STDMETHODCALLTYPE OnSample( /* [in] */ DWORD dwOutputNum,
													/* [in] */ QWORD cnsSampleTime,
													/* [in] */ QWORD cnsSampleDuration,
													/* [in] */ DWORD dwFlags,
													/* [in] */ INSSBuffer __RPC_FAR *pSample,
													/* [in] */ void __RPC_FAR *pvContext ) ;

	HRESULT STDMETHODCALLTYPE OnStatus( /* [in] */ WMT_STATUS Status,
													/* [in] */ HRESULT hr,
													/* [in] */ WMT_ATTR_DATATYPE dwType,
													/* [in] */ BYTE __RPC_FAR *pValue,
													/* [in] */ void __RPC_FAR *pvContext ) ;


	HRESULT Open(LPCTSTR szFilename);
	HRESULT Open(CDirectFile & file);
	BOOL Init();
	HRESULT Start();
	HRESULT Stop();
	DWORD SrcLength()
	{
		return m_InputStream.GetLength();
	}
	DWORD SrcPos()
	{
		return m_InputStream.GetPos();
	}
	void DeliverNextSample();
	BOOL m_bNeedNextSample;
	CWaveFile m_DstFile;
	DWORD m_DstCopyPos;
	long m_DstCopySample;

	IWMReader * m_Reader;
	IWMReaderAdvanced2 * m_pAdvReader;
	CDirectFileStream m_InputStream;
	CEvent m_SignalEvent;
	WMT_STATUS ReaderStatus;
	QWORD m_CurrentStreamTime;
	QWORD m_BufferLengthTime; //32kbytes in 100ns units
	QWORD m_StreamDuration;
	ULONG m_CurrentSamples;
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
		if ( riid == IID_IWMWriterSink )
		{
			*ppvObject = ( IWMWriterSink* )this;
		}
		else
		{
			return E_NOINTERFACE;
		}
		return S_OK;
	}

	ULONG STDMETHODCALLTYPE AddRef( void )
	{
		return 1;
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
	DWORD m_WrittenLength;
	void Open(CDirectFile & File)
	{
		m_DstFile = File;
		m_WrittenLength = 0;
	}
	DWORD GetWrittenLength()
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
	DWORD GetWrittenLength()
	{
		return m_FileWriter.GetWrittenLength();
	}
	BOOL Init();
	void DeInit();
	void SetArtist(LPCTSTR szArtist);
	void SetAlbum(LPCTSTR szAlbum);
	void SetGenre(LPCTSTR szGenre);
	BOOL SetFormat(WAVEFORMATEX * pDstWfx);
	BOOL Write(void * Buf, size_t size);
	WAVEFORMATEX m_SrcWfx;
protected:
	IWMWriter * m_pWriter;
	IWMWriterAdvanced * m_pWriterAdvanced;
	IWMProfileManager * m_pProfileManager;

	FileWriter m_FileWriter;
	DWORD m_SampleTimeMs;
	IWMHeaderInfo * m_pHeaderInfo;
	INSSBuffer * m_pBuffer;
};

#endif
