// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// DirectFile.cpp: implementation of the CDirectFile class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DirectFile.h"
#include <process.h>
#include "MessageBoxSynch.h"
#include "KInterlocked.h"

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
	void ReturnDataBuffer(File * pFile, void * pBuffer,
						long count, DWORD flags);

	BufferHeader * GetFreeBuffer(unsigned MaxMRU = 0xFFFFFFFFu);
	File * Open(LPCTSTR szName, DWORD flags);
	void RequestPrefetch(File * pFile, LONGLONG PrefetchPosition,
						LONGLONG PrefetchLength, unsigned MaxMRU);
	void RequestFlush(File * pFile, LONGLONG FlushPosition,
					LONG FlushLength);
	void FlushRequestedFiles();


private:
	// points to the only instance of the class
	BufferHeader * m_pHeaders;    // address of array
	// when you traverse from Head to Tail, jump to pNext.
	// when you traverse from Tail to Head , jump to pPrev.
	// Pointer to a buffer with highest MRU. This buffer have pMruPrev=NULL
	ListHead<BufferMruEntry> m_MruList;

	void * m_pBuffersArray;    // allocated area
	int m_NumberOfBuffers; // number of allocated buffers
	ListHead<BufferHeader> m_FreeBuffers;
	ListHead<File> m_FileList;
	DWORD m_Flags;
	ULONG_volatile m_MRU_Count;

	HANDLE m_hThread;
	HANDLE m_hEvent;
	HANDLE m_hThreadSuspendedEvent;
	BOOL m_bRunThread;

	LONG_volatile m_ThreadRunState;
	LONG_volatile m_InitCount;
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

static CDirectFileCache CacheInstance;

struct DirectFileCache::File : public ListItem<DirectFileCache::File>
{
	typedef CDirectFile::InstanceData InstanceData;
	HANDLE hFile;
	DWORD m_Flags;
	long RefCount;
	LockedListHead<BufferHeader> mutable BuffersList;
	// number of buffers in the list
	int BuffersCount;
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
		long WrittenMaskSize;
		long m_MemoryFileBufferSize;
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

	CDirectFile::InstanceData * GetInstanceData() const
	{
		return m_pInstanceData;
	}

	void ReturnDataBuffer(void * pBuffer, long count, DWORD flags = 0)
	{
		CacheInstance.ReturnDataBuffer(this, pBuffer, count, flags);
	}
	BOOL Close(DWORD flags);
	BOOL Commit(DWORD flags);
	BOOL Rename(LPCTSTR NewName, DWORD flags);

	void ReadDataBuffer(BufferHeader * pBuf, DWORD MaskToRead);
	void FlushRequestedRange();
	void FlushDirtyBuffers(BufferHeader * pBuf, unsigned long MaxKey = 0xFFFFFFFF);
	// read data, lock the buffer
	// and return the buffer address
	long GetDataBuffer(void * * ppBuf, LONGLONG length, LONGLONG position, DWORD flags = 0)
	{
		return CacheInstance.GetDataBuffer(this, ppBuf, length, position, flags);
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
		m_pInstanceData(new CDirectFile::InstanceData),
		m_PrefetchedBeginBlock(-1),
		m_PrefetchedEndBlock(-1),
		m_LastPrefetchTick(0),
		m_FlushBegin(0),
		m_FlushLength(0),
		// one sixth of cache size is allowed to prefetch
		m_MaxBlocksToPrefetch(CacheInstance.m_NumberOfBuffers / 6),
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
	ULONG_volatile ReadMask;     // 32 bits for data available (a bit per 2K)
	ULONG_volatile DirtyMask;    // 32 bits for dirty data
	File * pFile;
	unsigned long PositionKey;   // position / 0x10000
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
	if (flags & CDirectFile::CreateMemoryFile)
	{
		// create memory file of zero length
		File * pFile = new File("");
		if (NULL == pFile)
		{
			return NULL;
		}
		pFile->m_Flags |= CDirectFile::FileFlagsMemoryFile;
		CSimpleCriticalSectionLock lock(m_cs);

		m_FileList.InsertHead(pFile);

		SetLastError(ERROR_SUCCESS);
		return pFile;
	}
	CString FullName;
	TCHAR * pName = FullName.GetBuffer(512);

	LPTSTR pFilePart = NULL;
	GetFullPathName(szName, 511, pName, & pFilePart);
	FullName.ReleaseBuffer();
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
			SetLastError(ERROR_INVALID_FUNCTION);
			return NULL;
		}
		if (::GetFileInformationByHandle(hf, & info))
		{
			CloseHandle(hf);
			TRACE(_T("Volume ID: %08X, File index: %08X%08X\n"),
				info.dwVolumeSerialNumber, info.nFileIndexHigh,
				info.nFileIndexLow);
			// find a File struct with the same parameters
			CSimpleCriticalSectionLock lock(m_cs);

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
						SetLastError(ERROR_SHARING_VIOLATION);
						return NULL;
					}

					SetLastError(ERROR_ALREADY_EXISTS);
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
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return NULL;
	}

	do {
		DWORD access = GENERIC_READ;
		DWORD ShareMode = FILE_SHARE_READ;
		DWORD Disposition = OPEN_EXISTING;
		if (0 == (flags & CDirectFile::OpenReadOnly))
		{
			if (flags & (CDirectFile::OpenDirect | CDirectFile::OpenExisting))
			{
				ShareMode = 0;
				access = GENERIC_WRITE | GENERIC_READ;
			}
			else if (flags & CDirectFile::CreateAlways)
			{
				ShareMode = 0;
				access = GENERIC_WRITE | GENERIC_READ;
				Disposition = CREATE_ALWAYS;
			}
			else if (flags & CDirectFile::CreateNew)
			{
				ShareMode = 0;
				access = GENERIC_WRITE | GENERIC_READ;
				Disposition = CREATE_NEW;
			}
		}

		hf = CreateFile(FullName,
						access,
						ShareMode,
						NULL, // lpSecurity
						Disposition,
						FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NORMAL,
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

	if (FILE_TYPE_DISK != GetFileType(hf))
	{
		SetLastError(ERROR_INVALID_FUNCTION);
		CloseHandle(hf);
		pFile->Close(0); // delete
		return NULL;
	}

	::GetFileInformationByHandle(hf, & pFile->m_FileInfo);
	pFile->FileLength = pFile->m_FileInfo.nFileSizeLow
						| (ULONGLONG(pFile->m_FileInfo.nFileSizeHigh << 32));
	pFile->RealFileLength = pFile->FileLength;

	if (0 == (flags & CDirectFile::OpenReadOnly))
	{
		int WrittenMaskLength = 512 + int((pFile->FileLength + 0x7FFFF) >> 19);
		pFile->m_pWrittenMask = new char[WrittenMaskLength];
		if (NULL == pFile->m_pWrittenMask)
		{
			SetLastError(ERROR_NOT_ENOUGH_MEMORY);
			CloseHandle(hf);
			pFile->Close(0); // delete
			return NULL;
		}

		pFile->WrittenMaskSize = WrittenMaskLength;
		memset(pFile->m_pWrittenMask, 0, WrittenMaskLength);
		memset(pFile->m_pWrittenMask, 0xFF, size_t((pFile->FileLength +0xFFFF) >> 19));
		pFile->m_pWrittenMask[(pFile->FileLength +0xFFFF) >> 19] =
			0xFF >> (8 - (((pFile->FileLength +0xFFFF) >> 16) & 7));
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

	pFile->hFile = hf;

	CSimpleCriticalSectionLock lock(m_cs);
	m_FileList.InsertHead(pFile);

	SetLastError(ERROR_SUCCESS);
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

		Flush();

	}
	if (FileLength != RealFileLength)
	{
		CSimpleCriticalSectionLock lock(m_FileLock);

		CloseHandle(hFile);
		hFile = NULL;

		HANDLE hf = CreateFile(sName,
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
			hf = CreateFile(sName,
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
			::GetFileInformationByHandle(hFile, & m_FileInfo);
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
	if (MoveFile(sName, NewName))
	{
		sName = NewName;
		result = TRUE;
	}
	else
	{
		TRACE(_T("Couldn't rename the file from %s to %s, last error=%X\n"),
			LPCTSTR(sName), LPCTSTR(NewName), ::GetLastError());
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

	HANDLE hf = CreateFile(sName,
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
		::GetFileInformationByHandle(hFile, & m_FileInfo);
		RealFileLength = m_FileInfo.nFileSizeLow
						| (ULONGLONG(m_FileInfo.nFileSizeHigh << 32));
	}
	return result;
}

BOOL File::Close(DWORD flags)
{
	// dereference File structure.
	// stop all background operations on the files

	// if the file in prefetch is ours, then wait for the operation to finish
	// last Close operation is always synchronous, there is no parallel activity
	if (++CacheInstance.m_ThreadRunState < 0)
	{
		while (CacheInstance.m_ThreadRunState < 0)
		{
			WaitForSingleObject(CacheInstance.m_hThreadSuspendedEvent, 2000);
		}
	}

	if (--RefCount > 0)
	{
		// allow background thread to run along
		if (--CacheInstance.m_ThreadRunState == 0)
		{
			SetEvent(CacheInstance.m_hEvent);
		}
		return TRUE;
	}
	// if this file was set to prefetch, cancel prefetch
	InterlockedCompareExchange( & CacheInstance.m_pPrefetchFile, (File*)NULL, this);

	TRACE(_T("Closing file %s, flags=%X\n"), LPCTSTR(sName), m_Flags);
	// If the use count is 0, copy all remaining data
	// from the source file or init the rest and flush all the buffers,
	if (0 == (m_Flags & CDirectFile::FileFlagsDeleteAfterClose))
	{
		InitializeTheRestOfFile();
	}
	if (NULL != pSourceFile)
	{
		pSourceFile->Close(0);
		pSourceFile = NULL;
	}
	{
		BufferHeader * pBuf;
		while (NULL != (pBuf = BuffersList.RemoveHead()))
		{
			ASSERT(0 == pBuf->LockCount);
			// something strange: buffer not released
			if (pBuf->LockCount != 0)
			{
				TRACE("File::Close: Buffer not released!\n");
				// allow background thread to run along
				if (--CacheInstance.m_ThreadRunState == 0)
				{
					SetEvent(CacheInstance.m_hEvent);
				}
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

			BuffersCount--;

			CSimpleCriticalSectionLock lock1(CacheInstance.m_cs);

			CacheInstance.m_MruList.RemoveEntry(pBuf);

			pBuf->pFile = NULL;

			CacheInstance.m_FreeBuffers.InsertHead(pBuf);
		}
	}
	// close the handle and delete the structure
	{
		CSimpleCriticalSectionLock lock(CacheInstance.m_cs);
		// remove the structure from the list
		CacheInstance.m_FileList.RemoveEntry(this);
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
		DeleteFile(sName);
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
				HANDLE hf = CreateFile(sName,
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

	TRACE(_T("Closed file %s\n"), LPCTSTR(sName));
	delete this;

	// allow background thread to run along
	if (--CacheInstance.m_ThreadRunState == 0)
	{
		SetEvent(CacheInstance.m_hEvent);
	}

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
		if (m_FilePointer + position < 0)
		{
			return -1i64;
		}
		m_FilePointer += position;
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
	long TotalRead = 0;
	char * pDstBuf = (char *)buf;
	ASSERT(m_pFile);
	if (NULL == m_pFile)
	{
		return 0;
	}

	if (m_FilePointer >= m_pFile->FileLength)
	{
		// beyond end of file
		return 0;
	}
	if (count > 0 && count + m_FilePointer > m_pFile->FileLength)
	{
		count = long(m_pFile->FileLength - m_FilePointer);
	}

	while(count > 0)
	{
		void * pSrcBuf;
		long nRead = GetDataBuffer( & pSrcBuf, count, m_FilePointer, GetBufferAndPrefetchNext);
		if (0 == nRead)
		{
			break;
		}
		memmove(pDstBuf, pSrcBuf, nRead);
		ReturnDataBuffer(pSrcBuf, nRead, 0);
		TotalRead += nRead;
		pDstBuf += nRead;
		m_FilePointer += nRead;
		count -= nRead;
	}
	return TotalRead;
}

// read data ('count' bytes) from the given 'position' to *buf
long CDirectFile::ReadAt(void *buf, long count, LONGLONG Position)
{
	long TotalRead = 0;
	char * pDstBuf = (char *)buf;
	ASSERT(m_pFile);
	if (NULL == m_pFile)
	{
		return 0;
	}

	if (Position >= m_pFile->FileLength)
	{
		// beyond end of file
		return 0;
	}
	if (count > 0 && count + Position > m_pFile->FileLength)
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
	return TotalRead;
}

// write data ('count' bytes) from the current position to *buf
long CDirectFile::Write(const void *pBuf, long count)
{
	long TotalWritten = 0;
	const char * pSrcBuf = (const char *)pBuf;
	ASSERT(m_pFile);
	if (NULL == m_pFile)
	{
		return 0;
	}

	while(count > 0)
	{
		void * pDstBuf;
		long nWritten = GetDataBuffer( & pDstBuf, count, m_FilePointer, GetBufferWriteOnly);
		if (0 == nWritten)
		{
			break;
		}
		memmove(pDstBuf, pSrcBuf, nWritten);
		ReturnDataBuffer(pDstBuf, nWritten, ReturnBufferDirty);
		TotalWritten += nWritten;
		pSrcBuf += nWritten;
		m_FilePointer += nWritten;
		count -= nWritten;
	}
	if (TotalWritten != 0
		&& m_FilePointer > m_pFile->FileLength)
	{
		m_pFile->FileLength = m_FilePointer;
	}
	return TotalWritten;
}

// write data ('count' bytes) at the given 'position' from *buf
long CDirectFile::WriteAt(const void *buf, long count, LONGLONG Position)
{
	long TotalWritten = 0;
	char * pSrcBuf = (char *)buf;
	ASSERT(m_pFile);
	if (NULL == m_pFile)
	{
		return 0;
	}
	while(count > 0)
	{
		void * pDstBuf;
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
	return TotalWritten;
}

CDirectFileCache::CDirectFileCache()
	: m_hThread(NULL),
	m_hEvent(NULL),
	m_pHeaders(NULL),
	m_pBuffersArray(NULL),
	m_MRU_Count(1),
	m_Flags(0),
	m_NumberOfBuffers(0),
	m_pPrefetchFile(NULL),
	m_PrefetchPosition(0),
	m_PrefetchLength(0),
	m_MinPrefetchMRU(0),
	m_FlushRequest(0)
{
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
	CacheSize = (CacheSize + 0xFFFF) & ~0xFFFF;
	m_pBuffersArray = VirtualAlloc(NULL, CacheSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	m_NumberOfBuffers = CacheSize / 0x10000;

	m_pHeaders = new BufferHeader[m_NumberOfBuffers];

	for (int i = 0; i < m_NumberOfBuffers; i++)
	{
		m_FreeBuffers.InsertHead( & m_pHeaders[i]);

		m_pHeaders[i].pBuf = i * 0x10000 + (char *) m_pBuffersArray;
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
			ResumeThread(m_hThread);
		}
		else
		{
			TerminateThread(m_hThread, -1);
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
			TerminateThread(m_hThread, -1);
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

	if (NULL != m_pHeaders)
	{
		delete[] m_pHeaders;
		m_pHeaders = NULL;
	}

}

// Insert a buffer to the list with the specified key
// m_FileLock should be locked around this function call
void File::InsertBuffer(BufferHeader * pBuf)
{
	unsigned long const key = pBuf->PositionKey;
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
		pBufAfter = BuffersList.First();
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
	pBufAfter->ListItem<BufferHeader>::InsertNextItem(pBuf);
}

// find a buffer in the list with the specified key
// m_FileLock should be locked around this function call
BufferHeader * File::FindBuffer(unsigned long key) const
{
	// the buffers are ordered: BuffersListHead has lowest key, BuffersListTail has the highest key
	BufferHeader * pBuf;
#ifdef _DEBUG
	ValidateList();
#endif
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
									void * * ppBuf, LONGLONG length, LONGLONG position, DWORD flags, unsigned MaxMRU)
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
		if (length > 0)
		{
			if (position >= pFile->m_MemoryFileBufferSize)
			{
				return 0;
			}
			long MaxLength = pFile->m_MemoryFileBufferSize - long(position);
			if (length > MaxLength)
			{
				length = MaxLength;
			}
		}
		else if (length < 0)
		{
			if (position > pFile->m_MemoryFileBufferSize)
			{
				return 0;
			}
			if (-length > position)
			{
				length = -position;
			}
		}
		else
		{
			return 0;
		}
		*ppBuf = pFile->m_pMemoryFileBuffer + DWORD(position);
		pFile->m_MemoryBufferRefCount++;
		return long(length);
	}

	unsigned long BufferPosition;
	if (length > 0)
	{
		BufferPosition = long(position >> 16);
		OffsetInBuffer = int(position) & 0xFFFF;
		if (OffsetInBuffer + length > 0x10000)
		{
			BytesRequested = 0x10000 - OffsetInBuffer;
			BytesReturned = BytesRequested;
		}
		else
		{
			BytesRequested = long(length);
			BytesReturned = BytesRequested;
			if (flags & CDirectFile::GetBufferAndPrefetchNext)
			{
				// read the rest of the buffer
				BytesRequested = 0x10000 - OffsetInBuffer;
			}
		}
	}
	else if (length < 0)
	{
		if (-length > position)
		{
			length = -position;
			if (0 == length)
			{
				*ppBuf = NULL;
				return 0;
			}
		}
		OffsetInBuffer = 0x10000 - ((-long(position)) & 0xFFFF);
		if (OffsetInBuffer + length <= 0)    // length < 0
		{
			BytesRequested = OffsetInBuffer;
			BytesReturned = BytesRequested;
		}
		else
		{
			BytesRequested = -long(length);
			BytesReturned = BytesRequested;
			if (flags & CDirectFile::GetBufferAndPrefetchNext)
			{
				// read the rest of the buffer
				BytesRequested = OffsetInBuffer;
			}
		}
		BufferPosition = long((position - BytesRequested) >> 16);
		OffsetInBuffer = (long(position) - BytesRequested) & 0xFFFF;
	}
	else
	{
		*ppBuf = NULL;
		return 0;
	}

	DWORD RequestedMask = (0xFFFFFFFFu << (OffsetInBuffer >> 11));
	if (OffsetInBuffer + BytesRequested <= 0xF800)
	{
		RequestedMask &= ~(0xFFFFFFFFu << ((OffsetInBuffer + BytesRequested + 0x7FF) >> 11));
	}
	DWORD MaskToRead = RequestedMask;
	if (flags & CDirectFile::GetBufferWriteOnly)
	{
		MaskToRead = ~RequestedMask;
		if (OffsetInBuffer & 0x7FF)
		{
			MaskToRead |= (1L << (OffsetInBuffer >> 11));
		}
		if ((OffsetInBuffer + BytesRequested) & 0x7FF)
		{
			MaskToRead |= (1L << ((OffsetInBuffer + BytesRequested) >> 11));
		}
	}

	BufferHeader * pBuf = NULL;
	BufferHeader * pFreeBuf = NULL;

	{
		if (0 == ++m_MRU_Count)
		{
			CSimpleCriticalSectionLock lock(m_cs);
			// change it to 0x80000000, subtract 0x80000000 from all counters
			for (int i = 0; i < m_NumberOfBuffers; i++)
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
			// unable to find an available  buffer
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
			pBuf->BufferMruEntry::Init();

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
				if (0 || length <= BytesRequested)
				{
					// prefetch the next buffer
					length = (long(position) & ~0xFFFFL) + 0x20000 - long(position);
				}
				else
				{
					// round the length to make it prefetch the whole buffer
					length += 0xFFFF & -(long(position) + long(length));
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
					length = (position | 0xFFFF) + 1 - 0x20000 - position;
				}
				else
				{
					// round the length to make it prefetch the whole buffer
					length -= 0xFFFF & (long(position) - length);
				}
			}
			RequestPrefetch(pFile, position - BytesRequested, length + BytesRequested, m_MRU_Count);
		}
		ASSERT(BytesReturned <= -length);
		return -BytesReturned;
	}
}

void CDirectFileCache::RequestPrefetch(File * pFile,
										LONGLONG PrefetchPosition,
										LONGLONG PrefetchLength, unsigned MaxMRU)
{
	if (PrefetchLength > 0)
	{
		if (PrefetchPosition + PrefetchLength > pFile->FileLength)
		{
			PrefetchLength = pFile->FileLength - PrefetchPosition;
			if (PrefetchLength <= 0)
			{
				return;
			}
		}
	}
	if (0) TRACE("RequestPrefetch: 0x%X bytes at 0x%X\n", long(PrefetchLength),
				long(PrefetchPosition));
	{
		CSimpleCriticalSectionLock lock(m_cs);
		if (m_pPrefetchFile != pFile
			&& PrefetchPosition + PrefetchLength !=
			m_PrefetchPosition + m_PrefetchLength)
		{
			m_pPrefetchFile = pFile;
			m_PrefetchPosition = PrefetchPosition;
			m_PrefetchLength = PrefetchLength;
		}
		m_MinPrefetchMRU = MaxMRU;
	}
	SetEvent(m_hEvent);
}

void CDirectFileCache::RequestFlush(File * pFile, LONGLONG FlushPosition,
									LONG FlushLength)
{
	// flush only whole buffers
	if (0 == pFile->m_FlushLength)
	{
		pFile->m_FlushBegin = FlushPosition;
		pFile->m_FlushLength = FlushLength;
	}
	else
	{
		if (FlushPosition == pFile->m_FlushBegin + pFile->m_FlushLength)
		{
			pFile->m_FlushLength += FlushLength;
		}
		else
		{
			if (pFile->m_FlushBegin > FlushPosition)
			{
				pFile->m_FlushLength += LONG(pFile->m_FlushBegin) - LONG(FlushPosition);
				pFile->m_FlushBegin = FlushPosition;
			}
			if (pFile->m_FlushBegin + pFile->m_FlushLength < FlushPosition + FlushLength)
			{
				pFile->m_FlushLength = LONG(FlushPosition) - LONG(pFile->m_FlushBegin) + FlushLength;
			}
		}
	}
	// if flush boundary ends or crosses 64 boundary, request flush
	if ((LONG(pFile->m_FlushBegin) & 0xFFFF) + pFile->m_FlushLength >= 0x10000)
	{
		m_FlushRequest = TRUE;
		SetEvent(m_hEvent);
	}
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

void CDirectFileCache::ReturnDataBuffer(File * pFile,
										void * pBuffer, long count, DWORD flags)
{
	if (NULL == pBuffer || 0 == count)
	{
		return;
	}
	if (pFile->m_Flags & CDirectFile::FileFlagsMemoryFile)
	{
		pFile->m_MemoryBufferRefCount--;
		return;
	}
	// find the buffer descriptor
	DWORD dwBuffer = DWORD(pBuffer);
	if (count < 0)
	{
		dwBuffer += count;
		count = -count;
	}
	int BufferNumber = (dwBuffer - DWORD(m_pBuffersArray)) / 0x10000;
	BufferHeader * pBuf = & m_pHeaders[BufferNumber];
	ASSERT(BufferNumber >= 0 && BufferNumber < m_NumberOfBuffers);
	ASSERT(pBuf->pFile == pFile);
	ASSERT(pBuf->LockCount > 0);
	ASSERT((DWORD)pBuf->pBuf == (dwBuffer & ~0xFFFF));
	// mark the buffer dirty, if necessary
	int OffsetInBuffer = dwBuffer & 0xFFFF;
	LONGLONG FileOffset = OffsetInBuffer + (LONGLONG(pBuf->PositionKey) << 16);

	ASSERT(OffsetInBuffer + count <= 0x10000);

	{
		CSimpleCriticalSectionLock lock(pFile->BuffersList);
		if (flags & CDirectFile::ReturnBufferDirty)
		{
			DWORD RequestedMask = (0xFFFFFFFFu << (OffsetInBuffer >> 11));
			if (OffsetInBuffer + count <= 0xF800)
			{
				RequestedMask &= ~(0xFFFFFFFFu << ((OffsetInBuffer + count + 0x7FF) >> 11));
			}

			DWORD OldDirtyMask = pBuf->DirtyMask.Exchange_Or(RequestedMask);

			pBuf->ReadMask |= RequestedMask;
			if (0 == OldDirtyMask && 0 != RequestedMask)
			{
				pFile->DirtyBuffersCount++;
			}
		}
	}

	BufferMruEntry * pMru = pBuf;

	if (0 == ((count + dwBuffer) & 0xFFFF)
		&& (flags & CDirectFile::ReturnBufferDiscard))
	{
		// move the buffer to MRU tail
		// extract the buffer from MRU list
		CSimpleCriticalSectionLock lock(m_cs);

		m_MruList.RemoveEntry(pMru);
		// insert the buffer to the list tail
		// to do: insert the buffer before the first with MRU_Count==1
		pMru->MRU_Count = 1;

		BufferMruEntry * pBufAfter = m_MruList.Last();
		while (m_MruList.NotEnd(pBufAfter)
				&& 1 == pBufAfter->MRU_Count)
		{
			pBufAfter = m_MruList.Prev(pBufAfter);
		}

		pBufAfter->InsertNextItem(pMru);

	}
	// decrement lock count
	if (0 == --pBuf->LockCount)
	{
		// set a request for writing
		if (flags & CDirectFile::ReturnBufferFlush)
		{
			if (count >= 0)
			{
				RequestFlush(pFile, FileOffset, count);
			}
			else
			{
				RequestFlush(pFile, FileOffset + count, -count);
			}
		}
	}
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
		while (m_MruList.NotEnd(pMru)
				&& pMru->MRU_Count < MaxMRU
				&& (pMru->LockCount > 0
					|| 0 != pMru->DirtyMask))
		{
			pMru = m_MruList.Prev(pMru);
		}

		// unlocked buffer not found or buffer was too recent
		if (m_MruList.IsEnd(pMru) || pMru->MRU_Count >= MaxMRU)
		{
			// try to find a dirty buffer
			pMru = m_MruList.Last();
			while (m_MruList.NotEnd(pMru)
					&& pMru->MRU_Count < MaxMRU
					&& pMru->LockCount > 0)
			{
				pMru = m_MruList.Prev(pMru);
			}
			if (m_MruList.NotEnd(pMru)
				&& 0 == pMru->LockCount
				&& pMru->MRU_Count < MaxMRU)
			{
				// only "dirty" buffer available
				pMru->pFile->FlushDirtyBuffers(static_cast<BufferHeader *>(pMru));
				// try the loop again
				continue;
			}
			return NULL;  // unable to find a buffer
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

// read the data by 32-bit MaskToRead (each bit corresponds to 2K)
// to the buffer defined by BufferHeader *pBuf
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
		LONGLONG StartFilePtr = ULONGLONG(pBuf->PositionKey) << 16;
		char * buf = (char *) pBuf->pBuf;
		// check if the buffer is in the initialized part of the file
		if (0 == (m_Flags & CDirectFile::FileFlagsReadOnly))
		{
			int WrittenMaskOffset = pBuf->PositionKey >> 3;
			if (WrittenMaskOffset >= WrittenMaskSize)
			{
				int NewWrittenMaskSize = WrittenMaskOffset + 512;   // 256 more megs
				char * NewWrittenMask = new char[NewWrittenMaskSize];
				if (NULL == NewWrittenMask)
				{
					if (0) TRACE("NewWrittenMask == NULL\n");
					memset(pBuf->pBuf, 0, 0x10000);
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
				int ToZero = 0x10000;
				if (NULL != pSourceFile
					&& StartFilePtr < UseSourceFileLength
					&& MaskToRead != 0)
				{
					// read the data from the source file
					DWORD ToRead = 0x10000;
					if (UseSourceFileLength - StartFilePtr < 0x10000)
					{
						ToRead = DWORD(UseSourceFileLength) - DWORD(StartFilePtr);
					}

					CSimpleCriticalSectionLock lock(pSourceFile->m_FileLock);
					if (StartFilePtr != pSourceFile->m_FilePointer)
					{
						LONG FilePtrH = LONG(StartFilePtr >> 32);
						SetFilePointer(pSourceFile->hFile, (LONG)StartFilePtr, & FilePtrH, FILE_BEGIN);
						pSourceFile->m_FilePointer = StartFilePtr;
					}
					// round to sector size! otherwize ReadFile would fail
					if (1) TRACE("ReadSourceFile(%08x, pos=0x%08X, bytes=%X)\n", hFile, long(StartFilePtr), ToRead);
					ReadFile(pSourceFile->hFile, buf, (ToRead + 0x1FF) & ~0x1FF, & BytesRead, NULL);
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
			size_t ToRead;
			if (mask == 0xFFFFFFFF)
			{
				mask = 0;
				ToRead = 0x10000;
			}
			else
			{
				while (0 == (mask & 1))
				{
					mask >>= 1;
					StartFilePtr += 0x800;
					buf += 0x800;
				}
				ToRead = 0;
				while (mask & 1)
				{
					mask >>= 1;
					ToRead += 0x800;
				}
			}
			DWORD BytesRead = 0;

			if (0) TRACE("Stored file pointer: %X, actual: %X\n",
						long(m_FilePointer), SetFilePointer(hFile, 0, NULL, FILE_CURRENT));

			if (StartFilePtr != m_FilePointer)
			{
				LONG FilePtrH = LONG(StartFilePtr >> 32);
				SetFilePointer(hFile, (LONG)StartFilePtr, & FilePtrH, FILE_BEGIN);
				m_FilePointer = StartFilePtr;
			}
			if (1) TRACE("ReadFile(%08x, pos=0x%08X, bytes=%X)\n", hFile, long(StartFilePtr), ToRead);
			ReadFile(hFile, buf, ToRead, & BytesRead, NULL);
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
			StartFilePtr += BytesRead;
			ToRead -= BytesRead;
			if (ToRead)
			{
				memset(buf, 0, ToRead);
			}
			buf += ToRead;
			StartFilePtr += ToRead;
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

void File::FlushDirtyBuffers(BufferHeader * pDirtyBuf, unsigned long MaxKey)
{
	// flush all unlocked dirty buffers in sequence with pBuf;

	unsigned key = pDirtyBuf->PositionKey;
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
			LONGLONG StartFilePtr = ULONGLONG(pBuf->PositionKey) << 16;

			unsigned char * buf = (unsigned char *) pBuf->pBuf;

			int WrittenMaskOffset = pBuf->PositionKey >> 3;

			ASSERT(WrittenMaskOffset < WrittenMaskSize);
			ASSERT(m_pWrittenMask);

			DWORD mask = pBuf->DirtyMask.Exchange(0);
			if (mask) --DirtyBuffersCount;

			while (mask != 0)
			{
				size_t ToWrite;
				if (mask == 0xFFFFFFFF)
				{
					mask = 0;
					ToWrite = 0x10000;
				}
				else
				{
					while (0 == (mask & 1))
					{
						mask >>= 1;
						StartFilePtr += 0x800;
						buf += 0x800;
					}
					ToWrite = 0;
					while (mask & 1)
					{
						mask >>= 1;
						ToWrite += 0x800;
					}
				}
				DWORD BytesWritten = 0;
				if (0) TRACE("Stored file pointer: %X, actual: %X\n",
							long(m_FilePointer), SetFilePointer(hFile, 0, NULL, FILE_CURRENT));

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
				if (0) TRACE("WriteFile(%08x, pos=0x%08X, bytes=%X)\n", hFile, long(StartFilePtr), ToWrite);
				BOOL result = WriteFile(hFile, buf, ToWrite, & BytesWritten, NULL);

				if (0 == m_LastError)
				{
					m_LastError = ::GetLastError();
				}
				if (BytesWritten != ToWrite)
				{
					// TODO: run a message box in the main thread
					// if it is a secondary thread, post a command
				}

				m_FilePointer += BytesWritten;
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
	if (m_Flags & CDirectFile::FileFlagsMemoryFile)
	{
		return;
	}
	CSimpleCriticalSectionLock lock(BuffersList);
	BufferHeader * pBuf = BuffersList.First();

	int BufCount = 0;
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
			for (int nFile = 0 ; NULL == m_pPrefetchFile && 0x80000000 == m_ThreadRunState
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
				pFile->Flush();
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
			LONGLONG PrefetchPosition;
			LONGLONG PrefetchLength;
			unsigned PrefetchMRU;
			File * pPrefetchFile;
			if (NULL == m_pPrefetchFile)
			{
				break;
			}
			{
				CSimpleCriticalSectionLock lock(m_cs);
				pPrefetchFile = m_pPrefetchFile;
				PrefetchPosition = m_PrefetchPosition;
				PrefetchLength = m_PrefetchLength;
				PrefetchMRU = m_MinPrefetchMRU;
			}
			if (NULL == pPrefetchFile)
			{
				break;
			}

			// check for prefetch range expiration
			DWORD CurrentTick = GetTickCount();
			LONGLONG OriginalPrefetchPosition = PrefetchPosition;
			LONGLONG OriginalPrefetchLength = PrefetchLength;

			if (CurrentTick - pPrefetchFile->m_LastPrefetchTick
				> pPrefetchFile->PrefetchRangeExpirationTimeout)
			{
				pPrefetchFile->m_PrefetchedBeginBlock = -1;
				pPrefetchFile->m_PrefetchedEndBlock = -1;
			}
			pPrefetchFile->m_LastPrefetchTick = CurrentTick;

			// limit prefetch length
			if (PrefetchLength >= 0)
			{
				if (DWORD(PrefetchLength >> 16) > pPrefetchFile->m_MaxBlocksToPrefetch)
				{
					PrefetchLength = LONGLONG(pPrefetchFile->m_MaxBlocksToPrefetch) << 16;
				}
			}
			else
			{
				if (DWORD(( - PrefetchLength) >> 16) > pPrefetchFile->m_MaxBlocksToPrefetch)
				{
					PrefetchLength = -LONGLONG(pPrefetchFile->m_MaxBlocksToPrefetch) << 16;
				}
			}

			// if prefetch area is active
			// check already prefetched ranges
			if (pPrefetchFile->m_PrefetchedEndBlock != -1)
			{
				if (PrefetchLength >= 0)
				{
					if (DWORD(PrefetchPosition >> 16) < pPrefetchFile->m_PrefetchedEndBlock)
					{
						PrefetchLength -= (LONGLONG(pPrefetchFile->m_PrefetchedEndBlock) << 16)
										- PrefetchPosition;
						PrefetchPosition = LONGLONG(pPrefetchFile->m_PrefetchedEndBlock) << 16;

					}
					pPrefetchFile->m_PrefetchedBeginBlock = DWORD(PrefetchPosition >> 16);
					if (PrefetchLength < 0)
					{
						PrefetchLength = 0;
					}
				}
				else
				{
					if (DWORD(PrefetchPosition >> 16) > pPrefetchFile->m_PrefetchedBeginBlock)
					{
						PrefetchLength -= (LONGLONG(pPrefetchFile->m_PrefetchedBeginBlock) << 16)
										- PrefetchPosition;
						PrefetchPosition = LONGLONG(pPrefetchFile->m_PrefetchedBeginBlock) << 16;

					}
					pPrefetchFile->m_PrefetchedEndBlock = DWORD(PrefetchPosition >> 16);
					if (PrefetchLength >= 0)
					{
						PrefetchLength = 0;
					}
				}
			}
			else
			{
				// set prefetch area
				pPrefetchFile->m_PrefetchedBeginBlock = DWORD(PrefetchPosition >> 16);
				pPrefetchFile->m_PrefetchedEndBlock = pPrefetchFile->m_PrefetchedBeginBlock;
			}

			if (0 == PrefetchLength)
			{
				break;
			}
			void * pBuf = NULL;
			long ReadLength = GetDataBuffer(pPrefetchFile, & pBuf, PrefetchLength,
											PrefetchPosition, CDirectFile::GetBufferNoPrefetch, PrefetchMRU);
			//TRACE("Prefetched buffer at 0x%X\n", long(PrefetchPosition));
			if (pBuf)
			{
				ReturnDataBuffer(pPrefetchFile, pBuf, ReadLength, 0);
			}
			// prefetch was canceled for this file
			{
				CSimpleCriticalSectionLock lock(m_cs);
				if (m_pPrefetchFile != pPrefetchFile
					|| OriginalPrefetchPosition != m_PrefetchPosition
					|| OriginalPrefetchLength != m_PrefetchLength
					)
				{
					// prefetch assignment changed
					continue;
				}

				m_PrefetchLength -= ReadLength;
				if (0 != ReadLength
					&& 0 != m_PrefetchLength)
				{
					m_PrefetchPosition = PrefetchPosition + ReadLength;
					if (ReadLength >= 0)
					{
						pPrefetchFile->m_PrefetchedEndBlock = DWORD(PrefetchPosition >> 16);
					}
					else
					{
						pPrefetchFile->m_PrefetchedBeginBlock = DWORD(PrefetchPosition >> 16);
					}
					continue;
				}
				else
				{
					m_pPrefetchFile = NULL;
					m_PrefetchLength = 0;
					break;
				}
			}
		}
		// reset bit 0x80000000, but only if not equal ~0
		long tmp;
		while ((tmp = m_ThreadRunState) != ~0
				&& tmp != m_ThreadRunState.CompareExchange(tmp & ~ 0x80000000, tmp));
	}
#ifdef _DEBUG
	GetThreadTimes(GetCurrentThread(), & tmp, & tmp, & tmp, & EndTime);
	TRACE("File cache thread used time=%d ms\n",
		(EndTime.dwLowDateTime - UserTime.dwLowDateTime) / 10000);
#endif
	return 0;
}

BOOL File::InitializeTheRestOfFile(int timeout, int * pPercentCompleted)
{
	if (NULL == m_pWrittenMask
		|| (m_Flags & CDirectFile::FileFlagsReadOnly)
		|| (m_Flags & CDirectFile::FileFlagsMemoryFile))
	{
		if (pPercentCompleted)
		{
			*pPercentCompleted = 100;
		}
		return TRUE;
	}
	// find uninitialized buffer
	DWORD BeginTime = timeGetTime();
	for (unsigned i = 0; i < unsigned((FileLength + 0xFFFF) >> 16); i++)
	{
		if (0 == (m_pWrittenMask[i / 8] & (1 << (i & 7))))
		{
			void * ptr;
			LONG count = GetDataBuffer( & ptr, 0x10000, LONGLONG(i) << 16, 0);
			if (count)
			{
				ReturnDataBuffer(ptr, 0x10000, CDirectFile::ReturnBufferDiscard);
			}
		}
		if (timeout != 0
			&& int(timeGetTime() - BeginTime) > timeout)
		{
			if (pPercentCompleted)
			{
				*pPercentCompleted = MulDiv(100, i, int((FileLength + 0xFFFF) >> 16));
			}
			return FALSE;   // not finished yet
		}
	}
	Flush();
	if (pPercentCompleted)
	{
		*pPercentCompleted = 100;
	}
	return TRUE;
}

BOOL File::SetFileLength(LONGLONG NewLength)
{
	if (m_Flags & CDirectFile::FileFlagsMemoryFile)
	{
		// allocate or reallocate memory buffer
		if (NewLength > 0x100000
			|| NewLength < 0
			|| 0 != m_MemoryBufferRefCount)
		{
			// limit 1 M
			return FALSE;
		}
		if (long(NewLength) <= m_MemoryFileBufferSize)
		{
			if (NewLength > FileLength)
			{
				memset(m_pMemoryFileBuffer + FileLength, 0, size_t(NewLength - FileLength));
			}
			FileLength = NewLength;
			return TRUE;
		}
		char * NewBuf = new char[long(NewLength)];
		if (NULL == NewBuf)
		{
			return FALSE;
		}
		if (m_pMemoryFileBuffer)
		{
			memcpy(NewBuf, m_pMemoryFileBuffer, size_t(FileLength));
			delete m_pMemoryFileBuffer;
		}
		m_MemoryFileBufferSize = long(NewLength);
		m_pMemoryFileBuffer = NewBuf;
		memset(m_pMemoryFileBuffer + FileLength, 0, size_t(NewLength - FileLength));
		FileLength = NewLength;
		return TRUE;
	}
	if (m_Flags & CDirectFile::FileFlagsReadOnly)
	{
		SetLastError(ERROR_FILE_READ_ONLY);
		return FALSE;
	}
	// if the file becomes shorter, discard all buffers after the trunk point
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
	NewLength = (NewLength + 0x7FF) & ~0x7FF;
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
		CSimpleCriticalSectionLock lock(m_FileLock);
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
	while((LONG(m_FlushBegin) & 0xFFFF) + m_FlushLength >= 0x10000)
	{
		CSimpleCriticalSectionLock lock(m_FileLock);
		ULONG key = ULONG(m_FlushBegin >> 16);
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
		LONG Flushed = 0x10000 - (LONG(m_FlushBegin) & 0xFFFF);
		m_FlushBegin += Flushed;
		m_FlushLength -= Flushed;
	}
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
		return FALSE;
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

BOOL CDirectFile::InitializeTheRestOfFile(int timeout, int * pPercentCompleted)
{
	if (NULL != m_pFile)
	{
		return m_pFile->InitializeTheRestOfFile(timeout, pPercentCompleted);
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
	CacheInstance.ReturnDataBuffer(m_pFile, pBuffer, count, flags);
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
		return m_pFile->sName;
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
