Known problems and tasks:

If read-only file is saved, name should be different

Add MP3 save
Add ASF reading
Add ASF save
Use waveformat instead of template, for FileNew
Make "Expression Evaluation" command
Add spectrum section view
Sound view jerks during playback of the last screen.
Add spin support for selection dialog.
Rintime error during Undo on some systems
Make Paste Special command
When a file is opened in non-direct mode, peak info is saved with wrong time stamp
support copy from a file (FileName) in DoCopy
Add support for markers and regions: save on copy and with undo, move and delete on Cut,
	move on Paste
Double click selects between two markers
If delete (shrink) is done with one channel of two, fill the rest with zeros
Prb: GetBufferWriteOnly can conflict with SourceFile and WrittenMask
Prb: After file expansion, the new area may not contain zeros, 
	if the file has a SourceFile (not a problem, actually)
prb: time ruler is blinking when scrolled
Add support for 256 color display (palette)
If displaying data without peak info, call RescanPeaks for this range.
Show current scale in the "static" child control.
Add options dialog
Save current workspace
Add decibel view to CAmplitudeRuler
Add VU meter for playback
Make all correct error messages and message boxes
Allow "Minimized" channels
Add sound recording
Add mouse wheel support
Add autoscroll during selection

Done:
Add MP3 reading
prb: playback stops during FFT draw (could be caused by DirectFile list bug)
If the file has non-standard sample rate, in Save dialog create two PCM formats with this sample rates,
  along with format description
After saving compressed file, update file length and 'fact' chunk
Make Compressed file save
Make File Save function
Prb: When wave background is scrolled, it gets lines.
Fixed wrong m_pReadMask initialization
Make mono version of statistics dialog template
Process fact chunk for the compressed file (fact length in samples).
Prb: After pause cursor is not in the view center
Make caret centered after goto commands
Make faster registry access
Make Undo for Resampling and for sample rate change
Add visible area to OutlineView
Erase playback cursor after stop
Make Resampling.
Problem with wave view scroll - garbage left.
Problem with Wave Update.
Make zoom to selection command
Draw OutlineView from the real data if the file is too short
Add playback cursor to OutlineView
prb: Zoom In Full command moves to wrong position

File Save/Save As checklist:

Read-only file (or opened read-only) can only be saved as with different name.
Direct mode:
If file name and format is the same, all changes (including format and length)
should be committed, peak info saved after file commit, 
selection stays the same, undo/redo info kept.

If file name is not the same, commit all changes, save a copy (may be with conversion),
file stays open, undo/redo info kept.

If format is different, but file name is the same, convert the file, delete original,
rename saved to the original name and reopen it in direct or non-direct mode, depending on
the dialog box. Selection stays the same (adjusted for sample rate change). 
Peak info isn't saved and will be rescanned when the file is reopened.

Non-direct mode:

If saving a copy, create a new file, do all conversion, save all file info (RIFF structure),
source file kept open.

If not saving a copy, always reopen the source file, ask about direct mode if possible.
If saved with conversion, but with the same sampling rate and number of channels,
ask about reloading the file.

If save operation is aborted, delete the temporary file, keep the original files the same.

If the file cannot be renamed from the temporary name to the target, ask about new name.

If saving a file under the same name, copy security information from the old file to 
the new file.

If using the temporary file as 