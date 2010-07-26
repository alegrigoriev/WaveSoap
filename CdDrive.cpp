// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// CdDrive.cpp: implementation of the CCdDrive class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CdDrive.h"
#include <winioctl.h>
#include <Setupapi.h>

struct ReadCD_CDB : CD_CDB
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
	UCHAR Reserved4:5;

	UCHAR Control;
	// constructor:
	ReadCD_CDB(ULONG ReadLba, ULONG Length,
				UCHAR SectorType = SectorTypeCDDA,
				UCHAR Error = ErrorFieldNoError,
				UCHAR Subchannel = SubchannelNone)
	{
		memzero(*this);
		Opcode = OPCODE;
		StartLBA = ReadLba;
		TransferLength = Length;
		ExpectedSectorType = SectorType;
		ErrorField = Error;
		SubchannelSelect = Subchannel;
		UserData = 1;
	}
};

struct ReadCD_Plextor : CD_CDB
{
	enum { OPCODE = 0xD8};
	UCHAR reserved:5;
	UCHAR Lun:3;

	BigEndDword BeginBlock;
	BigEndDword NumBlocks;

	UCHAR Subcode;
	UCHAR Control;

	ReadCD_Plextor(ULONG BeginLba, ULONG nNumBlocks)
	{
		memzero(*this);
		Opcode = OPCODE;
		BeginBlock = BeginLba;
		NumBlocks = nNumBlocks;
	}
};

struct ReadCD_NEC : CD_CDB
{
	enum { OPCODE = 0xD4};
	UCHAR reserved1;

	BigEndDword BeginBlock;
	UCHAR reserved2;
	BigEndWord NumBlocks;

	UCHAR Control;

	ReadCD_NEC(ULONG BeginLba, USHORT nNumBlocks)
	{
		memzero(*this);
		Opcode = OPCODE;
		BeginBlock = BeginLba;
		NumBlocks = nNumBlocks;
	}
};

struct ReadCD_MSF_CDB : CD_CDB
{
	enum { OPCODE = 0xB9};
	UCHAR Reserved1:2;
	UCHAR ExpectedSectorType:3;
	enum{ SectorTypeAllTypes = 0,
		SectorTypeCDDA = 1,
		SectorTypeMode1 = 2,
		SectorTypeMode2Formless = 3,
		SectorTypeMode2Form1 = 4,
		SectorTypeMode2Form2 = 5};
	UCHAR Reserved2:3;

	UCHAR reserved3;
	UCHAR StartMinute;
	UCHAR StartSecond;
	UCHAR StartFrame;
	UCHAR EndMinute;
	UCHAR EndSecond;
	UCHAR EndFrame;

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
	UCHAR Reserved4:5;

	UCHAR Control;
	// constructor:
	ReadCD_MSF_CDB(CdAddressMSF StartAddr, CdAddressMSF EndAddr,
					UCHAR SectorType = SectorTypeCDDA,
					UCHAR Error = ErrorFieldNoError,
					UCHAR Subchannel = SubchannelNone)
	{
		memzero(*this);
		Opcode = OPCODE;

		StartMinute = StartAddr.Minute;
		StartSecond = StartAddr.Second;
		StartFrame = StartAddr.Frame;

		EndMinute = EndAddr.Minute;
		EndSecond = EndAddr.Second;
		EndFrame = EndAddr.Frame;

		ExpectedSectorType = SectorType;
		ErrorField = Error;
		SubchannelSelect = Subchannel;
		UserData = 1;
	}
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

	GetPerformanceCDB(USHORT nNumDescriptors,
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
		PageOrOpcode = UCHAR(nPageOrOpcode);
		AllocationLength = UCHAR(AllocLength);
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

//////////////////////////////////////////////////////////////////
// SET SPEED, SET STREAMING
//////////////////////////////////////////////////////////////////
struct SetCdSpeedCDB : CD_CDB
{
	enum { OPCODE = 0xBB};
	UCHAR Reserved1;

	BigEndWord ReadSpeed;
	BigEndWord WriteSpeed;

	UCHAR Reserved2[5];
	UCHAR Control;

	SetCdSpeedCDB(USHORT nReadSpeed, USHORT nWriteSpeed=0xFFFF)
	{
		memzero(*this);
		Opcode = OPCODE;
		ReadSpeed = nReadSpeed;
		WriteSpeed = nWriteSpeed;
	}
};

struct StreamingPerformanceDescriptor
{
	UCHAR RandomAccess:1;
	UCHAR Exact:1;
	UCHAR RestoreDefaults:1;
	UCHAR Reserved1:5;
	UCHAR Reserved2[3];

	BigEndDword StartLBA;
	BigEndDword EndLBA;

	BigEndDword ReadSize;
	BigEndDword ReadTime;

	BigEndDword WriteSize;
	BigEndDword WriteTime;
	StreamingPerformanceDescriptor(ULONG nStart, ULONG nEnd,
									ULONG ReadSpeed = 0xFFFF, ULONG WriteSpeed=0xFFFF)
	{
		memzero(*this);

		StartLBA = nStart;
		EndLBA = nEnd;

		ReadSize = ReadSpeed;
		ReadTime = 1000;

		WriteSize = WriteSpeed;
		WriteTime = 1000;
	}
};

struct SetStreamingCDB : CD_CDB
{
	enum { OPCODE = 0xB6};
	UCHAR Reserved1;
	UCHAR Reserved2[7];

	BigEndWord ParameterLength;

	UCHAR Control;

	SetStreamingCDB()
	{
		memzero(*this);
		Opcode = OPCODE;
		ParameterLength = sizeof (StreamingPerformanceDescriptor);
	}
};
//////////////////////////////////////////////////////////////////
// UNIT START/STOP
//////////////////////////////////////////////////////////////////
struct StartStopCdb : CD_CDB
{
	enum { OPCODE = 0x1B};
	UCHAR Immediate:1;
	UCHAR reserved1:7;

	UCHAR reserved2[2];

	UCHAR Start:1;
	UCHAR LoadEject:1;
	UCHAR reserved3:2;
	UCHAR PowerConditions:4;

	UCHAR Control;
	enum {
		NoChange = 0,
		Active = 1,
		Idle = 2,
		Standby = 3,
		Sleep = 5,
		IdleTimerToExpire = 0xA,
		StandbyTimerToExpire = 0xB,
	};
	StartStopCdb(int Power, bool bStart = false,
				bool bLoadEject = false, bool bImmed = true)
	{
		memzero(*this);
		Opcode = OPCODE;
		Immediate = bImmed;
		PowerConditions = Power;
		Start = bStart;
		LoadEject = bLoadEject;
	}
};

//////////////////////////////////////////////////////////////////
// Media removal
//////////////////////////////////////////////////////////////////
struct LockMediaCdb : CD_CDB
{
	enum { OPCODE = 0x1E};

	UCHAR reserved1[3];

	UCHAR LockDataTransport:1;
	UCHAR LockMediaChanger:1;
	UCHAR reserved2:6;

	LockMediaCdb(bool LockTransport = true, bool LockChanger = false)
	{
		memzero(*this);
		Opcode = OPCODE;
		LockDataTransport = LockTransport;
		LockMediaChanger = LockChanger;
	}
};

//////////////////////////////////////////////////////////////////
// Media removal
//////////////////////////////////////////////////////////////////
struct ReadTocCdb : CD_CDB
{
	enum { OPCODE = 0x43};

	UCHAR reserved1:1;
	UCHAR MSF:1;
	UCHAR reserved2:6;

	UCHAR DataFormat:4;

	enum {
		TOC = 0,
		SessionInfo = 1,
		FullToc = 2,
		PMA = 3,
		ATIP = 4,
		CDTEXT = 5,
	};
	UCHAR reserved3:4;

	UCHAR reserved4[3];

	UCHAR TrackSessionNumber;

	BigEndWord AllocationLength;

	UCHAR Control;

	ReadTocCdb(int TrackNum, WORD AllocLen, int nFormat=TOC, bool msf=true)
	{
		memzero(*this);
		Opcode = OPCODE;
		MSF = msf;
		DataFormat = nFormat;
		TrackSessionNumber = UCHAR(TrackNum);
		AllocationLength = AllocLen;
	}
};
//////////////////////////////////////////////////////////////////
// Generic SCSI structures
//////////////////////////////////////////////////////////////////

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

//////////////////////////////////////////////////////////////////
// Media removal
//////////////////////////////////////////////////////////////////
struct TestUnitReadyCdb : CD_CDB
{
	enum { OPCODE = 0x00};
	UCHAR reserved[4];
	UCHAR Control;

	TestUnitReadyCdb()
	{
		memzero(*this);
		Opcode = OPCODE;
	}
	static CdMediaChangeState TranslateSenseInfo(SCSI_SenseInfo * pSense);
	static bool CanOpenTray(SCSI_SenseInfo * pSense)
	{
		return pSense->SenseKey == 0
				|| (pSense->SenseKey == 2   // not ready
					&& 0x3A == pSense->AdditionalSenseCode
					&& 1 == pSense->AdditionalSenseQualifier);
	}
	static bool CanCloseTray(SCSI_SenseInfo * pSense)
	{
		return pSense->SenseKey == 2   // not ready
				&& 0x3A == pSense->AdditionalSenseCode
				&& 2 == pSense->AdditionalSenseQualifier;
	}
};
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

struct SRB_Abort : SRB
{
	SRB * pSRBToAbort;
	SRB_Abort(SRB * srb)
	{
		memzero(*this);
		Command = SC_ABORT_SRB;  // 01
		pSRBToAbort = srb;
	}
};

struct SRB_GetDevType : SRB
{
	UCHAR Target;
	UCHAR Lun;
	UCHAR DeviceType;   // filled out
	UCHAR reserved;
	SRB_GetDevType(UCHAR adapter, UCHAR target, UCHAR lun)
	{
		memzero(*this);
		Command = SC_GET_DEV_TYPE;  // 01
		HostAdapter = adapter;
		Target = target;
		Lun = lun;
	}
};

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

static LONG_volatile m_DriveBusyCount['Z' - 'A' + 1];
static LONG_volatile m_MediaLockCount['Z' - 'A' + 1];

/////////////////////////////////////////////////////////////////////
// Include SCSI and CD declarations here, because DDK headers are
// somehow incompatible with VC and SDK headers. If I specify
// DDK include path in the project, it would not compile because of conflicts
////////////////////////////////////////////////////////////////////

#define IOCTL_CDROM_BASE                 FILE_DEVICE_CD_ROM

#define IOCTL_CDROM_UNLOAD_DRIVER        CTL_CODE(IOCTL_CDROM_BASE, 0x0402, METHOD_BUFFERED, FILE_READ_ACCESS)

//
// CDROM Audio Device Control Functions
//

#define IOCTL_CDROM_READ_TOC              CTL_CODE(IOCTL_CDROM_BASE, 0x0000, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_SEEK_AUDIO_MSF        CTL_CODE(IOCTL_CDROM_BASE, 0x0001, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_STOP_AUDIO            CTL_CODE(IOCTL_CDROM_BASE, 0x0002, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_PAUSE_AUDIO           CTL_CODE(IOCTL_CDROM_BASE, 0x0003, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_RESUME_AUDIO          CTL_CODE(IOCTL_CDROM_BASE, 0x0004, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_VOLUME            CTL_CODE(IOCTL_CDROM_BASE, 0x0005, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_PLAY_AUDIO_MSF        CTL_CODE(IOCTL_CDROM_BASE, 0x0006, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_SET_VOLUME            CTL_CODE(IOCTL_CDROM_BASE, 0x000A, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_READ_Q_CHANNEL        CTL_CODE(IOCTL_CDROM_BASE, 0x000B, METHOD_BUFFERED, FILE_READ_ACCESS)
#if (NTDDI_VERSION < NTDDI_WS03)
#define IOCTL_CDROM_GET_CONTROL           CTL_CODE(IOCTL_CDROM_BASE, 0x000D, METHOD_BUFFERED, FILE_READ_ACCESS)
#else
#define OBSOLETE_IOCTL_CDROM_GET_CONTROL  CTL_CODE(IOCTL_CDROM_BASE, 0x000D, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif
#define IOCTL_CDROM_GET_LAST_SESSION      CTL_CODE(IOCTL_CDROM_BASE, 0x000E, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_RAW_READ              CTL_CODE(IOCTL_CDROM_BASE, 0x000F, METHOD_OUT_DIRECT,  FILE_READ_ACCESS)
#define IOCTL_CDROM_DISK_TYPE             CTL_CODE(IOCTL_CDROM_BASE, 0x0010, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_CDROM_GET_DRIVE_GEOMETRY    CTL_CODE(IOCTL_CDROM_BASE, 0x0013, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX CTL_CODE(IOCTL_CDROM_BASE, 0x0014, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_CDROM_READ_TOC_EX           CTL_CODE(IOCTL_CDROM_BASE, 0x0015, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_CONFIGURATION     CTL_CODE(IOCTL_CDROM_BASE, 0x0016, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_CDROM_EXCLUSIVE_ACCESS      CTL_CODE(IOCTL_CDROM_BASE, 0x0017, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_CDROM_SET_SPEED             CTL_CODE(IOCTL_CDROM_BASE, 0x0018, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_GET_INQUIRY_DATA      CTL_CODE(IOCTL_CDROM_BASE, 0x0019, METHOD_BUFFERED, FILE_READ_ACCESS)


// end_winioctl

//
// The following device control codes are common for all class drivers.  The
// functions codes defined here must match all of the other class drivers.
//
// Warning: these codes will be replaced in the future with the IOCTL_STORAGE
// codes included below
//

#define IOCTL_CDROM_CHECK_VERIFY    CTL_CODE(IOCTL_CDROM_BASE, 0x0200, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_MEDIA_REMOVAL   CTL_CODE(IOCTL_CDROM_BASE, 0x0201, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_EJECT_MEDIA     CTL_CODE(IOCTL_CDROM_BASE, 0x0202, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_LOAD_MEDIA      CTL_CODE(IOCTL_CDROM_BASE, 0x0203, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_RESERVE         CTL_CODE(IOCTL_CDROM_BASE, 0x0204, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_RELEASE         CTL_CODE(IOCTL_CDROM_BASE, 0x0205, METHOD_BUFFERED, FILE_READ_ACCESS)
#define IOCTL_CDROM_FIND_NEW_DEVICES CTL_CODE(IOCTL_CDROM_BASE, 0x0206, METHOD_BUFFERED, FILE_READ_ACCESS)

typedef enum _TRACK_MODE_TYPE {
	YellowMode2,
	XAForm2,
	CDDA,
	RawWithC2AndSubCode,   // CD_RAW_SECTOR_WITH_C2_AND_SUBCODE_SIZE per sector
	RawWithC2,             // CD_RAW_SECTOR_WITH_C2_SIZE per sector
	RawWithSubCode         // CD_RAW_SECTOR_WITH_SUBCODE_SIZE per sector
} TRACK_MODE_TYPE, *PTRACK_MODE_TYPE;

#define CD_RAW_READ_C2_SIZE                    (     296   )
#define CD_RAW_READ_SUBCODE_SIZE               (         96)
#define CD_RAW_SECTOR_WITH_C2_SIZE             (2352+296   )
#define CD_RAW_SECTOR_WITH_SUBCODE_SIZE        (2352    +96)
#define CD_RAW_SECTOR_WITH_C2_AND_SUBCODE_SIZE (2352+296+96)

//
// Passed to cdrom to describe the raw read, ie. Mode 2, Form 2, CDDA...
//

typedef struct __RAW_READ_INFO {
	LARGE_INTEGER DiskOffset;
	ULONG    SectorCount;
	TRACK_MODE_TYPE TrackMode;
} RAW_READ_INFO, *PRAW_READ_INFO;


//
// Define values for pass-through DataIn field.
//

#define SCSI_IOCTL_DATA_OUT          0
#define SCSI_IOCTL_DATA_IN           1
#define SCSI_IOCTL_DATA_UNSPECIFIED  2


#define IOCTL_SCSI_BASE                 FILE_DEVICE_CONTROLLER
#define IOCTL_SCSI_PASS_THROUGH         CTL_CODE(IOCTL_SCSI_BASE, 0x0401, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_SCSI_MINIPORT             CTL_CODE(IOCTL_SCSI_BASE, 0x0402, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_SCSI_GET_INQUIRY_DATA     CTL_CODE(IOCTL_SCSI_BASE, 0x0403, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SCSI_GET_CAPABILITIES     CTL_CODE(IOCTL_SCSI_BASE, 0x0404, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_SCSI_PASS_THROUGH_DIRECT  CTL_CODE(IOCTL_SCSI_BASE, 0x0405, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define IOCTL_SCSI_GET_ADDRESS          CTL_CODE(IOCTL_SCSI_BASE, 0x0406, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// SCSI port driver capabilities structure.
//

typedef struct _IO_SCSI_CAPABILITIES {

	//
	// Length of this structure
	//

	ULONG Length;

	//
	// Maximum transfer size in single SRB
	//

	ULONG MaximumTransferLength;

	//
	// Maximum number of physical pages per data buffer
	//

	ULONG MaximumPhysicalPages;

	//
	// Async calls from port to class
	//

	ULONG SupportedAsynchronousEvents;

	//
	// Alignment mask for data transfers.
	//

	ULONG AlignmentMask;

	//
	// Supports tagged queuing
	//

	BOOLEAN TaggedQueuing;

	//
	// Host adapter scans down for bios devices.
	//

	BOOLEAN AdapterScansDown;

	//
	// The host adapter uses programmed I/O.
	//

	BOOLEAN AdapterUsesPio;

} IO_SCSI_CAPABILITIES, *PIO_SCSI_CAPABILITIES;

typedef struct _SCSI_ADDRESS {
	ULONG Length;
	UCHAR PortNumber;
	UCHAR PathId;
	UCHAR TargetId;
	UCHAR Lun;
}SCSI_ADDRESS, *PSCSI_ADDRESS;

//
// Define the SCSI pass through structure.
//

typedef struct _SCSI_PASS_THROUGH {
	USHORT Length;
	UCHAR ScsiStatus;
	UCHAR PathId;
	UCHAR TargetId;
	UCHAR Lun;
	UCHAR CdbLength;
	UCHAR SenseInfoLength;
	UCHAR DataIn;
	ULONG DataTransferLength;
	ULONG TimeOutValue;
	ULONG_PTR DataBufferOffset;
	ULONG SenseInfoOffset;
	UCHAR Cdb[16];
}SCSI_PASS_THROUGH, *PSCSI_PASS_THROUGH;

//
// Define the SCSI pass through direct structure.
//

typedef struct _SCSI_PASS_THROUGH_DIRECT {
	USHORT Length;
	UCHAR ScsiStatus;
	UCHAR PathId;
	UCHAR TargetId;
	UCHAR Lun;
	UCHAR CdbLength;
	UCHAR SenseInfoLength;
	UCHAR DataIn;
	ULONG DataTransferLength;
	ULONG TimeOutValue;
	PVOID DataBuffer;
	ULONG SenseInfoOffset;
	UCHAR Cdb[16];
}SCSI_PASS_THROUGH_DIRECT, *PSCSI_PASS_THROUGH_DIRECT;

typedef struct _TRACK_DATA
{
	UCHAR Reserved;
	UCHAR Control : 4;
	UCHAR Adr : 4;
	UCHAR TrackNumber;
	UCHAR Reserved1;
	UCHAR Address[4];
} TRACK_DATA, *PTRACK_DATA;

#define MAXIMUM_NUMBER_TRACKS 100
#define MAXIMUM_CDROM_SIZE 804
#define MINIMUM_CDROM_READ_TOC_EX_SIZE 2  // two bytes min transferred

typedef struct _CDROM_TOC
{

	//
	// Header
	//

	UCHAR Length[2];  // add two bytes for this field
	UCHAR FirstTrack;
	UCHAR LastTrack;

	//
	// Track data
	//

	TRACK_DATA TrackData[MAXIMUM_NUMBER_TRACKS];
} CDROM_TOC, *PCDROM_TOC;

#define CDROM_TOC_SIZE sizeof(CDROM_TOC)

//
// CD ROM Table OF Contents
// Format 1 - Session Information
//

typedef struct _CDROM_TOC_SESSION_DATA {

	//
	// Header
	//

	UCHAR Length[2];  // add two bytes for this field
	UCHAR FirstCompleteSession;
	UCHAR LastCompleteSession;

	//
	// One track, representing the first track
	// of the last finished session
	//

	TRACK_DATA TrackData[1];

} CDROM_TOC_SESSION_DATA, *PCDROM_TOC_SESSION_DATA;



//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
class CCdDrive : public ICdDrive
{
public:
	CCdDrive(BOOL UseAspi = TRUE);
	CCdDrive(CCdDrive const & Drive, BOOL UseAspi = TRUE);
	virtual ~CCdDrive();

	BOOL Open(TCHAR letter);
	void Close();
	int FindCdDrives(TCHAR Drives['Z' - 'A' + 1]);
	DWORD GetDiskID();

	BOOL GetMaxReadSpeed(int * pMaxSpeed, int * pCurrentSpeed); // bytes/s

	BOOL SetReadSpeed(ULONG BytesPerSec, ULONG BeginLba = 0, ULONG NumSectors = 0);
	BOOL ReadCdData(void * pBuf, long Address, int nSectors);
	BOOL ReadCdData(void * pBuf, CdAddressMSF Address, int nSectors);
	//BOOL SetStreaming(long BytesPerSecond);

	//CString GetLastScsiErrorText();
	//BOOL GetMediaChanged(); // TRUE if disk was changed after previous call
	BOOL EnableMediaChangeDetection(bool Enable = true);
	BOOL DisableMediaChangeDetection()
	{
		return EnableMediaChangeDetection(false);
	}
	BOOL LockDoor(bool Lock = true);
	BOOL UnlockDoor()
	{
		return LockDoor(false);
	}

	BOOL ReadToc();
	virtual BOOL IsTrackCDAudio(unsigned track)
	{
		return 0 == (m_Toc.TrackData[track].Control & 0xC);
	}
	UCHAR GetFirstTrack()
	{
		return m_Toc.FirstTrack;
	}

	UCHAR GetLastTrack()
	{
		return m_Toc.LastTrack;
	}


	UCHAR GetNumberOfTracks()
	{
		return m_Toc.LastTrack - m_Toc.FirstTrack + 1;
	}
	CdAddressMSF GetTrackBegin(unsigned track)
	{
		CdAddressMSF msf;
		msf.Minute = m_Toc.TrackData[track].Address[1];
		msf.Second = m_Toc.TrackData[track].Address[2];
		msf.Frame = m_Toc.TrackData[track].Address[3];
		return msf;
	}

	LONG GetNumSectors(unsigned track)
	{
		return LONG(GetTrackBegin(track+1)) - LONG(GetTrackBegin(track));
	}

	LONG GetTrackNumber(unsigned track)
	{
		return m_Toc.TrackData[track].TrackNumber;
	}

	BOOL ReadSessions();
	void StopAudioPlay();

	bool CanEjectMedia();
	bool CanLoadMedia();
	void EjectMedia();
	void LoadMedia();
	BOOL IsTrayOpen();
	BOOL EjectSupported() const
	{
		return m_bEjectSupported;
	}

	BOOL IsSlotType() const
	{
		return m_bSlotLoading;
	}
	BOOL IsTrayType() const
	{
		return m_bTrayLoading;
	}

	void SetTrayOut(bool Out)
	{
		m_bTrayOut = Out;
	}

	CdMediaChangeState CheckForMediaChange();
	void ForceMountCD();

	BOOL SendScsiCommand(CD_CDB * pCdb, void * pData, DWORD * pDataLen,
						int DataDirection,   // SCSI_IOCTL_DATA_IN, SCSI_IOCTL_DATA_OUT,
						SCSI_SenseInfo * pSense,
						unsigned timeout = 20);
	BOOL ScsiInquiry(struct SRB_HAInquiry * pInq);
	BOOL QueryVendor(CString & Vendor);
	void StopDrive();

	void SetDriveBusy(bool Busy = true);
	bool IsDriveBusy(TCHAR letter) const;
	bool IsDriveBusy() const
	{
		return IsDriveBusy(m_DriveLetter);
	}

	//BOOL GetEcMode(BOOL * C2ErrorPointersSupported);

	//BOOL StartReading(int speed);

	virtual ICdDrive * Clone() const;

protected:
	HANDLE m_hDrive;
	HANDLE m_hDriveAttributes;
	HANDLE m_hEvent;
	TCHAR m_DriveLetter;

	SCSI_ADDRESS m_ScsiAddr;

	HMODULE m_hWinaspi32;
	ULONG m_MaxTransferSize;
	ULONG m_BufferAlignment;

	DWORD m_MediaChangeCount;
	ULONG m_OffsetBytesPerSector;
	CDROM_TOC m_Toc;

	bool m_bScsiCommandsAvailable;
	bool m_bMediaChangeNotificationDisabled;
	bool m_bDoorLocked;
	bool m_bStreamingFeatureSuported;
	bool m_bPlextorDrive;
	bool m_bNECDrive;
	bool m_bUseNonStandardRead;
	bool m_bTrayLoading;
	bool m_bSlotLoading;
	bool m_bEjectSupported;
	bool m_bTrayOut;
	bool m_bDriveBusy;

	void CommonInit(BOOL LoadAspi);
	DWORD (_cdecl * GetASPI32DLLVersion)();
	DWORD (_cdecl * GetASPI32SupportInfo)();
	DWORD (_cdecl * SendASPI32Command)(SRB * lpSRB);
	GETASPI32BUFFER GetAspi32Buffer;
	FREEASPI32BUFFER FreeAspi32Buffer;
	TRANSLATEASPI32ADDRESS TranslateAspi32Address;
	GETASPI32DRIVELETTER GetAspi32DriveLetter;
	GETASPI32HATARGETLUN GetAspi32HaTargetLun;

	CCdDrive & CCdDrive::operator =(CCdDrive const & Drive);
};

void CCdDrive::CommonInit(BOOL LoadAspi)
{
	m_hDrive = NULL;
	m_hDriveAttributes = NULL;
	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_DriveLetter = 0;
	m_hWinaspi32 = NULL;

	GetASPI32DLLVersion = NULL;
	GetASPI32SupportInfo = NULL;
	SendASPI32Command = NULL;
	GetAspi32Buffer = NULL;
	FreeAspi32Buffer = NULL;
	TranslateAspi32Address = NULL;
	GetAspi32DriveLetter = NULL;
	GetAspi32HaTargetLun = NULL;
	m_MaxTransferSize = 0x10000;
	m_BufferAlignment = 1;
	m_MediaChangeCount = ~0UL;
	m_OffsetBytesPerSector = 2048;
	m_bScsiCommandsAvailable = false;
	m_bMediaChangeNotificationDisabled = false;
	m_bStreamingFeatureSuported = true;

	m_bPlextorDrive = false;
	m_bNECDrive = false;
	m_bUseNonStandardRead = false;
	m_bDriveBusy = false;
	m_bTrayLoading = true;
	m_bTrayOut = false;
	m_bSlotLoading = false;
	m_bEjectSupported = true;
	m_bDoorLocked = false;

	memzero(m_ScsiAddr);

	if (LoadAspi)
	{
		m_hWinaspi32 = LoadLibrary(_T("cdral.dll"));

		if (NULL != m_hWinaspi32)
		{
			TRACE("Loaded CDRAL.DLL, getting function pointers\n");
			GetASPI32DLLVersion = (DWORD (_cdecl * )())
								GetProcAddress(m_hWinaspi32, "GetASPI32DLLVersion");
			TRACE("GetASPI32DLLVersion=%p\n", GetASPI32DLLVersion);

			GetASPI32SupportInfo = (DWORD (_cdecl * )())
									GetProcAddress(m_hWinaspi32, "GetASPI32SupportInfo");
			TRACE("GetASPI32SupportInfo=%p\n", GetASPI32SupportInfo);
			if (NULL != GetASPI32SupportInfo)
			{
				TRACE("GetASPI32SupportInfo()=%x\n", GetASPI32SupportInfo());
			}

			SendASPI32Command = (DWORD (_cdecl * )(SRB * ))
								GetProcAddress(m_hWinaspi32, "SendASPI32Command");
			TRACE("SendASPI32Command=%p\n", SendASPI32Command);

			GetAspi32Buffer = (GETASPI32BUFFER)
							GetProcAddress(m_hWinaspi32, "GetASPI32Buffer");
			TRACE("GetAspi32Buffer=%p\n", GetAspi32Buffer);

			FreeAspi32Buffer = (FREEASPI32BUFFER)
								GetProcAddress(m_hWinaspi32, "FreeASPI32Buffer");
			TRACE("FreeAspi32Buffer=%p\n", FreeAspi32Buffer);

			TranslateAspi32Address = (TRANSLATEASPI32ADDRESS)
									GetProcAddress(m_hWinaspi32, "TranslateASPI32Address");
			TRACE("TranslateAspi32Address=%p\n", TranslateAspi32Address);

			GetAspi32DriveLetter = (GETASPI32DRIVELETTER)
									GetProcAddress(m_hWinaspi32, "GetASPI32DriveLetter");
			TRACE("GetAspi32DriveLetter=%p\n", GetAspi32DriveLetter);

			GetAspi32HaTargetLun = (GETASPI32HATARGETLUN)
									GetProcAddress(m_hWinaspi32, "GetASPI32HaTargetLun");
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
									GetProcAddress(m_hWinaspi32, "GetASPI32DLLVersion");
				TRACE("GetASPI32DLLVersion=%p\n", GetASPI32DLLVersion);

				GetASPI32SupportInfo = (DWORD (_cdecl * )())
										GetProcAddress(m_hWinaspi32, "GetASPI32SupportInfo");
				TRACE("GetASPI32SupportInfo=%p\n", GetASPI32SupportInfo);
				if (NULL != GetASPI32SupportInfo)
				{
					TRACE("GetASPI32SupportInfo()=%x\n", GetASPI32SupportInfo());
				}

				SendASPI32Command = (DWORD (_cdecl * )(SRB * ))
									GetProcAddress(m_hWinaspi32, "SendASPI32Command");
				TRACE("SendASPI32Command=%p\n", SendASPI32Command);

				GetAspi32Buffer = (GETASPI32BUFFER)
								GetProcAddress(m_hWinaspi32, "GetASPI32Buffer");

				FreeAspi32Buffer = (FREEASPI32BUFFER)
									GetProcAddress(m_hWinaspi32, "FreeASPI32Buffer");

				TranslateAspi32Address = (TRANSLATEASPI32ADDRESS)
										GetProcAddress(m_hWinaspi32, "TranslateASPI32Address");

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
}

CCdDrive::CCdDrive(CCdDrive const & Drive, BOOL UseAspi)
{
	CommonInit(UseAspi);
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
	m_bStreamingFeatureSuported = Drive.m_bStreamingFeatureSuported;
	m_bPlextorDrive = Drive.m_bPlextorDrive;
	m_bNECDrive = Drive.m_bNECDrive;
	m_bUseNonStandardRead = Drive.m_bUseNonStandardRead;
	return *this;
}

CCdDrive::CCdDrive(BOOL UseAspi)
{
	CommonInit(UseAspi);
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
	if (NULL != m_hEvent
		&& INVALID_HANDLE_VALUE != m_hEvent)
	{
		CloseHandle(m_hEvent);
	}
}

BOOL CCdDrive::Open(TCHAR letter)
{
	BOOL res;
	DWORD bytes =0;

	Close();

	CString path;
	path.Format(_T("\\\\.\\%c:"), letter);

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
		// it is windows9x
		if (0 && NULL != GetAspi32HaTargetLun)
		{
			HaTargetLun Addr = GetAspi32HaTargetLun(UCHAR(letter));
			TRACE("Drive %c SCSI addr returned by cdral=%08X\n",
				letter, Addr);
			m_ScsiAddr.PortNumber = Addr.HaId;
			m_ScsiAddr.Lun = Addr.Lun;
			m_ScsiAddr.TargetId = Addr.TargetId;
		}
		else if (NULL != SendASPI32Command)
		{
			m_ScsiAddr.PortNumber = 0xFF;
			UCHAR TotalAdapters = UCHAR(0xFF & GetASPI32SupportInfo());
			for (UCHAR adapter = 0; adapter < TotalAdapters
				&& 0xFF == m_ScsiAddr.PortNumber
				; adapter++)
			{
				SRB_HAInquiry inq;
				memzero(inq);
				inq.HostAdapter = adapter;
				inq.Command = SC_HA_INQUIRY;

				if (SendASPI32Command( & inq))
				{
					for (UCHAR Target = 0; Target < inq.MaximumScsiTargets; Target++)
					{
						// query LUN 0 only
						SRB_GetDevType gdt(adapter, Target, 0);
						if (SendASPI32Command( & gdt))
						{
							if (gdt.DeviceType == 5)
							{
								TRACE("CD-ROM device at %d:%d\n", adapter, Target);
								m_ScsiAddr.PortNumber = adapter;
								m_ScsiAddr.TargetId = Target;
								m_ScsiAddr.Lun = 0;
							}
						}
					}
				}
			}
			if (0xFF == m_ScsiAddr.PortNumber)
			{
				return FALSE;
			}
		}
		else
		{
			return FALSE;
		}
	}
	else
	{

		m_hDrive = Drive;   // we will needit

		// access 0 is necessary to have READ_ATTRIBUTES privilege
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

		m_hDriveAttributes = Drive;


		IO_SCSI_CAPABILITIES ScsiCaps;

		res = DeviceIoControl(m_hDrive, IOCTL_SCSI_GET_CAPABILITIES,
							NULL, 0,
							& ScsiCaps, sizeof ScsiCaps,
							& bytes, NULL);

		if (res)
		{
			m_MaxTransferSize = ScsiCaps.MaximumTransferLength;
			m_BufferAlignment = ScsiCaps.AlignmentMask;
			TRACE("MaxTransferSize = %d, buffer alignment = %x, \n",
				m_MaxTransferSize, m_BufferAlignment);
		}

		if (NULL != GetAspi32HaTargetLun)
		{
			// find its equivalent in ASPI. The adapter number is supposed to be the same.
			// use either ASPI or IOCTL to find max transfer length
			HaTargetLun Addr = GetAspi32HaTargetLun(UCHAR(letter));
			TRACE("Drive %c SCSI addr returned by cdral=%08X\n",
				letter, Addr);
			m_ScsiAddr.PortNumber = Addr.HaId;
			m_ScsiAddr.Lun = Addr.Lun;
			m_ScsiAddr.TargetId = Addr.TargetId;
		}
		else
		{
			res = DeviceIoControl(m_hDrive, IOCTL_SCSI_GET_ADDRESS,
								NULL, 0,
								& m_ScsiAddr, sizeof m_ScsiAddr,
								& bytes, NULL);

			if ( ! res)
			{
				Close();
				return FALSE;
			}
		}
	}

	m_MediaChangeCount = ~0UL;

	m_DriveLetter = letter;

	m_bScsiCommandsAvailable = true;
	m_bStreamingFeatureSuported = true;
	m_bPlextorDrive = false;
	m_bNECDrive = false;
	m_bUseNonStandardRead = false;

	CString Vendor;
	if (QueryVendor(Vendor))
	{
		TRACE(_T("QueryVendor returned \"%s\"\n"), LPCTSTR(Vendor));
		if (0 == _tcsncmp(Vendor, _T("PLEXTOR"), 7))
		{
			m_bPlextorDrive = true;
		}
		else if (0 == _tcsncmp(Vendor, _T("NEC"), 3))
		{
			m_bNECDrive = true;
		}
	}
	struct MODE: ModeInfoHeader, CDCapabilitiesMechStatusModePage
	{
	} mode;
	ModeSenseCDB msc(sizeof mode, mode.Code);
	bytes = sizeof mode;
	if (SendScsiCommand( & msc, & mode, & bytes, SCSI_IOCTL_DATA_IN, NULL))
	{
		m_bSlotLoading = mode.LoadingMechanismType == 2;
		m_bTrayLoading = mode.LoadingMechanismType == 1;
		m_bEjectSupported = mode.Eject;
	}
	return TRUE;
}

void CCdDrive::Close()
{
	SetDriveBusy(false);
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

BOOL CCdDrive::EnableMediaChangeDetection(bool Enable)
{
	if (Enable == ! m_bMediaChangeNotificationDisabled)
	{
		return TRUE;
	}

	if (NULL == m_hDriveAttributes)
	{
		return FALSE;
	}
	DWORD BytesReturned;
	BOOLEAN McnDisable = ! Enable;

	if (DeviceIoControl(m_hDriveAttributes,IOCTL_STORAGE_MCN_CONTROL,
						& McnDisable, sizeof McnDisable,
						NULL, 0,
						& BytesReturned, NULL))
	{
		m_bMediaChangeNotificationDisabled = ! Enable;
	}

	return TRUE;
}

BOOL CCdDrive::LockDoor(bool Lock)
{
	if (Lock == m_bDoorLocked)
	{
		return TRUE;
	}

	if (NULL == m_hDriveAttributes)
	{
		int DriveIndex;
		if (m_DriveLetter >= 'A'
			&& m_DriveLetter <= 'Z')
		{
			DriveIndex = m_DriveLetter - 'A';
		}
		else if (m_DriveLetter >= 'a'
				&& m_DriveLetter <= 'z')
		{
			DriveIndex = m_DriveLetter - 'a';
		}
		else
		{
			return FALSE;
		}
		// maintain global lock count
		if (Lock)
		{
			if (++m_MediaLockCount[DriveIndex] > 1)
			{
				return TRUE;
			}
		}
		else
		{
			if (--m_MediaLockCount[DriveIndex] != 0)
			{
				return TRUE;
			}
		}

		LockMediaCdb cdb(Lock);
		DWORD Bytes = 0;

		SendScsiCommand(& cdb, & Bytes, & Bytes, SCSI_IOCTL_DATA_UNSPECIFIED, NULL);

		m_bDoorLocked = Lock;
		return TRUE;
	}

	DWORD BytesReturned;
	BOOLEAN LockMedia = Lock;

	if (DeviceIoControl(m_hDriveAttributes, IOCTL_STORAGE_EJECTION_CONTROL,
						& LockMedia, sizeof LockMedia,
						NULL, 0,
						&BytesReturned, NULL))
	{
		m_bDoorLocked = Lock;
	}

	return TRUE;
}

BOOL CCdDrive::SendScsiCommand(CD_CDB * pCdb,
								void * pData, DWORD * pDataLen,
								int DataDirection,   // SCSI_IOCTL_DATA_IN, SCSI_IOCTL_DATA_OUT,
								SCSI_SenseInfo * pSense,
								unsigned timeout)
{
	// issue IOCTL_SCSI_PASS_THROUGH_DIRECT or IOCTL_SCSI_PASS_THROUGH
	UCHAR CdbLength = 6;
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
		memzero(srb);
		srb.Command = SC_EXEC_SCSI_CMD;
		srb.HostAdapter = m_ScsiAddr.PortNumber;
		srb.Target = m_ScsiAddr.TargetId;
		srb.Lun = m_ScsiAddr.Lun;

		ResetEvent(m_hEvent);
		srb.hCompletionEvent = m_hEvent;
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
			//TRACE("Scsi request pending\n");
			if (ERROR_TIMEOUT == WaitForSingleObject(m_hEvent, timeout * 1000))
			{
				SRB_Abort abrt( & srb);
				SendASPI32Command( & abrt);
			}
		}

		if (pSense)
		{
			memcpy(pSense, & srb.SenseInfo, sizeof * pSense);
		}

		TRACE("SendASPI32Command Opcode=0x%02X returned %d, srb.Status=%X, Sense=%x/%x/%x\n",
			pCdb->Opcode, status, srb.Status, srb.SenseInfo.SenseKey,
			srb.SenseInfo.AdditionalSenseCode, srb.SenseInfo.AdditionalSenseQualifier);
		return SS_COMP == srb.Status;
	}
	else
	{
		// try to use IOCTL_SCSI_PASS_TROUGH or
		// IOCTL_SCSI_PASS_TROUGH_DIRECT
		if (*pDataLen <= 512)
		{
			// use IOCTL_SCSI_PASS_TROUGH
			struct OP : SCSI_PASS_THROUGH, SCSI_SenseInfo
			{
				char MoreBuffer[512];
			} spt;

			memzero(static_cast<SCSI_SenseInfo &>(spt));
			spt.Length = sizeof SCSI_PASS_THROUGH;
			spt.PathId = m_ScsiAddr.PathId;
			spt.TargetId = m_ScsiAddr.TargetId;
			spt.Lun = m_ScsiAddr.Lun;

			spt.SenseInfoLength = sizeof SCSI_SenseInfo;
			spt.SenseInfoOffset = sizeof SCSI_PASS_THROUGH;

			memcpy(spt.Cdb, pCdb, CdbLength);

			spt.CdbLength = CdbLength;
			spt.DataIn = UCHAR(DataDirection);
			spt.DataTransferLength = * pDataLen;

			DWORD InLength, OutLength;

			if (SCSI_IOCTL_DATA_OUT == DataDirection)
			{
				memcpy(spt.MoreBuffer, pData, spt.DataTransferLength);
				InLength = offsetof (OP, MoreBuffer) + spt.DataTransferLength;
				OutLength = offsetof (OP, MoreBuffer);
			}
			else
			{
				InLength = sizeof (SCSI_PASS_THROUGH);
				OutLength = offsetof (OP, MoreBuffer) + spt.DataTransferLength;
			}

			spt.DataBufferOffset = offsetof (OP, MoreBuffer);
			spt.TimeOutValue = timeout;

			BOOL res = DeviceIoControl(m_hDrive, IOCTL_SCSI_PASS_THROUGH,
										& spt, InLength,
										& spt, OutLength,
										& bytes, NULL);

			if (res)
			{
				*pDataLen = spt.DataTransferLength;

				if (pSense)
				{
					memcpy(pSense, (SCSI_SenseInfo *) & spt, sizeof * pSense);
				}
				// copy data back:
				if (SCSI_IOCTL_DATA_IN == DataDirection)
				{
					memcpy(pData, spt.MoreBuffer, spt.DataTransferLength);
				}

				return 0 == spt.ScsiStatus;
			}
			else
			{
				if (ERROR_ACCESS_DENIED == GetLastError())
				{
					m_bScsiCommandsAvailable = false;
				}
				TRACE("IOCTL_SCSI_PASS_THROUGH Opcode=0x%02X returned %d, error=%d, Sense=%x/%x\n",
					pCdb->Opcode, res, GetLastError(), spt.SenseKey, spt.AdditionalSenseCode);
				return FALSE;
			}
		}
		else
		{
			// use IOCTL_SCSI_PASS_TROUGH_DIRECT
			struct OP : SCSI_PASS_THROUGH_DIRECT, SCSI_SenseInfo
			{
			} spt;

			spt.Length = sizeof SCSI_PASS_THROUGH_DIRECT;
			spt.PathId = m_ScsiAddr.PathId;
			spt.TargetId = m_ScsiAddr.TargetId;
			spt.Lun = m_ScsiAddr.Lun;

			spt.SenseInfoLength = sizeof SCSI_SenseInfo;
			spt.SenseInfoOffset = sizeof SCSI_PASS_THROUGH_DIRECT;

			memcpy(spt.Cdb, pCdb, CdbLength);

			spt.CdbLength = CdbLength;
			spt.DataIn = UCHAR(DataDirection);

			spt.DataTransferLength = * pDataLen;
			spt.DataBuffer = pData;
			spt.TimeOutValue = timeout;

			BOOL res = DeviceIoControl(m_hDrive, IOCTL_SCSI_PASS_THROUGH_DIRECT,
										& spt, sizeof SCSI_PASS_THROUGH_DIRECT,
										& spt, sizeof spt,
										& bytes, NULL);

			if (res)
			{
				*pDataLen = spt.DataTransferLength;

				if (pSense)
				{
					memcpy(pSense, (SCSI_SenseInfo *) & spt, sizeof * pSense);
				}
				return 0 == spt.ScsiStatus;
			}
			else
			{
				if (ERROR_ACCESS_DENIED == GetLastError())
				{
					m_bScsiCommandsAvailable = false;
				}
				TRACE("IOCTL_SCSI_PASS_THROUGH_DIRECT Opcode=0x%02X returned %d, error=%d, Sense=%x/%x\n",
					pCdb->Opcode, res, GetLastError(), spt.SenseKey, spt.AdditionalSenseCode);

				return FALSE;
			}
		}
//        return FALSE;
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

BOOL CCdDrive::ReadToc()
{
	m_OffsetBytesPerSector = 2048;
	DWORD dwReturned = sizeof m_Toc;
	BOOL res;
	CDROM_TOC toc;
	if (NULL == m_hDrive)
	{
		ReadTocCdb cdb(0, sizeof toc);

		res = SendScsiCommand( & cdb, & toc, & dwReturned, SCSI_IOCTL_DATA_IN, NULL);
		if (res)
		{
			m_Toc = toc;
		}
	}
	else
	{
		res = DeviceIoControl(m_hDrive, IOCTL_CDROM_READ_TOC,
							NULL, 0,
							&m_Toc, sizeof m_Toc,
							& dwReturned,
							NULL);
	}

	TRACE("Get TOC IoControl returned %x, bytes: %d, last error = %d, First track %d, last track: %d, Length:%02X%02X\n",
		res, dwReturned, GetLastError(),
		m_Toc.FirstTrack, m_Toc.LastTrack, m_Toc.Length[1], m_Toc.Length[0]);

	if (res)
	{
		m_bTrayOut = false;
		DISK_GEOMETRY geom;
		if (NULL != m_hDrive
			&& DeviceIoControl(m_hDrive, IOCTL_CDROM_GET_DRIVE_GEOMETRY,
								NULL, 0, & geom, sizeof geom, & dwReturned, NULL)
			&& dwReturned == sizeof geom)
		{
			m_OffsetBytesPerSector = geom.BytesPerSector;
			TRACE("CD returned bytes per sector=%d\n", m_OffsetBytesPerSector);
		}
	}
	return res;
}

BOOL CCdDrive::ReadSessions()
{
	DWORD dwReturned = 0;
	BOOL res = DeviceIoControl(m_hDrive, IOCTL_CDROM_GET_LAST_SESSION,
								NULL, 0,
								&m_Toc, sizeof m_Toc,
								& dwReturned,
								NULL);

	TRACE("Get Last Session IoControl returned %x, bytes: %d, last error = %d, First track %d, last track: %d, Length:%02X%02X\n",
		res, dwReturned, GetLastError(),
		m_Toc.FirstTrack, m_Toc.LastTrack, m_Toc.Length[1], m_Toc.Length[0]);
	return res;
}

void CCdDrive::ForceMountCD()
{
	if (NULL != m_hDrive)
	{
		DWORD MediaChangeCount = 0;
		DWORD BytesReturned;
		DeviceIoControl(m_hDrive,
						IOCTL_STORAGE_CHECK_VERIFY,
						NULL, 0,
						& MediaChangeCount, sizeof MediaChangeCount,
						& BytesReturned, NULL);
	}
}

CdMediaChangeState CCdDrive::CheckForMediaChange()
{
	TestUnitReadyCdb cdb;
	SCSI_SenseInfo sense;
	DWORD bytes = 0;
	memzero(sense);

	if (NULL != m_hDrive)
	{
		DWORD MediaChangeCount = 0;
		DWORD BytesReturned;
		DWORD res = DeviceIoControl(m_hDrive,
									IOCTL_STORAGE_CHECK_VERIFY2,
									NULL, 0,
									& MediaChangeCount, sizeof MediaChangeCount,
									& BytesReturned, NULL);

		DWORD error = GetLastError();

		if (0) TRACE("GetLastError=%d, MediaChange=%d\n",
					error, MediaChangeCount);

		if (! res && error != ERROR_NOT_READY)
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
			if (m_bTrayLoading
				&& m_bScsiCommandsAvailable)
			{
				SendScsiCommand( & cdb, NULL, & bytes,
								SCSI_IOCTL_DATA_UNSPECIFIED, & sense);
				if (cdb.CanCloseTray( & sense))
				{
					m_bTrayOut = true;
				}
				if (cdb.CanOpenTray( & sense))
				{
					m_bTrayOut = false;
				}
			}
			//if (~0UL != m_MediaChangeCount)
			{
				m_MediaChangeCount = ~0UL;
				//TRACE("device not ready\n");
				// check door state
				return CdMediaStateNotReady;
			}
		}
		else if (MediaChangeCount != m_MediaChangeCount)
		{
			m_MediaChangeCount = MediaChangeCount;
			return CdMediaStateDiskChanged;
		}
		else
		{
			return CdMediaStateReady;
		}
	}
	else
	{
		if ( ! SendScsiCommand( & cdb, NULL, & bytes,
								SCSI_IOCTL_DATA_UNSPECIFIED, & sense))
		{
			if (m_bTrayLoading
				&& cdb.CanCloseTray( & sense))
			{
				m_bTrayOut = true;
			}
			if (cdb.CanOpenTray( & sense))
			{
				m_bTrayOut = false;
			}
			return cdb.TranslateSenseInfo( & sense);
		}
		else
		{
			return CdMediaStateReady;
		}
	}
//    return CdMediaStateNotReady;

}

int CCdDrive::FindCdDrives(TCHAR Drives['Z' - 'A' + 1])
{
	int NumberOfDrives = 0;
	for (TCHAR letter = 'A'; letter <= 'Z'; letter++)
	{
		CString s;
		s.Format(_T("%c:"), letter);
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
	_stprintf_s(root, 8, _T("%c:\\"), m_DriveLetter);

	if (GetVolumeInformation(root, NULL, 0, & DiskId,
							& MaxCompLength, & FilesysFlags, NULL, 0))
	{
		TRACE("CD Volume label %08X\n", DiskId);
	}
	return DiskId;
}

BOOL CCdDrive::GetMaxReadSpeed(int * pMaxSpeed, int * pCurrentSpeed) // bytes/s
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

	if (m_bStreamingFeatureSuported
		&& SendScsiCommand( & cdb, & perf, & DataLen, SCSI_IOCTL_DATA_IN, & ssi))
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
		for (unsigned i = 0; i < DataLength / sizeof (CdNominalPerformanceDescriptor); i++)
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
		if (pCurrentSpeed != NULL)
		{
			*pCurrentSpeed = INT_MAX;   // unknown
		}
		return TRUE;
	}
	m_bStreamingFeatureSuported = false;
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
		if (pCurrentSpeed != NULL)
		{
			*pCurrentSpeed = cdmp.CurrentReadSpeedSelected * 1000;
		}
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
		if (pCurrentSpeed != NULL)
		{
			*pCurrentSpeed = cdcp.CurrentReadSpeed * 1000;
		}
		return TRUE;
	}

	return FALSE;

}

void CCdDrive::StopAudioPlay()
{
	if (NULL != m_hDrive)
	{
		DWORD bytes =0;
		DeviceIoControl(m_hDrive, IOCTL_CDROM_STOP_AUDIO,
						NULL, 0,
						NULL, 0,
						& bytes, NULL);
	}
}

BOOL CCdDrive::ReadCdData(void * pBuf, long Address, int nSectors)
{
	DWORD Length = nSectors * CDDASectorSize;
	BOOL res = FALSE;

	if (m_bScsiCommandsAvailable)
	{
		ReadCD_CDB rcd(Address, Length);

		SCSI_SenseInfo ssi;

		if (! m_bUseNonStandardRead)
		{
			res = SendScsiCommand( & rcd, pBuf, & Length,
									SCSI_IOCTL_DATA_IN, & ssi);
			if (res)
			{
				return TRUE;
			}
			TRACE("READ CD error, SenseKey=%d, AdditionalSenseCode=%X\n",
				ssi.SenseKey, ssi.AdditionalSenseCode);
		}
		if (m_bPlextorDrive)
		{
			ReadCD_Plextor rcdpx(Address, nSectors);
			res = SendScsiCommand( & rcdpx, pBuf, & Length,
									SCSI_IOCTL_DATA_IN, & ssi);
		}
		else if (m_bNECDrive)
		{
			ReadCD_NEC rcdNec(Address, USHORT(nSectors));
			res = SendScsiCommand( & rcdNec, pBuf, & Length,
									SCSI_IOCTL_DATA_IN, & ssi);
		}
		return res;
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
	if (res && Length != DWORD(nSectors) * CDDASectorSize)
	{
		TRACE("Incomplete read! %d bytes instead of %d\n", Length, nSectors * CDDASectorSize);
	}
	return res;
}

BOOL CCdDrive::ReadCdData(void * pBuf, CdAddressMSF Address, int nSectors)
{
	DWORD Length = nSectors * CDDASectorSize;
	BOOL res = FALSE;

	if (m_bScsiCommandsAvailable)
	{
		TRACE("ReadCdData using SCSI, ADDR=%d:%02d.%02d\n",
			Address.Minute, Address.Second, Address.Frame);
		CdAddressMSF EndAddress;
		EndAddress = LONG(Address) + nSectors;

		SCSI_SenseInfo ssi;
		if (! m_bUseNonStandardRead)
		{
			ReadCD_MSF_CDB rcd(Address, EndAddress);

			res = SendScsiCommand( & rcd, pBuf, & Length,
									SCSI_IOCTL_DATA_IN, & ssi);
			if (res)
			{
				return TRUE;
			}
			TRACE("READ CD error, SenseKey=%d, AdditionalSenseCode=%X\n",
				ssi.SenseKey, ssi.AdditionalSenseCode);
		}

		if (m_bPlextorDrive)
		{
			ReadCD_Plextor rcdpx(Address - 150, nSectors);
			res = SendScsiCommand( & rcdpx, pBuf, & Length,
									SCSI_IOCTL_DATA_IN, & ssi);
		}
		else if (m_bNECDrive)
		{
			ReadCD_NEC rcdNec(Address - 150, USHORT(nSectors));
			res = SendScsiCommand( & rcdNec, pBuf, & Length,
									SCSI_IOCTL_DATA_IN, & ssi);
		}
		return res;
	}
	else
	{
		TRACE("ReadCdData using IOCTL_CDROM_RAW_READ, ADDR=%d:%02d.%02d\n",
			Address.Minute, Address.Second, Address.Frame);
		RAW_READ_INFO rri;
		rri.SectorCount = nSectors;
		rri.TrackMode = CDDA;
		// 2 seconds (150 sectors) offset between MSF address and logical block address
		rri.DiskOffset.QuadPart = m_OffsetBytesPerSector * (Address - 150);

		res = DeviceIoControl(m_hDrive, IOCTL_CDROM_RAW_READ,
							& rri, sizeof rri,
							pBuf, Length, & Length, NULL);

		if (! res)
		{
			TRACE("READ CD error=%d\n", GetLastError());
		}
	}
	if (Length != DWORD(nSectors) * CDDASectorSize)
	{
		TRACE("Incomplete read! %d bytes instead of %d\n", Length, nSectors * CDDASectorSize);
	}
	return res;
}

BOOL CCdDrive::SetReadSpeed(ULONG BytesPerSec, ULONG BeginLba, ULONG NumSectors)
{
	if (m_bStreamingFeatureSuported)
	{
		StreamingPerformanceDescriptor spd(BeginLba, BeginLba + NumSectors, BytesPerSec / 1024);
		SetStreamingCDB cdb;
		if (0 == NumSectors)
		{
			spd.RestoreDefaults = TRUE;
		}
		DWORD Length = sizeof spd;
		BOOL res = SendScsiCommand( & cdb, & spd, & Length,
									SCSI_IOCTL_DATA_OUT, NULL);
		TRACE("Set Streaming returned %d\n", res);
		if (res)
		{
			return TRUE;
		}
		m_bStreamingFeatureSuported = false;
	}

	SetCdSpeedCDB SetSpeed(USHORT(std::min(BytesPerSec / 1024, 0xFFFFul)));
	DWORD Length = 0;
	BOOL res = SendScsiCommand( & SetSpeed, NULL, & Length,
								SCSI_IOCTL_DATA_UNSPECIFIED, NULL);
	TRACE("Set CD Speed returned %d\n", res);
	return res;
}

BOOL CCdDrive::QueryVendor(CString & Vendor)
{
	InquiryData iqd;
	InquiryCDB iqcdb(sizeof iqd);
	DWORD Length = sizeof iqd;
	BOOL res = SendScsiCommand( & iqcdb, & iqd, & Length,
								SCSI_IOCTL_DATA_IN, NULL);
	if (res)
	{
		Vendor = CString(PSTR(iqd.VendorId), sizeof iqd.VendorId);
		return TRUE;
	}
	return FALSE;

}

void CCdDrive::StopDrive()
{
	DWORD Length = 0;
	StartStopCdb sscbd(StartStopCdb::NoChange, false);
	SendScsiCommand( & sscbd, & Length, & Length,
					SCSI_IOCTL_DATA_UNSPECIFIED, NULL);
}

void CCdDrive::SetDriveBusy(bool Busy)
{
	int DrvIndex;
	if (m_DriveLetter >= 'A'
		&& m_DriveLetter <= 'Z')
	{
		DrvIndex = m_DriveLetter - 'A';
	}
	else if (m_DriveLetter >= 'a'
			&& m_DriveLetter <= 'z')
	{
		DrvIndex = m_DriveLetter - 'a';
	}
	else
	{
		return;
	}
	if (Busy)
	{
		if (m_bDriveBusy)
		{
			return;
		}
		++m_DriveBusyCount[DrvIndex];
	}
	else
	{
		if ( ! m_bDriveBusy)
		{
			return;
		}
		--m_DriveBusyCount[DrvIndex];
	}
	m_bDriveBusy = Busy;
}

bool CCdDrive::IsDriveBusy(TCHAR letter)const
{
	if (letter >= 'A'
		&& letter <= 'Z')
	{
		return m_DriveBusyCount[letter - 'A'] != 0;
	}
	else if (letter >= 'a'
			&& letter <= 'z')
	{
		return m_DriveBusyCount[letter - 'a'] != 0;
	}
	else
	{
		return false;
	}
}

bool CCdDrive::CanEjectMedia()
{
	// true: tray/slot or unknown
	if (NULL == m_hDriveAttributes
		&& ! m_bScsiCommandsAvailable)
	{
		return false;
	}
	return m_bEjectSupported && ! m_bTrayOut;
}

bool CCdDrive::CanLoadMedia()
{
	if (NULL == m_hDriveAttributes
		&& ! m_bScsiCommandsAvailable)
	{
		return false;
	}
	// TRUE - tray mechanism
	// false: all others
	return m_bTrayLoading && m_bTrayOut;
}

void CCdDrive::EjectMedia()
{
	DWORD bytes = 0;
	if (m_hDriveAttributes)
	{
		BOOL res = DeviceIoControl(m_hDrive, IOCTL_STORAGE_EJECT_MEDIA,
									NULL, 0, NULL, 0, & bytes, NULL);
		TRACE("IOCTL_STORAGE_EJECT_MEDIA returned %d, last error=%d\n", res, GetLastError());
		m_bTrayOut = m_bTrayOut || res != 0;
	}
	else if (m_bScsiCommandsAvailable)
	{
		StartStopCdb ssc(StartStopCdb::NoChange, false, true, false);
		m_bTrayOut = 0 != SendScsiCommand( & ssc, NULL, & bytes, SCSI_IOCTL_DATA_UNSPECIFIED, NULL)
					|| m_bTrayOut;
	}
}

void CCdDrive::LoadMedia()
{
	DWORD bytes = 0;
	if (m_hDriveAttributes)
	{
		BOOL res = DeviceIoControl(m_hDrive, IOCTL_STORAGE_LOAD_MEDIA,
									NULL, 0, NULL, 0, & bytes, NULL);
		TRACE("IOCTL_STORAGE_LOAD_MEDIA returned %d, last error=%d\n", res, GetLastError());
	}
	else if (m_bScsiCommandsAvailable)
	{
		StartStopCdb ssc(StartStopCdb::NoChange, true, true, false);
		SendScsiCommand( & ssc, NULL, & bytes, SCSI_IOCTL_DATA_UNSPECIFIED, NULL);
	}
	m_bTrayOut = false;
}

BOOL CCdDrive::IsTrayOpen()
{
	return m_bTrayOut;
}

CdMediaChangeState TestUnitReadyCdb::TranslateSenseInfo(SCSI_SenseInfo * pSense)
{
	if (pSense->SenseKey == 2   // not ready
		&& 0x3A == pSense->AdditionalSenseCode)
	{
		switch (pSense->AdditionalSenseQualifier)
		{
		case 0:
			// medium not present
			break;
		case 1:
			// Medium not present, tray closed
			break;
		case 2:
			// medium not present, tray open
			break;
		case 3:
			// medium not present - loadable (load command allowed?)
			break;
		}
		return CdMediaStateNotReady;
	}
	else if (pSense->SenseKey == 6) // unit attention
	{
		// medium might be changed
		return CdMediaStateDiskChanged;
	}
	return CdMediaStateNotReady;
}

ICdDrive * CreateCdDrive(BOOL UseAspi)
{
	return new CCdDrive(UseAspi);
}

ICdDrive * CCdDrive::Clone() const
{
	CCdDrive * pDrive = new CCdDrive;
	*pDrive = *this;

	return pDrive;
}
