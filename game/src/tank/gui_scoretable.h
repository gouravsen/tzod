// gui_scoretable.h

#pragma once

#include <ui/Window.h>

class World;

namespace UI
{

class ScoreTable : public Window
{
public:
	ScoreTable(Window *parent, World &world);

protected:
	virtual void OnParentSize(float width, float height);
	virtual void DrawChildren(DrawingContext &dc, float sx, float sy) const;

private:
	size_t _font;
    World &_world;
};

} // end of namespace UI
// end of file
