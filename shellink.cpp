// shellink.cpp
#include <stdafx.h>
#include "shellink.h"
#include <shlobj.h>

CString ResolveShellLink(LPCTSTR file)
{
	WIN32_FIND_DATA fd;
	IPersistFile * pPersistFile = NULL;
	IShellLink * pShellLink = NULL;
	LPCOLESTR   lpOleStrLinkName;
	TCHAR szPath[MAX_PATH + 1];
#ifndef UNICODE
	WCHAR wszPath[MAX_PATH + 1];
#endif
	fd.cFileName[0] = 0;
	szPath[0] = 0;

	CString result;

	if (SUCCEEDED(CoInitialize(NULL)))
	{
		// create an instance of shell link interface
		if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, NULL,
										CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID *) & pShellLink)))
		{
			if (SUCCEEDED(pShellLink->QueryInterface(IID_IPersistFile,
													(LPVOID *) & pPersistFile)))
			{
				// convert the link name to unicode if necessary
#ifdef UNICODE
				lpOleStrLinkName = file;
#else
				lpOleStrLinkName = wszPath;
				if (0 != MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
											file, -1,
											wszPath, sizeof wszPath / sizeof wszPath[0]))
#endif
				if (SUCCEEDED(pPersistFile->Load(lpOleStrLinkName, 0))
					&& SUCCEEDED(pShellLink->Resolve(NULL, SLR_NO_UI))
					&& S_OK == pShellLink->GetPath(szPath,
													countof(szPath), & fd, 0))
				{
					result = szPath;
				}
				pPersistFile->Release();
				pPersistFile = NULL;
			}
			pShellLink->Release();
			pShellLink = NULL;
		}
		CoUninitialize();
	}
	// if any error occured, the result string is empty
	return result;
}

CString ResolveIfShellLink(LPCTSTR file)
{
	int index = _tcslen(file) - 4;
	if (index >= 0
		&& 0 == _tcsicmp(file + index, _T(".lnk")))
	{
		return ResolveShellLink(file);
	}
	return file;
}

