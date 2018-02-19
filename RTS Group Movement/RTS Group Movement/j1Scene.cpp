#include "Defs.h"
#include "p2Log.h"

#include "j1App.h"

#include "j1Input.h"
#include "j1Textures.h"
#include "j1Collision.h"
#include "j1Render.h"
#include "j1Window.h"
#include "j1Map.h"
#include "j1Scene.h"
#include "j1Pathfinding.h"

#include "j1EntityFactory.h"
#include "Unit.h"

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
	// Load an orthogonal or isometric map, depending on the index of that map

	// Load maps

	return ret;
}

// Called before the first frame
bool j1Scene::Start()
{
	bool ret = true;

	// Camera
	App->win->GetWindowSize(width, height);
	scale = App->win->GetScale();

	// Load map
	if (App->map->Load("iso_walk.tmx"))
	{
		int w, h;
		uchar* data = NULL;
		if (App->map->CreateWalkabilityMap(w, h, &data))
			App->pathfinding->SetMap(w, h, data);

		RELEASE_ARRAY(data);
	}

	debug_tex = App->tex->Load("data/assets/textures/maps/path2.png");

	// Pathfinding collision data
	/*
	App->pathfinding->SetMap(App->map->data.width, App->map->data.height, (uchar*)App->map->collisionLayer->data);
	*/

	return ret;
}

// Called each loop iteration
bool j1Scene::PreUpdate()
{
	bool ret = true;

	int x, y;
	App->input->GetMousePosition(x, y);
	iPoint mousePos = App->render->ScreenToWorld(x, y);
	iPoint mouseTile = App->map->WorldToMap(mousePos.x, mousePos.y);
	iPoint mouseTilePos = App->map->MapToWorld(mouseTile.x, mouseTile.y);

	// Create a unit
	if (App->input->GetMouseButtonDown(SDL_BUTTON_RIGHT) == KEY_DOWN) {
		EntityInfo entityInfo;
		UnitInfo unitInfo;

		mouseTilePos.x += App->map->data.tile_width / 2;
		mouseTilePos.y += App->map->data.tile_height;
		entityInfo.pos = { (float)mouseTilePos.x,(float)mouseTilePos.y };
		entityInfo.size = { 16,16 };

		unitInfo.color = ColorBloodyRed;

		App->entities->AddUnit(entityInfo, unitInfo);
	}


	// Debug pathfinding
	/*
	static iPoint origin;
	static bool origin_selected = false;

	iPoint p = App->render->ScreenToWorld(x, y);
	p = App->map->WorldToMap(p.x, p.y);

	if (App->input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN)
	{
		if (origin_selected == true)
		{
			App->pathfinding->CreatePath(origin, p, DISTANCE_TO);
			origin_selected = false;
		}
		else
		{
			origin = p;
			origin_selected = true;
		}
	}
	*/

	return ret;
}

// Called each loop iteration
bool j1Scene::Update(float dt)
{
	bool ret = true;

	int x, y;
	App->input->GetMousePosition(x, y);
	iPoint mousePos = App->render->ScreenToWorld(x, y);
	iPoint mouseTile = App->map->WorldToMap(mousePos.x, mousePos.y);
	iPoint mouseTilePos = App->map->MapToWorld(mouseTile.x, mouseTile.y);

	// Draw map
	App->map->Draw();
	App->entities->Draw();

	// Move screen
	if (App->input->GetKey(SDL_SCANCODE_UP) == KEY_REPEAT)
		App->render->camera.y += 10;

	if (App->input->GetKey(SDL_SCANCODE_DOWN) == KEY_REPEAT)
		App->render->camera.y -= 10;

	if (App->input->GetKey(SDL_SCANCODE_LEFT) == KEY_REPEAT)
		App->render->camera.x += 10;

	if (App->input->GetKey(SDL_SCANCODE_RIGHT) == KEY_REPEAT)
		App->render->camera.x -= 10;

	// Draw the tile under the mouse
	App->render->Blit(debug_tex, mouseTilePos.x, mouseTilePos.y);

	const vector<iPoint>* path = App->pathfinding->GetLastPath();

	for (uint i = 0; i < path->size(); ++i)
	{
		iPoint pos = App->map->MapToWorld(path->at(i).x, path->at(i).y);
		App->render->Blit(debug_tex, pos.x, pos.y);
	}

	// Rectangle drawing and selection of units
	if (App->input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_DOWN)
		startRectangle = mousePos;
	else if (App->input->GetMouseButtonDown(SDL_BUTTON_LEFT) == KEY_REPEAT) {

		// Draw the rectangle
		int width = mousePos.x - startRectangle.x;
		int height = mousePos.y - startRectangle.y;
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

	// Debug pathfinding ------------------------------

	// F1, F2, F3, F4, F5, F6, +, -
	DebugKeys();

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

	App->map->UnLoad();

	// Set to nullptr the pointers to the UI elements

	return ret;
}

// Debug keys
void j1Scene::DebugKeys()
{
	// F1: start from the beginning of the first level
	if (App->input->GetKey(SDL_SCANCODE_F1) == KEY_DOWN) {
		/*
		if (index == 0)
			App->entities->playerData->position = App->entities->playerData->start_pos;
		else
			index = 0;

		App->fade->FadeToBlack(this, this, FADE_LESS_SECONDS, fades::slider_fade);
		*/
	}

	// F2: start from the beginning of the current level
	if (App->input->GetKey(SDL_SCANCODE_F2) == KEY_DOWN) {
		/*
		App->fade->FadeToBlack(this, this, FADE_LESS_SECONDS, fades::slider_fade);
		*/
	}

	// F4: change between maps
	if (App->input->GetKey(SDL_SCANCODE_F4) == KEY_DOWN) {
		/*
		if (index == 0)
			index = 1;
		else
			index = 0;

		App->fade->FadeToBlack(this, this, FADE_LESS_SECONDS, fades::slider_fade);
		*/
	}

	// F5: save the current state
	if (App->input->GetKey(SDL_SCANCODE_F5) == KEY_DOWN) {
		App->SaveGame();
	}

	// F6: load the previous state
	if (App->input->GetKey(SDL_SCANCODE_F6) == KEY_DOWN) {
		App->LoadGame();
	}

	// F7: fullscreen
	if (App->input->GetKey(SDL_SCANCODE_F7) == KEY_DOWN) {
		if (App->win->fullscreen) {
			App->win->fullscreen = false;
			SDL_SetWindowFullscreen(App->win->window, SDL_WINDOW_SHOWN);
		}
		else {
			App->win->fullscreen = true;
			SDL_SetWindowFullscreen(App->win->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
	}
}

/*
void j1Scene::OnUIEvent(UIElement* UIelem, UIEvents UIevent)
{
	switch (UIevent)
	{
	case UIEvents::MOUSE_ENTER_:

		break;

	case UIEvents::MOUSE_LEAVE_:

		break;

	case UIEvents::MOUSE_LEFT_CLICK_:

		break;

	case UIEvents::MOUSE_LEFT_UP_:

		break;
	}
}
*/

// -------------------------------------------------------------
// -------------------------------------------------------------

// Save
bool j1Scene::Save(pugi::xml_node& save) const
{
	bool ret = true;

	/*
	if (save.child("gate") == NULL) {
	save.append_child("gate").append_attribute("opened") = gate;
	save.child("gate").append_attribute("fx") = fx;
	}
	else {
	save.child("gate").attribute("opened") = gate;
	save.child("gate").attribute("fx") = fx;
	}
	*/

	return ret;
}

// Load
bool j1Scene::Load(pugi::xml_node& save)
{
	bool ret = true;

	/*
	if (save.child("gate") != NULL) {
	gate = save.child("gate").attribute("opened").as_bool();
	fx = save.child("gate").attribute("fx").as_bool();
	}
	*/

	return ret;
}