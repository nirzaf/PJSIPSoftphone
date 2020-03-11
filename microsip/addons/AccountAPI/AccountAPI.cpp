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

#include "afxinet.h"
#include "global.h"
#include "utf.h"
#include "json.h"
#include "Markup.h"

#include <ws2tcpip.h>
#include <afxsock.h>

static CString accountAPIClientId;
static CStringList accountAPIClientIPs;

bool AccountAPIQuery(Account *account, CString URL, CStringA query, CString authUsername, CString authPassword)
{
	bool res = false;

	DWORD dwServiceType;
	CString strServer;
	CString strObject;
	INTERNET_PORT nPort;

	CString apiURL = msip_url_mask(_T(_GLOBAL_ACCOUNT_API));

	if (apiURL.IsEmpty()) {
		AfxMessageBox(_T("API URL is empty"), MB_ICONEXCLAMATION);
		return res;
	}

	AfxParseURL(apiURL, dwServiceType, strServer, strObject, nPort);

	CInternetSession session;
	CHttpConnection* pHttp = NULL;
	CHttpFile* pFile = NULL;

	try {
		pHttp = session.GetHttpConnection(strServer, (dwServiceType==AFX_INET_SERVICE_HTTPS ? INTERNET_FLAG_SECURE : 0), nPort);
		int nVerb = CHttpConnection::HTTP_VERB_GET;
		pFile = pHttp->OpenRequest(nVerb, strObject, 0, 1, 0, 0,
			INTERNET_FLAG_DONT_CACHE
			| (dwServiceType == AFX_INET_SERVICE_HTTPS ? INTERNET_FLAG_SECURE : 0)
			| (!authUsername.IsEmpty() ? INTERNET_FLAG_KEEP_CONNECTION : 0)
);
		if (dwServiceType == AFX_INET_SERVICE_HTTPS) {
			pFile->SetOption(INTERNET_OPTION_SECURITY_FLAGS,
				SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
				SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |
				SECURITY_FLAG_IGNORE_UNKNOWN_CA |
				SECURITY_FLAG_IGNORE_WRONG_USAGE
			);
		}
		pFile->SetOption(INTERNET_OPTION_CONNECT_TIMEOUT, 5000);

		if (!authUsername.IsEmpty()) {
			CString auth;
			pFile->SetOption(INTERNET_OPTION_USERNAME, authUsername.GetBuffer(), authUsername.GetLength());
			pFile->SetOption(INTERNET_OPTION_PASSWORD, authPassword.GetBuffer(), authPassword.GetLength());
		}

		pFile->SendRequest();
		account->username.Empty();
		if (query=="logout") {
			res = true;
		} else {
			//--
			CStringA body;
			CStringA buf;
			char pBuf[1024];
			int i;
			do {
				i = pFile->Read(pBuf,sizeof(pBuf)-1);
				pBuf[i] = 0;
				body.Append(pBuf);
			} while (i>0);
			DWORD dwStatusCode;
			pFile->QueryInfoStatusCode(dwStatusCode);

			if (dwStatusCode == 200 || dwStatusCode == 206 || dwStatusCode == 404) {
				//--
				if (!body.IsEmpty()) {
					CMarkup xml;
					BOOL bResult = xml.SetDoc(Utf8DecodeUni(body));
					if (bResult) {
						if (xml.FindChildElem(_T("username"))) {
							account->username = xml.GetChildData();
							xml.ResetPos();
						}
						if (xml.FindChildElem(_T("password"))) {
							account->password = xml.GetChildData();
							xml.ResetPos();
						}
						if (xml.FindChildElem(_T("userDisplayName"))) {
							account->displayName = xml.GetChildData();
							xml.ResetPos();
						}
						if (xml.FindChildElem(_T("host"))) {
							account->domain = xml.GetChildData();
							account->server = account->domain;
							account->proxy = account->domain;
							xml.ResetPos();
						}
						if (xml.FindChildElem(_T("subscribeForVoicemail"))) {
							if (xml.GetChildData() == _T("1")) {
								xml.ResetPos();
								if (xml.FindChildElem(_T("voiceMailNumber"))) {
									account->voicemailNumber = xml.GetChildData();
									xml.ResetPos();
								}
							}
							else {
								xml.ResetPos();
							}
						}
						if (xml.FindChildElem(_T("extProvInterval"))) {
							account->apiUpdateInterval = _wtoi(xml.GetChildData());
							xml.ResetPos();
						}
					}
					else {
						CString str;
						str = Utf8DecodeUni(body);
						AfxMessageBox(str, MB_ICONEXCLAMATION);
					}
				}
				else {
					AfxMessageBox(_T("API response is empty"), MB_ICONEXCLAMATION);
				}
			}
			else {
				CString str;
				str.Format(_T("API request failed. Response code: %d"), dwStatusCode);
				AfxMessageBox(str, MB_ICONEXCLAMATION);
			}
		}
	} catch (CInternetException *e) {
		AfxMessageBox(_T("API connection failed"), MB_ICONEXCLAMATION);
		e->Delete();
	}
	if (pFile!=NULL) {
		delete pFile;
	}
	if (pHttp!=NULL) {
		delete pHttp;
	}
	return res;
}
