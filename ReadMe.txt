Known problems and tasks:

Make "New File parameters" dialog
Support "Play" in selection dialog
Open files specified in the command line
Add CD grabbing
Add noise reduction estimation in spectrum section view
Add MP3 save
Add ASF save
Add sound recording
If read-only file is saved, name should be different
keep cursor in 10% from the view boundary.
Make Paste Special command
???? When a file is opened in non-direct mode, peak info is saved with wrong time stamp
Before Save, delete Undo and REdo which keeps reference to the target file
    (it can be file replacement Undo/Redo)
After Save As, peak info is not saved for the new PCM file.
???? File created from clipboard and saved can't be reopened in direct mode.
Undo/redo saves the selection
Add support for markers and regions: save on copy and with undo, move and delete on Cut,
	move on Paste
Double click selects between two markers
Delete/Insert operations can auto add markers and regions
If delete (shrink) is done with one channel of two, fill the rest with zeros
If displaying data without peak info, call RescanPeaks for this range.
Check if the file size will exceed 2 GB
Add options dialog
Save current workspace
Add decibel view to CAmplitudeRuler
Add VU meter for playback
Make tooltips
Make help file

Problems:
Expression evaluation selection longer than file length doesn't update file length
LOg Off query doesn't close the active dialog. Recursion is possible. Make sure to check after Cancel
Ctrl-End, Ctrl-Home loses synchronization between FFT and wave
When selecting to the begin of file, FFT is corrupted
MP3 open, WAV save as - can't reopen in direct until 2 seconds passed.
If there is not enough space on NTFS volume, it will be seen only during flush
Windows2000 is trying to zero the allocated file

Fixed:
If number of channels changed, amplitude ruler doesn't update extents
Rejected: Statistics: zero crossings shows double the frequency (as supposed??)
Doesn't detect new active document, when previous closed.
Doesn't see the document as active
when "sincos" entered, complains "Right parentheses expected" rather than "syntax"
Spin buttons didn't work in selection dialog
Noise function had only 15 bit resolution
Make save presets for Expression Evaluation command
Insert Silence of zero length still tries to modify the file
Multiple MP3 files open: all but one get stuck

Done:
Move most persistent dialog parameters to dialogs, to update registry dynamically
Notify when there is overflow during expression evaluation
Make group "All Expressions" in expression evaluation dialog
Make "Custom samplerate" dialog
Add Resample... command to sample rate context menu
Add Channels... command to menu
Make Combobox in InsertSilence dialog
Make "Begin" and "End" comboboxes in selection dialog
Make combobox in goto dialog
Combobox in selection dialog for time and selections
Frequency argument added to expression evaluation
Keep selected tab number in ExprEvaluation dialog
Show expression descriptions
Remember toolbar/status bar toggle
Remember last frame and view maximized state
Protect against exceptions during expression exaluation
Make more integer operations for Expression Evaluation (bit ops, modulo)
Change file extension in Save As dialog, if the type changed
Add "Cancel" (don't reopen, but close) to Reopen after save dialog
Add Hide in spectrum section context menu
Make more convesient Ctrl+Tab
add CDirectFile::operator= (WMA open crash)
Mouse selection for outline view and snap to maximum.
Mouse selection snaps to the max amplitude
Make all correct error messages and message boxes


For Version 2:
Make a few source samples available in expression and a few output samples too
    use wave(1...63) and wave(-1...-63)
    wave_ch(1, 1...63)
Support WAVEFORMATEXTENSIBLE.
Make multichannel editing
allow 24/32 bit data
Allow "Minimized" channels

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