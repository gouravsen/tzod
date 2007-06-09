// Object.h

#pragma once

#include "notify.h"

///////////////////////////////////////////////////////////////////////////////
// forward declarations
class MapFile;
class SaveFile;

class GC_Object;

///////////////////////////////////////////////////////////////////////////////

typedef PtrList<GC_Object> OBJECT_LIST;
typedef GridSet<OBJECT_LIST> OBJECT_GRIDSET;

/////////////////////////////////////////
// rtti and serialization

#define DECLARE_SELF_REGISTRATION(cls)  \
private:                                \
typedef cls __this_class;               \
static bool _registered;                \
static bool _self_register();           \
public:                                 \
static ObjectType this_type;            \
virtual ObjectType GetType()            \
{                                       \
	return this_type;                   \
} private:


#define IMPLEMENT_SELF_REGISTRATION(cls)                              \
ObjectType cls::this_type = _register_type<cls>(typeid(cls).name());  \
bool cls::_registered = cls::_self_register();                        \
bool cls::_self_register()


// for template classes (experimental)
#define IMPLEMENT_SELF_REGISTRATION_T(cls)             \
template<class T>                                      \
ObjectType cls<T>::this_type = (cls<T>::_registered,   \
	_register_type<cls<T> >(typeid(cls<T>).name()));   \
template<class T>                                      \
bool cls<T>::_registered = cls<T>::_self_register();   \
template<class T>                                      \
bool cls<T>::_self_register()


///////////////////////////////////////////////////////////

class ObjectProperty
{
public:
	enum PropertyType
	{
		TYPE_INTEGER,
		TYPE_STRING,
		TYPE_MULTISTRING,
	};

private:
	string_t               _name;
	PropertyType           _type;
	int                    _int_value;
	int                    _int_min;
	int                    _int_max;
	string_t               _str_value;
	std::vector<string_t>  _value_set;
	size_t                 _value_index;

public:
	ObjectProperty(PropertyType type, const string_t &name);

	string_t  GetName(void) const;
	PropertyType GetType(void) const;


	//
	// TYPE_INTEGER
	//
	int  GetValueInt(void) const;
	int  GetMin(void) const;
	int  GetMax(void) const;
	void SetValueInt(int value);
	void SetRange(int min, int max);


	//
	// TYPE_STRING
	//
	void SetValue(const string_t &str);
	const string_t& GetValue(void) const;


	//
	// TYPE_MULTISTRING
	//
	void   AddItem(const string_t &str);
	size_t GetCurrentIndex(void) const;
	void   SetCurrentIndex(size_t index);
	size_t GetSetSize(void) const;
	const string_t& GetSetValue(size_t index) const;
};

class PropertySet : public RefCounted
{
	GC_Object       *_object;
	ObjectProperty   _propName;

protected:
	GC_Object* GetObject() const
	{
		return _object;
	}

public:
	PropertySet(GC_Object *object);

	virtual int GetCount() const;
	virtual ObjectProperty* GetProperty(int index);
	virtual void Exchange(bool bApply);
};

////////////////////////////////////////////////////////////
// object flags

// general
#define GC_FLAG_OBJECT_KILLED                 0x00000001
#define GC_FLAG_OBJECT_NAMED                  0x00000002

// engine events
#define GC_FLAG_OBJECT_EVENTS_TS_FIXED        0x00000004
#define GC_FLAG_OBJECT_EVENTS_TS_FLOATING     0x00000008
#define GC_FLAG_OBJECT_EVENTS_ENDFRAME        0x00000010

#define GC_FLAG_OBJECT_EVENTS_ALL           \
	(GC_FLAG_OBJECT_EVENTS_TS_FIXED|        \
	GC_FLAG_OBJECT_EVENTS_TS_FLOATING|      \
	GC_FLAG_OBJECT_EVENTS_ENDFRAME)

#define GC_FLAG_OBJECT_                       0x00000020

typedef void (GC_Object::*NOTIFYPROC) (GC_Object *sender, void *param);

class GC_Object
{
	//
	// types
	//

protected:

	class MemberOfGlobalList
	{
		OBJECT_LIST           *_list;
		OBJECT_LIST::iterator  _pos;
	public:
		MemberOfGlobalList(OBJECT_LIST &list, GC_Object *obj)
		{
			list.push_front(obj);
			_list = &list;
			_pos  = list.begin();
		}
		~MemberOfGlobalList()
		{
			_list->safe_erase(_pos);
		}
	};


private:
	MemberOfGlobalList _memberOf;

	struct Notify
	{
		NotyfyType           type;
		bool                 once;      // ������� ������ ���� ������� ����� ����������
		bool                 removed;   // ������� �������� ��� ��������
		bool                 hasGuard;  // � ������� ���� ���� OnKillSubscriber
		SafePtr<GC_Object>   subscriber;
		NOTIFYPROC           handler;
		//---------------------------------------
		inline Notify()  { removed = false; }
		inline ~Notify() { subscriber = NULL; }
		bool operator == (const Notify &src) const
		{
			return subscriber == src.subscriber && type == src.type && handler == src.handler;
		}
		struct CleanUp
		{
			bool operator() ( Notify &test ) { return test.removed; }
		};
		void Serialize(SaveFile &f);
	};


	//
	// attributes
	//

private:
	DWORD           _flags;             // ��������� �������� ������������ �������
	int             _refCount;          // ����� ������ �� ������. ��� �������� = 1

	OBJECT_LIST::iterator _itPosFixed;      // ������� � Level::ts_fixed
	OBJECT_LIST::iterator _itPosFloating;   // ������� � Level::ts_floating
	OBJECT_LIST::iterator _itPosEndFrame;   // ������� � Level::endframe

	std::list<Notify> _notifyList;          // ���������, ����������� ������ ��������
	int  _notifyProtectCount;               // ������� ���������� �������� �� ������ _notifyList

protected:
	void SetFlags(DWORD flags)
	{
		_flags |= flags;
	}
	DWORD GetFlags()
	{
		return _flags;
	}
	void ClearFlags(DWORD flags)
	{
		_flags &= ~flags;
	}

	// return true if one of the flags is set
	bool CheckFlags(DWORD flags) const
	{
		return 0 != (_flags & flags);
	}


	//
	// access functions
	//

public:
	bool  IsKilled() const { return CheckFlags(GC_FLAG_OBJECT_KILLED); }


	//
	// construction/destruction
	//

	GC_Object();
	virtual ~GC_Object();


	//
	// operations
	//

private:
	void OnKillSubscriber(GC_Object *sender, void *param);

protected:
	void PulseNotify(NotyfyType type, void *param = NULL);

public:
	int  AddRef();
	int  Release();

	void SetEvents(DWORD dwEvents);

	const char* GetName() const;
	void SetName(const char *name);

	// ������������ ���� guard=false ����� ������ � ��� ������,
	// ���� ��������� ������������� ����� ������, ��� �������� �������
	void Subscribe(NotyfyType type, GC_Object *subscriber,
		NOTIFYPROC handler, bool once = true, bool guard = true);
	void Unsubscribe(GC_Object *subscriber);
	bool IsSubscriber(const GC_Object *object) const;
	GC_Object* GetSubscriber(ObjectType type)  const;


	//
	// serialization
	//

public:
	static GC_Object* CreateFromFile(SaveFile &file);
	virtual bool IsSaved() { return false; }
	virtual void Serialize(SaveFile &f);

protected:
	struct FromFile {};
	GC_Object(FromFile);

private:
	typedef GC_Object* (*LPFROMFILEPROC) (void);
	typedef std::map<ObjectType, LPFROMFILEPROC> _from_file_map;
	static _from_file_map& _get_from_file_map()
	{
		static _from_file_map ffm;
		return ffm;
	}
	template<class T> static GC_Object* _from_file_proc(void)
	{
		return new T(FromFile());
	}
    template<class T> static void _register_for_serialization(ObjectType type)
	{
		_ASSERT(_get_from_file_map().end() == _get_from_file_map().find(type));
		LPFROMFILEPROC pf = _from_file_proc<T>;
        _get_from_file_map()[type] = pf;
	}


	//
	// rtti support
	//

private:
	typedef std::map<string_t, size_t> _type_map;
	static _type_map& _get_type_map()
	{
		static _type_map tm;
		return tm;
	}

protected:
	template<class T>
	static ObjectType _register_type(const char *name)
	{
		size_t index = _get_type_map().size();
		_get_type_map()[name] = index;
		_register_for_serialization<T>((ObjectType) index);
		return (ObjectType) index;
	}

public:
	virtual ObjectType GetType() = 0;



	//
	// overrides
	//

public:
	virtual void Kill();

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);
	virtual void EndFrame();
	virtual void EditorAction();

	virtual SafePtr<PropertySet> GetProperties();
	virtual void mapExchange(MapFile &f);


	//
	// debug helpers
	//

#ifdef NETWORK_DEBUG
public:
	virtual DWORD checksum(void) const
	{
		return 0;
	}
#endif
};

///////////////////////////////////////////////////////////////////////////////
// end of file
