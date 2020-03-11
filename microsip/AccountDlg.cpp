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
#include "AccountDlg.h"
#include "mainDlg.h"
#include "langpack.h"
#include "atlrx.h"

#include <ws2tcpip.h>

AccountDlg::AccountDlg(CWnd* pParent /*=NULL*/)
: CDialog(AccountDlg::IDD, pParent)
{
	accountId = 0;
	Create (IDD, pParent);

}

AccountDlg::~AccountDlg(void)
{
}

int AccountDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (langPack.rtl) {
		ModifyStyleEx(0,WS_EX_LAYOUTRTL);
	}
	return 0;
}

BOOL AccountDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	TranslateDialog(this->m_hWnd);

	CString str;

	str.Format(_T("<a>%s</a>"),Translate(_T("display password")));
	GetDlgItem(IDC_SYSLINK_DISPLAY_PASSWORD)->SetWindowText(str);

	CComboBox *combobox;

	return TRUE;
}

void AccountDlg::OnDestroy()
{
	mainDlg->accountDlg = NULL;
	CDialog::OnDestroy();
}

void AccountDlg::PostNcDestroy()
{
	CDialog::PostNcDestroy();
	delete this;
}

BEGIN_MESSAGE_MAP(AccountDlg, CDialog)
	ON_WM_CREATE()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDCANCEL, &AccountDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, &AccountDlg::OnBnClickedOk)
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK_DISPLAY_PASSWORD, &AccountDlg::OnNMClickSyslinkDisplayPasswod)
	ON_NOTIFY(NM_RETURN, IDC_SYSLINK_DISPLAY_PASSWORD, &AccountDlg::OnNMClickSyslinkDisplayPasswod)
	ON_BN_CLICKED(IDC_ACCOUNT_REMOVE, &AccountDlg::OnBnClickedDelete)

	
END_MESSAGE_MAP()


void AccountDlg::OnClose() 
{
	DestroyWindow();
}

void AccountDlg::OnBnClickedCancel()
{
	OnClose();
}

void AccountDlg::Load(int id)
{
	CEdit* edit;
	CComboBox *combobox;
	accountId = id;
	if (accountSettings.AccountLoad(id,&m_Account)) {
		accountId = id;
		if (accountSettings.accountId == id && !accountSettings.account.rememberPassword) {
			m_Account.apiLogin = accountSettings.account.apiLogin;
			m_Account.apiPassword = accountSettings.account.apiPassword;
			m_Account.rememberPassword = false;
		}
	} else {
		accountId = 0;
	}


	edit = (CEdit*)GetDlgItem(IDC_EDIT_USERNAME);
	edit->SetWindowText(m_Account.apiLogin);

	edit = (CEdit*)GetDlgItem(IDC_EDIT_PASSWORD);
	edit->SetWindowText(m_Account.apiPassword);

	((CButton*)GetDlgItem(IDC_REMEMBER_PASSWORD))->SetCheck(m_Account.rememberPassword);

int i;

	if (accountId>0 && (!m_Account.apiLogin.IsEmpty() || accountId>1)) {
		GetDlgItem(IDC_ACCOUNT_REMOVE)->ShowWindow(SW_SHOW);
	} else {
		GetDlgItem(IDC_ACCOUNT_REMOVE)->ShowWindow(SW_HIDE);
	}
}

void AccountDlg::OnBnClickedOk()
{
	CEdit* edit;
	CString str;
	CComboBox *combobox;
	int i;

	edit = (CEdit*)GetDlgItem(IDC_EDIT_USERNAME);
	edit->GetWindowText(str);
	m_Account.apiLogin=str.Trim();
	m_Account.username.Empty();

	edit = (CEdit*)GetDlgItem(IDC_EDIT_PASSWORD);
	edit->GetWindowText(str);
	m_Account.apiPassword=str.Trim();

	m_Account.rememberPassword = ((CButton*)GetDlgItem(IDC_REMEMBER_PASSWORD))->GetCheck();

	if (m_Account.apiLogin.IsEmpty()) {
		CString str;
		str.Append(Translate(_T("Please fill out all required fields.")));
		AfxMessageBox(str);
		return;
	}


	this->ShowWindow(SW_HIDE);
	mainDlg->accountDlg = NULL;

	if (!accountId) {
		Account dummy;
		int i = 1;
		while (true) {
			if (!accountSettings.AccountLoad(i,&dummy)) {
				break;
			}
			i++;
		}
		accountId = i;
	}

	accountSettings.AccountSave(accountId,&m_Account);

	mainDlg->PJAccountDelete(true);

	accountSettings.accountId = accountId;
	accountSettings.account = m_Account;
	accountSettings.AccountLoad(accountSettings.accountId,&accountSettings.account);
	if (!m_Account.rememberPassword) {
		accountSettings.account.apiLogin = m_Account.apiLogin;
		accountSettings.account.apiPassword = m_Account.apiPassword;
		accountSettings.account.rememberPassword = false;
	}
	accountSettings.SettingsSave();
	mainDlg->pageDialer->RebuildButtons();
	mainDlg->PJAccountAdd();
	OnClose();
}

void AccountDlg::OnNMClickSyslinkDisplayPasswod(NMHDR *pNMHDR, LRESULT *pResult)
{
	GetDlgItem(IDC_SYSLINK_DISPLAY_PASSWORD)->ShowWindow(SW_HIDE);
	CEdit* edit = (CEdit*)GetDlgItem(IDC_EDIT_PASSWORD);
	edit->SetPasswordChar(0);
	edit->Invalidate();
	edit->SetFocus();
	int nLength = edit->GetWindowTextLength();
	edit->SetSel(nLength,nLength);
	*pResult = 0;
}

void AccountDlg::OnBnClickedDelete()
{
	if (accountId>0 && AfxMessageBox(Translate(_T("Are you sure you want to remove?")), MB_YESNO)==IDYES) {
		this->ShowWindow(SW_HIDE);
		mainDlg->accountDlg = NULL;

		Account account;
		int i = accountId;
		while (true) {
			if (!accountSettings.AccountLoad(i+1,&account)) {
				break;
			}
			accountSettings.AccountSave(i,&account);
			if (accountSettings.accountId == i+1) {
				accountSettings.accountId = i;
				accountSettings.SettingsSave();
				accountId = 0;
			}
			i++;
		}
		accountSettings.AccountDelete(i);
		if (accountId && accountId == accountSettings.accountId) {
			mainDlg->PJAccountDelete(true);
			if (i>1) {
				accountSettings.accountId = 1;
				accountSettings.AccountLoad(accountSettings.accountId,&accountSettings.account);
				mainDlg->PJAccountAdd();
			} else {
				accountSettings.accountId = 0;
			}
			accountSettings.SettingsSave();
		}
		OnClose();
	}
}

