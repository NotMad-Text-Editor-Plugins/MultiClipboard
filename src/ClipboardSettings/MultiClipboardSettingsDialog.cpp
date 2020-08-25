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

#ifndef UNITY_BUILD_SINGLE_INCLUDE
#include "MultiClipboardSettingsDialog.h"
#include "LoonySettingsManager.h"
#include "MultiClipboardSettings.h"
#include "PluginInterface.h"
#include "NativeLang_def.h"
#include "resource.h"
#include <windowsx.h>
#include <sstream>
#include <vector>
#endif

extern HINSTANCE g_hInstance;
extern LoonySettingsManager g_SettingsManager;
extern NppData				g_NppData;
#define NM_MOUSE_OVER_CONTROL 1000


// This is the subclassed wnd proc for the children control of the settings dialog.
// It is used to trap the mouse move before the message is being sent to the child controls
LRESULT CALLBACK MCBSettingsChildCtrlDlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
	case WM_NCHITTEST:
		HWND hDlgParent = ::GetParent( hwnd );
		int OwnID = ::GetDlgCtrlID( hwnd );

		::SendMessage( hDlgParent, WM_COMMAND, MAKEWPARAM( OwnID, NM_MOUSE_OVER_CONTROL ), (LPARAM)hwnd );
		break;
	}
	WNDPROC OwnWndProc = reinterpret_cast<WNDPROC>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
	return ::CallWindowProc( OwnWndProc, hwnd, msg, wParam, lParam );
}


LRESULT CALLBACK StaticTextChildCtrlDlgDlgProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
	case WM_NCHITTEST:
		HWND hDlgParent = ::GetParent( hwnd );
		int OwnID = ::GetDlgCtrlID( hwnd );

		::SendMessage( hDlgParent, WM_COMMAND, MAKEWPARAM( OwnID, NM_MOUSE_OVER_CONTROL ), (LPARAM)hwnd );
		// Static text needs to return this or Windows will return HTTRANSPARENT and pass this message away
		return HTCLIENT;
	}
	WNDPROC OwnWndProc = reinterpret_cast<WNDPROC>( GetWindowLongPtr( hwnd, GWLP_USERDATA ) );
	return ::CallWindowProc( OwnWndProc, hwnd, msg, wParam, lParam );
}


void MultiClipboardSettingsDialog::Init( HINSTANCE hInst, HWND hNpp )
{
	Window::init( hInst, hNpp );
	CurrentMouseOverID = 0;
}


void MultiClipboardSettingsDialog::ShowDialog( bool Show )
{
	if ( !isCreated() )
	{
		create( IDD_OPTIONS_DLG );
		LoadSettingsControlMap();
		SubclassAllChildControls();
	}
	if ( Show )
	{
		LoadMultiClipboardSettings();
	}
	display( Show );

	goToCenter();
}


BOOL CALLBACK MultiClipboardSettingsDialog::run_dlgProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch ( message )
	{
	case WM_INITDIALOG:
		// Change language
		NLChangeDialog( _hInst, g_NppData._nppHandle, _hSelf, TEXT("Options") );
		return TRUE;

	case WM_COMMAND:
		if ( ( HIWORD(wParam) == NM_MOUSE_OVER_CONTROL ) && ( LOWORD(wParam) != 0 ) )
		{
			if ( CurrentMouseOverID != LOWORD(wParam) )
			{
				CurrentMouseOverID = LOWORD(wParam);
				DisplayMouseOverIDHelp( CurrentMouseOverID );
			}
			break;
		}

		switch ( wParam )
		{
		case IDOK:
			SaveMultiClipboardSettings();
			// fall through
		case IDCANCEL:
			display(FALSE);
			return TRUE;

		default :
			break;
		}


	case WM_MOUSEMOVE:
		if ( CurrentMouseOverID != 0 )
		{
			CurrentMouseOverID = 0;
			DisplayMouseOverIDHelp( CurrentMouseOverID );
		}
		break;
	}
	return FALSE;
}


void MultiClipboardSettingsDialog::SetIntValueToDialog( const std::wstring & GroupName, const std::wstring & SettingName, const int DlgItemID )
{
	int intValue = g_SettingsManager.GetIntSetting( GroupName, SettingName );
	::SetDlgItemInt( _hSelf, DlgItemID, intValue, FALSE );
}


void MultiClipboardSettingsDialog::SetBoolValueToDialog( const std::wstring & GroupName, const std::wstring & SettingName, const int DlgItemID )
{
	bool boolValue = g_SettingsManager.GetBoolSetting( GroupName, SettingName );
	::CheckDlgButton( _hSelf, DlgItemID, boolValue ? BST_CHECKED : BST_UNCHECKED );
}


void MultiClipboardSettingsDialog::GetIntValueFromDialog( const std::wstring & GroupName, const std::wstring & SettingName, const int DlgItemID )
{
	int intValue = ::GetDlgItemInt( _hSelf, DlgItemID, NULL, FALSE );
	g_SettingsManager.SetIntSetting( GroupName, SettingName, intValue );
}


void MultiClipboardSettingsDialog::GetBoolValueFromDialog( const std::wstring & GroupName, const std::wstring & SettingName, const int DlgItemID )
{
	bool boolValue = BST_CHECKED == ::IsDlgButtonChecked( _hSelf, DlgItemID );
	g_SettingsManager.SetBoolSetting( GroupName, SettingName, boolValue );
}


void MultiClipboardSettingsDialog::LoadMultiClipboardSettings()
{
	for ( unsigned int i = 0; i < SettingsControlMap.size(); ++i )
	{
		switch ( SettingsControlMap[i].SettingType )
		{
		case SCTE_BOOL:
			SetBoolValueToDialog( SettingsControlMap[i].GroupName, SettingsControlMap[i].SettingName, SettingsControlMap[i].ControlID );
			break;

		case SCTE_INT:
			SetIntValueToDialog( SettingsControlMap[i].GroupName, SettingsControlMap[i].SettingName, SettingsControlMap[i].ControlID );
			break;

		default:
			break;;
		}
	}
}


void MultiClipboardSettingsDialog::SaveMultiClipboardSettings()
{
	for ( unsigned int i = 0; i < SettingsControlMap.size(); ++i )
	{
		switch ( SettingsControlMap[i].SettingType )
		{
		case SCTE_BOOL:
			GetBoolValueFromDialog( SettingsControlMap[i].GroupName, SettingsControlMap[i].SettingName, SettingsControlMap[i].ControlID );
			break;

		case SCTE_INT:
			GetIntValueFromDialog( SettingsControlMap[i].GroupName, SettingsControlMap[i].SettingName, SettingsControlMap[i].ControlID );
			break;

		default:
			break;;
		}
	}
}


void MultiClipboardSettingsDialog::DisplayMouseOverIDHelp( int ControlID )
{
	if ( ControlID == 0 )
	{
		::SetDlgItemText( _hSelf, IDC_OPTION_EXPLANATION, TEXT("") );
		return;
	}

	std::wostringstream HelpNativeLangIndex;
	HelpNativeLangIndex << ControlID << TEXT("_HELP");
	std::vector< TCHAR > HelpText(512);
	int len = NLGetText( g_hInstance, g_NppData._nppHandle, HelpNativeLangIndex.str().c_str(), &HelpText[0], HelpText.capacity() );
	if ( len == 0 )
	{
		::SetWindowText( ::GetDlgItem( _hSelf, IDC_OPTION_EXPLANATION ), GetControlHelpText( ControlID ) );
	}
	else
	{
		::SetWindowText( ::GetDlgItem( _hSelf, IDC_OPTION_EXPLANATION ), &HelpText[0] );
	}
}


void MultiClipboardSettingsDialog::SubclassChildControl( const int ControlID )
{
	HWND hChild = GetDlgItem( _hSelf, ControlID );
	WNDPROC ChildWndProc = (WNDPROC) SetWindowLong( hChild, GWL_WNDPROC, (LONG) MCBSettingsChildCtrlDlgProc );
	::SetWindowLongPtr( hChild, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ChildWndProc) );
}


void MultiClipboardSettingsDialog::SubclassStaticTextChildControl( const int ControlID )
{
	HWND hChild = GetDlgItem( _hSelf, ControlID );
	WNDPROC ChildWndProc = (WNDPROC) SetWindowLong( hChild, GWL_WNDPROC, (LONG) StaticTextChildCtrlDlgDlgProc );
	::SetWindowLongPtr( hChild, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ChildWndProc) );
}


void MultiClipboardSettingsDialog::SubclassAllChildControls()
{
	for ( unsigned int i = 0; i < SettingsControlMap.size(); ++i )
	{
		SubclassChildControl( SettingsControlMap[i].ControlID );
		if ( SettingsControlMap[i].ControlStaticTextID > 0 )
		{
			SubclassStaticTextChildControl( SettingsControlMap[i].ControlStaticTextID );
		}
	}
}


void MultiClipboardSettingsDialog::GetSettingsGroupAndName( const int Control, std::wstring & GroupName, std::wstring & SettingName )
{
	for ( unsigned int i = 0; i < SettingsControlMap.size(); ++i )
	{
		if ( Control == SettingsControlMap[i].ControlID ||
			 Control == SettingsControlMap[i].ControlStaticTextID )
		{
			GroupName = SettingsControlMap[i].GroupName;
			SettingName = SettingsControlMap[i].SettingName;
		}
	}
}


LPCTSTR MultiClipboardSettingsDialog::GetControlHelpText( int ControlID )
{
	for ( unsigned int i = 0; i < SettingsControlMap.size(); ++i )
	{
		if ( ControlID == SettingsControlMap[i].ControlID ||
			 ControlID == SettingsControlMap[i].ControlStaticTextID )
		{
			return SettingsControlMap[i].SettingHelp.c_str();
		}
	}
	return TEXT("");
}


// All settings to be defined here, and the rest of the functions will take care of the rest
void MultiClipboardSettingsDialog::LoadSettingsControlMap()
{
	SettingsControlMap.push_back( SettingsControlMapStruct(
		IDC_EDIT_MAX_CLIPLIST_SIZE, IDC_TEXT_MAX_CLIPLIST_SIZE, SCTE_INT,
		SETTINGS_GROUP_CLIPBOARDLIST, SETTINGS_MAX_CLIPBOARD_ITEMS,
		TEXT("Maximum number of text to be stored in clipboard buffer") ) );

	SettingsControlMap.push_back( SettingsControlMapStruct(
		IDC_CHECK_COPY_FROM_OTHER_PROGRAMS, SCTE_BOOL,
		SETTINGS_GROUP_OSCLIPBOARD, SETTINGS_COPY_FROM_OTHER_PROGRAMS,
		TEXT("Get text that are copied from other programs") ) );

	SettingsControlMap.push_back( SettingsControlMapStruct(
		IDC_CHECK_ONLY_WHEN_PASTE_IN_NPP, SCTE_BOOL,
		SETTINGS_GROUP_OSCLIPBOARD, SETTINGS_ONLY_WHEN_PASTED_IN_NPP,
		TEXT("When 'Copy text from other programs' is true, then text is only copied when it is immediately pasted into Notepad++") ) );

	SettingsControlMap.push_back( SettingsControlMapStruct(
		IDC_CHECK_USE_PASTE_MENU, SCTE_BOOL,
		SETTINGS_GROUP_PASTE_MENU, SETTINGS_USE_PASTE_MENU,
		TEXT("Use paste menu instead of cyclic paste when Ctrl-Shift-V") ) );

	SettingsControlMap.push_back( SettingsControlMapStruct(
		IDC_CHECK_NUMBERED_PASTE_MENU, SCTE_BOOL,
		SETTINGS_GROUP_PASTE_MENU, SETTINGS_SHOW_NUMBERED_PASTE_MENU,
		TEXT("Use numbers as shortcut keys for selecting menu items instead of the first character of the text") ) );

	SettingsControlMap.push_back( SettingsControlMapStruct(
		IDC_EDIT_PASTE_MENU_WIDTH, IDC_TEXT_PASTE_MENU_WIDTH, SCTE_INT,
		SETTINGS_GROUP_PASTE_MENU, SETTINGS_PASTE_MENU_WIDTH,
		TEXT("Maximum number of characters to display per text on the paste menu") ) );

	SettingsControlMap.push_back( SettingsControlMapStruct(
		IDC_CHECK_MIDDLE_CLICK_PASTE, SCTE_BOOL,
		SETTINGS_GROUP_PASTE_MENU, SETTINGS_MIDDLE_CLICK_PASTE,
		TEXT("Click middle mouse button to paste text from clipboard. Shift-middle click will show paste menu") ) );

	SettingsControlMap.push_back( SettingsControlMapStruct(
		IDC_CHECK_AUTO_COPY_SELECTION, SCTE_BOOL,
		SETTINGS_GROUP_AUTO_COPY, SETTINGS_AUTO_COPY_TEXT_SELECTION,
		TEXT("Automatically copies selected text into clipboard") ) );

	SettingsControlMap.push_back( SettingsControlMapStruct(
		IDC_CHECK_NO_COPY_LARGE_TEXT, SCTE_BOOL,
		SETTINGS_GROUP_OSCLIPBOARD, SETTINGS_IGNORE_LARGE_TEXT,
		TEXT("Do not store large text into MultiClipboard plugin. Improves performance when copy and pasting very large text") ) );

	SettingsControlMap.push_back( SettingsControlMapStruct(
		IDC_CHECK_NO_LARGE_TEXT_EDIT, SCTE_BOOL,
		SETTINGS_GROUP_MULTI_CLIP_VIEWER, SETTINGS_NO_EDIT_LARGE_TEXT,
		TEXT("Do not allow editing of large text in MultiClip Viewer. Improves performance when using the MultiClip Viewer") ) );

	SettingsControlMap.push_back( SettingsControlMapStruct(
		IDC_CHECK_PASTE_ALL_REVERSE, SCTE_BOOL,
		SETTINGS_GROUP_MULTI_CLIP_VIEWER, SETTINGS_PASTE_ALL_REVERSE,
		TEXT("When pasting all items, paste them in reverse order of the clipboard list") ) );

	SettingsControlMap.push_back( SettingsControlMapStruct(
		IDC_CHECK_PASTE_ALL_NEWLINE_BETWEEN, SCTE_BOOL,
		SETTINGS_GROUP_MULTI_CLIP_VIEWER, SETTINGS_PASTE_ALL_NEWLINE_BETWEEN,
		TEXT("When pasting all items, append a newline between each item") ) );

	SettingsControlMap.push_back( SettingsControlMapStruct(
		IDC_CHECK_PERSIST_CLIPBOARD_LIST, SCTE_BOOL,
		SETTINGS_GROUP_CLIPBOARDLIST, SETTINGS_SAVE_CLIPBOARD_SESSION,
		TEXT("Persist the clipboard list across Notepad++ sessions") ) );
}