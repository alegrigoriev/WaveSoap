Known problems and tasks:

Add "Export", "Import" to Evaluate Expression dialog
Add "Simple" equalizer
Add equalizer and low/high frequency filters functions
Use list instead of array for FFT data (for performance reason).
Include MP3 and WMA filters even if there is no WMP, but show warning, if the filter is selected
Test "reload compressed file" dialogs
Add options dialog
Try to load sound from AVI with WM functions
Put buttons for the view commands to the wave window status bar

Add CD grabbing
Add noise reduction estimation in spectrum section view
Add MP3 save
Add ASF (WMA) save
Add RAW format open
Add RAW format save
Add sound recording
Support "Play" in selection dialog
Make Paste Special command (with Fade In/Fade Out etc)
Make Undo/redo save the selection and regions
Add support for markers and regions: save on copy and with undo, move and delete on Cut,
	move on Paste
Double click selects between two markers
Delete/Insert operations can auto add markers and regions
If delete (shrink) is done with one channel of two, fill the rest with zeros
If displaying data without peak info, call RescanPeaks for this range.
Support CFSTR_FILECONTENTS clipboard format
Save current workspace
Use secondary stream to keep peak info
Support filenames with stream extension
Add VU meter for playback
Add Application Close Pending flag
Make tooltips in wave view and other important views
Show File Properties
Make help file
Add "Favorite formats" combobox to Save dialog
"Save selection as" in selection dialog
"Save As" in most process dialogs
When Open/Save dialog is resized, resize/move the controls
Verify that FileSave can be canceled
Reconsider Undo All Changes functionality and Redo All Changes
??Delete permanent undo: non-permanent file may become permanent after save, move call after save
Use ReplaceFile for renaming the file
Add DELETE premission when creating temp file
Use GetFileAttributes rather than FindFirst to check whether it's file or directory
Add Export Settings, Import Settings to Options dialog
Add splash screen

Problems:
After file length increased to 1 sample from 0, scroll bar set to wrond scale
Expression evaluation selection longer than file length doesn't update file length
Multiline edit box in child dialog eats Esc and Enter (DLGC_WANTALLCHARS)
If there is not enough space on NTFS volume, it will be seen only during flush
Windows2000 is trying to zero the allocated file
samples with 32767, -32768 are not visible

LOg Off query doesn't close the active dialog. Recursion is possible. Make sure to check after Cancel

Deferred:
CWmaNotInstalledDlg doesn't save "Don't show" flag
Save As dialog is not centered first time (comdlg problem?)
??? When time/seconds format is set for status bar, MM:SS is actually shown

Fixed:
Use document title rather than filename in dialogs
Drag distance too little in WinME (use resize border size)
No non-client painting under WinME
ATI card can't draw small circles (use memory bitmap)
No status bar in WinME
FFT doesn't invaludate some areas
Insert silence dialog showing channels for MONO sound
When switched to/from FFT view, scrollbar is set to wrong range (Suite1.wav)
Static in the child frame was incorrectly repositioned
SpectrumSection ruler sometimes doesn't draw smaller ticks
Spectrum section resize uses wrong position
Equalizer window could be drawn with non-client backgrould in the client area
Selection dialog shows channels for MONO sound
Expression evaluation didn't show status string
Backward file prefetch from position 0 would cause memory overallocation (500 MB) and hang
Expression dialog: OK not disabled on the beginning, if expression is empty
If there is no selection, Selection button dialog doesn't give option From Cursor, To Cursor
"Interpolate" command not disabled properly
Last 64 KB block is not read from the master file (need to round read length to sector size
Channels swapped in Spectrum Section view
FFT view is ahead of the data??? 
SetSelection now moves only active child frame views.
Ctrl-End, Ctrl-Home loses synchronization between FFT and wave
When selecting to the begin of file, FFT is corrupted
When Save As from LLADPCM to PCM, suggests 8 bit (now chooises max number of bits)
Stop button doesn't work for saving the compressed file
Reset "Playing" status message after it stopped
In Statistics dialog: replace "Minimum sample", Maximum sample" 
     with "peak positive value, peak negative value"
Change channels to copy from: no ':'
Resample: Change tempo and pitch clipped
Normalize: "Equally adjust" clipped
Adjust DC: Automatically detect and remove" clipped, Compute DC offset clipped
Click removal: More settings button too small, "Log all clicks to a file" clipped
After Save As, peak info is saved with wrong timestamp for the new PCM file.
Wrong minimum/maximum valies shown for a zero length file in Statistics (command disabled for such file)
???? When a file is opened in non-direct mode, peak info is saved with wrong time stamp

Done:
Add Save/Load to equalizer
Remember Equalizer dialog size
Process Enter on EDIT_GAIN edit box
EQ window: draw dot-caret if no focus, draw without blink, when dragging
In CSelectionDialog: replace combobox strings with LoadString
Show file names in PostRetire dialogs, because they can be shown for background DOC.
Show file name in Statistics Dialog
Add decibel view to CAmplitudeRuler
Result of FFT normalized to maximum for max square wave, fro FFT view and spectrum section
FFT ruler menu command changed to frequency-related
Draw decibels in Spectrum Section view
If Wave view and spectrum section are shown, show both amplitude ruler (along wave view) and frequency ruler
Draw crosshair in Spectrum Section view
Make sliders working in Resample Dialog
Max WAV file size: [x] 2GB [ ] 4 GB
Selection dialog shows more options in combobox and doesn't show non-applicable options
Add "Close File" option to other "reload after save" dialogs
Place all CApplicationProfile members to the end of class, or add RemoveAll or UnloadAll() call to the destructors
Make default for "Don't show MP3/WMA warning" FALSE
Add toolbar button for channels lock
keep cursor in 5% from the view boundary.
Add FFT windowing choice
Process WM_SETTINGSCHANGE if settings or metrics changed (do RecalcLayout)
Show FFT selection with thick XOR rectangle
Remember Spectrum Section settings (FFT order) and size
Appropriate commands are disabled when the file has zero length
calculate max file length in seconds, given sampling rate and number of channels
Change default lower frequency for noise suppression to 1000
in ULF suppression: replace 'differential' with "vertical wobble" (in double quotes)
"Zoom In vertical" tooltip: change to "Zoom in vertical"
Move 'f' button a bit down in Operands tab


For Version 2:
Make middle button scroll (drop anchor window)
Add Undo/Redo drop list
Make a few source samples available in expression and a few output samples too
    use wave[-63...63]
    wave(1)
    wave_ch[1, 1...63]
Support WAVEFORMATEXTENSIBLE.
Make multichannel editing
allow 24/32 bit data
Allow "Minimized" channels
Use UNICODE

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


ScaledScroolView:

SlaveViews and master views

Operations:

Set Max boundaries:
Forwarded to master view SetMaxExtentsMaster(). Then master notifies the slave views.

Set view boundaries:
Set view origin and extents:
Call master view OnChangeOrgExtMaster. It will then notify slave views

Scroll by world units:
Recalculated to pixel units. Called master view MasterScrollBy

Scroll by pixels:
Call master view MasterScrollBy


When origin is set, it is possible to round it to whole pixels. It is also done when zoom is done
