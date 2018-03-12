#include "stdafx.h"
#include "TransformFilterAdvancePage.h"
#include "transformfilterguid.h"
CUnknown * WINAPI TransformFilterAdvancePage::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	TransformFilterAdvancePage *pNewObj = new TransformFilterAdvancePage(lpunk, phr);
	//doesn't need to check whether new failed -- the caller handles it
	return pNewObj;
}
TransformFilterAdvancePage::TransformFilterAdvancePage(LPUNKNOWN pUnk, HRESULT *phr)
	: CBasePropertyPage(NAME("Transform Filter Advance Pages"), pUnk, IDD_DIALOG, IDS_TITLE)
	, m_pProgram(NULL)
{

}
HRESULT TransformFilterAdvancePage::OnConnect(IUnknown *pUnknown)
{
	ASSERT(m_pProgram == NULL);
	CheckPointer(pUnknown, E_POINTER);

	HRESULT hr = pUnknown->QueryInterface(IID_TransformFilterInterface, (void **)&m_pProgram);
	if (FAILED(hr))
	{
		return E_NOINTERFACE;
	}
	ASSERT(m_pProgram);
	return NOERROR;
}
HRESULT TransformFilterAdvancePage::OnActivate(void)
{
	return S_OK;
}
BOOL TransformFilterAdvancePage::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lPara)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		m_hwndDialog = hwnd;
		m_hwndOutputWidth = GetDlgItem(hwnd, IDC_EDITWidth);
		m_hwndOutputHeight = GetDlgItem(hwnd, IDC_EDITHeight);
		m_hwndInputStereoFormat = (CComboBox*)GetDlgItem(hwnd, IDC_COMBOSTEREO);
		m_hwndInputLayout = (CComboBox*)GetDlgItem(hwnd, IDC_COMBOINPUTLAYOUT);
		m_hwndInputLayout = (CComboBox*)GetDlgItem(hwnd, IDC_COMBOOUTPUTLAYOUT);
		m_hwndOutputFrameFormat = (CComboBox*)GetDlgItem(hwnd, IDC_COMBOFRAMEOUTPUT);
		m_hwndCubeEdge = GetDlgItem(hwnd, IDC_EDITLength);
		m_hwndCubeEdgeMax = GetDlgItem(hwnd, IDC_EDITMax);
		return TRUE;
	}
	default:
		return FALSE;
	}
	return FALSE;
}
void TransformFilterAdvancePage::init_ui()
{

}
void TransformFilterAdvancePage::init_stereo_list()
{
	m_hwndInputStereoFormat->AddString(L"STEREO_FORMAT_MONO");
	m_hwndInputStereoFormat->AddString(L"STEREO_FORMAT_TB");
	m_hwndInputStereoFormat->AddString(L"STEREO_FORMAT_LR");
	m_hwndInputStereoFormat->AddString(L"STEREO_FORMAT_GUESS");
	m_hwndInputStereoFormat->AddString(L"STEREO_FORMAT_N");
}
void TransformFilterAdvancePage::init_layout_list()
{

}
void TransformFilterAdvancePage::init_sub_list()
{
}
void TransformFilterAdvancePage::init_edit_text()
{
	pHeight = (CEdit*)GetDlgItem(m_hwndDialog, IDC_EDITHeight);
	pWidth = (CEdit*)GetDlgItem(m_hwndDialog, IDC_EDITHeight);

}