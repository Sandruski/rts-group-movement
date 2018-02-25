#ifndef __j1ENTITY_FACTORY_H__
#define __j1ENTITY_FACTORY_H__

#include "j1Module.h"
#include "Unit.h"

#include "p2Point.h"

#include <algorithm>
using namespace std;

#define MAX_ENTITIES_SELECTED 8

struct SDL_Texture;
struct SDL_Rect;

class Entity;
class Unit;
struct EntityInfo;
struct UnitInfo;

class j1EntityFactory : public j1Module
{
public:

	j1EntityFactory();
	virtual ~j1EntityFactory();
	bool Awake(pugi::xml_node&);
	bool Start();
	bool PreUpdate();
	bool Update(float dt);
	bool PostUpdate();
	bool CleanUp();
	void OnCollision(Collider* c1, Collider* c2);
	void Draw();

	Unit* AddUnit(const EntityInfo& entityInfo, const UnitInfo& unitInfo);

	// Returns a list with all the entities within a given rectangle
	void SelectEntitiesWithinRectangle(SDL_Rect rectangleRect);



	// Looks for the closest walkable tile to the tile passed as an argument
	iPoint FindClosestWalkableTile(Entity* entity, iPoint tile) const;

	// Returns a list with the last units selection
	list<Entity*> GetLastEntitiesSelected() const;

	// Get entities info
	UnitInfo& GetUnitInfo();

	bool Save(pugi::xml_node& save) const;
	bool Load(pugi::xml_node& save);

private:

	list<Entity*> unitsSelected;

	list<Entity*> toSpawnEntities;

	// Entities textures
	string archerTexName;
	SDL_Texture* archerTex = nullptr;

	// Entities info
	UnitInfo unitInfo;

public:

	list<Entity*> activeEntities;

};

#endif //__j1ENTITY_FACTORY_H__
