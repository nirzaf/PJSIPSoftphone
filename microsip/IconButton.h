#pragma once 

#include "const.h"

struct BUTTONICONINFO
{
	int     nWidth;
	int     nHeight;
	int     nBitsPerPixel;
};

class CIconButton : public CButton 
{ 
	DECLARE_DYNAMIC(CIconButton) 
public: 
	CIconButton();
	virtual ~CIconButton();
private:
	BUTTONICONINFO buttonIconInfo;
	BUTTONICONINFO GetButtonIconInfo(HICON hIcon);
protected: 
	DECLARE_MESSAGE_MAP()
	virtual void PreSubclassWindow();
	afx_msg void OnMouseMove(UINT, CPoint);
public: 
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct); 
	HICON SetIcon(HICON icon);
};
