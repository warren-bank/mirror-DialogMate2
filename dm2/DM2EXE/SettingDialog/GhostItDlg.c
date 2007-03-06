/* http://dm2.sf.net */
#include <windows.h>
#include "SettingDlg.h"
#include "../MImage.h"
#include "../util.h"

extern HFONT hLocalFont;
extern SGHOST_DATA Ghost_data;
extern SGHOST_DATA Ghost_data_temp;
INT_PTR WINAPI SettingDlgGhostItProc(HWND hwndDlg, UINT uMsg, 
										  WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) 
	{
	case WM_INITDIALOG:
		SetLanguageFont(hLocalFont, hwndDlg);
		TranslateLanguage(lang_file, hwndDlg, "SettingsDialogGhost");
		{
			char szStr[4];
			wsprintf(szStr, "%d", Ghost_data_temp.awo);
			SetDlgItemText(hwndDlg, IDC_EDIT_AWO, szStr);
			wsprintf(szStr, "%d", Ghost_data_temp.iawo);
			SetDlgItemText(hwndDlg, IDC_EDIT_IAWO, szStr);
			SendDlgItemMessage(hwndDlg, IDC_EDIT_AWO,  EM_LIMITTEXT, 3, 0);
			SendDlgItemMessage(hwndDlg, IDC_EDIT_IAWO,  EM_LIMITTEXT, 3, 0);
		}
		return TRUE;
	
	case WM_DM2_SAVESETTING:
		{
			char szStr[4];
			GetDlgItemText(hwndDlg, IDC_EDIT_AWO, szStr, 4);
			Ghost_data_temp.awo = setIntRange(atoi(szStr), 0, 100);
			GetDlgItemText(hwndDlg, IDC_EDIT_IAWO, szStr, 4);
			Ghost_data_temp.iawo = setIntRange(atoi(szStr), 0, 100);
		}
		break;
	case WM_DESTROY:
		break;
	}

	return FALSE;
}

BOOL AddGhostIt(HWND hwnd, PGHOSTIT *git)
{
	PGHOSTIT pgit = *git;
	
	if(*git == NULL)
	{
		//PGHOSTIT is null
		*git = halloc(sizeof(GHOSTIT));
		pgit = *git;
		SetTimer(pshared->DM2wnd, GHOST_TIMER, 100, NULL);
	}
	else
	{
		while(pgit)
		{
			//exist ghost
			if(pgit->hwnd == hwnd)
				return FALSE;

			if(pgit->next == NULL) break;
			pgit = pgit->next;
		}
		pgit->next = halloc(sizeof(GHOSTIT));
		
		pgit = pgit->next;
	}
	
	memset(pgit, 0, sizeof(GHOSTIT));
	pgit->hwnd = hwnd;
	SetWindowOpacity(pgit->hwnd, (Ghost_data.awo * 255) / 100);
	pgit->orig_style = GetWindowLong(hwnd, GWL_EXSTYLE);
	SetWindowPos(pgit->hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
	
	return TRUE;
}

void RemoveHWNDGhostIt(HWND hwnd, PGHOSTIT *git)
{
	PGHOSTIT pgit_next, pgit = *git;
	if(pgit == NULL) return;

	if((*git)->hwnd == hwnd)
	{
		pgit_next = (*git)->next;
		SetWindowPos((*git)->hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
		SetWindowLong((*git)->hwnd, GWL_EXSTYLE, (*git)->orig_style);
		SetWindowOpacity((*git)->hwnd, 255);
		hfree(*git);
		*git = pgit_next;
	}
	else
	{
		while(pgit)
		{
			pgit_next = pgit->next;
			if(pgit_next == NULL) break;
			
			if(pgit_next->hwnd == hwnd)
			{
				pgit->next = pgit_next->next;
				SetWindowPos(pgit_next->hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
				SetWindowLong(pgit_next->hwnd, GWL_EXSTYLE, pgit_next->orig_style);
				SetWindowOpacity(pgit_next->hwnd, 255);
				hfree(pgit_next);
				break;
			}
			
			pgit = pgit->next;
		}
	}

	if(*git == NULL)
		KillTimer(pshared->DM2wnd, GHOST_TIMER);
}

void SetGhostStatus(PGHOSTIT *git)
{
	PGHOSTIT pgit = *git;
	HWND fw = GetForegroundWindow();
	while(pgit)
	{
		if(IsWindow(pgit->hwnd) == 0)
		{
			//if window close
			PGHOSTIT next = pgit->next;
			RemoveHWNDGhostIt(pgit->hwnd, git);
			pgit = next;
			continue;
		}
		else
		{
			//is foreground window?
			if(pgit->hwnd == fw)
			{
				if(GetWindowLong(pgit->hwnd, GWL_EXSTYLE) != pgit->orig_style)
				{
					SetWindowOpacity(pgit->hwnd, (Ghost_data.awo * 255) / 100);
					SetWindowLong(pgit->hwnd, GWL_EXSTYLE, 
						pgit->orig_style);
				}
			}
			else
			{
				if((GetWindowLong(pgit->hwnd, GWL_EXSTYLE) & WS_EX_TRANSPARENT) == FALSE)
				{
					SetWindowOpacity(pgit->hwnd, (Ghost_data.iawo * 255) / 100);
					SetWindowLong(pgit->hwnd, GWL_EXSTYLE, 
						pgit->orig_style|WS_EX_TRANSPARENT|WS_EX_LAYERED);
				}
			}
		}

		pgit = pgit->next;
	}
}

void RemoveGhostIt(PGHOSTIT *git)
{
	PGHOSTIT pgit = *git;
	PGHOSTIT pgit_next;

	KillTimer(pshared->DM2wnd, GHOST_TIMER);
	while(pgit)
	{
		pgit_next = pgit->next;
		SetWindowPos(pgit->hwnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
		SetWindowLong(pgit->hwnd, GWL_EXSTYLE, pgit->orig_style);
		SetWindowOpacity(pgit->hwnd, 255);
		hfree(pgit);
		pgit = pgit_next;
	}
	*git = NULL;
}
