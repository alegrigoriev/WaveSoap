// ScaledGraphView.h
#ifndef __SCALED_GRAPH_VIEW_H__
#define __SCALED_GRAPH_VIEW_H__

#ifndef __SCALED_SCROLL_VIEW_H__
#include "ScaledScrollView.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// CScaledGraphView view

class CScaledGraphView : public CScaledScrollView
{
protected:
	CScaledGraphView();           // protected constructor used by dynamic creation
	DECLARE_DYNAMIC(CScaledGraphView);
public:
	void SetXTicks(double dTickStep);
	void SetYTicks(double dTickStep);
	void SetMinXTickSpacing(int nTickSpacing);
	void SetMaxXTickSpacing(int nTickSpacing);
	void SetMinYTickSpacing(int nTickSpacing);
	void SetMaxYTickSpacing(int nTickSpacing);
	void SetGraphStyle(DWORD Style);
	void SetGraphColor(COLORREF color);
	DWORD GetGraphStyle() const { return m_dwGraphStyle; }
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CScaledGraphView)
public:
	virtual void OnDraw(CDC *);
	//}}AFX_VIRTUAL
	virtual void PostDraw(CDC *);
	virtual BOOL OnNeedText( UINT id, NMHDR * pNotifyStruct, LRESULT * result );

	enum
	{
		SGV_STYLE_SOLID = 1,    // draw the graph with solid line
		SGV_STYLE_DOT   = 2,    // draw the graph as dots
		SGV_STYLE_BARS  = 4,    // draw the graph as bars from Y=0 line
		SGV_STYLE_TICKS = 8,    // draw ticks (as dashed lines)
		SGV_STYLE_MINMAX = 0x10,    // draw lines between min and max values
		SGV_STYLE_X_AXIS = 0x20,    // plot X axis (Y=0)
		SGV_STYLE_Y_AXIS = 0x40,    // plot Y axis (X=0)
		SGV_STYLE_INTERPOLATE = 0x80,   // interpolate the graph
		SGV_STYLE_UPDATE_ALL = 0x80000000   // when scrolling, update all view instead of ScrollWindow
	};
protected:
	// overrides:
	virtual double Evaluate(double arg, int nIndex);
	virtual double Evaluate(double arg, int nIndex, int nGraph);
	virtual double WrapValue(double arg, double dMin, double dMax);
	virtual int PrepareData(double dXBegin, double dXEnd, int nClientWidth);
	virtual void UnprepareData();
	virtual double GetLeftLimit();
	virtual double GetRightLimit();
	virtual int GetNumberOfGraphs();
	virtual COLORREF GetGraphColor(int nGraphNum);
protected:
	DWORD m_dwGraphStyle; // bits are SGV_STYLE_ enums
	double m_dXTicks;   // tick interval on X axis
	double m_dYTicks;   // tick interval on Y axis
	int m_MinXTickSpacing;  // min tick spacing in pixels
	int m_MinYTickSpacing;
	int m_MaxXTickSpacing;  // max tick spacing in pixels
	int m_MaxYTickSpacing;
	COLORREF m_Color;   // graph color
	// Generated message map functions
protected:
	//{{AFX_MSG(CScaledGraphView)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif  //#ifndef __SCALED_GRAPH_VIEW_H__
