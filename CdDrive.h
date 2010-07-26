// Copyright Alexander Grigoriev, 1997-2002, All Rights Reserved
// CdDrive.h: interface for the ICdDrive class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CDDRIVE_H__444EC2DA_90D7_4205_BD0C_E0A478C802CD__INCLUDED_)
#define AFX_CDDRIVE_H__444EC2DA_90D7_4205_BD0C_E0A478C802CD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "KInterlocked.h"

#define SRB_HAInquiry    _SRB_HAInquiry
#define PSRB_HAInquiry   _PSRB_HAInquiry
#define SRB_ExecSCSICmd  _SRB_ExecSCSICmd
#define PSRB_ExecSCSICmd _PSRB_ExecSCSICmd
#define SRB_Abort        _SRB_Abort
#define PSRB_Abort       _PSRB_Abort
#define SRB_BusDeviceReset _SRB_BusDeviceReset
#define PSRB_BusDeviceReset _PSRB_BusDeviceReset
#define SRB_GetDiskInfo     _SRB_GetDiskInfo
#define PSRB_GetDiskInfo    _PSRB_GetDiskInfo

#include "wnaspi32.h"

#undef  SRB_HAInquiry
#undef PSRB_HAInquiry
#undef SRB_ExecSCSICmd
#undef PSRB_ExecSCSICmd
#undef SRB_Abort
#undef PSRB_Abort
#undef SRB_BusDeviceReset
#undef PSRB_BusDeviceReset
#undef SRB_GetDiskInfo
#undef PSRB_GetDiskInfo

#pragma pack(push, 1)

enum CdMediaChangeState
{
	CdMediaStateUnknown,
	CdMediaStateNotReady,
	CdMediaStateReady,
	CdMediaStateDiskChanged,
	CdMediaStateBusy,
	CdMediaStateNoDrives,
};

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
	LONG SectorNumber(int FramesPerSecond = 75) const
	{
		return Frame + (Second + Minute * 60) * FramesPerSecond;
	}
	operator LONG() const
	{
		return Frame + (Second + Minute * 60) * 75;
	}

	CdAddressMSF & operator =(LONG sector)
	{
		Frame = UCHAR((sector % 75) & 0xFF);
		Second = UCHAR(((sector / 75) % 60) & 0xFF);
		Minute = UCHAR((sector / (75 * 60)) & 0xFF);
		return *this;
	}
};

struct BigEndWord
{
	UCHAR num[2];

	BigEndWord & operator =(USHORT src)
	{
		num[0] = UCHAR((src >> 8) & 0xFF);
		num[1] = UCHAR(src & 0xFF);
		return * this;
	}
	operator USHORT() { return num[1] | (num[0] << 8); }
};

struct BigEndTriple
{
	UCHAR num[3];
	BigEndTriple & operator =(ULONG src)
	{
		num[0] = UCHAR((src >> 16) & 0xFF);
		num[1] = UCHAR((src >> 8) & 0xFF);
		num[2] = UCHAR(src & 0xFF);
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
		num[0] = UCHAR((src >> 24) & 0xFF);
		num[1] = UCHAR((src >> 16) & 0xFF);
		num[2] = UCHAR((src >> 8) & 0xFF);
		num[3] = UCHAR(src & 0xFF);
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

enum { CDDASectorSize = 2352} ;

struct RiffCddaFmt
{
	DWORD Riff; //"RIFF"
	DWORD Fmt; // "fmt "
	DWORD Size; // 0x18
	USHORT Session; // 1 ??
	USHORT Track;   //
	USHORT VolumeSerialNumber;
	ULONG FirstSector;
	ULONG LengthSectors;
	CdAddressMSF FirstSectorMsf;
	CdAddressMSF LengthMsf;
};

struct CdTrackInfo
{
	CString Artist;
	CString Album;
	CString Track;
	CString TrackFileName;
	bool Checked;
	bool IsAudio;
	CdAddressMSF TrackBegin;
	LONG NumSectors;
	CdTrackInfo()
	{
		Checked = FALSE;
		NumSectors = 0;
		TrackBegin.reserved = 0;
		TrackBegin.Minute = 0;
		TrackBegin.Second = 0;
		TrackBegin.Frame = 0;
	}
};

typedef struct _CDROM_TOC CDROM_TOC;
struct SCSI_SenseInfo;
struct SRB_HAInquiry;
struct SRB;


class ICdDrive
{
public:
	virtual ~ICdDrive() {}

	virtual int FindCdDrives(TCHAR Drives['Z' - 'A' + 1]) = 0;
	virtual BOOL Open(TCHAR letter) = 0;
	virtual void Close() = 0;
	virtual DWORD GetDiskID() = 0;

	virtual BOOL GetMaxReadSpeed(int * pMaxSpeed, int * pCurrentSpeed) = 0; // bytes/s

	virtual BOOL SetReadSpeed(ULONG BytesPerSec, ULONG BeginLba = 0, ULONG NumSectors = 0) = 0;
	virtual BOOL ReadCdData(void * pBuf, long Address, int nSectors) = 0;
	virtual BOOL ReadCdData(void * pBuf, CdAddressMSF Address, int nSectors) = 0;
	//virtual BOOL SetStreaming(long BytesPerSecond) = 0;

	//virtual CString GetLastScsiErrorText() = 0;
	//virtual BOOL GetMediaChanged() = 0; // TRUE if disk was changed after previous call
	virtual BOOL EnableMediaChangeDetection(bool Enable = true) = 0;
	virtual BOOL DisableMediaChangeDetection() = 0;
	virtual BOOL LockDoor(bool Lock = true) = 0;
	virtual BOOL UnlockDoor() = 0;
	virtual BOOL ReadToc() = 0;

	virtual BOOL IsTrackCDAudio(unsigned track) = 0;
	virtual UCHAR GetNumberOfTracks() = 0;
	virtual UCHAR GetFirstTrack() = 0;
	virtual UCHAR GetLastTrack() = 0;
	virtual CdAddressMSF GetTrackBegin(unsigned track) = 0;
	virtual LONG GetNumSectors(unsigned track) = 0;
	virtual LONG GetTrackNumber(unsigned track) = 0;

	//virtual BOOL ReadSessions() = 0;
	virtual void StopAudioPlay() = 0;

	virtual bool CanEjectMedia() = 0;
	virtual bool CanLoadMedia() = 0;
	virtual void EjectMedia() = 0;
	virtual void LoadMedia() = 0;
	virtual BOOL IsTrayOpen() = 0;
	virtual BOOL EjectSupported() const = 0;

	virtual BOOL IsSlotType() const = 0;
	virtual BOOL IsTrayType() const = 0;

	virtual void SetTrayOut(bool Out) = 0;

	virtual CdMediaChangeState CheckForMediaChange() = 0;
	virtual void ForceMountCD() = 0;

	virtual BOOL ScsiInquiry(struct SRB_HAInquiry * pInq) = 0;
	virtual BOOL QueryVendor(CString & Vendor) = 0;
	virtual void StopDrive() = 0;

	virtual void SetDriveBusy(bool Busy = true) = 0;
	virtual bool IsDriveBusy(TCHAR letter)const  = 0;
	virtual bool IsDriveBusy() const = 0;

	//virtual BOOL GetEcMode(BOOL * C2ErrorPointersSupported) = 0;

	//virtual BOOL StartReading(int speed) = 0;

	virtual ICdDrive * Clone() const = 0;

};

ICdDrive * CreateCdDrive(BOOL UseAspi = TRUE);

#pragma pack(pop)
#endif // !defined(AFX_CDDRIVE_H__444EC2DA_90D7_4205_BD0C_E0A478C802CD__INCLUDED_)
