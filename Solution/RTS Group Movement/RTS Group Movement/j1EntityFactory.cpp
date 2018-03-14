#include "Defs.h"
#include "p2Log.h"

#include "j1Module.h"
#include "j1App.h"

#include "j1EntityFactory.h"
#include "j1Render.h"

#include "j1Textures.h"
#include "j1Scene.h"
#include "j1Map.h"
#include "j1Pathfinding.h"
#include "j1Collision.h"

#include "Entity.h"
#include "Unit.h"

#include "Brofiler\Brofiler.h"

j1EntityFactory::j1EntityFactory()
{
	name.assign("entities");
}

// Destructor
j1EntityFactory::~j1EntityFactory()
{
}

bool j1EntityFactory::Awake(pugi::xml_node& config) {

	bool ret = true;

	// Load spritesheets
	pugi::xml_node spritesheets = config.child("spritesheets");
	archerTexName = spritesheets.child("archer").attribute("name").as_string();

	// Load animations
	// Archer
	pugi::xml_node archerAnimations = config.child("animations").child("archer");
	pugi::xml_node currentAnimation;

	// up
	currentAnimation = archerAnimations.child("up");
	unitInfo.up.speed = currentAnimation.attribute("speed").as_float();
	unitInfo.up.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		unitInfo.up.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down
	currentAnimation = archerAnimations.child("down");
	unitInfo.down.speed = currentAnimation.attribute("speed").as_float();
	unitInfo.down.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		unitInfo.down.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// left
	currentAnimation = archerAnimations.child("left");
	unitInfo.left.speed = currentAnimation.attribute("speed").as_float();
	unitInfo.left.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		unitInfo.left.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// right
	currentAnimation = archerAnimations.child("right");
	unitInfo.right.speed = currentAnimation.attribute("speed").as_float();
	unitInfo.right.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		unitInfo.right.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// up-left
	currentAnimation = archerAnimations.child("upLeft");
	unitInfo.upLeft.speed = currentAnimation.attribute("speed").as_float();
	unitInfo.upLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		unitInfo.upLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// up-right
	currentAnimation = archerAnimations.child("upRight");
	unitInfo.upRight.speed = currentAnimation.attribute("speed").as_float();
	unitInfo.upRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		unitInfo.upRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down-left
	currentAnimation = archerAnimations.child("downLeft");
	unitInfo.downLeft.speed = currentAnimation.attribute("speed").as_float();
	unitInfo.downLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		unitInfo.downLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down-right
	currentAnimation = archerAnimations.child("downRight");
	unitInfo.downRight.speed = currentAnimation.attribute("speed").as_float();
	unitInfo.downRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		unitInfo.downRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// idle
	currentAnimation = archerAnimations.child("idle");
	unitInfo.idle.speed = currentAnimation.attribute("speed").as_float();
	unitInfo.idle.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		unitInfo.idle.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}

	return ret;
}

bool j1EntityFactory::Start()
{
	bool ret = true;

	LOG("Loading entities textures");

	archerTex = App->tex->Load(archerTexName.data());

	return ret;
}

bool j1EntityFactory::PreUpdate()
{
	bool ret = true;

	// Spawn entities
	list<Entity*>::const_iterator it = toSpawnEntities.begin();

	while (it != toSpawnEntities.end()) {

		// Move the entity from the waiting list to the active list
		activeEntities.push_back(*it);

		int x = (*it)->entityInfo.pos.x * App->scene->scale;
		int y = (*it)->entityInfo.pos.y * App->scene->scale;
		iPoint spawnPos = App->map->WorldToMap(x, y);
		LOG("Spawning entity at tile %d,%d", spawnPos.x, spawnPos.y);

		it++;
	}
	toSpawnEntities.clear();

	return ret;
}

// Called before render is available
bool j1EntityFactory::Update(float dt)
{
	bool ret = true;

	// Update active entities
	list<Entity*>::const_iterator it = activeEntities.begin();

	while (it != activeEntities.end()) {
		(*it)->Move(dt);
		it++;
	}

	return ret;
}

bool j1EntityFactory::PostUpdate()
{
	bool ret = true;

	// Remove entities
	list<Entity*>::const_iterator it = activeEntities.begin();

	while (it != activeEntities.end()) {
		if ((*it)->remove) {
			delete *it;
			activeEntities.remove(*it);
		}
		it++;
	}

	return ret;
}

// Called before quitting
bool j1EntityFactory::CleanUp()
{
	bool ret = true;

	LOG("Freeing all entities");

	// Remove all entities
	list<Entity*>::const_iterator it = activeEntities.begin();

	while (it != activeEntities.end()) {
		delete *it;
		it++;
	}
	activeEntities.clear();

	it = toSpawnEntities.begin();

	while (it != toSpawnEntities.end()) {
		delete *it;
		it++;
	}
	toSpawnEntities.clear();
	unitsSelected.clear();

	// Free all textures
	App->tex->UnLoad(archerTex);

	return ret;
}

void j1EntityFactory::OnCollision(Collider* c1, Collider* c2)
{
	// Check for collisions
	list<Entity*>::const_iterator it = activeEntities.begin();

	while (it != activeEntities.end()) {
		if ((*it)->GetCollider() == c1) {
			(*it)->OnCollision(c1, c2);
			break;
		}
		it++;
	}
}

void j1EntityFactory::Draw()
{
	// Blit active entities
	list<Entity*>::const_iterator it = activeEntities.begin();

	while (it != activeEntities.end()) {

		switch ((*it)->type) {

		case EntityType_Unit:
			(*it)->Draw(archerTex);
			break;
		}

		it++;
	}
}

// Adds a unit to the toSpawnEntities list
Unit* j1EntityFactory::AddUnit(const EntityInfo& entityInfo, uint priority)
{
	Unit* unit = new Unit(entityInfo, priority);
	toSpawnEntities.push_back(unit);

	return unit;
}

// Returns a pointer to the unit by its entity
Unit* j1EntityFactory::GetUnitByEntity(Entity* entity)
{
	Unit* u = (Unit*)(entity);

	return u;
}

// Returns true if there is a unit on the tile
bool j1EntityFactory::IsUnitOnTile(iPoint tile) const
{
	list<Entity*>::const_iterator active = activeEntities.begin();

	while (active != activeEntities.end()) {

		// ONLY UNITS
		if ((*active)->type == EntityType_Unit) {

			iPoint entityTile = App->map->WorldToMap((*active)->entityInfo.pos.x, (*active)->entityInfo.pos.y);

			if (tile.x == entityTile.x && tile.y == entityTile.y)
				return true;
		}

		active++;
	}

	// We do also need to check the toSpawn list (just in case)
	list<Entity*>::const_iterator toSpawn = toSpawnEntities.begin();

	while (toSpawn != toSpawnEntities.end()) {

		// ONLY UNITS
		if ((*toSpawn)->type == EntityType_Unit) {

			iPoint entityTile = App->map->WorldToMap((*toSpawn)->entityInfo.pos.x, (*toSpawn)->entityInfo.pos.y);

			if (tile.x == entityTile.x && tile.y == entityTile.y)
				return true;
		}

		toSpawn++;
	}

	return false;
}

// Selects the unit within the tile
Unit* j1EntityFactory::SelectUnit(iPoint tile)
{
	BROFILER_CATEGORY(__FUNCTION__, Profiler::Color::Orchid);

	Unit* ret = nullptr;

	list<Entity*>::const_iterator it = activeEntities.begin();

	while (it != activeEntities.end()) {

		// ONLY UNITS
		if ((*it)->type == EntityType_Unit) {

			iPoint entityTile = App->map->WorldToMap((*it)->entityInfo.pos.x, (*it)->entityInfo.pos.y);

			if (tile.x == entityTile.x && tile.y == entityTile.y) {

				// If the unit isn't in the unitsSelected list, add it
				if (find(unitsSelected.begin(), unitsSelected.end(), *it) == unitsSelected.end()) {

					Unit* u = GetUnitByEntity(*it);
					unitsSelected.push_back(u);
					(*it)->isSelected = true;

					ret = u;
				}
			}
			else {

				// If the unit is in the unitsSelected list, remove it
				if (find(unitsSelected.begin(), unitsSelected.end(), *it) != unitsSelected.end()) {
					unitsSelected.remove(GetUnitByEntity(*it));
					(*it)->isSelected = false;
				}
			}
		}

		it++;
	}

	SetUnitsSelectedColor();

	return ret;
}

// Selects the units within the rectangle
void j1EntityFactory::SelectUnitsWithinRectangle(SDL_Rect rectangleRect)
{
	BROFILER_CATEGORY(__FUNCTION__, Profiler::Color::Orchid);

	list<Entity*>::const_iterator it = activeEntities.begin();

	while (it != activeEntities.end()) {

		// ONLY UNITS
		if ((*it)->type == EntityType_Unit) {

			SDL_Rect entityRect = { (*it)->entityInfo.pos.x, (*it)->entityInfo.pos.y, (*it)->entityInfo.size.x, (*it)->entityInfo.size.y };

			// If the unit is within the selection:
			if (SDL_HasIntersection(&entityRect, &rectangleRect)) {

				// It there are less units than MAX_UNITS_SELECTED selected:
				if (unitsSelected.size() < MAX_UNITS_SELECTED) {

					// If the unit isn't in the unitsSelected list, add it
					if (find(unitsSelected.begin(), unitsSelected.end(), *it) == unitsSelected.end()) {
						unitsSelected.push_back(GetUnitByEntity(*it));
						(*it)->isSelected = true;
					}
				}
			}
			else {

				// If the unit is in the unitsSelected list, remove it
				if (find(unitsSelected.begin(), unitsSelected.end(), *it) != unitsSelected.end()) {
					unitsSelected.remove(GetUnitByEntity(*it));
					(*it)->isSelected = false;
				}
			}
		}

		it++;
	}

	SetUnitsSelectedColor();
}

// Returns a list with the last units selected
list<Unit*> j1EntityFactory::GetLastUnitsSelected() const
{
	return unitsSelected;
}

// Changes the debug color of the units selected
void j1EntityFactory::SetUnitsSelectedColor()
{
	SDL_Color colors[8] = { ColorYellow, ColorDarkGreen, ColorBrightBlue, ColorOrange, ColorPink, ColorPurple, ColorGrey, ColorBlack };
	string colorNames[8] = { "Yellow", "DarkGreen", "BrightBlue", "Orange", "Pink", "Purple", "Grey", "Black" };

	list<Unit*>::const_iterator it = unitsSelected.begin();
	uint i = 0;

	while (it != unitsSelected.end())
	{
		(*it)->SetColor(colors[i], colorNames[i]);
		it++;
		i++;
	}
}

// Returns the unitInfo (normally read from the config file)
UnitInfo& j1EntityFactory::GetUnitInfo()
{
	return unitInfo;
}

// -------------------------------------------------------------
// -------------------------------------------------------------

bool j1EntityFactory::Load(pugi::xml_node& save)
{
	bool ret = true;

	pugi::xml_node node;

	list<Entity*>::const_iterator it = activeEntities.begin();

	while (it != activeEntities.end()) {
		// Load the entities
		it++;
	}

	return ret;
}

bool j1EntityFactory::Save(pugi::xml_node& save) const
{
	bool ret = true;

	pugi::xml_node node;

	list<Entity*>::const_iterator it = activeEntities.begin();

	while (it != activeEntities.end()) {
		// Save the entities
		it++;
	}

	return ret;
}