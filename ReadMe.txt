Known problems and tasks:

Add saved from CD files to the MRU list
Handle "Compatible/All" formats for MP3, WMA
Raw file: make format tag and save attributes
If CD recording not supported, SET SPEED WriteSpeed set to zero
Restore CD speed to max rather than current!
Set speed doesn't work on Goldstar CDRW	 SetSpeed returned sense 5/24
Pass wave format to CD grabbing dialog
Process Loss Of Streaming error
Make option to ask for file reopen
Handle "Save from CD immediately" option
Read CD text
Open CDA files
Set icons to all resizable dialogs (for XP)
Load sound from AVI
Add options dialog
Enter WMA file attributes (title, author, etc)
Enter MP3 file attributes
In outline view, change mouse cursor over caret and view and selection boundaries
TODO: check 4GB WAV files

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

During playback, outline is invalidated. (??)
WinXP doesn't have CDRAL
Multisession disk shows only begin of tracks. Read the whole structure.
Daylight saving time change invalidates peak info timespamp (FAT only??)
Expression evaluation selection longer than file length doesn't update file length
After file length increased to 1 sample from 0, scroll bar set to wrond scale
Multiline edit box in child dialog eats Esc and Enter (DLGC_WANTALLCHARS) (MFC CDialog::PreTranslateMessage() bug
If there is not enough space on NTFS volume, it will be seen only during flush
Windows2000 is trying to zero the allocated file
samples with 32767, -32768 are not visible

Log Off query doesn't close the active dialog. Recursion is possible. Make sure to check after Cancel

Fixed:
Undo/redo order was wrong
Path change doesn't update OK
checkmark draw with gray background - all black drawn
Wrong calculation of click position in outline view
Unnecessary selection after click in outline view
READ CD SCSI command doesn't work for old NEC drive.
Open WMA file through dialog crashes the app
".wav" added one more time in File Save dialog
IOCTL_CDROM_RAW_READ doesn't work on old Panasonic
File title is not passed to SaveAs
display color depth change from 24 to 16 bit under XP crashes the app
During playback, horizontal ruler is invalidated.
Track names are not updated in the program.
No status string during CD reading?
When opening CD drive, set its media state to undefined
No Disk In Drive has a checkmark
"No Disk in drive" is updated all the time
CD list combo height too low

Deferred:
Add context menu to track list (check/uncheck all/selected)
Add check/uncheck icons to the header
Move WMA error dialog to PostRetire, all initialization to Init()
16 and 20 kbit/s WMA save is incomplete (not reproduced)
During playback, scrolled remnants of playback cursor seen (video driver?)
Put buttons for the view commands to the wave window status bar
CWmaNotInstalledDlg doesn't save "Don't show" flag
Save As dialog is not centered first time (comdlg problem?)
??? When time/seconds format is set for status bar, MM:SS is actually shown

Done:
When showing a dialog for a document, make the doc active, then restore the previous one.
Add file format selection for CD grabbing
When PCM is selected for CD grabbing, show default format
For WMA format: When saving from non-WMA file, remember selected bitrate, restore
For MP3 format: When saving from non-MP3 file, remember selected bitrate, restore
Add CD grabbing
Include PCM format always
If there is no compatible PCM format, add one
Enumerate format tags (filter by tag)
If no format was returned on EnumFormats, try to call SuggestFormat.
Sort formats in datarate order
Add eject CD button, CD buttons with bitmaps
Make a few retries to read TOC after disk change
CD grab: prompt for file replacement
Handle situation when the CD file save directory is not accessible for writing
Add comboboxes to Author, Album, Folder
Save WAVEFORMAT of an open WMA/MP3 file
Process Space key in track list
Get rid of "grab CD to a single file" mode
Call default OnContextMenu!
Created CWaveFormat class
During CD read, create next document during Context::Execute()
Use temporary filename for CD temp files
Add file extension to CD track file name
Set CD reading speed !
use CDRALW2K.sys service before trying ASPI
check file length param on new file creation
Add flush control to CDirectFile
make CDRAL.DLL first choice
Disable label edit, if the track is not audio
Draw gray background outside file area on FFT
Add overflow dialog on resample	and other waveproc
Test "reload compressed file" dialogs


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
----------------------------------------------------------------------------------------------

Enumerating format tags:

Given source file format, check all format tags and see if conversion is possible from the source format to the target format. Source PCM format address is passed to the function.
If there are tags to select, check first if the format tag is among them.
Then enumerate formats to see if there are compatible with source format.
In enum call check if the format is from the requested array, or if no array, that the format tag matches the tag returned for tag enumeration.

If there are tags to exclude, check if the format tag is among them (unless it is WAVE_FORMAT_EXTENSIBLE). During formats enumeration also exclude the WAVE_FORMAT_EXTENSIBLE formats.


Enumerating formats:
