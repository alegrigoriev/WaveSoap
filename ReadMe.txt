Known problems and tasks:

Make Paste Special command
When a file is opened in non-direct mode, peak info is saved with wrong time stamp
support copy from a file (FileName) in DoCopy
Add support for markers and regions: save on copy and with undo, move and delete on Cut,
	move on Paste
Double click selects between two markers
If delete (shrink) is done with one channel of two, fill the rest with zeros
Prb: After pause cursor is not in the view center
prb: font may not fit in the time ruler. Change size calculation
Prb: GetBufferWriteOnly can conflict with SourceFile and WrittenMask
Prb: After file expansion, the new area may not contain zeros, 
	if the file has a SourceFile (not a problem, actually)
Add Selection... command and dialog
prb: time ruler is blinking when scrolled
Add context menu to the status bar
Make zoom to selection command
Add GoTo dialog and command
Add support for 256 color display (palette)
If displaying data without peak info, call RescanPeaks for this range.
When wave background is scrolled, it gets lines.
Show Outline view
Show current scale in the "static" child control.
(almost done) Add Drag and Drop support
Add options dialog
Save current workspace
Add decibel view to CAmplitudeRuler
Add VU meter for playback
Make all correct error messages and message boxes
Allow "Minimized" channels

Add mouse wheel support
Add autoscroll during selection

Done:
Make Paste with selection
Make paste with channel selection
Make Undo consistent with changes, if the operation was interrupted
Update max extents to reserve 100 pixels after end of file
Add Zoom buttons to the toolbar
Add undo/redo operation contexts
Add context menu to FFT view, switches number of analyse bands
Make context menu for wave views
Add undo/redo operation contexts for replace operations


