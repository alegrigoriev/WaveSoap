Known problems and tasks:

Reset "Playing" status message after it stopped
When Open/Save dialog is resized, resize/move the controls
Verify that FileSave can be canceled
Reconsider Undo All Changes functionality and Redo All Changes
??Delete permanent undo: non-permanent file may become permanent after save, move call after save
V2: Add Undo/Redo drop list
Remember Spectrum Section settings and size
Add FFT windowing choice
Make middle button scroll (drop anchor window)

Add Application Close Pending flag
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
Add toolbar button for channels lock
Show File Properties
Make sliders working in Resample Dialog
Save current workspace
Use secondary stream to keep peak info
Support filenames with stream extension
Add decibel view to CAmplitudeRuler
Draw decibels and crosshair in Spectrum Section view
Add VU meter for playback
Make tooltips in wave view and other important views
Make help file
Add "Favorite formats" combobox to Save dialog

Problems:
Stop button doesn't work for saving the compressed file
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
After Save As, peak info is saved with wrong timestamp for the new PCM file.
Wrong minimum/maximum valies shown for a zero length file in Statistics (command disabled for such file)
???? When a file is opened in non-direct mode, peak info is saved with wrong time stamp

Done:
Appropriate commands are disabled when the file has zero length
calculate max file length in seconds, given sampling rate and number of channels
Change default lower frequency for noise suppression to 1000
in ULF suppression: replace 'differential' with "vertical wobble" (in double quotes)
"Zoom In vertical" tooltip: change to "Zoom in vertical"
Move 'f' button a bit down in Operands tab


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