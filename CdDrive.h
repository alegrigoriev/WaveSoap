// CdDrive.h: interface for the CCdDrive class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CDDRIVE_H__444EC2DA_90D7_4205_BD0C_E0A478C802CD__INCLUDED_)
#define AFX_CDDRIVE_H__444EC2DA_90D7_4205_BD0C_E0A478C802CD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <devioctl.h>
#include <ntddcdrm.h>
#include <winioctl.h>
#include <ntddscsi.h>

#pragma pack(push, 1)

struct CD_CDB
{
	UCHAR Opcode;
};

struct ReadCD_Cdb : CD_CDB
{
	enum { OPCODE = 0xBE};
	UCHAR Reladdr:1;    // set to 0
	UCHAR Reserved1:1;
	UCHAR ExpectedSectorType:3;
	enum{ SectorTypeAllTypes = 0,
		SectorTypeCDDA = 1,
		SectorTypeMode1 = 2,
		SectorTypeMode2Formless = 3,
		SectorTypeMode2Form1 = 4,
		SectorTypeMode2Form2 = 5};
	UCHAR Reserved2:3;
	UCHAR StartLBA[4];  // MSB first
	UCHAR TransferLength[3]; // MSB first

	UCHAR Reserved3:1;
	UCHAR ErrorField:2;
	enum { ErrorFieldNoError = 0,
		ErrorFieldC2 = 1,   // return 2352 error bits
		ErrorFieldC2AndBlockError = 2, };
	UCHAR EccEdc:1;     // set to 1 if you want ECC EDC for data CD
	UCHAR UserData:1;   // set to 1 to return data
	UCHAR HeaderCodes:2;
	UCHAR Sync:1;       // set to 1 if you want Sync field from the sector

	UCHAR SubchannelSelect:3;
	enum { SubchannelNone = 0,
		SubchannelRAW = 1,
		SubchannelQ = 2,
		SubchannelPW = 4, };
	//UCHAR Reserved4:5;

	UCHAR Control;
};

struct GetConfigurationCDB : CD_CDB
{
	enum { OPCODE = 0x46};
	UCHAR RequestedType:2;
	enum {
		RequestedTypeAllDescriptors = 0,
		RequestedTypeCurrentDescriptors = 1,
		RequestedTypeOneDescriptor = 2};
	UCHAR Reserved2:6;

	UCHAR StartingFeatureNumber[2]; // MSB first

	UCHAR Reserved[3];

	UCHAR AllocationLength[2];  // MSB first

	UCHAR Control;  // ??
};

struct SCSI_SenseInfo
{
	UCHAR ResponseCode:7;
	enum { CurrentErrors=0x70, DeferredErrors=0x71 };
	UCHAR Valid:1;

	UCHAR SegmentNumber;

	UCHAR SenseKey:4;
	UCHAR Reserved1:1;
	UCHAR IncorrectLengthIndicator:1;
	UCHAR EndOfMedium:1;
	UCHAR Filemark:1;

	UCHAR Unformation[4];
	UCHAR AdditionalSenseLength;

	UCHAR CommandSpecificInfo[4];
	UCHAR AdditionalSenseCode;
	UCHAR AdditionalSenseQualifier;
	UCHAR FieldReplaceableUnitCode;

	UCHAR BitPointer:3;
	UCHAR BitPointerValid:1;
	UCHAR Reserved2:1;
	UCHAR SegmentDescriptor:1;
	UCHAR CommandOrData:1;
	UCHAR SenseKeySpecificValid:1;
	union {
		UCHAR FieldPointerBytes[2];
		UCHAR ActualRetryCount[2];
		UCHAR ProgressIndication[2];
		UCHAR FieldPointer[2];
	};
	UCHAR Extra[14];    // total 32
};  // 18 bytes
struct CDParametersModePage
{
	UCHAR PageCode:6;
	enum {Code = 0x0D, Length=6};
	UCHAR Reserved1:1;
	UCHAR PS:1; // parameter savable

	UCHAR PageLength;

	UCHAR Reserved2;

	UCHAR InactivityTimerMultiplier:4;
	UCHAR Reserved3:4;

	UCHAR NumberOf_S_per_M[2];  // S in M in MSF format (60), MSB first

	UCHAR NumberOf_F_per_F[2];  // F in S in MSF format (75), MSB first
};

struct CDErrorRecoveryModePage
{
	UCHAR PageCode:6;
	enum {Code = 1, Length=0xA};
	UCHAR Reserved1:1;
	UCHAR PS:1; // parameter savable

	UCHAR PageLength;

	UCHAR DCR:1;
	UCHAR DTE:1;
	UCHAR PER:1;
	UCHAR Reserved2:1;
	UCHAR RC:1;
	UCHAR TB:1;
	UCHAR ARRE:1;
	UCHAR ARWE:1;

	UCHAR ReadRetryCount;

	UCHAR Reserved3[4];

	UCHAR WriteRetryCount;

	UCHAR Reserved4;

	UCHAR RecoveryTimeLimit[2]; // MSB first, should be set to zero
};
// see T10/1228-D, NCITS 333, clause 5.5.10 table 137
struct CDCapabilitiesMechStatusModePage
{
	UCHAR PageCode:6;
	enum {Code = 0x2A, Length=0x18};
	UCHAR Reserved1:1;
	UCHAR PS:1; // parameter savable

	UCHAR PageLength;

	UCHAR CDRRead:1;
	UCHAR CDRWRead:1;
	UCHAR Method2:1;
	UCHAR DVDROMRead:1;
	UCHAR DVDR_Read:1;
	UCHAR DVDRAMRead:1;
	UCHAR Reserved2:2;

	UCHAR CDR_Write:1;
	UCHAR CDRW_Write:1;
	UCHAR TestWrite:1;
	UCHAR Reserved3:1;
	UCHAR DVDR_Write:1;
	UCHAR DVDRAM_Write:1;
	UCHAR Reserved3a:2;

	UCHAR AudioPlay:1;
	UCHAR Composite:1;
	UCHAR DigitalPort_1:1;
	UCHAR DigitalPort_2:1;
	UCHAR Mode2Form1:1;
	UCHAR Mode2Form2:1;
	UCHAR Multisession:1;
	UCHAR Reserved4:1;

	UCHAR CDDACommandsSupported:1;
	UCHAR CDDAStreamAccurate:1;
	UCHAR RWChanSupport:1;
	UCHAR RWDeinterleavedCorrected:1;
	UCHAR C2PointersSupported:1;
	UCHAR ISRC:1;
	UCHAR UPC:1;
	UCHAR ReadBarCode:1;

	UCHAR Lock:1;
	UCHAR LockState:1;
	UCHAR PreventJumper:1;
	UCHAR Eject:1;  // supported
	UCHAR Reserved5:1;
	UCHAR LoadingMechanismType:3;

	UCHAR SeparateVolumeLevels:1;
	UCHAR SeparateChannelMute:1;
	UCHAR ChangerSupportsDiskPresent:1;
	UCHAR SW_SlotSelection:1;
	UCHAR SideChangeCapable:1;
	UCHAR PW_InLeaiIn:1;
	UCHAR Reserved6:2;

	UCHAR Reserved7[2];

	UCHAR NumberOfVolumeLevelsSupported[2]; // MSB first

	UCHAR BufferSizeSupported[2];   // MSB first

	UCHAR Reserved8[2];

	UCHAR Reserved9;

	// byte 17:
	UCHAR Reserved10:1;
	UCHAR BCKF:1;
	UCHAR RCK:1;
	UCHAR LSBF:1;
	UCHAR LengthBCKs:2;
	UCHAR Reserved:2;

	UCHAR Obsolete[4];

	UCHAR CopyManagementRevision[2];

	UCHAR Reserved11[2];
};

struct FeatureDescriptor
{
	UCHAR FeatureCode[2];

	UCHAR Current:1;
	UCHAR Persistent:1;
	UCHAR Version:4;
	UCHAR Reserved:2;

	UCHAR AdditionalLength;
};

struct FeatureHeader
{
	UCHAR DataLength[4];    // msb first

	UCHAR Reserved[2];
	UCHAR CurrentProfile[2];
	// FeatureDescriptor Descriptor[0];
};

struct RealTimeStreamingFeatureDesc : FeatureDescriptor
{
	enum {Code0 = 0x01, Code1 = 0x07, AddLength = 0};
};

struct CoreFeatureDesc : FeatureDescriptor
{
	enum {Code0 = 0x00, Code1 = 0x01, AddLength = 4};
	UCHAR PhysicalInterfaceStandard[4]; // MSB first
	// 00000001 SCSI family
	// 00000002 ATAPI
	// 00000003 IEEE-1394/1995
	// 00000004 IEEE-1394A
};

struct ProfileListDesc : FeatureDescriptor
{
	enum {Code0 = 0x00, Code1 = 0x00, AddLength = 4*16};
	struct ProfileDescriptor
	{
		UCHAR ProfileNumber[2]; // MSB first

		UCHAR CurrentP:1;
		UCHAR Reserved1:7;

		UCHAR Reserved2;
	}
	Descriptors[16];
};

enum {
	ProfileLsbCDROM = 0x08,
	ProfileLsbCDR = 0x09,
	ProfileLsbCDRW = 0x0A,
	ProfileLsbDVDROM = 0x10,
	ProfileLsbDVDR = 0x11,
	ProfileLsbDVDRAMRW = 0x12
};


struct MultiReadFeatureDesc : FeatureDescriptor
{
	enum {Code0 = 0x00, Code1 = 0x1D, AddLength = 0};
};

struct CDReadFeatureDesc : FeatureDescriptor
{
	enum {Code0 = 0x00, Code1 = 0x1E, AddLength = 4};
	UCHAR CDText:1;
	UCHAR C2Flag:1; // supports C2 error pointers
	UCHAR Reserved1:6;

	UCHAR Reserved2[3];
};

struct SerialNumberFeatureDesc : FeatureDescriptor
{
	enum {Code0 = 0x01, Code1 = 0x08, AddLength = 128};
	UCHAR SerialNumber[128];  // number of bytes is in AdditionalLength
};

class CCdDrive
{
public:
	CCdDrive();
	virtual ~CCdDrive();
	BOOL Open(TCHAR letter);
	void Close();
	long GetMaxReadSpeed(); // bytes/s
	long GetMinReadSpeed();
	BOOL SetReadSpeed(long BytesPerSec);
	BOOL ReadCdData(void * pBuf, long SectorNum, int nSectors);
	BOOL SetStreaming(long BytesPerSecond);
	CString GetLastScsiErrorText();
	BOOL GetMediaChanged(); // TRUE if disk was changed after previous call
	BOOL EnableMediaChangeDetection();
	BOOL DisableMediaChangeDetection();
	BOOL LockDoor();
	BOOL UnlockDoor();

	BOOL SendAtapiCommand(CD_CDB * pCdb, void * pData, DWORD * pDataLen,
						int DataDirection,
						SCSI_SenseInfo * pSense);  // SCSI_IOCTL_DATA_IN, SCSI_IOCTL_DATA_OUT,
private:
	HANDLE m_hDrive;
	SCSI_ADDRESS m_ScsiAddr;
	HMODULE m_hWinaspi32;
	bool m_bReserved;
	bool m_bMediaChangeNotificationDisabled;
	bool m_bDoorLocked;
	CDROM_TOC m_Toc;
	DWORD (_cdecl * GetASPI32DLLVersion)();
	DWORD (_cdecl * GetASPI32SupportInfo)();
	DWORD (_cdecl * SendASPI32Command)(PVOID lpSRB);

};

#pragma pack(pop)
#endif // !defined(AFX_CDDRIVE_H__444EC2DA_90D7_4205_BD0C_E0A478C802CD__INCLUDED_)
