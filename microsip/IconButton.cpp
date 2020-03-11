// CIconButton.cpp : implementation file   
//   
#include "StdAfx.h"   
#include "IconButton.h"   

// CIconButton   
IMPLEMENT_DYNAMIC(CIconButton, CButton)
CIconButton::CIconButton()   
{   
}
   
CIconButton::~CIconButton()   
{   
}

BEGIN_MESSAGE_MAP(CIconButton, CButton)   
    //{{AFX_MSG_MAP(CIconButton)   
	ON_WM_MOUSEMOVE()
    //}}AFX_MSG_MAP   
END_MESSAGE_MAP()

BUTTONICONINFO CIconButton::GetButtonIconInfo(HICON hIcon)
{
	BUTTONICONINFO _info;
	ZeroMemory(&_info, sizeof(_info));

	ICONINFO info;
	ZeroMemory(&info, sizeof(info));

	BOOL bRes = FALSE;

	bRes = GetIconInfo(hIcon, &info);
	if (!bRes)
		return _info;

	BITMAP bmp;
	ZeroMemory(&bmp, sizeof(bmp));

	if (info.hbmColor)
	{
		const int nWrittenBytes = GetObject(info.hbmColor, sizeof(bmp), &bmp);
		if (nWrittenBytes > 0)
		{
			_info.nWidth = bmp.bmWidth;
			_info.nHeight = bmp.bmHeight;
			_info.nBitsPerPixel = bmp.bmBitsPixel;
		}
	}
	else if (info.hbmMask)
	{
		// Icon has no color plane, image data stored in mask
		const int nWrittenBytes = GetObject(info.hbmMask, sizeof(bmp), &bmp);
		if (nWrittenBytes > 0)
		{
			_info.nWidth = bmp.bmWidth;
			_info.nHeight = bmp.bmHeight / 2;
			_info.nBitsPerPixel = 1;
		}
	}

	if (info.hbmColor)
		DeleteObject(info.hbmColor);
	if (info.hbmMask)
		DeleteObject(info.hbmMask);

	return _info;
}

HICON CIconButton::SetIcon(HICON icon)
{
	buttonIconInfo = GetButtonIconInfo(icon);
	return CButton::SetIcon(icon);
}

void CIconButton::PreSubclassWindow()
{
	DWORD dwStyle = ::GetClassLong(m_hWnd, GCL_STYLE);
	dwStyle &= ~CS_DBLCLKS;
	::SetClassLong(m_hWnd, GCL_STYLE, dwStyle);
}

void CIconButton::OnMouseMove(UINT nFlags, CPoint point)
{
	CRect rect;
	GetClientRect(&rect);
	if (rect.PtInRect(point)) {
		if (GetCapture()!=this) {
			SetCapture();
			Invalidate();
		}
	}
	else {
		ReleaseCapture();
		Invalidate();
	}
	CButton::OnMouseMove(nFlags, point);
}

void CIconButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)   
{   
	CDC dc;
	dc.Attach(lpDrawItemStruct->hDC);		//Get device context object
	CRect rect = lpDrawItemStruct->rcItem;
	UINT state = lpDrawItemStruct->itemState;

	dc.FillSolidRect(rect, dc.GetBkColor());
	dc.SetBkMode(TRANSPARENT);

	UINT uStyleTheme = RBS_NORMAL;
	if ((state & ODS_SELECTED)) {
		uStyleTheme = PBS_PRESSED;
	}
	else if (GetCapture() == this) {
		uStyleTheme = PBS_HOT;
	}
	if (uStyleTheme != RBS_NORMAL) {
		COLORREF color;
		BYTE corr;
		if (uStyleTheme == PBS_HOT) {
			corr = 20;
		}
		else {
			corr = 30;
		}
		color = dc.GetBkColor();
		*((BYTE*)(&color) + 0) -= corr;
		*((BYTE*)(&color) + 1) -= corr;
		*((BYTE*)(&color) + 2) -= corr;
		dc.FillSolidRect(rect, color);
	}
	
	DrawIconEx(dc.m_hDC, (rect.Width() - buttonIconInfo.nWidth) / 2, (rect.Height() - buttonIconInfo.nHeight) / 2, GetIcon(), buttonIconInfo.nWidth, buttonIconInfo.nHeight, NULL, NULL, DI_NORMAL);
	
	if ((state & ODS_FOCUS))       // If the button is focused
	{
		dc.DrawFocusRect(rect);
	}
	dc.Detach();
}

