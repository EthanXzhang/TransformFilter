#include "stdafx.h"
#include "TransformFilterPage.h"
#include "transformfilterguid.h"
CUnknown * WINAPI TransformFilterPage::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	TransformFilterPage *pNewObj = new TransformFilterPage(lpunk, phr);
	//doesn't need to check whether new failed -- the caller handles it
	return pNewObj;
}
TransformFilterPage::TransformFilterPage(LPUNKNOWN pUnk, HRESULT *phr)
	: CBasePropertyPage(NAME("Transform Filter Pages"), pUnk, IDD_DIALOG, IDS_TITLE)
	,m_pProgram(NULL)
{

}
HRESULT TransformFilterPage::OnConnect(IUnknown *pUnknown)
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
HRESULT TransformFilterPage::OnActivate(void)
{
	return S_OK;
}
void TransformFilterPage::SetDirty()
{
	m_bDirty = TRUE;
	if (m_pPageSite)
	{
		m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	}
}
BOOL TransformFilterPage::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lPara)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			m_pProgram->GetSetting(&output_w, &output_h, &inputlayout, &outputlayout, &frameformat, &cubelength, &cubemax, &interpolationALG, &lowpass_enable, &lowpass_MT, &vs, &hs);
			if (interpolationALG == 4)
			{
				interpolationALG--;
			}
			init_ui(hwnd);
			init_interpolationALG_list();
			init_layout_list();
			init_frame_list();
			init_edit_text();
			low_pass_filter();
			return TRUE;
		}
		case WM_DESTROY:
		{
			DestroyWindow(m_hwndDialog);
			return TRUE;
		}
		case WM_COMMAND:
		{
			SetDirty();
			return TRUE;
		}
		default:
			return FALSE;
	}
	return FALSE;
}
HRESULT TransformFilterPage::OnApplyChanges()
{
	if (!m_bDirty)
	{
		return (NOERROR);
	}
	get_setting();
	m_pProgram->DoSetting(output_w,output_h,inputlayout,outputlayout,frameformat,cubelength,cubemax,interpolationALG,lowpass_enable,lowpass_MT,vs,hs);
	m_bDirty = FALSE;
	return(NOERROR);
}
void TransformFilterPage::init_ui(HWND hwnd)
{
	m_hwndDialog = hwnd;
	m_hwndOutputWidth = GetDlgItem(hwnd, IDC_EDITWidth);
	ASSERT(m_hwndOutputWidth);
	m_hwndOutputHeight = GetDlgItem(hwnd, IDC_EDITHeight);
	ASSERT(m_hwndOutputHeight);
	m_hwndInputLayout = GetDlgItem(hwnd, IDC_COMBOINPUTLAYOUT);
	ASSERT(m_hwndInputLayout);
	m_hwndOutputLayout = GetDlgItem(hwnd, IDC_COMBOOUTPUTLAYOUT);
	ASSERT(m_hwndOutputLayout);
	m_hwndOutputFrameFormat = GetDlgItem(hwnd, IDC_COMBOFRAMEOUTPUT);
	ASSERT(m_hwndOutputFrameFormat);
	m_hwndCubeEdge = GetDlgItem(hwnd, IDC_EDITLength);
	ASSERT(m_hwndCubeEdge);
	m_hwndCubeEdgeMax = GetDlgItem(hwnd, IDC_EDITMax);
	ASSERT(m_hwndCubeEdgeMax);
	m_hwndInterpolationALG = GetDlgItem(hwnd, IDC_COMBOALG);
	ASSERT(m_hwndInterpolationALG);
	m_hwndLowPassEnable = GetDlgItem(hwnd, IDC_CHECKLOWPASS);
	ASSERT(m_hwndLowPassEnable);
	m_hwndLowPassMT = GetDlgItem(hwnd, IDC_CHECKMT);
	ASSERT(m_hwndLowPassMT);
	m_hwndVS = GetDlgItem(hwnd, IDC_EDITVS);
	ASSERT(m_hwndVS);
	m_hwndHS = GetDlgItem(hwnd, IDC_EDITHS);
	ASSERT(m_hwndHS);
}
void TransformFilterPage::init_interpolationALG_list()
{
	CComboBox *pComboBox;
	pComboBox = (CComboBox*)CWnd::FromHandle(m_hwndInterpolationALG);
	pComboBox->AddString(L"NEAREST");
	pComboBox->AddString(L"LINEAR");
	pComboBox->AddString(L"CUBIC");
	pComboBox->AddString(L"LANCZOS4");
	pComboBox->SetCurSel(interpolationALG);
}
void TransformFilterPage::init_layout_list()
{
	CComboBox *pComboBoxInput,*pComboBoxOutput;
	pComboBoxInput = (CComboBox*)CWnd::FromHandle(m_hwndInputLayout);
	pComboBoxOutput = (CComboBox*)CWnd::FromHandle(m_hwndOutputLayout);
	pComboBoxInput->AddString(L"CUBEMAP 32");
	pComboBoxInput->AddString(L"CUBEMAP 23 OFFCENTER");
	pComboBoxInput->AddString(L"EQUIRECT");
	pComboBoxInput->AddString(L"BARREL");
	pComboBoxInput->AddString(L"EQUI AREA CUBEMAP 32");
	pComboBoxInput->SetCurSel(inputlayout);
	pComboBoxOutput->AddString(L"CUBEMAP 32");
	pComboBoxOutput->AddString(L"CUBEMAP 23 OFFCENTER");
	pComboBoxOutput->AddString(L"EQUIRECT");
	pComboBoxOutput->AddString(L"BARREL");
	pComboBoxOutput->AddString(L"EQUI AREA CUBEMAP 32");
	pComboBoxOutput->SetCurSel(outputlayout);
}
void TransformFilterPage::init_frame_list()
{
	CComboBox *pComboBox;
	pComboBox= (CComboBox*)CWnd::FromHandle(m_hwndOutputFrameFormat);
	pComboBox->AddString(L"IYUV(YUV420P)");
	pComboBox->AddString(L"YV12");
	pComboBox->AddString(L"NV12");
	pComboBox->AddString(L"RGB24");
	pComboBox->AddString(L"RGB32");
	pComboBox->SetCurSel(frameformat);
}
void TransformFilterPage::init_edit_text()
{
	CEdit *pEditW, *pEditH, *pEditCE, *pEditCEMax, *pEditHS, *pEditVS;
	CString str;
	pEditW = (CEdit*)CWnd::FromHandle(m_hwndOutputWidth);
	if (output_w == 0)
	{
		pEditW->SetWindowTextW(L"Auto");
	}
	else
	{
		str.Format(_T("%d"),output_w);
		pEditW->SetWindowTextW(str);
	}
	pEditH = (CEdit*)CWnd::FromHandle(m_hwndOutputHeight);
	if (output_h == 0)
	{
		pEditH->SetWindowTextW(L"Auto");
	}
	else
	{
		str.Format(_T("%d"), output_h);
		pEditH->SetWindowTextW(str);
	}
	pEditCE = (CEdit*)CWnd::FromHandle(m_hwndCubeEdge);
	if (cubelength == 0)
	{
		pEditCE->SetWindowTextW(L"Auto");
	}
	else
	{
		str.Format(_T("%d"), cubelength);
		pEditCE->SetWindowTextW(str);
	}
	pEditCEMax = (CEdit*)CWnd::FromHandle(m_hwndCubeEdgeMax);
	str.Format(_T("%d"), cubemax);
	pEditCEMax->SetWindowTextW(str);
	pEditHS = (CEdit*)CWnd::FromHandle(m_hwndHS);
	str.Format(_T("%d"), hs);
	pEditHS->SetWindowTextW(str);
	pEditVS = (CEdit*)CWnd::FromHandle(m_hwndVS);
	str.Format(_T("%d"), vs);
	pEditVS->SetWindowTextW(str);
}
void TransformFilterPage::low_pass_filter()
{
	CButton *pButtonEnable, *pButtonMT;
	pButtonEnable=(CButton*)CWnd::FromHandle(m_hwndLowPassEnable);
	pButtonMT = (CButton*)CWnd::FromHandle(m_hwndLowPassMT);
	pButtonEnable->SetCheck(lowpass_enable);
	pButtonMT->SetCheck(lowpass_MT);
}
void TransformFilterPage::get_setting()
{
	CComboBox *pComboBox;
	pComboBox = (CComboBox*)CWnd::FromHandle(m_hwndInterpolationALG);
	interpolationALG=pComboBox->GetCurSel();
	if (interpolationALG == 3)
	{
		interpolationALG++;
	}
	pComboBox= (CComboBox*)CWnd::FromHandle(m_hwndInputLayout);
	inputlayout= pComboBox->GetCurSel();
	pComboBox = (CComboBox*)CWnd::FromHandle(m_hwndOutputLayout);
	outputlayout = pComboBox->GetCurSel();
	pComboBox = (CComboBox*)CWnd::FromHandle(m_hwndOutputFrameFormat);
	frameformat= pComboBox->GetCurSel();

	CButton *pButton;
	pButton= (CButton*)CWnd::FromHandle(m_hwndLowPassEnable);
	lowpass_enable = pButton->GetCheck();
	pButton = (CButton*)CWnd::FromHandle(m_hwndLowPassMT);
	lowpass_MT= pButton->GetCheck();

	CEdit *pEdit;
	CString str,destr="Auto";
	pEdit = (CEdit*)CWnd::FromHandle(m_hwndOutputWidth);
	pEdit->GetWindowTextW(str);
	if (str.CompareNoCase(destr))
	{
		output_w = _ttoi(str);
	}	
	pEdit = (CEdit*)CWnd::FromHandle(m_hwndOutputHeight);
	pEdit->GetWindowTextW(str);
	if (str.CompareNoCase(destr))
	{
		output_h = _ttoi(str);
	}
	pEdit = (CEdit*)CWnd::FromHandle(m_hwndCubeEdge);
	pEdit->GetWindowTextW(str);
	if (str.CompareNoCase(destr))
	{
		cubelength = _ttoi(str);
	}
	pEdit = (CEdit*)CWnd::FromHandle(m_hwndCubeEdgeMax);
	pEdit->GetWindowTextW(str);
	cubemax = _ttoi(str);
	pEdit = (CEdit*)CWnd::FromHandle(m_hwndHS);
	pEdit->GetWindowTextW(str);
	hs = _ttoi(str);
	pEdit = (CEdit*)CWnd::FromHandle(m_hwndVS);
	pEdit->GetWindowTextW(str);
	vs = _ttoi(str);
}