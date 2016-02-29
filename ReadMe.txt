TODO tasks:
waveproc.cpp: refactor CDeclick::ProcessSoundBuffer.
waveproc.cpp: split finding clicks and interpolating them to separate phases
Change "Removing clicks" to "Detecting clicks"
Set icons to all resizable dialogs (for XP)

Show/edit markers as a table
Delete/Insert operations can auto add markers and regions
Unnamed region name is composed from its boundaries. Marker name - from its position
Show the marker position when dragging it.

When saving the selection as, set the file title from the marker at beginning of the selection (unless all the file is selected)

Add prefix to CD-saved files
Add "Single file" option to CD-grab dialog
Make "URL-safe filenames" option in CD grab dialog and file split dialog.

Make Paste Special command (with Fade In/Fade Out etc)
Make Paste From File command
When doing Paste from file, make the file title a region name.

Make color-neutral theme (for color-blind users).
If metadata doesn't come as the very last chunk of the file, copy the original file to 
 a file where it does (for non-compressed file, that is)

Allow dragging NR threshold points
Show FFT of NR result in spectrum view

Make context menu for outline view
In outline view, change mouse cursor over caret and view and selection boundaries

Make "Lock channels" more consistent (disable items in the dialogs, if channels are locked).

Do rewriting from a source file to the work file in background thread.

Move "Import/Export clicks" to "Advanced" dialog
Make Windows/Close All command
Put copyright notices to all files

When starting playing selection, bring the playback cursor into active view
Add "Show" button to the selection dialog
Put "Save As" and "Save Copy As" files to file MRU
Make option to ask for file reopen
Don't ask to replace the file if Save As with the same name
Add "Retry" to error dialog boxes
Enter WMA file attributes (title, author, etc)
Enter MP3 file attributes

Support "arrow up/down" during label editing
If CD recording not supported, SET SPEED WriteSpeed set to zero
Restore CD speed to max rather than current!
Process Loss Of Streaming error
Read CD text
Open CDA files

Make sure file read doesn't lock the critical section, so other requests won't have to wait
For network-based files: use FILE_FLAG_SEQUENTIAL_SCAN option
In SetDeclickData: use a member function. Make a structure for declick parameters.
Remove all CArray use

Load sound from AVI
Add options dialog
check 4GB WAV files

Add sound recording
Support "Play" in selection dialog
If displaying data without peak info, call RescanPeaks for this range.
Support CFSTR_FILECONTENTS clipboard format
Save current workspace
Write operation log on the MDI client background
Add VU meter for playback
Add volume control
Add Application Close Pending flag
Make tooltips in wave view and other important views
Show File Properties
Make help file
Add "Favorite formats" combobox to Save dialog
Verify that FileSave can be canceled
Reconsider Undo All Changes functionality and Redo All Changes
??Delete permanent undo: non-permanent file may become permanent after save, move call after save
Pass saved Raw file parameters back for reopen
Use ReplaceFile for renaming the file
Add DELETE permission when creating temp file
Use GetFileAttributes rather than FindFirst to check whether it's file or directory
Add Export Settings, Import Settings to Options dialog
Add splash screen
"Save selection as" in selection dialog
"Save As" in most process dialogs
Make recording from Internet stream
Find which alignment better for edit box labels: left or right
Add Tools/Synthesis command.
Support multimedia keys

Change Home/End keys semantics, when no selection.

Add help topics for all popup menus.
Support "What is it" help for the window parts
Support popup help for dialogs.
Make help index and contents
Don't show contents bar by default.
Show the help window maximized by height
Instead of "Failed to launch help" show default help page

Add 48000 Hz to the PCM format list (deferred)

Problems:

Progress procent wrong during resample (peak scan interference?) TODO: allow percent per stage
22050->44100 resample takes too long: scanning for peaks gets stuck.
Ctrl+S doesn't always work
When playing, zoom should use the playback point as a center.
WMA format list shows only compatible formats, even when checkbox not checked.
sweep.wav passed through noise reduction gives a ghost reflected off Fsampling/4.
WMA encoded/decoded is delayed by 2048 samples.
SaveAs shows AVI as possible type
Last columns (of the file) in FFT view are not getting erased/drawn properly
Operation status text may get sticky after undo/redo ???
Vertical scroll in the wave view makes marker labels blinking
If there is not enough space to load a compressed file, doesn't show an error
During exit, asks to reopen the file
Save As adds "Copy of" for direct file
Save As fails if the file replaced is read-only
WinXP doesn't have CDRAL
Multisession disk shows only begin of tracks. Read the whole structure.
Multiline edit box in child dialog eats Esc and Enter (DLGC_WANTALLCHARS) (MFC CDialog::PreTranslateMessage() bug
If there is not enough space on NTFS volume, it will be seen only during flush
Windows2000 is trying to zero the allocated file

Log Off query doesn't close the active dialog. Recursion is possible. Make sure to check after Cancel

Fixed:

WMA decorer gives wrong length.
Set custom sample rate dialog: focus not set
Resample dialog doesn't init the slider to the correct position
After resample, prompts to save with 8 bit format
Double click marker edit not disabled in read-only mode.
Save with resample 22050 stereo to 44100 stereo doesn't give correct number of samples.
When a file is connected as source, format chunk address and all RIFF info if not copied over
File length is wrong for 8 bit file.
Peak info saved wrong for 8 bit file
Save as 8 bit doesn't work
"Statistics" dialog shows only file name instead of full path (Sound1.wav opened from MRU).
Insert Silence and GoTo allows position beyound the file.
Reopen files dialog size too small.
After Save As, The document title is still the old one.
When dialogs are resized, combo box edit text is selected and reset to default.
Selection dialog does not find region in the Selection combo.
Mute undo (and all other UNDO) doesn't restore selection
Resample: selection not adjusted
SaveAs with the same name fails (Unable to rename)
Context menu in Decibel ruler over spectrum section is inherited from ScaledScrollView and doesn't work as expected.
Spin doesn't work in DC dialog
After Reset button, Filter graph mouse/keyboard interface doesn't work
Stopped WMA save crashes the program.
When Split To Files stopped, unsaved files are not deleted
Beta 0.710:
Save split to files shows MP3 formats for MP3 codec with different sample rate, even though "Show compatible formats" is selected
Selected Mpeg3 compressor is not saved
Go to max peak goes a bit off
mm:ss.FF if not available in Marker/Region dialog
Unable to reopen after Save As compressed
Compressed data saved corrupted!
Save Copy (or save compressed or Save As): does not save markers!
Resample doesn't set modified flag
If a clipboard operation gets stopped, need to cancel all the operations that depend on it.
MRU list doesn't show directories
Simple sample rate change did not work
File resample doesn't tell about overflow
When scrolling during playback, checkered background after EOF is constantly blinking (caused by LCD)
Reopen after save new file doesn't work	 (test with marker set/change)
Losing metadata on save
Click interpolation: selection 2 samples long causes overflow
No redo after "Undo Interpolate"
New file conversion: mono to stereo - does not work
Wrong normalization coefficient
Click interpolation: divide by zero could occur
FFT view - last (incomplete) column is drawn from the first
Spectrum section: if FFT is zoomed vertically, section is not redrawn
Last WavePeak is not rescanned when file length is cut
Expression evaluation selection longer than file length doesn't update file length
Wrong CP used for UNICODE->ANSI conversion
Using up-down controls doesn't cause any "release" notification
Lines duplicated when scrolling the horizontal ruler
If delete (shrink) is done with one channel of two, fill the rest with zeros
Playback cursor not shown properly
Saving short file with zeros as u-law doesn't save anything
Mono file with compression is not fuly saved
When play operation stops, played range is redrawn
samples with 32767, -32768 are not visible
Wrong file length if the direct file is not saved. Need Always update RIFF/data
Scrolling with scrollbar may corrupt sample number on the hor ruler
"Save as" from LLADPCM: PCM16 is not shown when "Compatible only" selected
Wave data change doesn't update FFT sometimes
During playback, outline is invalidated. (??)
After file length increased to 1 sample from 0, scroll bar set to wrond scale
Initial rectangle is not drawn in the outline window for WMA and compressed file.
Open dialog doesn't show attributes for PCM format.
Crash with old wspk file?
Wrong dialog when saving/loading filter file.
WMA vertical ruler is not updated
Cache buffers discarded too early.
File outline doesn't scale properly
Cache prefetch goes way ahead
Playback position notification does invalidate. playback cursor is not moving.
Resampling AND mono->stereo change breaks the conversion!
Non-direct file: Save As with compression and the same name failed
"Operate Directly On the sound file" truncated
If there is a selection, Ctrl+Shift+End, Ctrl+Shift+Home doesn't work as expected
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

Deferred or not confirmed:
Selection mode in FFT view may not exit on capture loss
Suggests u-Law when saving a file from clipboard (??)
Daylight saving time change invalidates peak info timestamp (FAT only??)
doesn't show caret on the outline (as designed)
32 kbps file reading does too much read ahead (1.wma)

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
Support WAVEFORMATEXTENSIBLE.
Make multichannel editing
allow 24/32 bit data
Allow "Minimized" channels

For Version 2:
Make customizable context menu in views
Make middle button scroll (drop anchor window)
Add Undo/Redo drop list
Make a few source samples available in expression and a few output samples too
    use wave[-63...63]
    wave(1)
    wave_ch[1, 1...63]

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

-----------------------------------------------------------------

Attributes to read from WMA/MP3/WAV file:

WM/AlbumTitle
WM/Author	 (artist)
WM/Title  (file title)
WM/Year
WM/Genre (ignore??)
WM/Description  (comment)

WAV file keeps all the meta information in the end of file. 
If it is in the beginning, it may be moved to the end.

When the file needs expansion, this information is moved.
Copy of info is always kept in the memory.
Total size of metainformation is limited to 256 kbytes.

Common data may be allocated at any time.
The structures need copying, when reallocated for the bigger size.
For the given class, size of the structure is always constant.
If old size of structure was less than new size, the added part needs constructing.
When the file is closed, the structure needs deletion. Destructor is virtual.

The following chunks are processed:

LIST INFO
    IART    Original Artist
    ICMT    Comments
    ICOP    Copyright
    IENG    Engineers
    IGNR    Genre
    IKEY    Keywords
    IMED    Original medium
    INAM    Name
    ISRC    Source supplier
    ITCH    Digitizer
    ISBJ    Subject
    ISRF    Digitization source
cue     Cue sheet
LIST adtl
    ltxt
    labl
    note
DISP    display title


Locks:

Direct cache lock m_cs:

locks m_FileList, m_MruList,

Revisit FFT way of locating large clicks:

1. Do low-order (64 bands) FFT with Hamming window. 
2. Keep sliding average in every band.
3. Detect a peak when the current value in higher half is over sliding average by some threshold.
4. Locate a peak by finding where FFT power is max (by sliding it)
