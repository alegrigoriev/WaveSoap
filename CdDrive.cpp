// CdDrive.cpp: implementation of the CCdDrive class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
//#include "WaveSoapFront.h"
#include "CdDrive.h"
#include <Setupapi.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void CCdDrive::LoadAspi()
{
	m_hWinaspi32 = LoadLibrary(_T("cdral.dll"));

	if (NULL != m_hWinaspi32)
	{
		TRACE("Loaded CDRAL.DLL, getting function pointers\n");
		GetASPI32DLLVersion = (DWORD (_cdecl * )())
							GetProcAddress(m_hWinaspi32, _T("GetASPI32DLLVersion"));
		TRACE("GetASPI32DLLVersion=%p\n", GetASPI32DLLVersion);

		GetASPI32SupportInfo = (DWORD (_cdecl * )())
								GetProcAddress(m_hWinaspi32, _T("GetASPI32SupportInfo"));
		TRACE("GetASPI32SupportInfo=%p\n", GetASPI32SupportInfo);
		if (NULL != GetASPI32SupportInfo)
		{
			TRACE("GetASPI32SupportInfo()=%x\n", GetASPI32SupportInfo());
		}

		SendASPI32Command = (DWORD (_cdecl * )(SRB * ))
							GetProcAddress(m_hWinaspi32, _T("SendASPI32Command"));
		TRACE("SendASPI32Command=%p\n", SendASPI32Command);

		GetAspi32Buffer = (GETASPI32BUFFER)
						GetProcAddress(m_hWinaspi32, _T("GetASPI32Buffer"));
		TRACE("GetAspi32Buffer=%p\n", GetAspi32Buffer);

		FreeAspi32Buffer = (FREEASPI32BUFFER)
							GetProcAddress(m_hWinaspi32, _T("FreeASPI32Buffer"));
		TRACE("FreeAspi32Buffer=%p\n", FreeAspi32Buffer);

		TranslateAspi32Address = (TRANSLATEASPI32ADDRESS)
								GetProcAddress(m_hWinaspi32, _T("TranslateASPI32Address"));
		TRACE("TranslateAspi32Address=%p\n", TranslateAspi32Address);

		GetAspi32DriveLetter = (GETASPI32DRIVELETTER)
								GetProcAddress(m_hWinaspi32, _T("GetASPI32DriveLetter"));
		TRACE("GetAspi32DriveLetter=%p\n", GetAspi32DriveLetter);

		GetAspi32HaTargetLun = (GETASPI32HATARGETLUN)
								GetProcAddress(m_hWinaspi32, _T("GetASPI32HaTargetLun"));
		TRACE("GetAspi32HaTargetLun=%p\n", GetAspi32HaTargetLun);

		if (NULL == GetASPI32DLLVersion
			|| NULL == GetASPI32SupportInfo
			|| NULL == SendASPI32Command
			|| NULL == GetAspi32DriveLetter
			|| NULL == GetAspi32HaTargetLun
			|| 1 != ((0xFF00 & GetASPI32SupportInfo()) >> 8))
		{
			SendASPI32Command = NULL;
			GetASPI32SupportInfo = NULL;
			GetASPI32DLLVersion = NULL;

			FreeLibrary(m_hWinaspi32);
			m_hWinaspi32 = NULL;
		}
	}

	if (NULL == m_hWinaspi32)
	{
		m_hWinaspi32 = LoadLibrary(_T("wnaspi32.dll"));
		if (NULL != m_hWinaspi32)
		{
			TRACE("Loaded CDRAL.DLL, getting function pointers\n");
			GetASPI32DLLVersion = (DWORD (_cdecl * )())
								GetProcAddress(m_hWinaspi32, _T("GetASPI32DLLVersion"));
			TRACE("GetASPI32DLLVersion=%p\n", GetASPI32DLLVersion);

			GetASPI32SupportInfo = (DWORD (_cdecl * )())
									GetProcAddress(m_hWinaspi32, _T("GetASPI32SupportInfo"));
			TRACE("GetASPI32SupportInfo=%p\n", GetASPI32SupportInfo);
			if (NULL != GetASPI32SupportInfo)
			{
				TRACE("GetASPI32SupportInfo()=%x\n", GetASPI32SupportInfo());
			}

			SendASPI32Command = (DWORD (_cdecl * )(SRB * ))
								GetProcAddress(m_hWinaspi32, _T("SendASPI32Command"));
			TRACE("SendASPI32Command=%p\n", SendASPI32Command);

			GetAspi32Buffer = (GETASPI32BUFFER)
							GetProcAddress(m_hWinaspi32, _T("GetASPI32Buffer"));

			FreeAspi32Buffer = (FREEASPI32BUFFER)
								GetProcAddress(m_hWinaspi32, _T("FreeASPI32Buffer"));

			TranslateAspi32Address = (TRANSLATEASPI32ADDRESS)
									GetProcAddress(m_hWinaspi32, _T("TranslateASPI32Address"));

			GetAspi32DriveLetter = NULL;

			GetAspi32HaTargetLun = NULL;

			if (NULL == GetASPI32DLLVersion
				|| NULL == GetASPI32SupportInfo
				|| NULL == SendASPI32Command
				|| 1 != ((0xFF00 & GetASPI32SupportInfo()) >> 8))
			{
				SendASPI32Command = NULL;
				GetASPI32SupportInfo = NULL;
				GetASPI32DLLVersion = NULL;

				FreeLibrary(m_hWinaspi32);
				m_hWinaspi32 = NULL;
			}
		}
	}

}

CCdDrive::CCdDrive(CCdDrive const & Drive, BOOL UseAspi)
	: m_hDrive(NULL),
	m_hDriveAttributes(NULL),
	m_DriveLetter(0),
	m_hWinaspi32(NULL),

	GetASPI32DLLVersion(NULL),
	GetASPI32SupportInfo(NULL),
	SendASPI32Command(NULL),
	GetAspi32Buffer(NULL),
	FreeAspi32Buffer(NULL),
	TranslateAspi32Address(NULL),
	GetAspi32DriveLetter(NULL),
	GetAspi32HaTargetLun(NULL),
	m_MediaChangeCount(-1),
	m_OffsetBytesPerSector(2048),
	m_bMediaChangeNotificationDisabled(false),
	m_bDoorLocked(false)
{
	if (UseAspi)
	{
		LoadAspi();
	}
	*this = Drive;
}

CCdDrive & CCdDrive::operator =(CCdDrive const & Drive)
{
	m_DriveLetter = Drive.m_DriveLetter;
	m_MediaChangeCount = Drive.m_MediaChangeCount;
	m_ScsiAddr = Drive.m_ScsiAddr;

	if (NULL != Drive.m_hDrive)
	{
		DuplicateHandle(GetCurrentProcess(), Drive.m_hDrive,
						GetCurrentProcess(), & m_hDrive,
						0, FALSE, DUPLICATE_SAME_ACCESS);
	}
	else
	{
		m_hDrive = NULL;
	}
	if (NULL != Drive.m_hDriveAttributes)
	{
		DuplicateHandle(GetCurrentProcess(), Drive.m_hDriveAttributes,
						GetCurrentProcess(), & m_hDriveAttributes,
						0, FALSE, DUPLICATE_SAME_ACCESS);
	}
	else
	{
		m_hDriveAttributes = NULL;
	}

	m_bScsiCommandsAvailable = Drive.m_bScsiCommandsAvailable;
	m_OffsetBytesPerSector = Drive.m_OffsetBytesPerSector;
	return *this;
}

CCdDrive::CCdDrive(BOOL UseAspi)
	: m_hDrive(NULL),
	m_hDriveAttributes(NULL),
	m_DriveLetter(0),
	m_hWinaspi32(NULL),
	GetASPI32DLLVersion(NULL),
	GetASPI32SupportInfo(NULL),
	SendASPI32Command(NULL),
	GetAspi32Buffer(NULL),
	FreeAspi32Buffer(NULL),
	TranslateAspi32Address(NULL),
	GetAspi32DriveLetter(NULL),
	GetAspi32HaTargetLun(NULL),
	m_MaxTransferSize(0x10000),
	m_BufferAlignment(1),
	m_MediaChangeCount(-1),
	m_bScsiCommandsAvailable(false),
	m_bMediaChangeNotificationDisabled(false),
	m_bDoorLocked(false)
{
	memzero(m_ScsiAddr);
	if (UseAspi)
	{
		LoadAspi();
	}
#if 0//def _DEBUG
	SetLastError(0);
	HDEVINFO di = SetupDiGetClassDevs( & GUID_DEVINTERFACE_CDROM,
										NULL, AfxGetMainWnd()->m_hWnd, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	TRACE("HDEVINFO=%X, last error=%d\n", di, GetLastError());

	for (int i = 0; ; i++)
	{
		SP_DEVICE_INTERFACE_DATA spdid;
		spdid.cbSize = sizeof spdid;
		spdid.InterfaceClassGuid = GUID_DEVINTERFACE_CDROM;

		if ( ! SetupDiEnumDeviceInterfaces(di,
											NULL, & GUID_DEVINTERFACE_CDROM,
											i, & spdid))
		{
			TRACE("No more devices, last error=%d\n", GetLastError());
			break;
		}
		TRACE("CDROM Device interface found!\n");
		struct IntDetailData: SP_DEVICE_INTERFACE_DETAIL_DATA
		{
			TCHAR MorePath[256];
		} DetailData;
		DetailData.cbSize = sizeof SP_DEVICE_INTERFACE_DETAIL_DATA;
		DetailData.DevicePath[0] = 0;
		DetailData.DevicePath[1] = 0;
		DWORD FilledSize = 0;;
		SP_DEVINFO_DATA sdi;
		sdi.cbSize = sizeof sdi;

		SetupDiGetDeviceInterfaceDetail(di, &spdid,
										NULL, 0, & FilledSize, NULL);
		TRACE("Required data size=%d\n", FilledSize);

		if ( ! SetupDiGetDeviceInterfaceDetail(di, & spdid,
												& DetailData, sizeof DetailData, NULL, NULL))
		{
			TRACE("SetupDiGetDeviceInterfaceDetail failed, error=%d\n", GetLastError());
			continue;
		}

		HANDLE hFile = CreateFile(DetailData.DevicePath,
								GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL, OPEN_EXISTING,
								0, // flags
								NULL);
		if (NULL == hFile || INVALID_HANDLE_VALUE == hFile)
		{
			TRACE("Failed to open device for READ access, error=%d\n", GetLastError());
		}
		else
		{
			TRACE("Opened device for READ access\n");
			CloseHandle(hFile);
		}
		hFile = CreateFile(DetailData.DevicePath,
							0, FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL, OPEN_EXISTING,
							0, // flags
							NULL);
		if (NULL == hFile || INVALID_HANDLE_VALUE == hFile)
		{
			TRACE("Failed to open device for ATTRIBUTE access, error=%d\n", GetLastError());
		}
		else
		{
			TRACE("Opened device for ATTRIBUTE access\n");
			CloseHandle(hFile);
		}

		hFile = CreateFile(DetailData.DevicePath,
							GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL, OPEN_EXISTING,
							0, // flags
							NULL);
		if (NULL == hFile || INVALID_HANDLE_VALUE == hFile)
		{
			TRACE("Failed to open device for WRITE access, error=%d\n", GetLastError());
		}
		else
		{
			TRACE("Opened device for WRITE access\n");
			CloseHandle(hFile);
		}

		hFile = CreateFile(DetailData.DevicePath,
							GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL, OPEN_EXISTING,
							0, // flags
							NULL);
		if (NULL == hFile || INVALID_HANDLE_VALUE == hFile)
		{
			TRACE("Failed to open device for READ/WRITE access, error=%d\n", GetLastError());
		}
		else
		{
			TRACE("Opened device for READ/WRITE access\n");
			CloseHandle(hFile);
		}

		hFile = CreateFile(DetailData.DevicePath,
							MAXIMUM_ALLOWED, FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL, OPEN_EXISTING,
							0, // flags
							NULL);
		if (NULL == hFile || INVALID_HANDLE_VALUE == hFile)
		{
			TRACE("Failed to open device for MAXIMIM_ALLOWED access, error=%d\n", GetLastError());
		}
		else
		{
			TRACE("Opened device for MAXIMIM_ALLOWED access\n");
			DWORD bytes;

			BOOL res = DeviceIoControl(hFile, IOCTL_SCSI_GET_ADDRESS,
										NULL, 0,
										& m_ScsiAddr, sizeof m_ScsiAddr,
										& bytes, NULL);

			struct OP : SCSI_PASS_THROUGH, SCSI_SenseInfo
			{
				char CdConfig[1024];
			} spt;
			spt.Length = sizeof spt;
			spt.PathId = m_ScsiAddr.PathId;
			spt.TargetId = m_ScsiAddr.TargetId;
			spt.Lun = m_ScsiAddr.Lun;
			spt.SenseInfoLength = sizeof SCSI_SenseInfo;
			spt.SenseInfoOffset = sizeof SCSI_PASS_THROUGH;

			GetConfigurationCDB cdb(sizeof spt.CdConfig);

			memcpy(spt.Cdb, & cdb, sizeof cdb);

			spt.CdbLength = sizeof cdb;
			spt.DataIn = FALSE;

			spt.DataTransferLength = sizeof spt.CdConfig;
			spt.DataBufferOffset = sizeof SCSI_PASS_THROUGH + sizeof SCSI_SenseInfo;
			spt.TimeOutValue = 1000;

			res = DeviceIoControl(hFile, IOCTL_SCSI_PASS_THROUGH,
								& spt, sizeof spt,
								& spt, sizeof spt,
								& bytes, NULL);

			TRACE("IOCTL_SCSI_PASS_THROUGH returned %d, error=%d\n", res, GetLastError());

			CloseHandle(hFile);
		}
	}
	SetupDiDestroyDeviceInfoList(di);
#endif
}

CCdDrive::~CCdDrive()
{
	Close();
	if (NULL != m_hWinaspi32)
	{
		FreeLibrary(m_hWinaspi32);
		m_hWinaspi32 = NULL;
	}
}

BOOL CCdDrive::Open(TCHAR letter)
{
	BOOL res;
	Close();
	CString path;
	path.Format("\\\\.\\%c:", letter);

	// access MAXIMUM_ALLOWED is necessary to have READ_ATTRIBUTES privilege
	HANDLE Drive = CreateFile(
							path,
							MAXIMUM_ALLOWED,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL,
							OPEN_EXISTING,
							0,
							NULL);

	if (INVALID_HANDLE_VALUE == Drive || NULL == Drive)
	{
		return FALSE;
	}

	m_hDrive = Drive;   // we will needit

	Drive = CreateFile(
						path,
						0,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);

	if (INVALID_HANDLE_VALUE == Drive || NULL == Drive)
	{
		Close();
		return FALSE;
	}

	m_hDriveAttributes = Drive;   // we will needit
	m_MediaChangeCount = -1;

	m_DriveLetter = letter;

	m_bScsiCommandsAvailable = true;
	DWORD bytes =0;
	res = DeviceIoControl(m_hDrive, IOCTL_SCSI_GET_ADDRESS,
						NULL, 0,
						& m_ScsiAddr, sizeof m_ScsiAddr,
						& bytes, NULL);

	if ( ! res)
	{
		Close();
		return FALSE;
	}
	// find its equivalent in ASPI. The adapter number is supposed to be the same.
	// use either ASPI or IOCTL to find max transfer length
	IO_SCSI_CAPABILITIES ScsiCaps;
	memzero(ScsiCaps);
	memzero(m_ScsiAddr);

	if (NULL != GetAspi32HaTargetLun)
	{
		HaTargetLun Addr = GetAspi32HaTargetLun(letter);
		TRACE("Drive %c SCSI addr returned by cdral=%08X\n",
			Addr);
		m_ScsiAddr.PortNumber = Addr.HaId;
		m_ScsiAddr.Lun = Addr.Lun;
		m_ScsiAddr.TargetId = Addr.TargetId;
	}
	else
	{
		// WINASPI is used or IOCTL_SCSI_PASS_THROUGH
		res = DeviceIoControl(m_hDrive, IOCTL_SCSI_GET_ADDRESS,
							NULL, 0,
							& m_ScsiAddr, sizeof m_ScsiAddr,
							& bytes, NULL);
	}

	res = DeviceIoControl(m_hDrive, IOCTL_SCSI_GET_CAPABILITIES,
						NULL, 0,
						& ScsiCaps, sizeof ScsiCaps,
						& bytes, NULL);

	m_MaxTransferSize = ScsiCaps.MaximumTransferLength;
	m_BufferAlignment = ScsiCaps.AlignmentMask;

	TRACE("MaxTransferSize = %d, buffer alignment = %x, \n",
		m_MaxTransferSize, m_BufferAlignment
		);

	return TRUE;
}

void CCdDrive::Close()
{
	if (NULL != m_hDriveAttributes)
	{
		UnlockDoor();
		EnableMediaChangeDetection();
		CloseHandle(m_hDriveAttributes);
		m_hDriveAttributes = NULL;
	}

	if (NULL != m_hDrive)
	{
		CloseHandle(m_hDrive);
		m_hDrive = NULL;
	}

	m_DriveLetter = 0;
}

BOOL CCdDrive::EnableMediaChangeDetection()
{
	if ( ! m_bMediaChangeNotificationDisabled)
	{
		return TRUE;
	}

	if (NULL == m_hDriveAttributes)
	{
		return FALSE;
	}
	DWORD BytesReturned;
	BOOLEAN McnDisable = FALSE;

	BOOL res = DeviceIoControl(m_hDriveAttributes,IOCTL_STORAGE_MCN_CONTROL,
								& McnDisable, sizeof McnDisable,
								NULL, 0,
								&BytesReturned, NULL);
	m_bMediaChangeNotificationDisabled = false;

	return TRUE;
}

BOOL CCdDrive::DisableMediaChangeDetection()
{
	if (m_bMediaChangeNotificationDisabled)
	{
		return TRUE;
	}

	if (NULL == m_hDriveAttributes)
	{
		return FALSE;
	}
	DWORD BytesReturned;
	BOOLEAN McnDisable = TRUE;

	BOOL res = DeviceIoControl(m_hDriveAttributes, IOCTL_STORAGE_MCN_CONTROL,
								& McnDisable, sizeof McnDisable,
								NULL, 0,
								&BytesReturned, NULL);
	TRACE("DisableMediaChangeDetection IOCTL returned %d\n", res);
	m_bMediaChangeNotificationDisabled = true;

	return TRUE;
}

BOOL CCdDrive::LockDoor()
{
	if (m_bDoorLocked)
	{
		return TRUE;
	}

	if (NULL == m_hDriveAttributes)
	{
		return FALSE;
	}
	DWORD BytesReturned;
	BOOLEAN LockMedia = TRUE;

	BOOL res = DeviceIoControl(m_hDriveAttributes, IOCTL_STORAGE_EJECTION_CONTROL,
								& LockMedia, sizeof LockMedia,
								NULL, 0,
								&BytesReturned, NULL);

	m_bDoorLocked = true;

	return TRUE;
}

BOOL CCdDrive::UnlockDoor()
{
	if ( ! m_bDoorLocked)
	{
		return TRUE;
	}

	if (NULL == m_hDriveAttributes)
	{
		return FALSE;
	}

	DWORD BytesReturned;
	BOOLEAN LockMedia = FALSE;

	BOOL res = DeviceIoControl(m_hDriveAttributes, IOCTL_STORAGE_EJECTION_CONTROL,
								& LockMedia, sizeof LockMedia,
								NULL, 0,
								&BytesReturned, NULL);

	m_bDoorLocked = false;

	return TRUE;
}

BOOL CCdDrive::SendScsiCommand(CD_CDB * pCdb,
								void * pData, DWORD * pDataLen,
								int DataDirection,
								SCSI_SenseInfo * pSense)  // SCSI_IOCTL_DATA_IN, SCSI_IOCTL_DATA_OUT,
{
	// issue IOCTL_SCSI_PASS_THROUGH_DIRECT or IOCTL_SCSI_PASS_THROUGH
	int CdbLength;
	DWORD bytes;

	if ( ! m_bScsiCommandsAvailable)
	{
		return FALSE;
	}

	switch (pCdb->Opcode >> 5)  // command group
	{
	case 0:
		CdbLength = 6;
		break;
	case 1:
	case 2:
		CdbLength = 10;
		break;
	case 4:
		CdbLength = 16;
		break;
	case 5:
		CdbLength = 12;
		break;
	case 3:
	case 6:
	case 7:
		CdbLength = 16; // unknown??
		break;
	}
	if (NULL != m_hWinaspi32)
	{
		SRB_ExecSCSICmd srb;
		HANDLE hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		memset( & srb, 0, sizeof srb);
		srb.Command = SC_EXEC_SCSI_CMD;
		srb.HostAdapter = m_ScsiAddr.PortNumber;
		srb.Target = m_ScsiAddr.TargetId;
		srb.Lun = m_ScsiAddr.Lun;

		srb.hCompletionEvent = hEvent;
		srb.Flags |= SRB_EVENT_NOTIFY;

		if (SCSI_IOCTL_DATA_IN == DataDirection)
		{
			// to host
			srb.Flags |= SRB_DIR_IN;
		}
		else if (SCSI_IOCTL_DATA_OUT == DataDirection)
		{
			// from host
			srb.Flags |= SRB_DIR_OUT;
		}

		srb.BufLen = * pDataLen;
		srb.BufPointer = pData;
		srb.SenseLen = sizeof srb.SenseInfo;
		srb.CDBLen = CdbLength;
		memcpy(& srb.CDBByte, pCdb, CdbLength);

		DWORD status = SendASPI32Command( & srb);
		if (SS_PENDING == status)
		{
			TRACE("Scsi request pending\n");
			WaitForSingleObject(hEvent, INFINITE);
		}
		CloseHandle(hEvent);
		if (pSense)
		{
			memcpy(pSense, & srb.SenseInfo, sizeof * pSense);
		}

		return SS_COMP == srb.Status;
	}
	else
	{
		// try to use IOCTL_SCSI_PASS_TROUGH or
		// IOCTL_SCSI_PASS_TROUGH_DIRECT
		if (*pDataLen < 256)
		{
			// use IOCTL_SCSI_PASS_TROUGH
			struct OP : SCSI_PASS_THROUGH, SCSI_SenseInfo
			{
				char MoreBuffer[256];
			} spt;

			spt.Length = sizeof SCSI_PASS_THROUGH;
			spt.PathId = m_ScsiAddr.PathId;
			spt.TargetId = m_ScsiAddr.TargetId;
			spt.Lun = m_ScsiAddr.Lun;

			spt.SenseInfoLength = sizeof SCSI_SenseInfo;
			spt.SenseInfoOffset = sizeof SCSI_PASS_THROUGH;

			memcpy(spt.Cdb, pCdb, CdbLength);

			spt.CdbLength = CdbLength;
			spt.DataIn = SCSI_IOCTL_DATA_IN == DataDirection;

			spt.DataTransferLength = * pDataLen;
			spt.DataBufferOffset = offsetof (OP, MoreBuffer);
			spt.TimeOutValue = 1000;

			BOOL res = DeviceIoControl(m_hDrive, IOCTL_SCSI_PASS_THROUGH,
										& spt, sizeof spt,
										& spt, sizeof spt,
										& bytes, NULL);

			TRACE("IOCTL_SCSI_PASS_THROUGH returned %d, error=%d\n", res, GetLastError());

			if (res)
			{
				*pDataLen = spt.DataTransferLength;

				if (pSense)
				{
					memcpy(pSense, (SCSI_SenseInfo *) & spt, sizeof * pSense);
				}
				return SS_COMP == spt.ScsiStatus;
			}
			else
			{
				if (ERROR_ACCESS_DENIED == GetLastError())
				{
					m_bScsiCommandsAvailable = false;
				}
				return FALSE;
			}
		}
		else
		{
		}
		return FALSE;
	}

}

BOOL CCdDrive::ScsiInquiry(SRB_HAInquiry * pInq)
{
	pInq->Command = SC_HA_INQUIRY;
	pInq->Flags = 0;
	pInq->HostAdapter = m_ScsiAddr.PortNumber;
	DWORD status = SendASPI32Command(pInq);
	return status == SS_COMP;
}

BOOL CCdDrive::ReadToc(CDROM_TOC * pToc)
{
	DWORD dwReturned = 0;
	BOOL res = DeviceIoControl(m_hDrive, IOCTL_CDROM_READ_TOC,
								NULL, 0,
								pToc, sizeof *pToc,
								& dwReturned,
								NULL);

	TRACE("Get TOC IoControl returned %x, bytes: %d, last error = %d, First track %d, last track: %d, Length:%02X%02X\n",
		res, dwReturned, GetLastError(),
		pToc->FirstTrack, pToc->LastTrack, pToc->Length[1], pToc->Length[0]);

	m_OffsetBytesPerSector = 2048;
	if (res)
	{
		DISK_GEOMETRY geom;
		if (DeviceIoControl(m_hDrive, IOCTL_CDROM_GET_DRIVE_GEOMETRY,
							NULL, 0, & geom, sizeof geom, & dwReturned, NULL)
			&& dwReturned == sizeof geom)
		{
			m_OffsetBytesPerSector = geom.BytesPerSector;
			TRACE("CD returned bytes per sector=%d\n", m_OffsetBytesPerSector);
		}
	}
	return res;
}

BOOL CCdDrive::ReadSessions(CDROM_TOC * pToc)
{
	DWORD dwReturned = 0;
	BOOL res = DeviceIoControl(m_hDrive, IOCTL_CDROM_GET_LAST_SESSION,
								NULL, 0,
								pToc, sizeof *pToc,
								& dwReturned,
								NULL);

	TRACE("Get Last Session IoControl returned %x, bytes: %d, last error = %d, First track %d, last track: %d, Length:%02X%02X\n",
		res, dwReturned, GetLastError(),
		pToc->FirstTrack, pToc->LastTrack, pToc->Length[1], pToc->Length[0]);
	return res;
}

CdMediaChangeState CCdDrive::CheckForMediaChange()
{
	if (NULL != m_hDrive)
	{
		DWORD MediaChangeCount = 0;
		DWORD BytesReturned;
		DWORD res = DeviceIoControl(m_hDrive,
									IOCTL_STORAGE_CHECK_VERIFY2,
									NULL, 0,
									& MediaChangeCount, sizeof MediaChangeCount,
									& BytesReturned, NULL);

		if (0) TRACE("GetLastError=%d, MediaChange=%d\n",
					GetLastError(),
					MediaChangeCount);

		if (! res && GetLastError() != ERROR_NOT_READY)
		{
			res = DeviceIoControl(m_hDrive,
								IOCTL_STORAGE_CHECK_VERIFY,
								NULL, 0,
								& MediaChangeCount, sizeof MediaChangeCount,
								& BytesReturned, NULL);
			TRACE("GetLastError=%d, MediaChange=%d\n",
				GetLastError(),
				MediaChangeCount);
		}

		if (! res)
		{
			if (-1 != m_MediaChangeCount)
			{
				m_MediaChangeCount = -1;
				TRACE("device not ready\n");
				return CdMediaStateNotReady;
			}
		}
		else if (MediaChangeCount != m_MediaChangeCount)
		{
			m_MediaChangeCount = MediaChangeCount;
			return CdMediaStateChanged;
		}
		else
		{
			return CdMediaStateSameMedia;
		}
	}
	return CdMediaStateNotReady;

}

int CCdDrive::FindCdDrives(TCHAR Drives['Z' - 'A' + 1])
{
	int NumberOfDrives = 0;
	for (int letter = 'A'; letter <= 'Z'; letter++)
	{
		CString s;
		s.Format("%c:", letter);
		if (DRIVE_CDROM == GetDriveType(s))
		{
			Drives[NumberOfDrives] = letter;
			NumberOfDrives++;
		}
	}
	return NumberOfDrives;
}

DWORD CCdDrive::GetDiskID()
{
	DWORD MaxCompLength, FilesysFlags, DiskId = 0;

	TCHAR root[8];
	sprintf(root, "%c:\\", m_DriveLetter);

	if (GetVolumeInformation(root, NULL, 0, & DiskId,
							& MaxCompLength, & FilesysFlags, NULL, 0))
	{
		TRACE("CD Volume label %08X\n", DiskId);
	}
	return DiskId;
}

BOOL CCdDrive::GetMaxReadSpeed(int * pMaxSpeed) // bytes/s
{
	SCSI_SenseInfo ssi;
	DWORD DataLen;
	BOOL res;
	// try to use Get Performance command
	// if the command is not supported, use Set Speed and get the max speed
	// from mech status mode page (offset 8 / 176.4)

	struct PERF : CdPerformanceDataHeader
	{
		CdNominalPerformanceDescriptor desc[10];
	} perf;
	GetPerformanceCDB cdb(10, 0);
	DataLen = sizeof perf;

	if (SendScsiCommand( & cdb, & perf, & DataLen, SCSI_IOCTL_DATA_IN, & ssi))
	{
		// check that header is valid
		const int DataLength = perf.DataLength - 4;
		if (DataLength > 10 * sizeof (CdNominalPerformanceDescriptor)
			|| 0 != DataLength % sizeof (CdNominalPerformanceDescriptor)
			|| 0 == DataLength)
		{
			TRACE("Wrong performance data length=%d\n", DataLength);
			return FALSE;
		}
		TRACE("CD speed obtained by GetPerformanceCDB\n");
		DWORD MaxPerformance = 0;
		for (int i = 0; i < DataLength / sizeof (CdNominalPerformanceDescriptor); i++)
		{
			DWORD tmp = perf.desc[i].StartPerformance;
			if (MaxPerformance < tmp)
			{
				MaxPerformance = tmp;
			}

			tmp = perf.desc[i].EndPerformance;
			if (MaxPerformance < tmp)
			{
				MaxPerformance = tmp;
			}
		}
		* pMaxSpeed = MaxPerformance * 1024;
		return TRUE;
	}
	// use CD MAE current capabilities page
	// use mech status mode page
	struct CdCaps: ModeInfoHeader, CDCapabilitiesMechStatusModePage
	{
	} cdmp;
	ModeSenseCDB msdb(sizeof cdmp, cdmp.Code);
	DataLen = sizeof cdmp;

	res = SendScsiCommand( & msdb, & cdmp, & DataLen, SCSI_IOCTL_DATA_IN, & ssi);

	if (res)
	{
		TRACE("CD speed obtained by CDCapabilitiesMechStatusModePage,current = %d, max=%d\n",
			LONG(cdmp.CurrentReadSpeedSelected), LONG(cdmp.MaxReadSpeedSupported));
		* pMaxSpeed = cdmp.MaxReadSpeedSupported * 1000;
		return TRUE;
	}

	struct CdCaps: ModeInfoHeader, CDCurrentCapabilitiesModePage
	{
	} cdcp;
	ModeSenseCDB msdb2(sizeof cdcp, cdcp.Code);
	DataLen = sizeof cdcp;

	res = SendScsiCommand( & msdb2, & cdcp, & DataLen, SCSI_IOCTL_DATA_IN, & ssi);

	if (res)
	{
		TRACE("CD speed obtained by CDCurrentCapabilitiesModePage,current = %d, max=%d\n",
			LONG(cdcp.CurrentReadSpeed), LONG(cdcp.MaximumReadSpeed));
		* pMaxSpeed = cdcp.MaximumReadSpeed * 1000;
		return TRUE;
	}

	return FALSE;

}

void CCdDrive::StopAudioPlay()
{
	if (NULL != m_hDrive)
	{
		DWORD bytes =0;
		BOOL res = DeviceIoControl(m_hDrive, IOCTL_CDROM_STOP_AUDIO,
									NULL, 0,
									NULL, 0,
									& bytes, NULL);
	}
}

BOOL CCdDrive::ReadCdData(void * pBuf, long Address, int nSectors)
{
	DWORD Length = nSectors * CDDASectorSize;
	BOOL res;
	if (m_bScsiCommandsAvailable)
	{
		ReadCD_CDB rcd(Address, Length);

		SCSI_SenseInfo ssi;

		res = SendScsiCommand( & rcd, pBuf, & Length,
								SCSI_IOCTL_DATA_IN, & ssi);
		if (! res)
		{
			TRACE("READ CD error, SenseKey=%d, AdditionalSenseCode=%X\n",
				ssi.SenseKey, ssi.AdditionalSenseCode);
		}
	}
	else
	{
		RAW_READ_INFO rri;
		rri.SectorCount = nSectors;
		rri.TrackMode = CDDA;
		// when the address is calculated, one sector is 2048 bytes
		rri.DiskOffset.QuadPart = m_OffsetBytesPerSector * Address;
		res = DeviceIoControl(m_hDrive, IOCTL_CDROM_RAW_READ,
							& rri, sizeof rri,
							pBuf, Length, & Length, NULL);

		if (! res)
		{
			TRACE("READ CD error=%d\n", GetLastError());
		}
	}
	if (res && Length != nSectors * CDDASectorSize)
	{
		TRACE("Incomplete read! %d bytes instead of %d\n", Length, nSectors * CDDASectorSize);
	}
	return res;
}

BOOL CCdDrive::ReadCdData(void * pBuf, CdAddressMSF Address, int nSectors)
{
	DWORD Length = nSectors * CDDASectorSize;
	BOOL res;
	if (m_bScsiCommandsAvailable)
	{
		CdAddressMSF EndAddress;
		EndAddress = LONG(Address) + nSectors;
		ReadCD_MSF_CDB rcd(Address, EndAddress);
		SCSI_SenseInfo ssi;

		res = SendScsiCommand( & rcd, pBuf, & Length,
								SCSI_IOCTL_DATA_IN, & ssi);
		if (! res)
		{
			TRACE("READ CD error, SenseKey=%d, AdditionalSenseCode=%X\n",
				ssi.SenseKey, ssi.AdditionalSenseCode);
		}
	}
	else
	{
		RAW_READ_INFO rri;
		rri.SectorCount = nSectors;
		rri.TrackMode = CDDA;
		// 2 seconds (150 sectors) offset between MSF address and logical block address
		rri.DiskOffset.QuadPart = m_OffsetBytesPerSector * (LONG(Address) - 150);

		res = DeviceIoControl(m_hDrive, IOCTL_CDROM_RAW_READ,
							& rri, sizeof rri,
							pBuf, Length, & Length, NULL);

		if (! res)
		{
			TRACE("READ CD error=%d\n", GetLastError());
		}
	}
	if (Length != nSectors * CDDASectorSize)
	{
		TRACE("Incomplete read! %d bytes instead of %d\n", Length, nSectors * CDDASectorSize);
	}
	return res;
}

