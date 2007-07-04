// gui_desktop.cpp

#include "stdafx.h"

#include "gui_widgets.h"
#include "gui_desktop.h"
#include "gui_console.h"
#include "gui_editor.h"
#include "gui_settings.h"
#include "gui.h"

#include "GuiManager.h"

#include "config/Config.h"

#include "Level.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

Desktop::Desktop(GuiManager* manager) : Window(manager)
{
	_msg = new MessageArea(this, 100, 100);

	_editor = new EditorLayout(this);
	_editor->Show(false);

	_con = new Console(this, 10, 0);
	_con->Show(false);


	_fps = new FpsCounter(this, 0, 0, alignTextLB);
	g_conf.ui_showfps->eventChange.bind( &Desktop::OnChangeShowFps, this );
	OnChangeShowFps();

	_time = new TimeElapsed(this, 0, 0, alignTextRB );
	g_conf.ui_showtime->eventChange.bind( &Desktop::OnChangeShowTime, this );
	OnChangeShowTime();

	OnRawChar(VK_ESCAPE); // to invoke main menu dialog
}

Desktop::~Desktop()
{
	g_conf.ui_showfps->eventChange.clear();
	g_conf.ui_showtime->eventChange.clear();
}

void Desktop::ShowDesktopBackground(bool show)
{
	SetTexture(show ? "window" : NULL);
}

void Desktop::OnCloseChild(int result)
{
	ShowDesktopBackground(false);
}

MessageArea* Desktop::GetMsgArea() const
{
	return _msg;
}

void Desktop::OnRawChar(int c)
{
	Dialog *dlg = NULL;

	switch( c )
	{
	case VK_OEM_3: // '~'
		_con->Show(true);
		break;

	case VK_ESCAPE:
		dlg = new MainMenuDlg(this);
		ShowDesktopBackground(true);
		dlg->eventClose.bind( &Desktop::OnCloseChild, this );
		break;

	case VK_F2:
		dlg = new NewGameDlg(this);
		ShowDesktopBackground(true);
		dlg->eventClose.bind( &Desktop::OnCloseChild, this );
		break;

	case VK_F12:
		dlg = new SettingsDlg(this);
		ShowDesktopBackground(true);
		dlg->eventClose.bind( &Desktop::OnCloseChild, this );
		break;

	case VK_F5:
		if( g_level )
		{
			g_level->ToggleEditorMode();
			_editor->Show(g_level->_modeEditor);
		}
		break;
	}
}

bool Desktop::OnFocus(bool focus)
{
	return true;
}

void Desktop::OnSize(float width, float height)
{
	_editor->Resize(GetWidth(), GetHeight());
	_con->Resize(GetWidth() - 20, GetHeight() * 0.5f);
	_fps->Move(1, GetHeight() - 1);
	_time->Move( GetWidth() - 1, GetHeight() - 1 );
}

void Desktop::OnChangeShowFps()
{
	_fps->Show(g_conf.ui_showfps->Get());
}

void Desktop::OnChangeShowTime()
{
	_fps->Show(g_conf.ui_showfps->Get());
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
