// CdDrive.cpp: implementation of the CCdDrive class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
//#include "WaveSoapFront.h"
#include "CdDrive.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCdDrive::CCdDrive(BOOL UseAspi)
	: m_hDrive(NULL),
	m_hDriveAttributes(NULL),
	m_DriveLetter(0),
	m_hWinaspi32(NULL),
	GetASPI32DLLVersion(NULL),
	GetASPI32SupportInfo(NULL),
	SendASPI32Command(NULL),
	m_bMediaChangeNotificationDisabled(false),
	m_bDoorLocked(false)
{
	memset( & m_ScsiAddr, 0, sizeof m_ScsiAddr);
	if (UseAspi)
	{
		m_hWinaspi32 = LoadLibrary("wnaspi32.dll");
		if (NULL != m_hWinaspi32)
		{
			GetASPI32DLLVersion = (DWORD (_cdecl * )())
								GetProcAddress(m_hWinaspi32, "GetASPI32DLLVersion");
			GetASPI32SupportInfo = (DWORD (_cdecl * )())
									GetProcAddress(m_hWinaspi32, "GetASPI32SupportInfo");
			SendASPI32Command = (DWORD (_cdecl * )(SRB * ))
								GetProcAddress(m_hWinaspi32, "SendASPI32Command");
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
	Close();
	CString path;
	path.Format("\\\\.\\%c:", letter);

	HANDLE Drive = CreateFile(
							path,
							GENERIC_READ,
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
						0,  // handle with READ_ATTRIBUTES rights
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

	m_hDriveAttributes = Drive;
	m_DriveLetter = letter;

	DWORD bytes =0;
	BOOL res = DeviceIoControl(m_hDrive, IOCTL_SCSI_GET_ADDRESS,
								NULL, 0,
								& m_ScsiAddr, sizeof m_ScsiAddr,
								& bytes, NULL);

	if ( ! res)
	{
		Close();
		return FALSE;
	}
	// find its equivalent in ASPI. The adapter number is suposed to be the same.
	// use either ASPI or IOCTL to find max transfer length
	if (0 && NULL != m_hWinaspi32)
	{
		SRB_HAInquiry inq;
		memset(& inq, 0, sizeof inq);
		if (ScsiInquiry( & inq))
		{
			m_MaxTransferSize = inq.MaximumTransferLength;
			m_BufferAlignment = inq.BufferAlignment;
		}
		else
		{
			Close();
			return FALSE;
		}
	}
	else
	{
		IO_SCSI_CAPABILITIES isc;
		memzero(isc);

		BOOL res = DeviceIoControl(m_hDrive, IOCTL_SCSI_GET_ADDRESS,
									NULL, 0,
									& isc, sizeof isc,
									& bytes, NULL);

		isc.Length = sizeof isc;
		m_MaxTransferSize = isc.MaximumTransferLength;
		m_BufferAlignment = isc.AlignmentMask;

		TRACE("MaxTransferSize = %d, buffer alignment = %x\n",
			m_MaxTransferSize, m_BufferAlignment);
	}
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
	DWORD dwReturned;
	BOOL res = DeviceIoControl(m_hDrive, IOCTL_CDROM_READ_TOC,
								NULL, 0,
								pToc, sizeof *pToc,
								& dwReturned,
								NULL);

	TRACE("Get TOC IoControl returned %x, bytes: %d, First track %d, last track: %d, Length:%02X%02X\n",
		res, dwReturned, pToc->FirstTrack, pToc->LastTrack, pToc->Length[1], pToc->Length[0]);
	return res;
}

CdMediaChangeState CCdDrive::CheckForMediaChange()
{
	if (NULL != m_hDrive)
	{
		DWORD MediaChangeCount = 0;
		DWORD BytesReturned;
		DWORD res = DeviceIoControl(m_hDriveAttributes,
									IOCTL_STORAGE_CHECK_VERIFY2,
									NULL, 0,
									& MediaChangeCount, sizeof MediaChangeCount,
									& BytesReturned, NULL);

		TRACE("GetLastError=%d, MediaChange=%d\n",
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

