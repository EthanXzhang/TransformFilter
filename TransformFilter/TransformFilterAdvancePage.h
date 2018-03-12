#pragma once
#include "stdafx.h"
#include <atlbase.h>
#include <atlconv.h>
#include <streams.h>
#include <olectl.h>
#include <stdlib.h>
#include <stdio.h>
#include "interface.h"
#include "resource.h"
class TransformFilterAdvancePage :public CBasePropertyPage
{
public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	TransformFilterAdvancePage(LPUNKNOWN lpunk, HRESULT *phr);
private:
	BOOL OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HRESULT OnConnect(IUnknown *pUnknown);
	HRESULT OnActivate(void);
	//HRESULT OnDisconnect();
	//HRESULT OnDeactivate();
	//HRESULT OnApplyChanges();

	void init_ui();
	void init_stereo_list();
	void init_layout_list();
	void init_sub_list();
	void init_edit_text();


private:
	TransformFilterInterface *m_pProgram;
	CComboBox *pComboStereo, *pComboLayout, *pSub_w, *pSub_h;
	CEdit *pHeight, *pWidth, *pSize, *pCubeLength, *pCubeMax;
	//UI
	HWND    m_hwndDialog;
	HWND	m_hwndOutputWidth, m_hwndOutputHeight;
	CComboBox*	m_hwndInputStereoFormat;
	CComboBox*	m_hwndInputLayout, m_hwndOutputLayout;
	CComboBox*	m_hwndOutputFrameFormat;
	HWND	m_hwndCubeEdge, m_hwndCubeEdgeMax;
};