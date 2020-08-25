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

#ifndef CLIPBOARD_LIST_H
#define CLIPBOARD_LIST_H

#ifndef UNITY_BUILD_SINGLE_INCLUDE
#include "ModelViewController.h"
#include "MultiClipboardProxy.h"
#include <list>
#include <string>
#endif


// Extends from TextItem, so we can store other things we need for the list
class ClipboardListItem : public TextItem
{
public:
	ClipboardListItem();
	ClipboardListItem( const TextItem & textItem );
	bool operator==( const TextItem & rhs ) const;

	void UpdateColumnText();
};


class ClipboardList : public IModel
{
public:
	ClipboardList();

	bool AddText( const TextItem & textItem );
	void RemoveText( const unsigned int index );
	void RemoveAllTexts();
	const ClipboardListItem & GetText( const unsigned int index );
	const ClipboardListItem & PasteText( const unsigned int index );	// Returns text at index, and also move it to the front of the list
	bool EditText( const int index, const std::wstring & newText );
	bool ModifyTextItem( const TextItem & fromTextItem, const TextItem & toTextItem );
	void SetTextNewIndex( const unsigned int index, const unsigned int newIndex );

	bool IsTextAvailable( const std::wstring & text ) const;
	int GetTextItemIndex( const TextItem & text ) const;
	unsigned int GetNumText() const;

	const unsigned int GetMaxListSize() const { return MaxListSize; }
	void SetMaxListSize( const int NewSize );

	void SaveClipboardSession();
	void LoadClipboardSession();

	virtual void OnObserverAdded( LoonySettingsManager * SettingsManager );
	virtual void OnSettingsChanged( const stringType & GroupName, const stringType & SettingName );

private:
	typedef std::list< ClipboardListItem > TextListType;
	typedef TextListType::iterator TextListIterator;
	typedef TextListType::const_iterator ConstTextListIterator;
	typedef TextListType::const_reverse_iterator ConstReverseTextListIterator;
	TextListType textList;

	// Empty struct to return when text invalid index is requested
	ClipboardListItem NullStruct;

	// The max number of entry in text list
	unsigned int MaxListSize;

	// Whether to save clipboard item to disk for next session
	bool bSaveClipboardSession;

	TextListType::iterator GetIterAtIndex( const unsigned int index );
};


#endif