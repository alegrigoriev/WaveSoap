Known problems and tasks:

When Open/Save dialog is resized, resize/move the controls
Verify that FileSave can be canceled

Broadcast UpdateAllViews if settings or metrics changed
Add CD grabbing
Add noise reduction estimation in spectrum section view
Add MP3 save
Add ASF save
Add sound recording
Add splash screen
Support "Play" in selection dialog
keep cursor in 10% from the view boundary.
Make Paste Special command (with Fade In/Fade Out etc)
Make Undo/redo save the selection and regions
Add support for markers and regions: save on copy and with undo, move and delete on Cut,
	move on Paste
Double click selects between two markers
Delete/Insert operations can auto add markers and regions
If delete (shrink) is done with one channel of two, fill the rest with zeros
If displaying data without peak info, call RescanPeaks for this range.
Support CFSTR_FILECONTENTS clipboard format
Add options dialog
Show File Properties
Save current workspace
Use secondary stream to keep peak info
Add decibel view to CAmplitudeRuler
Add VU meter for playback
Make tooltips
Make help file

Problems:
After Save As, peak info is saved with wrong timestamp for the new PCM file.
???? When a file is opened in non-direct mode, peak info is saved with wrong time stamp
When Save As from LLADPCM to PCM, suggests 8 bit
 
Multiline edit box in child dialog eats Esc and Enter (DLGC_WANTALLCHARS)
If there is not enough space on NTFS volume, it will be seen only during flush
Windows2000 is trying to zero the allocated file

Ctrl-End, Ctrl-Home loses synchronization between FFT and wave
When selecting to the begin of file, FFT is corrupted
Expression evaluation selection longer than file length doesn't update file length
LOg Off query doesn't close the active dialog. Recursion is possible. Make sure to check after Cancel

Deferred:

Save As dialog is not centered first time (comdlg problem?)
??? When time/seconds format is set for status bar, MM:SS is actually shown

Fixed:
If the ACM decoder is not available, the file still can be opened, but is empty
File New dialog: wrong validation message
Expression evaluation still crashes in the release version (Wrong OnKickIdle prototype)
Even though the length is too much, the file is created with non-zero length
For the new file, peak info is not zeroed
Set focus to file length in File New dialog, move it to the top
Noise reduction: "Fft order" label short, "continuous tone detection" clipped, 
    "Transient area" clipped, 
    More Settings button small and inactive, Maximum noise clipped,
ULF reduction: checkbox labels are a bit short
LP noise reduction button clipped on the lower edge
For new file, "Copy Of" name is suggested
File/New with length didn't preallocate peak info
WS_EX_CONTROLPARENT must not be set for child dialogs!
File opened for read-only can only be reopened for read only too
Read only file cannot change length (File::SetFileLength disabled)
MP3 open, WAV save as - can't reopen in direct until 2 seconds passed.
Name in reopen dialog is incorrect
CTimeEdit increments by 0.1 s even if miliseconds not shown
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
Check if the file size will exceed 2 GB 
    (paste, insert silence, resample, save As with resample,
    change number of channels (also when Save), create new file)
Make SaveAll
Remember Open and Save As folders
Keep Save File name when switching folders on Recent Folders
Add Recent Folders to FileSave dialog
Show current folder name in "Recent"
If read-only file is saved, name should be different
If saving copy, assign different file name in the Save dialog
Make Percent default setting for amplitude ruler
Ctrl+Shift+N is also accelerator for File New
Before Save, delete Undo and REdo which keeps reference to the target file
    (it can be file replacement Undo/Redo) or rename the file
    When the file is about to be saved,
    scan all undo/redo for the document and delete items which refer to non-temporary file
If a file is already open, activate its view
Open files specified in the command line
Init file length for new file dialog
Remember new file length in seconds
Make "New File parameters" dialog
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
    use wave[-63...63]
    wave(1)
    wave_ch[1, 1...63]
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