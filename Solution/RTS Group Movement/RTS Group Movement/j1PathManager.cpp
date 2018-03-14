#include "Defs.h"
#include "p2Log.h"

#include "j1App.h"
#include "j1PathFinding.h"
#include "j1PathManager.h"
#include "j1Map.h"
#include "j1Movement.h"

#include "Entity.h"
#include "Unit.h"

#include "Brofiler\Brofiler.h"

j1PathManager::j1PathManager(double msSearchPerUpdate) : j1Module(), msSearchPerUpdate(msSearchPerUpdate)
{
	name.assign("pathmanager");
}

// Destructor
j1PathManager::~j1PathManager()
{

}

// Called before quitting
bool j1PathManager::CleanUp()
{

	return true;
}

bool j1PathManager::Update(float dt) 
{
	bool ret = true;

	if (searchRequests.size() > 0)
		UpdateSearches();

	return ret;
}

void j1PathManager::UpdateSearches() 
{
	BROFILER_CATEGORY(__FUNCTION__, Profiler::Color::Orchid);

	timer.Start();

	list<PathPlanner*>::const_iterator currPath = searchRequests.begin();

	while (timer.ReadMs() < msSearchPerUpdate && searchRequests.size() > 0) {

		// Make one search cycle of this path request
		PathfindingStatus result = (*currPath)->CycleOnce();

		// If the search has terminated, remove this path from the list
		if (result == PathfindingStatus_PathFound || result == PathfindingStatus_PathNotFound) {
			currPath = searchRequests.erase(currPath);
		}
		else {
			// Move on to the next path
			currPath++;
		}

		// The iterator may now be pointing to the end of the list.
		// If so, it must be reset to the beginning
		if (currPath == searchRequests.end())
			currPath = searchRequests.begin();
	}
}

void j1PathManager::Register(PathPlanner* pathPlanner) 
{
	list<PathPlanner*>::const_iterator it = find(searchRequests.begin(), searchRequests.end(), pathPlanner);

	if (it == searchRequests.end())
		searchRequests.push_back(pathPlanner);
}

void j1PathManager::UnRegister(PathPlanner* pathPlanner) 
{
	searchRequests.remove(pathPlanner);
}

// ---------------------------------------------------------------------
// PATH PLANNER
// ---------------------------------------------------------------------

PathPlanner::PathPlanner(Entity* owner) :entity(owner) {}

PathPlanner::~PathPlanner()
{
	if (currentSearch != nullptr)
		delete currentSearch;
	currentSearch = nullptr;

	entity = nullptr;
}

bool PathPlanner::RequestAStar(iPoint origin, iPoint destination)
{
	bool ret = false;

	App->pathmanager->UnRegister(this);
	GetReadyForNewSearch();

	Unit* u = (Unit*)entity;
	u->isPath = false;

	currentSearch = new j1PathFinding();

	// Set the walkability map
	int w, h;
	uchar* data = NULL;

	ret = App->map->CreateWalkabilityMap(w, h, &data);
	if (ret)
		currentSearch->SetMap(w, h, data);

	RELEASE_ARRAY(data);

	// Invalidate if origin or destination are non-walkable
	ret = currentSearch->Initialize(origin, destination);

	if (ret)
		App->pathmanager->Register(this);

	return ret;
}

bool PathPlanner::RequestDijkstra(iPoint origin, iPoint destination) 
{
	bool ret = true;

	return ret;
}

void PathPlanner::GetReadyForNewSearch()
{
	// Clear the waypoint list and delete any active search
	path.clear();

	if (currentSearch != nullptr)
		delete currentSearch;
	currentSearch = nullptr;
}

PathfindingStatus PathPlanner::CycleOnce()
{
	PathfindingStatus result = currentSearch->CycleOnce();

	// Let the bot know of the failure to find a path
	if (result == PathfindingStatus_PathNotFound) {

	}
	// Let the bot know a path has been found
	else if (result == PathfindingStatus_PathFound) {
		path = *currentSearch->GetLastPath();
		Unit* u = (Unit*)entity;
		u->isPath = true;
	}

	return result;
}

vector<iPoint> PathPlanner::GetAStarPath() const
{
	return path;
}

iPoint PathPlanner::GetDijkstraTile() const 
{
	return tile;
}