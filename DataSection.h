// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// DataSection.h

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
	bool Allocate(unsigned nCount);
	void Invalidate()
	{
		m_nCountInBuffer = 0;
	}
	void InvalidateRange(ULONGLONG nOffset, unsigned int nCount);
	int GetData(T ** ppBuf, ULONGLONG nOffset, unsigned int nCount, C * pSource);

protected:
	int ReadData(T * pBuf, ULONGLONG nOffset, unsigned int nCount, C * pSource);
	T * m_pBuffer;
	ULONGLONG GetSourceCount(C * pSource);
	unsigned m_nBufferSize; // in sizeof(T) units
	unsigned m_nCountInBuffer;
	ULONGLONG m_BufferOffset;
};

template <typename T, typename C>
bool CDataSection<T, C>::Allocate(unsigned nCount)
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
		// TODO: copy data from the old buffer to the new
		delete[] m_pBuffer;
		m_pBuffer = NULL;
	}
	m_pBuffer = pNewBuffer;
	m_nBufferSize = nCount;
	m_nCountInBuffer = 0;
	return true;
}

template <typename T, typename C>
void CDataSection<T, C>::InvalidateRange(ULONGLONG nOffset, unsigned int nCount)
{
	if (nOffset < m_BufferOffset + m_nCountInBuffer
		&& nOffset + nCount > m_BufferOffset)
	{
		m_nCountInBuffer = 0;
	}
}

template <typename T, typename C>
int CDataSection<T, C>::GetData(T ** ppBuf, ULONGLONG nOffset, unsigned int nCount, C * pSource)
{
	ULONGLONG TotalSourceCount = GetSourceCount(pSource);
	if (nOffset >= TotalSourceCount)
	{
		return 0;
	}
	if (nOffset + nCount > TotalSourceCount)
	{
		nCount = unsigned(TotalSourceCount - nOffset);
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
		unsigned MoveBy = unsigned(m_BufferOffset) - unsigned(nOffset);
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
		unsigned int MoveBy = unsigned(nOffset) + nCount -
							(unsigned(m_BufferOffset) + m_nBufferSize);
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
		int ReadCount = nCount + unsigned(nOffset) - (unsigned(m_BufferOffset) + m_nCountInBuffer);
		ASSERT(m_nCountInBuffer + ReadCount <= m_nBufferSize);
		ReadCount = ReadData(m_pBuffer + m_nCountInBuffer,
							m_BufferOffset + m_nCountInBuffer, ReadCount, pSource);
		m_nCountInBuffer += ReadCount;
		nCount = unsigned(m_BufferOffset) + m_nCountInBuffer - unsigned(nOffset);
	}

	ASSERT (nOffset >= m_BufferOffset
			&& nOffset + nCount <= m_BufferOffset + m_nCountInBuffer);
#if 0//def _DEBUG
	// verify that the buffer contains the correct data
	{
		__int16 * pVerBuf = new T[nSamples];
		int ReadSamples = ReadData(pVerBuf, nOffset, nSamples, pSource);
		ASSERT (0 == memcmp(pVerBuf, m_pBuffer + nOffset - m_BufferOffset,
							ReadSamples * sizeof(T)));
		delete[] pVerBuf;
	}
#endif

	* ppBuf = m_pBuffer + (unsigned(nOffset) - unsigned(m_BufferOffset));

	return nCount;
}
