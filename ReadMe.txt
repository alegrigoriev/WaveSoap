Known problems and tasks:

Make save presets for Expression Evaluation command
Add noise reduction estimation in spectrum section view
Add MP3 save
Add ASF save
Add sound recording
Add CD grabbing
If read-only file is saved, name should be different
Statistics: zero crossings shows double the frequency (as supposed??)
Make Paste Special command
???? When a file is opened in non-direct mode, peak info is saved with wrong time stamp
Before Save, delete Undo and REdo which keeps reference to the target file
    (it can be file replacement Undo/Redo)
After Save As, peak info is not saved for the new PCM file.
???? File created from clipboard and saved can't be reopened in direct mode.
Add support for markers and regions: save on copy and with undo, move and delete on Cut,
	move on Paste
Double click selects between two markers
Delete/Insert operations can auto add markers and regions
If delete (shrink) is done with one channel of two, fill the rest with zeros
If displaying data without peak info, call RescanPeaks for this range.
Add options dialog
Save current workspace
Add decibel view to CAmplitudeRuler
Add VU meter for playback
Make all correct error messages and message boxes
Make tooltips
Make help file

For Version 2:
Make multichannel editing
allow 24/32 bit data
Allow "Minimized" channels

Done:
Add support for 256 color display (palette)
Add mouse wheel support
Add autoscroll during selection
Spectrum section view: drawing on the edges.
Add spin support for selection, insert silence and GOTO dialog.
Channel conversion
Prb: GetBufferWriteOnly can conflict with SourceFile and WrittenMask
Wrong caret size after window resize during playback
Prb: After file expansion, the new area may not contain zeros, 
	if the file has a SourceFile and the operation was interrupted 
Draw zero line in outline view for empty wave file
Use waveformat instead of template, for FileNew
prb: time ruler is blinking when scrolled
Make delay load for Windows Media DLL, to work on systems without WMP installed
Document closed now after long save operation
Make status string and percent for CCommitFileSaveContext
Make context for background WriteRestOfTheFile


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