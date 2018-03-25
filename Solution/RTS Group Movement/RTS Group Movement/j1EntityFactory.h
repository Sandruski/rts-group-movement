#ifndef __j1ENTITY_FACTORY_H__
#define __j1ENTITY_FACTORY_H__

#include "j1Module.h"

#include "p2Point.h"

// DynamicEntities (also called Units)
#include "Footman.h"
#include "Grunt.h"

#include <string>
#include <algorithm>
using namespace std;

#define MAX_UNITS_SELECTED 10

struct SDL_Texture;
struct SDL_Rect;

class Entity;
class DynamicEntity;
struct EntityInfo;
enum DynamicEntityType;

//
class Unit;
struct UnitInfo;
//

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

	// Adds a DynamicEntity
	DynamicEntity* AddDynamicEntity(DynamicEntityType dynamicEntityType, fPoint pos, iPoint size, uint currLife, uint maxLife, const UnitInfo& unitInfo, const EntityInfo& entityInfo);

	// Returns a pointer to the Entity that is on the tile or nullptr
	Entity* IsEntityOnTile(iPoint tile, EntityType entityType = EntityType_NoType, EntitySide entitySide = EntitySide_NoSide) const;

	// Selects an Entity
	bool SelectEntity(Entity* entity);

	// Selects the entities within a rectangle
	void SelectEntitiesWithinRectangle(SDL_Rect rectangleRect, EntitySide entitySide = EntitySide_NoSide);

	DynamicEntity* GetDynamicEntityByEntity(Entity* entity) const;

	// Returns a list with the last selected units (unitsSelected list)
	list<DynamicEntity*> GetLastUnitsSelected() const;

	// Updates the selection color of all entities
	void SetUnitsSelectedColor();

	// Returns the entityInfo (normally read from the config file)
	EntityInfo& GetDynamicEntityInfo(DynamicEntityType dynamicEntityType);

	bool Save(pugi::xml_node& save) const;
	bool Load(pugi::xml_node& save);

private:

	list<Entity*> toSpawnEntities;
	list<DynamicEntity*> activeDynamicEntities;

	list<DynamicEntity*> unitsSelected;

	// Entities textures
	string footmanTexName;
	string gruntTexName;
	SDL_Texture* footmanTex = nullptr;
	SDL_Texture* gruntTex = nullptr;

	// Entities info
	FootmanInfo footmanInfo;
	GruntInfo gruntInfo;
};

#endif //__j1ENTITY_FACTORY_H__