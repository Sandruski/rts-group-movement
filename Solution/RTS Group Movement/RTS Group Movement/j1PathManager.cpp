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

	list<PathPlanner*>::const_iterator currSearch = searchRequests.begin();

	while (timer.ReadMs() < msSearchPerUpdate && searchRequests.size() > 0) {

		// Make one search cycle of this path request
		PathfindingStatus result = (*currSearch)->CycleOnce();

		// If the search has terminated, remove it from the list
		if (result == PathfindingStatus_PathFound || result == PathfindingStatus_PathNotFound
			|| result == PathfindingStatus_TileFound || result == PathfindingStatus_TileNotFound) {
			currSearch = searchRequests.erase(currSearch);
		}
		else {
			// Move on to the next search
			currSearch++;
		}

		// The iterator may now be pointing to the end of the list.
		// If so, it must be reset to the beginning
		if (currSearch == searchRequests.end())
			currSearch = searchRequests.begin();
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

PathPlanner::PathPlanner(Entity* owner) :entity(owner) 
{
	trigger = new FindActiveTrigger();

	Unit* u = (Unit*)entity;
	walkabilityMap = u->walkabilityMap;
}

PathPlanner::~PathPlanner()
{
	if (currentSearch != nullptr)
		delete currentSearch;
	currentSearch = nullptr;

	if (trigger != nullptr)
		delete trigger;
	trigger = nullptr;

	entity = nullptr;
}

bool PathPlanner::RequestAStar(iPoint origin, iPoint destination)
{
	bool ret = false;

	App->pathmanager->UnRegister(this);
	GetReadyForNewSearch();

	pathfindingAlgorithmType = PathfindingAlgorithmType_AStar;

	Unit* u = (Unit*)entity;
	u->isSearchComplete = false;

	currentSearch = new j1PathFinding();

	// Set the walkability map
	ret = walkabilityMap->SetWalkabilityMap(currentSearch);

	// Invalidate if origin or destination are non-walkable
	if (ret)
		ret = currentSearch->InitializeAStar(origin, destination);

	if (ret)
		App->pathmanager->Register(this);

	return ret;
}

bool PathPlanner::RequestDijkstra(iPoint origin, iPoint destination) 
{
	bool ret = true;

	App->pathmanager->UnRegister(this);
	GetReadyForNewSearch();

	pathfindingAlgorithmType = PathfindingAlgorithmType_Dijkstra;

	Unit* u = (Unit*)entity;
	u->isSearchComplete = false;

	currentSearch = new j1PathFinding();

	walkabilityMap->SetWalkabilityMap(currentSearch);

	// Invalidate if origin is non-walkable
	if (ret)
		ret = currentSearch->InitializeDijkstra(origin);

	if (ret)
		App->pathmanager->Register(this);
	
	return ret;
}

void PathPlanner::GetReadyForNewSearch()
{
	// Clear the waypoint list of the path (A Star)
	path.clear();
	// Clear the last tile found (Dijkstra)
	tile = { 0,0 };

	pathfindingAlgorithmType = PathfindingAlgorithmType_NoType;

	// Delete any active search
	if (currentSearch != nullptr)
		delete currentSearch;
	currentSearch = nullptr;
}

PathfindingStatus PathPlanner::CycleOnce()
{
	PathfindingStatus result;

	switch (pathfindingAlgorithmType) {

	case PathfindingAlgorithmType_AStar:

		result = currentSearch->CycleOnceAStar();

		// Let the bot know of the failure to find a path
		if (result == PathfindingStatus_PathNotFound) {
			// ERROR!
		}
		// Let the bot know a path has been found
		else if (result == PathfindingStatus_PathFound) {
			path = *currentSearch->GetLastPath();
			Unit* u = (Unit*)entity;
			u->isSearchComplete = true;
		}

		break;

	case PathfindingAlgorithmType_Dijkstra:

		result = currentSearch->CycleOnceDijkstra();

		{
			iPoint tile = currentSearch->GetLastTile();

			if (trigger->isSatisfied(tile, entity)) {

				this->tile = tile;
				result = PathfindingStatus_TileFound;
			}
		}

		// Let the bot know of the failure to find a tile
		if (result == PathfindingStatus_TileNotFound) {
			// ERROR!
		}
		// Let the bot know a tile has been found
		else if (result == PathfindingStatus_TileFound) {
			Unit* u = (Unit*)entity;
			u->isSearchComplete = true;
		}

		break;
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

// FindActiveTrigger class ---------------------------------------------------------------------------------

bool FindActiveTrigger::isSatisfied(iPoint tile, Entity* entity) 
{

	bool isSatisfied = false;

	Unit* u = (Unit*)entity;

	if (App->movement->IsValidTile(u->singleUnit, tile, u->singleUnit->checkEverything, u->singleUnit->checkEverything, true))
		return isSatisfied;
}

// WalkabilityMap struct ---------------------------------------------------------------------------------

bool WalkabilityMap::CreateWalkabilityMap()
{
	return App->map->CreateWalkabilityMap(w, h, &data);
}

bool WalkabilityMap::SetWalkabilityMap(j1PathFinding* currentSearch) const
{
	if (currentSearch == nullptr)
		return false;

	currentSearch->SetMap(w, h, data);

	return true;
}