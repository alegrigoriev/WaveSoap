Known problems and tasks:

prb: Zoom In Full command moves to wrong position
Add spin support for selection dialog.
Add GoTo dialog and command
Rintime error during Undo on some systems
Make Paste Special command
When a file is opened in non-direct mode, peak info is saved with wrong time stamp
support copy from a file (FileName) in DoCopy
Add support for markers and regions: save on copy and with undo, move and delete on Cut,
	move on Paste
Double click selects between two markers
If delete (shrink) is done with one channel of two, fill the rest with zeros
Prb: After pause cursor is not in the view center
Prb: GetBufferWriteOnly can conflict with SourceFile and WrittenMask
Prb: After file expansion, the new area may not contain zeros, 
	if the file has a SourceFile (not a problem, actually)
prb: time ruler is blinking when scrolled
Make zoom to selection command
Add support for 256 color display (palette)
If displaying data without peak info, call RescanPeaks for this range.
When wave background is scrolled, it gets lines.
Show Outline view
Show current scale in the "static" child control.
Add options dialog
Save current workspace
Add decibel view to CAmplitudeRuler
Add VU meter for playback
Make all correct error messages and message boxes
Allow "Minimized" channels

Add mouse wheel support
Add autoscroll during selection

Done:
Make check for clipping in VolumeChange command
prb: font may not fit in the time ruler. Change size calculation
Add Selection... command and dialog
Add Drag and Drop support
Add context menu to the status bar
Make Volume Change dialog
Horizontal scale is automatically changed to stretch the sound to the view
Fixed vertical scroll problem with FFT view
Ruler background made consistent with menu color.
Time ruler height calculated from the font height