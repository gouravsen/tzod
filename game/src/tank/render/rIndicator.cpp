#include "rIndicator.h"
#include "gc/RigidBody.h"
#include "gc/Weapons.h"
#include "gc/World.h"
#include <video/TextureManager.h>
#include <video/DrawingContext.h>

R_HealthIndicator::R_HealthIndicator(TextureManager &tm, bool dynamic)
	: _tm(tm)
	, _texId(tm.FindSprite("indicator_health"))
	, _dynamic(dynamic)
{
}

void R_HealthIndicator::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_RigidBodyStatic*>(&actor));
	auto &rigidBody = static_cast<const GC_RigidBodyStatic&>(actor);
	
	vec2d pos = rigidBody.GetPos();
	float radius = _dynamic ? rigidBody.GetRadius() : rigidBody.GetHalfWidth();
	float val = rigidBody.GetHealth() / rigidBody.GetHealthMax();
	dc.DrawIndicator(_texId, pos.x, std::max(pos.y - radius - _tm.GetFrameHeight(_texId, 0), .0f), val);
}


static void DrawWeaponIndicator(const World &world,
								const TextureManager &tm,
								DrawingContext &dc,
								size_t texId,
								const GC_Weapon &weapon,
								float value)
{
	if( GC_RigidBodyStatic *carrier = weapon.GetCarrier() )
	{
		
		vec2d pos = carrier->GetPos();
		float radius = carrier->GetRadius();
		float indicatorHeight = tm.GetFrameHeight(texId, 0);
		dc.DrawIndicator(texId, pos.x, std::min(pos.y + radius, world._sy - indicatorHeight*2), value);
	}
}


R_AmmoIndicator::R_AmmoIndicator(TextureManager &tm)
	: _tm(tm)
	, _texId(tm.FindSprite("indicator_ammo"))
{
}

void R_AmmoIndicator::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_Weap_AutoCannon*>(&actor));
	auto &ac = static_cast<const GC_Weap_AutoCannon&>(actor);
	float value = 1 - (float) ac.GetShots() / (float) ac.GetShotsTotal();
	DrawWeaponIndicator(world, _tm, dc, _texId, ac, value);
}


R_FuelIndicator::R_FuelIndicator(TextureManager &tm)
	: _tm(tm)
	, _texId(tm.FindSprite("indicator_fuel"))
{
}

void R_FuelIndicator::Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const
{
	assert(dynamic_cast<const GC_Weap_Ram*>(&actor));
	auto &ram = static_cast<const GC_Weap_Ram&>(actor);
	DrawWeaponIndicator(world, _tm, dc, _texId, ram, ram.GetFuel() / ram.GetFuelMax());
}
