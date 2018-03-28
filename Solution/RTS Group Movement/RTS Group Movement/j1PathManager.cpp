#include "Defs.h"
#include "p2Log.h"

#include "j1App.h"
#include "j1PathFinding.h"
#include "j1PathManager.h"
#include "j1Map.h"
#include "j1Movement.h"

#include "Entity.h"

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
	bool ret = true;

	list<PathPlanner*>::const_iterator it = searchRequests.begin();

	while (it != searchRequests.end()) {
	
		delete *it;
		it++;
	}
	searchRequests.clear();

	return ret;
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
		if (result == PathfindingStatus_PathFound || result == PathfindingStatus_PathNotFound) {
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
	pathPlanner->SetSearchRequested(false);

	searchRequests.remove(pathPlanner);
}

// ---------------------------------------------------------------------
// PATH PLANNER
// ---------------------------------------------------------------------

PathPlanner::PathPlanner(Entity* owner, Navgraph& navgraph) :entity(owner), navgraph(navgraph) {}

PathPlanner::~PathPlanner()
{
	entity = nullptr;

	// Remove Current Search
	App->pathmanager->UnRegister(this);

	if (currentSearch != nullptr)
		delete currentSearch;
	currentSearch = nullptr;

	// Remove Trigger
	if (trigger != nullptr)
		delete trigger;
	trigger = nullptr;
}

bool PathPlanner::RequestAStar(iPoint origin, iPoint destination)
{
	bool ret = false;

	if (isSearchRequested)
		return false;

	App->pathmanager->UnRegister(this);
	GetReadyForNewSearch();

	pathfindingAlgorithmType = PathfindingAlgorithmType_AStar;

	currentSearch = new j1PathFinding();

	// Set the walkability map
	ret = navgraph.SetNavgraph(currentSearch);

	// Invalidate if origin or destination are non-walkable
	if (ret)
		ret = currentSearch->InitializeAStar(origin, destination);

	if (ret)
		App->pathmanager->Register(this);

	return ret;
}

bool PathPlanner::RequestDijkstra(iPoint origin, FindActiveTrigger::ActiveTriggerType activeTriggerType, bool isPathRequested)
{
	bool ret = true;

	if (isSearchRequested)
		return false;

	App->pathmanager->UnRegister(this);
	GetReadyForNewSearch();

	pathfindingAlgorithmType = PathfindingAlgorithmType_Dijkstra;

	this->isPathRequested = isPathRequested;

	currentSearch = new j1PathFinding();

	switch (activeTriggerType) {

	case FindActiveTrigger::ActiveTriggerType_Goal:

		trigger = new FindActiveTrigger(activeTriggerType, entity);

		break;

	case FindActiveTrigger::ActiveTriggerType_Object:

		trigger = new FindActiveTrigger(activeTriggerType, entity->entityType);

		break;

	case FindActiveTrigger::ActiveTriggerType_NoType:
	default:

		break;
	}

	navgraph.SetNavgraph(currentSearch);

	// Invalidate if origin is non-walkable
	if (ret)
		ret = currentSearch->InitializeDijkstra(origin, trigger, isPathRequested);

	if (ret)
		App->pathmanager->Register(this);
	
	return ret;
}

void PathPlanner::GetReadyForNewSearch()
{
	pathfindingAlgorithmType = PathfindingAlgorithmType_NoType;
	isSearchCompleted = false;
	isSearchRequested = true;

	// Delete any active search
	if (currentSearch != nullptr)
		delete currentSearch;
	currentSearch = nullptr;

	if (trigger != nullptr)
		delete trigger;
	trigger = nullptr;
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
		else if (result == PathfindingStatus_PathFound)

			isSearchCompleted = true;

		break;

	case PathfindingAlgorithmType_Dijkstra:

		result = currentSearch->CycleOnceDijkstra();

		// Let the bot know of the failure to find a path/tile
		if (result == PathfindingStatus_PathNotFound) {
			// ERROR!
		}
		// Let the bot know a path/tile has been found
		else if (result == PathfindingStatus_PathFound)

			isSearchCompleted = true;

		break;
	}

	return result;
}

vector<iPoint> PathPlanner::GetPath() const
{
	if (isSearchCompleted)

		if (pathfindingAlgorithmType == PathfindingAlgorithmType_AStar || (pathfindingAlgorithmType == PathfindingAlgorithmType_Dijkstra && isPathRequested))

			return *currentSearch->GetLastPath();
}

iPoint PathPlanner::GetTile() const
{
	if (isSearchCompleted)

		return currentSearch->GetLastTile();
}

bool PathPlanner::IsSearchCompleted() const
{
	return isSearchCompleted;
}

bool PathPlanner::IsSearchRequested() const
{
	return isSearchRequested;
}

void PathPlanner::SetSearchRequested(bool isSearchRequested)
{
	this->isSearchRequested = isSearchRequested;
}

void PathPlanner::SetCheckingCurrTile(bool isCheckingCurrTile) 
{
	if (trigger != nullptr)
		trigger->isCheckingCurrTile = isCheckingCurrTile;
}

void PathPlanner::SetCheckingNextTile(bool isCheckingNextTile) 
{
	if (trigger != nullptr)
		trigger->isCheckingNextTile = isCheckingNextTile;
}

void PathPlanner::SetCheckingGoalTile(bool isCheckingGoalTile) 
{
	if (trigger != nullptr)
		trigger->isCheckingGoalTile = isCheckingGoalTile;
}

// WalkabilityMap struct ---------------------------------------------------------------------------------

bool Navgraph::CreateNavgraph()
{
	return App->map->CreateWalkabilityMap(w, h, &data);
}

bool Navgraph::SetNavgraph(j1PathFinding* currentSearch) const
{
	if (currentSearch == nullptr)
		return false;

	currentSearch->SetMap(w, h, data);

	return true;
}

// FindActiveTrigger class ---------------------------------------------------------------------------------

FindActiveTrigger::FindActiveTrigger(ActiveTriggerType activeTriggerType, Entity* entity) :activeTriggerType(activeTriggerType), entity(entity) {}

FindActiveTrigger::FindActiveTrigger(ActiveTriggerType activeTriggerType, EntityType entityType) : activeTriggerType(activeTriggerType), entityType(entityType) {}

bool FindActiveTrigger::isSatisfied(iPoint tile) const
{
	bool isSatisfied = false;

	DynamicEntity* dynamicEntity = (DynamicEntity*)entity;

	switch (activeTriggerType) {

	case ActiveTriggerType_Goal:

		if (App->movement->IsValidTile(dynamicEntity->GetSingleUnit(), tile, isCheckingCurrTile, isCheckingNextTile, isCheckingGoalTile))
			isSatisfied = true;

		break;

	case ActiveTriggerType_Object:

		isSatisfied = true;

		break;

	case ActiveTriggerType_NoType:
	default:

		break;
	}

	return isSatisfied;
}