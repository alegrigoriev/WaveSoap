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
		CSimpleCriticalSection()
		{
			InitializeCriticalSection( & m_cs);
		}
		~CSimpleCriticalSection()
		{
			DeleteCriticalSection( & m_cs);
		}
		void Lock()
		{
			EnterCriticalSection( & m_cs);
		}
		void Unlock()
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
	long Write(const void * pBuf, long count);
	long Read(void * buf, long count);
	long ReadAt(void * buf, long count, LONGLONG Position);
	LONGLONG Seek(LONGLONG position, int flag);

	enum { ReturnBufferDirty = 1, // buffer contains changed data
		ReturnBufferDiscard = 2 // make the buffer lower priority
	};
	// unlock the buffer, mark dirty if necessary
	void ReturnDataBuffer(void * pBuffer, long count, DWORD flags = 0)
	{
		ASSERT(NULL != CDirectFileCache::SingleInstance);
		ASSERT(m_pFile != NULL);
		if (NULL == m_pFile)
		{
			return;
		}
		CDirectFileCache::SingleInstance->ReturnDataBuffer(m_pFile, pBuffer, count, flags);
	}

	enum {GetBufferWriteOnly = 1,
		GetBufferNoPrefetch = 2,
		GetBufferAndPrefetchNext = 4,
	};
	// read data, lock the buffer
	// and return the buffer address
	long GetDataBuffer(void * * ppBuf, LONGLONG length, LONGLONG position, DWORD flags = 0)
	{
		ASSERT(NULL != CDirectFileCache::SingleInstance);
		ASSERT(m_pFile != NULL);
		if (NULL == m_pFile)
		{
			return 0;
		}
		return CDirectFileCache::SingleInstance->GetDataBuffer(m_pFile, ppBuf, length, position, flags);
	}

	CDirectFile();
	virtual ~CDirectFile();
	enum {
		OpenReadOnly = 1,
		CreateNew = 2,
		OpenToTemporary = 4,
		OpenDirect = 8
	};
	BOOL Open(LPCTSTR szName, DWORD flags);
	BOOL Close(DWORD flags);
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
	struct File
	{
		File * pPrev;   // prev link
		File * pNext;   // next link
		HANDLE hFile;
		DWORD Flags;
		long RefCount;
		struct BufferHeader * volatile BuffersListHead;
		struct BufferHeader * volatile BuffersListTail;
		LONGLONG FilePointer;
		CSimpleCriticalSection m_ListLock;    // synchronize BufferList changes
		CSimpleCriticalSection m_FileLock;    // synchronize FileRead, FileWrite
		CString sName;
		BufferHeader * FindBuffer(unsigned long key) const;
		void InsertBuffer(BufferHeader * pBuf);

		File(CString name) : hFile(NULL),
			sName(name),
			Flags(0),
			FilePointer(0),
			BuffersListHead(NULL),
			BuffersListTail(NULL),
			RefCount(1),
			pPrev(NULL),
			pNext(NULL) {}
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
	};
	friend struct File;

public:
	class CDirectFileCache
	{
		friend class CDirectFile;

		long GetDataBuffer(File * pFile, void * * ppBuf,
							LONGLONG length, LONGLONG position, DWORD flags = 0,
							unsigned MaxMRU = 0);
		void ReturnDataBuffer(File * pFile, void * pBuffer,
							long count, DWORD flags);

		BufferHeader * GetFreeBuffer(unsigned MaxMRU = 0xFFFFFFFFu);
		void ReadDataBuffer(BufferHeader * pBuf, DWORD MaskToRead);
		void FlushDirtyBuffers(BufferHeader * pBuf);
		File * Open(LPCTSTR szName, DWORD flags);
		BOOL Close(File * pFile, DWORD flags);
		void RequestPrefetch(File * pFile, LONGLONG PrefetchPosition,
							LONGLONG PrefetchLength, unsigned MaxMRU);

	public:
		CDirectFileCache(size_t CacheSize);
		~CDirectFileCache();
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
	//LONGLONG m_FileLength;
};

#endif // !defined(AFX_DIRECTFILE_H__B7AA7401_4036_11D4_9ADD_00C0F0583C4B__INCLUDED_)
