Known problems and tasks:

Make context for background WriteRestOfTheFile
Make save presets for Expression Evaluation command
Add noise reduction estimation in spectrum section view
Make delay load for Windows Media DLL, to work on systems without WMP installed
Add MP3 save
Add ASF save
Use waveformat instead of template, for FileNew
If read-only file is saved, name should be different
Add spin support for selection, insert silence and GOTO dialog.
Statistics: zero crossings shows double the frequency (as supposed??)
Make Paste Special command
???? When a file is opened in non-direct mode, peak info is saved with wrong time stamp
After Save As, peak info is not saved for the new PCM file.
???? File created from clipboard and saved can't be reopened in direct mode.
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
Add options dialog
Save current workspace
Add decibel view to CAmplitudeRuler
Add VU meter for playback
Make all correct error messages and message boxes
Allow "Minimized" channels
Add sound recording
Add CD grabbing
Add mouse wheel support
Add autoscroll during selection

Done:
Make Insert Silence command
Show current scale in the status bar.
Fft view didn't get focus after switch
support copy from a file (FileName) in DoCopy
When number of channels changed after save, view parameters are screwed up.
Sound and FFT view jerks during playback of the last screen.
Lost sync between horz ruler scrolling and waveform.
After zoom in max, cursor jumps to not the view center.
Noise reduction doesn't save aggressivness parameter.
Make horizontal zoom and context menu for Spectrum Section View.
Show playback position spectrum in Spectrum Section View
Wrong selection display in FFT view.


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