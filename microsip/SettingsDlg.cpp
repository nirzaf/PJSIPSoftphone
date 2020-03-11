/*
 * Copyright (C) 2011-2018 MicroSIP (http://www.microsip.org)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "StdAfx.h"
#include "SettingsDlg.h"
#include "mainDlg.h"
#include "settings.h"
#include "Preview.h"
#include "langpack.h"
#include <afxshellmanager.h>

SettingsDlg::SettingsDlg(CWnd* pParent /*=NULL*/)
	: CDialog(SettingsDlg::IDD, pParent)
{
	Create(IDD, pParent);
}

SettingsDlg::~SettingsDlg(void)
{
}

int SettingsDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (langPack.rtl) {
		ModifyStyleEx(0, WS_EX_LAYOUTRTL);
	}
	return 0;
}

BOOL SettingsDlg::OnInitDialog()
{
	CComboBox *combobox;
	CComboBox *combobox2;
	unsigned count;
	int i;
	CString str;

	CDialog::OnInitDialog();

	TranslateDialog(this->m_hWnd);

	GetDlgItem(IDC_SETTINGS_RINGING_SOUND)->SetWindowText(accountSettings.ringingSound);
	GetDlgItem(IDC_SETTINGS_RECORDING)->SetWindowText(accountSettings.recordingPath);
	((CButton*)GetDlgItem(IDC_SETTINGS_RECORDING_CHECKBOX))->SetCheck(accountSettings.autoRecording);
	combobox = (CComboBox*)GetDlgItem(IDC_SETTINGS_MICROPHONE);
	combobox->AddString(Translate(_T("Default")));
	combobox->SetCurSel(0);

	pjmedia_aud_dev_info aud_dev_info[PJMEDIA_AUD_MAX_DEVS];
	count = PJMEDIA_AUD_MAX_DEVS;
	pjsua_enum_aud_devs(aud_dev_info, &count);
	for (unsigned i = 0; i < count; i++)
	{
		if (aud_dev_info[i].input_count) {
			CString audDevName = AnsiToWideChar(aud_dev_info[i].name);
			combobox->AddString(audDevName);
			if (!accountSettings.audioInputDevice.Compare(audDevName))
			{
				combobox->SetCurSel(combobox->GetCount() - 1);
			}
		}
	}
	combobox = (CComboBox*)GetDlgItem(IDC_SETTINGS_SPEAKERS);
	combobox->AddString(Translate(_T("Default")));
	combobox->SetCurSel(0);
	combobox2 = (CComboBox*)GetDlgItem(IDC_SETTINGS_RING);
	combobox2->AddString(Translate(_T("Default")));
	combobox2->SetCurSel(0);
	for (unsigned i = 0; i < count; i++)
	{
		if (aud_dev_info[i].output_count) {
			CString audDevName = AnsiToWideChar(aud_dev_info[i].name);
			combobox->AddString(audDevName);
			combobox2->AddString(audDevName);
			if (!accountSettings.audioOutputDevice.Compare(audDevName))
			{
				combobox->SetCurSel(combobox->GetCount() - 1);
			}
			if (!accountSettings.audioRingDevice.Compare(audDevName))
			{
				combobox2->SetCurSel(combobox->GetCount() - 1);
			}
		}
	}

	((CButton*)GetDlgItem(IDC_SETTINGS_MIC_AMPLIF))->SetCheck(accountSettings.micAmplification);
	((CButton*)GetDlgItem(IDC_SETTINGS_SW_ADJUST))->SetCheck(accountSettings.swLevelAdjustment);

	pjsua_codec_info codec_info[PJMEDIA_CODEC_MGR_MAX_CODECS];

	((CButton*)GetDlgItem(IDC_SETTINGS_VAD))->SetCheck(accountSettings.vad);
	((CButton*)GetDlgItem(IDC_SETTINGS_EC))->SetCheck(accountSettings.ec);
	((CButton*)GetDlgItem(IDC_SETTINGS_FORCE_CODEC))->SetCheck(accountSettings.forceCodec);

#ifdef _GLOBAL_VIDEO
	combobox = (CComboBox*)GetDlgItem(IDC_SETTINGS_VID_CAP_DEV);
	combobox->AddString(Translate(_T("Default")));
	combobox->SetCurSel(0);

	count = PJMEDIA_VID_DEV_MAX_DEVS;
	pjmedia_vid_dev_info vid_dev_info[PJMEDIA_VID_DEV_MAX_DEVS];
	pjsua_vid_enum_devs(vid_dev_info, &count);
	for (unsigned i = 0; i < count; i++)
	{
		if (vid_dev_info[i].fmt_cnt && (vid_dev_info[i].dir == PJMEDIA_DIR_ENCODING || vid_dev_info[i].dir == PJMEDIA_DIR_ENCODING_DECODING))
		{
			CString vidDevName = AnsiToWideChar(vid_dev_info[i].name);
			combobox->AddString(vidDevName);
			if (!accountSettings.videoCaptureDevice.Compare(vidDevName))
			{
				combobox->SetCurSel(combobox->GetCount() - 1);
			}
		}
	}

	combobox = (CComboBox*)GetDlgItem(IDC_SETTINGS_VIDEO_CODEC);
	combobox->AddString(Translate(_T("Default")));
	combobox->SetCurSel(0);
	count = PJMEDIA_CODEC_MGR_MAX_CODECS;
	pjsua_vid_enum_codecs(codec_info, &count);
	for (unsigned i = 0; i < count; i++)
	{
		combobox->AddString(PjToStr(&codec_info[i].codec_id));
		if (!accountSettings.videoCodec.Compare(PjToStr(&codec_info[i].codec_id)))
		{
			combobox->SetCurSel(combobox->GetCount() - 1);
		}
	}

	((CButton*)GetDlgItem(IDC_SETTINGS_VIDEO_H264))->SetCheck(accountSettings.videoH264);
	((CButton*)GetDlgItem(IDC_SETTINGS_VIDEO_H263))->SetCheck(accountSettings.videoH263);
	((CButton*)GetDlgItem(IDC_SETTINGS_VIDEO_VP8))->SetCheck(accountSettings.videoVP8);
	if (!accountSettings.videoBitrate) {
		const pj_str_t codec_id = { "H264", 4 };
		pjmedia_vid_codec_param param;
		pjsua_vid_codec_get_param(&codec_id, &param);
		accountSettings.videoBitrate = param.enc_fmt.det.vid.max_bps / 1000;
	}
	str.Format(_T("%d"), accountSettings.videoBitrate);
	GetDlgItem(IDC_SETTINGS_VIDEO_BITRATE)->SetWindowText(str);

#endif

	((CButton*)GetDlgItem(IDC_SETTINGS_RPORT))->SetCheck(accountSettings.rport);
	str.Format(_T("%d"), accountSettings.sourcePort);
	GetDlgItem(IDC_SETTINGS_SOURCE_PORT)->SetWindowText(str);
	str.Format(_T("%d"), accountSettings.rtpPortMin);
	GetDlgItem(IDC_SETTINGS_RTP_PORT_MIN)->SetWindowText(str);
	str.Format(_T("%d"), accountSettings.rtpPortMax);
	GetDlgItem(IDC_SETTINGS_RTP_PORT_MAX)->SetWindowText(str);

	GetDlgItem(IDC_SETTINGS_DNS_SRV_NS)->SetWindowText(accountSettings.dnsSrvNs);
	((CButton*)GetDlgItem(IDC_SETTINGS_DNS_SRV_CHECKBOX))->SetCheck(accountSettings.dnsSrv);

	GetDlgItem(IDC_SETTINGS_STUN)->SetWindowText(accountSettings.stun);
	((CButton*)GetDlgItem(IDC_SETTINGS_STUN_CHECKBOX))->SetCheck(accountSettings.enableSTUN);

	combobox = (CComboBox*)GetDlgItem(IDC_SETTINGS_DTMF_METHOD);
	combobox->AddString(Translate(_T("Auto")));
	combobox->AddString(Translate(_T("In-band")));
	combobox->AddString(Translate(_T("RFC2833")));
	combobox->AddString(Translate(_T("SIP-INFO")));
	combobox->SetCurSel(accountSettings.DTMFMethod);

	combobox = (CComboBox*)GetDlgItem(IDC_SETTINGS_AUTO_ANSWER);
	combobox->AddString(Translate(_T("No")));
	autoAnswerValues.Add(_T(""));
	combobox->AddString(Translate(_T("Control Button")));
	autoAnswerValues.Add(_T("button"));
	combobox->AddString(Translate(_T("SIP Header")));
	autoAnswerValues.Add(_T("header"));
	combobox->AddString(Translate(_T("All Calls")));
	autoAnswerValues.Add(_T("all"));
	combobox->SetCurSel(0);
	for (i = 0; i < autoAnswerValues.GetCount(); i++) {
		if (accountSettings.autoAnswer == autoAnswerValues.GetAt(i)) {
			combobox->SetCurSel(i);
			break;
		}
	}

	combobox = (CComboBox*)GetDlgItem(IDC_SETTINGS_DENY_INCOMING);
	combobox->AddString(Translate(_T("No")));
	denyIncomingValues.Add(_T(""));
	combobox->AddString(Translate(_T("Control Button")));
	denyIncomingValues.Add(_T("button"));
	combobox->AddString(Translate(_T("All Calls")));
	denyIncomingValues.Add(_T("all"));
	combobox->SetCurSel(0);
	for (i = 0; i < denyIncomingValues.GetCount(); i++) {
		if (accountSettings.denyIncoming == denyIncomingValues.GetAt(i)) {
			combobox->SetCurSel(i);
			break;
		}
	}

	((CButton*)GetDlgItem(IDC_SETTINGS_MEDIA_BUTTONS))->SetCheck(accountSettings.enableMediaButtons);
	((CButton*)GetDlgItem(IDC_SETTINGS_LOCAL_DTMF))->SetCheck(accountSettings.localDTMF);
	((CButton*)GetDlgItem(IDC_SETTINGS_SINGLE_MODE))->SetCheck(accountSettings.singleMode);
	((CButton*)GetDlgItem(IDC_SETTINGS_ENABLE_LOG))->SetCheck(accountSettings.enableLog);
	((CButton*)GetDlgItem(IDC_SETTINGS_BRING_TO_FRONT))->SetCheck(accountSettings.bringToFrontOnIncoming);
	((CButton*)GetDlgItem(IDC_SETTINGS_ANSWER_BOX_RANDOM))->SetCheck(accountSettings.randomAnswerBox);

	return TRUE;
}

void SettingsDlg::OnDestroy()
{
	mainDlg->settingsDlg = NULL;
	CDialog::OnDestroy();
}

void SettingsDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

BEGIN_MESSAGE_MAP(SettingsDlg, CDialog)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDCANCEL, &SettingsDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, &SettingsDlg::OnBnClickedOk)
	ON_MESSAGE(UM_UPDATE_SETTINGS, &SettingsDlg::OnUpdateSettings)
#ifdef _GLOBAL_VIDEO
	ON_BN_CLICKED(IDC_SETTINGS_PREVIEW, &SettingsDlg::OnBnClickedPreview)
#endif
	ON_BN_CLICKED(IDC_SETTINGS_BROWSE, &SettingsDlg::OnBnClickedBrowse)
	ON_EN_CHANGE(IDC_SETTINGS_RINGING_SOUND, &SettingsDlg::OnEnChangeRingingSound)
	ON_BN_CLICKED(IDC_SETTINGS_DEFAULT, &SettingsDlg::OnBnClickedDefault)
	ON_BN_CLICKED(IDC_SETTINGS_RECORDING_BROWSE, &SettingsDlg::OnBnClickedRecordingBrowse)
	ON_EN_CHANGE(IDC_SETTINGS_RECORDING, &SettingsDlg::OnEnChangeRecording)
	ON_BN_CLICKED(IDC_SETTINGS_RECORDING_DEFAULT, &SettingsDlg::OnBnClickedRecordingDefault)
END_MESSAGE_MAP()


void SettingsDlg::OnClose()
{
	DestroyWindow();
}

void SettingsDlg::OnBnClickedCancel()
{
	OnClose();
}

void SettingsDlg::OnBnClickedOk()
{
	this->ShowWindow(SW_HIDE);
	mainDlg->PJDestroy();
	PostMessage(UM_UPDATE_SETTINGS, 0, 0);
}

LRESULT SettingsDlg::OnUpdateSettings(WPARAM wParam, LPARAM lParam)
{
	CString str; 

	CComboBox *combobox;
	int i;

	GetDlgItem(IDC_SETTINGS_MICROPHONE)->GetWindowText(accountSettings.audioInputDevice);
	if (accountSettings.audioInputDevice == Translate(_T("Default")))
	{
		accountSettings.audioInputDevice = _T("");
	}

	GetDlgItem(IDC_SETTINGS_SPEAKERS)->GetWindowText(accountSettings.audioOutputDevice);
	if (accountSettings.audioOutputDevice == Translate(_T("Default")))
	{
		accountSettings.audioOutputDevice = _T("");
	}

	GetDlgItem(IDC_SETTINGS_RING)->GetWindowText(accountSettings.audioRingDevice);
	if (accountSettings.audioRingDevice == Translate(_T("Default")))
	{
		accountSettings.audioRingDevice = _T("");
	}

	accountSettings.micAmplification = ((CButton*)GetDlgItem(IDC_SETTINGS_MIC_AMPLIF))->GetCheck();
	accountSettings.swLevelAdjustment = ((CButton*)GetDlgItem(IDC_SETTINGS_SW_ADJUST))->GetCheck();

	accountSettings.vad = ((CButton*)GetDlgItem(IDC_SETTINGS_VAD))->GetCheck();
	accountSettings.ec = ((CButton*)GetDlgItem(IDC_SETTINGS_EC))->GetCheck();
	accountSettings.forceCodec = ((CButton*)GetDlgItem(IDC_SETTINGS_FORCE_CODEC))->GetCheck();

#ifdef _GLOBAL_VIDEO
	GetDlgItem(IDC_SETTINGS_VID_CAP_DEV)->GetWindowText(accountSettings.videoCaptureDevice);
	if (accountSettings.videoCaptureDevice == Translate(_T("Default")))
	{
		accountSettings.videoCaptureDevice = _T("");
	}

	GetDlgItem(IDC_SETTINGS_VIDEO_CODEC)->GetWindowText(accountSettings.videoCodec);
	if (accountSettings.videoCodec == Translate(_T("Default")))
	{
		accountSettings.videoCodec = _T("");
	}
	accountSettings.videoH264 = ((CButton*)GetDlgItem(IDC_SETTINGS_VIDEO_H264))->GetCheck();
	accountSettings.videoH263 = ((CButton*)GetDlgItem(IDC_SETTINGS_VIDEO_H263))->GetCheck();
	accountSettings.videoVP8 = ((CButton*)GetDlgItem(IDC_SETTINGS_VIDEO_VP8))->GetCheck();
	GetDlgItem(IDC_SETTINGS_VIDEO_BITRATE)->GetWindowText(str);
	accountSettings.videoBitrate = _wtoi(str);
#endif

	accountSettings.rport = ((CButton*)GetDlgItem(IDC_SETTINGS_RPORT))->GetCheck();
	GetDlgItem(IDC_SETTINGS_SOURCE_PORT)->GetWindowText(str);
	accountSettings.sourcePort = _wtoi(str);
	GetDlgItem(IDC_SETTINGS_RTP_PORT_MIN)->GetWindowText(str);
	accountSettings.rtpPortMin = _wtoi(str);
	GetDlgItem(IDC_SETTINGS_RTP_PORT_MAX)->GetWindowText(str);
	accountSettings.rtpPortMax = _wtoi(str);

	GetDlgItem(IDC_SETTINGS_DNS_SRV_NS)->GetWindowText(accountSettings.dnsSrvNs);
	accountSettings.dnsSrvNs.Trim();
	accountSettings.dnsSrv = ((CButton*)GetDlgItem(IDC_SETTINGS_DNS_SRV_CHECKBOX))->GetCheck();

	GetDlgItem(IDC_SETTINGS_STUN)->GetWindowText(accountSettings.stun);
	accountSettings.stun.Trim();
	accountSettings.enableSTUN = ((CButton*)GetDlgItem(IDC_SETTINGS_STUN_CHECKBOX))->GetCheck();

	combobox = (CComboBox*)GetDlgItem(IDC_SETTINGS_DTMF_METHOD);
	accountSettings.DTMFMethod = combobox->GetCurSel();

	combobox = (CComboBox*)GetDlgItem(IDC_SETTINGS_AUTO_ANSWER);
	accountSettings.autoAnswer = autoAnswerValues.GetAt(combobox->GetCurSel());

	combobox = (CComboBox*)GetDlgItem(IDC_SETTINGS_DENY_INCOMING);
	accountSettings.denyIncoming = denyIncomingValues.GetAt(combobox->GetCurSel());

	accountSettings.enableMediaButtons = ((CButton*)GetDlgItem(IDC_SETTINGS_MEDIA_BUTTONS))->GetCheck();
	accountSettings.localDTMF = ((CButton*)GetDlgItem(IDC_SETTINGS_LOCAL_DTMF))->GetCheck();
	accountSettings.singleMode = ((CButton*)GetDlgItem(IDC_SETTINGS_SINGLE_MODE))->GetCheck();
	accountSettings.enableLog = ((CButton*)GetDlgItem(IDC_SETTINGS_ENABLE_LOG))->GetCheck();
	accountSettings.bringToFrontOnIncoming = ((CButton*)GetDlgItem(IDC_SETTINGS_BRING_TO_FRONT))->GetCheck();
	accountSettings.randomAnswerBox = ((CButton*)GetDlgItem(IDC_SETTINGS_ANSWER_BOX_RANDOM))->GetCheck();
	GetDlgItem(IDC_SETTINGS_RINGING_SOUND)->GetWindowText(accountSettings.ringingSound);
	GetDlgItem(IDC_SETTINGS_RECORDING)->GetWindowText(accountSettings.recordingPath);
	accountSettings.recordingPath.Trim();
	accountSettings.autoRecording = ((CButton*)GetDlgItem(IDC_SETTINGS_RECORDING_CHECKBOX))->GetCheck();

	accountSettings.SettingsSave();
	mainDlg->pageDialer->RebuildButtons();
	mainDlg->PJCreate();
	mainDlg->PJAccountAdd();

	OnClose();
	return 0;
}

void SettingsDlg::OnBnClickedBrowse()
{
	CFileDialog dlgFile(TRUE, _T("wav"), 0, OFN_NOCHANGEDIR, _T("WAV Files (*.wav)|*.wav|"));
	if (dlgFile.DoModal() == IDOK) {
		CString cwd;
		LPTSTR ptr = cwd.GetBuffer(MAX_PATH);
		::GetCurrentDirectory(MAX_PATH, ptr);
		cwd.ReleaseBuffer();
		if (cwd.MakeLower() + _T("\\") + dlgFile.GetFileName().MakeLower() == dlgFile.GetPathName().MakeLower()) {
			GetDlgItem(IDC_SETTINGS_RINGING_SOUND)->SetWindowText(dlgFile.GetFileName());
		}
		else {
			GetDlgItem(IDC_SETTINGS_RINGING_SOUND)->SetWindowText(dlgFile.GetPathName());
		}
	}
}

void SettingsDlg::OnEnChangeRingingSound()
{
	CString str;
	GetDlgItem(IDC_SETTINGS_RINGING_SOUND)->GetWindowText(str);
	GetDlgItem(IDC_SETTINGS_DEFAULT)->EnableWindow(str.GetLength() > 0);
}

void SettingsDlg::OnBnClickedDefault()
{
	GetDlgItem(IDC_SETTINGS_RINGING_SOUND)->SetWindowText(_T(""));
}

void SettingsDlg::OnBnClickedRecordingBrowse()
{
	CString strOutFolder;
	CShellManager* pShellManager = ((CWinAppEx*)AfxGetApp())->GetShellManager();
	GetDlgItem(IDC_SETTINGS_RECORDING)->GetWindowText(strOutFolder);
	if (pShellManager->BrowseForFolder(strOutFolder,this, strOutFolder))
	{
		GetDlgItem(IDC_SETTINGS_RECORDING)->SetWindowText(strOutFolder);
	}
}

void SettingsDlg::OnEnChangeRecording()
{
	CString str;
	GetDlgItem(IDC_SETTINGS_RECORDING)->GetWindowText(str);
	GetDlgItem(IDC_SETTINGS_RECORDING_DEFAULT)->EnableWindow(str.GetLength() > 0);
}

void SettingsDlg::OnBnClickedRecordingDefault()
{
	GetDlgItem(IDC_SETTINGS_RECORDING)->SetWindowText(_T(""));
}

#ifdef _GLOBAL_VIDEO
void SettingsDlg::OnBnClickedPreview()
{
	CComboBox *combobox;
	combobox = (CComboBox*)GetDlgItem(IDC_SETTINGS_VID_CAP_DEV);
	CString name;
	combobox->GetWindowText(name);
	if (!mainDlg->previewWin) {
		mainDlg->previewWin = new Preview(mainDlg);
	}
	mainDlg->previewWin->Start(mainDlg->VideoCaptureDeviceId(name));
}
#endif


