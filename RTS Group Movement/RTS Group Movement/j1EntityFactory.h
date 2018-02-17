#ifndef __j1ENTITY_FACTORY_H__
#define __j1ENTITY_FACTORY_H__

#include "j1Module.h"

#include "p2Point.h"

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
	void SelectEntitiesWithinRectangle(SDL_Rect rectangleRect);
	void CreateGroupWithSelectedEntities();

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
	/*
	SDL_Texture* CatPeasantTex = nullptr;
	*/

	// Entities info
	/*
	PlayerInfo player;
	*/
};

#endif //__j1ENTITY_FACTORY_H__
