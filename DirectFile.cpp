// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// DirectFile.cpp: implementation of the CDirectFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DirectFile.h"
#include <process.h>
#include "MessageBoxSynch.h"
#include "KInterlocked.h"
#include "resource.h"
#include "LastError.h"
#include "ElapsedTime.h"
#include "PathEx.h"

typedef DWORD BLOCK_INDEX;
typedef NUM_volatile<DWORD> SUBBLOCK_MASK;

#define TRACE_PREFETCH 0
#define TRACE_READ 0
#define TRACE_WRITE 0
#define TRACE_MRU 0
#define DO_VALIDATE 0
#define TRACE_CLOSE 0

#define BLOCK_SIZE_SHIFT 17
#define CACHE_BLOCK_SIZE (1UL << BLOCK_SIZE_SHIFT)
#define SUBBLOCK_SIZE_SHIFT (BLOCK_SIZE_SHIFT - 5)
#define CACHE_SUBBLOCK_SIZE (1UL << SUBBLOCK_SIZE_SHIFT)
#define MASK_OFFSET_IN_BLOCK (CACHE_BLOCK_SIZE - 1)
#define MASK_OFFSET_IN_SUBBLOCK (CACHE_SUBBLOCK_SIZE - 1)
#define FILE_OFFSET_TO_BLOCK_NUMBER(offset) \
	(BLOCK_INDEX((offset) >> BLOCK_SIZE_SHIFT))
#define FILE_OFFSET_TO_WRITTEN_MASK_OFFSET(offset) \
	(unsigned((offset) >> (BLOCK_SIZE_SHIFT + 3)))

#define FILE_LENGTH_TO_WRITTEN_MASK_LENGTH(length) \
	(unsigned((length + (1 << (BLOCK_SIZE_SHIFT + 3)) - 1) >> (BLOCK_SIZE_SHIFT + 3)))

#define MEMORY_FILE_SIZE_LIMIT 0x100000UL
#define MAX_PREFETCH_REGIONS    6

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#pragma function(memcpy)
namespace DirectFileCache
{
struct BufferMruEntry;
struct BufferHeader;
}
using namespace DirectFileCache;

static BOOL UseOverlappedIo = FALSE;

int CDirectFile::CacheBufferSize()
{
	return CACHE_BLOCK_SIZE;
}

struct PrefetchDescriptor : ListItem<PrefetchDescriptor>
{
	// prefetch control
	// shows last prefetched range (in 64K units
	// The manager prefetches a max range block,
	// then it won't continue prefetching until half of the data is used
	// while it's prefetching, it only allows non-prefetchable requests
	// to go through (sound playback?).
	PrefetchDescriptor()
		: m_pFile(NULL),
		m_PrefetchedBeginBlock(0),
		m_PrefetchedEndBlock(0),
		m_PrefetchPosition(0),
		m_LastPrefetchTick(0)
	{
	}
	File * m_pFile;
	BLOCK_INDEX m_PrefetchedBeginBlock;
	BLOCK_INDEX m_PrefetchedEndBlock;
	BLOCK_INDEX m_PrefetchPosition;
	// the ranges expire after 5 seconds
	DWORD m_LastPrefetchTick;
	enum { PrefetchRangeExpirationTimeout = 5000, };
};

class DirectFileCache::CDirectFileCache
{
	friend class CDirectFile;
	friend struct DirectFileCache::File;
	friend class CDirectFileCacheProxy;

public:
	CDirectFileCache();
	~CDirectFileCache();
protected:
	void InitCache(size_t MaxCacheSize);
	void DeInitCache();

	long GetDataBuffer(File * pFile, void * * ppBuf,
						LONGLONG length, LONGLONG position, DWORD flags = 0,
						unsigned MaxMRU = 0);

	BufferHeader * GetFreeBuffer(unsigned MaxMRU = 0xFFFFFFFFu);
	void FreeBuffer(BufferHeader * pBuf);
	// get a buffer header by its buffer addr
	BufferHeader * GetBufferHeader(void * pAddr);
	unsigned GetOffsetInBuffer(void * pAddr);

	void MakeBufferLeastRecent(BufferHeader * pBuf);

	File * Open(LPCTSTR szName, DWORD flags);

	void RequestPrefetch(File * pFile, LONGLONG PrefetchPosition,
						LONGLONG PrefetchLength, unsigned MaxMRU);
	void CancelPrefetch(File const * pFile);

	void FlushRequestedFiles();

	void SetFlushRequest()
	{
		m_FlushRequest = TRUE;
		SetEvent(m_hEvent);
	}

private:
	// points to the only instance of the class
	BufferHeader * m_pHeaders;    // address of array
	// when you traverse from Head to Tail, jump to pNext.
	// when you traverse from Tail to Head , jump to pPrev.
	// Pointer to a buffer with highest MRU. This buffer have pMruPrev=NULL
	ListHead<BufferMruEntry> m_MruList;

	void * m_pBuffersArray;    // allocated area
	unsigned m_NumberOfBuffers; // number of allocated buffers
	ListHead<BufferHeader> m_FreeBuffers;
	ListHead<File> m_FileList;

	// access to the list is protected by m_cs
	ListHead<PrefetchDescriptor> m_ListToPrefetch;
	ListHead<PrefetchDescriptor> m_ListPrefetched;  // or empty

	// max number of 64K block allowed to read ahead
	unsigned m_MaxBlocksToPrefetch;

	//DWORD m_Flags;
	ULONG_volatile m_MRU_Count;

	HANDLE m_hThread;
	HANDLE m_hEvent;

protected:
	HANDLE m_hThreadSuspendedEvent;

	LONG_volatile m_ThreadRunState;
	LONG_volatile m_InitCount;
	// Run State:
	// 0 - free running, waiting for operations
	// 0x80000000 - executing prefetch or flush, no suspend requested
	// > 0 - suspend requested n times, thread waiting for m_hEvent
	// 0x800000xx - thread executing, suspend requested xx times
	// ~0 (0xFFFFFFFF) - exit requested

	BOOL volatile m_FlushRequest;

	//unsigned volatile m_MinPrefetchMRU;

	static unsigned __stdcall ThreadProc(void * arg)
	{
		return ((CDirectFileCache *) arg)->_ThreadProc();
	}
	unsigned _ThreadProc();

public:
	CSimpleCriticalSection m_cs;
	void StopPrefetchThread()
	{
		if (++m_ThreadRunState < 0)
		{
			SetEvent(m_hEvent);
			while (m_ThreadRunState < 0)
			{
				WaitForSingleObject(m_hThreadSuspendedEvent, 2000);
			}
		}
	}

	void ResumePrefetchThread()
	{
		// allow background thread to run along
		if (--m_ThreadRunState == 0)
		{
			SetEvent(m_hEvent);
		}
	}
};

static CDirectFileCache CacheInstance;

class CacheThreadLock
{
public:
	CacheThreadLock(//DirectFileCache & Cache = CacheInstance
					)
	{
		CacheInstance.StopPrefetchThread();
	}
	~CacheThreadLock()
	{
		CacheInstance.ResumePrefetchThread();
	}
};

struct DirectFileCache::File : public ListItem<DirectFileCache::File>
{
	typedef CDirectFile::InstanceData InstanceData;
	HANDLE hFile;
	bool m_UseOverlappedIo;

	DWORD m_Flags;
	long RefCount;
	LockedListHead<BufferHeader> mutable BuffersList;
	// number of buffers in the list
	unsigned BuffersCount;
	LONG_volatile DirtyBuffersCount;
	LONG_volatile m_MemoryBufferRefCount;
	// bitmask of 64K blocks that were once initialized.
	// if the bit is 0, the block contains garbage and it should be
	// zeroed or read from the source file
	union {
		char * m_pWrittenMask;
		char * m_pMemoryFileBuffer;
	};
	union {
		size_t WrittenMaskSize;
		size_t m_MemoryFileBufferSize;
	};
	// data common for all CDirectFile instances, attached to this File
	InstanceData * m_pInstanceData;
	InstanceData * ReplaceInstanceData(InstanceData * ptr)
	{
		InstanceData * pOld = InterlockedExchange( & m_pInstanceData, ptr);
		pOld->MoveDataTo(ptr);
		return pOld;
	}

	// pointer to the source file. The information is copied from there
	// when it is read first time.
	File * pSourceFile;
	int m_LastError;
	LONGLONG UseSourceFileLength;
	LONGLONG m_FilePointer;     // current file pointer (in synch with the handle's pointer)
	LONGLONG RealFileLength;    // actual length of the written file (may be rounded to a sector boundary)
	LONGLONG FileLength;        // assumed file length
	LONGLONG m_FlushBegin;      // position from where to flush buffers
	LONG m_FlushLength;         // how much to flush

	CSimpleCriticalSection mutable m_FileLock;    // synchronize FileRead, FileWrite
	BY_HANDLE_FILE_INFORMATION m_FileInfo;

	CString m_FileName;

	BufferHeader * FindBuffer(BLOCK_INDEX key) const;

	void InsertBuffer(BufferHeader * pBuf);
	BOOL SetFileLength(LONGLONG NewLength);

	BOOL Flush();
	BOOL InitializeTheRestOfFile(int timeout = 0, LONGLONG * pSizeCompleted = NULL);
	BOOL SetSourceFile(File * pOriginalFile);

	CDirectFile::InstanceData * GetInstanceData() const
	{
		return m_pInstanceData;
	}

	void ReturnDataBuffer(void * pBuffer, long count, DWORD flags = 0);

	bool RequestFlush(LONGLONG FlushPosition, LONG FlushLength);

	BOOL Close(DWORD flags);
	BOOL Commit(DWORD flags);
	BOOL Rename(LPCTSTR NewName, DWORD flags);

	void ReadDataBuffer(BufferHeader * pBuf, DWORD MaskToRead);
	BOOL ReadFileAt(LONGLONG Position, void * pBuf,
					DWORD ToRead, DWORD * pWasRead);
	BOOL WriteFileAt(LONGLONG Position, void const * pBuf,
					DWORD ToWrite, DWORD * pWasWritten);
	void FlushRequestedRange();
	void FlushDirtyBuffers(BufferHeader * pBuf, unsigned long MaxKey = 0xFFFFFFFF);
	// read data, lock the buffer
	// and return the buffer address
	long GetDataBuffer(void * * ppBuf, LONGLONG length, LONGLONG position, DWORD flags = 0)
	{
		return CacheInstance.GetDataBuffer(this, ppBuf, length, position, flags);
	}

	void LoadFileInformation(); // load m_FileInfo member from the file info

	File(CString name) : hFile(NULL),
		m_FileName(name),
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
		m_pInstanceData(new CDirectFile::InstanceData),
		m_FlushBegin(0),
		m_FlushLength(0),
		m_UseOverlappedIo(0 != UseOverlappedIo),
		// one sixth of cache size is allowed to prefetch
		m_LastError(0)
	{
		memzero(m_FileInfo);
	}
	void ValidateList() const
#ifndef _DEBUG
	{}
#else
	;
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

struct DirectFileCache::BufferMruEntry : public ListItem<DirectFileCache::BufferMruEntry>
{
	BufferMruEntry()
		: m_Flags(0)
		, MRU_Count(0)
		, pFile(NULL)
		, PositionKey(0)
	{
	}
	LONG_volatile LockCount;
	unsigned MRU_Count;
	DWORD m_Flags;
	SUBBLOCK_MASK ReadMask;     // 32 bits for data available (a bit per 2K)
	SUBBLOCK_MASK DirtyMask;    // 32 bits for dirty data
	File * pFile;
	BLOCK_INDEX PositionKey;   // position / 0x10000
};

struct DirectFileCache::BufferHeader : public ListItem<DirectFileCache::BufferHeader>, public BufferMruEntry
{
	// pointer in the common list
	void * pBuf;        // corresponding buffer
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CDirectFile::CDirectFile()
	: m_FilePointer(0),
	m_pFile(NULL)
{

}

CDirectFile::~CDirectFile()
{
	Close(0);
}

BOOL CDirectFile::Open(LPCTSTR szName, UINT flags)
{
	Close(0);

	File * pFile = CacheInstance.Open(szName, flags);
	if (NULL == pFile)
	{
		return FALSE;
	}
	m_pFile = pFile;
	m_FilePointer = 0;
	return TRUE;
}

BOOL CDirectFile::Attach(CDirectFile * const pOriginalFile)
{
	if (NULL == pOriginalFile->m_pFile)
	{
		return FALSE;
	}

	pOriginalFile->m_pFile->RefCount++;

	Close(0);
	m_pFile = pOriginalFile->m_pFile;
	m_FilePointer = 0;
	return TRUE;
}

BOOL CDirectFile::SetSourceFile(CDirectFile * const pOriginalFile)
{
	if (NULL == pOriginalFile->m_pFile
		|| NULL == m_pFile
		|| m_pFile == pOriginalFile->m_pFile
		|| pOriginalFile->m_pFile == m_pFile)
	{
		return FALSE;
	}

	m_FilePointer = 0;
	return m_pFile->SetSourceFile(pOriginalFile->m_pFile);
}

// The function detaches pSourceFile previously set by SetSourceFile
// We need to call it before saving the data back to the main file
BOOL CDirectFile::DetachSourceFile()
{
	if (NULL == m_pFile
		|| ! m_pFile->InitializeTheRestOfFile())
	{
		return FALSE;
	}
	// all data from the source file is copied to the target,
	// we can detach it
	File * pFile = m_pFile->pSourceFile;
	m_pFile->pSourceFile = NULL;
	if (pFile)
	{
		return pFile->Close(0);
	}
	else
		return TRUE;
}

File * CDirectFileCache::Open(LPCTSTR szName, DWORD flags)
{
	// find if the file with this name is in the list.
	// if it is, add a reference
	CLastError err;

	CSimpleCriticalSectionLock lock(m_cs);

	if (flags & CDirectFile::CreateMemoryFile)
	{
		// create memory file of zero length
		File * pFile = new File("");
		if (NULL == pFile)
		{
			return NULL;
		}
		pFile->m_Flags |= CDirectFile::FileFlagsMemoryFile;


		m_FileList.InsertHead(pFile);

		err.Set(ERROR_SUCCESS);
		return pFile;
	}

	CPathEx FullName(szName);
	FullName.MakeFullPath();

	TRACE(_T("Full name: %s\n"), (LPCTSTR) FullName);

	HANDLE hf;
	BY_HANDLE_FILE_INFORMATION info;
	// open the file to query information
	hf = CreateFile(FullName,
					0,
					FILE_SHARE_WRITE | FILE_SHARE_READ,
					NULL, // lpSecurity
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if (hf != INVALID_HANDLE_VALUE)
	{
		if (FILE_TYPE_DISK != GetFileType(hf))
		{
			CloseHandle(hf);
			err.Set(ERROR_INVALID_FUNCTION);
			return NULL;
		}

		if (::GetFileInformationByHandle(hf, & info))
		{
			CloseHandle(hf);
			TRACE(_T("Volume ID: %08X, File index: %08X%08X\n"),
				info.dwVolumeSerialNumber, info.nFileIndexHigh,
				info.nFileIndexLow);
			// find a File struct with the same parameters

			for (File * pFile = m_FileList.First();
				m_FileList.NotEnd(pFile); pFile = pFile->Next())
			{
				if (info.nFileIndexHigh == pFile->m_FileInfo.nFileIndexHigh
					&& info.nFileIndexLow == pFile->m_FileInfo.nFileIndexLow)
				{
					TRACE("File is in the list\n");
					// read-only file can be reopened only for reading
					if ((pFile->m_Flags & CDirectFile::FileFlagsReadOnly)
						&& 0 == (flags & CDirectFile::OpenReadOnly))
					{
						err.Set(ERROR_SHARING_VIOLATION);
						return NULL;
					}

					err.Set(ERROR_ALREADY_EXISTS);
					pFile->RefCount++;
					return pFile;
				}
			}
		}
		else
		{
			CloseHandle(hf);
		}
	}
	// otherwise construct a new File structure, open the file
	// and put it to the list.
	File * pFile = new File(FullName);
	if (NULL == pFile)
	{
		err.Set(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
	}

	do {
		DWORD access = GENERIC_READ;
		DWORD ShareMode = FILE_SHARE_READ;
		DWORD Disposition = OPEN_EXISTING;
		if (0 == (flags & CDirectFile::OpenReadOnly))
		{
			ShareMode = 0;
			access = GENERIC_WRITE | GENERIC_READ;

			if (flags & (CDirectFile::OpenDirect | CDirectFile::OpenExisting))
			{
				//ShareMode = 0;
				//access = GENERIC_WRITE | GENERIC_READ;
			}
			else if (flags & CDirectFile::CreateAlways)
			{
				//ShareMode = 0;
				//access = GENERIC_WRITE | GENERIC_READ;
				Disposition = CREATE_ALWAYS;
			}
			else if (flags & CDirectFile::CreateNew)
			{
				//ShareMode = 0;
				//access = GENERIC_WRITE | GENERIC_READ;
				Disposition = CREATE_NEW;
			}
			else
			{
				ASSERT(0);
			}
		}

		DWORD FileFlags = FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NORMAL;

		if (pFile->m_UseOverlappedIo)
		{
			FileFlags |= FILE_FLAG_OVERLAPPED;
		}

		hf = CreateFile(FullName,
						access,
						ShareMode,
						NULL, // lpSecurity
						Disposition,
						FileFlags,
						NULL);
		if (INVALID_HANDLE_VALUE == hf)
		{
			if ((::GetLastError() == ERROR_SHARING_VIOLATION
					|| ::GetLastError() == ERROR_ACCESS_DENIED)
				&& (flags & CDirectFile::OpenExisting)
				&& (flags & CDirectFile::OpenAllowReadOnlyFallback))
			{
				flags &= ~(CDirectFile::OpenExisting | CDirectFile::OpenAllowReadOnlyFallback);
				flags |= CDirectFile::OpenReadOnly;
				continue;
			}
			pFile->Close(0); // delete
			return NULL;
		}
	}
	while (hf == INVALID_HANDLE_VALUE);

	pFile->hFile = hf;

	if (FILE_TYPE_DISK != GetFileType(hf))
	{
		err.Set(ERROR_INVALID_FUNCTION);
		pFile->Close(0); // delete
		return NULL;
	}

	pFile->LoadFileInformation();

	pFile->FileLength = pFile->m_FileInfo.nFileSizeLow
						| (ULONGLONG(pFile->m_FileInfo.nFileSizeHigh) << 32);

	pFile->RealFileLength = pFile->FileLength;

	if (0 == (flags & CDirectFile::OpenReadOnly))
	{
		int WrittenMaskLength = 512 + FILE_LENGTH_TO_WRITTEN_MASK_LENGTH(pFile->FileLength);

		pFile->m_pWrittenMask = new char[WrittenMaskLength];
		if (NULL == pFile->m_pWrittenMask)
		{
			err.Set(ERROR_NOT_ENOUGH_MEMORY);
			pFile->Close(0); // delete
			return NULL;
		}

		pFile->WrittenMaskSize = WrittenMaskLength;
		memset(pFile->m_pWrittenMask, 0, WrittenMaskLength);
		size_t const LastByteOffset =
			FILE_OFFSET_TO_WRITTEN_MASK_OFFSET(pFile->FileLength +
												CACHE_BLOCK_SIZE - 1);

		memset(pFile->m_pWrittenMask, 0xFF, LastByteOffset);

		pFile->m_pWrittenMask[LastByteOffset] =
			char(0xFF >> (8 - (((pFile->FileLength + MASK_OFFSET_IN_BLOCK) >> BLOCK_SIZE_SHIFT) & 7)));
	}
	else
	{
		pFile->m_pWrittenMask = NULL;
		pFile->WrittenMaskSize = 0;
	}

	if ((flags & CDirectFile::OpenDeleteAfterClose)
		&& (flags & (CDirectFile::CreateNew | CDirectFile::CreateAlways)))
	{
		pFile->m_Flags |= CDirectFile::FileFlagsDeleteAfterClose;
	}
	if (flags & CDirectFile::OpenReadOnly)
	{
		pFile->m_Flags |= CDirectFile::FileFlagsReadOnly;
	}

	m_FileList.InsertHead(pFile);

	err.Set(ERROR_SUCCESS);
	return pFile;
}

BOOL File::SetSourceFile(File * pOriginalFile)
{
	ASSERT(0 == (m_Flags & CDirectFile::FileFlagsMemoryFile));
	if (m_Flags & CDirectFile::FileFlagsMemoryFile)
	{
		return FALSE;
	}
	if (pOriginalFile->pSourceFile != 0)
	{
		// double indirection is not allowed
		return FALSE;
	}

	pOriginalFile->RefCount++;

	if (pSourceFile != NULL)
	{
		pSourceFile->Close(0);
	}
	// zero m_pWrittenMask
	if (NULL != m_pWrittenMask)
	{
		memset(m_pWrittenMask, 0, WrittenMaskSize);
	}
	pSourceFile = pOriginalFile;
	if (NULL != pOriginalFile
		&& SetFileLength(pOriginalFile->FileLength))
	{
		UseSourceFileLength = pOriginalFile->FileLength;
	}
	return TRUE;
}

BOOL File::Commit(DWORD flags)
{
	if (m_Flags & CDirectFile::FileFlagsMemoryFile)
	{
		return TRUE;
	}
	if (m_Flags & CDirectFile::FileFlagsReadOnly)
	{
		return FALSE;
	}
	// close the file, set length and reopen again if necessary
	if (flags & CDirectFile::CommitFileFlushBuffers)
	{
		InitializeTheRestOfFile();

		if (NULL != pSourceFile)
		{
			pSourceFile->Close(0);
			pSourceFile = NULL;
		}

		VERIFY(Flush());

	}
	if (FileLength != RealFileLength)
	{
		CSimpleCriticalSectionLock lock(m_FileLock);

		CloseHandle(hFile);
		hFile = NULL;

		HANDLE hf = CreateFile(m_FileName,
								GENERIC_WRITE,
								0,
								NULL, // lpSecurity
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL,
								NULL);

		long MoveHigh = long(FileLength >> 32);

		SetLastError(ERROR_SUCCESS);

		if (hf != INVALID_HANDLE_VALUE)
		{
			SetFilePointer(hf, long(FileLength), & MoveHigh, FILE_BEGIN);
			if (::GetLastError() == ERROR_SUCCESS)
			{
				SetEndOfFile(hf);
			}
			CloseHandle(hf);
		}
		if (0 == (flags & CDirectFile::CommitFileDontReopen))
		{
			hf = CreateFile(m_FileName,
							GENERIC_WRITE | GENERIC_READ,
							0,
							NULL, // lpSecurity
							OPEN_EXISTING,
							FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NORMAL,
							NULL);
			if (INVALID_HANDLE_VALUE == hf)
			{
				// couldnt reopen!!
				return FALSE;
			}
			hFile = hf;
			m_FilePointer = 0;

		}
		if (NULL != hFile)
		{
			TRACE("Refreshing file info after file commit, prev modif. time=0x%08X%08X, ",
				m_FileInfo.ftLastWriteTime.dwHighDateTime, m_FileInfo.ftLastWriteTime.dwLowDateTime);

			LoadFileInformation();

			TRACE("new modif. time=0x%08X%08X\n",
				m_FileInfo.ftLastWriteTime.dwHighDateTime, m_FileInfo.ftLastWriteTime.dwLowDateTime);
			RealFileLength = m_FileInfo.nFileSizeLow
							| (ULONGLONG(m_FileInfo.nFileSizeHigh << 32));
		}
	}
	return TRUE;
}

BOOL File::Rename(LPCTSTR NewName, DWORD flags)
{
	if (m_Flags & CDirectFile::FileFlagsMemoryFile)
	{
		return FALSE;
	}
	if (m_Flags & CDirectFile::FileFlagsReadOnly)
	{
		return FALSE;
	}
	// close the file, set length and reopen again if necessary
	CSimpleCriticalSectionLock lock(m_FileLock);
	if (FALSE == Commit(flags | CDirectFile::CommitFileDontReopen))
	{
		return FALSE;
	}
	if (NULL != hFile)
	{
		CloseHandle(hFile);
		hFile = NULL;
	}
	BOOL result = FALSE;
	if (MoveFile(m_FileName, NewName))
	{
		m_FileName = NewName;
		result = TRUE;
	}
	else
	{
		TRACE(_T("Couldn't rename the file from %s to %s, last error=%X\n"),
			LPCTSTR(m_FileName), LPCTSTR(NewName), ::GetLastError());
	}
	if (flags & CDirectFile::CommitFileDontReopen)
	{
		return result;
	}

	DWORD access = GENERIC_WRITE | GENERIC_READ;

	if (flags & CDirectFile::RenameFileOpenReadOnly)
	{
		access = GENERIC_READ;
		m_Flags |= CDirectFile::FileFlagsReadOnly;
	}

	HANDLE hf = CreateFile(m_FileName,
							access,
							0,
							NULL, // lpSecurity
							OPEN_EXISTING,
							FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NORMAL,
							NULL);

	if (INVALID_HANDLE_VALUE == hf)
	{
		// couldnt reopen!!
		TRACE("Couldn't reopen the file, last error=%X\n", ::GetLastError());
		return FALSE;
	}

	hFile = hf;
	m_FilePointer = 0;

	if (NULL != hFile)
	{
		LoadFileInformation();

		RealFileLength = m_FileInfo.nFileSizeLow
						| (ULONGLONG(m_FileInfo.nFileSizeHigh << 32));
	}
	return result;
}

BOOL File::Close(DWORD /*flags*/)
{
	// dereference File structure.
	// stop all background operations on the files

	// if the file in prefetch is ours, then wait for the operation to finish
	// last Close operation is always synchronous, there is no parallel activity

	CacheThreadLock CacheLock;
	// if this file was set to prefetch, cancel prefetch

	CSimpleCriticalSectionLock lock1(CacheInstance.m_cs);
	if (--RefCount > 0)
	{
		return TRUE;
	}

	CacheInstance.CancelPrefetch(this);

	if (TRACE_CLOSE) TRACE(_T("Closing file %s, flags=%X\n"), LPCTSTR(m_FileName), m_Flags);
	// If the use count is 0, copy all remaining data
	// from the source file or init the rest and flush all the buffers,
	// remove the structure from the list
	CacheInstance.m_FileList.RemoveEntry(this);

	if (0 == (m_Flags & CDirectFile::FileFlagsDeleteAfterClose))
	{
		InitializeTheRestOfFile();
	}
	if (NULL != pSourceFile)
	{
		pSourceFile->Close(0);
		pSourceFile = NULL;
	}

	// close the handle and delete the structure
	while ( ! BuffersList.IsEmpty())
	{
		BufferHeader * pBuf = BuffersList.First();
		ASSERT(0 == pBuf->LockCount);
		// something strange: buffer not released
		if (pBuf->LockCount != 0)
		{
			TRACE("File::Close: Buffer not released!\n");
			return FALSE;
		}

		if (0 != pBuf->DirtyMask)
		{
			if (0 == (m_Flags & CDirectFile::FileFlagsDeleteAfterClose))
			{
				FlushDirtyBuffers(pBuf);
			}
			else
			{
				DirtyBuffersCount--;
				pBuf->DirtyMask = 0;
			}
		}

		ASSERT(0 == pBuf->DirtyMask);
		BuffersList.RemoveEntryUnsafe(pBuf);

		BuffersCount--;

		CacheInstance.FreeBuffer(pBuf);
	}

	if (m_Flags & CDirectFile::FileFlagsDeleteAfterClose)
	{
		// in Windows NT, the file is filled with zeros before being closed
		// if the file wasn't written, CloseHandle may take a while
		// so we better reset file length
		if (NULL != hFile)
		{
			SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
			if (::GetLastError() == ERROR_SUCCESS)
			{
				SetEndOfFile(hFile);
			}
			CloseHandle(hFile);
			hFile = NULL;
		}
		DeleteFile(m_FileName);
	}
	else
	{
		if (NULL != hFile)
		{
			CloseHandle(hFile);
			hFile = NULL;
		}
		// set file length
		// if the file is open for writing, open it in normal mode,
		// set length, and close. When the file is open in direct mode,
		// we cannot set arbitrary file length
		if (0 == (m_Flags & CDirectFile::FileFlagsReadOnly))
		{
			if (0 == (m_Flags & CDirectFile::FileFlagsMemoryFile)
				&& FileLength != RealFileLength)
			{
				HANDLE hf = CreateFile(m_FileName,
										GENERIC_WRITE,
										0,
										NULL, // lpSecurity
										OPEN_EXISTING,
										FILE_ATTRIBUTE_NORMAL,
										NULL);
				long MoveHigh = long(FileLength >> 32);
				SetLastError(ERROR_SUCCESS);
				if (hf != INVALID_HANDLE_VALUE)
				{
					SetFilePointer(hf, long(FileLength), & MoveHigh, FILE_BEGIN);
					if (::GetLastError() == ERROR_SUCCESS)
					{
						SetEndOfFile(hf);
					}
					CloseHandle(hf);
				}
			}
		}
	}

	if (TRACE_CLOSE) TRACE(_T("Closed file %s\n"), LPCTSTR(m_FileName));
	delete this;

	return TRUE;
}

BOOL CDirectFile::Close(DWORD flags)
{
	if (NULL == m_pFile)
	{
		return FALSE;
	}
	m_pFile->Close(flags);
	m_pFile = NULL;
	return TRUE;
}

LONGLONG CDirectFile::Seek(LONGLONG position, int origin)
{
	switch (origin)
	{
	case FILE_BEGIN:
		if (position < 0)
		{
			return -1i64;
		}
		m_FilePointer = position;
		break;
	case FILE_CURRENT:
		position += m_FilePointer;
		if (position < 0)
		{
			return -1i64;
		}
		m_FilePointer = position;
		break;
	case FILE_END:
		position += GetLength();
		if (position < 0)
		{
			return -1i64;
		}
		m_FilePointer = position;
		break;
	default:
		return -1i64;
	}
	return m_FilePointer;
}

// read data ('count' bytes) from the current position to *buf
long CDirectFile::Read(void *buf, long count)
{
	long TotalRead = ReadAt(buf, count, m_FilePointer);
	m_FilePointer += TotalRead;

	return TotalRead;
}

// read data ('count' bytes) from the given 'position' to *buf
// if count < 0, read backward
long CDirectFile::ReadAt(void *buf, long count, LONGLONG Position)
{
	long TotalRead = 0;
	char * pDstBuf = (char *)buf;
	ASSERT(m_pFile);
	if (NULL == m_pFile)
	{
		return 0;
	}

	if (Position < 0
		|| Position >= m_pFile->FileLength)
	{
		// beyond end of file
		return 0;
	}

	if (count > 0)
	{
		if (count + Position > m_pFile->FileLength)
		{
			count = long(m_pFile->FileLength - Position);
		}

		while(count > 0)
		{
			void * pSrcBuf;
			long nRead = GetDataBuffer( & pSrcBuf, count, Position, GetBufferAndPrefetchNext);
			if (0 == nRead)
			{
				break;
			}
			memmove(pDstBuf, pSrcBuf, nRead);
			ReturnDataBuffer(pSrcBuf, nRead, 0);

			Position += nRead;
			TotalRead += nRead;
			pDstBuf += nRead;
			count -= nRead;
		}
	}
	else if (count < 0)
	{
		if (-count > Position)
		{
			count = -long(Position & 0x7FFFFFFF);
		}

		while(count < 0)
		{
			void * pSrcBuf;
			long nRead = GetDataBuffer( & pSrcBuf, count, Position, GetBufferAndPrefetchNext);
			if (0 == nRead)
			{
				break;
			}
			ASSERT(nRead < 0);

			pDstBuf += nRead;
			memmove(pDstBuf, nRead + (char *)pSrcBuf, -nRead);
			ReturnDataBuffer(pSrcBuf, nRead, 0);

			Position += nRead;
			TotalRead += nRead;
			count -= nRead;
		}
	}
	return TotalRead;
}

// write data ('count' bytes) from the current position to *buf
// if count < 0, write backward
long CDirectFile::Write(const void *pBuf, long count)
{
	long TotalWritten = WriteAt(pBuf, count, m_FilePointer);
	m_FilePointer += TotalWritten;
	return TotalWritten;
}

// write data ('count' bytes) at the given 'position' from *buf
// if count < 0, write backward
long CDirectFile::WriteAt(const void *buf, long count, LONGLONG Position)
{
	long TotalWritten = 0;
	char * pSrcBuf = (char *)buf;
	ASSERT(m_pFile);
	if (NULL == m_pFile)
	{
		return 0;
	}

	if (Position < 0)
	{
		// beyond end of file
		return 0;
	}

	void * pDstBuf;
	if (count > 0)
	{
		while(count > 0)
		{
			long nWritten = GetDataBuffer( & pDstBuf, count, Position, GetBufferWriteOnly);

			if (0 == nWritten)
			{
				break;
			}
			memmove(pDstBuf, pSrcBuf, nWritten);
			ReturnDataBuffer(pDstBuf, nWritten, ReturnBufferDirty);

			Position += nWritten;
			TotalWritten += nWritten;
			pSrcBuf += nWritten;
			count -= nWritten;
		}
		if (TotalWritten != 0
			&& Position > m_pFile->FileLength)
		{
			m_pFile->FileLength = Position;
		}
	}
	else
	{
		if (Position < -count)
		{
			count = -long(Position & 0x7FFFFFFF);
		}
		LONGLONG StartPosition = Position;
		while(count < 0)
		{
			long nWritten = GetDataBuffer( & pDstBuf, count, Position, GetBufferWriteOnly);

			if (0 == nWritten)
			{
				break;
			}
			ASSERT(nWritten < 0);
			pSrcBuf += nWritten;
			memmove(nWritten + (char*)pDstBuf, pSrcBuf, -nWritten);

			ReturnDataBuffer(pDstBuf, nWritten, ReturnBufferDirty);

			Position += nWritten;
			TotalWritten += nWritten;
			count -= nWritten;
		}
		if (TotalWritten != 0
			&& StartPosition > m_pFile->FileLength)
		{
			m_pFile->FileLength = StartPosition;
		}
	}
	return TotalWritten;
}

CDirectFileCache::CDirectFileCache()
	: m_hThread(NULL),
	m_hEvent(NULL),
	m_pHeaders(NULL),
	m_pBuffersArray(NULL),
	m_MRU_Count(1),
	m_NumberOfBuffers(0),
//m_MinPrefetchMRU(0),
	m_MaxBlocksToPrefetch(2),
	m_FlushRequest(0)
{
	if (0 == (GetVersion() & 0x80000000))
	{
//        UseOverlappedIo = TRUE;
	}
	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	// this is manual reset event
	m_hThreadSuspendedEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	// round up to 64K
}

void CDirectFileCache::InitCache(size_t MaxCacheSize)
{
	if (1 != ++m_InitCount)
	{
		return;
	}
	size_t CacheSize;
	MEMORYSTATUS st;
	GlobalMemoryStatus( & st);
	CacheSize = st.dwTotalPhys / 8;
	if (MaxCacheSize != 0
		&& CacheSize > MaxCacheSize)
	{
		CacheSize = MaxCacheSize;
	}

	TRACE("Direct file CacheSize = %d MB\n", CacheSize / 0x100000);
	CacheSize = (CacheSize + CACHE_BLOCK_SIZE - 1) & ~MASK_OFFSET_IN_BLOCK;
	m_pBuffersArray = VirtualAlloc(NULL, CacheSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	m_NumberOfBuffers = unsigned(CacheSize / CACHE_BLOCK_SIZE);

	m_pHeaders = new BufferHeader[m_NumberOfBuffers];

	unsigned i;
	for (i = 0; i != m_NumberOfBuffers; i++)
	{
		m_FreeBuffers.InsertHead( & m_pHeaders[i]);

		m_pHeaders[i].pBuf = i * CACHE_BLOCK_SIZE + (char *) m_pBuffersArray;
	}

	m_MaxBlocksToPrefetch = m_NumberOfBuffers / (MAX_PREFETCH_REGIONS + 1);

	for (i = 0; i != MAX_PREFETCH_REGIONS; i++)
	{
		m_ListPrefetched.InsertTail(new PrefetchDescriptor);
	}

	unsigned ThreadID = NULL;

	m_hThread = (HANDLE) _beginthreadex(NULL, 0x10000, ThreadProc,
										this, CREATE_SUSPENDED, & ThreadID);
	if (m_hThread != NULL)
	{
		if (m_hEvent != NULL
			&& m_hThreadSuspendedEvent != NULL
			&& m_pBuffersArray != NULL
			&& m_pHeaders != NULL)
		{
			SetThreadPriority(m_hThread, THREAD_PRIORITY_ABOVE_NORMAL);
			::ResumeThread(m_hThread);
		}
		else
		{
			TerminateThread(m_hThread, 0xFF);
			CloseHandle(m_hThread);
			m_hThread = NULL;
		}
	}
}

CDirectFileCache::~CDirectFileCache()
{
	if (NULL != m_hEvent)
	{
		CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}
	if (NULL != m_hThreadSuspendedEvent)
	{
		CloseHandle(m_hThreadSuspendedEvent);
		m_hThreadSuspendedEvent = NULL;
	}
}

void CDirectFileCache::DeInitCache()
{
	if (0 != --m_InitCount)
	{
		return;
	}
	// stop the thread
	if (NULL != m_hThread)
	{
		m_ThreadRunState = ~0;
#ifdef _DEBUG
		DWORD Time = timeGetTime();
#endif
		SetEvent(m_hEvent);
		if (WAIT_TIMEOUT == WaitForSingleObjectAcceptSends(m_hThread, 20000))
		{
			TerminateThread(m_hThread, 0xFF);
		}
#ifdef _DEBUG
		TRACE("File Cache Thread finished in %d ms\n",
			timeGetTime() - Time);
#endif
		CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	// close the files
	// free the buffers
	if (NULL != m_pBuffersArray)
	{
		VirtualFree(m_pBuffersArray, 0, MEM_DECOMMIT | MEM_RELEASE);
		m_pBuffersArray = NULL;
		m_NumberOfBuffers = 0;
	}

	while ( ! m_FreeBuffers.IsEmpty())
	{
		m_FreeBuffers.RemoveHead();
	}

	while ( ! m_ListPrefetched.IsEmpty())
	{
		delete m_ListPrefetched.RemoveTail();
	}

	if (NULL != m_pHeaders)
	{
		delete[] m_pHeaders;
		m_pHeaders = NULL;
	}

}

// get a buffer header by its buffer addr
BufferHeader * CDirectFileCache::GetBufferHeader(void * pAddr)
{
	unsigned BufferNumber = unsigned((PUCHAR(pAddr) - PUCHAR(m_pBuffersArray)) / CACHE_BLOCK_SIZE);
	ASSERT(BufferNumber >= 0 && BufferNumber < m_NumberOfBuffers);

	BufferHeader * pBuf = & m_pHeaders[BufferNumber];

	ASSERT(pBuf->LockCount > 0);
	ASSERT(((PUCHAR(pAddr) - PUCHAR(m_pBuffersArray)) & ~MASK_OFFSET_IN_BLOCK)
			== ((PUCHAR(pBuf->pBuf) - PUCHAR(m_pBuffersArray)) & ~MASK_OFFSET_IN_BLOCK));

	return pBuf;
}

inline unsigned CDirectFileCache::GetOffsetInBuffer(void * pAddr)
{
	return unsigned((PUCHAR(pAddr) - PUCHAR(m_pBuffersArray)) % CACHE_BLOCK_SIZE);
}

void CDirectFileCache::MakeBufferLeastRecent(BufferHeader * pBuf)
{
	// move the buffer to MRU tail
	// extract the buffer from MRU list
	CSimpleCriticalSectionLock lock(m_cs);

	m_MruList.RemoveEntry(pBuf);
	// insert the buffer to the list tail
	// to do: insert the buffer before the first with MRU_Count==1
	pBuf->MRU_Count = 1;

	BufferMruEntry * pBufAfter = m_MruList.Last();
	while (m_MruList.NotEnd(pBufAfter)
			&& 1 == pBufAfter->MRU_Count)
	{
		pBufAfter = m_MruList.Prev(pBufAfter);
	}

	pBufAfter->InsertAsNextItem(pBuf);
}

// Insert a buffer to the list with the specified key
// m_FileLock should be locked around this function call
void File::InsertBuffer(BufferHeader * pBuf)
{
	BLOCK_INDEX const key = pBuf->PositionKey;
	BuffersCount++;
	if (BuffersList.IsEmpty()
		|| BuffersList.First()->PositionKey > key)
	{
		BuffersList.InsertHead(pBuf);
		// insert to the empty list
		return;
	}
	else if (BuffersList.Last()->PositionKey < key)
	{
		// insert to the tail
		BuffersList.InsertTail(pBuf);
		return;
	}

	// find which side may be closer to the key
	BufferHeader * pBufAfter;
	if (key - BuffersList.First()->PositionKey > BuffersList.Last()->PositionKey - key)
	{
		// search from tail
		pBufAfter = BuffersList.Last();
		while (pBufAfter->PositionKey > key)
		{
			pBufAfter = BuffersList.Prev(pBufAfter);
		}
	}
	else
	{
		// search from head
		pBufAfter = BuffersList.First();
		while (pBufAfter->PositionKey < key)
		{
			pBufAfter = BuffersList.Next(pBufAfter);
			ASSERT(pBufAfter != NULL);
		}
		pBufAfter = BuffersList.Prev(pBufAfter);
		ASSERT(pBufAfter != NULL);
	}

	// pBufAfter->PositionKey < pBuf->PositionKey
	pBufAfter->ListItem<BufferHeader>::InsertAsNextItem(pBuf);

	ValidateList();
}

// find a buffer in the list with the specified key
// m_FileLock should be locked around this function call
BufferHeader * File::FindBuffer(BLOCK_INDEX key) const
{
	// the buffers are ordered: BuffersListHead has lowest key, BuffersListTail has the highest key
	BufferHeader * pBuf;
	ValidateList();

	if (BuffersList.IsEmpty()
		|| BuffersList.First()->PositionKey > key
		|| BuffersList.Last()->PositionKey < key)
	{
		return NULL;
	}
	// find which side may be closer to the key
	if (key - BuffersList.First()->PositionKey > BuffersList.Last()->PositionKey - key)
	{
		// search from tail
		for (pBuf = BuffersList.Last();
			BuffersList.NotEnd(pBuf) && pBuf->PositionKey >= key;
			pBuf = BuffersList.Prev(pBuf))
		{
			if (pBuf->PositionKey == key)
			{
				return pBuf;
			}
		}
	}
	else
	{
		// search from head

		for (pBuf = BuffersList.First();
			BuffersList.NotEnd(pBuf) && pBuf->PositionKey <= key;
			pBuf = BuffersList.Next(pBuf))
		{
			if (pBuf->PositionKey == key)
			{
				return pBuf;
			}
		}
	}
	return NULL;    // buffer not found
}

long CDirectFileCache::GetDataBuffer(File * pFile,
									void * * ppBuf, LONGLONG long_length, LONGLONG position, DWORD flags, unsigned MaxMRU)
{
	ASSERT(pFile);
	ASSERT(ppBuf);

	if (NULL == pFile)
	{
		return 0;
	}

	long BytesRequested = 0;
	long BytesReturned = 0;
	int OffsetInBuffer = 0;

	if (pFile->m_Flags & CDirectFile::FileFlagsMemoryFile)
	{
		if (NULL == pFile->m_pMemoryFileBuffer)
		{
			return 0;
		}
		*ppBuf = NULL;
		if (long_length > 0)
		{
			if (position >= pFile->m_MemoryFileBufferSize)
			{
				return 0;
			}
			long MaxLength = long(pFile->m_MemoryFileBufferSize - position);

			if (long_length > MaxLength)
			{
				long_length = MaxLength;
			}
		}
		else if (long_length < 0)
		{
			if (position > pFile->m_MemoryFileBufferSize)
			{
				return 0;
			}
			if (-long_length > position)
			{
				long_length = -position;
			}
		}
		else
		{
			return 0;
		}
		*ppBuf = pFile->m_pMemoryFileBuffer + DWORD(position);
		pFile->m_MemoryBufferRefCount++;
		return long(long_length);
	}

	BLOCK_INDEX BufferPosition;
	long length;

	if (long_length > 0)
	{
		if (long_length > (m_MaxBlocksToPrefetch << BLOCK_SIZE_SHIFT))
		{
			length = m_MaxBlocksToPrefetch << BLOCK_SIZE_SHIFT;
		}
		else
		{
			length = long(long_length);
		}

		BufferPosition = FILE_OFFSET_TO_BLOCK_NUMBER(position);
		OffsetInBuffer = long(position) & MASK_OFFSET_IN_BLOCK;

		if (OffsetInBuffer + length > CACHE_BLOCK_SIZE)
		{
			BytesRequested = CACHE_BLOCK_SIZE - OffsetInBuffer;
			BytesReturned = BytesRequested;
		}
		else
		{
			BytesRequested = length;
			BytesReturned = BytesRequested;
			if (flags & CDirectFile::GetBufferAndPrefetchNext)
			{
				// read the rest of the buffer
				BytesRequested = CACHE_BLOCK_SIZE - OffsetInBuffer;
			}
		}
	}
	else if (long_length < 0)
	{
		if (-long_length > position)
		{
			long_length = -position;
			if (0 == long_length)
			{
				*ppBuf = NULL;
				return 0;
			}
		}

		if (long_length < -long(m_MaxBlocksToPrefetch << BLOCK_SIZE_SHIFT))
		{
			length = -long(m_MaxBlocksToPrefetch << BLOCK_SIZE_SHIFT);
		}
		else
		{
			length = long(long_length);
		}

		OffsetInBuffer = CACHE_BLOCK_SIZE - ((-long(position)) & MASK_OFFSET_IN_BLOCK);
		if (OffsetInBuffer + length <= 0)    // length < 0
		{
			BytesRequested = OffsetInBuffer;
			BytesReturned = BytesRequested;
		}
		else
		{
			BytesRequested = -length;
			BytesReturned = BytesRequested;
			if (flags & CDirectFile::GetBufferAndPrefetchNext)
			{
				// read the rest of the buffer
				BytesRequested = OffsetInBuffer;
			}
		}

		BufferPosition = BLOCK_INDEX((position - BytesRequested) >> BLOCK_SIZE_SHIFT);
		OffsetInBuffer = (BLOCK_INDEX(position) - BytesRequested) & MASK_OFFSET_IN_BLOCK;
	}
	else
	{
		*ppBuf = NULL;
		return 0;
	}

	DWORD RequestedMask = (0xFFFFFFFFu << (OffsetInBuffer >> SUBBLOCK_SIZE_SHIFT));
	if (OffsetInBuffer + BytesRequested <= (CACHE_BLOCK_SIZE - CACHE_SUBBLOCK_SIZE))
	{
		RequestedMask &= ~(0xFFFFFFFFu << ((OffsetInBuffer + BytesRequested + MASK_OFFSET_IN_SUBBLOCK) >> SUBBLOCK_SIZE_SHIFT));
	}

	DWORD MaskToRead = RequestedMask;
	if (flags & CDirectFile::GetBufferWriteOnly)
	{
		MaskToRead = ~RequestedMask;
		if (OffsetInBuffer & MASK_OFFSET_IN_SUBBLOCK)
		{
			MaskToRead |= (1L << (OffsetInBuffer >> SUBBLOCK_SIZE_SHIFT));
		}
		if ((OffsetInBuffer + BytesRequested) & MASK_OFFSET_IN_SUBBLOCK)
		{
			MaskToRead |= (1L << ((OffsetInBuffer + BytesRequested) >> SUBBLOCK_SIZE_SHIFT));
		}
	}

	BufferHeader * pBuf = NULL;
	BufferHeader * pFreeBuf = NULL;

	{
		if (0 == ++m_MRU_Count)
		{
			CSimpleCriticalSectionLock lock(m_cs);
			// change it to 0x80000000, subtract 0x80000000 from all counters
			for (unsigned i = 0; i < m_NumberOfBuffers; i++)
			{
				if (m_pHeaders[i].MRU_Count > 0x80000000u)
				{
					m_pHeaders[i].MRU_Count -= 0x80000000u;
				}
				else
				{
					m_pHeaders[i].MRU_Count = 0;
				}
			}
			m_MRU_Count = 0x80000000u;
		}
	}

	{
		CSimpleCriticalSectionLock lock(pFile->BuffersList);
		pBuf = pFile->FindBuffer(BufferPosition);
		if (NULL != pBuf)
		{
			pBuf->LockCount++;
			pBuf->MRU_Count = m_MRU_Count;
		}
	}

	if (NULL == pBuf)
	{
		// the buffer is not found, get another free one
		pFreeBuf = GetFreeBuffer(MaxMRU);
		if (NULL == pFreeBuf)
		{
			TRACE("unable to find an available  buffer\n");
			*ppBuf = NULL;
			return 0;
		}
		CSimpleCriticalSectionLock lock(pFile->BuffersList);
		// make sure the buffer with the same position was not added
		// while we might flush the buffers
		pBuf = pFile->FindBuffer(BufferPosition);

		if (NULL != pBuf)
		{
			pBuf->LockCount++;
		}
		else
		{
			pBuf = pFreeBuf;
			pFreeBuf = NULL;

			pBuf->LockCount = 1;
			pBuf->ReadMask = 0;
			pBuf->DirtyMask = 0;
			pBuf->pFile = pFile;
			pBuf->PositionKey = BufferPosition;
			// insert the buffer to the list for file,
			// to the correct position
			pFile->InsertBuffer(pBuf);
		}
		pBuf->MRU_Count = m_MRU_Count;
	}
	{
		// extract the buffer from MRU list
		CSimpleCriticalSectionLock lock(m_cs);

		m_MruList.RemoveEntry(pBuf);
		// since we already have the required buffer,
		// return the free buffer back
		if (NULL != pFreeBuf)
		{
			m_FreeBuffers.InsertHead(pFreeBuf);
		}
		// insert the buffer to the list head
		m_MruList.InsertHead(pBuf);
	}

	if (MaskToRead != (pBuf->ReadMask & MaskToRead)
		|| (flags & CDirectFile::GetBufferWriteOnly))
	{
		pFile->ReadDataBuffer(pBuf, MaskToRead);
	}
	// start prefetch for the rest of the data, if GetBufferWriteOnly is not set
	// return number of bytes available

	if (length >= 0)
	{
		*ppBuf = OffsetInBuffer + (char *)pBuf->pBuf;
		if (0 == (flags & (CDirectFile::GetBufferNoPrefetch | CDirectFile::GetBufferWriteOnly))
			&& (BytesRequested < length || (flags & CDirectFile::GetBufferAndPrefetchNext)))
		{
			if (flags & CDirectFile::GetBufferAndPrefetchNext)
			{
				if (0 || length <= BytesRequested) // ??? BUGBUG
				{
					// prefetch the next buffer
					length = (long(position) & ~MASK_OFFSET_IN_BLOCK) + (CACHE_BLOCK_SIZE*2) - long(position);
				}
				else
				{
					// round the length to make it prefetch the whole buffer
					length += MASK_OFFSET_IN_BLOCK & -(long(position) + length);
				}
			}
			RequestPrefetch(pFile, position + BytesRequested, length - BytesRequested, m_MRU_Count);
		}
		ASSERT(BytesReturned <= length);
		return BytesReturned;
	}
	else
	{
		* ppBuf = OffsetInBuffer + BytesRequested + (char *)pBuf->pBuf;
		if (0 == (flags & (CDirectFile::GetBufferNoPrefetch | CDirectFile::GetBufferWriteOnly))
			&& (BytesRequested < -length || (flags & CDirectFile::GetBufferAndPrefetchNext)))
		{
			if (flags & CDirectFile::GetBufferAndPrefetchNext)
			{
				if ( 0 || -length <= BytesRequested)
				{
					// prefetch the next buffer
					length = (long(position) | MASK_OFFSET_IN_BLOCK)  - (CACHE_BLOCK_SIZE*2 - 1) - long(position);
				}
				else
				{
					// round the length to make it prefetch the whole buffer
					length -= MASK_OFFSET_IN_BLOCK & (long(position) - length);
				}
			}
			RequestPrefetch(pFile, position - BytesRequested, length + BytesRequested, m_MRU_Count);
		}
		ASSERT(BytesReturned <= -length);
		return -BytesReturned;
	}
}

void CDirectFileCache::CancelPrefetch(File const * pFile)
{
	// the thread must be in waiting state
	ASSERT(m_ThreadRunState > 0);
	if (TRACE_PREFETCH) TRACE("Cancel prefetch for file %X\n", pFile->hFile);

	CSimpleCriticalSectionLock lock(m_cs);
	PrefetchDescriptor * pDesc;

	for (pDesc = m_ListPrefetched.First(); m_ListPrefetched.NotEnd(pDesc);
		pDesc = m_ListPrefetched.Next(pDesc))
	{
		if (pDesc->m_pFile == pFile)
		{
			pDesc->m_pFile = NULL;
		}
	}

	for (pDesc = m_ListToPrefetch.First(); m_ListToPrefetch.NotEnd(pDesc); )
	{
		PrefetchDescriptor * next = m_ListToPrefetch.Next(pDesc);

		if (pDesc->m_pFile == pFile)
		{
			m_ListToPrefetch.RemoveEntry(pDesc);
			pDesc->m_pFile = NULL;
			m_ListPrefetched.InsertHead(pDesc);
		}
		pDesc = next;
	}
}

// for prefetching forward:
// find an existing prefetch item.
// If the prefetch begin falls into an active item, just return.
// If the prefetch falls into a pending item, adjust its beginning
// If the prefetch begin falls into a done item, reactivate it if reached middle of the range.
CString DumpQueue(ListHead<PrefetchDescriptor> & list, File * pFile)
{
	CString s, s1;
	for (PrefetchDescriptor * pDesc = list.First(); list.NotEnd(pDesc);
		pDesc = list.Next(pDesc))
	{
		if (pDesc->m_pFile != pFile)
		{
			continue;
		}
		s1.Format(_T("(%x %x %x) "),
				pDesc->m_PrefetchedBeginBlock,
				pDesc->m_PrefetchedEndBlock,
				pDesc->m_PrefetchPosition);
		s += s1;
	}
	return s;
}

void CDirectFileCache::RequestPrefetch(File * pFile,
										LONGLONG PrefetchPosition,
										LONGLONG PrefetchLength, unsigned /*MaxMRU*/)
{
	BLOCK_INDEX PrefetchBlockBegin;
	BLOCK_INDEX PrefetchBlockEnd;

	if (PrefetchLength >= 0)
	{
		// the range should not go outside the file length
		if (PrefetchPosition + PrefetchLength > pFile->FileLength)
		{
			PrefetchLength = pFile->FileLength - PrefetchPosition;
		}
		if (PrefetchLength <= 0)
		{
			return;
		}
		// FIRST block to prefetch
		PrefetchBlockBegin = FILE_OFFSET_TO_BLOCK_NUMBER(PrefetchPosition);
		// LAST block to prefetch
		PrefetchBlockEnd = FILE_OFFSET_TO_BLOCK_NUMBER(
														PrefetchPosition + PrefetchLength + (CACHE_BLOCK_SIZE - 1)) - 1;

		if (PrefetchBlockEnd - PrefetchBlockBegin >= m_MaxBlocksToPrefetch)
		{
			PrefetchBlockEnd = PrefetchBlockBegin + m_MaxBlocksToPrefetch - 1;
		}
	}
	else
	{
		if (PrefetchPosition + PrefetchLength < 0)
		{
			PrefetchLength = -PrefetchPosition;
		}
		if (PrefetchPosition > pFile->FileLength)
		{
			PrefetchLength += pFile->FileLength - PrefetchPosition;
		}
		if (PrefetchLength >= 0)
		{
			return;
		}
		// FIRST block to prefetch
		PrefetchBlockBegin = FILE_OFFSET_TO_BLOCK_NUMBER(PrefetchPosition - 1);
		// LAST block to prefetch
		PrefetchBlockEnd = FILE_OFFSET_TO_BLOCK_NUMBER(
														PrefetchPosition + PrefetchLength);

		if (PrefetchBlockBegin - PrefetchBlockEnd >= m_MaxBlocksToPrefetch)
		{
			PrefetchBlockEnd = PrefetchBlockBegin - (m_MaxBlocksToPrefetch - 1);
		}
	}


	{
		CSimpleCriticalSectionLock lock(m_cs);

		if (TRACE_PREFETCH) TRACE(_T("RequestPrefetch for file %x: block 0x%x to 0x%x, Q1: %s, Q2: %s\n"),
								pFile->hFile, PrefetchBlockBegin, PrefetchBlockEnd,
								LPCTSTR(DumpQueue(m_ListToPrefetch, pFile)),
								LPCTSTR(DumpQueue(m_ListPrefetched, pFile)));
		// find a prefetched range which falls into
		PrefetchDescriptor * pDesc;

		for (pDesc = m_ListToPrefetch.First(); m_ListToPrefetch.NotEnd(pDesc);
			pDesc = m_ListToPrefetch.Next(pDesc))
		{
			if (pDesc->m_pFile != pFile)
			{
				continue;
			}

			if (pDesc->m_PrefetchedEndBlock >= pDesc->m_PrefetchedBeginBlock
				&& PrefetchBlockEnd >= PrefetchBlockBegin)
			{
				// non-empty "forward" prefetch block
				// chehck if the first block falls into the present range
				if (PrefetchBlockBegin >= pDesc->m_PrefetchedBeginBlock
					&& PrefetchBlockBegin <= pDesc->m_PrefetchedEndBlock + 1)
				{
					// if it's not _first_ item in the queue, adjust the begin pointers
					if (m_ListToPrefetch.First() != pDesc)
					{
						pDesc->m_PrefetchedBeginBlock = PrefetchBlockBegin;
						pDesc->m_PrefetchPosition = PrefetchBlockBegin;
					}
					// allow extending the range
					if (pDesc->m_PrefetchedEndBlock < PrefetchBlockEnd)
					{
						pDesc->m_PrefetchedEndBlock = PrefetchBlockEnd;
						// limit the range with m_MaxBlocksToPrefetch
						if (pDesc->m_PrefetchedEndBlock
							- pDesc->m_PrefetchedBeginBlock
							> m_MaxBlocksToPrefetch - 1)
						{
							pDesc->m_PrefetchedEndBlock =
								pDesc->m_PrefetchedBeginBlock + m_MaxBlocksToPrefetch - 1;
						}
					}
					if (TRACE_PREFETCH) TRACE("Item modified: %x: block 0x%x to 0x%x, current %x\n",
											pFile->hFile, pDesc->m_PrefetchedBeginBlock, pDesc->m_PrefetchedEndBlock,
											pDesc->m_PrefetchPosition);
					return;
				}
			}
			else if (pDesc->m_PrefetchedEndBlock <= pDesc->m_PrefetchedBeginBlock
					&& PrefetchBlockEnd <= PrefetchBlockBegin)
			{
				// non-empty "backward" prefetch block
				if (PrefetchBlockBegin <= pDesc->m_PrefetchedBeginBlock
					&& PrefetchBlockBegin + 1 >= pDesc->m_PrefetchedEndBlock)
				{
					if (m_ListToPrefetch.First() != pDesc)
					{
						pDesc->m_PrefetchedBeginBlock = PrefetchBlockBegin;
						pDesc->m_PrefetchPosition = PrefetchBlockBegin;
					}

					// allow extending the range
					if (pDesc->m_PrefetchedEndBlock > PrefetchBlockEnd)
					{
						pDesc->m_PrefetchedEndBlock = PrefetchBlockEnd;
						// limit the range with m_MaxBlocksToPrefetch
						if (pDesc->m_PrefetchedBeginBlock
							- pDesc->m_PrefetchedEndBlock
							> m_MaxBlocksToPrefetch - 1)
						{
							pDesc->m_PrefetchedEndBlock =
								pDesc->m_PrefetchedBeginBlock - (m_MaxBlocksToPrefetch - 1);
						}
					}
					if (TRACE_PREFETCH) TRACE("Item modified: %x: block %x to %x, current %x\n",
											pFile->hFile, pDesc->m_PrefetchedBeginBlock, pDesc->m_PrefetchedEndBlock,
											pDesc->m_PrefetchPosition);
					return;
				}
			}
		}

		PrefetchDescriptor * pUseDesc = NULL;
		// now check "done" list
		for (pDesc = m_ListPrefetched.First(); m_ListPrefetched.NotEnd(pDesc);
			pDesc = m_ListPrefetched.Next(pDesc))
		{
			if (pDesc->m_pFile != pFile)
			{
				continue;
			}

			if (pDesc->m_PrefetchedBeginBlock <= pDesc->m_PrefetchedEndBlock
				&& PrefetchBlockBegin <= PrefetchBlockEnd)
			{
				if (PrefetchBlockBegin >= pDesc->m_PrefetchedBeginBlock
					// extend the "done" range, if the new block begins just after the existing item
					&& PrefetchBlockBegin <= pDesc->m_PrefetchedEndBlock + 1)
				{
					// begin is in inside range
					if (PrefetchBlockBegin < (pDesc->m_PrefetchedBeginBlock + pDesc->m_PrefetchedEndBlock) / 2
						|| PrefetchBlockEnd <= pDesc->m_PrefetchedEndBlock)
					{
						// too early or not necessary to start another prefetch
						return;
					}

					pDesc->m_PrefetchPosition = pDesc->m_PrefetchedEndBlock + 1;
					pDesc->m_PrefetchedEndBlock = PrefetchBlockEnd;

					// limit the _back_ range with m_MaxBlocksToPrefetch
					if (pDesc->m_PrefetchedEndBlock
						- pDesc->m_PrefetchedBeginBlock
						> m_MaxBlocksToPrefetch - 1)
					{
						pDesc->m_PrefetchedBeginBlock =
							pDesc->m_PrefetchedEndBlock - (m_MaxBlocksToPrefetch - 1);
					}

					m_ListPrefetched.RemoveEntry(pDesc);
					pUseDesc = pDesc;
					break;
				}
			}

			if (pDesc->m_PrefetchedBeginBlock >= pDesc->m_PrefetchedEndBlock
				&& PrefetchBlockBegin >= PrefetchBlockEnd)
			{
				if (PrefetchBlockBegin <= pDesc->m_PrefetchedBeginBlock
					&& PrefetchBlockBegin + 1 >= pDesc->m_PrefetchedEndBlock)
				{
					// begin is in inside range
					if (PrefetchBlockBegin > (pDesc->m_PrefetchedBeginBlock + pDesc->m_PrefetchedEndBlock) / 2
						|| PrefetchBlockEnd >= pDesc->m_PrefetchedEndBlock)
					{
						// too early or not necessary to start another prefetch
						return;
					}

					pDesc->m_PrefetchPosition = pDesc->m_PrefetchedEndBlock - 1;
					pDesc->m_PrefetchedEndBlock = PrefetchBlockEnd;

					// limit the _back_ range with m_MaxBlocksToPrefetch
					if (pDesc->m_PrefetchedBeginBlock
						- pDesc->m_PrefetchedEndBlock
						> m_MaxBlocksToPrefetch - 1)
					{
						pDesc->m_PrefetchedBeginBlock =
							pDesc->m_PrefetchedEndBlock + (m_MaxBlocksToPrefetch - 1);
					}
					m_ListPrefetched.RemoveEntry(pDesc);
					pUseDesc = pDesc;
					break;
				}
			}
		}

		if (NULL == pUseDesc)
		{
			if (m_ListPrefetched.IsEmpty())
			{
				return;
			}

			pUseDesc = m_ListPrefetched.RemoveHead();
			pUseDesc->m_PrefetchPosition = PrefetchBlockBegin;
			pUseDesc->m_PrefetchedBeginBlock = PrefetchBlockBegin;
			pUseDesc->m_PrefetchedEndBlock = PrefetchBlockEnd;
		}
		pUseDesc->m_pFile = pFile;


		m_ListToPrefetch.InsertTail(pUseDesc);
		if (TRACE_PREFETCH) TRACE("Item inserted: %x: block %x to %x, current %x\n",
								pFile->hFile, pUseDesc->m_PrefetchedBeginBlock, pUseDesc->m_PrefetchedEndBlock,
								pUseDesc->m_PrefetchPosition);

//        m_MinPrefetchMRU = MaxMRU;
	}
	SetEvent(m_hEvent);
}

bool File::RequestFlush(LONGLONG FlushPosition, LONG FlushLength)
{
	// flush only whole buffers
	if (0 == m_FlushLength)
	{
		m_FlushBegin = FlushPosition;
		m_FlushLength = FlushLength;
	}
	else
	{
		if (FlushPosition == m_FlushBegin + m_FlushLength)
		{
			m_FlushLength += FlushLength;
		}
		else
		{
			if (m_FlushBegin > FlushPosition)
			{
				m_FlushLength += LONG(m_FlushBegin) - LONG(FlushPosition);
				m_FlushBegin = FlushPosition;
			}
			if (m_FlushBegin + m_FlushLength < FlushPosition + FlushLength)
			{
				m_FlushLength = LONG(FlushPosition) - LONG(m_FlushBegin) + FlushLength;
			}
		}
	}
	// if flush boundary ends or crosses 64 boundary, request flush
	return LONG(m_FlushBegin & MASK_OFFSET_IN_BLOCK) + m_FlushLength >= CACHE_BLOCK_SIZE;
}

void CDirectFileCache::FlushRequestedFiles()
{
	// file cannot be removed during this call

	for (File * pFile = m_FileList.First(); m_FileList.NotEnd(pFile);
		pFile = m_FileList.Next(pFile))
	{
		pFile->FlushRequestedRange();
	}
}

void File::ReturnDataBuffer(void * pBuffer, long count, DWORD flags)
{
	if (NULL == pBuffer || 0 == count)
	{
		return;
	}
	if (m_Flags & CDirectFile::FileFlagsMemoryFile)
	{
		m_MemoryBufferRefCount--;
		return;
	}
	// find the buffer descriptor
	PUCHAR pucBuffer = PUCHAR(pBuffer);

	if (count < 0)
	{
		pucBuffer += count;
		count = -count;
	}

	BufferHeader * pBuf = CacheInstance.GetBufferHeader(pucBuffer);

	ASSERT(pBuf->pFile == this);

	// mark the buffer dirty, if necessary
	int OffsetInBuffer = CacheInstance.GetOffsetInBuffer(pucBuffer);

	LONGLONG FileOffset = OffsetInBuffer + (LONGLONG(pBuf->PositionKey) << BLOCK_SIZE_SHIFT);

	ASSERT(OffsetInBuffer + count <= CACHE_BLOCK_SIZE);

	{
		CSimpleCriticalSectionLock lock(BuffersList);
		if (flags & CDirectFile::ReturnBufferDirty)
		{
			DWORD RequestedMask = (0xFFFFFFFFu << (OffsetInBuffer >> SUBBLOCK_SIZE_SHIFT));
			if (OffsetInBuffer + count <= CACHE_BLOCK_SIZE - CACHE_SUBBLOCK_SIZE)
			{
				RequestedMask &= ~(0xFFFFFFFFu << ((OffsetInBuffer + count + CACHE_SUBBLOCK_SIZE - 1) >> SUBBLOCK_SIZE_SHIFT));
			}

			DWORD OldDirtyMask = pBuf->DirtyMask.Exchange_Or(RequestedMask);

			pBuf->ReadMask |= RequestedMask;
			if (0 == OldDirtyMask && 0 != RequestedMask)
			{
				DirtyBuffersCount++;
			}
		}
	}

	if (0 == ((count + OffsetInBuffer) & MASK_OFFSET_IN_BLOCK)
		&& (flags & CDirectFile::ReturnBufferDiscard))
	{
		CacheInstance.MakeBufferLeastRecent(pBuf);
	}

	// decrement lock count
	if (0 == --pBuf->LockCount)
	{
		// set a request for writing
		if (flags & CDirectFile::ReturnBufferFlush)
		{
			bool result;
			if (count >= 0)
			{
				result = RequestFlush(FileOffset, count);
			}
			else
			{
				result = RequestFlush(FileOffset + count, -count);
			}

			if (result)
			{
				CacheInstance.SetFlushRequest();
			}
		}
	}
}

void CDirectFileCache::FreeBuffer(BufferHeader * pBuf)
{
	CSimpleCriticalSectionLock lock(m_cs); // already locked
	m_MruList.RemoveEntry(pBuf);

	pBuf->pFile = NULL;

	m_FreeBuffers.InsertHead(pBuf);
}

BufferHeader * CDirectFileCache::GetFreeBuffer(unsigned MaxMRU)
{
	while (1)
	{
		CSimpleCriticalSectionLock lock(m_cs);
		{
			BufferHeader * pBuf = NULL;
			// try to find an empty buffer
			if ( ! m_FreeBuffers.IsEmpty())
			{
				pBuf = m_FreeBuffers.RemoveHead();
				pBuf->ReadMask = 0;
				pBuf->DirtyMask = 0;
				return pBuf;
			}
		}

		// find an unlocked buffer with oldest MRU stamp
		if (0 == MaxMRU)
		{
			MaxMRU = m_MRU_Count;
		}
		BufferMruEntry * pMru = m_MruList.Last();
		// find least recent unlocked and non-dirty buffer (with lowest MRU)
		// whose MRU is at most MaxMRU.
#if 0
		while (m_MruList.NotEnd(pMru)
				&& pMru->MRU_Count < MaxMRU
				&& (pMru->LockCount > 0
					|| 0 != pMru->DirtyMask))
		{
			pMru = m_MruList.Prev(pMru);
		}
		// unlocked buffer not found or buffer was too recent
		if (m_MruList.IsEnd(pMru) || pMru->MRU_Count >= MaxMRU)
#endif
		{
			// try to find a dirty buffer

			for (pMru = m_MruList.Last();
				m_MruList.NotEnd(pMru) && pMru->MRU_Count < MaxMRU
				&& pMru->LockCount > 0;
				pMru = m_MruList.Prev(pMru))
			{
			}
			if (m_MruList.NotEnd(pMru)
				&& 0 == pMru->LockCount
				&& pMru->MRU_Count < MaxMRU)
			{
				if (0 != pMru->DirtyMask)
				{
					// oldest buffer is dirty
					pMru->pFile->FlushDirtyBuffers(static_cast<BufferHeader *>(pMru));
					// try the loop again
					continue;
				}
				// buffer found
			}
			else
			{
				return NULL;  // unable to find a buffer
			}
		}

		File * pFile = pMru->pFile;
		CSimpleCriticalSectionLock lock1(pFile->BuffersList);
		// check if the buffer changed
		if (0 != pMru->LockCount
			|| 0 != pMru->DirtyMask)
		{
			continue;   // try again
		}

		// get the buffer from the MRU list
		m_MruList.RemoveEntry(pMru);

		BufferHeader * pBuf = static_cast<BufferHeader *>(pMru);

		if(TRACE_MRU) TRACE("Invalidate buffer: file %X, offset %X0000\n", pFile->hFile, pBuf->PositionKey);

		pFile->BuffersList.RemoveEntryUnsafe(pBuf);

		pFile->BuffersCount--;
		ASSERT(0 == pBuf->DirtyMask);
		pBuf->ReadMask = 0;
		pBuf->DirtyMask = 0;
		pBuf->pFile = NULL;
		return pBuf;
	}
	return NULL;
}

#if 0
// read the data by 32-bit MaskToRead (each bit corresponds to 2K)
// to the buffer defined by BufferHeader *pBuf
BOOL File::ReadFileAt(LONGLONG Position, void * pBuf, DWORD ToRead, DWORD * pWasRead)
{
	BOOL result;
	CLastError LastError;
	while (1)
	{
		if (m_UseOverlappedIo)
		{
			OVERLAPPED ov;
			ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
			// no need to use m_FilePointer
			ov.Offset = (ULONG)(Position & 0xFFFFFFFF);
			ov.OffsetHigh = (ULONG)(Position >> 32);
			ov.Internal = 0;
			ov.InternalHigh = 0;

			result = ReadFile(hFile, pBuf, ToRead, pWasRead, &ov);
			if (FALSE == result
				&& ERROR_IO_PENDING == GetLastError())
			{
				result = GetOverlappedResult(hFile, & ov, pWasRead, TRUE);
			}

			LastError.Get();
			if (0 == m_LastError)
			{
				m_LastError = LastError;
			}
		}
		else    // using synchronous I/O
		{
			CSimpleCriticalSectionLock lock(m_FileLock);
			if (0) TRACE("Stored file pointer: %X, actual: %X\n",
						long(m_FilePointer), SetFilePointer(hFile, 0, NULL, FILE_CURRENT));

			DebugTimeStamp time;

			if (Position != m_FilePointer)
			{
				LONG FilePtrH = LONG(Position >> 32);
				SetFilePointer(hFile, (ULONG)(Position & 0xFFFFFFFF), & FilePtrH, FILE_BEGIN);
				m_FilePointer = Position;
			}

			result = ReadFile(hFile, pBuf, ToRead, pWasRead, NULL);

			if (TRACE_READ) TRACE("ReadFile(%08x, pos=0x%08X, bytes=%X), elapsed time=%d ms/10\n",
								hFile, (ULONG)(Position & 0xFFFFFFFF), ToRead, time.ElapsedTimeTenthMs());
			LastError.Get();
			if (0 == m_LastError)
			{
				m_LastError = LastError;
			}
#ifdef _DEBUG
			if (*pWasRead < ToRead)
			{
				if (0) TRACE("ToRead=%x, BytesRead=%x\n", ToRead, *pWasRead);
			}
#endif
			m_FilePointer += *pWasRead;
		}
		if (result)
		{
			break;
		}

		CString s;
		CHeapPtr<TCHAR, CLocalAllocator > pszTemp(NULL);

		DWORD dwResult = FormatMessage(
										FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
										NULL, LastError, 0, reinterpret_cast<LPTSTR>( & pszTemp),
										0, NULL);

		s.Format(IDC_FILE_READ_ERROR, LPCTSTR(m_FileName), LPCTSTR(pszTemp));

		DWORD flags = MB_DEFBUTTON2 | MB_ICONERROR | MB_CANCELTRYCONTINUE | MB_SETFOREGROUND;
		if (0x80000000 & GetVersion())
		{
			// Win9x
			flags = MB_DEFBUTTON2 | MB_ICONERROR | MB_RETRYCANCEL | MB_SETFOREGROUND;
		}

		INT_PTR retcode = MessageBoxSync(s, flags);
		if (IDRETRY != retcode)
		{
			break;
		}
	}

	return result;
}
BOOL File::WriteFileAt(LONGLONG Position, void const * pBuf,
						DWORD ToWrite, DWORD * pWasWritten)
{
	//TODO
	return 0;
}
#endif
void File::ReadDataBuffer(BufferHeader * pBuf, DWORD MaskToRead)
{
	// check if the required data is already in the buffer
	if (0 != MaskToRead
		&& (pBuf->ReadMask & MaskToRead) == MaskToRead)
	{
		return;
	}
	int OldPriority;
	{
		CSimpleCriticalSectionLock lock(m_FileLock);
		// the operation in progress might change the mask
		if (0 != MaskToRead
			&& (pBuf->ReadMask & MaskToRead) == MaskToRead)
		{
			return;
		}
		MaskToRead &= ~ pBuf->ReadMask;
		// read the required data
		LONGLONG StartFilePtr = ULONGLONG(pBuf->PositionKey) << BLOCK_SIZE_SHIFT;
		char * buf = (char *) pBuf->pBuf;
		// check if the buffer is in the initialized part of the file
		if (0 == (m_Flags & CDirectFile::FileFlagsReadOnly))
		{
			size_t WrittenMaskOffset = pBuf->PositionKey >> 3;
			if (WrittenMaskOffset >= WrittenMaskSize)
			{
				size_t NewWrittenMaskSize = WrittenMaskOffset + 512;   // 256 more megs
				char * NewWrittenMask = new char[NewWrittenMaskSize];
				if (NULL == NewWrittenMask)
				{
					if (0) TRACE("NewWrittenMask == NULL\n");
					memset(pBuf->pBuf, 0, CACHE_BLOCK_SIZE);
					pBuf->ReadMask = 0xFFFFFFFF;
					return;
				}
				if (0) TRACE("Allocated NewWrittenMask, size=%d\n", NewWrittenMaskSize);
				memset(NewWrittenMask, 0, NewWrittenMaskSize);
				memcpy(NewWrittenMask, m_pWrittenMask, WrittenMaskSize);
				delete[] m_pWrittenMask;
				m_pWrittenMask = NewWrittenMask;
				WrittenMaskSize = NewWrittenMaskSize;
			}
			ASSERT(m_pWrittenMask);
			if (0 == (m_pWrittenMask[WrittenMaskOffset]
					& (1 << (pBuf->PositionKey & 7))))
			{
				m_pWrittenMask[WrittenMaskOffset] |= 1 << (pBuf->PositionKey & 7);

				DWORD BytesRead = 0;
				int ToZero = CACHE_BLOCK_SIZE;
				if (NULL != pSourceFile
					&& StartFilePtr < UseSourceFileLength
					&& MaskToRead != 0)
				{
					// read the data from the source file
					DWORD ToRead = CACHE_BLOCK_SIZE;
					if (UseSourceFileLength - StartFilePtr < CACHE_BLOCK_SIZE)
					{
						ToRead = DWORD(UseSourceFileLength) - DWORD(StartFilePtr);
					}

					CSimpleCriticalSectionLock lock(pSourceFile->m_FileLock);

					DebugTimeStamp time;

					if (StartFilePtr != pSourceFile->m_FilePointer)
					{
						LONG FilePtrH = LONG(StartFilePtr >> 32);
						SetFilePointer(pSourceFile->hFile, (LONG)StartFilePtr, & FilePtrH, FILE_BEGIN);
						pSourceFile->m_FilePointer = StartFilePtr;
					}
					// round to sector size! otherwize ReadFile would fail
					ReadFile(pSourceFile->hFile, buf,
							(ToRead + (CACHE_SUBBLOCK_SIZE - 1)) & ~MASK_OFFSET_IN_SUBBLOCK, & BytesRead, NULL);

					if (TRACE_READ) TRACE("ReadFile(%08x, pos=0x%08X, bytes=%X), elapsed time=%d ms/10\n",
										hFile, (ULONG)(StartFilePtr & 0xFFFFFFFF), ToRead, time.ElapsedTimeTenthMs());
#ifdef _DEBUG
					if (BytesRead < ToRead)
					{
						if (0) TRACE("ToRead=%x, BytesRead=%x\n", ToRead, BytesRead);
					}
#endif
					if (0 == m_LastError)
					{
						m_LastError = ::GetLastError();
					}

					pSourceFile->m_FilePointer += BytesRead;
					ToZero -= BytesRead;
				}

				if (ToZero > 0)
				{
					memset(buf + BytesRead, 0, ToZero);
				}

				pBuf->ReadMask = 0xFFFFFFFF;
				if (0 == pBuf->DirtyMask.Exchange(0xFFFFFFFF))
				{
					++DirtyBuffersCount;
				}
				return;
			}
		}

		DWORD mask = MaskToRead;
		while (mask != 0)
		{
			unsigned ToRead;
			if (mask == 0xFFFFFFFF)
			{
				mask = 0;
				ToRead = CACHE_BLOCK_SIZE;
			}
			else
			{
				while (0 == (mask & 1))
				{
					mask >>= 1;
					StartFilePtr += CACHE_SUBBLOCK_SIZE;
					buf += CACHE_SUBBLOCK_SIZE;
				}
				ToRead = 0;
				while (mask & 1)
				{
					mask >>= 1;
					ToRead += CACHE_SUBBLOCK_SIZE;
				}
			}
			DWORD BytesRead = 0;

			if (0) TRACE("Stored file pointer: %X, actual: %X\n",
						long(m_FilePointer), SetFilePointer(hFile, 0, NULL, FILE_CURRENT));

			DebugTimeStamp time;

			if (StartFilePtr != m_FilePointer)
			{
				LONG FilePtrH = LONG(StartFilePtr >> 32);
				SetFilePointer(hFile, (LONG)StartFilePtr, & FilePtrH, FILE_BEGIN);
				m_FilePointer = StartFilePtr;
			}

			ReadFile(hFile, buf, ToRead, & BytesRead, NULL);

			if (TRACE_READ) TRACE("ReadFile(%08x, pos=0x%08X, bytes=%X), elapsed time=%d ms/10\n",
								hFile, (ULONG)(StartFilePtr & 0xFFFFFFFF), ToRead, time.ElapsedTimeTenthMs());

			if (0 == m_LastError)
			{
				m_LastError = ::GetLastError();
			}
#ifdef _DEBUG
			if (BytesRead < ToRead)
			{
				if (0) TRACE("ToRead=%x, BytesRead=%x\n", ToRead, BytesRead);
			}
#endif
			m_FilePointer += BytesRead;

			buf += BytesRead;
			StartFilePtr += ToRead;
			ToRead -= BytesRead;
			if (ToRead)
			{
				memset(buf, 0, ToRead);
			}
			buf += ToRead;
		}

		pBuf->ReadMask |= MaskToRead;

		DWORD OldDirtyMask = pBuf->DirtyMask.Exchange_And( ~MaskToRead);

		if (0 != OldDirtyMask && 0 == (OldDirtyMask & ~MaskToRead))
		{
			--DirtyBuffersCount;
		}

		OldPriority = ::GetThreadPriority(GetCurrentThread());
		// yield execution to the waiting thread by lowering priority
		::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
	}
	::SetThreadPriority(GetCurrentThread(), OldPriority);
}

void File::FlushDirtyBuffers(BufferHeader * pDirtyBuf, BLOCK_INDEX MaxKey)
{
	// flush all unlocked dirty buffers in sequence with pBuf;

	BLOCK_INDEX key = pDirtyBuf->PositionKey;
	ASSERT(NULL != pDirtyBuf);

	CSimpleCriticalSectionLock lock(BuffersList);
	BufferHeader * pBuf;

	for (pBuf = BuffersList.Prev(pDirtyBuf);
		BuffersList.NotEnd(pBuf) && key - pBuf->PositionKey <= 2; pBuf = BuffersList.Prev(pBuf))
	{
		if (0 != pBuf->DirtyMask)
		{
			key = pBuf->PositionKey;
			pDirtyBuf = pBuf;
		}
	}

	pBuf = pDirtyBuf;
	key = pBuf->PositionKey;
	// 55ms resolution is OK

	DWORD StartTickCount = GetTickCount();
	do {
		if (pBuf->PositionKey > MaxKey)
		{
			break;
		}
		if (pBuf->DirtyMask)
		{
			key = pBuf->PositionKey;

			CSimpleCriticalSectionLock lock(m_FileLock);
			LONGLONG StartFilePtr = ULONGLONG(pBuf->PositionKey) << BLOCK_SIZE_SHIFT;

			unsigned char * buf = (unsigned char *) pBuf->pBuf;

			size_t WrittenMaskOffset = pBuf->PositionKey >> 3;

			ASSERT(WrittenMaskOffset < WrittenMaskSize);
			ASSERT(m_pWrittenMask);

			DWORD mask = pBuf->DirtyMask.Exchange(0);
			if (mask)
			{
				--DirtyBuffersCount;
			}

			while (mask != 0)
			{
				unsigned ToWrite;
				if (mask == 0xFFFFFFFF)
				{
					mask = 0;
					ToWrite = CACHE_BLOCK_SIZE;
				}
				else
				{
					while (0 == (mask & 1))
					{
						mask >>= 1;
						StartFilePtr += CACHE_SUBBLOCK_SIZE;
						buf += CACHE_SUBBLOCK_SIZE;
					}
					ToWrite = 0;
					while (mask & 1)
					{
						mask >>= 1;
						ToWrite += CACHE_SUBBLOCK_SIZE;
					}
				}
				DWORD BytesWritten = 0;
				if (0) TRACE("Stored file pointer: %X, actual: %X\n",
							long(m_FilePointer), SetFilePointer(hFile, 0, NULL, FILE_CURRENT));

				DebugTimeStamp time;

				if (StartFilePtr != m_FilePointer)
				{
					LONG FilePtrH = LONG(StartFilePtr >> 32);
					SetFilePointer(hFile, (LONG)StartFilePtr, & FilePtrH, FILE_BEGIN);
					m_FilePointer = StartFilePtr;
				}
				if (0) TRACE("Writing h=%X, pos=%X, len=%X, "
							"data=%02X %02X %02X %02X %02X %02X %02X %02X\n", hFile,
							long(StartFilePtr), ToWrite,
							buf[0], buf[1], buf[2], buf[3], buf[4],
							buf[5], buf[6], buf[7]);

				BOOL result = WriteFile(hFile, buf, ToWrite, & BytesWritten, NULL);

				if (! result
					&& 0 == m_LastError)
				{
					m_LastError = ::GetLastError();
				}

				if (TRACE_WRITE) TRACE("WriteFile(%08x, pos=0x%08X, bytes=%X), elapsed time=%d ms/10\n",
										hFile, (ULONG)(StartFilePtr & 0xFFFFFFFF), ToWrite, time.ElapsedTimeTenthMs());

				m_FilePointer += BytesWritten;
				if (BytesWritten != ToWrite)
				{
					// TODO: run a message box in the main thread
					// if it is a secondary thread, post a command
				}

				buf += ToWrite;
				StartFilePtr += ToWrite;

				if (StartFilePtr > RealFileLength)
				{
					RealFileLength = StartFilePtr;
				}
				if (BytesWritten != ToWrite)
				{
					break;
				}
			}
		}
		pBuf = BuffersList.Next(pBuf);
	}
	while (BuffersList.NotEnd(pBuf)
			&& pBuf->PositionKey - key <= 2
			// limit the operation to 500 ms
			&& GetTickCount() - StartTickCount < 300);
}

#ifdef _DEBUG
#define VL_ASSERT(expr) if ( ! (expr)) \
	{ TRACE("FALSE ==(" #expr ")\n"); \
	__asm int 3 \
		}

void File::ValidateList() const
{
	if ( ! DO_VALIDATE)
	{
		return;
	}
	if (m_Flags & CDirectFile::FileFlagsMemoryFile)
	{
		return;
	}
	CSimpleCriticalSectionLock lock(BuffersList);

	BufferHeader * pBuf = BuffersList.First();

	unsigned BufCount = 0;
	int DirtyBufCount = 0;
	while (BuffersList.NotEnd(pBuf))
	{
		VL_ASSERT(pBuf->pFile == this);

		//VL_ASSERT(pBuf);
		BufCount++;
		if (pBuf->DirtyMask)
		{
			DirtyBufCount++;
		}
		if (BuffersList.NotEnd(BuffersList.Next(pBuf)))
		{
			VL_ASSERT(pBuf->PositionKey < BuffersList.Next(pBuf)->PositionKey);
		}
		pBuf = BuffersList.Next(pBuf);
	}
	VL_ASSERT(BufCount == BuffersCount && DirtyBufCount == DirtyBuffersCount);
}
#endif

unsigned CDirectFileCache::_ThreadProc()
{
#ifdef _DEBUG
	FILETIME UserTime, EndTime, tmp;
	GetThreadTimes(GetCurrentThread(), & tmp, & tmp, & tmp, & UserTime);
#endif
	while (m_ThreadRunState != ~0)
	{
		int WaitStatus = WaitForSingleObject(m_hEvent, 2000);
		if (0 != m_ThreadRunState.CompareExchange(0x80000000, 0))
		{
			// signal that the thread is inactive
			SetEvent(m_hThreadSuspendedEvent);
			while (m_ThreadRunState != ~0 && 0 != m_ThreadRunState)
			{
				WaitStatus = WaitForSingleObject(m_hEvent, 2000);
			}
			ResetEvent(m_hThreadSuspendedEvent);
			continue;
		}

		if (WAIT_TIMEOUT == WaitStatus)
		{
			// flush the files
			for (int nFile = 0 ; m_ListToPrefetch.IsEmpty() && 0x80000000 == m_ThreadRunState
				; nFile++)
			{
				File * pFile = 0;
				{
					CSimpleCriticalSectionLock lock(m_cs);

					pFile = m_FileList.First();

					for (int i = 0; m_FileList.NotEnd(pFile)
						&& (i < nFile
							|| 0 == pFile->DirtyBuffersCount
							|| (pFile->m_Flags & CDirectFile::FileFlagsMemoryFile)); i++,
						pFile = m_FileList.Next(pFile))
					{
						// empty
					}

					nFile = i;
					if (m_FileList.IsEnd(pFile))
					{
						break;
					}
					if (0 == pFile->DirtyBuffersCount)
					{
						continue;
					}
					// there is no need to add a reference to file
					// Close() will wait for the loop to suspend
				}
				if ( ! pFile->Flush())
				{
					TRACE(_T("Could not flush idle file %s\n"),
						LPCTSTR(pFile->m_FileName));
				}
			}
		}

		while(0x80000000 == m_ThreadRunState)
		{
			// flush the files
			while (m_FlushRequest)
			{
				m_FlushRequest = FALSE;
				FlushRequestedFiles();
			}

			// prune the prefetched queue from expired ranges

			PrefetchDescriptor * pPrefetch;

			for (pPrefetch = m_ListPrefetched.First(); m_ListPrefetched.NotEnd(pPrefetch);
				pPrefetch = m_ListPrefetched.Next(pPrefetch))
			{
				if (NULL != pPrefetch->m_pFile
					&& GetTickCount() - pPrefetch->m_LastPrefetchTick
					> pPrefetch->PrefetchRangeExpirationTimeout)
				{
					if (TRACE_PREFETCH) TRACE("Item expired: %x: block 0x%x to 0x%x\n",
						pPrefetch->m_pFile->hFile, pPrefetch->m_PrefetchedBeginBlock, pPrefetch->m_PrefetchedEndBlock);
					pPrefetch->m_pFile = NULL;
				}
			}

			if (m_ListToPrefetch.IsEmpty())
			{
				break;
			}

			pPrefetch = m_ListToPrefetch.First();

			// if prefetch area is active
			// check already prefetched ranges
			LONGLONG PrefetchPosition = LONGLONG(pPrefetch->m_PrefetchPosition) << BLOCK_SIZE_SHIFT;

			//LONG PrefetchLength = CACHE_BLOCK_SIZE;

			void * pBuf = NULL;

			long ReadLength = GetDataBuffer(pPrefetch->m_pFile,
											& pBuf, CACHE_BLOCK_SIZE,
											PrefetchPosition, CDirectFile::GetBufferNoPrefetch);

			if (TRACE_PREFETCH) TRACE("Prefetched block 0x%X file %x\n",
									pPrefetch->m_PrefetchPosition, pPrefetch->m_pFile->hFile);

			pPrefetch->m_pFile->ReturnDataBuffer(pBuf, ReadLength, 0);

			{
				CSimpleCriticalSectionLock lock(m_cs);
				if (pPrefetch->m_PrefetchedBeginBlock <= pPrefetch->m_PrefetchedEndBlock)
				{
					if (pPrefetch->m_PrefetchPosition < pPrefetch->m_PrefetchedEndBlock)
					{
						pPrefetch->m_PrefetchPosition++;
						continue;
					}
				}
				else
				{
					if (pPrefetch->m_PrefetchPosition > pPrefetch->m_PrefetchedEndBlock)
					{
						pPrefetch->m_PrefetchPosition--;
						continue;
					}
				}
				// retire the prefetch item
				m_ListToPrefetch.RemoveEntry(pPrefetch);

				pPrefetch->m_LastPrefetchTick = GetTickCount();

				m_ListPrefetched.InsertTail(pPrefetch);
			}
		}
		// reset bit 0x80000000, but only if not equal ~0
		long tmp;
		while ((tmp = m_ThreadRunState) != ~0UL
				&& tmp != m_ThreadRunState.CompareExchange(tmp & ~ 0x80000000, tmp));
	}
#ifdef _DEBUG
	GetThreadTimes(GetCurrentThread(), & tmp, & tmp, & tmp, & EndTime);
	TRACE("File cache thread used time=%d ms\n",
		(EndTime.dwLowDateTime - UserTime.dwLowDateTime) / 10000);
#endif
	return 0;
}

BOOL File::InitializeTheRestOfFile(int timeout, LONGLONG * pSizeCompleted)
{
	if (NULL == m_pWrittenMask
		|| (m_Flags & CDirectFile::FileFlagsReadOnly)
		|| (m_Flags & CDirectFile::FileFlagsMemoryFile))
	{
		if (pSizeCompleted)
		{
			*pSizeCompleted = FileLength;
		}
		return TRUE;
	}
	// find uninitialized buffer
	DWORD BeginTime = timeGetTime();
	for (unsigned i = 0; i < unsigned((FileLength + CACHE_BLOCK_SIZE - 1) >> BLOCK_SIZE_SHIFT); i++)
	{
		if (0 == (m_pWrittenMask[i / 8] & (1 << (i & 7))))
		{
			void * ptr;
			LONG count = GetDataBuffer( & ptr, CACHE_BLOCK_SIZE, LONGLONG(i) << BLOCK_SIZE_SHIFT, 0);
			if (count)
			{
				ReturnDataBuffer(ptr, CACHE_BLOCK_SIZE, CDirectFile::ReturnBufferDiscard);
			}
		}
		if (timeout != 0
			&& int(timeGetTime() - BeginTime) > timeout)
		{
			if (pSizeCompleted)
			{
				*pSizeCompleted = i * ULONGLONG(CACHE_BLOCK_SIZE);
			}
			return FALSE;   // not finished yet
		}
	}

	VERIFY(Flush());

	if (pSizeCompleted)
	{
		*pSizeCompleted = FileLength;
	}
	return TRUE;
}

BOOL File::SetFileLength(LONGLONG NewLength)
{
	if (m_Flags & CDirectFile::FileFlagsMemoryFile)
	{
		// allocate or reallocate memory buffer
		if (NewLength > MEMORY_FILE_SIZE_LIMIT
			|| NewLength < 0
			|| 0 != m_MemoryBufferRefCount)
		{
			// limit 1 M
			return FALSE;
		}

		ULONG MemofyFileLength = ULONG(NewLength);
		if (MemofyFileLength <= m_MemoryFileBufferSize)
		{
			if (MemofyFileLength > FileLength)
			{
				memset(m_pMemoryFileBuffer + FileLength, 0, size_t(MemofyFileLength - FileLength));
			}
			FileLength = MemofyFileLength;
			return TRUE;
		}
		char * NewBuf = new char[MemofyFileLength];
		if (NULL == NewBuf)
		{
			return FALSE;
		}
		if (m_pMemoryFileBuffer)
		{
			memcpy(NewBuf, m_pMemoryFileBuffer, size_t(FileLength));
			delete m_pMemoryFileBuffer;
		}
		m_MemoryFileBufferSize = MemofyFileLength;
		m_pMemoryFileBuffer = NewBuf;

		memset(m_pMemoryFileBuffer + FileLength, 0, size_t(MemofyFileLength - FileLength));
		FileLength = MemofyFileLength;

		return TRUE;
	}

	if (m_Flags & CDirectFile::FileFlagsReadOnly)
	{
		SetLastError(ERROR_FILE_READ_ONLY);
		return FALSE;
	}
	// if the file becomes shorter, discard all buffers after the trunk point
	// disable all prefetch for the file
	CacheThreadLock CacheLock;

	CacheInstance.CancelPrefetch(this);

	if (NewLength < FileLength)
	{
		Flush();
		UseSourceFileLength = NewLength;
		// TODO: clear written mask
	}
	else if (NewLength > FileLength)
	{
	}
	if (NewLength == RealFileLength)
	{
		return TRUE;
	}

	CSimpleCriticalSectionLock lock(m_FileLock);
	FileLength = NewLength;
	// set the length to the multiple of 2K
	NewLength = (NewLength + CACHE_SUBBLOCK_SIZE - 1) & ~MASK_OFFSET_IN_SUBBLOCK;
	LONG MoveHigh = LONG(NewLength>> 32);
	DWORD SizeLow, SizeHigh;
	SetLastError(0);
	m_FilePointer = -1i64;  // invalidate file pointer
	if ((0xFFFFFFFF != SetFilePointer(hFile, LONG(NewLength), & MoveHigh, FILE_BEGIN)
			|| ::GetLastError() == NO_ERROR)
		&& SetEndOfFile(hFile)
		&& FlushFileBuffers(hFile))
	{
		SizeLow = ::GetFileSize(hFile, & SizeHigh);
		if (::GetLastError() == NO_ERROR)
		{
			RealFileLength = SizeLow | (LONGLONG(SizeHigh) << 32);
			return TRUE;
		}
		else
		{
			SetLastError(ERROR_DISK_FULL);
			return FALSE;
		}
	}
	else
	{
		SizeLow = ::GetFileSize(hFile, & SizeHigh);
		if (::GetLastError() == NO_ERROR)
		{
			RealFileLength = SizeLow | (LONGLONG(SizeHigh) << 32);
		}
		SetLastError(ERROR_DISK_FULL);
		return FALSE;
	}

}

BOOL File::Flush()
{
	// flush all the buffers
	m_FlushLength = 0;

	while(1)
	{
		CSimpleCriticalSectionLock lock(BuffersList);
		BufferHeader * pBuf = BuffersList.First();

		while (1)
		{
			if (BuffersList.IsEnd(pBuf))
			{
				return TRUE;
			}
			if (0 != pBuf->DirtyMask)
			{
				if (0 != pBuf->LockCount)
				{
					return FALSE;
				}
				FlushDirtyBuffers(pBuf);
				break;
			}
			pBuf = BuffersList.Next(pBuf);
		}
	}
}

void File::FlushRequestedRange()
{
	// flush all the buffers
	while((LONG(m_FlushBegin) & MASK_OFFSET_IN_BLOCK) + m_FlushLength >= CACHE_BLOCK_SIZE)
	{
		CSimpleCriticalSectionLock lock(m_FileLock);
		ULONG key = ULONG(m_FlushBegin >> BLOCK_SIZE_SHIFT);
		BufferHeader * pBuf = FindBuffer(key);

		if (NULL != pBuf)
		{
			if (0 != pBuf->LockCount)
			{
				break;
			}
			if (0 != pBuf->DirtyMask)
			{
				if (0) TRACE("Block %d flushed\n", key);
				FlushDirtyBuffers(pBuf, key);
			}
		}
		LONG Flushed = CACHE_BLOCK_SIZE - (LONG(m_FlushBegin) & MASK_OFFSET_IN_BLOCK);
		m_FlushBegin += Flushed;
		m_FlushLength -= Flushed;
	}
}

// load m_FileInfo member from the file info
void File::LoadFileInformation()
{
	if (::GetFileInformationByHandle(hFile, & m_FileInfo))
	{
		return;
	}
	// GetFileInformationByHandle may fail on some network redirectors
	// simulate GetFileInformation

	m_FileInfo.dwFileAttributes = GetFileAttributes(m_FileName);
	::GetFileTime(hFile, & m_FileInfo.ftCreationTime, & m_FileInfo.ftLastAccessTime,
				& m_FileInfo.ftLastWriteTime);

	m_FileInfo.nFileSizeLow = ::GetFileSize(hFile, & m_FileInfo.nFileSizeHigh);
	m_FileInfo.nNumberOfLinks = 1;

	m_FileInfo.nFileIndexHigh = DWORD(ULONG_PTR(this));
	m_FileInfo.nFileIndexLow = DWORD(ULONG_PTR(this));
}

CDirectFile const & CDirectFile::operator=(CDirectFile & file)
{
	if (this == & file)
	{
		ASSERT(this != & file);
		return * this;
	}
	Close(0);
	m_FilePointer = 0;
	Attach( & file);
	return *this;
}

int CDirectFile::GetFileRefCount() const
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

bool CDirectFile::IsReadOnly() const
{
	if (NULL == m_pFile)
	{
		return false;
	}

	return 0 != (m_pFile->m_Flags & FileFlagsReadOnly);
}

void CDirectFile::ReplaceInstanceData(InstanceData * ptr)
{
	if (NULL != m_pFile)
	{
		delete m_pFile->ReplaceInstanceData(ptr);
	}
}

CDirectFile::InstanceData * CDirectFile::GetInstanceData() const
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

void CDirectFile::ModifyFlags(DWORD set, DWORD reset)
{
	if (NULL != m_pFile)
	{
		m_pFile->m_Flags &= ~reset;
		m_pFile->m_Flags |= set;
	}
}

void CDirectFile::DeleteOnClose(bool Delete)
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

BOOL CDirectFile::Commit(DWORD flags)
{
	if (NULL != m_pFile)
	{
		return m_pFile->Commit(flags);
	}
	else
		return FALSE;
}

BOOL CDirectFile::Rename(LPCTSTR NewName, DWORD flags)
{
	if (NULL != m_pFile)
	{
		return m_pFile->Rename(NewName, flags);
	}
	else
		return FALSE;
}

BOOL CDirectFile::InitializeTheRestOfFile(int timeout, LONGLONG * pSizeCompleted)
{
	if (NULL != m_pFile)
	{
		return m_pFile->InitializeTheRestOfFile(timeout, pSizeCompleted);
	}
	else
		return TRUE;    // operation completed
}

DWORD CDirectFile::Flags() const
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

void CDirectFile::ReturnDataBuffer(void * pBuffer, long count, DWORD flags)
{
	ASSERT(m_pFile != NULL);
	if (NULL == m_pFile)
	{
		return;
	}
	m_pFile->ReturnDataBuffer(pBuffer, count, flags);
}

long CDirectFile::GetDataBuffer(void * * ppBuf, LONGLONG length, LONGLONG position, DWORD flags)
{
	ASSERT(m_pFile != NULL);
	if (NULL == m_pFile)
	{
		return 0;
	}
	return CacheInstance.GetDataBuffer(m_pFile, ppBuf, length, position, flags);
}

DWORD CDirectFile::GetFileSize(LPDWORD lpFileSizeHigh)
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

LONGLONG CDirectFile::GetLength() const
{
	if (NULL == m_pFile)
	{
		return 0;
	}
	return m_pFile->FileLength;
}

LPCTSTR CDirectFile::GetName() const
{
	if (NULL == m_pFile)
	{
		return _T("");
	}
	else
	{
		return m_pFile->m_FileName;
	}
}

BOOL CDirectFile::SetFileLength(LONGLONG NewLength)
{
	if (NULL == m_pFile)
	{
		return FALSE;
	}
	return m_pFile->SetFileLength(NewLength);
}

BY_HANDLE_FILE_INFORMATION const & CDirectFile::GetFileInformation() const
{
	return m_pFile->m_FileInfo;
}

int CDirectFile::GetLastError() const
{
	if (m_pFile != NULL)
		return m_pFile->m_LastError;
	else
		return 0;
}

void CDirectFile::ResetLastError()
{
	if (m_pFile != NULL)
		m_pFile->m_LastError = 0;
}

CDirectFileCacheProxy::CDirectFileCacheProxy(size_t MaxCacheSize)
{
	CacheInstance.InitCache(MaxCacheSize);
}

CDirectFileCacheProxy::~CDirectFileCacheProxy()
{
	CacheInstance.DeInitCache();
}

DWORD GetSectorSize(LPCTSTR szFilename)
{
#if 1
	CPathEx curr_dir;

	if ( ! curr_dir.GetCurrentDirectory())
	{
		return 0x1000;
	}
#endif

	CPathEx file_dir(szFilename);

	if ( ! file_dir.MakeFullPath())
	{
		return 0x1000;
	}

	if (file_dir.IsUNC())
	{
		return 0x1000;
	}

	if ( ! file_dir.StripToRoot())
	{
		return 0x1000;
	}

	DWORD sector_size = 0;
	DWORD sectors_per_cluster;
	DWORD free_clusters;
	DWORD total_clusters;

	if (! GetDiskFreeSpace(file_dir, & sectors_per_cluster,
							& sector_size, & free_clusters, & total_clusters)
		|| 0 == sector_size)
	{
		sector_size = 0x1000;
	}

	return sector_size;
}

