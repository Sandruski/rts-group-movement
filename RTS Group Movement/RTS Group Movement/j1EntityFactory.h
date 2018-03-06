#ifndef __j1ENTITY_FACTORY_H__
#define __j1ENTITY_FACTORY_H__

#include "j1Module.h"
#include "Unit.h"

#include "p2Point.h"

#include <string>
#include <algorithm>
using namespace std;

#define MAX_UNITS_SELECTED 8

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

	Unit* AddUnit(const EntityInfo& entityInfo, uint priority);

	Unit* GetUnitByEntity(Entity* entity);

	bool IsUnitOnTile(iPoint tile) const;

	// Selects the unit within the tile
	Unit* SelectUnit(iPoint tile);

	// Selects the units within the rectangle
	void SelectUnitsWithinRectangle(SDL_Rect rectangleRect);

	void SetUnitsSelectedColor();

	// Returns a list with the last units selected
	list<Unit*> GetLastUnitsSelected() const;

	// Get entities info
	UnitInfo& GetUnitInfo();

	bool Save(pugi::xml_node& save) const;
	bool Load(pugi::xml_node& save);

private:

	list<Entity*> activeEntities;
	list<Entity*> toSpawnEntities;
	list<Unit*> unitsSelected;

	// Entities textures
	string archerTexName;
	SDL_Texture* archerTex = nullptr;

	// Entities info
	UnitInfo unitInfo;
};

#endif //__j1ENTITY_FACTORY_H__
