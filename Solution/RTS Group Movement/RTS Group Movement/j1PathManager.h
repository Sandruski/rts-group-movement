#ifndef __j1PATH_MANAGER_H__
#define __j1PATH_MANAGER_H__

#include "j1Module.h"
#include "j1Movement.h"
#include "p2Point.h"

#include <list>
#include <vector>
#include <algorithm>
using namespace std;

enum PathfindingAlgorithmType {

	PathfindingAlgorithmType_NoType,
	PathfindingAlgorithmType_AStar,
	PathfindingAlgorithmType_Dijkstra,
	PathfindingAlgorithmType_MaxTypes
};

class Entity;
class PathPlanner;

// ---------------------------------------------------------------------
// Helper struct to set a walkability map
// ---------------------------------------------------------------------
struct Navgraph
{
public:

	// HighLevelNavgraph (used to quickly determine paths at the "room" level)
	// LowLevelNavgraph (used to determine paths at the "point" level)

	bool CreateNavgraph();

	bool SetNavgraph(j1PathFinding* currentSearch) const;

public:

	int w = 0, h = 0;
	uchar* data = nullptr;
};

// ---------------------------------------------------------------------
// Helper class to determine whether a condition for termination is fulfilled
// ---------------------------------------------------------------------
class FindActiveTrigger
{
public:

	enum ActiveTriggerType {

		ActiveTriggerType_NoType,
		ActiveTriggerType_Goal,
		ActiveTriggerType_Object,
		ActiveTriggerType_MaxTypes
	};

	FindActiveTrigger(ActiveTriggerType activeTriggerType, Entity* entity);
	FindActiveTrigger(ActiveTriggerType activeTriggerType, EntityType entityType);

	bool isSatisfied(iPoint tile) const;

private:

	ActiveTriggerType activeTriggerType = ActiveTriggerType_NoType;
	Entity* entity = nullptr;

public:

	EntityType entityType = EntityType_NoType;
	bool isCheckingCurrTile = false;
	bool isCheckingNextTile = false;
	bool isCheckingGoalTile = true;
};

// ---------------------------------------------------------------------

class j1PathManager : public j1Module
{
public:

	j1PathManager(double msSearchPerUpdate);

	// Destructor
	~j1PathManager();

	// Called before quitting
	bool CleanUp();

	bool Update(float dt);

	// Every time this is called, the total amount of search cycles available will be shared out
	// equally between all the active path requests. If a search completes successfully or fails,
	// the method will notify the relevant bot
	void UpdateSearches();

	// A path planner should call this method to register a search with the manager
	// (this method checks to ensure the path planner is only registered once)
	void Register(PathPlanner* pathPlanner);

	// An agent can use this method to remove a search request
	void UnRegister(PathPlanner* pathPlanner);

private:

	list<PathPlanner*> searchRequests; // a container of all the active search requests
	
	// total ms to spend on search cycles each update allocated to the manager
	// each update step these are divided equally among all registered path requests
	double msSearchPerUpdate = 0.0f;

	j1PerfTimer timer; // timer to keep track of the ms spent on each update
};

class PathPlanner
{
public:

	PathPlanner(Entity* owner, Navgraph& navgraph);

	~PathPlanner();

	// Creates an instance of the A* time-sliced search and registers it with the path manager
	bool RequestAStar(iPoint origin, iPoint destination);

	// Creates an instance of the Dijkstra time-sliced search and registers it with the path manager
	bool RequestDijkstra(iPoint origin, FindActiveTrigger::ActiveTriggerType activeTriggerType, bool isPathRequested = false);

	// Initializes all the PathPlanner variables for the next search
	void GetReadyForNewSearch();

	// The PathManager calls this to iterate once though the search cycle of the currently
	// asigned search algorithm. When a search is terminated the method messages the owner
	PathfindingStatus CycleOnce();

	// Called by an agent after it has been notified that the search algorithm has terminated successfully
	// to request the path found
	vector<iPoint> GetPath() const;

	// Called by an agent after it has been notified that the search algorithm (Dijkstra) has terminated successfully
	// to request the tile found
	iPoint GetTile() const;

	bool IsSearchCompleted() const;

	bool IsSearchRequested() const;

	void SetSearchRequested(bool isSearchRequested);

	void SetCheckingCurrTile(bool isCheckingCurrTile);
	void SetCheckingNextTile(bool isCheckingNextTile);
	void SetCheckingGoalTile(bool isCheckingGoalTile);

private:

	Entity* entity = nullptr; // a pointer to the owner of this class
	bool isSearchRequested = false;
	bool isSearchCompleted = false;
	bool isPathRequested = false;

	PathfindingAlgorithmType pathfindingAlgorithmType = PathfindingAlgorithmType_NoType;
	j1PathFinding* currentSearch = nullptr; // a pointer to the current search
	Navgraph& navgraph; // a local reference to the navgraph

	// Dijkstra
	FindActiveTrigger* trigger = nullptr; // a pointer to the FindActiveTrigger class
};

#endif //__j1PATH_MANAGER_H__