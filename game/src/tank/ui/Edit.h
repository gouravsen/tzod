// Edit.h

#pragma once

#include "Base.h"
#include "Window.h"


namespace UI
{

///////////////////////////////////////////////////////////////////////////////
// simple EditBox

class Edit : public Window
{
	int   _selStart;
	int   _selEnd;
	int   _offset;
	float _time;
	size_t _font;
	size_t _cursor;
	size_t _selection;

public:
	Edit(Window *parent);
	static Edit* Create(Window *parent, float x, float y, float width);

	int GetTextLength() const;

	void SetInt(int value);
	int  GetInt() const;

	void  SetFloat(float value);
	float GetFloat() const;

	void SetSel(int begin, int end); // -1 means end of string
	int GetSelStart() const;
	int GetSelEnd() const;
	int GetSelMin() const;
	int GetSelMax() const;
	int GetSelLength() const;

	void Paste();
	void Copy() const;

	Delegate<void()> eventChange;

protected:
	virtual void DrawChildren(const DrawingContext *dc, float sx, float sy) const;
	virtual bool OnChar(int c);
	virtual bool OnRawChar(int c);
	virtual bool OnMouseDown(float x, float y, int button);
	virtual bool OnMouseUp(float x, float y, int button);
	virtual bool OnMouseMove(float x, float y);
	virtual bool OnFocus(bool focus);
	virtual void OnEnabledChange(bool enable, bool inherited);
	virtual void OnTextChange();
	virtual void OnTimeStep(float dt);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
