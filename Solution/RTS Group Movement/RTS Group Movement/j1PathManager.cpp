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

j1PathManager::j1PathManager(uint numSearchCyclesPerUpdate) : j1Module(), numSearchCyclesPerUpdate(numSearchCyclesPerUpdate)
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

	UpdateSearches();

	return ret;
}

void j1PathManager::UpdateSearches() 
{
	// RESTRICT THE PATH MANAGER TO A SPECIFIC AMOUNT OF TIME!

	int numSearchCyclesRemaining = numSearchCyclesPerUpdate;

	// Iterate through the search requests until either all requests have been
	// fulfilled or there are no search cycles remaining for this update step
	
	list<PathPlanner*>::const_iterator currPath = searchRequests.begin();

	while (numSearchCyclesRemaining-- && searchRequests.size() > 0) {

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

bool PathPlanner::RequestPathToTarget(iPoint goal) 
{
	bool ret = true;

	GetReadyForNewSearch();

	Unit* u = (Unit*)entity;
	u->isPath = false;
	// GetClosestNodeToPosition
	// invalidate if origin or destination are non-walkable

	currentSearch = new j1PathFinding();

	int w, h;
	uchar* data = NULL;
	if (App->map->CreateWalkabilityMap(w, h, &data))
		currentSearch->SetMap(w, h, data);

	RELEASE_ARRAY(data);

	currentSearch->Initialize(u->singleUnit->currTile, goal);

	App->pathmanager->Register(this);

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

vector<iPoint> PathPlanner::GetPath() 
{
	return path;
}