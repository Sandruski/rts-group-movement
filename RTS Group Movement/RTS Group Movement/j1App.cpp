#include <iostream> 

#include "Defs.h"
#include "p2Log.h"

#include "j1Window.h"
#include "j1Input.h"
#include "j1Render.h"
#include "j1Textures.h"
#include "j1Scene.h"
#include "j1Map.h"
#include "j1Collision.h"
#include "j1EntityFactory.h"
#include "j1Pathfinding.h"

#include "j1App.h"
#include "Brofiler\Brofiler.h"


// Constructor
j1App::j1App(int argc, char* args[]) : argc(argc), args(args)
{
	want_to_save = want_to_load = false;

	input = new j1Input();
	win = new j1Window();
	render = new j1Render();
	tex = new j1Textures();
	scene = new j1Scene();
	map = new j1Map();
	collision = new j1Collision();
	entities = new j1EntityFactory();
	pathfinding = new j1PathFinding();

	// Ordered for awake / Start / Update
	// Reverse order of CleanUp
	AddModule(input);
	AddModule(win);
	AddModule(tex);
	AddModule(map);
	AddModule(pathfinding);
	AddModule(entities);
	AddModule(collision);

	AddModule(scene);

	// render last to swap buffer
	AddModule(render);
}

// Destructor
j1App::~j1App()
{
	// release modules
	list<j1Module*>::reverse_iterator item;
	item = modules.rbegin();

	while (item != modules.rend())
	{
		delete *item;
		item++;
	}
	modules.clear();
}

void j1App::AddModule(j1Module* module)
{
	module->Init();
	modules.push_back(module);
}

// Called before render is available
bool j1App::Awake()
{
	pugi::xml_document	config_file;
	pugi::xml_node		config;
	pugi::xml_node		app_config;

	bool ret = false;

	load_game = "save_game.xml";
	save_game = "save_game.xml";

	config = LoadConfig(config_file); //root node

	if (config.empty() == false)
	{
		// self-config
		ret = true;
		app_config = config.child("app");
		title.assign(app_config.child("title").child_value());
		organization.assign(app_config.child("organization").child_value());
		capFrames = config.child("renderer").child("capFrames").attribute("value").as_uint();
	}

	if (ret == true)
	{
		list<j1Module*>::const_iterator item;
		item = modules.begin();

		while (item != modules.end() && ret == true)
		{
			ret = (*item)->Awake(config.child((*item)->name.data()));
			item++;
		}
	}

	return ret;
}

// Called before the first frame
bool j1App::Start()
{
	bool ret = true;
	list<j1Module*>::const_iterator item;
	item = modules.begin();

	while (item != modules.end() && ret == true)
	{
		if ((*item)->active)
			ret = (*item)->Start();

		item++;
	}

	return ret;
}

// Called each loop iteration
bool j1App::Update()
{
	bool ret = true;
	PrepareUpdate();

	if (input->GetWindowEvent(WE_QUIT) == true || quit_game)
		ret = false;

	if (ret == true)
		ret = PreUpdate();

	if (ret == true)
		ret = DoUpdate();

	if (ret == true)
		ret = PostUpdate();

	FinishUpdate();
	return ret;
}

// ---------------------------------------------
pugi::xml_node j1App::LoadConfig(pugi::xml_document& config_file) const
{
	pugi::xml_node ret;

	pugi::xml_parse_result result = config_file.load_file("config.xml");

	if (result == NULL)
		LOG("Could not load map xml file config.xml. pugi error: %s", result.description());
	else
		ret = config_file.child("config");

	return ret;
}

// ---------------------------------------------
void j1App::PrepareUpdate()
{
	perfClock.Start();
}

// ---------------------------------------------
void j1App::FinishUpdate()
{
	if (want_to_save == true)
		SavegameNow();

	if (want_to_load == true)
		LoadGameNow();

	float mseconds_since_startup = clock.Read();

	uint32 actual_frame_ms = perfClock.ReadMs();

	last_frame_ms = actual_frame_ms;

	uint32 frames_on_last_update = 0;
	frame_count++;

	/*
	if (App->entities->playerData != nullptr) {
		if (App->input->GetKey(SDL_SCANCODE_F11) == KEY_DOWN
			&& (App->entities->playerData->animationPlayer == &App->entities->playerData->player.idle || App->entities->playerData->animationPlayer == &App->entities->playerData->player.idle2)
			&& !App->menu->active)
			toCap = !toCap;
	}
	else
		if (App->input->GetKey(SDL_SCANCODE_F11) == KEY_DOWN && !App->menu->active)
			toCap = !toCap;
	*/

	// Cap frames
	if (!App->render->vsync && toCap) {
		float toVsync = 1000 / capFrames;

		if (actual_frame_ms < toVsync)
			SDL_Delay(toVsync - actual_frame_ms);
	}

	double fps = 1000.0f / perfClock.ReadMs();

	double avgFPS = (float)frame_count / clock.ReadSec();

	dt = 1.0f / fps;

	string capOnOff;
	if (toCap)
		capOnOff = "on";
	else
		capOnOff = "off";

	string vsyncOnOff;
	if (App->render->vsync)
		vsyncOnOff = "on";
	else
		vsyncOnOff = "off";

	// Mouse position
	int x, y;
	App->input->GetMousePosition(x, y);
	iPoint mouseTile = App->map->WorldToMap(x - App->render->camera.x, y - App->render->camera.y);

	static char title[256];

	sprintf_s(title, 256, "FPS: %.2f | AvgFPS: %.2f | Last Frame Ms: %02u | capFrames: %s | Vsync: %s | Mouse: %d,%d",
		fps, avgFPS, actual_frame_ms, capOnOff.data(), vsyncOnOff.data(), mouseTile.x, mouseTile.y);

	App->win->SetTitle(title);
}

// Call modules before each loop iteration
bool j1App::PreUpdate()
{
	bool ret = true;
	list<j1Module*>::const_iterator item;
	item = modules.begin();
	j1Module* pModule = NULL;

	for (item = modules.begin(); item != modules.end() && ret == true; ++item)
	{
		pModule = *item;

		if (pModule->active == false) {
			continue;
		}

		ret = (*item)->PreUpdate();
	}

	return ret;
}

// Call modules on each loop iteration
bool j1App::DoUpdate()
{
	bool ret = true;
	list<j1Module*>::const_iterator item;
	item = modules.begin();

	j1Module* pModule = NULL;

	for (item = modules.begin(); item != modules.end() && ret == true; ++item)
	{
		pModule = *item;

		if (pModule->active == false) {
			continue;
		}

		ret = (*item)->Update(dt);
	}

	return ret;
}

// Call modules after each loop iteration
bool j1App::PostUpdate()
{
	BROFILER_CATEGORY("PostUpdate", Profiler::Color::LightSeaGreen);

	bool ret = true;
	list<j1Module*>::const_iterator item;
	j1Module* pModule = NULL;

	for (item = modules.begin(); item != modules.end() && ret == true; ++item)
	{
		pModule = *item;

		if (pModule->active == false) {
			continue;
		}

		ret = (*item)->PostUpdate();
	}

	return ret;
}

// Called before quitting
bool j1App::CleanUp()
{
	bool ret = true;
	list<j1Module*>::reverse_iterator item;
	item = modules.rbegin();

	while (item != modules.rend() && ret == true)
	{
		ret = (*item)->CleanUp();
		item++;
	}

	return ret;
}

// ---------------------------------------
int j1App::GetArgc() const
{
	return argc;
}

// ---------------------------------------
const char* j1App::GetArgv(int index) const
{
	if (index < argc)
		return args[index];
	else
		return NULL;
}

// ---------------------------------------
const char* j1App::GetTitle() const
{
	return title.data();
}

// ---------------------------------------
const char* j1App::GetOrganization() const
{
	return organization.data();
}

// Load / Save
void j1App::LoadGame()
{
	// we should be checking if that file actually exist
	// from the "GetSaveGames" list

	want_to_load = true;
}

// ---------------------------------------
void j1App::SaveGame() const
{
	// we should be checking if that file actually exist
	// from the "GetSaveGames" list ... should we overwrite ?

	want_to_save = true;
}

// ---------------------------------------
void j1App::GetSaveGames(list<string>& list_to_fill) const
{
	// need to add functionality to file_system module for this to work
}

bool j1App::LoadGameNow()
{
	bool ret = false;

	pugi::xml_document data;
	pugi::xml_node root;

	pugi::xml_parse_result result = data.load_file(load_game.data());

	if (result != NULL)
	{
		LOG("Loading new Game State from %s...", load_game.data());

		root = data.child("game_state");

		list<j1Module*>::const_iterator item = modules.begin();
		ret = true;

		while (item != modules.end() && ret == true)
		{
			ret = (*item)->Load(root.child((*item)->name.data()));
			item++;
		}

		data.reset();
		if (ret == true)
			LOG("...finished loading");
		else
			LOG("...loading process interrupted with error on module %s", (*item) ? (*item)->name.data() : "unknown");
	}
	else
		LOG("Could not parse game state xml file %s. pugi error: %s", load_game.data(), result.description());

	want_to_load = false;
	return ret;
}

bool j1App::SavegameNow() const
{
	bool ret = true;

	LOG("Saving Game State to %s...", save_game.data());

	// xml object were we will store all data
	pugi::xml_document data;
	pugi::xml_node root;

	root = data.append_child("game_state");

	list<j1Module*>::const_iterator item = modules.begin();

	while (item != modules.end() && ret == true)
	{
		ret = (*item)->Save(root.append_child((*item)->name.data()));
		item++;
	}

	if (ret == true)
	{
		data.save_file(save_game.data());
		LOG("... finished saving", );
	}
	else
		LOG("Save process halted from an error in module %s", (*item) ? (*item)->name.data() : "unknown");

	data.reset();
	want_to_save = false;
	return ret;
}