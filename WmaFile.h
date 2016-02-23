// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
#ifndef __WMAFILE_H_INCLUDED
#define __WMAFILE_H_INCLUDED
#include <wtypes.h>
#include <wmsdk.h>
#include <atlsync.h>
#define _INC_WINDOWSX	// DO NOT PARSE windowsx.h
#include <dshow.h>

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

class CWmaDecoderSync

{
public:

	CWmaDecoderSync();
	~CWmaDecoderSync();
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

#include <streams.h>
//#include <pullpin.h>

class CDirectShowDecoderDataSink
{
public:
	virtual HRESULT STDMETHODCALLTYPE Receive(
											/* [in] */ IMediaSample *pSample) = 0;

	virtual HRESULT STDMETHODCALLTYPE EndOfStream(void) = 0;
	virtual HRESULT STDMETHODCALLTYPE Stop(void) = 0;
};

class CDirectShowDecoder : public IPin, public IBaseFilter, IMemInputPin
{
public:
	CDirectShowDecoder(CDirectShowDecoderDataSink * MemInputPinDelegate);
	~CDirectShowDecoder();
	HRESULT Open(LPCWSTR szFilename);
	void Close();

	BOOL Init();
	void DeInit();

	HRESULT StartDecode();
	HRESULT StopDecode();

	bool IsOpened() const
	{
		return m_DecoderState >= DecoderStateOpened;
	}
	bool IsStarted() const
	{
		return m_DecoderState >= DecoderStateRunning;
	}

	CWaveFormat const & GetDstFormat() const
	{
		return m_DstWf;
	}

	NUMBER_OF_SAMPLES GetTotalSamples() const;

	enum DecoderState
	{
		DecoderStateUninitialized,
		DecoderStateInitialized,
		DecoderStateOpened,
		DecoderStatePaused,
		DecoderStateStopped,
		DecoderStateRunning,
	};

	DecoderState GetDecoderState() const
	{
		return m_DecoderState;
	}

	FILTER_STATE GetFilterState() const
	{
		return m_FilterState;
	}

protected:
	// IUnknown overrides:
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(
													/* [in] */ REFIID riid,
													/* [iid_is][out] */ _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);

	virtual ULONG STDMETHODCALLTYPE AddRef(void);

	virtual ULONG STDMETHODCALLTYPE Release(void);
	// IPin overrides:
	virtual HRESULT STDMETHODCALLTYPE Connect(
											/* [in] */ IPin *pReceivePin,
		/* [annotation][in] */
											_In_opt_  const AM_MEDIA_TYPE *pmt);

	virtual HRESULT STDMETHODCALLTYPE ReceiveConnection(
														/* [in] */ IPin *pConnector,
														/* [in] */ const AM_MEDIA_TYPE *pmt);

	virtual HRESULT STDMETHODCALLTYPE Disconnect(void);

	virtual HRESULT STDMETHODCALLTYPE ConnectedTo(
		/* [annotation][out] */
												_Out_  IPin **pPin);

	virtual HRESULT STDMETHODCALLTYPE ConnectionMediaType(
		/* [annotation][out] */
														_Out_  AM_MEDIA_TYPE *pmt);

	virtual HRESULT STDMETHODCALLTYPE QueryPinInfo(
		/* [annotation][out] */
													_Out_  PIN_INFO *pInfo);

	virtual HRESULT STDMETHODCALLTYPE QueryDirection(
		/* [annotation][out] */
													_Out_  PIN_DIRECTION *pPinDir);

	virtual HRESULT STDMETHODCALLTYPE QueryId(
		/* [annotation][out] */
											_Out_  LPWSTR *Id);

	virtual HRESULT STDMETHODCALLTYPE QueryAccept(
												/* [in] */ const AM_MEDIA_TYPE *pmt);

	virtual HRESULT STDMETHODCALLTYPE EnumMediaTypes(
		/* [annotation][out] */
													_Out_  IEnumMediaTypes **ppEnum);

	virtual HRESULT STDMETHODCALLTYPE QueryInternalConnections(
		/* [annotation][out] */
																_Out_writes_to_opt_(*nPin, *nPin)  IPin **apPin,
																/* [out][in] */ ULONG *nPin);

	virtual HRESULT STDMETHODCALLTYPE EndOfStream(void);

	virtual HRESULT STDMETHODCALLTYPE BeginFlush(void);

	virtual HRESULT STDMETHODCALLTYPE EndFlush(void);

	virtual HRESULT STDMETHODCALLTYPE NewSegment(
												/* [in] */ REFERENCE_TIME tStart,
												/* [in] */ REFERENCE_TIME tStop,
												/* [in] */ double dRate);
	/////////////////////////////////////////////////////////////////////////////////////
	// IBaseFilter overrides
	virtual HRESULT STDMETHODCALLTYPE EnumPins(
		/* [annotation][out] */
												_Out_  IEnumPins **ppEnum);

	virtual HRESULT STDMETHODCALLTYPE FindPin(
											/* [string][in] */ LPCWSTR Id,
		/* [annotation][out] */
											_Out_  IPin **ppPin);

	virtual HRESULT STDMETHODCALLTYPE QueryFilterInfo(
		/* [annotation][out] */
													_Out_  FILTER_INFO *pInfo);

	virtual HRESULT STDMETHODCALLTYPE JoinFilterGraph(
		/* [annotation][in] */
													_In_opt_  IFilterGraph *pGraph,
		/* [annotation][string][in] */
													_In_opt_  LPCWSTR pName);

	virtual HRESULT STDMETHODCALLTYPE QueryVendorInfo(
		/* [annotation][string][out] */
													_Out_  LPWSTR *pVendorInfo);
	////////////////////////////////////////////////////////////////
	// IMediaFilter overrides
	virtual HRESULT STDMETHODCALLTYPE Stop(void);

	virtual HRESULT STDMETHODCALLTYPE Pause(void);

	virtual HRESULT STDMETHODCALLTYPE Run(
										REFERENCE_TIME tStart);

	virtual HRESULT STDMETHODCALLTYPE GetState(
												/* [in] */ DWORD dwMilliSecsTimeout,
		/* [annotation][out] */
												_Out_  FILTER_STATE *State);

	virtual HRESULT STDMETHODCALLTYPE SetSyncSource(
		/* [annotation][in] */
													_In_opt_  IReferenceClock *pClock);

	virtual HRESULT STDMETHODCALLTYPE GetSyncSource(
		/* [annotation][out] */
													_Outptr_result_maybenull_  IReferenceClock **pClock);
	///////////////////////////////////////////////////////////////////////
	// IPersist overrides
	virtual HRESULT STDMETHODCALLTYPE GetClassID(
												/* [out] */ __RPC__out CLSID *pClassID);

	/////////////////////////////////////////////////////////////////////
	// IMemInputPin overrides
	virtual HRESULT STDMETHODCALLTYPE GetAllocator(
		/* [annotation][out] */
													_Out_  IMemAllocator **ppAllocator);

	virtual HRESULT STDMETHODCALLTYPE NotifyAllocator(
													/* [in] */ IMemAllocator *pAllocator,
													/* [in] */ BOOL bReadOnly);

	virtual HRESULT STDMETHODCALLTYPE GetAllocatorRequirements(
		/* [annotation][out] */
																_Out_  ALLOCATOR_PROPERTIES *pProps);

	virtual HRESULT STDMETHODCALLTYPE Receive(
											/* [in] */ IMediaSample *pSample);

	virtual HRESULT STDMETHODCALLTYPE ReceiveMultiple(
		/* [annotation][size_is][in] */
													_In_reads_(nSamples)  IMediaSample **pSamples,
													/* [in] */ long nSamples,
		/* [annotation][out] */
													_Out_  long *nSamplesProcessed);

	virtual HRESULT STDMETHODCALLTYPE ReceiveCanBlock(void);

	CStringW m_FilterName;

	CComPtr<IGraphBuilder> m_GraphBuilder;
	CMediaType m_ConnectedMediaType;

	CComPtr<IReferenceClock> m_RefClock;
	CComPtr<IPin> m_ConnectedPin;
	CComQIPtr<IAsyncReader> m_AsyncReader;
	CComPtr<IFilterGraph> m_FilterGraph;
	CDirectShowDecoderDataSink * m_DataSink;

	CWaveFormat m_DstWf;
	FILTER_STATE m_FilterState;
	DecoderState m_DecoderState;
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
