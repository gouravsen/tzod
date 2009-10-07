// gui_editor.cpp

#include "stdafx.h"

#include "gui_editor.h"
#include "GuiManager.h"

#include "Text.h"
#include "Edit.h"
#include "Combo.h"
#include "List.h"
#include "Scroll.h"
#include "Button.h"
#include "Button.h"

#include "gc/Object.h"
#include "gc/2dSprite.h"
#include "gc/Camera.h"

#include "config/Config.h"
#include "config/Language.h"

#include "Level.h"
#include "Macros.h"
#include "script.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////

NewMapDlg::NewMapDlg(Window *parent)
  : Dialog(parent, 256, 256)
{
	Text *header = Text::Create(this, 128, 20, g_lang->newmap_title->Get(), alignTextCT);
	header->SetFont("font_default");

	Text::Create(this, 40, 75, g_lang->newmap_width->Get(), alignTextLT);
	_width = Edit::Create(this, 60, 90, 80);
	_width->SetInt(g_conf->ed_width->GetInt());

	Text::Create(this, 40, 115, g_lang->newmap_height->Get(), alignTextLT);
	_height = Edit::Create(this, 60, 130, 80);
	_height->SetInt(g_conf->ed_height->GetInt());

	Button::Create(this, g_lang->common_ok->Get(), 20, 200)->eventClick.bind(&NewMapDlg::OnOK, this);
	Button::Create(this, g_lang->common_cancel->Get(), 140, 200)->eventClick.bind(&NewMapDlg::OnCancel, this);

	GetManager()->SetFocusWnd(_width);
}

NewMapDlg::~NewMapDlg()
{
}

void NewMapDlg::OnOK()
{
	g_conf->ed_width->SetInt( __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, _width->GetInt())) );
	g_conf->ed_height->SetInt( __max(LEVEL_MINSIZE, __min(LEVEL_MAXSIZE, _height->GetInt())) );
	script_exec(g_env.L, "reset(); newmap(conf.ed_width, conf.ed_height)");
	Close(_resultOK);
}

void NewMapDlg::OnCancel()
{
	Close(_resultCancel);
}

///////////////////////////////////////////////////////////////////////////////
// PropertyList class implementation

PropertyList::Container::Container(Window *parent)
  : Window(parent)
{
}

//bool PropertyList::Container::OnRawChar(int c)
//{
//	return GetParent()->OnRawChar(c); // pass messages through
//}

PropertyList::PropertyList(Window *parent, float x, float y, float w, float h)
  : Dialog(parent, w, h, false)
{
	Move(x, y);
	_psheet = new Container(this);

	_scrollBar = ScrollBarVertical::Create(this, 0, 0, h);
	_scrollBar->Move(w - _scrollBar->GetWidth(), 0);
	_scrollBar->eventScroll.bind(&PropertyList::OnScroll, this);
	_scrollBar->SetLimit(100);

	Resize(w, h);
	SetEasyMove(true);
	SetClipChildren(true);
}

void PropertyList::DoExchange(bool applyToObject)
{
	int focus = -1;

	if( applyToObject )
	{
		assert(_ps);
		for( int i = 0; i < _ps->GetCount(); ++i )
		{
			ObjectProperty *prop = _ps->GetProperty(i);
			Window         *ctrl = _ctrls[i];

			if( GetManager()->GetFocusWnd() == ctrl )
			{
				focus = i;
			}

			switch( prop->GetType() )
			{
			case ObjectProperty::TYPE_INTEGER:
				assert( dynamic_cast<Edit*>(ctrl) );
				int n;
				n = static_cast<Edit*>(ctrl)->GetInt();
				if( n < prop->GetIntMin() || n > prop->GetIntMax() )
				{
					GetConsole().Printf(1, "WARNING: value %s out of range [%d, %d]\n",
						prop->GetName().c_str(), prop->GetIntMin(), prop->GetIntMax());
					n = __max(prop->GetIntMin(), __min(prop->GetIntMax(), n));
				}
				prop->SetIntValue(n);
				break;
			case ObjectProperty::TYPE_FLOAT:
				assert( dynamic_cast<Edit*>(ctrl) );
				float f;
				f = static_cast<Edit*>(ctrl)->GetFloat();
				if( f < prop->GetFloatMin() || f > prop->GetFloatMax() )
				{
					GetConsole().Printf(1, "WARNING: value %s out of range [%g, %g]\n",
						prop->GetName().c_str(), prop->GetFloatMin(), prop->GetFloatMax());
					f = __max(prop->GetFloatMin(), __min(prop->GetFloatMax(), f));
				}
				prop->SetFloatValue(f);
				break;
			case ObjectProperty::TYPE_STRING:
				assert( dynamic_cast<Edit*>(ctrl) );
				prop->SetStringValue(static_cast<Edit*>(ctrl)->GetText());
				break;
			case ObjectProperty::TYPE_MULTISTRING:
				assert( dynamic_cast<ComboBox*>(ctrl) );
				int index;
				index = static_cast<ComboBox*>(ctrl)->GetCurSel();
				prop->SetCurrentIndex(index);
				break;
			default:
				assert(FALSE);
			}
		}
		_ps->Exchange(true);
	}


	// clear old controls
	while( _psheet->GetFirstChild() )
		_psheet->GetFirstChild()->Destroy();
	_ctrls.clear();

	// create new controls
	if( _ps )
	{
		_ps->Exchange(false);
		float y = 5;
		for( int i = 0; i < _ps->GetCount(); ++i )
		{
			ObjectProperty *prop = _ps->GetProperty(i);
			
			std::stringstream labelTextBuffer;
			labelTextBuffer << prop->GetName();

			Text *label = Text::Create(_psheet, 5, y, "", alignTextLT);
			y += label->GetHeight();
			y += 5;

			Window *ctrl = NULL;

			switch( prop->GetType() )
			{
			case ObjectProperty::TYPE_INTEGER:
				ctrl = Edit::Create(_psheet, 32, y, _psheet->GetWidth() - 64);
				static_cast<Edit*>(ctrl)->SetInt(prop->GetIntValue());
				labelTextBuffer << " (" << prop->GetIntMin() << " - " << prop->GetIntMax() << ")";
				break;
			case ObjectProperty::TYPE_FLOAT:
				ctrl = Edit::Create(_psheet, 32, y, _psheet->GetWidth() - 64);
				static_cast<Edit*>(ctrl)->SetFloat(prop->GetFloatValue());
				labelTextBuffer << " (" << prop->GetFloatMin() << " - " << prop->GetFloatMax() << ")";
				break;
			case ObjectProperty::TYPE_STRING:
				ctrl = Edit::Create(_psheet, 32, y, _psheet->GetWidth() - 64);
				static_cast<Edit*>(ctrl)->SetText(prop->GetStringValue());
				labelTextBuffer << " (string)";
				break;
			case ObjectProperty::TYPE_MULTISTRING:
				ctrl = ComboBox::Create(_psheet, 32, y, _psheet->GetWidth() - 64);
				for( size_t index = 0; index < prop->GetListSize(); ++index )
				{
					static_cast<ComboBox*>(ctrl)->GetList()->AddItem(prop->GetListValue(index));
				}
				static_cast<ComboBox*>(ctrl)->SetCurSel(prop->GetCurrentIndex());
				static_cast<ComboBox*>(ctrl)->GetList()->AlignHeightToContent();
				break;
			default:
				assert(FALSE);
			} // end of switch( prop->GetType() )

			label->SetText(labelTextBuffer.str());

			if( focus == i )
			{
				if( Edit *edit = dynamic_cast<Edit*>(ctrl) )
				{
					edit->SetSel(0, -1);
				}
				GetManager()->SetFocusWnd(ctrl);
			}

			assert(NULL != ctrl);
			_ctrls.push_back(ctrl);
			y += ctrl->GetHeight();
			y += 10;
		}

		_psheet->Resize(_psheet->GetWidth(), y);
		_scrollBar->SetLimit(y - GetHeight());
	}
	else
	{
		_psheet->Resize(_psheet->GetWidth(), 0);
		_scrollBar->SetLimit(0);
	}
	OnScroll(_scrollBar->GetPos());
}

void PropertyList::ConnectTo(const SafePtr<PropertySet> &ps)
{
	if( _ps == ps ) return;
	_ps = ps;
	DoExchange(false);
}

void PropertyList::OnScroll(float pos)
{
	_psheet->Move(0, -floorf(_scrollBar->GetPos()));
}

void PropertyList::OnSize(float width, float height)
{
	_scrollBar->Resize(_scrollBar->GetWidth(), height);
	_scrollBar->Move(width - _scrollBar->GetWidth(), 0);
	_scrollBar->SetLimit(_psheet->GetHeight() - height);
	_psheet->Resize(_scrollBar->GetX(), _psheet->GetHeight());
}

bool PropertyList::OnRawChar(int c)
{
	switch(c)
	{
	case VK_RETURN:
		DoExchange(true);
		_ps->SaveToConfig();
		break;
	case VK_ESCAPE:
		g_conf->ed_showproperties->Set(false);
		SetVisible(false);
		break;
	default:
		return __super::OnRawChar(c);
	}
	return true;
}

bool PropertyList::OnMouseWheel(float x, float y, float z)
{
	_scrollBar->SetPos( _scrollBar->GetPos() - z * 10.0f );
	OnScroll(_scrollBar->GetPos());
	return true;
}

///////////////////////////////////////////////////////////////////////////////

ServiceListDataSource::ServiceListDataSource()
  : _listener(NULL)
{
	assert(!g_level->_serviceListener);
	g_level->_serviceListener = this;
}

ServiceListDataSource::~ServiceListDataSource()
{
	assert(this == g_level->_serviceListener);
	g_level->_serviceListener = NULL;
}

void ServiceListDataSource::SetListener(ListDataSourceListener *listener)
{
	_listener = listener;
}

int ServiceListDataSource::GetItemCount() const
{
	return g_level->GetList(LIST_services).size();
}

int ServiceListDataSource::GetSubItemCount(int index) const
{
	return 2;
}

ULONG_PTR ServiceListDataSource::GetItemData(int index) const
{
	ObjectList::iterator it = g_level->GetList(LIST_services).begin();
	for( int i = 0; i < index; ++i )
	{
		++it;
		assert(g_level->GetList(LIST_services).end() != it);
	}
	return (ULONG_PTR) *it;
}

const string_t& ServiceListDataSource::GetItemText(int index, int sub) const
{
	GC_Object *s = (GC_Object *) GetItemData(index);
	const char *name;
	switch( sub )
	{
	case 0:
		return g_lang.GetRoot()->GetStr(g_level->GetTypeInfo(s->GetType()).desc, NULL)->Get();
	case 1:
		name = s->GetName();
		_nameCache = name ? name : "";
		return _nameCache;
	}
	assert(false);
	_nameCache = "";
	return _nameCache;
}

int ServiceListDataSource::FindItem(const string_t &text) const
{
	return -1;
}

void ServiceListDataSource::OnCreate(GC_Object *obj)
{
	_listener->OnAddItem();
}

void ServiceListDataSource::OnKill(GC_Object *obj)
{
	ObjectList &list = g_level->GetList(LIST_services);
	int found = -1;
	int idx = 0;
	for( ObjectList::iterator it = list.begin(); it != list.end(); ++it )
	{
		if( *it == obj )
		{
			found = idx;
			break;
		}
		++idx;
	}
	assert(-1 != found);

	_listener->OnDeleteItem(found);
}

///////////////////////////////////////////////////////////////////////////////

ServiceList::ServiceList(Window *parent, float x, float y, float w, float h)
  : Dialog(parent, h, w, false)
  , _margins(5)
{
	_labelService = Text::Create(this, _margins, _margins, g_lang->service_type->Get(), alignTextLT);
	_labelName = Text::Create(this, w/2, _margins, g_lang->service_name->Get(), alignTextLT);

	_list = List::Create(this, _margins, _margins + _labelService->GetY() + _labelService->GetHeight(), 1, 1);
	_list->SetData(WrapRawPtr(new ServiceListDataSource()));
	_list->SetDrawBorder(true);
	_list->eventChangeCurSel.bind(&ServiceList::OnSelectService, this);

	_btnCreate = Button::Create(this, g_lang->service_create->Get(), 0, 0);
	_btnCreate->eventClick.bind(&ServiceList::OnCreateService, this);

	_combo = ComboBox::Create(this, _margins, _margins, 1);
	List *ls = _combo->GetList();
	for( int i = 0; i < Level::GetTypeCount(); ++i )
	{
		if( Level::GetTypeInfoByIndex(i).service )
		{
			const char *desc0 = Level::GetTypeInfoByIndex(i).desc;
			ls->AddItem(g_lang.GetRoot()->GetStr(desc0, NULL)->Get(), Level::GetTypeByIndex(i));
		}
	}
	ls->SetTabPos(1, 128);
	ls->AlignHeightToContent();
	ls->Sort();

	Move(x, y);
	Resize(w, h);
	SetEasyMove(true);

	assert(!GetEditorLayout()->eventOnChangeSelection);
	GetEditorLayout()->eventOnChangeSelection.bind(&ServiceList::OnChangeSelectionGlobal, this);
}

ServiceList::~ServiceList()
{
	assert(GetEditorLayout()->eventOnChangeSelection);
	GetEditorLayout()->eventOnChangeSelection.clear();
}

void ServiceList::OnChangeSelectionGlobal(GC_Object *obj)
{
	if( obj )
	{
		for( int i = 0; i < _list->GetItemCount(); ++i )
		{
			if( _list->GetItemData(i) == (ULONG_PTR) obj )
			{
				_list->SetCurSel(i, true);
				break;
			}
		}
	}
}

EditorLayout* ServiceList::GetEditorLayout() const
{
	assert(dynamic_cast<EditorLayout*>(GetParent()));
	return static_cast<EditorLayout*>(GetParent());
}

void ServiceList::OnCreateService()
{
	if( -1 != _combo->GetCurSel() )
	{
		ObjectType type = (ObjectType) _combo->GetList()->GetItemData(_combo->GetCurSel());
		GC_Object *service = g_level->CreateObject(type, 0, 0);
		GetEditorLayout()->SelectNone();
		GetEditorLayout()->Select(service, true);
	}
}

void ServiceList::OnSelectService(int i)
{
	if( -1 == i )
	{
		GetEditorLayout()->SelectNone();
	}
	else
	{
		GetEditorLayout()->Select((GC_Object *) _list->GetItemData(i), true);
	}
}

void ServiceList::OnSize(float width, float height)
{
	_btnCreate->Move(width - _btnCreate->GetWidth() - _margins,
		height - _btnCreate->GetHeight() - _margins);

	_combo->Resize(width - _margins * 3 - _btnCreate->GetWidth(), _combo->GetHeight());
	_combo->Move(_margins, _btnCreate->GetY() + (_btnCreate->GetHeight() - _combo->GetHeight()) / 2);

	_list->Resize(width - 2*_list->GetX(), _btnCreate->GetY() - _list->GetY() - _margins);
	_list->SetTabPos(1, _list->GetWidth() / 2);
}

bool ServiceList::OnRawChar(int c)
{
	switch(c)
	{
	case 'S':
	case VK_ESCAPE:
		g_conf->ed_showservices->Set(false);
		SetVisible(false);
		break;
	default:
		return __super::OnRawChar(c);
	}
	return true;
}


///////////////////////////////////////////////////////////////////////////////

EditorLayout::EditorLayout(Window *parent)
  : Window(parent)
  , _selectedObject(NULL)
{
	SetTexture(NULL, false);

	_help = Text::Create(this, 10, 10, g_lang->f1_help_editor->Get(), alignTextLT);
	_help->SetVisible(false);

	_propList = new PropertyList(this, 5, 5, 512, 256);
	_propList->SetVisible(false);

	_serviceList = new ServiceList(this, 5, 300, 512, 256);
	_serviceList->SetVisible(g_conf->ed_showservices->Get());

	_layerDisp = Text::Create(this, 0, 0, "", alignTextRT);

	_typeList = ComboBox::Create(this, 0, 0, 256);
	List *ls = _typeList->GetList();
	for( int i = 0; i < Level::GetTypeCount(); ++i )
	{
		if( Level::GetTypeInfoByIndex(i).service ) continue;
		const char *desc0 = Level::GetTypeInfoByIndex(i).desc;
		ls->AddItem(g_lang.GetRoot()->GetStr(desc0, NULL)->Get(), Level::GetTypeByIndex(i));
	}
	ls->SetTabPos(1, 128);
	ls->AlignHeightToContent();
	ls->Sort();
	_typeList->eventChangeCurSel.bind(&EditorLayout::OnChangeObjectType, this);
	_typeList->SetCurSel(g_conf->ed_object->GetInt());

	_selectionRect = Window::Create(this);
	_selectionRect->SetTexture("ui/selection", true);
	_selectionRect->SetDrawBorder(true);
	_selectionRect->SetVisible(false);
	_selectionRect->BringToBack();

	_isObjectNew = false;
	_click = true;
	_mbutton = 0;

	assert(!g_conf->ed_uselayers->eventChange);
	g_conf->ed_uselayers->eventChange.bind(&EditorLayout::OnChangeUseLayers, this);
	OnChangeUseLayers();
}

EditorLayout::~EditorLayout()
{
	g_conf->ed_uselayers->eventChange.clear();
}

void EditorLayout::OnKillSelected(GC_Object *sender, void *param)
{
	Select(sender, false);
}

void EditorLayout::OnMoveSelected(GC_Object *sender, void *param)
{
	assert(_selectedObject == sender);
}

void EditorLayout::Select(GC_Object *object, bool bSelect)
{
	assert(object);

	if( bSelect )
	{
		if( _selectedObject != object )
		{
			if( _selectedObject )
			{
				Select(_selectedObject, false);
			}

			_selectedObject = object;
			_propList->ConnectTo(_selectedObject->GetProperties());
			if( g_conf->ed_showproperties->Get() )
			{
				_propList->SetVisible(true);
			}
		}
	}
	else
	{
		assert(object == _selectedObject);
		_selectedObject = NULL;
		_isObjectNew = false;

		_propList->ConnectTo(NULL);
		_selectionRect->SetVisible(false);
		_propList->SetVisible(false);
	}

	if( eventOnChangeSelection )
		INVOKE(eventOnChangeSelection)(_selectedObject);
}

void EditorLayout::SelectNone()
{
	if( _selectedObject )
	{
		Select(_selectedObject, false);
	}
}

bool EditorLayout::OnMouseWheel(float x, float y, float z)
{
	if( z > 0 )
	{
		_typeList->SetCurSel( __max(0, _typeList->GetCurSel() - 1) );
	}
	if( z < 0 )
	{
		_typeList->SetCurSel( __min(_typeList->GetList()->GetItemCount()-1, _typeList->GetCurSel() + 1) );
	}
	return true;
}

bool EditorLayout::OnMouseMove(float x, float y)
{
	if( _mbutton )
	{
		OnMouseDown(x, y, _mbutton);
	}
	return true;
}

bool EditorLayout::OnMouseUp(float x, float y, int button)
{
	if( _mbutton == button )
	{
		_click = true;
		_mbutton = 0;
		ReleaseCapture();
	}
	return true;
}

bool EditorLayout::OnMouseDown(float x, float y, int button)
{
	if( 0 == _mbutton )
	{
		GetManager()->SetCapture(this);
		_mbutton = button;
	}

	if( _mbutton != button )
	{
		return true;
	}

	vec2d mouse;
	if( !g_level->IsEmpty() && GC_Camera::GetWorldMousePos(mouse) )
	{
		ObjectType type = static_cast<ObjectType>(
			_typeList->GetList()->GetItemData(g_conf->ed_object->GetInt()) );

		float align = Level::GetTypeInfo(type).align;
		float offset = Level::GetTypeInfo(type).offset;

		vec2d pt;
		pt.x = __min(g_level->_sx - align, __max(align - offset, mouse.x));
		pt.y = __min(g_level->_sy - align, __max(align - offset, mouse.y));
		pt.x -= fmodf(pt.x + align * 0.5f - offset, align) - align * 0.5f;
		pt.y -= fmodf(pt.y + align * 0.5f - offset, align) - align * 0.5f;

		int layer = -1;
		if( g_conf->ed_uselayers->Get() )
		{
			layer = Level::GetTypeInfo(_typeList->GetList()->GetItemData(_typeList->GetCurSel())).layer;
		}

		if( GC_Object *object = g_level->PickEdObject(mouse, layer) )
		{
			if( 1 == button )
			{
				if( _click && _selectedObject == object )
				{
					object->EditorAction();
					_propList->DoExchange(false);
					if( _isObjectNew )
					{
						// save properties for new object
						object->GetProperties()->SaveToConfig();
					}
				}
				else
				{
					Select(object, true);
				}
			}

			if( 2 == button )
			{
				if( _selectedObject == object )
				{
					Select(object, false);
				}
				object->Kill();
			}
		}
		else
		{
			if( 1 == button )
			{
				// create object
				GC_Object *object = g_level->CreateObject(type, pt.x, pt.y);
				SafePtr<PropertySet> properties = object->GetProperties();

				// set default properties if Ctrl key is not pressed
				if( 0 == (GetAsyncKeyState(VK_CONTROL) & 0x8000) )
				{
					properties->LoadFromConfig();
					properties->Exchange(true);
				}

				Select(object, true);
				_isObjectNew = true;
			}
		}
	}

	_click = false;
	return true;
}

bool EditorLayout::OnFocus(bool focus)
{
	return true;
}

bool EditorLayout::OnRawChar(int c)
{
	switch(c)
	{
	case VK_RETURN:
		if( _selectedObject )
		{
			_propList->SetVisible(true);
			g_conf->ed_showproperties->Set(true);
		}
		break;
	case 'S':
		_serviceList->SetVisible(!_serviceList->GetVisible());
		g_conf->ed_showservices->Set(_serviceList->GetVisible());
		break;
	case VK_DELETE:
		if( _selectedObject )
		{
			GC_Object *o = _selectedObject;
			Select(_selectedObject, false);
			o->Kill();
		}
		break;
	case VK_F1:
		_help->SetVisible(!_help->GetVisible());
		break;
	case VK_F9:
		g_conf->ed_uselayers->Set(!g_conf->ed_uselayers->Get());
		break;
	case 'G':
		g_conf->ed_drawgrid->Set(!g_conf->ed_drawgrid->Get());
		break;
	case VK_ESCAPE:
		if( _selectedObject )
		{
			Select(_selectedObject, false);
		}
		else
		{
			return false;
		}
		break;
	default:
		return false;
	}
	return true;
}

void EditorLayout::OnSize(float width, float height)
{
	_typeList->Move(width - _typeList->GetWidth() - 5, 5);
	_layerDisp->Move(width - _typeList->GetWidth() - 5, 6);
}

void EditorLayout::OnVisibleChange(bool visible, bool inherited)
{
	if( !visible )
	{
		SelectNone();
	}
}

void EditorLayout::OnChangeObjectType(int index)
{
	g_conf->ed_object->SetInt(index);

	std::ostringstream buf;
	buf << g_lang->layer->Get() << Level::GetTypeInfo(_typeList->GetList()->GetItemData(index)).layer << ": ";
	_layerDisp->SetText(buf.str());
}

void EditorLayout::OnChangeUseLayers()
{
	_layerDisp->SetVisible(g_conf->ed_uselayers->Get());
}

void EditorLayout::DrawChildren(const DrawingContext *dc, float sx, float sy) const
{
	if( GC_2dSprite *s = dynamic_cast<GC_2dSprite *>(_selectedObject) )
	{
		assert(g_level);
		GC_Camera *camera = NULL;

		FOREACH( g_level->GetList(LIST_cameras), GC_Camera, c )
		{
			if( c->IsActive() )
			{
				camera = c;
				break;
			}
		}

		assert(camera);
		RECT viewport;
		camera->GetViewport(viewport);

		FRECT rt;
		s->GetGlobalRect(rt);

		_selectionRect->SetVisible(true);
		_selectionRect->Move(
			(float) viewport.left + (rt.left - camera->GetPos().x) * camera->_zoom,
			(float) viewport.top + (rt.top - camera->GetPos().y) * camera->_zoom );
		_selectionRect->Resize(s->GetSpriteWidth() * camera->_zoom, s->GetSpriteHeight() * camera->_zoom);
	}
	else
	{
		_selectionRect->SetVisible(false);
	}
	Window::DrawChildren(dc, sx, sy);
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
