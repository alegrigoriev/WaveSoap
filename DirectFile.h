// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// DirectFile.h: interface for the CDirectFile class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DIRECTFILE_H__B7AA7401_4036_11D4_9ADD_00C0F0583C4B__INCLUDED_)
#define AFX_DIRECTFILE_H__B7AA7401_4036_11D4_9ADD_00C0F0583C4B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "KListEntry.h"
namespace DirectFileCache
{
class CDirectFileCache;
struct File;
}

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
		virtual InstanceData & operator = (InstanceData const &)
		{
			return * this;
		}
		virtual void MoveDataTo(InstanceData * /*dst*/)
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

	virtual BOOL Open(LPCTSTR szName, UINT flags);
	BOOL Close(DWORD flags);
	BOOL Attach(CDirectFile * const pOriginalFile);
	BOOL SetSourceFile(CDirectFile * const pOriginalFile);
	BOOL DetachSourceFile();

	static int CacheBufferSize();
	// allocate data common for all instances
	// attached to the same File
	bool IsOpen() const
	{
		return m_pFile != 0;
	}

	int GetFileRefCount() const;

	bool IsReadOnly() const;

	template<typename T>
	T * AllocateInstanceData()
	{
		if (sizeof(T) > GetInstanceData()->m_size)
		{
			T * ptr = new T;
			if (NULL == ptr)
			{
				return NULL;
			}
			ReplaceInstanceData(ptr);
		}
		return static_cast<T *>(GetInstanceData());
	}

	InstanceData * GetInstanceData() const;

	long Read(void * buf, long count);
	long ReadAt(void * buf, long count, LONGLONG Position);
	long Write(const void * pBuf, long count);
	long WriteAt(const void * buf, long count, LONGLONG Position);
	LONGLONG Seek(LONGLONG position, int flag = FILE_BEGIN);
	LONGLONG GetFilePointer() const
	{
		return m_FilePointer;
	}

	UINT_PTR GetFileID() const
	{
		return UINT_PTR(m_pFile);
	}
	// file MUST be open!
	void ModifyFlags(DWORD set, DWORD reset);

	void DeleteOnClose(bool Delete = true);

	enum {
		CommitFileFlushBuffers = 1,
		RenameFileFlushBuffers = CommitFileFlushBuffers,
		CommitFileDontReopen = 2,
		RenameFileDontReopen = CommitFileDontReopen,
		RenameFileOpenReadOnly = 4,
	};

	BOOL Commit(DWORD flags = CommitFileFlushBuffers);

	BOOL Rename(LPCTSTR NewName, DWORD flags);

	BOOL InitializeTheRestOfFile(int timeout = 0, int * pPercentCompleted = NULL);

	DWORD Flags() const;

	enum {
		ReturnBufferDirty = 1, // buffer contains changed data
		ReturnBufferDiscard = 2, // make the buffer lower priority
		ReturnBufferFlush = 4,  // request buffer flush
	};

	// unlock the buffer, mark dirty if necessary
	void ReturnDataBuffer(void * pBuffer, long count, DWORD flags = 0);

	enum
	{
		GetBufferWriteOnly = 1,
		GetBufferNoPrefetch = 2,
		GetBufferAndPrefetchNext = 4,
	};
	// read data, lock the buffer
	// and return the buffer address
	long GetDataBuffer(void * * ppBuf, LONGLONG length, LONGLONG position, DWORD flags = 0);

	DWORD GetFileSize(LPDWORD lpFileSizeHigh);

	LONGLONG GetLength() const;

	LPCTSTR GetName() const;

	BOOL SetFileLength(LONGLONG NewLength);

	BY_HANDLE_FILE_INFORMATION const & GetFileInformation() const;

	int GetLastError() const;

	void ResetLastError();

	bool IsTemporaryFile() const
	{
		return IsOpen()
				&& 0 == (Flags() & (FileFlagsDeleteAfterClose | FileFlagsMemoryFile));
	}

protected:
	enum {
		FileFlagsDeleteAfterClose = OpenDeleteAfterClose,
		FileFlagsReadOnly = OpenReadOnly,
		FileFlagsMemoryFile = CreateMemoryFile,
	};

	friend struct DirectFileCache::File;

	friend class DirectFileCache::CDirectFileCache;
protected:
	DirectFileCache::File * m_pFile;
	LONGLONG m_FilePointer;
private:
	void ReplaceInstanceData(InstanceData * ptr);
	// copy constructor inaccessible
	CDirectFile(CDirectFile const &);
};

	// this structure should be a base class of the application class
struct DirectFileParameters
{
	int m_MaxMemoryFileSize;    // in Kbytes
	int m_MaxFileCache;        // in megabytes
	bool m_bUseMemoryFiles;  // File proppage
	CString m_sTempDir;         // File proppage

	DirectFileParameters()
		: m_bUseMemoryFiles(true),
		m_MaxMemoryFileSize(64),
		m_MaxFileCache(64)
	{
	}
};

class CDirectFileCacheProxy
{
public:
	CDirectFileCacheProxy(size_t MaxCacheSize);
	~CDirectFileCacheProxy();
};

#endif // !defined(AFX_DIRECTFILE_H__B7AA7401_4036_11D4_9ADD_00C0F0583C4B__INCLUDED_)
