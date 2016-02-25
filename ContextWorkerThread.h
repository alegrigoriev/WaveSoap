#pragma once
#include "KListEntry.h"

class CContextWorkerThread : public CWinThread
{
public:
	CContextWorkerThread(CThisApp * pApp, int Priority = THREAD_PRIORITY_NORMAL);
	~CContextWorkerThread();
	void Kick();	// set the event
	bool Start();
	bool Stop();
	void SetForegroundDocument(class CDocument* pDoc);
	void QueueOperation(class COperationContext * pContext);

private:
	static UINT AFX_CDECL ThreadProc(PVOID arg)
	{
		return ((CContextWorkerThread *)arg)->_ThreadProc();
	}
	unsigned _ThreadProc();
	bool volatile m_RunThread;		// false, when needs to stop
	int m_Priority;
	HANDLE m_hThreadEvent;
	CThisApp * m_pApp;
	CDocument * m_pForegroundDocument;
	LockedListHead<COperationContext> m_OpList;

};
