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
	m_hWinaspi32(NULL),
	GetASPI32DLLVersion(NULL),
	GetASPI32SupportInfo(NULL),
	SendASPI32Command(NULL),
	m_bMediaChangeNotificationDisabled(false),
	m_bDoorLocked(false)
{
	memset( & m_Toc, 0, sizeof m_Toc);
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
		Drive = NULL;
		return FALSE;
	}

	DWORD bytes =0;
	BOOL res = DeviceIoControl(Drive, IOCTL_SCSI_GET_ADDRESS,
								NULL, 0,
								& m_ScsiAddr, sizeof m_ScsiAddr,
								& bytes, NULL);
	if ( ! res)
	{
		CloseHandle(Drive);
		return FALSE;
	}
	// find its equivalent in ASPI. The adapter number is suposed to be the same.
	// use either ASPI or IOCTL to find max transfer length

	m_hDrive = Drive;   // we will needit
	if (NULL != m_hWinaspi32)
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
			CloseHandle(m_hDrive);
			m_hDrive = NULL;
			return FALSE;
		}
	}
	else
	{
	}
#if 0
	path.Format("\\\\.\\Scsi%d:", m_ScsiAddr.PortNumber);

	m_hDrive = CreateFile(
						path,
						GENERIC_READ | GENERIC_WRITE,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						0,
						NULL);

	if (INVALID_HANDLE_VALUE == m_hDrive || NULL == m_hDrive)
	{
		m_hDrive = NULL;
		return FALSE;
	}
#endif
	return TRUE;
}

void CCdDrive::Close()
{
	if (NULL != m_hDrive)
	{
		UnlockDoor();
		EnableMediaChangeDetection();
		CloseHandle(m_hDrive);
		m_hDrive = NULL;
	}
}

BOOL CCdDrive::EnableMediaChangeDetection()
{
	return FALSE;
}
BOOL CCdDrive::DisableMediaChangeDetection()
{
	return FALSE;
}
BOOL CCdDrive::LockDoor()
{
	return FALSE;
}
BOOL CCdDrive::UnlockDoor()
{
	return FALSE;
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
	if (* pDataLen <= 1024 - sizeof SCSI_PASS_THROUGH - sizeof SCSI_SenseInfo)
	{
		struct SPT : SCSI_PASS_THROUGH
		{
			ULONG align1;
			SCSI_SenseInfo ssi;
			ULONG align2;
			UCHAR buffer[1024 - sizeof SCSI_PASS_THROUGH - sizeof SCSI_SenseInfo];
		} spt;
		memset( & spt, 0, sizeof spt);
		memcpy( & spt.Cdb, pCdb, CdbLength);
		//CdbLength = 12;
		spt.Length = sizeof SCSI_PASS_THROUGH;
		if (SCSI_IOCTL_DATA_OUT == DataDirection)
		{
			memmove(spt.buffer, pData, *pDataLen);
		}
		spt.TargetId = 1;
		spt.CdbLength = CdbLength;
		spt.DataIn = DataDirection;
		spt.DataTransferLength = * pDataLen;
		spt.DataBufferOffset = offsetof (SPT, buffer);
		spt.SenseInfoLength = sizeof spt.ssi;
		spt.SenseInfoOffset = offsetof (SPT, ssi);

		DWORD BytesReturned = 0;
		BOOL res = DeviceIoControl(m_hDrive, IOCTL_SCSI_PASS_THROUGH,
									& spt, sizeof(SCSI_PASS_THROUGH),
									& spt, spt.DataBufferOffset + spt.DataTransferLength, // return the same back
									& BytesReturned,
									NULL);    // no OVERLAPPED
		*pDataLen = spt.DataTransferLength;
		if (SCSI_IOCTL_DATA_IN == DataDirection)
		{
			memmove(pData, spt.buffer, *pDataLen);
		}
		if (NULL != pSense)
		{
			memcpy(pSense, & spt.ssi, sizeof spt.ssi);
		}
	}
	else
	{
		struct SPTD : SCSI_PASS_THROUGH_DIRECT
		{
			SCSI_SenseInfo ssi;
		}
		sptd;
		memset( & sptd, 0, sizeof sptd);
		memcpy( & sptd.Cdb, pCdb, CdbLength);
		sptd.Length = sizeof sptd;
		sptd.CdbLength = CdbLength;
		sptd.SenseInfoLength = sizeof sptd.ssi;
		sptd.DataIn = DataDirection;
		sptd.DataTransferLength = * pDataLen;
		sptd.DataBuffer = pData;
		sptd.SenseInfoOffset = offsetof (SPTD, ssi);

		DWORD BytesReturned = 0;
		BOOL res = DeviceIoControl(m_hDrive, IOCTL_SCSI_PASS_THROUGH_DIRECT,
									& sptd, sizeof sptd,
									& sptd, sizeof sptd, // return the same back
									& BytesReturned,
									NULL);    // no OVERLAPPED
		*pDataLen = sptd.DataTransferLength;
		if (NULL != pSense)
		{
			memcpy(pSense, & sptd.ssi, sizeof sptd.ssi);
		}
	}

	return TRUE;
}

BOOL CCdDrive::ScsiInquiry(SRB_HAInquiry * pInq)
{
	pInq->Command = SC_HA_INQUIRY;
	pInq->Flags = 0;
	pInq->HostAdapter = m_ScsiAddr.PortNumber;
	DWORD status = SendASPI32Command(pInq);
	return status == SS_COMP;
}
