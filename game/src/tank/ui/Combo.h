// Combo.h

#pragma once

#include "Base.h"
#include "Window.h"


namespace UI
{

class ComboBox : public Window
{
public:
	static ComboBox* Create(Window *parent, ListDataSource *dataSource)
	{
		return new ComboBox(parent, dataSource);
	}

	void Resize(float width) { Window::Resize(width, GetHeight()); }

	ListDataSource* GetData() const;

	void SetCurSel(int index);
	int GetCurSel() const;

	List* GetList() const;
	void DropList();

	Delegate<void(int)> eventChangeCurSel;

protected:
	ComboBox(Window *parent, ListDataSource *dataSource);

	void OnEnabledChange(bool enable, bool inherited);
	bool OnRawChar(int c);
	bool OnFocus(bool focus);
	void OnSize(float width, float height);

	void OnClickItem(int index);
	void OnChangeSelection(int index);

	void OnListLostFocus();

private:
	TextButton  *_text;
	ButtonBase  *_btn;
	List        *_list;
	int _curSel;
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
