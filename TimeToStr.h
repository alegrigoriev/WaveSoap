#pragma once

enum
{
	SampleToString_Sample = 0,
	SampleToString_HhMmSs = 1,
	SampleToString_Seconds = 2,
	SampleToString_Mask = 0xF,
	TimeToHhMmSs_NeedsMs = 0x100,
	TimeToHhMmSs_NeedsHhMm = 0x200,
};
CString TimeToHhMmSs(unsigned TimeMs, int Flags = TimeToHhMmSs_NeedsMs);
CString SampleToString(SAMPLE_INDEX Sample, int nSamplesPerSec,
						int Flags = SampleToString_HhMmSs
									| TimeToHhMmSs_NeedsHhMm
									| TimeToHhMmSs_NeedsMs);
