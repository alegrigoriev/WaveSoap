// DirectFile.h: interface for the CDirectFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DIRECTFILE_H__B7AA7401_4036_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_DIRECTFILE_H__B7AA7401_4036_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CDirectFile
{
	class CSimpleCriticalSection
	{
		CRITICAL_SECTION m_cs;
	public:
		CSimpleCriticalSection() throw()
		{
			InitializeCriticalSection( & m_cs);
		}
		~CSimpleCriticalSection() throw()
		{
			DeleteCriticalSection( & m_cs);
		}
		void Lock() throw()
		{
			EnterCriticalSection( & m_cs);
		}
		void Unlock() throw()
		{
			LeaveCriticalSection( & m_cs);
		}
	};
	class CSimpleCriticalSectionLock
	{
		CSimpleCriticalSection & m_cs;
	public:
		CSimpleCriticalSectionLock(CSimpleCriticalSection & cs)
			: m_cs(cs)
		{
			cs.Lock();
		}
		~CSimpleCriticalSectionLock()
		{
			m_cs.Unlock();
		}
	};
public:
	CDirectFile();
	virtual ~CDirectFile();
	enum {
		OpenReadOnly = 1,
		CreateNew = 2,
		OpenToTemporary = 4,
		OpenDirect = 8,
		OpenExisting = 0x10,
		OpenDeleteAfterClose = 0x20,
		CreateAlways = 0x40,
	};
	BOOL Open(LPCTSTR szName, DWORD flags);
	BOOL Close(DWORD flags);
	BOOL Attach(CDirectFile * const pOriginalFile);
	// allocate data common for all instances
	// attached to the same File
	void * AllocateCommonData(size_t size)
	{
		if (NULL != m_pFile)
		{
			return m_pFile->AllocateCommonData(size);
		}
		else
		{
			return NULL;
		}
	}
	void * GetCommonData() const
	{
		if (NULL != m_pFile)
		{
			return m_pFile->GetCommonData();
		}
		else
		{
			return NULL;
		}
	}
	size_t GetCommonDataSize() const
	{
		if (NULL != m_pFile)
		{
			return m_pFile->GetCommonDataSize();
		}
		else
		{
			return 0;
		}
	}

	long Read(void * buf, long count);
	long ReadAt(void * buf, long count, LONGLONG Position);
	long Write(const void * pBuf, long count);
	long WriteAt(const void * buf, long count, LONGLONG Position);
	LONGLONG Seek(LONGLONG position, int flag);
	DWORD GetFileID() const
	{
		return DWORD(m_pFile);
	}

	enum { ReturnBufferDirty = 1, // buffer contains changed data
		ReturnBufferDiscard = 2, // make the buffer lower priority
	};
	// unlock the buffer, mark dirty if necessary
	void ReturnDataBuffer(void * pBuffer, long count, DWORD flags = 0)
	{
		ASSERT(NULL != GetCache());
		ASSERT(m_pFile != NULL);
		if (NULL == m_pFile)
		{
			return;
		}
		GetCache()->ReturnDataBuffer(m_pFile, pBuffer, count, flags);
	}

	enum {GetBufferWriteOnly = 1,
		GetBufferNoPrefetch = 2,
		GetBufferAndPrefetchNext = 4,
	};
	// read data, lock the buffer
	// and return the buffer address
	long GetDataBuffer(void * * ppBuf, LONGLONG length, LONGLONG position, DWORD flags = 0)
	{
		ASSERT(NULL != GetCache());
		ASSERT(m_pFile != NULL);
		if (NULL == m_pFile)
		{
			return 0;
		}
		return GetCache()->GetDataBuffer(m_pFile, ppBuf, length, position, flags);
	}

	DWORD GetFileSize(LPDWORD lpFileSizeHigh)
	{
		if (m_pFile != NULL
			&& m_pFile->hFile != NULL)
		{
			return ::GetFileSize(m_pFile->hFile, lpFileSizeHigh);
		}
		else
		{
			if (lpFileSizeHigh)
			{
				*lpFileSizeHigh = 0xFFFFFFFF;
			}
			return 0xFFFFFFFF;
		}
	}
	LONGLONG GetLength() const
	{
		return m_pFile->FileLength;
	}

	LPCTSTR GetName() const
	{
		if (NULL == m_pFile)
		{
			return _T("");
		}
		else
		{
			return m_pFile->sName;
		}
	}
	BOOL SetFileLength(LONGLONG NewLength)
	{
		if (NULL == m_pFile)
		{
			return FALSE;
		}
		return m_pFile->SetFileLength(NewLength);
	}

	BOOL GetFileInformationByHandle(LPBY_HANDLE_FILE_INFORMATION lpFileInformation)
	{
		if (m_pFile != NULL
			&& m_pFile->hFile != NULL)
		{
			return ::GetFileInformationByHandle(m_pFile->hFile, lpFileInformation);
		}
		else
		{
			return FALSE;
		}
	}

protected:
	struct BufferHeader;
	enum {
		FileFlagsDeleteAfterClose = OpenDeleteAfterClose,
		FileFlagsReadOnly = OpenReadOnly,
	};
	struct File
	{
		File * pPrev;   // prev link
		File * pNext;   // next link
		HANDLE hFile;
		DWORD Flags;
		long RefCount;
		struct BufferHeader * volatile BuffersListHead;
		struct BufferHeader * volatile BuffersListTail;
		// number of buffers in the list
		int BuffersCount;
		int DirtyBuffersCount;
		// bitmask of 64K blocks that were once initialized.
		// if the bit is 0, the block contains garbage and it should be
		// zeroed or read from the source file
		char * m_pWrittenMask;
		int WrittenMaskSize;
		// data common for all CDirectFile instances, attached to this File
		void * m_pCommonData;
		size_t m_CommonDataSize;
		// pointer to the source file. The information is copied from there
		// when it is read first time.
		File * pSourceFile;
		LONGLONG FilePointer;
		LONGLONG RealFileLength;
		LONGLONG FileLength;
		CSimpleCriticalSection mutable m_ListLock;    // synchronize BufferList changes
		CSimpleCriticalSection mutable m_FileLock;    // synchronize FileRead, FileWrite
		BY_HANDLE_FILE_INFORMATION m_FileInfo;
		CString sName;
		BufferHeader * FindBuffer(unsigned long key) const;
		void InsertBuffer(BufferHeader * pBuf);
		BOOL SetFileLength(LONGLONG NewLength);
		BOOL Flush();
		BOOL InitializeTheRestOfFile();
		void * AllocateCommonData(size_t size);
		void * GetCommonData() const
		{
			return m_pCommonData;
		}
		size_t GetCommonDataSize() const
		{
			return m_CommonDataSize;
		}

		void ReturnDataBuffer(void * pBuffer, long count, DWORD flags = 0)
		{
			ASSERT(NULL != GetCache());
			GetCache()->ReturnDataBuffer(this, pBuffer, count, flags);
		}
		BOOL Close(DWORD flags);

		// read data, lock the buffer
		// and return the buffer address
		long GetDataBuffer(void * * ppBuf, LONGLONG length, LONGLONG position, DWORD flags = 0)
		{
			ASSERT(NULL != GetCache());
			return GetCache()->GetDataBuffer(this, ppBuf, length, position, flags);
		}
		File(CString name) : hFile(NULL),
			sName(name),
			Flags(0),
			FilePointer(0),
			FileLength(0),
			RealFileLength(0),
			BuffersListHead(NULL),
			BuffersListTail(NULL),
			BuffersCount(0),
			DirtyBuffersCount(0),
			RefCount(1),
			m_pWrittenMask(NULL),
			WrittenMaskSize(0),
			pSourceFile(NULL),
			m_pCommonData(NULL),
			m_CommonDataSize(0),
			pPrev(NULL),
			pNext(NULL)
		{
			memset(& m_FileInfo, 0, sizeof m_FileInfo);
		}
			#ifdef _DEBUG
		void ValidateList() const;
			#endif
	private:
		~File()
		{
			if (NULL != m_pWrittenMask)
			{
				delete[] m_pWrittenMask;
			}
			if (NULL != m_pCommonData)
			{
				delete[] (char *) m_pCommonData;
			}
		}
	};
	struct BufferHeader
	{
		// pointer in the common list
		BufferHeader * pPrev;   // prev link for file
		BufferHeader * pNext;
		BufferHeader * pMruPrev;    // prev link for all buffers in MRU order
		BufferHeader * pMruNext;    // next link for all buffers in MRU order
		void * pBuf;        // corresponding buffer
		long LockCount;
		unsigned MRU_Count;
		DWORD Flags;
		DWORD ReadMask;     // 32 bits for data available (a bit per 2K)
		DWORD DirtyMask;    // 32 bits for dirty data
		File * pFile;
		unsigned long PositionKey;   // position / 0x10000
		void FlushDirtyBuffers();
	};
	friend struct File;

public:
	class CDirectFileCache
	{
		friend class CDirectFile;
		friend struct File;

		long GetDataBuffer(File * pFile, void * * ppBuf,
							LONGLONG length, LONGLONG position, DWORD flags = 0,
							unsigned MaxMRU = 0);
		void ReturnDataBuffer(File * pFile, void * pBuffer,
							long count, DWORD flags);

		BufferHeader * GetFreeBuffer(unsigned MaxMRU = 0xFFFFFFFFu);
		void ReadDataBuffer(BufferHeader * pBuf, DWORD MaskToRead);
		File * Open(LPCTSTR szName, DWORD flags);
		void RequestPrefetch(File * pFile, LONGLONG PrefetchPosition,
							LONGLONG PrefetchLength, unsigned MaxMRU);

	public:
		CDirectFileCache(size_t CacheSize);
		~CDirectFileCache();
		static CDirectFileCache * GetInstance()
		{
			return SingleInstance;
		}
	private:
		// points to the only instance of the class
		static CDirectFileCache * SingleInstance;
		BufferHeader * m_pHeaders;    // address of array
		BufferHeader * m_pMruListHead;
		BufferHeader * m_pMruListTail;
		void * m_pBuffersArray;    // allocated area
		int m_NumberOfBuffers; // number of allocated buffers
		BufferHeader * m_pFreeBuffers;
		File * m_pFileList;
		DWORD m_Flags;
		unsigned volatile m_MRU_Count;

		HANDLE m_hThread;
		HANDLE m_hEvent;
		BOOL m_bRunThread;

		File * volatile m_pPrefetchFile;
		LONGLONG volatile m_PrefetchPosition;
		LONGLONG volatile m_PrefetchLength;
		unsigned volatile m_MinPrefetchMRU;

		CSimpleCriticalSection m_cs;
		static unsigned __stdcall ThreadProc(void * arg)
		{
			return ((CDirectFileCache *) arg)->_ThreadProc();
		}
		unsigned _ThreadProc();
	};
	friend class CDirectFileCache;
protected:
	File * m_pFile;
	LONGLONG m_FilePointer;
public:
	static CDirectFileCache * GetCache()
	{
		return CDirectFileCache::GetInstance();
	}
};

#endif // !defined(AFX_DIRECTFILE_H__B7AA7401_4036_11D4_9ADD_00C0F0583C4B__INCLUDED_)
