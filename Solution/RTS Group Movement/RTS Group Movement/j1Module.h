// ----------------------------------------------------
// j1Module.h
// Interface for all engine modules
// ----------------------------------------------------

#ifndef __j1MODULE_H__
#define __j1MODULE_H__

#include "PugiXml\src\pugixml.hpp"

#include <string>
using namespace std;

class j1App;

struct Collider;
struct ColliderGroup;
enum CollisionState;

class UIElement;
enum UIEvents;

class j1Module
{
public:

	j1Module() : active(false)
	{}

	void Init()
	{
		active = true;
	}

	// Called before render is available
	virtual bool Awake(pugi::xml_node&)
	{
		return true;
	}

	// Called before the first frame
	virtual bool Start()
	{
		return true;
	}

	// Called each loop iteration
	virtual bool PreUpdate()
	{
		return true;
	}

	// Called each loop iteration
	virtual bool Update(float dt)
	{
		return true;
	}

	// Called each loop iteration
	virtual bool PostUpdate()
	{
		return true;
	}

	// Called before quitting
	virtual bool CleanUp()
	{
		return true;
	}

	virtual bool Load(pugi::xml_node&)
	{
		return true;
	}

	virtual bool Save(pugi::xml_node&) const
	{
		return true;
	}

	// Collision
	virtual void OnCollision(ColliderGroup* c1, ColliderGroup* c2, CollisionState collisionState) {}

	virtual void OnUIEvent(UIElement* UIelem, UIEvents UIevent) {}

public:

	string		name;
	bool		active;

};

#endif //__j1MODULE_H__
