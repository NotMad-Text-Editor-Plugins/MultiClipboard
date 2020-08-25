/*
This file is part of MultiClipboard Plugin for Notepad++
Copyright (C) 2009 LoonyChewy

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

// include files
#ifndef UNITY_BUILD_SINGLE_INCLUDE
#include "stdafx.h"

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRA_LEAN
#include "PluginInterface.h"

#include "SciSubClassWrp.h"
#include "MCSubClassWndProc.h"
#include "MultiClipboardProxy.h"
#include "LoonySettingsManager.h"
#include "MultiClipboardSettingsDialog.h"
#include "AboutDialog.h"
#include "NativeLang_def.h"

#include "ClipboardList.h"
#include "OSClipboardController.h"
#include "MultiClipViewerDialog.h"
#include "MultiClipPasteMenu.h"
#include "MultiClipCyclicPaste.h"
#include "SelectedTextAutoCopier.h"
#endif


// information for notepad++
CONST INT	nbFunc	= 4;
CONST TCHAR	PLUGIN_NAME[] = TEXT("&MultiClipboard");

// global values
HINSTANCE			g_hInstance = NULL;
NppData				g_NppData;
MultiClipboardProxy	g_ClipboardProxy;
LoonySettingsManager g_SettingsManager( TEXT("MultiClipboardSettings") );
FuncItem			funcItem[nbFunc];
toolbarIcons		g_TBWndMgr;
SciSubClassWrp		g_ScintillaMain, g_ScintillaSecond;
WNDPROC				g_NppWndProc;

// dialog classes
AboutDialog			AboutDlg;
MultiClipboardSettingsDialog OptionsDlg;

// settings

// ini file name
CONST TCHAR configFileName[] = TEXT( "\\MultiClipboard.xml" );
TCHAR configPath[MAX_PATH];
TCHAR SettingsFilePath[MAX_PATH];

// MVC components for plugin
ClipboardList clipboardList;
OSClipboardController OSClipboard;
MultiClipViewerDialog clipViewerDialog;
MultiClipPasteMenu clipPasteMenu;
MultiClipCyclicPaste cyclicPaste;
SelectedTextAutoCopier autoCopier;

// Function prototypes for this plugin
void ShutDownPlugin();

// load and save properties from/into settings file
void LoadSettings();
void SaveSettings();

// menu functions
void ToggleView();
void ShowClipPasteMenu();
void ShowAboutDlg();
void ShowOptionsDlg();

// main function of dll
BOOL APIENTRY DllMain( HINSTANCE hModule,
                       DWORD  reasonForCall,
                       LPVOID lpReserved )
{
	g_hInstance = hModule;

	switch (reasonForCall)
	{
		case DLL_PROCESS_ATTACH:
		{
			// Set function pointers
			funcItem[0]._pFunc = ToggleView;
			funcItem[1]._pFunc = ShowClipPasteMenu;
			funcItem[2]._pFunc = ShowOptionsDlg;
			funcItem[3]._pFunc = ShowAboutDlg;

			// Fill menu names
			lstrcpy( funcItem[0]._itemName, TEXT("&MultiClip Viewer...") );
			lstrcpy( funcItem[1]._itemName, TEXT("MultiClipboard &Paste") );
			lstrcpy( funcItem[2]._itemName, TEXT("&Options...") );
			lstrcpy( funcItem[3]._itemName, TEXT("&About...") );

			// Set shortcuts
			funcItem[0]._pShKey = new ShortcutKey;
			funcItem[0]._pShKey->_isAlt		= true;
			funcItem[0]._pShKey->_isCtrl	= true;
			funcItem[0]._pShKey->_isShift	= false;
			funcItem[0]._pShKey->_key		= 'V';
			funcItem[1]._pShKey = new ShortcutKey;
			funcItem[1]._pShKey->_isAlt		= false;
			funcItem[1]._pShKey->_isCtrl	= true;
			funcItem[1]._pShKey->_isShift	= true;
			funcItem[1]._pShKey->_key		= 'V';
			funcItem[2]._pShKey = NULL;
			funcItem[3]._pShKey = NULL;
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			delete funcItem[0]._pShKey;
			delete funcItem[1]._pShKey;

			// save settings
			SaveSettings();

			ShutDownPlugin();
			break;
		}
		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;
	}

	return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	// For OLE drag and drop
	OleInitialize( NULL );

	// stores notepad data
	g_NppData = notpadPlusData;

	g_ClipboardProxy.Init();

	// Subclass the Notepad++ windows procedure
	g_NppWndProc = (WNDPROC) SetWindowLong( g_NppData._nppHandle, GWL_WNDPROC, (LONG) MCSubClassNppWndProc );
	// As well as the 2 scintilla windows'
	g_ScintillaMain.Init( g_NppData._scintillaMainHandle, MCSubClassSciWndProc );
	g_ScintillaSecond.Init( g_NppData._scintillaSecondHandle, MCSubClassSciWndProc );

	// load data of plugin
	LoadSettings();

	// initial dialogs
	AboutDlg.Init( g_hInstance, g_NppData );
	OptionsDlg.Init( g_hInstance, g_NppData._nppHandle );

	// Initialisation of MVC components
	g_SettingsManager.AddSettingsObserver( &clipboardList );
	clipboardList.LoadClipboardSession();
	OSClipboard.Init( &clipboardList, &g_ClipboardProxy, &g_SettingsManager );
	clipViewerDialog.Init( &clipboardList, &g_ClipboardProxy, &g_SettingsManager );
	clipPasteMenu.Init( &clipboardList, &g_ClipboardProxy, &g_SettingsManager );
	cyclicPaste.Init( &clipboardList, &g_ClipboardProxy, &g_SettingsManager );
	autoCopier.Init( &clipboardList, &g_ClipboardProxy, &g_SettingsManager );
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(INT *nbF)
{
	*nbF = nbFunc;
	return funcItem;
}

/***
 *	beNotification()
 *
 *	This function is called, if a notification in Scantilla/Notepad++ occurs
 */
extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	if (notifyCode->nmhdr.hwndFrom == g_NppData._nppHandle)
	{
		// Change menu language
		NLChangeNppMenu( (HINSTANCE)g_hInstance, g_NppData._nppHandle, (LPTSTR)PLUGIN_NAME, funcItem, nbFunc );

		// On this notification code you can register your plugin icon in Notepad++ toolbar
		if (notifyCode->nmhdr.code == NPPN_TBMODIFICATION)
		{
			g_TBWndMgr.hToolbarBmp = (HBITMAP)::LoadImage( (HINSTANCE)g_hInstance, MAKEINTRESOURCE(IDB_EX_MULTICLIPBOARD), IMAGE_BITMAP, 0, 0, (LR_LOADMAP3DCOLORS) );
			::SendMessage( g_NppData._nppHandle, NPPM_ADDTOOLBARICON, (WPARAM)funcItem[MULTICLIPBOARD_DOCKABLE_WINDOW_INDEX]._cmdID, (LPARAM)&g_TBWndMgr );
		}
	}
}

/***
 *	messageProc()
 *
 *	This function is called, if a notification from Notepad occurs
 */
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	return TRUE;
}


#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
	return TRUE;
}
#endif //UNICODE


/***
 *	LoadSettings()
 *
 *	Load the parameters of plugin
 */
void LoadSettings(void)
{
	// initialize the config directory
	::SendMessage( g_NppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configPath );

	// Test if config path exist
	if ( ::PathFileExists( configPath ) == FALSE )
	{
		::CreateDirectory( configPath, NULL );
	}

	lstrcpy( SettingsFilePath, configPath );
	lstrcat( SettingsFilePath, configFileName );
	if ( ::PathFileExists(SettingsFilePath) == FALSE )
	{
		::CloseHandle( ::CreateFile( SettingsFilePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL ) );
	}

	g_SettingsManager.LoadSettings( SettingsFilePath );
}

/***
 *	SaveSettings()
 *
 *	Saves the parameters of plugin
 */
void SaveSettings(void)
{
	g_SettingsManager.SaveSettings( SettingsFilePath );

	clipboardList.SaveClipboardSession();
}

void ShutDownPlugin()
{
	clipViewerDialog.Shutdown();
	g_ClipboardProxy.Destroy();
	// Shutdown COM for OLE drag drop
	OleUninitialize();
}

/**************************************************************************
 *	Interface functions
 */
void ToggleView(void)
{
	// get menu and test if dockable dialog is open
	HMENU hMenu = ::GetMenu(g_NppData._nppHandle);
	UINT state = ::GetMenuState(hMenu, funcItem[MULTICLIPBOARD_DOCKABLE_WINDOW_INDEX]._cmdID, MF_BYCOMMAND);

	clipViewerDialog.ShowDialog( state & MF_CHECKED ? false : true );
}


void ShowAboutDlg(void)
{
	AboutDlg.doDialog();
}


void ShowOptionsDlg()
{
	OptionsDlg.ShowDialog();
}


void ShowClipPasteMenu()
{
	if ( clipPasteMenu.IsUsePasteMenu() )
	{
		clipPasteMenu.ShowPasteMenu();
	}
	else
	{
		cyclicPaste.DoCyclicPaste();
	}
}