Known problems and tasks:

use CDRALW2K.sys service before trying ASPI
Add CD grabbing
Open CDA files
Load sound from AVI
Add options dialog
Save WAVEFORMAT of an open WMA/MP3 file
Enter WMA file attributes (title, author, etc)
Enter MP3 file attributes
In outline view, change mouse cursor over caret and view and selection boundaries

Add noise reduction estimation in spectrum section view
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
Add "phase corrected" stereo to mono conversion
Add VU meter for playback
Add volume control
Add Application Close Pending flag
Make tooltips in wave view and other important views
Show File Properties
Make help file
Add "Favorite formats" combobox to Save dialog
When Open/Save dialog is resized, resize/move the controls
Verify that FileSave can be canceled
Reconsider Undo All Changes functionality and Redo All Changes
??Delete permanent undo: non-permanent file may become permanent after save, move call after save
Pass saved Raw file parameters back for reopen
Use ReplaceFile for renaming the file
Add DELETE premission when creating temp file
Use GetFileAttributes rather than FindFirst to check whether it's file or directory
Add Export Settings, Import Settings to Options dialog
Add splash screen
"Save selection as" in selection dialog
"Save As" in most process dialogs
Make recording from Internet stream

Problems:

Multisession disk shows only begin of tracks. Read the whole structure.

Expression evaluation selection longer than file length doesn't update file length
After file length increased to 1 sample from 0, scroll bar set to wrond scale
Multiline edit box in child dialog eats Esc and Enter (DLGC_WANTALLCHARS)
If there is not enough space on NTFS volume, it will be seen only during flush
Windows2000 is trying to zero the allocated file
samples with 32767, -32768 are not visible

Log Off query doesn't close the active dialog. Recursion is possible. Make sure to check after Cancel

Fixed:
No Disk In Drive has a checkmark
"No Disk in drive" is updated all the time
CD list combo height too low
Click on outline view moves the wave view a bit after the button released
When FFT array is reallocated, it is invalidated (was disabled in the code)
Paste operation doesn't check if sampling rate is the same. Add dialog to resample
Notch filter initial setting is weird
Originally selected filter point may be hidden because the filter is disabled
Too many digits in filter/equalizer dialogs number boxes
FFT zoom out doesn't put the range to the proper limits
After resample, frequency ruler not updated
Save copy reopen file dialog has "Close" instead of "Cancel"
OpenWmaFile crashes if the temporary file couldn't be created
File dialog didn't open in Win98
Equalizer and filter views invalidated wrong area (CXDRAG instead of CXSIZEFRAME)
Tooltips could be shown only after TTN_GETTEXT failed once, otherwise the tips was inactive
in FFT view, sound modification doesn't invalidate enough FFT data
in 1:1 scale, need to invalidate 1 more column
CEqualizerContext didn't set Dirty flag in the buffers
Min/Max sample position not divided by the sample size
Save, Undo, Modify - causes to mark the file as unchanged
Right-only volume change skipped some samples
Use document title rather than filename in dialogs
Drag distance too little in WinME (use resize border size)
No non-client painting under WinME
ATI card can't draw small circles (use memory bitmap)
No status bar in WinME
FFT doesn't invaludate some areas

Deferred:
16 and 20 kbit/s WMA save is incomplete (not reproduced)
During playback, scrolled remnants of playback cursor seen (video driver?)
Put buttons for the view commands to the wave window status bar
CWmaNotInstalledDlg doesn't save "Don't show" flag
Save As dialog is not centered first time (comdlg problem?)
??? When time/seconds format is set for status bar, MM:SS is actually shown

Done:
Draw gray background outside file area on FFT
Add overflow dialog on resample	and other waveproc
Test "reload compressed file" dialogs
Add WMA save
Fill WMA formats combo
Add RAW format save
When saving a copy, change the dialog title
Pass file type flags for MP3 file reopen
Add MP3 save
Save Copy As asks for file reopen
File/New and File/Save items moved to submenus
Put zoom buttons instead of the static fields in the view:
  On top static: horizontal zoom,
  On Bottom static: vertical zoom
Save peak info for compressed files, too, but recalculate it when loading
Add low/high frequency and notch filters functions
Add "Swap Channels" function
Make "CRC" field for the statistics
Add Checksum field for the statistics
F12 - shortcut for Save As, Shift+F12 - shortcut for Save Copy
Add "Zero phase" option to the equalizer
Make shortcuts:
    show waveform: Alt+1
    Show FFT: Alt+2
    Toggle Spectrum section: Alt+3
    Toggle outline view:Alt+4
    Toggle horizontal ruler: Alt+5
    Toggle vertical ruler: Alt+6
    Increase number of FFT bands: Ctrl+Gray+
    Decrease number of FFT bands: Ctrl+Gray-
Add RAW format open
Include MP3 and WMA filters even if there is no WMP, but show warning, if the filter is selected
Use circular array for FFT data (for performance reason).
Add "Simple" equalizer. Multiband equalizer done.
Support f1, f2, f3 in Evaluate expr
Put line breaks to the replace expression message
Add "Export", "Import" to Evaluate Expression dialog
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
