#pragma once
#include "ObjectView.h"

#include <stddef.h>

class TextureManager;

class R_WeaponBase : public ObjectView
{
public:
	// ObjectView
	virtual enumZOrder GetZ(const GC_Actor &actor) const;
};

class R_Weapon : public R_WeaponBase
{
public:
	R_Weapon(TextureManager &tm, const char *tex);
	
	// ObjectView
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	size_t _texId;
};

class R_WeaponMinigun : public R_WeaponBase
{
public:
	R_WeaponMinigun(TextureManager &tm);
	
	// ObjectView
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	size_t _texId1;
	size_t _texId2;
};