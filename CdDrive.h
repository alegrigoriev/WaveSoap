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

struct CdAddressMSF
{
	UCHAR reserved;
	UCHAR Minute;
	UCHAR Second;
	UCHAR Frame;
	LONG FrameNumber(int FramesPerSecond = 75)
	{
		return Frame + (Second + Minute * 60) * FramesPerSecond;
	}
};

struct BigEndWord
{
	UCHAR num[2];

	BigEndWord & operator =(USHORT src)
	{
		num[0] = UCHAR(src >> 8);
		num[1] = UCHAR(src);
		return * this;
	}
	operator USHORT() { return num[1] | (num[0] << 8); }
};

struct BigEndTriple
{
	UCHAR num[3];
	BigEndTriple & operator =(ULONG src)
	{
		num[0] = UCHAR(src >> 16);
		num[1] = UCHAR(src >> 8);
		num[2] = UCHAR(src);
		return * this;
	}
	operator ULONG()
	{
		return num[2]
				| (num[1] << 8)
				| (num[0] << 16);
	}
};

struct BigEndDword
{
	UCHAR num[4];

	BigEndDword & operator =(ULONG src)
	{
		num[0] = UCHAR(src >> 24);
		num[1] = UCHAR(src >> 16);
		num[2] = UCHAR(src >> 8);
		num[3] = UCHAR(src);
		return * this;
	}
	operator ULONG()
	{
		return num[3]
				| (num[2] << 8)
				| (num[1] << 16)
				| (num[0] << 24);
	}
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
	BigEndDword StartLBA;  // MSB first
	BigEndTriple TransferLength; // MSB first

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

///////////////////////////////////////////////////////////////////////////
// Get Performance command (MMC2)
///////////////////////////////////////////////////////////////////////////
struct CdPerformanceDataHeader
{
	BigEndDword DataLength;
	UCHAR Except:1;
	UCHAR Write:1;
	UCHAR reserved1:6;

	UCHAR reserved2[3];
};

struct CdNominalPerformanceDescriptor
{
	BigEndDword StartLba;
	BigEndDword StartPerformance;
	BigEndDword EndLba;
	BigEndDword EndPerformance;
};

struct GetPerformanceCDB : CD_CDB
{
	enum { OPCODE = 0xAC};

	UCHAR Except:2;
	UCHAR Write:1;
	UCHAR Tolerance:2;
	UCHAR reserved1:3;

	BigEndDword StartingLba;
	UCHAR reserved2[2];
	BigEndWord MaxNumberOfDescriptors;
	UCHAR Reserved3;
	UCHAR Control;

	GetPerformanceCDB(int nNumDescriptors,
					long lStartLba = 0,
					int nExcept = 0,
					BOOL bWrite = FALSE)
	{
		memzero(*this);
		Opcode = OPCODE;
		Tolerance = 2;
		Write = bWrite != 0;
		Except = nExcept;
		MaxNumberOfDescriptors = nNumDescriptors;
		StartingLba = lStartLba;
	}
};

///////////////////////////////////////////////////////////////////////////
// Device inquiry command
///////////////////////////////////////////////////////////////////////////
struct InquiryCDB : CD_CDB
{
	enum { OPCODE = 0x12};
	UCHAR EnableVitalProductData:1;
	UCHAR CmdData:1;
	UCHAR reserved1:6;

	UCHAR PageOrOpcode;
	UCHAR reserved;
	UCHAR AllocationLength;
	UCHAR Control;
	InquiryCDB(int AllocLength, int nPageOrOpcode = 0, bool EVPD = false, bool CmdDt = false)
	{
		memzero(*this);
		Opcode = OPCODE;
		EnableVitalProductData = EVPD;
		CmdData = CmdDt;
		PageOrOpcode = nPageOrOpcode;
		AllocationLength = AllocLength;
	}
};

struct InquiryData
{
	UCHAR PeripheralDeviceType:5;
	UCHAR PeripheralQualifier:3;

	UCHAR reserved1:7;
	UCHAR RemovableMedium:1;

	UCHAR Version;

	UCHAR ResponseDataFormat:4;
	UCHAR HiSup:1;
	UCHAR NormAca:1;
	UCHAR Obsolete1:1;
	UCHAR AsyncEventReporting:1;

	UCHAR AdditionalLength;

	UCHAR reserved2:7;
	UCHAR SCC_Support:1;

	UCHAR ADDR16:1;
	UCHAR Obsolete2:2;
	UCHAR MChanger:1;
	UCHAR MultiPort:1;
	UCHAR VS1:1;
	UCHAR EncServ:1;
	UCHAR BQue:1;

	UCHAR VS2:1;
	UCHAR CmdQue:1;
	UCHAR TranDis:1;
	UCHAR Linked:1;
	UCHAR Sync:1;
	UCHAR WBUS16:1;
	UCHAR Obsolete3:1;
	UCHAR RelAdr:1;

	UCHAR VendorId[8];

	UCHAR ProductID[16];
	UCHAR ProductRevision[4];

	UCHAR VendorSpecific[20];

	UCHAR IUS:1;
	UCHAR QAS:1;
	UCHAR CLOCKING:2;
	UCHAR reserved3:4;

	UCHAR reserved4;

	UCHAR VersionDescriptor[8][2];

	UCHAR reserved5[22];
};

//C_ASSERT(96 == sizeof (InquiryData));
///////////////////////////////////////////////////////////////////////////
// Mode Sense command and Mode pages
///////////////////////////////////////////////////////////////////////////
struct ModeSenseCDB : CD_CDB
{
	enum { OPCODE = 0x1A};
	UCHAR Reserved1:3;
	UCHAR DisableBlockDescriptor:1;
	UCHAR Reserved2:4;

	UCHAR PageCode:6;
	UCHAR PageControl:2;
	enum {PageCurrentValues = 0,
		PageChangeableValues = 1,
		PageDefaultValues = 2,
		PageSavedValues = 3
	};

	UCHAR Reserved3;
	UCHAR AllocationLength;
	UCHAR Control;
	ModeSenseCDB(UCHAR nLength, int nPageCode,
				int nPageControl = PageCurrentValues, bool dbd = true)
	{
		memzero(*this);
		Opcode = OPCODE;
		AllocationLength = nLength;
		PageCode = nPageCode;
		PageControl = nPageControl;
		DisableBlockDescriptor = dbd;
	}
};

struct ModeInfoHeader
{
	UCHAR ModeDataLength;
	UCHAR MediumType;
	UCHAR DeviceSpecific;
	UCHAR BlockDescriptorLength;
};

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

	BigEndWord NumberOf_S_per_M;  // S in M in MSF format (60), MSB first

	BigEndWord NumberOf_F_per_F;  // F in S in MSF format (75), MSB first
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

	BigEndWord RecoveryTimeLimit; // MSB first, should be set to zero
};

// Reduced Multimedia Command Set (obsolete)
struct CDCurrentCapabilitiesModePage
{
	UCHAR PageCode:6;
	enum {Code = 0x0C, Length=0x0E};
	UCHAR Reserved1:1;
	UCHAR PS:1; // parameter savable

	UCHAR PageLength;

	BigEndWord MaximumReadSpeed;
	BigEndWord CurrentReadSpeed;

	UCHAR ReadCDRW:1;
	UCHAR ReadATIP:1;
	UCHAR ReadData:1;
	UCHAR CAV:1;
	UCHAR reserved1:4;

	BigEndWord MaximumWriteSpeed;
	BigEndWord CurrentWriteSpeed;

	UCHAR WriteCDR:1;
	UCHAR WriteCDRW:1;
	UCHAR WriteData:1;
	UCHAR reserved2:5;

	UCHAR reserved3[2];
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
	UCHAR PW_InLeadIn:1;
	UCHAR Reserved6:2;

	BigEndWord MaxReadSpeedSupported;   // obsolete

	BigEndWord NumberOfVolumeLevelsSupported; // MSB first

	BigEndWord BufferSizeSupported;   // MSB first

	BigEndWord CurrentReadSpeedSelected; // obsolete

	UCHAR Reserved9;

	// byte 17:
	UCHAR Reserved10:1;
	UCHAR BCKF:1;
	UCHAR RCK:1;
	UCHAR LSBF:1;
	UCHAR LengthBCKs:2;
	UCHAR Reserved:2;

	BigEndWord MaxWriteSpeedSupported;   // obsolete
	BigEndWord CurrentWriteSpeedSelected; // obsolete
	// may not be available:
	BigEndWord CopyManagementRevision;

	UCHAR Reserved11[2];
};

///////////////////////////////////////////////////////////////////////////
// Get Configuration command and Feature descriptors
///////////////////////////////////////////////////////////////////////////
struct GetConfigurationCDB : CD_CDB
{
	enum { OPCODE = 0x46};
	UCHAR RequestedType:2;
	enum {
		RequestedTypeAllDescriptors = 0,
		RequestedTypeCurrentDescriptors = 1,
		RequestedTypeOneDescriptor = 2};
	UCHAR Reserved2:6;

	BigEndWord StartingFeatureNumber;

	UCHAR Reserved[3];

	BigEndWord AllocationLength;

	UCHAR Control;
	GetConfigurationCDB(USHORT allocLength,
						USHORT StartingFeature = 0, int RequestType = RequestedTypeAllDescriptors)
	{
		memzero(*this);
		Opcode = OPCODE;
		AllocationLength = allocLength;
		StartingFeatureNumber = StartingFeature;
		RequestedType = RequestType;
	}
};

struct FeatureDescriptor
{
	BigEndWord FeatureCode;

	UCHAR Current:1;
	UCHAR Persistent:1;
	UCHAR Version:4;
	UCHAR Reserved:2;

	UCHAR AdditionalLength;
};

struct FeatureHeader
{
	BigEndDword DataLength;    // msb first

	UCHAR Reserved[2];
	BigEndWord CurrentProfile;
	// FeatureDescriptor Descriptor[0];
};

struct RealTimeStreamingFeatureDesc : FeatureDescriptor
{
	enum {Code = 0x0107, AddLength = 0};
};

struct CoreFeatureDesc : FeatureDescriptor
{
	enum {Code = 0x0001, AddLength = 4};
	BigEndDword PhysicalInterfaceStandard; // MSB first
	// 00000001 SCSI family
	// 00000002 ATAPI
	// 00000003 IEEE-1394/1995
	// 00000004 IEEE-1394A
};

struct ProfileListDesc : FeatureDescriptor
{
	enum {Code = 0x0000, AddLength = 4*16};
	struct ProfileDescriptor
	{
		BigEndWord ProfileNumber; // MSB first

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
	enum {Code = 0x001D, AddLength = 0};
};

struct CDReadFeatureDesc : FeatureDescriptor
{
	enum {Code = 0x001E, AddLength = 4};
	UCHAR CDText:1;
	UCHAR C2Flag:1; // supports C2 error pointers
	UCHAR Reserved1:6;

	UCHAR Reserved2[3];
};

struct SerialNumberFeatureDesc : FeatureDescriptor
{
	enum {Code = 0x0108, AddLength = 128};
	UCHAR SerialNumber[128];  // number of bytes is in AdditionalLength
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

	BigEndDword Unformation;
	UCHAR AdditionalSenseLength;

	BigEndDword CommandSpecificInfo;
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
		BigEndWord FieldPointerBytes;
		BigEndWord ActualRetryCount;
		BigEndWord ProgressIndication;
		BigEndWord FieldPointer;
	};
	UCHAR Extra[14];    // total 32
};  // 18 bytes

struct SRB
{
	BYTE        Command;            // ASPI command code
	BYTE        Status;         // ASPI command status byte
	BYTE        HostAdapter;           // ASPI host adapter number
	BYTE        Flags;          // ASPI request flags
	DWORD       dwReserved;       // Reserved, MUST = 0
};

struct SRB_HAInquiry : SRB
{
	BYTE        HA_Count;           // Number of host adapters present
	BYTE        HA_SCSI_ID;         // SCSI ID of host adapter
	BYTE        HA_ManagerId[16];   // String describing the manager
	BYTE        HA_Identifier[16];  // String describing the host adapter
	union {
		BYTE        HA_Unique[16];      // Host Adapter Unique parameters
		struct {
			USHORT BufferAlignment;
			UCHAR Reserved1:1;
			UCHAR ResidualByteCountSupported:1;
			UCHAR MaximumScsiTargets;
			ULONG MaximumTransferLength;
			// the rest is reserved
		};
	};
	WORD        HA_Rsvd1;
};

struct SRB_ExecSCSICmd : SRB
{
	BYTE        Target;         // Target's SCSI ID
	BYTE        Lun;            // Target's LUN number
	WORD        Rsvd1;          // Reserved for Alignment
	DWORD       BufLen;         // Data Allocation Length
	void *      BufPointer;    // Data Buffer Point
	BYTE        SenseLen;       // Sense Allocation Length
	BYTE        CDBLen;         // CDB Length
	BYTE        HostStatus;         // Host Adapter Status
	BYTE        TargetStatus;       // Target Status
	// to be defined: calling convention
	union {
		void        (_cdecl*SRB_PostProc)(SRB_ExecSCSICmd*);  // Post routine
		HANDLE      hCompletionEvent;
	};
	void        *Reserved2;         // Reserved
	BYTE        Reserved3[16];      // Reserved for expansion
	BYTE        CDBByte[16];        // SCSI CDB
	SCSI_SenseInfo  SenseInfo; // Request Sense buffer
};

typedef SRB_HAInquiry *PSRB_HAInquiry;

#define SC_HA_INQUIRY      0

#define SC_GET_DEV_TYPE    1
#define SC_EXEC_SCSI_CMD   2
#define SC_ABORT_SRB       3
#define SC_RESET_DEV       4
#define SC_GET_DISK_INFO   6

#define SRB_POSTING                0x01
#define SRB_ENABLE_RESIDUAL_COUNT  0x04
#define SRB_DIR_IN                 0x08
#define SRB_DIR_OUT                0x10
#define SRB_EVENT_NOTIFY           0x40

#define SS_PENDING         0x00
#define SS_COMP            0x01
#define SS_ABORTED         0x02
#define SS_ABORT_FAIL      0x03
#define SS_ERR             0x04
#define SS_INVALID_HA      0x81
#define SS_INVALID_SRB     0xE0
#define SS_BUFFER_ALIGN    0xE1
#define SS_ASPI_IS_BUSY    0xE5
#define SS_BUFFER_TOO_BIG   0xE6

enum CdMediaChangeState
{
	CdMediaStateNotReady,
	CdMediaStateSameMedia,
	CdMediaStateChanged,
};
class CCdDrive
{
public:
	CCdDrive(BOOL UseAspi = TRUE);
	virtual ~CCdDrive();

	BOOL Open(TCHAR letter);
	void Close();
	static int FindCdDrives(TCHAR Drives['Z' - 'A' + 1]);
	DWORD GetDiskID();

	BOOL GetMaxReadSpeed(int * pMaxSpeed); // bytes/s

	BOOL SetReadSpeed(long BytesPerSec);
	BOOL ReadCdData(void * pBuf, long SectorNum, int nSectors);
	BOOL SetStreaming(long BytesPerSecond);

	CString GetLastScsiErrorText();
	BOOL GetMediaChanged(); // TRUE if disk was changed after previous call
	BOOL EnableMediaChangeDetection();
	BOOL DisableMediaChangeDetection();
	BOOL LockDoor();
	BOOL UnlockDoor();
	BOOL ReadToc(CDROM_TOC * pToc);
	BOOL ReadSessions(CDROM_TOC * pToc);
	void StopAudioPlay();

	CdMediaChangeState CheckForMediaChange();

	BOOL SendScsiCommand(CD_CDB * pCdb, void * pData, DWORD * pDataLen,
						int DataDirection,
						SCSI_SenseInfo * pSense);  // SCSI_IOCTL_DATA_IN, SCSI_IOCTL_DATA_OUT,
	BOOL ScsiInquiry(SRB_HAInquiry * pInq);

	BOOL GetEcMode(BOOL * C2ErrorPointersSupported);

	BOOL StartReading(int speed);

	CCdDrive & operator =(CCdDrive const & Drive);

protected:
	HANDLE m_hDrive;
	HANDLE m_hDriveAttributes;
	TCHAR m_DriveLetter;

	SCSI_ADDRESS m_ScsiAddr;
	IO_SCSI_CAPABILITIES m_ScsiCaps;

	HMODULE m_hWinaspi32;
	ULONG m_MaxTransferSize;
	USHORT m_BufferAlignment;

	DWORD m_MediaChangeCount;

	bool m_bMediaChangeNotificationDisabled;
	bool m_bDoorLocked;

	DWORD (_cdecl * GetASPI32DLLVersion)();
	DWORD (_cdecl * GetASPI32SupportInfo)();
	DWORD (_cdecl * SendASPI32Command)(SRB * lpSRB);


};

#pragma pack(pop)
#endif // !defined(AFX_CDDRIVE_H__444EC2DA_90D7_4205_BD0C_E0A478C802CD__INCLUDED_)
