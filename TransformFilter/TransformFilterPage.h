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
class TransformFilterPage:public CBasePropertyPage
{
public:
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	TransformFilterPage(LPUNKNOWN lpunk, HRESULT *phr);
private:
	BOOL OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	HRESULT OnConnect(IUnknown *pUnknown);
	HRESULT OnActivate(void);
	HRESULT OnApplyChanges();
	void SetDirty();

	void init_ui(HWND hwnd);
	void init_interpolationALG_list();
	void init_layout_list();
	void init_frame_list();
	void init_edit_text();
	void low_pass_filter();
	void get_setting();

private :
	TransformFilterInterface *m_pProgram;
	int output_w, output_h, inputlayout, outputlayout, frameformat, cubelength, cubemax, interpolationALG;
	BOOL lowpass_enable, lowpass_MT;
	int vs, hs;
	//UI
	HWND    m_hwndDialog;
	HWND	m_hwndOutputWidth, m_hwndOutputHeight;
	HWND	m_hwndInputLayout, m_hwndOutputLayout;
	HWND	m_hwndOutputFrameFormat;
	HWND	m_hwndCubeEdge, m_hwndCubeEdgeMax;
	HWND	m_hwndInterpolationALG;
	HWND	m_hwndLowPassEnable, m_hwndLowPassMT;
	HWND	m_hwndVS, m_hwndHS;
};