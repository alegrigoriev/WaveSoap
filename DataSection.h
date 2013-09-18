// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// DataSection.h
#define DEBUG_DATASECTION_GETDATA 0

template <typename T, typename C>
class CDataSection
{
public:
	CDataSection()
		: m_pBuffer(NULL)
		, m_nBufferSize(0)
		, m_nCountInBuffer(0)
		, m_BufferOffset(0)
	{
	}
	~CDataSection()
	{
		delete[] m_pBuffer;
	}
	bool Allocate(long nCount);
	void Invalidate()
	{
		m_nCountInBuffer = 0;
	}
	void InvalidateRange(LONGLONG nOffset, long nCount);

	// returns number of items of type T. nCount is in units of type T.
	// nOffset is in units of T.
	int GetData(T ** ppBuf, LONGLONG nOffset, long nCount, C * pSource);

protected:
	// returns number of items of type T. nCount is in units of type T.
	// nOffset is in units of T.
	int ReadData(T * pBuf, LONGLONG nOffset, long nCount, C * pSource);
	T * m_pBuffer;
	LONGLONG GetSourceCount(C * pSource);
	long m_nBufferSize; // in sizeof(T) units
	long m_nCountInBuffer;
	LONGLONG m_BufferOffset;   // in units of T
};

template <typename T, typename C>
bool CDataSection<T, C>::Allocate(long nCount)
{
	if (m_nBufferSize >= nCount)
	{
		return true;
	}
	// reallocate the buffer
	T * pNewBuffer;
	pNewBuffer = new T[nCount];
	if (NULL == pNewBuffer)
	{
		return false;
	}
	if (NULL != m_pBuffer)
	{
		delete[] m_pBuffer;
		m_pBuffer = NULL;
	}
	m_pBuffer = pNewBuffer;
	m_nBufferSize = nCount;
	m_nCountInBuffer = 0;
	return true;
}

template <typename T, typename C>
void CDataSection<T, C>::InvalidateRange(LONGLONG nOffset, long nCount)
{
	if (nOffset < m_BufferOffset + m_nCountInBuffer
		&& nOffset + nCount > m_BufferOffset)
	{
		m_nCountInBuffer = 0;
	}
}

// returns number of items of type T. nCount is in units of type T.
// nOffset is in units of T.
template <typename T, typename C>
int CDataSection<T, C>::GetData(T ** ppBuf, LONGLONG nOffset, long nCount, C * pSource)
{
	LONGLONG TotalSourceCount = GetSourceCount(pSource);
	if (nOffset >= TotalSourceCount)
	{
		return 0;
	}
	if (nOffset + nCount > TotalSourceCount)
	{
		nCount = long(TotalSourceCount - nOffset);
	}

	if ( ! Allocate(nCount))
	{
		return 0;
	}
	//check if we can reuse some of the data in the buffer
	// use nOffset, NumOfSamples, m_BufferOffset, m_nCountInBuffer
	if (nOffset + nCount <= m_BufferOffset
		|| nOffset >= m_BufferOffset + m_nCountInBuffer)
	{
		// none of the data in the buffer can be reused
		m_BufferOffset = nOffset;
		m_nCountInBuffer = 0;
	}

	if (nOffset < m_BufferOffset)
	{
		// move data up
		long MoveBy = long(m_BufferOffset) - long(nOffset);
		if (m_nCountInBuffer + MoveBy > m_nBufferSize)
		{
			m_nCountInBuffer = m_nBufferSize - MoveBy;
		}
		memmove(m_pBuffer + MoveBy, m_pBuffer,
				m_nCountInBuffer * sizeof(T));
		m_nCountInBuffer += MoveBy;
		ASSERT(m_nCountInBuffer <= m_nBufferSize);

		ReadData(m_pBuffer, nOffset, MoveBy, pSource);
		m_BufferOffset = nOffset;
	}

	if (nOffset + nCount >
		m_BufferOffset + m_nBufferSize)
	{
		// move data down
		long MoveBy = long(nOffset) + nCount -
					(long(m_BufferOffset) + m_nBufferSize);
		if (MoveBy != 0)
		{
			ASSERT(m_nCountInBuffer > MoveBy);
			m_nCountInBuffer -= MoveBy;
			memmove(m_pBuffer, m_pBuffer + MoveBy,
					m_nCountInBuffer * sizeof(T));
			m_BufferOffset += MoveBy;
		}
	}

	if (nOffset + nCount >
		m_BufferOffset + m_nCountInBuffer)
	{
		// adjust NumOfSamples:
		long ReadCount = nCount + long(nOffset) - (long(m_BufferOffset) + m_nCountInBuffer);
		ASSERT(m_nCountInBuffer + ReadCount <= m_nBufferSize);
		long WasRead = ReadData(m_pBuffer + m_nCountInBuffer,
								m_BufferOffset + m_nCountInBuffer, ReadCount, pSource);
		ASSERT(ReadCount == WasRead);
		m_nCountInBuffer += WasRead;
		nCount = long(m_BufferOffset) + m_nCountInBuffer - long(nOffset);
	}

	ASSERT (nOffset >= m_BufferOffset
			&& nOffset + nCount <= m_BufferOffset + m_nCountInBuffer);
#if DEBUG_DATASECTION_GETDATA && defined _DEBUG
	// verify that the buffer contains the correct data
	{
		WAVE_SAMPLE * pVerBuf = new T[nSamples];
		int ReadSamples = ReadData(pVerBuf, nOffset, nCount, pSource);
		ASSERT (0 == memcmp(pVerBuf, m_pBuffer + nOffset - m_BufferOffset,
							ReadSamples * sizeof(T)));
		delete[] pVerBuf;
	}
#endif

	* ppBuf = m_pBuffer + (long(nOffset) - long(m_BufferOffset));

	return nCount;
}
