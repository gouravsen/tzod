// Trigger.cpp

#include "stdafx.h"
#include "Trigger.h"
#include "Vehicle.h"
#include "ai.h"

#include "level.h"
#include "Macros.h"
#include "script.h"

#include "fs/SaveFile.h"
#include "fs/MapFile.h"

IMPLEMENT_SELF_REGISTRATION(GC_Trigger)
{
	ED_ITEM( "trigger", "obj_trigger", 6 );
	return true;
}


GC_Trigger::GC_Trigger(float x, float y)
  : GC_2dSprite()
  , _team(0)
  , _radius(1)
  , _radiusDelta(0)
{
	SetTexture("editor_trigger");
	MoveTo(vec2d(x, y));
	SetZ(Z_WOOD);
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
	SetFlags(GC_FLAG_TRIGGER_ENABLED|GC_FLAG_TRIGGER_ONLYVISIBLE, true);
}

GC_Trigger::GC_Trigger(FromFile)
  : GC_2dSprite(FromFile())
{
}

GC_Trigger::~GC_Trigger()
{
}

bool GC_Trigger::GetVisible(const GC_Vehicle *v) const
{
	GC_RigidBodyStatic *object = g_level->TraceNearest(
		g_level->grid_rigid_s, NULL, GetPos(), v->GetPos() - GetPos());
	return object == v;
}

bool GC_Trigger::Test(const GC_Vehicle *v) const
{
	assert(v);
	float rr = (GetPos() - v->GetPos()).sqr();
	float r = (_radius + _radiusDelta) * CELL_SIZE;
	return rr <= r*r && (!CheckFlags(GC_FLAG_TRIGGER_ONLYVISIBLE) 
		|| rr <= _veh->GetRadius() * _veh->GetRadius() || GetVisible(_veh));
}

void GC_Trigger::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);

	f.Serialize(_radius);
	f.Serialize(_radiusDelta);
	f.Serialize(_onEnter);
	f.Serialize(_onLeave);
	f.Serialize(_team);
	f.Serialize(_veh);
}

void GC_Trigger::MapExchange(MapFile &f)
{
	GC_2dSprite::MapExchange(f);

	int onlyVisible = CheckFlags(GC_FLAG_TRIGGER_ONLYVISIBLE);
	int onlyHuman = CheckFlags(GC_FLAG_TRIGGER_ONLYHUMAN);
	int active = CheckFlags(GC_FLAG_TRIGGER_ENABLED);

	MAP_EXCHANGE_INT(active, active, 1);
	MAP_EXCHANGE_INT(only_visible, onlyVisible, 0);
	MAP_EXCHANGE_INT(only_human, onlyHuman, 0);
	MAP_EXCHANGE_INT(team, _team, 0);
	MAP_EXCHANGE_FLOAT(radius, _radius, 1);
	MAP_EXCHANGE_FLOAT(radius_delta, _radiusDelta, 0);
	MAP_EXCHANGE_STRING(on_enter, _onEnter, "");
	MAP_EXCHANGE_STRING(on_leave, _onLeave, "");

	if( f.loading() )
	{
		SetFlags(GC_FLAG_TRIGGER_ONLYVISIBLE, 0!=onlyVisible);
		SetFlags(GC_FLAG_TRIGGER_ONLYHUMAN, 0!=onlyHuman);
		SetFlags(GC_FLAG_TRIGGER_ENABLED, 0!=active);
	}
}

void GC_Trigger::TimeStepFixed(float dt)
{
	if( CheckFlags(GC_FLAG_TRIGGER_ACTIVATED) )
	{
		if( !_veh || !Test(_veh) || !CheckFlags(GC_FLAG_TRIGGER_ENABLED) )
		{
			_veh = NULL;
			SetFlags(GC_FLAG_TRIGGER_ACTIVATED, false);
			script_exec(g_env.L, _onLeave.c_str());
		}
	}
	else if( CheckFlags(GC_FLAG_TRIGGER_ENABLED) )
	{
		assert(!_veh);

		// find nearest vehicle
		float rr_min = _radius * _radius * CELL_SIZE * CELL_SIZE;
		FOREACH( g_level->GetList(LIST_vehicles), GC_Vehicle, veh )
		{
			if( !veh->GetOwner() 
				|| CheckFlags(GC_FLAG_TRIGGER_ONLYHUMAN) 
					&& dynamic_cast<GC_PlayerAI*>(veh->GetOwner()) )
			{
				continue;
			}
			if( _team && veh->GetOwner()->GetTeam() != _team )
			{
				continue;
			}
			float rr = (GetPos() - veh->GetPos()).sqr();
			if( rr < rr_min )
			{
				if( CheckFlags(GC_FLAG_TRIGGER_ONLYVISIBLE) && rr > veh->GetRadius() * veh->GetRadius() )
				{
					if( !GetVisible(veh) ) continue; // vehicle is invisible. skipping
				}
				rr_min = rr;
				_veh = veh;
			}
		}
		if( _veh )
		{
			SetFlags(GC_FLAG_TRIGGER_ACTIVATED, true);

			std::stringstream buf;
			buf << "return function(self,who)";
			buf << _onEnter;
			buf << "\nend";

			if( luaL_loadstring(g_env.L, buf.str().c_str()) )
			{
				GetConsole().Printf(1, "syntax error %s", lua_tostring(g_env.L, -1));
				lua_pop(g_env.L, 1); // pop the error message from the stack
			}
			else
			{
				if( lua_pcall(g_env.L, 0, 1, 0) )
				{
					GetConsole().WriteLine(1, lua_tostring(g_env.L, -1));
					lua_pop(g_env.L, 1); // pop the error message from the stack
				}
				else
				{
					luaT_pushobject(g_env.L, this);
					luaT_pushobject(g_env.L, _veh);
					if( lua_pcall(g_env.L, 2, 0, 0) )
					{
						GetConsole().WriteLine(1, lua_tostring(g_env.L, -1));
						lua_pop(g_env.L, 1); // pop the error message from the stack
					}
				}
			}
		}
	}
}

void GC_Trigger::Draw() const
{
	if( g_level->GetEditorMode() )
	{
		__super::Draw();
	}
}

PropertySet* GC_Trigger::NewPropertySet()
{
	return new MyPropertySet(this);
}

///////////////////////////////////////////////////////////////////////////////

GC_Trigger::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propActive(ObjectProperty::TYPE_INTEGER, "active")
  , _propTeam(ObjectProperty::TYPE_INTEGER, "team")
  , _propRadius(ObjectProperty::TYPE_FLOAT, "radius")
  , _propRadiusDelta(ObjectProperty::TYPE_FLOAT, "radius_delta")
  , _propOnlyHuman(ObjectProperty::TYPE_INTEGER, "only_human")
  , _propOnlyVisible(ObjectProperty::TYPE_INTEGER, "only_visible")
  , _propOnEnter(ObjectProperty::TYPE_STRING, "on_enter")
  , _propOnLeave(ObjectProperty::TYPE_STRING, "on_leave")
{
	_propActive.SetIntRange(0, 1);
	_propOnlyHuman.SetIntRange(0, 1);
	_propOnlyVisible.SetIntRange(0, 1);
	_propTeam.SetIntRange(0, MAX_TEAMS);
	_propRadius.SetFloatRange(0, 100);
	_propRadiusDelta.SetFloatRange(0, 100);
}

int GC_Trigger::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 8;
}

ObjectProperty* GC_Trigger::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
		case 0: return &_propActive;
		case 1: return &_propOnlyHuman;
		case 2: return &_propOnlyVisible;
		case 3: return &_propTeam;
		case 4: return &_propRadius;
		case 5: return &_propRadiusDelta;
		case 6: return &_propOnEnter;
		case 7: return &_propOnLeave;
	}

	assert(false);
	return NULL;
}

void GC_Trigger::MyPropertySet::MyExchange(bool applyToObject)
{
	BASE::MyExchange(applyToObject);

	GC_Trigger *tmp = static_cast<GC_Trigger *>(GetObject());

	if( applyToObject )
	{
		tmp->SetFlags(GC_FLAG_TRIGGER_ONLYHUMAN, 0!=_propOnlyHuman.GetIntValue());
		tmp->SetFlags(GC_FLAG_TRIGGER_ONLYVISIBLE, 0!=_propOnlyVisible.GetIntValue());
		tmp->SetFlags(GC_FLAG_TRIGGER_ENABLED, 0!=_propActive.GetIntValue());
		tmp->_team = _propTeam.GetIntValue();
		tmp->_radius = _propRadius.GetFloatValue();
		tmp->_radiusDelta = _propRadiusDelta.GetFloatValue();
		tmp->_onEnter = _propOnEnter.GetStringValue();
		tmp->_onLeave = _propOnLeave.GetStringValue();
	}
	else
	{
		_propOnlyHuman.SetIntValue(tmp->CheckFlags(GC_FLAG_TRIGGER_ONLYHUMAN) ? 1 : 0);
		_propOnlyVisible.SetIntValue(tmp->CheckFlags(GC_FLAG_TRIGGER_ONLYVISIBLE) ? 1 : 0);
		_propActive.SetIntValue(tmp->CheckFlags(GC_FLAG_TRIGGER_ENABLED) ? 1 : 0);
		_propTeam.SetIntValue(tmp->_team);
		_propRadius.SetFloatValue(tmp->_radius);
		_propRadiusDelta.SetFloatValue(tmp->_radiusDelta);
		_propOnEnter.SetStringValue(tmp->_onEnter);
		_propOnLeave.SetStringValue(tmp->_onLeave);
	}
}


// end of file
