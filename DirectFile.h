// DirectFile.h: interface for the CDirectFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DIRECTFILE_H__B7AA7401_4036_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_DIRECTFILE_H__B7AA7401_4036_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "KListEntry.h"

class CDirectFile
{
public:
	CDirectFile();
	virtual ~CDirectFile();
	CDirectFile const & operator=(CDirectFile & file);
	struct InstanceData
	{
		virtual ~InstanceData() {}
		size_t m_size;
		InstanceData() : m_size(sizeof (InstanceData)) {}
		virtual void CopyDataTo(InstanceData * dst)
		{
		}
	};

	enum {
		OpenReadOnly = 1,
		CreateNew = 2,
		OpenToTemporary = 4,
		OpenDirect = 8,
		OpenExisting = 0x10,
		OpenDeleteAfterClose = 0x20,
		CreateAlways = 0x40,
		// if couldn't be opened for writing, try read-only
		OpenAllowReadOnlyFallback = 0x80,
		CreateMemoryFile = 0x100,   // real file won't be created
	};
	BOOL Open(LPCTSTR szName, DWORD flags);
	BOOL Close(DWORD flags);
	BOOL Attach(CDirectFile * const pOriginalFile);
	BOOL SetSourceFile(CDirectFile * const pOriginalFile);
	BOOL DetachSourceFile();
	// allocate data common for all instances
	// attached to the same File
	BOOL IsOpen() const
	{
		return m_pFile != 0;
	}
	int GetFileRefCount() const
	{
		if (NULL != m_pFile)
		{
			return m_pFile->RefCount;
		}
		else
		{
			return 0;
		}
	}

	BOOL IsReadOnly() const
	{
		if (NULL == m_pFile)
		{
			return FALSE;
		}

		return 0 != (m_pFile->m_Flags & FileFlagsReadOnly);
	}

	template<typename T>
	T * AllocateInstanceData();

	InstanceData * GetInstanceData() const
	{
		if (NULL != m_pFile)
		{
			return m_pFile->GetInstanceData();
		}
		else
		{
			return NULL;
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
	// file MUST be open!
	void ModifyFlags(DWORD set, DWORD reset)
	{
		if (NULL != m_pFile)
		{
			m_pFile->m_Flags &= ~reset;
			m_pFile->m_Flags |= set;
		}
	}
	void DeleteOnClose(bool Delete = true)
	{
		if (NULL != m_pFile)
		{
			if (Delete)
			{
				m_pFile->m_Flags |= FileFlagsDeleteAfterClose;
			}
			else
			{
				m_pFile->m_Flags &= ~FileFlagsDeleteAfterClose;
			}
		}
	}

	enum {
		CommitFileFlushBuffers = 1,
		RenameFileFlushBuffers = CommitFileFlushBuffers,
		CommitFileDontReopen = 2,
		RenameFileDontReopen = CommitFileDontReopen,
		RenameFileOpenReadOnly = 4,
	};

	BOOL Commit(DWORD flags = CommitFileFlushBuffers)
	{
		if (NULL != m_pFile)
		{
			return m_pFile->Commit(flags);
		}
		else
			return FALSE;
	}

	BOOL Rename(LPCTSTR NewName, DWORD flags)
	{
		if (NULL != m_pFile)
		{
			return m_pFile->Rename(NewName, flags);
		}
		else
			return FALSE;
	}

	BOOL InitializeTheRestOfFile(int timeout = 0, int * pPercentCompleted = NULL)
	{
		if (NULL != m_pFile)
		{
			return m_pFile->InitializeTheRestOfFile(timeout, pPercentCompleted);
		}
		else
			return TRUE;    // operation completed
	}

	DWORD Flags() const
	{
		if (NULL != m_pFile)
		{
			return m_pFile->m_Flags;
		}
		else
		{
			return 0;
		}
	}

	enum { ReturnBufferDirty = 1, // buffer contains changed data
		ReturnBufferDiscard = 2, // make the buffer lower priority
		ReturnBufferFlush = 4,  // request buffer flush
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
			if (m_pFile->m_Flags & FileFlagsMemoryFile)
			{
				if (NULL != lpFileSizeHigh)
				{
					*lpFileSizeHigh = 0;
					return DWORD(m_pFile->FileLength);
				}
			}
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
		if (NULL == m_pFile)
		{
			return 0;
		}
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

	BY_HANDLE_FILE_INFORMATION const & GetFileInformation() const
	{
		return m_pFile->m_FileInfo;
	}
	int GetLastError() const
	{
		if (m_pFile != NULL)
			return m_pFile->m_LastError;
		else
			return 0;
	}
	void ResetLastError()
	{
		if (m_pFile != NULL)
			m_pFile->m_LastError = 0;
	}
	bool IsTemporaryFile() const
	{
		return IsOpen()
				&& 0 == (Flags() & (FileFlagsDeleteAfterClose | FileFlagsMemoryFile));
	}


protected:
	struct BufferHeader;
	enum {
		FileFlagsDeleteAfterClose = OpenDeleteAfterClose,
		FileFlagsReadOnly = OpenReadOnly,
		FileFlagsMemoryFile = CreateMemoryFile,
	};
	struct File : public KListEntry<File>
	{
		HANDLE hFile;
		DWORD m_Flags;
		long RefCount;
		LockedListHead<BufferHeader> volatile mutable BuffersList;
		// number of buffers in the list
		int BuffersCount;
		union {
			long DirtyBuffersCount;
			long m_MemoryBufferRefCount;
		};
		// bitmask of 64K blocks that were once initialized.
		// if the bit is 0, the block contains garbage and it should be
		// zeroed or read from the source file
		union {
			char * m_pWrittenMask;
			char * m_pMemoryFileBuffer;
		};
		union {
			long WrittenMaskSize;
			long m_MemoryFileBufferSize;
		};
		// data common for all CDirectFile instances, attached to this File
		InstanceData * m_pInstanceData;

		// pointer to the source file. The information is copied from there
		// when it is read first time.
		File * pSourceFile;
		int m_LastError;
		LONGLONG UseSourceFileLength;
		LONGLONG m_FilePointer;
		LONGLONG RealFileLength;
		LONGLONG FileLength;
		LONGLONG m_FlushBegin;
		LONG m_FlushLength;

		// prefetch control
		// shows last prefetched range (in 64K units
		DWORD m_PrefetchedBeginBlock;
		DWORD m_PrefetchedEndBlock;
		// max number of 64K block allowed to read ahead
		DWORD m_MaxBlocksToPrefetch;

		// the ranges expire after 1 second
		DWORD m_LastPrefetchTick;
		enum { PrefetchRangeExpirationTimeout = 1000, };

		CSimpleCriticalSection mutable m_FileLock;    // synchronize FileRead, FileWrite
		BY_HANDLE_FILE_INFORMATION m_FileInfo;
		CString sName;
		BufferHeader * FindBuffer(unsigned long key) const;
		void InsertBuffer(BufferHeader * pBuf);
		BOOL SetFileLength(LONGLONG NewLength);
		BOOL Flush();
		BOOL InitializeTheRestOfFile(int timeout = 0, int * pPercentCompleted = NULL);
		BOOL SetSourceFile(File * pOriginalFile);
		template<typename T>
		T * AllocateInstanceData();

		InstanceData * GetInstanceData() const
		{
			return m_pInstanceData;
		}

		void ReturnDataBuffer(void * pBuffer, long count, DWORD flags = 0)
		{
			ASSERT(NULL != GetCache());
			GetCache()->ReturnDataBuffer(this, pBuffer, count, flags);
		}
		BOOL Close(DWORD flags);
		BOOL Commit(DWORD flags);
		BOOL Rename(LPCTSTR NewName, DWORD flags);

		void ReadDataBuffer(BufferHeader * pBuf, DWORD MaskToRead);
		void FlushRequestedRange();
		// read data, lock the buffer
		// and return the buffer address
		long GetDataBuffer(void * * ppBuf, LONGLONG length, LONGLONG position, DWORD flags = 0)
		{
			ASSERT(NULL != GetCache());
			return GetCache()->GetDataBuffer(this, ppBuf, length, position, flags);
		}
		File(CString name) : hFile(NULL),
			sName(name),
			m_Flags(0),
			m_FilePointer(0),
			FileLength(0),
			RealFileLength(0),
			BuffersCount(0),
			DirtyBuffersCount(0),
			RefCount(1),
			m_pWrittenMask(NULL),
			WrittenMaskSize(0),
			pSourceFile(NULL),
			UseSourceFileLength(0),
			m_pInstanceData(new InstanceData),
			m_PrefetchedBeginBlock(-1),
			m_PrefetchedEndBlock(-1),
			m_LastPrefetchTick(0),
			m_FlushBegin(0),
			m_FlushLength(0),
			// one sixth of cache size is allowed to prefetch
			m_MaxBlocksToPrefetch(GetCache()->m_NumberOfBuffers / 6),
			m_LastError(0)
		{
			memzero(m_FileInfo);
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
			delete m_pInstanceData;
		}
	};

	struct BufferMruEntry : public KListEntry<BufferMruEntry>
	{
		long LockCount;
		unsigned MRU_Count;
		DWORD m_Flags;
		DWORD ReadMask;     // 32 bits for data available (a bit per 2K)
		DWORD DirtyMask;    // 32 bits for dirty data
		File * pFile;
		unsigned long PositionKey;   // position / 0x10000
	};
	struct BufferHeader : public KListEntry<BufferHeader>, public BufferMruEntry
	{
		// pointer in the common list
		void * pBuf;        // corresponding buffer
		void FlushDirtyBuffers(unsigned long MaxKey = 0xFFFFFFFF);
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
		File * Open(LPCTSTR szName, DWORD flags);
		void RequestPrefetch(File * pFile, LONGLONG PrefetchPosition,
							LONGLONG PrefetchLength, unsigned MaxMRU);
		void RequestFlush(File * pFile, LONGLONG FlushPosition,
						LONG FlushLength);
		void FlushRequestedFiles();

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
		// when you traverse from Head to Tail, jump to pNext.
		// when you traverse from Tail to Head , jump to pPrev.
		// Pointer to a buffer with highest MRU. This buffer have pMruPrev=NULL
		KListEntry<BufferMruEntry> m_MruList;

		void * m_pBuffersArray;    // allocated area
		int m_NumberOfBuffers; // number of allocated buffers
		KListEntry<BufferHeader> m_FreeBuffers;
		KListEntry<File> m_FileList;
		DWORD m_Flags;
		unsigned volatile m_MRU_Count;

		HANDLE m_hThread;
		HANDLE m_hEvent;
		HANDLE m_hThreadSuspendedEvent;
		BOOL m_bRunThread;

		volatile LONG m_ThreadRunState;
		// Run State:
		// 0 - free running, waiting for operations
		// 0x80000000 - executing prefetch or flush, no suspend requested
		// > 0 - suspend requested n times, thread waiting for m_hEvent
		// 0x800000xx - thread executing, suspend requested xx times
		// ~0 (0xFFFFFFFF) - exit requested

		File * volatile m_pPrefetchFile;
		BOOL volatile m_FlushRequest;
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
private:
	// copy constructor inaccessible
	CDirectFile(CDirectFile const &);
};

template<typename T>
inline T * CDirectFile::File::AllocateInstanceData()
{
	if (sizeof (T) > m_pInstanceData->m_size)
	{
		T * ptr = new T;
		m_pInstanceData->CopyDataTo(ptr);
		delete m_pInstanceData;
		m_pInstanceData = ptr;
	}
	return static_cast<T *>(m_pInstanceData);
}
template<typename T>
inline T * CDirectFile::AllocateInstanceData()
{
	if (NULL != m_pFile)
	{
		return m_pFile->AllocateInstanceData<T>();
	}
	else
	{
		return NULL;
	}
}
#endif // !defined(AFX_DIRECTFILE_H__B7AA7401_4036_11D4_9ADD_00C0F0583C4B__INCLUDED_)
