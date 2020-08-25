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
#include "MultiClipboardListbox.h"
#include <commctrl.h>
#endif


void MultiClipboardListbox::init(HINSTANCE hInst, HWND parent)
{
	hNewFont = 0;

	Window::init( hInst, parent );

	_hSelf = CreateWindow( TEXT("listbox"), NULL,
		WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_STANDARD ^ LBS_SORT | LBS_NOINTEGRALHEIGHT,
		0, 0, 1, 1, parent, 0, hInst, NULL );

	if ( !_hSelf )
	{
		return;
	}

	// subclass the listbox control
	oldWndProc = (WNDPROC)::SetWindowLong( _hSelf, GWL_WNDPROC, (LONG)StaticListboxProc );

	// associate this class instance with the listbox instance
	::SetWindowLongPtr( _hSelf, GWL_USERDATA, (LONG)this );

	// Make items draggable
	MakeDragList( _hSelf );

	hNewFont = (HFONT)::SendMessage( _hSelf, WM_GETFONT, 0, 0 );
	if ( hNewFont == NULL )
	{
		hNewFont = (HFONT)::GetStockObject( SYSTEM_FONT );
	}
	LOGFONT lf;
	::GetObject( hNewFont, sizeof( lf ), &lf );
	lf.lfHeight = 15;
	lf.lfWidth = 0;
	lf.lfWeight = FW_NORMAL;
	lstrcpy( lf.lfFaceName, TEXT("Courier New") );
	hNewFont = ::CreateFontIndirect( &lf );
	::SendMessage( _hSelf, WM_SETFONT, (WPARAM)hNewFont, 1 );
}


void MultiClipboardListbox::destroy()
{
	::DeleteObject( hNewFont );
}


void MultiClipboardListbox::AddItem( std::wstring item )
{
	::SendMessage( _hSelf, LB_ADDSTRING, 0, (LPARAM)item.c_str() );
}


void MultiClipboardListbox::ClearAll()
{
	::SendMessage( _hSelf, LB_RESETCONTENT, 0, 0 );
}


INT MultiClipboardListbox::GetItemCount()
{
	return (INT)::SendMessage( _hSelf, LB_GETCOUNT, 0, 0 );
}


INT MultiClipboardListbox::GetCurrentSelectionIndex()
{
	return (INT)::SendMessage( _hSelf, LB_GETCURSEL, 0, 0 );
}


void MultiClipboardListbox::SetCurrentSelectedItem( INT NewSelectionIndex, BOOL bStrictSelect )
{
	if ( bStrictSelect )
	{
		::SendMessage( _hSelf, LB_SETCURSEL, NewSelectionIndex, 0 );
	}
	else
	{
		INT ItemCount = GetItemCount();
		if ( ItemCount == LB_ERR || ItemCount <= 0)
		{
			// Error, can't get item count or no items, don't select anything
			return;
		}
		if ( ItemCount > NewSelectionIndex )
		{
			// Requested index is valid, select it
			::SendMessage( _hSelf, LB_SETCURSEL, NewSelectionIndex, 0 );
		}
		else
		{
			// Requested index out of bounds, select the last item
			::SendMessage( _hSelf, LB_SETCURSEL, ItemCount-1, 0 );
		}
	}
}


LRESULT MultiClipboardListbox::runProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch ( message )
	{
	case WM_KEYUP:
		switch ( wParam )
		{
		case VK_DELETE:
			SendMessage( _hParent, WM_COMMAND, MAKEWPARAM(0, LBN_DELETEITEM), (LPARAM)_hSelf );
			return 0;
		}
	}

	return ::CallWindowProc( oldWndProc, hwnd, message, wParam, lParam );
}