Known problems and tasks:

Prb: GetBufferWriteOnly can conflict with SourceFile and WritttenMask
prb: Draw when loading compressed file, after the file was expanded.
Add playback Stop/Pause commands to Doc
Add indicators in the status bar (position, selection)
Support Cut, Delete commands
Add undo/redo operation contexts
Add Selection... command and dialog
Add GoTo dialog and command
Show Outline view
Show current scale in the "static" child control.
Add Cut and Delete commands
Add Drag and Drop support
Add options dialog
Save current workspace
Add decibel view to CAmplitudeRuler
Add VU meter for playback

Add mouse wheel support
Add autoscroll during selection

Done: load 8-bit and compressed files
Support file opened for editing	(Source file in class File)
Done: DetachSourceFile()
Done: Do not reset prefetch pointer, if such a prefetch is already in progress
Done: customized File Open dialog, to allow different modes and multiple files
Done: Do not move buffer to the end of MRU, if 0 != ((pos + len) & 0xFFFF)
Done: Play command