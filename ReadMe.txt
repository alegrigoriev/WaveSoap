Known problems and tasks:

Add undo/redo operation contexts
Prb: After pause cursor is not in the view center
Prb: GetBufferWriteOnly can conflict with SourceFile and WrittenMask
Prb: After file expansion, the new area may not contain zeros, 
	if the file has a SourceFile (not a problem, actually)
Add selection display to FFT view.
Support Cut, Delete commands
Add Selection... command and dialog
Add GoTo dialog and command
Show Outline view
Show current scale in the "static" child control.
Add Cut and Delete commands
(almost done) Add Drag and Drop support
Add options dialog
Save current workspace
Add decibel view to CAmplitudeRuler
Add VU meter for playback
Make all correct error messages and message boxes

Add mouse wheel support
Add autoscroll during selection

Done:
Support single channel playback
Add status bar to the child view
Empty the status bar panes
make Current Position indicator rather than separate caret and playback indicators
Make tooltips in the status bar
Add indicators in the status bar (position, selection)
Add playback Stop/Pause commands to Doc
Add playback position cursor to the view
Add Retire() method to COperationContext.
Fixed: Draw when loading compressed file, after the file was expanded.
Fixed: Scan for peaks was broken
Fixed: memory leak in CWaveDevice
Fixed: negative samples close to 0 are drawn on -1 position.
