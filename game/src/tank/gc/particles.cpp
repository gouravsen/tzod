// particles.cpp

#include "stdafx.h"
#include "particles.h"

#include "functions.h"

#include "fs/SaveFile.h"

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_POOLED_ALLOCATION(GC_Brick_Fragment_01)

IMPLEMENT_SELF_REGISTRATION(GC_Brick_Fragment_01)
{
	return true;
}

GC_Brick_Fragment_01::GC_Brick_Fragment_01(const vec2d &x0, const vec2d &v0) : GC_2dSprite()
{
	SetZ(Z_PARTICLE);

	_StartFrame = rand();
	_time = 0;
	_time_life = frand(0.5f) + 1.0f;

	SetTexture("particle_brick");

	MoveTo(x0);
	_velocity = v0;

	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FLOATING);
}

GC_Brick_Fragment_01::GC_Brick_Fragment_01(FromFile) : GC_2dSprite(FromFile())
{
}

void GC_Brick_Fragment_01::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);
	f.Serialize(_StartFrame);
	f.Serialize(_time);
	f.Serialize(_time_life);
	f.Serialize(_velocity);
}

void GC_Brick_Fragment_01::TimeStepFloat(float dt)
{
	_time += dt;

	if( _time >= _time_life * 0.5f )
	{
		Kill();
		return;
	}

	SetFrame(int((float)_StartFrame + (float)(GetFrameCount() - 1) *
		_time / _time_life)%(GetFrameCount() - 1) );

	MoveTo( GetPos() + _velocity * dt );
	_velocity += vec2d(0, 300.0f) * dt;
}

/////////////////////////////////////////////////////////////

IMPLEMENT_POOLED_ALLOCATION(GC_Particle)

IMPLEMENT_SELF_REGISTRATION(GC_Particle)
{
	return true;
}

GC_Particle::GC_Particle(const vec2d &pos, const vec2d &v, const TextureCache &texture, float LifeTime)
  : GC_2dSprite()
{
	SetZ(Z_PARTICLE);

	_fade = false;
	_time = 0;
	_time_life = LifeTime;
	_velocity = v;

	SetTexture(texture);

	MoveTo(pos);
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FLOATING);

	_ASSERT(_time_life > 0);
}

GC_Particle::GC_Particle(const vec2d &pos, const vec2d &v, const TextureCache &texture,
						 float LifeTime, float orient) : GC_2dSprite()
{
	SetZ(Z_PARTICLE);

	_fade = false;
	_time = 0;
	_time_life = LifeTime;
	_velocity = v;

	SetTexture(texture);
	SetRotation(orient);

	MoveTo(pos);
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FLOATING);

	_ASSERT(_time_life > 0);
}

GC_Particle::GC_Particle(FromFile) : GC_2dSprite(FromFile())
{
}

void GC_Particle::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);
	f.Serialize(_fade);
	f.Serialize(_time);
	f.Serialize(_time_life);
	f.Serialize(_velocity);
}

void GC_Particle::TimeStepFloat(float dt)
{
	_ASSERT(_time_life > 0);
	_time += dt;

	if( _time >= _time_life )
	{
		Kill();
		return;
	}
	SetFrame( int((float)(GetFrameCount() - 1) * _time / _time_life) );
	if( _fade )
		SetOpacity(1.0f - _time / _time_life);

	MoveTo( GetPos() + _velocity * dt );
}

void GC_Particle::SetFade(bool fade)
{
	_fade = fade;
	if( _fade )
		SetOpacity(1.0f - _time / _time_life);
	else
		SetOpacity1i(255);
}

///////////////////////////////////////////////////////////////////////////////
// end of file
