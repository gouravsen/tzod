// pickup.cpp

#include "stdafx.h"

#include "pickup.h"

#include "macros.h"
#include "Level.h"
#include "functions.h"

#include "fs/SaveFile.h"
#include "fs/MapFile.h"

#include "core/Console.h"

#include "GameClasses.h"
#include "indicators.h"
#include "vehicle.h"
#include "Player.h"
#include "Sound.h"
#include "particles.h"

#include "Weapons.h"

///////////////////////////////////////////////////////////////////////////////

GC_Pickup::GC_Pickup(float x, float y)
  : _memberOf(this)
{
	MoveTo(vec2d(x, y));
	AddContext(&g_level->grid_pickup);

	_label = new GC_HideLabel(x, y);

	_radius         = 25.0;
	_timeRespawn    = 0;
	_timeAnimation  = 0;
	_timeAttached   = 0;
	_autoSwitch     = true;
	_respawn        = false;
	_blink          = false;

	SetShadow(true);
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FLOATING | GC_FLAG_OBJECT_EVENTS_TS_FIXED);
	SetZ(Z_FREE_ITEM);
}

GC_Pickup::GC_Pickup(FromFile)
  : GC_2dSprite(FromFile())
  , _memberOf(this)
{
}

void GC_Pickup::Kill()
{
	SAFE_KILL(_label);
	if( Disappear() )
	{
		return; // the object has been already killed
	}
	GC_2dSprite::Kill();
}

void GC_Pickup::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);

	f.Serialize(_owner);
	f.Serialize(_blink);
	f.Serialize(_autoSwitch);
	f.Serialize(_respawn);
	f.Serialize(_timeAttached);
	f.Serialize(_timeAnimation);
	f.Serialize(_timeRespawn);
	f.Serialize(_radius);
	f.Serialize(_scriptOnPickup);
	f.Serialize(_label);

	if( !IsKilled() && f.loading() )
		AddContext(&g_level->grid_pickup);
}

GC_Actor* GC_Pickup::FindNewOwner() const
{
	float r_sq = GetRadius() * GetRadius();

	FOREACH( g_level->GetList(LIST_vehicles), GC_Vehicle, veh )
	{
		if( !veh->IsKilled() )
		{
			if( (GetPos() - veh->GetPos()).sqr() < r_sq )
			{
				if( _autoSwitch || veh->_state._bState_AllowDrop )
					return veh;
			}
		}
	}

	return NULL;
}

void GC_Pickup::Attach(GC_Actor *actor)
{
	_ASSERT(!_owner);
	_owner         = actor;
	_timeAttached  = 0;
	MoveTo(actor->GetPos());
	actor->Subscribe(NOTIFY_ACTOR_MOVE, this, (NOTIFYPROC) &GC_Pickup::OnOwnerMove, false);
	actor->Subscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_Pickup::OnOwnerKill);
	actor->OnPickup(this, true);
}

void GC_Pickup::Detach()
{
	_ASSERT(_owner);
	SetZ(Z_FREE_ITEM);
	_owner->Unsubscribe(this);
	_owner->OnPickup(this, false);
	_owner = NULL;
}

void GC_Pickup::Respawn()
{
	SetRespawn(false);
	Show(true);
	PLAY(SND_puRespawn, GetPos());

	static const TextureCache tex1("particle_1");
	for( int n = 0; n < 50; ++n )
	{
		vec2d a(PI2 * (float) n / 50);
		new GC_Particle(GetPos() + a * 25, a * 25, tex1, frand(0.5f) + 0.1f);
	}
}

bool GC_Pickup::Disappear()
{
	if( IsAttached() )
	{
		Detach();
	}
	if( !IsVisible() )
	{
		return IsKilled();
	}
	Show(false);
	PulseNotify(NOTIFY_PICKUP_DISAPPEAR);
	if( _label )
	{
		MoveTo(_label->GetPos());
		_timeAttached = 0;
		return false;
	}
	Kill();
	return true;
}

void GC_Pickup::SetRespawnTime(float respawnTime)
{
	_timeRespawn = respawnTime;
}

float GC_Pickup::GetRespawnTime() const
{
	return _timeRespawn;
}

void GC_Pickup::SetAutoSwitch(bool autoSwitch)
{
	_autoSwitch = autoSwitch;
}

void GC_Pickup::SetBlinking(bool blink)
{
	_ASSERT(CheckFlags(GC_FLAG_OBJECT_EVENTS_TS_FLOATING));
	_blink = blink;
}

void GC_Pickup::TimeStepFloat(float dt)
{
	_timeAnimation += dt;

	if( !IsAttached() && IsVisible() )
		SetFrame( int((_timeAnimation * ANIMATION_FPS)) % (GetFrameCount()) );

	GC_2dSprite::TimeStepFloat(dt);
}

void GC_Pickup::TimeStepFixed(float dt)
{
	_timeAttached += dt;

	if( !IsAttached() )
	{
		if( IsVisible() )
		{
			if( GC_Actor *actor = FindNewOwner() )
			{
				Attach(actor);

				//
				// call on_pickup script
				//

				const char *who = actor->GetName();
				std::stringstream buf;
				buf << "return function(who)";
				buf << _scriptOnPickup;
				buf << "\nend";

				if( luaL_loadstring(g_env.L, buf.str().c_str()) )
				{
					g_console->printf("syntax error %s\n", lua_tostring(g_env.L, -1));
					lua_pop(g_env.L, 1); // pop the error message from the stack
				}
				else
				{
					if( lua_pcall(g_env.L, 0, 1, 0) )
					{
						g_console->printf("%s\n", lua_tostring(g_env.L, -1));
						lua_pop(g_env.L, 1); // pop the error message from the stack
					}
					lua_pushstring(g_env.L, who ? who : "");
					if( lua_pcall(g_env.L, 1, 0, 0) )
					{
						g_console->printf("%s\n", lua_tostring(g_env.L, -1));
						lua_pop(g_env.L, 1); // pop the error message from the stack
					}
				}
			}
		}
		else
		{
			if( _timeAttached > _timeRespawn )  // FIXME
				Respawn();
		}
	}

	GC_2dSprite::TimeStepFixed(dt);
}

void GC_Pickup::Draw()
{
	if( !_blink || fmodf(_timeAnimation, 0.16f) > 0.08f || g_level->_modeEditor )
		GC_2dSprite::Draw();
}

void GC_Pickup::mapExchange(MapFile &f)
{
	GC_2dSprite::mapExchange(f);
	MAP_EXCHANGE_FLOAT(respawn_time,  _timeRespawn, GetDefaultRespawnTime());
	MAP_EXCHANGE_STRING(on_pickup, _scriptOnPickup, "");
}

void GC_Pickup::OnOwnerMove(GC_Object *sender, void *param)
{
	_ASSERT(IsAttached());
	_ASSERT(GetOwner() == sender);
	MoveTo(static_cast<GC_Actor*>(sender)->GetPos());
}

void GC_Pickup::OnOwnerKill(GC_Object *sender, void *param)
{
	Disappear();
}

PropertySet* GC_Pickup::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_Pickup::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propTimeRespawn(ObjectProperty::TYPE_INTEGER, "respawn_time")
  , _propOnPickup(ObjectProperty::TYPE_STRING, "on_pickup")
{
	_propTimeRespawn.SetIntRange(0, 1000000);
}

int GC_Pickup::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 2;
}

ObjectProperty* GC_Pickup::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propTimeRespawn; break;
	case 1: return &_propOnPickup;    break;
	}

	_ASSERT(FALSE);
	return NULL;
}

void GC_Pickup::MyPropertySet::Exchange(bool applyToObject)
{
	BASE::Exchange(applyToObject);

	GC_Pickup *obj = static_cast<GC_Pickup*>(GetObject());
	if( applyToObject )
	{
		obj->_timeRespawn = (float) _propTimeRespawn.GetIntValue() / 1000.0f;
		obj->_scriptOnPickup = _propOnPickup.GetStringValue();
	}
	else
	{
		_propTimeRespawn.SetIntValue(int(obj->_timeRespawn * 1000.0f + 0.5f));
		_propOnPickup.SetStringValue(obj->_scriptOnPickup);
	}
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_pu_Health)
{
	ED_ITEM( "pu_health", "obj_health", 4 );
	return true;
}

GC_pu_Health::GC_pu_Health(float x, float y)
  : GC_Pickup(x, y)
{
	SetRespawnTime( GetDefaultRespawnTime() );
	SetTexture("pu_health");
}

GC_pu_Health::GC_pu_Health(FromFile)
  : GC_Pickup(FromFile())
{
}

AIPRIORITY GC_pu_Health::GetPriority(GC_Vehicle *veh)
{
	if( veh->GetHealth() < veh->GetHealthMax() )
		return AIP_HEALTH * (veh->GetHealth() / veh->GetHealthMax());

	return AIP_NOTREQUIRED;
}

void GC_pu_Health::Attach(GC_Actor *actor)
{
	GC_Pickup::Attach(actor);

	static_cast<GC_RigidBodyStatic*>(actor)->SetHealthCur(
		static_cast<GC_RigidBodyStatic*>(actor)->GetHealthMax() );
	PLAY(SND_Pickup, GetPos());

	Disappear();
}

GC_Actor* GC_pu_Health::FindNewOwner() const
{
	GC_Vehicle *vehicle = static_cast<GC_Vehicle *>(GC_Pickup::FindNewOwner());

	if( vehicle && !vehicle->_state._bState_AllowDrop &&
		vehicle->GetHealth() >= vehicle->GetHealthMax() )
			return NULL;

	return vehicle;
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_pu_Mine)
{
	ED_ITEM( "pu_mine", "obj_mine", 5 );
	return true;
}

GC_pu_Mine::GC_pu_Mine(float x, float y)
  : GC_Pickup(x, y)
{
	SetRespawnTime( GetDefaultRespawnTime() );
	SetTexture("item_mine");
	SetShadow(false);
}

GC_pu_Mine::GC_pu_Mine(FromFile)
  : GC_Pickup(FromFile())
{
}

AIPRIORITY GC_pu_Mine::GetPriority(GC_Vehicle *veh)
{
	return AIP_NOTREQUIRED;
}

void GC_pu_Mine::Attach(GC_Actor *actor)
{
//	GC_Pickup::Attach(actor);

	_ASSERT(dynamic_cast<GC_RigidBodyStatic*>(actor));
	new GC_Boom_Standard(GetPos(), static_cast<GC_RigidBodyStatic*>(actor));
	Kill();
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_pu_Invulnerablity)
{
	ED_ITEM( "pu_shield", "obj_shield", 4 );
	return true;
}

GC_pu_Invulnerablity::GC_pu_Invulnerablity(float x, float y)
  : GC_Pickup(x, y)
{
	SetRespawnTime( GetDefaultRespawnTime() );
	SetTexture("pu_inv");
	SetAutoSwitch(true);

	_timeHit = 0;
}

GC_pu_Invulnerablity::GC_pu_Invulnerablity(FromFile)
  : GC_Pickup(FromFile())
{
}

AIPRIORITY GC_pu_Invulnerablity::GetPriority(GC_Vehicle *veh)
{
	return AIP_INVULN;
}

void GC_pu_Invulnerablity::Attach(GC_Actor *actor)
{
	if( GC_Object *p = actor->GetSubscriber(GetType()) )
	{
		_ASSERT(dynamic_cast<GC_pu_Invulnerablity*>(p));
		static_cast<GC_Pickup*>(p)->Disappear();
	}

	GC_Pickup::Attach(actor);

	PLAY(SND_Inv, GetPos());

	actor->Subscribe(NOTIFY_DAMAGE_FILTER, this,
		(NOTIFYPROC) &GC_pu_Invulnerablity::OnOwnerDamage, false);

	SetZ(Z_PARTICLE);
	SetTexture("shield");
	SetShadow(false);
}

void GC_pu_Invulnerablity::Detach()
{
	SetTexture("pu_inv");
	SetShadow(true);
	SetBlinking(false);
	SetOpacity(1);

	GC_Pickup::Detach();
}

void GC_pu_Invulnerablity::TimeStepFixed(float dt)
{
	GC_Pickup::TimeStepFixed(dt);

	if( IsAttached() )
	{
		if( GetTimeAttached() + 2.0f > PROTECT_TIME )
		{
			if( !GetBlinking() )
			{
				PLAY(SND_InvEnd, GetPos());
				SetBlinking(true);
			}
			SetOpacity( (PROTECT_TIME - GetTimeAttached()) / 2.0f );

			if( GetTimeAttached() > PROTECT_TIME )
			{
				Disappear();
			}
		}
	}
}

void GC_pu_Invulnerablity::TimeStepFloat(float dt)
{
	GC_Pickup::TimeStepFloat(dt);
	if( IsAttached() )
	{
		_timeHit = __max(0, _timeHit - dt);
		SetFrame( int((GetTimeAnimation() * ANIMATION_FPS)) % (GetFrameCount()) );
	}
}

void GC_pu_Invulnerablity::OnOwnerDamage(GC_Object *sender, void *param)
{
	static TextureCache tex("particle_3");

	DamageDesc *pdd = reinterpret_cast<DamageDesc*>(param);
	_ASSERT(NULL != pdd);
	if( pdd->damage > 5 || 0 == rand() % 4 || 0 == _timeHit )
	{
		const vec2d &pos = static_cast<GC_Actor*>(sender)->GetPos();

		_timeHit = 0.2f;

		PLAY(rand() % 2 ? SND_InvHit1 : SND_InvHit2, pos);
		vec2d dir = (pdd->hit - pos).Normalize();
		vec2d p   = vec2d(dir.y, -dir.x);
		vec2d v   = ((GC_Vehicle *) sender)->_lv;
		for( int i = 0; i < 7; i++ )
		{
			new GC_Particle(pos + dir * 26.0f + p * (float) (i<<1), v, tex, frand(0.4f)+0.1f);
			new GC_Particle(pos + dir * 26.0f - p * (float) (i<<1), v, tex, frand(0.4f)+0.1f);
		}
	}
	pdd->damage = 0;
}

void GC_pu_Invulnerablity::Serialize(SaveFile &f)
{
	GC_Pickup::Serialize(f);
	f.Serialize(_timeHit);
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_pu_Shock)
{
	ED_ITEM( "pu_shock", "obj_shock", 4 );
	return true;
}

GC_pu_Shock::GC_pu_Shock(float x, float y)
  : GC_Pickup(x, y)
{
	SetRespawnTime(GetDefaultRespawnTime());
	SetAutoSwitch(false);
	SetTexture("pu_shock");

	_timeout = 1.5f;
}

GC_pu_Shock::GC_pu_Shock(FromFile)
  : GC_Pickup(FromFile())
{
}

GC_pu_Shock::~GC_pu_Shock()
{
}

void GC_pu_Shock::Kill()
{
	SAFE_KILL(_light);
	SAFE_KILL(_effect);
	GC_Pickup::Kill();
}

void GC_pu_Shock::Serialize(SaveFile &f)
{
	GC_Pickup::Serialize(f);
	f.Serialize(_timeout);
	f.Serialize(_effect);
	f.Serialize(_light);
}

AIPRIORITY GC_pu_Shock::GetPriority(GC_Vehicle *veh)
{
	GC_Vehicle *tmp = FindNearVehicle(veh);
	if( !tmp ) return AIP_NOTREQUIRED;

	if( tmp->GetPlayer()->GetTeam() == veh->GetPlayer()->GetTeam()
		&& 0 != tmp->GetPlayer()->GetTeam() )
	{
		return AIP_NOTREQUIRED;
	}

	return AIP_SHOCK;
}

void GC_pu_Shock::Attach(GC_Actor* actor)
{
	_ASSERT(dynamic_cast<GC_RigidBodyStatic*>(actor));

	GC_Pickup::Attach(actor);
	PLAY(SND_ShockActivate, GetPos());
	SetTexture("explosion_g");
}

void GC_pu_Shock::Detach()
{
	GC_Pickup::Detach();
	SetTexture("pu_shock");
	SAFE_KILL(_effect);
	SAFE_KILL(_light);
}

GC_Vehicle* GC_pu_Shock::FindNearVehicle(const GC_RigidBodyStatic *ignore)
{
	//
	// ������� ���������� �����
	//

	float min_dist = AI_MAX_SIGHT * CELL_SIZE;
	float dist;

	GC_Vehicle *pNearTarget = NULL;
	FOREACH( g_level->GetList(LIST_vehicles), GC_Vehicle, pTargetObj )
	{
		if( !pTargetObj->IsKilled() && pTargetObj != ignore )
		{
			// ���������� �� �������
			dist = (GetPos() - pTargetObj->GetPos()).len();

			if( dist < min_dist )
			{
				GC_RigidBodyStatic *pObstacle = g_level->agTrace(g_level->grid_rigid_s,
					static_cast<GC_RigidBodyStatic*>(GetOwner()),
					GetPos(), pTargetObj->GetPos() - GetPos());

				if( pObstacle == pTargetObj )
				{
					// ����������� �������� � ������� ������. �������� ���.
					pNearTarget = pTargetObj;
					min_dist = dist;
				}
			}
		}
	}

	return pNearTarget;
}

void GC_pu_Shock::TimeStepFixed(float dt)
{
	GC_Pickup::TimeStepFixed(dt);

	if( IsAttached() )
	{
		if( !_effect )
		{
			if( GetTimeAttached() >= _timeout )
			{
				GC_Vehicle *pNearTarget = FindNearVehicle(
					static_cast<GC_RigidBodyStatic*>(GetOwner()));

				if( pNearTarget )
				{
					_effect = new GC_Line(GetPos(), pNearTarget->GetPos(), "lighting");
					_effect->SetPhase(frand(1));
					_effect->SetZ(Z_FREE_ITEM);

					_light = new GC_Light(GC_Light::LIGHT_DIRECT);
					_light->MoveTo(GetPos());
					_light->SetRadius(100);
					_light->SetLength((pNearTarget->GetPos() - GetPos()).len());
					_light->SetAngle((pNearTarget->GetPos() - GetPos()).Angle());

					pNearTarget->TakeDamage(1000.0, pNearTarget->GetPos(),
						static_cast<GC_RigidBodyStatic*>(GetOwner()));
				}
				else
				{
					static_cast<GC_RigidBodyStatic*>(GetOwner())
						->TakeDamage(1000.0, GetOwner()->GetPos(),
							static_cast<GC_RigidBodyStatic*>(GetOwner()));
					Disappear();
				}
			}
		}
		else
		{
			float a = (GetTimeAttached() - _timeout) * 5.0f;
			if( a > 1 )
			{
				Disappear();
			}
			else
			{
				_effect->SetOpacity(1.0f - a);
				_light->SetIntensity(1.0f - powf(a, 6));
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_pu_Booster)
{
	ED_ITEM( "pu_booster", "obj_booster", 4 );
	return true;
}

GC_pu_Booster::GC_pu_Booster(float x, float y)
  : GC_Pickup(x, y)
{
	SetRespawnTime(GetDefaultRespawnTime());
	SetTexture("pu_booster");
}

GC_pu_Booster::GC_pu_Booster(FromFile)
  : GC_Pickup(FromFile())
{
}

GC_pu_Booster::~GC_pu_Booster()
{
}

void GC_pu_Booster::Serialize(SaveFile &f)
{
	GC_Pickup::Serialize(f);
	f.Serialize(_sound);
}

AIPRIORITY GC_pu_Booster::GetPriority(GC_Vehicle *veh)
{
	if( !veh->GetWeapon() )
	{
		return AIP_NOTREQUIRED;
	}

	return veh->GetWeapon()->GetAdvanced() ? AIP_BOOSTER_HAVE : AIP_BOOSTER;
}

void GC_pu_Booster::Attach(GC_Actor* actor)
{
	GC_Weapon *w = dynamic_cast<GC_Weapon*>(actor);

	if( NULL == w ) // disappear if actor is not a weapon.
	{
		PLAY(SND_B_End, actor->GetPos());
		Disappear();
		return;
	}

	GC_Pickup::Attach(w);

	if( w->GetAdvanced() )
	{
		// find existing booster
		FOREACH( g_level->GetList(LIST_pickups), GC_Pickup, pickup )
		{
			if( pickup->GetType() == GetType() && this != pickup )
			{
				_ASSERT(dynamic_cast<GC_pu_Booster*>(pickup));
				if( static_cast<GC_pu_Booster*>(pickup)->GetOwner() == w )
				{
					pickup->Disappear(); // detach previous booster
					break;
				}
			}
		}
	}

	w->SetAdvanced(true);
	w->Subscribe(NOTIFY_PICKUP_DISAPPEAR, this, (NOTIFYPROC) &GC_pu_Booster::OnWeaponDisappear);

	PLAY(SND_B_Start, GetPos());
	_ASSERT(NULL == _sound);
	_sound = new GC_Sound_link(SND_B_Loop, SMODE_LOOP, this);

	SetTexture("booster");
	SetShadow(false);
}

void GC_pu_Booster::Detach()
{
	_ASSERT(dynamic_cast<GC_Weapon*>(GetOwner()));
	static_cast<GC_Weapon*>(GetOwner())->SetAdvanced(false);
	SetTexture("pu_booster");
	SetShadow(true);
	SAFE_KILL(_sound);
	GC_Pickup::Detach();
}

GC_Actor* GC_pu_Booster::FindNewOwner() const
{
	GC_Vehicle *veh = static_cast<GC_Vehicle *>(GC_Pickup::FindNewOwner());
	if( veh && !veh->_state._bState_AllowDrop && !veh->GetWeapon() )
		return NULL;
	return (veh && veh->GetWeapon()) ? veh->GetWeapon() : static_cast<GC_Actor *>(veh);
}

void GC_pu_Booster::TimeStepFloat(float dt)
{
	GC_Pickup::TimeStepFloat(dt);
	if( IsAttached() )
	{
		SetRotation(GetTimeAnimation() * 50);
	}
}

void GC_pu_Booster::TimeStepFixed(float dt)
{
	GC_Pickup::TimeStepFixed(dt);
	if( IsAttached() )
	{
		_ASSERT(!GetOwner()->IsKilled());
		if( GetTimeAttached() > BOOSTER_TIME )
		{
			PLAY(SND_B_End, GetPos());
			Disappear();
		}
	}
}

void GC_pu_Booster::OnWeaponDisappear(GC_Object *sender, void *param)
{
	Disappear();
}

///////////////////////////////////////////////////////////////////////////////
// end of file
