#include "Defs.h"
#include "p2Log.h"

#include "j1App.h"

#include "j1Input.h"
#include "j1Textures.h"
#include "j1Render.h"
#include "j1Window.h"
#include "j1Map.h"
#include "j1Scene.h"
#include "j1Pathfinding.h"
#include "j1Movement.h"
#include "j1EntityFactory.h"
#include "j1Collision.h"

#include"Brofiler\Brofiler.h"

j1Scene::j1Scene() : j1Module()
{
	name.assign("scene");
}

// Destructor
j1Scene::~j1Scene()
{}

// Called before render is available
bool j1Scene::Awake(pugi::xml_node& config)
{
	bool ret = true;

	LOG("Loading scene");

	// Load maps
	pugi::xml_node maps = config.child("maps");

	warcraftMap = maps.child("warcraft").attribute("name").as_string();
	warcraftActive = maps.child("warcraft").attribute("active").as_bool();
	warcraftTexName = maps.child("warcraft").attribute("tex").as_string();

	return ret;
}

// Called before the first frame
bool j1Scene::Start()
{
	bool ret = false;

	// Save camera info
	App->win->GetWindowSize(width, height);
	scale = App->win->GetScale();

	// Load a warcraft-based map
	if (warcraftActive) {
		ret = App->map->Load(warcraftMap.data());
		debugTex = App->tex->Load(warcraftTexName.data());
	}

	// Create walkability map
	if (ret)
	{
		int w, h;
		uchar* data = NULL;
		if (App->map->CreateWalkabilityMap(w, h, &data))
			App->pathfinding->SetMap(w, h, data);

		RELEASE_ARRAY(data);
	}

	return ret;
}

// Called each loop iteration
bool j1Scene::PreUpdate()
{
	bool ret = true;

	// Save mouse position (world and map coords)
	int x, y;
	App->input->GetMousePosition(x, y);
	iPoint mousePos = App->render->ScreenToWorld(x, y);
	iPoint mouseTile = App->map->WorldToMap(mousePos.x, mousePos.y);
	iPoint mouseTilePos = App->map->MapToWorld(mouseTile.x, mouseTile.y);

	// ---------------------------------------------------------------------

	// Entities info
	/// Entity
	fPoint pos = { (float)mouseTilePos.x,(float)mouseTilePos.y };
	iPoint size = { App->map->data.tile_width,App->map->data.tile_height };
	uint maxLife = 30;
	int currLife = (int)maxLife;

	/// DynamicEntity
	UnitInfo unitInfo;
	unitInfo.maxSpeed = 50.0f;
	unitInfo.damage = 2;

	/// Footman
	FootmanInfo footmanInfo;

	/// Grunt
	GruntInfo gruntInfo;

	// Entities creation
	if (App->entities->IsEntityOnTile(mouseTile, EntityType_DynamicEntity) == nullptr && App->pathfinding->IsWalkable(mouseTile)) {

		// 1: spawn a Footman with priority 1
		unitInfo.sightRadius = 6;
		unitInfo.attackRadius = 3;

		if (App->input->GetKey(SDL_SCANCODE_1) == KEY_DOWN)
			App->entities->AddDynamicEntity(DynamicEntityType_Footman, pos, size, currLife, maxLife, unitInfo, (EntityInfo&)footmanInfo);

		// 2: spawn a Grunt with priority 1
		unitInfo.sightRadius = 4;
		unitInfo.attackRadius = 2;
		maxLife = 20;
		currLife = (int)maxLife;

		if (App->input->GetKey(SDL_SCANCODE_2) == KEY_DOWN)
			App->entities->AddDynamicEntity(DynamicEntityType_Grunt, pos, size, currLife, maxLife, unitInfo, (EntityInfo&)gruntInfo);
	}

	return ret;
}

// Called each loop iteration
bool j1Scene::Update(float dt)
{
	BROFILER_CATEGORY(__FUNCTION__, Profiler::Color::Orchid);

	bool ret = true;

	// Save mouse position (world and map coords)
	int x, y;
	App->input->GetMousePosition(x, y);
	iPoint mousePos = App->render->ScreenToWorld(x, y);
	iPoint mouseTile = App->map->WorldToMap(mousePos.x, mousePos.y);
	iPoint mouseTilePos = App->map->MapToWorld(mouseTile.x, mouseTile.y);

	// ---------------------------------------------------------------------

	if (App->input->GetKey(SDL_SCANCODE_I) == KEY_DOWN)
		isFrameByFrame = !isFrameByFrame;

	if (App->input->GetKey(SDL_SCANCODE_F1) == KEY_DOWN)
		debugDrawMovement = !debugDrawMovement;

	if (App->input->GetKey(SDL_SCANCODE_F2) == KEY_DOWN)
		debugDrawPath = !debugDrawPath;

	if (App->input->GetKey(SDL_SCANCODE_F3) == KEY_DOWN)
		debugDrawMap = !debugDrawMap;

	if (App->input->GetKey(SDL_SCANCODE_F4) == KEY_DOWN)
		debugDrawAttack = !debugDrawAttack;

	// Draw
	App->map->Draw(); // map

	App->entities->Draw(); // entities

	if (debugDrawAttack)
		App->collision->DebugDraw(); // debug draw collisions

	if (debugDrawMovement)
		App->movement->DebugDraw(); // debug draw movement

	App->render->Blit(debugTex, mouseTilePos.x, mouseTilePos.y); // tile under the mouse pointer

	// Select units by mouse click
	if (App->input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN) {
		startRectangle = mousePos;

		Entity* entity = App->entities->IsEntityOnTile(mouseTile);

		if (entity != nullptr)
			App->entities->SelectEntity(entity);
		else
			App->entities->UnselectAllEntities();
	}

	int width = mousePos.x - startRectangle.x;
	int height = mousePos.y - startRectangle.y;

	// Select units by rectangle drawing
	if (abs(width) >= RECTANGLE_MIN_AREA && abs(height) >= RECTANGLE_MIN_AREA && App->input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_REPEAT) {

		// Draw the rectangle
		SDL_Rect mouseRect = { startRectangle.x, startRectangle.y, width, height };
		App->render->DrawQuad(mouseRect, 255, 255, 255, 255, false);

		// Select units within the rectangle
		if (width < 0) {
			mouseRect.x = mousePos.x;
			mouseRect.w *= -1;
		}
		if (height < 0) {
			mouseRect.y = mousePos.y;
			mouseRect.h *= -1;
		}

		App->entities->SelectEntitiesWithinRectangle(mouseRect);
	}

	// Select a new goal for the selected units (single click or drag)
	if (App->input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_REPEAT) {

		if (App->movement->GetGroupByUnits(App->entities->GetLastUnitsSelected()) == nullptr)

			// Selected units will now behave as a group
			App->movement->CreateGroupFromUnits(App->entities->GetLastUnitsSelected());

		App->movement->GetGroupByUnits(App->entities->GetLastUnitsSelected())->DrawShapedGoal(mouseTile);
	}
	if (App->input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_UP) {

		if (App->movement->GetGroupByUnits(App->entities->GetLastUnitsSelected()) == nullptr)

			// Selected units will now behave as a group
			App->movement->CreateGroupFromUnits(App->entities->GetLastUnitsSelected());

		UnitGroup* group = App->movement->GetGroupByUnits(App->entities->GetLastUnitsSelected());
	
		if (!group->SetShapedGoal())
			group->SetGoal(mouseTile);
	}

	return ret;
}

// Called each loop iteration
bool j1Scene::PostUpdate()
{
	bool ret = true;

	if (App->input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN)
		ret = false;

	return ret;
}

// Called before quitting
bool j1Scene::CleanUp()
{
	bool ret = true;

	LOG("Freeing scene");

	// Unload the map
	App->map->UnLoad();

	// Free all textures
	App->tex->UnLoad(debugTex);

	return ret;
}

// -------------------------------------------------------------
// -------------------------------------------------------------

// Save
bool j1Scene::Save(pugi::xml_node& save) const
{
	bool ret = true;

	return ret;
}

// Load
bool j1Scene::Load(pugi::xml_node& save)
{
	bool ret = true;

	return ret;
}