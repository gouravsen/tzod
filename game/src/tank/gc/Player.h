// Player.h

#pragma once

#include "Service.h"
#include "network/ControlPacket.h"
#include "LevelInterfaces.h"


///////////////////////////////////////////////////////////////////////////////
// forward declarations

struct VehicleState;
class GC_Vehicle;

///////////////////////////////////////////////////////////////////////////////

class GC_Player
	: public GC_Service
	, public PlayerHandle
{
	MemberOfGlobalList<LIST_players> _memberOf;

	float     _timeRespawn;

	int       _team;
	int       _score;
	string_t  _nick;
	string_t  _class;
	string_t  _skin;
	string_t  _vehname;
	string_t  _scriptOnDie;
	string_t  _scriptOnRespawn;

	ObjPtr<GC_Vehicle> _vehicle;

protected:
	class MyPropertySet : public GC_Service::MyPropertySet
	{
		typedef GC_Service::MyPropertySet BASE;

		ObjectProperty _propTeam;
		ObjectProperty _propScore;
		ObjectProperty _propNick;
		ObjectProperty _propClass;
		ObjectProperty _propSkin;
		ObjectProperty _propOnDie;
		ObjectProperty _propOnRespawn;
		ObjectProperty _propVehName;

	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

protected:
	virtual void OnRespawn();
	virtual void OnDie();

public:
	GC_Vehicle* GetVehicle() const { return _vehicle; }

	void SetSkin(const string_t &skin);
	const string_t& GetSkin() const { return _skin; }

	void SetNick(const string_t &nick);
	const string_t& GetNick() const { return _nick; }

	void SetClass(const string_t &c);
	const string_t& GetClass() const { return _class; }

	void SetTeam(int team);
	int GetTeam() const { return _team; }

	void SetScore(int score);
	int GetScore() const { return _score; }

public:
	GC_Player();
	GC_Player(FromFile);
	virtual ~GC_Player();
	void UpdateSkin();

	// PlayerHandle
	virtual size_t GetIndex() const;

	// GC_Object
	virtual void Kill();
	virtual void Serialize(SaveFile &f);
	virtual void MapExchange(MapFile &f);
	virtual void TimeStepFixed(float dt);

private:
	void OnVehicleDestroy(GC_Object *sender, void *param);
	void OnVehicleKill(GC_Object *sender, void *param);
};

///////////////////////////////////////////////////////////////////////////////

class GC_PlayerHuman : public GC_Player
{
protected:
	VehicleState _ctrlState;
	bool _ready;
public:
	GC_PlayerHuman();
	GC_PlayerHuman(FromFile);
	virtual ~GC_PlayerHuman() = 0;

	void SetControllerState(const VehicleState &vs);

	bool GetReady() const { return _ready; }
	void SetReady(bool ready) { _ready = ready; }

	virtual void Serialize(SaveFile &f);
};

///////////////////////////////////////////////////////////////////////////////

class GC_PlayerLocal
	: public GC_PlayerHuman
{
	DECLARE_SELF_REGISTRATION(GC_PlayerLocal);

	std::deque<VehicleState> _stateHistory;

public:
	GC_PlayerLocal();
	GC_PlayerLocal(FromFile);
	virtual ~GC_PlayerLocal();

	void StepPredicted(VehicleState &vs, float dt);

	virtual void TimeStepFixed(float dt);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
