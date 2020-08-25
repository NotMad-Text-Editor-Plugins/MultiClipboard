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
#include "MultiClipViewerDialog.h"
#include "resource.h"
#include "ClipboardList.h"
#include "MultiClipboardProxy.h"
#include "NativeLang_def.h"
#include "MultiClipboardSettings.h"
#include "MultiClipboardSettingsDialog.h"
#endif


//************************ define here your toolbar layout *********************

// messages needs to be added in resource.h

ToolBarButtonUnit ListBoxToolBarButtons[] = {
	{IDM_EX_UP, -1, -1, -1, IDB_EX_UP, 0 },
	{IDM_EX_DOWN, -1, -1, -1, IDB_EX_DOWN, 0 },
	{IDM_EX_PASTE, -1, -1, -1, IDB_EX_PASTE, 0 },
	{IDM_EX_PASTE_ALL, -1, -1, -1, IDB_EX_PASTE_ALL, 0 },
	{IDM_EX_COPY_TO_CLIPBOARD, -1, -1, -1, IDB_EX_COPY_TO_CLIPBOARD, 0 },
	{IDM_EX_DELETE, -1, -1, -1, IDB_EX_DELETE, 0 },
	{IDM_EX_DELETE_ALL, -1, -1, -1, IDB_EX_DELETE_ALL, 0 },
	{IDM_EX_OPTIONS, -1, -1, -1, IDB_EX_OPTIONS, 0 }
};
#define ListBoxToolBarSize sizeof(ListBoxToolBarButtons)/sizeof(ToolBarButtonUnit)

//	Note: On change, keep sure to change order of IDM_EX_... also in function GetNameStrFromCmd
LPTSTR ListBoxToolBarToolTip[] = {
	TEXT("Move Item Up"),
	TEXT("Move Item Down"),
	TEXT("Paste Item"),
	TEXT("Paste All Items"),
	TEXT("Copy To OS Clipboard"),
	TEXT("Delete Item"),
	TEXT("Delete All Items"),
	TEXT("MultiClipboard Options")
};


extern HINSTANCE g_hInstance;
extern NppData g_NppData;
extern MultiClipboardProxy g_ClipboardProxy;
extern MultiClipboardSettingsDialog OptionsDlg;


MultiClipViewerDialog::MultiClipViewerDialog()
: DockingDlgInterface(IDD_DOCK_DLG)
, IsShown( false )
, DragListMessage( 0 )
, bNoEditLargeText( TRUE )
, NoEditLargeTextSize( 10000 )
, LargeTextDisplaySize( 2048 )
, bPasteAllReverseOrder( TRUE )
, bPasteAllEOLBetweenItems( TRUE )
, pDataObject( NULL )
, pDropSource( NULL )
{
}


MultiClipViewerDialog::~MultiClipViewerDialog()
{
}


void MultiClipViewerDialog::Init( IModel * pNewModel, MultiClipboardProxy * pClipboardProxy, LoonySettingsManager * pSettings )
{
	DockingDlgInterface::init( g_hInstance, g_NppData._nppHandle );
	IController::Init( pNewModel, pClipboardProxy, pSettings );
	DragListMessage = ::RegisterWindowMessage( DRAGLISTMSGSTRING );

	MultiClipOLEDataObject::CreateDataObject( &pDataObject );
	MultiClipOLEDropSource::CreateDropSource( &pDropSource );
}


void MultiClipViewerDialog::Shutdown()
{
	pDataObject->Release();
	pDropSource->Release();
}


void MultiClipViewerDialog::ShowDialog( bool Show )
{
	if ( !isCreated() )
	{
		create( &TBData );

		// define the default docking behaviour
		if ( !NLGetText( g_hInstance, g_NppData._nppHandle, TEXT("MultiClip Viewer"), TBData.pszName, MAX_PATH) )
		{
			lstrcpy( TBData.pszName, TEXT("MultiClip Viewer") );
		}
		TBData.uMask			= DWS_DF_CONT_LEFT | DWS_ICONTAB;
		TBData.hIconTab		= (HICON)::LoadImage(_hInst, MAKEINTRESOURCE(IDI_MULTICLIPBOARD), IMAGE_ICON, 0, 0, LR_LOADMAP3DCOLORS | LR_LOADTRANSPARENT);
		TBData.pszModuleName	= getPluginFileName();
		TBData.dlgID			= MULTICLIPBOARD_DOCKABLE_WINDOW_INDEX;
		::SendMessage( _hParent, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&TBData );
	}

	display( Show );
	IsShown = Show;
	ShowClipText();
}


BOOL CALLBACK MultiClipViewerDialog::run_dlgProc( HWND hWnd, UINT msg, WPARAM wp, LPARAM lp )
{
	if ( msg == DragListMessage )
	{
		return OnDragListMessage( (LPDRAGLISTINFO)lp );
	}

	switch ( msg )
	{
	case WM_INITDIALOG:
		InitialiseDialog();
		break;

	case WM_SIZE:
	case WM_MOVE:
		{
			RECT rc;
			getClientRect(rc);
			SetSplitterOrientation();
			MultiClipViewerPanel.reSizeTo(rc);
			break;
		}

	case WM_COMMAND:
		if ( (HWND)lp == MultiClipViewerListbox.getHSelf() )
		{
			switch ( HIWORD(wp) )
			{
			case LBN_SELCHANGE:
				OnListSelectionChanged();
				return 0;

			case LBN_DBLCLK:
				OnListDoubleClicked();
				return 0;

			case LBN_DELETEITEM:
				DeleteSelectedItem();
				return 0;
			}
		}
		else if ( (HWND)lp == MultiClipViewerEditBox.getHSelf() )
		{
			switch ( HIWORD(wp) )
			{
			case EN_UPDATE:
				OnEditBoxUpdated();
				return 0;
			}
		}
		else if ( (HWND)lp == ListBoxToolBar.getHSelf() )
		{
			OnToolBarCommand( LOWORD(wp) );
			return 0;
		}
		break;

	case WM_NOTIFY:
		{
			LPNMHDR nmhdr = (LPNMHDR) lp;
			if ( nmhdr->hwndFrom == _hParent )
			{
				switch ( LOWORD( nmhdr->code ) )
				{
				case DMN_FLOAT:
				case DMN_DOCK:
					{
						if ( LOWORD( nmhdr->code ) == DMN_FLOAT )
						{
							_isFloating = true;
						}
						else
						{
							_isFloating = false;
							_iDockedPos = HIWORD( nmhdr->code );
						}
						SetSplitterOrientation();
						break;
					}
				default:
					// Parse all other notifications to docking dialog interface
					return DockingDlgInterface::run_dlgProc( _hSelf, msg, wp, lp );
				}
			}
			else if ( nmhdr->code == TTN_GETDISPINFO )
			{
				OnToolBarRequestToolTip(nmhdr);

				return TRUE;
			}
			else if ( nmhdr->code == RBN_CHEVRONPUSHED )
			{
				NMREBARCHEVRON * lpnm = (NMREBARCHEVRON*)nmhdr;
				if (lpnm->wID == REBAR_BAR_TOOLBAR)
				{
					POINT pt;
					pt.x = lpnm->rc.left;
					pt.y = lpnm->rc.bottom;
					ClientToScreen( nmhdr->hwndFrom, &pt );
					OnToolBarCommand( ListBoxToolBar.doPopop( pt ) );
					return TRUE;
				}
				break;
			}
			else
			{
				// Parse all other notifications to docking dialog interface
				return DockingDlgInterface::run_dlgProc( _hSelf, msg, wp, lp );
			}
			break;
		}

	case WM_DESTROY:
		// Destroy icon of tab
		::DestroyIcon( TBData.hIconTab );
		break;

	default:
		return DockingDlgInterface::run_dlgProc( _hSelf, msg, wp, lp );
	}

	return FALSE;
}


void MultiClipViewerDialog::InitialiseDialog()
{
	MultiClipViewerPanel.init( _hInst, _hSelf );

	ListBoxPanel.init( _hInst, MultiClipViewerPanel.getHSelf() );
	MultiClipViewerListbox.init( _hInst, ListBoxPanel.getHSelf() );
	MultiClipViewerPanel.pChildWin1 = &ListBoxPanel;
	ListBoxPanel.SetChildWindow( &MultiClipViewerListbox );
	ListBoxToolBar.init( _hInst, ListBoxPanel.getHSelf(), TB_STANDARD, ListBoxToolBarButtons, ListBoxToolBarSize );
	ListBoxToolBar.display();
	ListBoxPanel.SetToolbar( &ListBoxToolBar );

	EditBoxPanel.init( _hInst, MultiClipViewerPanel.getHSelf() );
	MultiClipViewerEditBox.init( _hInst, EditBoxPanel.getHSelf() );
	MultiClipViewerPanel.pChildWin2 = &EditBoxPanel;
	EditBoxPanel.SetChildWindow( &MultiClipViewerEditBox );
	MultiClipViewerEditBox.EnableEditBox( FALSE );
}


void MultiClipViewerDialog::SetSplitterOrientation()
{
	RECT rc = {0};

	getClientRect(rc);
	if ( _isFloating )
	{
		if ( (rc.bottom-rc.top) >= (rc.right-rc.left) )
		{
			MultiClipViewerPanel.SetSplitterPanelOrientation( ESPO_VERTICAL );
		}
		else
		{
			MultiClipViewerPanel.SetSplitterPanelOrientation( ESPO_HORIZONTAL );
		}
	}
	else
	{
		if ( _iDockedPos == CONT_LEFT || _iDockedPos == CONT_RIGHT )
		{
			MultiClipViewerPanel.SetSplitterPanelOrientation( ESPO_VERTICAL );
		}
		else
		{
			MultiClipViewerPanel.SetSplitterPanelOrientation( ESPO_HORIZONTAL );
		}
	}
}


void MultiClipViewerDialog::OnModelModified()
{
	ShowClipText();
}


void MultiClipViewerDialog::ShowClipText()
{
	if ( !IsShown )
	{
		return;
	}

	ClipboardList * pClipboardList = (ClipboardList *)GetModel();
	if ( !pClipboardList )
	{
		return;
	}

	MultiClipViewerListbox.ClearAll();
	for ( unsigned int i = 0; i < pClipboardList->GetNumText(); ++i )
	{
		std::wstring textToAdd = pClipboardList->GetText( i ).text;
		if ( textToAdd.size() > 100 )
		{
			textToAdd.resize( 100 );
		}
		MultiClipViewerListbox.AddItem( textToAdd );
	}
}


void MultiClipViewerDialog::OnListSelectionChanged()
{
	INT Index = MultiClipViewerListbox.GetCurrentSelectionIndex();
	if ( Index == LB_ERR )
	{
		return;
	}

	ClipboardList * pClipboardList = (ClipboardList *)GetModel();
	const std::wstring & text = pClipboardList->GetText( Index ).text;
	// Check if text is too large, and we only want to display a bit of it, read-only
	if ( bNoEditLargeText && text.size() > NoEditLargeTextSize )
	{
		// Get internationalised string for large text
		std::wstring largeTextDisplay( TEXT("*** Large Text ***\r\n") );
		std::vector< TCHAR > largeText(512);
		int len = NLGetText( g_hInstance, g_NppData._nppHandle, TEXT("Large Text"), &largeText[0], largeText.capacity() );
		if ( len > 0 )
		{
			largeTextDisplay = std::wstring( TEXT("***") ) + &largeText[0] + std::wstring( TEXT("***\r\n") );
		}

		// Append snippet of text to edit box, up to 1024 chars long
		unsigned int textSize = text.size() > LargeTextDisplaySize ? LargeTextDisplaySize : text.size();
		largeTextDisplay += text.substr( 0, textSize );
		MultiClipViewerEditBox.SetText( largeTextDisplay.c_str() );
		// And make it enabled, but read-only
		MultiClipViewerEditBox.EnableEditBox();
		MultiClipViewerEditBox.SetEditBoxReadOnly( TRUE );
	}
	else
	{
		// Else, just display all the text and make it editable
		MultiClipViewerEditBox.SetText( text.c_str() );
		MultiClipViewerEditBox.EnableEditBox();
		// And make it read-write
		MultiClipViewerEditBox.SetEditBoxReadOnly( FALSE );
	}
	//g_ClipboardProxy.SetFocusToDocument();
}


void MultiClipViewerDialog::OnListDoubleClicked()
{
	PasteSelectedItem();
}


void MultiClipViewerDialog::OnEditBoxUpdated()
{
	ClipboardList * pClipboardList = (ClipboardList *)IController::GetModel();
	if ( !pClipboardList )
	{
		return;
	}

	std::wstring text;
	MultiClipViewerEditBox.GetText( text );
	int SelIndex = MultiClipViewerListbox.GetCurrentSelectionIndex();
	pClipboardList->EditText( SelIndex, text );
	MultiClipViewerListbox.SetCurrentSelectedItem( SelIndex );
}


void MultiClipViewerDialog::OnToolBarRequestToolTip( LPNMHDR nmhdr )
{
	// Tooltip request of toolbar
	LPTOOLTIPTEXT lpttt;

	lpttt = (LPTOOLTIPTEXT)nmhdr;
	lpttt->hinst = _hInst;

	// Specify the resource identifier of the descriptive
	// text for the given button.
	int resId = (int)lpttt->hdr.idFrom;
	int ToolTipIndex = resId - ListBoxToolBarButtons[0]._cmdID;

	TCHAR ToolTipText[MAX_PATH];
	int len = NLGetText( g_hInstance, g_NppData._nppHandle, ListBoxToolBarToolTip[ToolTipIndex], ToolTipText, sizeof(ToolTipText) );
	if ( len == 0 )
	{
		lpttt->lpszText = ListBoxToolBarToolTip[ToolTipIndex];
	}
	else
	{
		lpttt->lpszText = ToolTipText;
	}
}


void MultiClipViewerDialog::OnToolBarCommand( UINT Cmd )
{
	ClipboardList * pClipboardList = (ClipboardList *)IController::GetModel();
	if ( !pClipboardList )
	{
		return;
	}

	// Commands here don't need an active selection in the list box
	switch ( Cmd )
	{
	case IDM_EX_PASTE_ALL:
		PasteAllItems();
		return;

	case IDM_EX_DELETE_ALL:
		DeleteAllItems();
		return;

	case IDM_EX_OPTIONS:
		OptionsDlg.ShowDialog();
		return;
	}

	// Commands below need an active selection in the list box
	int SelIndex = MultiClipViewerListbox.GetCurrentSelectionIndex();
	if ( SelIndex < 0 || SelIndex >= (int)pClipboardList->GetNumText() )
	{
		return;
	}

	switch ( Cmd )
	{
	case IDM_EX_UP:
		if ( SelIndex > 0 )
		{
			pClipboardList->SetTextNewIndex( SelIndex, SelIndex-1 );
			MultiClipViewerListbox.SetCurrentSelectedItem( SelIndex-1 );
		}
		break;

	case IDM_EX_DOWN:
		if ( SelIndex < (int)pClipboardList->GetNumText()-1 )
		{
			pClipboardList->SetTextNewIndex( SelIndex, SelIndex+1 );
			MultiClipViewerListbox.SetCurrentSelectedItem( SelIndex+1 );
		}
		break;

	case IDM_EX_PASTE:
		PasteSelectedItem();
		break;

	case IDM_EX_DELETE:
		DeleteSelectedItem();
		break;

	case IDM_EX_COPY_TO_CLIPBOARD:
		CopySelectedItemToClipboard();
		break;
	}
}


BOOL MultiClipViewerDialog::OnDragListMessage( LPDRAGLISTINFO pDragListInfo )
{

	if ( pDragListInfo->uNotification == DL_BEGINDRAG )
	{
		int listBoxItem = LBItemFromPt( MultiClipViewerListbox.getHSelf(), pDragListInfo->ptCursor, FALSE );
		if ( listBoxItem >= 0 )
		{

			ClipboardList * pClipboardList = (ClipboardList *)IController::GetModel();
			if ( pClipboardList )
			{
				const ClipboardListItem & textItem = pClipboardList->GetText( listBoxItem );
				unsigned int textSizeInBytes = (textItem.text.size() + 1) * 2;	// Wide char = 2 bytes + null terminator

				pDataObject->SetMultiClipDragData( textItem.text.c_str(), textSizeInBytes, textItem.textMode == TCM_COLUMN );
				DWORD dwEffect = 0;
				::DoDragDrop( pDataObject, pDropSource, DROPEFFECT_COPY, &dwEffect );
			}
		}

		// Store the return value in DWL_MSGRESULT. Set to false so we don't receive the rest of the drag messages
		// http://support.microsoft.com/kb/183115
		SetWindowLong( getHSelf(), DWL_MSGRESULT, FALSE );
	}
	return TRUE;
}


void MultiClipViewerDialog::PasteSelectedItem()
{
	INT Index = MultiClipViewerListbox.GetCurrentSelectionIndex();
	if ( Index == LB_ERR )
	{
		return;
	}

	MultiClipViewerEditBox.EnableEditBox();
	ClipboardList * pClipboardList = (ClipboardList *)GetModel();
	g_ClipboardProxy.PasteTextToNpp( pClipboardList->GetText( Index ) );
	g_ClipboardProxy.SetFocusToDocument();
}


void MultiClipViewerDialog::PasteAllItems()
{
	ClipboardList * pClipboardList = (ClipboardList *)GetModel();
	if ( pClipboardList->GetNumText() == 0 )
	{
		return;
	}

	std::wstring eolChar;
	if ( bPasteAllEOLBetweenItems )
	{
		eolChar = g_ClipboardProxy.GetEOLString();
	}

	// Paste all the clipboard text into this string stream first
	g_ClipboardProxy.BeginUndoAction();
	if ( bPasteAllReverseOrder )
	{
		// Use signed type for index because --index will cause value to wrap around when index == 0
		for ( int index = pClipboardList->GetNumText() - 1; index >= 0; --index )
		{
			g_ClipboardProxy.ReplaceSelectionText( pClipboardList->GetText( (unsigned int) index ) );
			g_ClipboardProxy.ReplaceSelectionText( eolChar );
		}
	}
	else
	{
		for ( unsigned int index = 0; index < pClipboardList->GetNumText(); ++index )
		{
			g_ClipboardProxy.ReplaceSelectionText( pClipboardList->GetText( (unsigned int) index ) );
			g_ClipboardProxy.ReplaceSelectionText( eolChar );
		}
	}
	g_ClipboardProxy.EndUndoAction();
	g_ClipboardProxy.SetFocusToDocument();
}


void MultiClipViewerDialog::DeleteSelectedItem()
{
	ClipboardList * pClipboardList = (ClipboardList *)IController::GetModel();
	if ( !pClipboardList )
	{
		return;
	}
	int SelIndex = MultiClipViewerListbox.GetCurrentSelectionIndex();
	if ( SelIndex < 0 || SelIndex >= (int)pClipboardList->GetNumText() )
	{
		return;
	}

	pClipboardList->RemoveText( SelIndex );
	// Select the next item in the list
	MultiClipViewerListbox.SetCurrentSelectedItem( SelIndex, FALSE );
	// Check whether selection is successful
	SelIndex = MultiClipViewerListbox.GetCurrentSelectionIndex();
	if ( SelIndex < 0 || SelIndex >= (int)pClipboardList->GetNumText() )
	{
		// Not successful, clear and disable textbox
		MultiClipViewerEditBox.SetText( std::wstring() );
		MultiClipViewerEditBox.EnableEditBox( FALSE );
	}
	else
	{
		OnListSelectionChanged();
	}
}


void MultiClipViewerDialog::DeleteAllItems()
{
	ClipboardList * pClipboardList = (ClipboardList *)IController::GetModel();
	if ( !pClipboardList )
	{
		return;
	}

	pClipboardList->RemoveAllTexts();
	// Clear and disable textbox since there is nothing left
	MultiClipViewerEditBox.SetText( std::wstring() );
	MultiClipViewerEditBox.EnableEditBox( FALSE );
}


void MultiClipViewerDialog::CopySelectedItemToClipboard()
{
	INT Index = MultiClipViewerListbox.GetCurrentSelectionIndex();
	if ( Index == LB_ERR )
	{
		return;
	}

	MultiClipViewerEditBox.EnableEditBox();
	ClipboardList * pClipboardList = (ClipboardList *)GetModel();
	g_ClipboardProxy.SetTextToSystemClipboard( pClipboardList->GetText( Index ) );
	g_ClipboardProxy.SetFocusToDocument();
}


void MultiClipViewerDialog::OnObserverAdded( LoonySettingsManager * SettingsManager )
{
	SettingsObserver::OnObserverAdded( SettingsManager );

	// Add default settings if it doesn't exists
	SET_SETTINGS_BOOL( SETTINGS_GROUP_MULTI_CLIP_VIEWER, SETTINGS_NO_EDIT_LARGE_TEXT, bNoEditLargeText )
	SET_SETTINGS_INT( SETTINGS_GROUP_MULTI_CLIP_VIEWER, SETTINGS_NO_EDIT_LARGE_TEXT_SIZE, NoEditLargeTextSize )
	SET_SETTINGS_INT( SETTINGS_GROUP_MULTI_CLIP_VIEWER, SETTINGS_LARGE_TEXT_DISPLAY_SIZE, LargeTextDisplaySize )
	SET_SETTINGS_BOOL( SETTINGS_GROUP_MULTI_CLIP_VIEWER, SETTINGS_PASTE_ALL_REVERSE, bPasteAllReverseOrder )
	SET_SETTINGS_BOOL( SETTINGS_GROUP_MULTI_CLIP_VIEWER, SETTINGS_PASTE_ALL_NEWLINE_BETWEEN, bPasteAllEOLBetweenItems )
}


void MultiClipViewerDialog::OnSettingsChanged( const stringType & GroupName, const stringType & SettingName )
{
	if ( GroupName != SETTINGS_GROUP_MULTI_CLIP_VIEWER )
	{
		return;
	}

	IF_SETTING_CHANGED_BOOL( SETTINGS_GROUP_MULTI_CLIP_VIEWER, SETTINGS_NO_EDIT_LARGE_TEXT, bNoEditLargeText )
	else IF_SETTING_CHANGED_INT( SETTINGS_GROUP_MULTI_CLIP_VIEWER, SETTINGS_NO_EDIT_LARGE_TEXT_SIZE, NoEditLargeTextSize )
	else IF_SETTING_CHANGED_INT( SETTINGS_GROUP_MULTI_CLIP_VIEWER, SETTINGS_LARGE_TEXT_DISPLAY_SIZE, LargeTextDisplaySize )
	else IF_SETTING_CHANGED_BOOL( SETTINGS_GROUP_MULTI_CLIP_VIEWER, SETTINGS_PASTE_ALL_REVERSE, bPasteAllReverseOrder )
	else IF_SETTING_CHANGED_BOOL( SETTINGS_GROUP_MULTI_CLIP_VIEWER, SETTINGS_PASTE_ALL_NEWLINE_BETWEEN, bPasteAllEOLBetweenItems )
}