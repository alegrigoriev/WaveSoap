// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

// Do not include this file directly (included by AFXWIN.H)

/////////////////////////////////////////////////////////////////////////////

// Entries in a message map (a 'AFX_MSGMAP_ENTRY') table can be of 9 formats
//
// 1) control notification message (i.e. in response to WM_COMMAND)
//      WM_COMMAND, nNotifyCode, nControlID, nControlID, signature type, parameterless member function
//      (eg: WM_COMMAND, LBN_SELCHANGE, IDC_LISTBOX, AfxSig_vv, ... )
// 2) control notification message range (i.e. in response to WM_COMMAND)
//      WM_COMMAND, nNotifyCode, nControlIDFirst, nControlIDLast, signature type, parameterless member function
//      (eg: WM_COMMAND, LBN_SELCHANGE, IDC_LISTBOX1, IDC_LISTBOX5, AfxSig_vw, ... )
// 3) WM_NOTIFY notification
//      WM_NOTIFY, nNotifyCode, nControlID, nControlID, signature type, ...)
// 3) Update Command UI
//      -1, 0, nControlID, 0, signature Unknown, parameterless member function
// 4) Update Command UI Range
//      -1, 0, nControlIDFirst, nControlIDLast, signature Unknown, parameterless member function
// 5) menu/accelerator notification message (i.e. special case of first format)
//      WM_COMMAND, 0, nID, 0, signature type, parameterless member function
//      (eg: WM_COMMAND, 0, IDM_FILESAVE, 0, AfxSig_vv, ... )
// 6) menu/accelerator notification message range
//      WM_COMMAND, 0, nIDFirst, nIDLast, signature type, parameterless member function
//      (eg: WM_COMMAND, 0, IDM_FILE_MRU1, IDM_FILE_MRU4, AfxSig_vw, ... )
// 7) constant windows message
//      nMessage, 0, 0, 0, signature type, member function
//      (eg: WM_PAINT, 0, ...)
// 8) variable windows message (using RegisterWindowMessage)
//      0xC000, 0, 0, 0, &nMessage, special member function
//
// The end of the message map is marked with a special value
//      0, 0, AfxSig_end, 0
/////////////////////////////////////////////////////////////////////////////

enum AfxSig
{
	AfxSig_end = 0,     // [marks end of message map]

	AfxSig_bD,      // BOOL (CDC*)
	AfxSig_bb,      // BOOL (BOOL)
	AfxSig_bWww,    // BOOL (CWnd*, UINT, UINT)
	AfxSig_hDWw,    // HBRUSH (CDC*, CWnd*, UINT)
	AfxSig_hDw,     // HBRUSH (CDC*, UINT)
	AfxSig_iwWw,    // int (UINT, CWnd*, UINT)
	AfxSig_iww,     // int (UINT, UINT)
	AfxSig_iWww,    // int (CWnd*, UINT, UINT)
	AfxSig_is,      // int (LPTSTR)
	AfxSig_lwl,     // LRESULT (WPARAM, LPARAM)
	AfxSig_lwwM,    // LRESULT (UINT, UINT, CMenu*)
	AfxSig_vv,      // void (void)

	AfxSig_vw,      // void (UINT)
	AfxSig_vww,     // void (UINT, UINT)
	AfxSig_vvii,    // void (int, int) // wParam is ignored
	AfxSig_vwww,    // void (UINT, UINT, UINT)
	AfxSig_vwii,    // void (UINT, int, int)
	AfxSig_vwl,     // void (UINT, LPARAM)
	AfxSig_vbWW,    // void (BOOL, CWnd*, CWnd*)
	AfxSig_vD,      // void (CDC*)
	AfxSig_vM,      // void (CMenu*)
	AfxSig_vMwb,    // void (CMenu*, UINT, BOOL)

	AfxSig_vW,      // void (CWnd*)
	AfxSig_vWww,    // void (CWnd*, UINT, UINT)
	AfxSig_vWp,     // void (CWnd*, CPoint)
	AfxSig_vWh,     // void (CWnd*, HANDLE)
	AfxSig_vwW,     // void (UINT, CWnd*)
	AfxSig_vwWb,    // void (UINT, CWnd*, BOOL)
	AfxSig_vwwW,    // void (UINT, UINT, CWnd*)
	AfxSig_vwwx,    // void (UINT, UINT)
	AfxSig_vs,      // void (LPTSTR)
	AfxSig_vOWNER,  // void (int, LPTSTR), force return TRUE
	AfxSig_iis,     // int (int, LPTSTR)
	AfxSig_wp,      // UINT (CPoint)
	AfxSig_wv,      // UINT (void)
	AfxSig_vPOS,    // void (WINDOWPOS*)
	AfxSig_vCALC,   // void (BOOL, NCCALCSIZE_PARAMS*)
	AfxSig_vNMHDRpl,    // void (NMHDR*, LRESULT*)
	AfxSig_bNMHDRpl,    // BOOL (NMHDR*, LRESULT*)
	AfxSig_vwNMHDRpl,   // void (UINT, NMHDR*, LRESULT*)
	AfxSig_bwNMHDRpl,   // BOOL (UINT, NMHDR*, LRESULT*)
	AfxSig_bHELPINFO,   // BOOL (HELPINFO*)
	AfxSig_vwSIZING,    // void (UINT, LPRECT) -- return TRUE

	// signatures specific to CCmdTarget
	AfxSig_cmdui,   // void (CCmdUI*)
	AfxSig_cmduiw,  // void (CCmdUI*, UINT)
	AfxSig_vpv,     // void (void*)
	AfxSig_bpv,     // BOOL (void*)

	// Other aliases (based on implementation)
	AfxSig_vwwh,                // void (UINT, UINT, HANDLE)
	AfxSig_vwp,                 // void (UINT, CPoint)
	AfxSig_bw = AfxSig_bb,      // BOOL (UINT)
	AfxSig_bh = AfxSig_bb,      // BOOL (HANDLE)
	AfxSig_iw = AfxSig_bb,      // int (UINT)
	AfxSig_ww = AfxSig_bb,      // UINT (UINT)
	AfxSig_bv = AfxSig_wv,      // BOOL (void)
	AfxSig_hv = AfxSig_wv,      // HANDLE (void)
	AfxSig_vb = AfxSig_vw,      // void (BOOL)
	AfxSig_vbh = AfxSig_vww,    // void (BOOL, HANDLE)
	AfxSig_vbw = AfxSig_vww,    // void (BOOL, UINT)
	AfxSig_vhh = AfxSig_vww,    // void (HANDLE, HANDLE)
	AfxSig_vh = AfxSig_vw,      // void (HANDLE)
	AfxSig_viSS = AfxSig_vwl,   // void (int, STYLESTRUCT*)
	AfxSig_bwl = AfxSig_lwl,
	AfxSig_vwMOVING = AfxSig_vwSIZING,  // void (UINT, LPRECT) -- return TRUE

	AfxSig_vW2,                 // void (CWnd*) (CWnd* comes from lParam)
	AfxSig_bWCDS,               // BOOL (CWnd*, COPYDATASTRUCT*)
	AfxSig_bwsp,                // BOOL (UINT, short, CPoint)
	AfxSig_vws,
};

/////////////////////////////////////////////////////////////////////////////
// Command notifications for CCmdTarget notifications

#define CN_COMMAND              0               // void ()
#define CN_UPDATE_COMMAND_UI    ((UINT)(-1))    // void (CCmdUI*)
#define CN_EVENT                ((UINT)(-2))    // OLE event
#define CN_OLECOMMAND           ((UINT)(-3))    // OLE document command
#define CN_OLE_UNREGISTER       ((UINT)(-4))    // OLE unregister
// > 0 are control notifications
// < 0 are for MFC's use
class CWnd;
    typedef BOOL (CWnd::* fAfxSig_bD)(CDC*);
    typedef BOOL (CWnd::* fAfxSig_bb)(BOOL);
    typedef BOOL (CWnd::* fAfxSig_bw)(UINT);
    typedef BOOL (CCmdTarget::* fcAfxSig_bw)(UINT);
    typedef BOOL (CWnd::* fAfxSig_bWww)(CWnd*, UINT, UINT);
    typedef HBRUSH (CWnd::* fAfxSig_hDWw)(CDC*, CWnd*, UINT);
    typedef HBRUSH (CWnd::* fAfxSig_hDw)(CDC*, UINT);
    typedef int (CWnd::* fAfxSig_iwWw)(UINT, CWnd*, UINT);
    typedef int (CWnd::* fAfxSig_iwLBw)(UINT, CListBox*, UINT);
    typedef int (CWnd::* fAfxSig_iww)(UINT, UINT);
    typedef int (CWnd::* fAfxSig_iWww)(CWnd*, UINT, UINT);
    typedef int (CWnd::* fAfxSig_is)(LPTSTR);
    typedef int (CWnd::* fAfxSig_iCREATE)(LPCREATESTRUCT);
    typedef LRESULT (CWnd::* fAfxSig_lwl)(WPARAM, LPARAM);
    typedef LRESULT (CWnd::* fAfxSig_lwwM)(UINT, UINT, CMenu*);
    typedef void (CWnd::* fAfxSig_vw)(UINT);
    typedef void (CWnd::* fAfxSig_vv)();
    typedef void (CCmdTarget::* fcAfxSig_vv)();
    typedef void (CCmdTarget::* fcAfxSig_vw)(UINT);
    typedef void (CWnd::* fAfxSig_vww)(UINT, UINT);
    typedef void (CWnd::* fAfxSig_vvii)(int, int);
    typedef void (CWnd::* fAfxSig_vwww)(UINT, UINT, UINT);
    typedef void (CWnd::* fAfxSig_vwii)(UINT, int, int);
    typedef void (CWnd::* fAfxSig_vwl)(UINT, LPARAM);
    typedef void (CWinThread::* ftAfxSig_vwl)(UINT, LPARAM);
    typedef void (CWnd::* fAfxSig_vbWW)(BOOL, CWnd*, CWnd*);
    typedef void (CWnd::* fAfxSig_vD)(CDC*);
    typedef void (CWnd::* fAfxSig_vM)(CMenu*);
    typedef void (CWnd::* fAfxSig_vMwb)(CMenu*, UINT, BOOL);
    typedef void (CWnd::* fAfxSig_vW)(CWnd*);
    typedef void (CWnd::* fAfxSig_vWww)(CWnd*, UINT, UINT);
    typedef void (CWnd::* fAfxSig_vWp)(CWnd*, CPoint);
    typedef void (CWnd::* fAfxSig_vWh)(CWnd*, HANDLE);
    typedef void (CWnd::* fAfxSig_vwW)(UINT, CWnd*);
    typedef void (CWnd::* fAfxSig_vwWb)(UINT, CWnd*, BOOL);
    typedef void (CWnd::* fAfxSig_vwwW)(UINT, UINT, CWnd*);
    typedef void (CWnd::* fAfxSig_vwwSB)(UINT, UINT, CScrollBar*);
    typedef void (CWnd::* fAfxSig_vwwx)(UINT, UINT);
    typedef void (CWnd::* fAfxSig_vs)(LPTSTR);
    typedef void (CWnd::* fAfxSig_vMM)(MINMAXINFO *);
    typedef void (CWnd::* fAfxSig_vOWNER)(int, LPTSTR);
    typedef void (CWnd::* fAfxSig_viDRAWITEM)(int, LPDRAWITEMSTRUCT);
    typedef void (CWnd::* fAfxSig_vDRAWITEM)(LPDRAWITEMSTRUCT);
    typedef void (CWnd::* fAfxSig_viMEASUREITEM)(int, LPMEASUREITEMSTRUCT);
    typedef void (CWnd::* fAfxSig_vMEASUREITEM)(LPMEASUREITEMSTRUCT);
    typedef void (CWnd::* fAfxSig_viDELETEITEM)(int, LPDELETEITEMSTRUCT);
    typedef void (CWnd::* fAfxSig_vDELETEITEM)(LPDELETEITEMSTRUCT);
    typedef int (CWnd::* fAfxSig_iis)(int, LPTSTR);
    typedef UINT (CWnd::* fAfxSig_wp)(CPoint);
    typedef UINT (CWnd::* fAfxSig_wv)();
    typedef HANDLE (CWnd::* fAfxSig_hv)();
    typedef BOOL (CWnd::* fAfxSig_bv)();
    typedef BOOL (CCmdTarget::* fcAfxSig_bv)();
    typedef void (CWnd::* fAfxSig_vPOS)(WINDOWPOS*);
    typedef void (CWnd::* fAfxSig_vCALC)(BOOL, NCCALCSIZE_PARAMS*);
    typedef void (CWnd::* fAfxSig_vNMHDRpl)(NMHDR*, LRESULT*);
    typedef void (CCmdTarget::* fcAfxSig_vNMHDRpl)(NMHDR*, LRESULT*);
    typedef BOOL (CWnd::* fAfxSig_bNMHDRpl)(NMHDR*, LRESULT*);
    typedef void (CWnd::* fAfxSig_vwNMHDRpl)(UINT, NMHDR*, LRESULT*);
    typedef void (CCmdTarget::* fcAfxSig_vwNMHDRpl)(UINT, NMHDR*, LRESULT*);
    typedef BOOL (CWnd::* fAfxSig_bwNMHDRpl)(UINT, NMHDR*, LRESULT*);
    typedef BOOL (CCmdTarget::* fcAfxSig_bwNMHDRpl)(UINT, NMHDR*, LRESULT*);
    typedef BOOL (CWnd::* fAfxSig_bHELPINFO)(HELPINFO*);
    typedef void (CWnd::* fAfxSig_vwSIZING)(UINT, LPRECT);
    typedef void (CCmdTarget::* fAfxSig_cmdui)(CCmdUI*);
    typedef void (CCmdTarget::* fAfxSig_cmduiw)(CCmdUI*, UINT);
    typedef void (CCmdTarget::* fAfxSig_vpv)(void*);
    typedef BOOL (CCmdTarget::* fAfxSig_bpv)(void*);
    typedef BOOL (CWnd::* fAfxSig_bwsp)(UINT, short, CPoint);
    typedef void (CWnd::* fAfxSig_vwp)(UINT, CPoint);
#if 0
    typedef BOOL (CWnd::* f)();
    typedef BOOL (CWnd::* f)();
    typedef BOOL (CWnd::* f)();
    typedef BOOL (CWnd::* f)();
    typedef BOOL (CWnd::* f)();
#endif
#define ON_COMMAND(id, memberFxn) \
	{ WM_COMMAND, CN_COMMAND, (WORD)id, (WORD)id, AfxSig_vv, \
    (AFX_PMSG)static_cast<fcAfxSig_vv>(memberFxn) },
		// ON_COMMAND(id, OnFoo) is the same as
		//   ON_CONTROL(0, id, OnFoo) or ON_BN_CLICKED(0, id, OnFoo)

#define ON_COMMAND_RANGE(id, idLast, memberFxn) \
	{ WM_COMMAND, CN_COMMAND, (WORD)id, (WORD)idLast, AfxSig_vw, \
    (AFX_PMSG)static_cast<fcAfxSig_vw>(memberFxn)  },
		// ON_COMMAND_RANGE(id, idLast, OnFoo) is the same as
		//   ON_CONTROL_RANGE(0, id, idLast, OnFoo)

#define ON_COMMAND_EX(id, memberFxn) \
	{ WM_COMMAND, CN_COMMAND, (WORD)id, (WORD)id, AfxSig_bw, \
    (AFX_PMSG)static_cast<fcAfxSig_bw>(memberFxn) },

#define ON_COMMAND_EX_RANGE(id, idLast, memberFxn) \
	{ WM_COMMAND, CN_COMMAND, (WORD)id, (WORD)idLast, AfxSig_bw, \
    (AFX_PMSG)static_cast<fcAfxSig_bw>(memberFxn)},

// update ui's are listed as WM_COMMAND's so they get routed like commands
#define ON_UPDATE_COMMAND_UI(id, memberFxn) \
	{ WM_COMMAND, CN_UPDATE_COMMAND_UI, (WORD)id, (WORD)id, AfxSig_cmdui, \
    (AFX_PMSG)static_cast<fAfxSig_cmdui>(memberFxn) },

#define ON_UPDATE_COMMAND_UI_RANGE(id, idLast, memberFxn) \
	{ WM_COMMAND, CN_UPDATE_COMMAND_UI, (WORD)id, (WORD)idLast, AfxSig_cmdui, \
    (AFX_PMSG)static_cast<fAfxSig_cmdui>(memberFxn) },

#define ON_NOTIFY(wNotifyCode, id, memberFxn) \
	{ WM_NOTIFY, (WORD)(int)wNotifyCode, (WORD)id, (WORD)id, AfxSig_vNMHDRpl, \
    (AFX_PMSG)static_cast<fcAfxSig_vNMHDRpl>(memberFxn) },

#define ON_NOTIFY_RANGE(wNotifyCode, id, idLast, memberFxn) \
	{ WM_NOTIFY, (WORD)(int)wNotifyCode, (WORD)id, (WORD)idLast, AfxSig_vwNMHDRpl, \
    (AFX_PMSG)static_cast<fcAfxSig_vwNMHDRpl>(memberFxn) },

#define ON_NOTIFY_EX(wNotifyCode, id, memberFxn) \
	{ WM_NOTIFY, (WORD)(int)wNotifyCode, (WORD)id, (WORD)id, AfxSig_bwNMHDRpl, \
    (AFX_PMSG)static_cast<fcAfxSig_bwNMHDRpl>(memberFxn) },

#define ON_NOTIFY_EX_RANGE(wNotifyCode, id, idLast, memberFxn) \
	{ WM_NOTIFY, (WORD)(int)wNotifyCode, (WORD)id, (WORD)idLast, AfxSig_bwNMHDRpl, \
    (AFX_PMSG)static_cast<fcAfxSig_bwNMHDRpl>(memberFxn) },

// for general controls
#define ON_CONTROL(wNotifyCode, id, memberFxn) \
	{ WM_COMMAND, (WORD)wNotifyCode, (WORD)id, (WORD)id, AfxSig_vv, \
    (AFX_PMSG)static_cast<fAfxSig_vv>(memberFxn) },

#define ON_CONTROL_RANGE(wNotifyCode, id, idLast, memberFxn) \
	{ WM_COMMAND, (WORD)wNotifyCode, (WORD)id, (WORD)idLast, AfxSig_vw, \
    (AFX_PMSG)static_cast<fcAfxSig_vw>(memberFxn) },

#define WM_REFLECT_BASE 0xBC00

// for control notification reflection
#define ON_CONTROL_REFLECT(wNotifyCode, memberFxn) \
	{ WM_COMMAND+WM_REFLECT_BASE, (WORD)wNotifyCode, 0, 0, AfxSig_vv, \
    (AFX_PMSG)static_cast<fAfxSig_vv>(memberFxn) },

#define ON_CONTROL_REFLECT_EX(wNotifyCode, memberFxn) \
	{ WM_COMMAND+WM_REFLECT_BASE, (WORD)wNotifyCode, 0, 0, AfxSig_bv, \
    (AFX_PMSG)static_cast<fcAfxSig_bv>(memberFxn) },

#define ON_NOTIFY_REFLECT(wNotifyCode, memberFxn) \
	{ WM_NOTIFY+WM_REFLECT_BASE, (WORD)(int)wNotifyCode, 0, 0, AfxSig_vNMHDRpl, \
    (AFX_PMSG)static_cast<fcAfxSig_vNMHDRpl>(memberFxn) },

#define ON_NOTIFY_REFLECT_EX(wNotifyCode, memberFxn) \
	{ WM_NOTIFY+WM_REFLECT_BASE, (WORD)(int)wNotifyCode, 0, 0, AfxSig_bNMHDRpl, \
    (AFX_PMSG)static_cast<fcAfxSig_bNMHDRpl>(memberFxn) },

#define ON_UPDATE_COMMAND_UI_REFLECT(memberFxn) \
	{ WM_COMMAND+WM_REFLECT_BASE, (WORD)CN_UPDATE_COMMAND_UI, 0, 0, AfxSig_cmdui, \
    (AFX_PMSG)static_cast<fAfxSig_cmdui>(memberFxn) },

/////////////////////////////////////////////////////////////////////////////
// Message map tables for Windows messages

#define ON_WM_CREATE() \
	{ WM_CREATE, 0, 0, 0, AfxSig_is, \
    (AFX_PMSG)static_cast<fAfxSig_iCREATE>(OnCreate) },
#define ON_WM_COPYDATA() \
	{ WM_COPYDATA, 0, 0, 0, AfxSig_bWCDS, \
    (AFX_PMSG)static_cast<fAfxSig_bWCDS>(OnCopyData) },
#define ON_WM_DESTROY() \
	{ WM_DESTROY, 0, 0, 0, AfxSig_vv, \
    (AFX_PMSG)static_cast<fAfxSig_vv>(OnDestroy) },
#define ON_WM_MOVE() \
	{ WM_MOVE, 0, 0, 0, AfxSig_vvii, \
    (AFX_PMSG)static_cast<fAfxSig_vvii>(OnMove) },
#define ON_WM_SIZE() \
	{ WM_SIZE, 0, 0, 0, AfxSig_vwii, \
    (AFX_PMSG)static_cast<fAfxSig_vwii>(OnSize) },
#define ON_WM_ACTIVATE() \
	{ WM_ACTIVATE, 0, 0, 0, AfxSig_vwWb, \
    (AFX_PMSG)static_cast<fAfxSig_vwWb>(OnActivate) },
#define ON_WM_SETFOCUS() \
	{ WM_SETFOCUS, 0, 0, 0, AfxSig_vW, \
    (AFX_PMSG)static_cast<fAfxSig_vW>(OnSetFocus) },
#define ON_WM_KILLFOCUS() \
	{ WM_KILLFOCUS, 0, 0, 0, AfxSig_vW, \
    (AFX_PMSG)static_cast<fAfxSig_vW>(OnKillFocus) },
#define ON_WM_ENABLE() \
	{ WM_ENABLE, 0, 0, 0, AfxSig_vb, \
    (AFX_PMSG)static_cast<fAfxSig_vb>(OnEnable) },
#define ON_WM_HELPINFO() \
	{ WM_HELP, 0, 0, 0, AfxSig_bHELPINFO, \
    (AFX_PMSG)static_cast<fAfxSig_bHELPINFO>(OnHelpInfo) },
#define ON_WM_PAINT() \
	{ WM_PAINT, 0, 0, 0, AfxSig_vv, \
    (AFX_PMSG)static_cast<fAfxSig_vv>(OnPaint) },
#define ON_WM_CLOSE() \
	{ WM_CLOSE, 0, 0, 0, AfxSig_vv, \
    (AFX_PMSG)static_cast<fAfxSig_vv>(OnClose) },
#define ON_WM_QUERYENDSESSION() \
	{ WM_QUERYENDSESSION, 0, 0, 0, AfxSig_bv, \
    (AFX_PMSG)static_cast<fAfxSig_bv>(OnQueryEndSession) },
#define ON_WM_QUERYOPEN() \
	{ WM_QUERYOPEN, 0, 0, 0, AfxSig_bv, \
    (AFX_PMSG)static_cast<fAfxSig_bv>(OnQueryOpen) },
#define ON_WM_ERASEBKGND() \
	{ WM_ERASEBKGND, 0, 0, 0, AfxSig_bD, \
    (AFX_PMSG)static_cast<fAfxSig_bD>(OnEraseBkgnd) },
#define ON_WM_SYSCOLORCHANGE() \
	{ WM_SYSCOLORCHANGE, 0, 0, 0, AfxSig_vv, \
    (AFX_PMSG)static_cast<fAfxSig_vv>(OnSysColorChange) },
#define ON_WM_ENDSESSION() \
	{ WM_ENDSESSION, 0, 0, 0, AfxSig_vb, \
    (AFX_PMSG)static_cast<fAfxSig_vb>(OnEndSession) },
#define ON_WM_SHOWWINDOW() \
	{ WM_SHOWWINDOW, 0, 0, 0, AfxSig_vbw, \
    (AFX_PMSG)static_cast<fAfxSig_vbw>(OnShowWindow) },
#define ON_WM_CTLCOLOR() \
	{ WM_CTLCOLOR, 0, 0, 0, AfxSig_hDWw, \
    (AFX_PMSG)static_cast<fAfxSig_hDWw>(OnCtlColor) },
#define ON_WM_CTLCOLOR_REFLECT() \
	{ WM_CTLCOLOR+WM_REFLECT_BASE, 0, 0, 0, AfxSig_hDw, \
    (AFX_PMSG)static_cast<fAfxSig_hDw>(CtlColor) },
#if (WINVER >= 0x0400)
#define ON_WM_SETTINGCHANGE() \
	{ WM_SETTINGCHANGE, 0, 0, 0, AfxSig_vws, \
    (AFX_PMSG)static_cast<fAfxSig_vws>(OnSettingChange) },
#endif
#define ON_WM_WININICHANGE() \
	{ WM_WININICHANGE, 0, 0, 0, AfxSig_vs, \
    (AFX_PMSG)static_cast<fAfxSig_vs>(OnWinIniChange) },
#define ON_WM_DEVMODECHANGE() \
	{ WM_DEVMODECHANGE, 0, 0, 0, AfxSig_vs, \
    (AFX_PMSG)static_cast<fAfxSig_vs>(OnDevModeChange) },
#define ON_WM_ACTIVATEAPP() \
	{ WM_ACTIVATEAPP, 0, 0, 0, AfxSig_vbh, \
    (AFX_PMSG)static_cast<fAfxSig_vbh>(OnActivateApp) },
#define ON_WM_FONTCHANGE() \
	{ WM_FONTCHANGE, 0, 0, 0, AfxSig_vv, \
    (AFX_PMSG)static_cast<fAfxSig_vv>(OnFontChange) },
#define ON_WM_TIMECHANGE() \
	{ WM_TIMECHANGE, 0, 0, 0, AfxSig_vv, \
    (AFX_PMSG)static_cast<fAfxSig_vv>(OnTimeChange) },
#define ON_WM_CANCELMODE() \
	{ WM_CANCELMODE, 0, 0, 0, AfxSig_vv, \
    (AFX_PMSG)static_cast<fAfxSig_vv>(OnCancelMode) },
#define ON_WM_SETCURSOR() \
	{ WM_SETCURSOR, 0, 0, 0, AfxSig_bWww, \
    (AFX_PMSG)static_cast<fAfxSig_bWww>(OnSetCursor) },
#define ON_WM_MOUSEACTIVATE() \
	{ WM_MOUSEACTIVATE, 0, 0, 0, AfxSig_iWww, \
    (AFX_PMSG)static_cast<fAfxSig_iWww>(OnMouseActivate) },
#define ON_WM_CHILDACTIVATE() \
	{ WM_CHILDACTIVATE, 0, 0, 0, AfxSig_vv, \
    (AFX_PMSG)static_cast<fAfxSig_vv>(OnChildActivate) },
#define ON_WM_GETMINMAXINFO() \
	{ WM_GETMINMAXINFO, 0, 0, 0, AfxSig_vs, \
    (AFX_PMSG)static_cast<fAfxSig_vMM>(OnGetMinMaxInfo) },
#define ON_WM_ICONERASEBKGND() \
	{ WM_ICONERASEBKGND, 0, 0, 0, AfxSig_vD, \
    (AFX_PMSG)static_cast<fAfxSig_vD>(OnIconEraseBkgnd) },
#define ON_WM_SPOOLERSTATUS() \
	{ WM_SPOOLERSTATUS, 0, 0, 0, AfxSig_vww, \
    (AFX_PMSG)static_cast<fAfxSig_vww>(OnSpoolerStatus },
#define ON_WM_DRAWITEM() \
	{ WM_DRAWITEM, 0, 0, 0, AfxSig_vOWNER, \
    (AFX_PMSG)static_cast<fAfxSig_viDRAWITEM>(OnDrawItem) },
#define ON_WM_DRAWITEM_REFLECT() \
	{ WM_DRAWITEM+WM_REFLECT_BASE, 0, 0, 0, AfxSig_vs, \
    (AFX_PMSG)static_cast<fAfxSig_vDRAWITEM>(DrawItem) },
#define ON_WM_MEASUREITEM() \
	{ WM_MEASUREITEM, 0, 0, 0, AfxSig_vOWNER, \
    (AFX_PMSG)static_cast<fAfxSig_viMEASUREITEM>(OnMeasureItem) },
#define ON_WM_MEASUREITEM_REFLECT() \
	{ WM_MEASUREITEM+WM_REFLECT_BASE, 0, 0, 0, AfxSig_vs, \
    (AFX_PMSG)static_cast<fAfxSig_vMEASUREITEM>(MeasureItem) },
#define ON_WM_DELETEITEM() \
	{ WM_DELETEITEM, 0, 0, 0, AfxSig_vOWNER, \
    (AFX_PMSG)static_cast<fAfxSig_viDELETEITEM>(OnDeleteItem) },
#define ON_WM_DELETEITEM_REFLECT() \
	{ WM_DELETEITEM+WM_REFLECT_BASE, 0, 0, 0, AfxSig_vs, \
    (AFX_PMSG)static_cast<fAfxSig_vDELETEITEM>(DeleteItem) },
#define ON_WM_CHARTOITEM() \
	{ WM_CHARTOITEM, 0, 0, 0, AfxSig_iwWw, \
    (AFX_PMSG)static_cast<fAfxSig_iwLBw>(OnCharToItem) },
#define ON_WM_CHARTOITEM_REFLECT() \
	{ WM_CHARTOITEM+WM_REFLECT_BASE, 0, 0, 0, AfxSig_iww, \
    (AFX_PMSG)static_cast<fAfxSig_iww>(CharToItem) },
#define ON_WM_VKEYTOITEM() \
	{ WM_VKEYTOITEM, 0, 0, 0, AfxSig_iwWw, \
    (AFX_PMSG)static_cast<fAfxSig_iwLBw>(OnVKeyToItem) },
#define ON_WM_VKEYTOITEM_REFLECT() \
	{ WM_VKEYTOITEM+WM_REFLECT_BASE, 0, 0, 0, AfxSig_iww, \
    (AFX_PMSG)static_cast<fAfxSig_iww>(VKeyToItem },
#define ON_WM_QUERYDRAGICON() \
	{ WM_QUERYDRAGICON, 0, 0, 0, AfxSig_hv, \
		(AFX_PMSG)(AFX_PMSGW)(HCURSOR (AFX_MSG_CALL CWnd::*)())&OnQueryDragIcon },
#define ON_WM_COMPAREITEM() \
	{ WM_COMPAREITEM, 0, 0, 0, AfxSig_iis, \
		(AFX_PMSG)(AFX_PMSGW)(int (AFX_MSG_CALL CWnd::*)(int, LPCOMPAREITEMSTRUCT))&OnCompareItem },
#define ON_WM_COMPAREITEM_REFLECT() \
	{ WM_COMPAREITEM+WM_REFLECT_BASE, 0, 0, 0, AfxSig_is, \
		(AFX_PMSG)(AFX_PMSGW)(int (AFX_MSG_CALL CWnd::*)(LPCOMPAREITEMSTRUCT))CompareItem },
#define ON_WM_COMPACTING() \
	{ WM_COMPACTING, 0, 0, 0, AfxSig_vw, \
		(AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(UINT))&OnCompacting },
#define ON_WM_NCCREATE() \
	{ WM_NCCREATE, 0, 0, 0, AfxSig_is, \
    (AFX_PMSG)static_cast<fAfxSig_iCREATE>(OnNcCreate) },
#define ON_WM_NCDESTROY() \
	{ WM_NCDESTROY, 0, 0, 0, AfxSig_vv, \
    (AFX_PMSG)static_cast<fAfxSig_vv>(OnNcDestroy) },
#define ON_WM_NCCALCSIZE() \
	{ WM_NCCALCSIZE, 0, 0, 0, AfxSig_vCALC, \
    (AFX_PMSG)static_cast<fAfxSig_vCALC>(OnNcCalcSize) },
#define ON_WM_NCHITTEST() \
	{ WM_NCHITTEST, 0, 0, 0, AfxSig_wp, \
    (AFX_PMSG)static_cast<fAfxSig_wp>(OnNcHitTest) },
#define ON_WM_NCPAINT() \
	{ WM_NCPAINT, 0, 0, 0, AfxSig_vv, \
    (AFX_PMSG)static_cast<fAfxSig_vv>(OnNcPaint) },
#define ON_WM_NCACTIVATE() \
	{ WM_NCACTIVATE, 0, 0, 0, AfxSig_bb, \
    (AFX_PMSG)static_cast<fAfxSig_bb>(OnNcActivate) },
#define ON_WM_GETDLGCODE() \
	{ WM_GETDLGCODE, 0, 0, 0, AfxSig_wv, \
    (AFX_PMSG)static_cast<fAfxSig_wv>(OnGetDlgCode) },
#define ON_WM_NCMOUSEMOVE() \
	{ WM_NCMOUSEMOVE, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnNcMouseMove) },
#define ON_WM_NCLBUTTONDOWN() \
	{ WM_NCLBUTTONDOWN, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnNcLButtonDown) },
#define ON_WM_NCLBUTTONUP() \
	{ WM_NCLBUTTONUP, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnNcLButtonUp) },
#define ON_WM_NCLBUTTONDBLCLK() \
	{ WM_NCLBUTTONDBLCLK, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnNcLButtonDblClk) },
#define ON_WM_NCRBUTTONDOWN() \
	{ WM_NCRBUTTONDOWN, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnNcRButtonDown) },
#define ON_WM_NCRBUTTONUP() \
	{ WM_NCRBUTTONUP, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnNcRButtonUp) },
#define ON_WM_NCRBUTTONDBLCLK() \
	{ WM_NCRBUTTONDBLCLK, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnNcRButtonDblClk) },
#define ON_WM_NCMBUTTONDOWN() \
	{ WM_NCMBUTTONDOWN, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnNcMButtonDown) },
#define ON_WM_NCMBUTTONUP() \
	{ WM_NCMBUTTONUP, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnNcMButtonUp) },
#define ON_WM_NCMBUTTONDBLCLK() \
	{ WM_NCMBUTTONDBLCLK, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnNcMButtonDblClk) },
#define ON_WM_KEYDOWN() \
	{ WM_KEYDOWN, 0, 0, 0, AfxSig_vwww, \
    (AFX_PMSG)static_cast<fAfxSig_vwww>(OnKeyDown) },
#define ON_WM_KEYUP() \
	{ WM_KEYUP, 0, 0, 0, AfxSig_vwww, \
    (AFX_PMSG)static_cast<fAfxSig_vwww>(OnKeyUp) },
#define ON_WM_CHAR() \
	{ WM_CHAR, 0, 0, 0, AfxSig_vwww, \
    (AFX_PMSG)static_cast<fAfxSig_vwww>(OnChar) },
#define ON_WM_DEADCHAR() \
	{ WM_DEADCHAR, 0, 0, 0, AfxSig_vwww, \
    (AFX_PMSG)static_cast<fAfxSig_vwww>(OnDeadChar) },
#define ON_WM_SYSKEYDOWN() \
	{ WM_SYSKEYDOWN, 0, 0, 0, AfxSig_vwww, \
    (AFX_PMSG)static_cast<fAfxSig_vwww>(OnSysKeyDown) },
#define ON_WM_SYSKEYUP() \
	{ WM_SYSKEYUP, 0, 0, 0, AfxSig_vwww, \
    (AFX_PMSG)static_cast<fAfxSig_vwww>(OnSysKeyUp) },
#define ON_WM_SYSCHAR() \
	{ WM_SYSCHAR, 0, 0, 0, AfxSig_vwww, \
    (AFX_PMSG)static_cast<fAfxSig_vwww>(OnSysChar) },
#define ON_WM_SYSDEADCHAR() \
	{ WM_SYSDEADCHAR, 0, 0, 0, AfxSig_vwww, \
    (AFX_PMSG)static_cast<fAfxSig_vwww>(OnSysDeadChar) },
#define ON_WM_SYSCOMMAND() \
	{ WM_SYSCOMMAND, 0, 0, 0, AfxSig_vwl, \
    (AFX_PMSG)static_cast<fAfxSig_vwl>(OnSysCommand) },
#define ON_WM_TCARD() \
	{ WM_TCARD, 0, 0, 0, AfxSig_vwl, \
    (AFX_PMSG)static_cast<fAfxSig_vwl>(OnTCard },
#define ON_WM_TIMER() \
	{ WM_TIMER, 0, 0, 0, AfxSig_vw, \
    (AFX_PMSG)static_cast<fAfxSig_vw>(OnTimer) },
#define ON_WM_HSCROLL() \
	{ WM_HSCROLL, 0, 0, 0, AfxSig_vwwW, \
    (AFX_PMSG)static_cast<fAfxSig_vwwSB>(OnHScroll) },
#define ON_WM_HSCROLL_REFLECT() \
	{ WM_HSCROLL+WM_REFLECT_BASE, 0, 0, 0, AfxSig_vwwx, \
    (AFX_PMSG)static_cast<fAfxSig_vwwx>(HScroll) },
#define ON_WM_VSCROLL() \
	{ WM_VSCROLL, 0, 0, 0, AfxSig_vwwW, \
    (AFX_PMSG)static_cast<fAfxSig_vwwSB>(OnVScroll) },
#define ON_WM_VSCROLL_REFLECT() \
	{ WM_VSCROLL+WM_REFLECT_BASE, 0, 0, 0, AfxSig_vwwx, \
    (AFX_PMSG)static_cast<AfxSig_vwwx>(VScroll) },
#define ON_WM_INITMENU() \
	{ WM_INITMENU, 0, 0, 0, AfxSig_vM, \
    (AFX_PMSG)static_cast<fAfxSig_vM>(OnInitMenu) },
#define ON_WM_INITMENUPOPUP() \
	{ WM_INITMENUPOPUP, 0, 0, 0, AfxSig_vMwb, \
    (AFX_PMSG)static_cast<fAfxSig_vMwb>(OnInitMenuPopup) },
#define ON_WM_MENUSELECT() \
	{ WM_MENUSELECT, 0, 0, 0, AfxSig_vwwh, \
		(AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(UINT, UINT, HMENU))&OnMenuSelect },
#define ON_WM_MENUCHAR() \
	{ WM_MENUCHAR, 0, 0, 0, AfxSig_lwwM, \
		(AFX_PMSG)(AFX_PMSGW)(LRESULT (AFX_MSG_CALL CWnd::*)(UINT, UINT, CMenu*))&OnMenuChar },
#define ON_WM_ENTERIDLE() \
	{ WM_ENTERIDLE, 0, 0, 0, AfxSig_vwW, \
		(AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(UINT, CWnd*))&OnEnterIdle },
#define ON_WM_MOUSEMOVE() \
	{ WM_MOUSEMOVE, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnMouseMove) },
#define ON_WM_MOUSEWHEEL() \
	{ WM_MOUSEWHEEL, 0, 0, 0, AfxSig_bwsp, \
    (AFX_PMSG)static_cast<fAfxSig_bwsp>(OnMouseWheel) },
#define ON_WM_LBUTTONDOWN() \
	{ WM_LBUTTONDOWN, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnLButtonDown) },
#define ON_WM_LBUTTONUP() \
	{ WM_LBUTTONUP, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnLButtonUp) },
#define ON_WM_LBUTTONDBLCLK() \
	{ WM_LBUTTONDBLCLK, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnLButtonDblClk) },
#define ON_WM_RBUTTONDOWN() \
	{ WM_RBUTTONDOWN, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnRButtonDown) },
#define ON_WM_RBUTTONUP() \
	{ WM_RBUTTONUP, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnRButtonUp) },
#define ON_WM_RBUTTONDBLCLK() \
	{ WM_RBUTTONDBLCLK, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnRButtonDblClk) },
#define ON_WM_MBUTTONDOWN() \
	{ WM_MBUTTONDOWN, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnMButtonDown) },
#define ON_WM_MBUTTONUP() \
	{ WM_MBUTTONUP, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnMButtonUp) },
#define ON_WM_MBUTTONDBLCLK() \
	{ WM_MBUTTONDBLCLK, 0, 0, 0, AfxSig_vwp, \
    (AFX_PMSG)static_cast<fAfxSig_vwp>(OnMButtonDblClk) },
#define ON_WM_PARENTNOTIFY() \
	{ WM_PARENTNOTIFY, 0, 0, 0, AfxSig_vwl, \
    (AFX_PMSG)static_cast<fAfxSig_vwl>(OnParentNotify) },
#define ON_WM_PARENTNOTIFY_REFLECT() \
	{ WM_PARENTNOTIFY+WM_REFLECT_BASE, 0, 0, 0, AfxSig_vwl, \
    (AFX_PMSG)static_cast<fAfxSig_vwl>(ParentNotify) },
#define ON_WM_MDIACTIVATE() \
	{ WM_MDIACTIVATE, 0, 0, 0, AfxSig_vbWW, \
    (AFX_PMSG)static_cast<fAfxSig_vbWW>(OnMDIActivate) },
#define ON_WM_RENDERFORMAT() \
	{ WM_RENDERFORMAT, 0, 0, 0, AfxSig_vw, \
    (AFX_PMSG)static_cast<fAfxSig_vw>(OnRenderFormat) },
#define ON_WM_RENDERALLFORMATS() \
	{ WM_RENDERALLFORMATS, 0, 0, 0, AfxSig_vv, \
    (AFX_PMSG)static_cast<fAfxSig_vv>(OnRenderAllFormats) },
#define ON_WM_DESTROYCLIPBOARD() \
	{ WM_DESTROYCLIPBOARD, 0, 0, 0, AfxSig_vv, \
    (AFX_PMSG)static_cast<fAfxSig_vv>(OnDestroyClipboard) },
#define ON_WM_DRAWCLIPBOARD() \
	{ WM_DRAWCLIPBOARD, 0, 0, 0, AfxSig_vv, \
    (AFX_PMSG)static_cast<fAfxSig_vv>(OnDrawClipboard) },
#define ON_WM_PAINTCLIPBOARD() \
	{ WM_PAINTCLIPBOARD, 0, 0, 0, AfxSig_vWh, \
		(AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(CWnd*, HGLOBAL))&OnPaintClipboard },
#define ON_WM_VSCROLLCLIPBOARD() \
	{ WM_VSCROLLCLIPBOARD, 0, 0, 0, AfxSig_vWww, \
		(AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(CWnd*, UINT, UINT))&OnVScrollClipboard },
#define ON_WM_CONTEXTMENU() \
	{ WM_CONTEXTMENU, 0, 0, 0, AfxSig_vWp, \
    (AFX_PMSG)static_cast<fAfxSig_vWp>(OnContextMenu) },
#define ON_WM_SIZECLIPBOARD() \
	{ WM_SIZECLIPBOARD, 0, 0, 0, AfxSig_vWh, \
		(AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(CWnd*, HGLOBAL))&OnSizeClipboard },
#define ON_WM_ASKCBFORMATNAME() \
	{ WM_ASKCBFORMATNAME, 0, 0, 0, AfxSig_vwl, \
		(AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(UINT, LPTSTR))&OnAskCbFormatName },
#define ON_WM_CHANGECBCHAIN() \
	{ WM_CHANGECBCHAIN, 0, 0, 0, AfxSig_vhh, \
		(AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(HWND, HWND))&OnChangeCbChain },
#define ON_WM_HSCROLLCLIPBOARD() \
	{ WM_HSCROLLCLIPBOARD, 0, 0, 0, AfxSig_vWww, \
		(AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(CWnd*, UINT, UINT))&OnHScrollClipboard },
#define ON_WM_QUERYNEWPALETTE() \
	{ WM_QUERYNEWPALETTE, 0, 0, 0, AfxSig_bv, \
    (AFX_PMSG)static_cast<fAfxSig_bv>(OnQueryNewPalette) },
#define ON_WM_PALETTECHANGED() \
	{ WM_PALETTECHANGED, 0, 0, 0, AfxSig_vW, \
    (AFX_PMSG)static_cast<fAfxSig_vW>(OnPaletteChanged) },

#define ON_WM_PALETTEISCHANGING() \
	{ WM_PALETTEISCHANGING, 0, 0, 0, AfxSig_vW, \
    (AFX_PMSG)static_cast<fAfxSig_vW>(OnPaletteIsChanging) },
#define ON_WM_DROPFILES() \
	{ WM_DROPFILES, 0, 0, 0, AfxSig_vh, \
		(AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(HDROP))&OnDropFiles },
#define ON_WM_WINDOWPOSCHANGING() \
	{ WM_WINDOWPOSCHANGING, 0, 0, 0, AfxSig_vPOS, \
    (AFX_PMSG)static_cast<fAfxSig_vPOS>(OnWindowPosChanging) },
#define ON_WM_WINDOWPOSCHANGED() \
	{ WM_WINDOWPOSCHANGED, 0, 0, 0, AfxSig_vPOS, \
    (AFX_PMSG)static_cast<fAfxSig_vPOS>(OnWindowPosChanged) },

#define ON_WM_EXITMENULOOP() \
	{ WM_EXITMENULOOP, 0, 0, 0, AfxSig_vb, \
    (AFX_PMSG)static_cast<fAfxSig_vb>(OnExitMenuLoop) },
#define ON_WM_ENTERMENULOOP() \
	{ WM_ENTERMENULOOP, 0, 0, 0, AfxSig_vb, \
    (AFX_PMSG)static_cast<fAfxSig_vb>(OnEnterMenuLoop) },

#define ON_WM_STYLECHANGED() \
	{ WM_STYLECHANGED, 0, 0, 0, AfxSig_viSS, \
		(AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(int, LPSTYLESTRUCT))&OnStyleChanged },
#define ON_WM_STYLECHANGING() \
	{ WM_STYLECHANGING, 0, 0, 0, AfxSig_viSS, \
		(AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(int, LPSTYLESTRUCT))&OnStyleChanging },
#define ON_WM_SIZING() \
	{ WM_SIZING, 0, 0, 0, AfxSig_vwSIZING, \
    (AFX_PMSG)static_cast<fAfxSig_vwSIZING>(OnSizing) },
#define ON_WM_MOVING() \
	{ WM_MOVING, 0, 0, 0, AfxSig_vwMOVING, \
    (AFX_PMSG)static_cast<fAfxSig_vwMOVING>(OnMoving) },
#define ON_WM_CAPTURECHANGED() \
	{ WM_CAPTURECHANGED, 0, 0, 0, AfxSig_vW2, \
    (AFX_PMSG)static_cast<fAfxSig_vW>(OnCaptureChanged) },
#define ON_WM_DEVICECHANGE() \
	{ WM_DEVICECHANGE, 0, 0, 0, AfxSig_bwl, \
		(AFX_PMSG)(AFX_PMSGW)(BOOL (AFX_MSG_CALL CWnd::*)(UINT, DWORD))&OnDeviceChange },

/////////////////////////////////////////////////////////////////////////////
// Message map tables for Control Notification messages

// Static control notification codes
#define ON_STN_CLICKED(id, memberFxn) \
	ON_CONTROL(STN_CLICKED, id, memberFxn)
#define ON_STN_DBLCLK(id, memberFxn) \
	ON_CONTROL(STN_DBLCLK, id, memberFxn)
#define ON_STN_ENABLE(id, memberFxn) \
	ON_CONTROL(STN_ENABLE, id, memberFxn)
#define ON_STN_DISABLE(id, memberFxn) \
	ON_CONTROL(STN_DISABLE, id, memberFxn)


// Edit Control Notification Codes
#define ON_EN_SETFOCUS(id, memberFxn) \
	ON_CONTROL(EN_SETFOCUS, id, memberFxn)
#define ON_EN_KILLFOCUS(id, memberFxn) \
	ON_CONTROL(EN_KILLFOCUS, id, memberFxn)
#define ON_EN_CHANGE(id, memberFxn) \
	ON_CONTROL(EN_CHANGE, id, memberFxn)
#define ON_EN_UPDATE(id, memberFxn) \
	ON_CONTROL(EN_UPDATE, id, memberFxn)
#define ON_EN_ERRSPACE(id, memberFxn) \
	ON_CONTROL(EN_ERRSPACE, id, memberFxn)
#define ON_EN_MAXTEXT(id, memberFxn) \
	ON_CONTROL(EN_MAXTEXT, id, memberFxn)
#define ON_EN_HSCROLL(id, memberFxn) \
	ON_CONTROL(EN_HSCROLL, id, memberFxn)
#define ON_EN_VSCROLL(id, memberFxn) \
	ON_CONTROL(EN_VSCROLL, id, memberFxn)

// User Button Notification Codes
#define ON_BN_CLICKED(id, memberFxn) \
	ON_CONTROL(BN_CLICKED, id, memberFxn)
#define ON_BN_DOUBLECLICKED(id, memberFxn) \
	ON_CONTROL(BN_DOUBLECLICKED, id, memberFxn)
#define ON_BN_SETFOCUS(id, memberFxn) \
	ON_CONTROL(BN_SETFOCUS, id, memberFxn)
#define ON_BN_KILLFOCUS(id, memberFxn) \
	ON_CONTROL(BN_KILLFOCUS, id, memberFxn)

// old BS_USERBUTTON button notifications - obsolete in Win31
#define ON_BN_PAINT(id, memberFxn) \
	ON_CONTROL(BN_PAINT, id, memberFxn)
#define ON_BN_HILITE(id, memberFxn) \
	ON_CONTROL(BN_HILITE, id, memberFxn)
#define ON_BN_UNHILITE(id, memberFxn) \
	ON_CONTROL(BN_UNHILITE, id, memberFxn)
#define ON_BN_DISABLE(id, memberFxn) \
	ON_CONTROL(BN_DISABLE, id, memberFxn)

// Listbox Notification Codes
#define ON_LBN_ERRSPACE(id, memberFxn) \
	ON_CONTROL(LBN_ERRSPACE, id, memberFxn)
#define ON_LBN_SELCHANGE(id, memberFxn) \
	ON_CONTROL(LBN_SELCHANGE, id, memberFxn)
#define ON_LBN_DBLCLK(id, memberFxn) \
	ON_CONTROL(LBN_DBLCLK, id, memberFxn)
#define ON_LBN_SELCANCEL(id, memberFxn) \
	ON_CONTROL(LBN_SELCANCEL, id, memberFxn)
#define ON_LBN_SETFOCUS(id, memberFxn) \
	ON_CONTROL(LBN_SETFOCUS, id, memberFxn)
#define ON_LBN_KILLFOCUS(id, memberFxn) \
	ON_CONTROL(LBN_KILLFOCUS, id, memberFxn)

// Check Listbox Notification codes
#define CLBN_CHKCHANGE (40)
#define ON_CLBN_CHKCHANGE(id, memberFxn) \
	ON_CONTROL(CLBN_CHKCHANGE, id, memberFxn)

// Combo Box Notification Codes
#define ON_CBN_ERRSPACE(id, memberFxn) \
	ON_CONTROL(CBN_ERRSPACE, id, memberFxn)
#define ON_CBN_SELCHANGE(id, memberFxn) \
	ON_CONTROL(CBN_SELCHANGE, id, memberFxn)
#define ON_CBN_DBLCLK(id, memberFxn) \
	ON_CONTROL(CBN_DBLCLK, id, memberFxn)
#define ON_CBN_SETFOCUS(id, memberFxn) \
	ON_CONTROL(CBN_SETFOCUS, id, memberFxn)
#define ON_CBN_KILLFOCUS(id, memberFxn) \
	ON_CONTROL(CBN_KILLFOCUS, id, memberFxn)
#define ON_CBN_EDITCHANGE(id, memberFxn) \
	ON_CONTROL(CBN_EDITCHANGE, id, memberFxn)
#define ON_CBN_EDITUPDATE(id, memberFxn) \
	ON_CONTROL(CBN_EDITUPDATE, id, memberFxn)
#define ON_CBN_DROPDOWN(id, memberFxn) \
	ON_CONTROL(CBN_DROPDOWN, id, memberFxn)
#define ON_CBN_CLOSEUP(id, memberFxn)  \
	ON_CONTROL(CBN_CLOSEUP, id, memberFxn)
#define ON_CBN_SELENDOK(id, memberFxn)  \
	ON_CONTROL(CBN_SELENDOK, id, memberFxn)
#define ON_CBN_SELENDCANCEL(id, memberFxn)  \
	ON_CONTROL(CBN_SELENDCANCEL, id, memberFxn)

/////////////////////////////////////////////////////////////////////////////
// User extensions for message map entries

// for Windows messages
#define ON_MESSAGE(message, memberFxn) \
	{ message, 0, 0, 0, AfxSig_lwl, \
    (AFX_PMSG)static_cast<fAfxSig_lwl>(memberFxn) },

// for Registered Windows messages
#define ON_REGISTERED_MESSAGE(nMessageVariable, memberFxn) \
	{ 0xC000, 0, 0, 0, (UINT)(UINT*)(&nMessageVariable), \
		/*implied 'AfxSig_lwl'*/ \
    (AFX_PMSG)static_cast<fAfxSig_lwl>(memberFxn) },

// for Thread messages
#define ON_THREAD_MESSAGE(message, memberFxn) \
	{ message, 0, 0, 0, AfxSig_vwl, \
		(AFX_PMSG)static_cast<ftAfxSig_vwl>(memberFxn) },

// for Registered Windows messages
#define ON_REGISTERED_THREAD_MESSAGE(nMessageVariable, memberFxn) \
	{ 0xC000, 0, 0, 0, (UINT)(UINT*)(&nMessageVariable), \
		/*implied 'AfxSig_vwl'*/ \
		(AFX_PMSG)static_cast<ftAfxSig_vwl>(memberFxn) },

/////////////////////////////////////////////////////////////////////////////
