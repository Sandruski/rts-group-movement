#ifndef __j1ENTITY_FACTORY_H__
#define __j1ENTITY_FACTORY_H__

#include "j1Module.h"

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
	list<Entity*> SelectEntitiesWithinRectangle(SDL_Rect rectangleRect);

	// Returns true if an entity occupies the tile
	bool IsAnotherEntityOnTile(Entity* entity, iPoint tile);

	// Get entities info
	/*
	PlayerInfo& GetPlayerInfo() { return player; }
	*/

	bool Save(pugi::xml_node& save) const;
	bool Load(pugi::xml_node& save);

private:

	list<Entity*> unitsSelected;

	list<Entity*> toSpawnEntities;
	list<Entity*> activeEntities;

	string CatPeasant_spritesheet;

	// Entities textures
	string footmanTexName;
	SDL_Texture* footmanTex = nullptr;


	// Entities info
	/*
	PlayerInfo player;
	*/
};

#endif //__j1ENTITY_FACTORY_H__
