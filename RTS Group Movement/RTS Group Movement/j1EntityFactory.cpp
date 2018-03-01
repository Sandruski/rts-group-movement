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

UnitInfo& j1EntityFactory::GetUnitInfo() 
{
	return unitInfo;
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

	// TODO: redefine blit order

	// Draw map
	//App->map->Draw();

	// Draw entities
	/*
	for (uint i = 0; i < MAX_ENTITIES; ++i)
		if (entities[i] != nullptr) {
			
			if (entities[i]->type == EntityType_Unit)
				entities[i]->Draw(CatPeasantTex);
			
		}
	*/

	// Draw above layer
	//App->map->DrawAboveLayer();

	return ret;
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

void j1EntityFactory::SelectEntity(iPoint tile)
{
	list<Entity*>::const_iterator it = activeEntities.begin();

	while (it != activeEntities.end()) {

		iPoint entityTile = App->map->WorldToMap((*it)->entityInfo.pos.x, (*it)->entityInfo.pos.y);

		if (tile.x == entityTile.x && tile.y == entityTile.y) {

			// If the entity isn't in the unitsSelected list, add it
			if (find(unitsSelected.begin(), unitsSelected.end(), *it) == unitsSelected.end()) {
				unitsSelected.push_back(*it);
				(*it)->isSelected = true;
			}
		}
		else {

			// If the entity is in the unitsSelected list, remove it
			if (find(unitsSelected.begin(), unitsSelected.end(), *it) != unitsSelected.end()) {
				unitsSelected.remove(*it);
				(*it)->isSelected = false;
			}
		}
		it++;
	}
}

void j1EntityFactory::SelectEntitiesWithinRectangle(SDL_Rect rectangleRect)
{
	list<Entity*>::const_iterator it = activeEntities.begin();

	while (it != activeEntities.end()) {

		SDL_Rect entityRect = { (*it)->entityInfo.pos.x, (*it)->entityInfo.pos.y, (*it)->entityInfo.size.x, (*it)->entityInfo.size.y };

		// If the entity is within the selection:
		if (SDL_HasIntersection(&entityRect, &rectangleRect)) {

			// It there are less entities than MAX_ENTITIES_SELECTED selected:
			if (unitsSelected.size() < MAX_ENTITIES_SELECTED) {

				// If the entity isn't in the unitsSelected list, add it
				if (find(unitsSelected.begin(), unitsSelected.end(), *it) == unitsSelected.end()) {
					unitsSelected.push_back(*it);
					(*it)->isSelected = true;
				}
			}
		}
		else {

			// If the entity is in the unitsSelected list, remove it
			if (find(unitsSelected.begin(), unitsSelected.end(), *it) != unitsSelected.end()) {
				unitsSelected.remove(*it);
				(*it)->isSelected = false;
			}
		}
		it++;
	}
}

list<Entity*> j1EntityFactory::GetLastEntitiesSelected() const 
{
	return unitsSelected;
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

Unit* j1EntityFactory::AddUnit(const EntityInfo& entityInfo, uint priority)
{
	Unit* unit = new Unit(entityInfo, priority);
	toSpawnEntities.push_back(unit);

	return unit;
}

// -------------------------------------------------------------
// -------------------------------------------------------------

bool j1EntityFactory::Load(pugi::xml_node& save)
{
	bool ret = true;

	pugi::xml_node node;

	list<Entity*>::const_iterator it = activeEntities.begin();

	while (it != activeEntities.end()) {
		// MYTODO: Add some code here
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
		// MYTODO: Add some code here
		it++;
	}

	return ret;
}
