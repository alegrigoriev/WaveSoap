#pragma once

enum
{
	SampleToString_Sample = 0,
	SampleToString_HhMmSs = 1,
	SampleToString_Seconds = 2,
	SampleToString_HhMmSsFf = 3,    // seconds and CD frames (75 fps)
	SampleToString_Mask = 0xF,
	TimeToHhMmSs_NeedsMs = 0x100,
	TimeToHhMmSs_NeedsHhMm = 0x200,
	TimeToHhMmSs_Frames75 = 0x400,  // time % 75 is a frame count, not miliseconds
};
CString TimeToHhMmSs(unsigned TimeMs, int Flags = TimeToHhMmSs_NeedsMs);
CString SampleToString(SAMPLE_INDEX Sample, int nSamplesPerSec,
						int Flags = SampleToString_HhMmSs
									| TimeToHhMmSs_NeedsHhMm
									| TimeToHhMmSs_NeedsMs);
