Known problems and tasks:

Show current scale in the "static" child control.
Vertical ruler doesn't draw
Vertical ruler makes horizontal scroll
Add a static for another 'hole'.
Add: customized File Open dialog, to allow different modes and multiple files
Add Selection... command and dialog
Add GoTo dialog and command
Add undo/redo operation contexts
Add Cut and Delete commands
Add Drag and Drop support
Add options dialog

Add mouse wheel support
Add autoscroll during selection

Fixed: Prb: Fft view is not linked to wave view
Done:Add file length update in SoundChanged
Done:Add copy to a selected channel to CCopyContext
Done:Add file Expand/shrink functionality to CCopyContext
Done:Add peak data update
Done:Add "Paste" as insert data
Done:Fix selection boundary  when direction changed (anchor point)
Done:Fix mouse cursor over caret without selection
Done: Put '*' in the title for the modified document
Fixed: Problem: use the title without appendices for doc operations
Done: Don't flush data on close for DeleteOnClose file
Done: Flush data on timer in DirectFileCache::_ThreadProc
Done:Adjust priorities in ReadDataBlock (block protected by m_FileLock)
	for better cooperation of threads
Done: Add Select All command
