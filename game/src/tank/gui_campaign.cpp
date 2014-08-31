// gui_campaign.cpp

#include "globals.h"
#include "gui_campaign.h"
#include "gui_desktop.h"

#include "script.h"

#include "config/Config.h"
#include "config/Language.h"

#include <fs/FileSystem.h>
#include <ui/List.h>
#include <ui/Button.h>
#include <ui/Text.h>
#include <ui/DataSourceAdapters.h>
#include <ui/GuiManager.h>

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

NewCampaignDlg::NewCampaignDlg(Window *parent, FS::FileSystem &fs)
  : Dialog(parent, 512, 400)
  , _fs(fs)
{
	Text *t = Text::Create(this, GetWidth() / 2, 16, g_lang.campaign_title.Get(), alignTextCT);
	t->SetFont("font_default");

	_files = DefaultListBox::Create(this);
	_files->Move(20, 56);
	_files->Resize(472, 280);

	auto files = _fs.GetFileSystem("campaign")->EnumAllFiles("*.lua");
	for( auto it = files.begin(); it != files.end(); ++it )
	{
		it->erase(it->length() - 4); // cut out the file extension
		_files->GetData()->AddItem(*it);
	}
	_files->GetData()->Sort();

	Button::Create(this, g_lang.campaign_ok.Get(), 290, 360)->eventClick = std::bind(&NewCampaignDlg::OnOK, this);
	Button::Create(this, g_lang.campaign_cancel.Get(), 400, 360)->eventClick = std::bind(&NewCampaignDlg::OnCancel, this);
}

NewCampaignDlg::~NewCampaignDlg()
{
}

void NewCampaignDlg::OnOK()
{
	if( -1 == _files->GetCurSel() )
	{
		return;
	}

	g_conf.ui_showmsg.Set(true);

	const std::string& name = _files->GetData()->GetItemText(_files->GetCurSel(), 0);
	if( !script_exec_file(g_env.L, ("campaign/" + name + ".lua").c_str()) )
	{
		static_cast<Desktop*>(GetManager().GetDesktop())->ShowConsole(true);
	}

	Close(_resultOK);
}

void NewCampaignDlg::OnCancel()
{
	Close(_resultCancel);
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

