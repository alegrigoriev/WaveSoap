// WaveFile.h
#ifndef WAVE_FILE__H__
#define WAVE_FILE__H__
/*
Class CWaveFile serves as a WAV file input/output
helper.
*/
#ifndef __AFX_H__
 #include <afx.h>
#endif

class CMmioFile : public CFile
{
public:
	// construction
	CMmioFile();
	CMmioFile(int hFile);
	CMmioFile( LPCTSTR lpszFileName, UINT nOpenFlags );
	virtual BOOL Open( LPCTSTR lpszFileName, UINT nOpenFlags, CFileException* pError = NULL );
	virtual void Close( );

	// read/write:
	virtual UINT Read( void* lpBuf, UINT nCount );
	virtual void Write( const void* lpBuf, UINT nCount );
	virtual void Flush( );

	~CMmioFile();
};
#endif //#ifndef WAVE_FILE__H__
