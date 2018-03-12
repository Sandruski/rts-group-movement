#ifndef __j1PATH_MANAGER_H__
#define __j1PATH_MANAGER_H__

#include "j1Module.h"
#include "p2Point.h"

#include <list>
#include <vector>
#include <algorithm>
using namespace std;

class Entity;
class PathPlanner;

class j1PathManager : public j1Module
{
public:

	j1PathManager(uint numSearchCyclesPerUpdate);

	// Destructor
	~j1PathManager();

	// Called before quitting
	bool CleanUp();

	bool Update(float dt);

	// every time this is called, the total amount of search cycles available will be shared out
	// equally between all the active path requests. If a search completes successfully or fails,
	// the method will notify the relevant bot
	void UpdateSearches(); 

	// a path planner should call this method to register a search with the manager
	// (this method checks to ensure the path planner is only registered once)
	void Register(PathPlanner* pathPlanner);

	// an agent can use this method to remove a search request
	void UnRegister(PathPlanner* pathPlanner);

private:

	list<PathPlanner*> searchRequests; // a container of all the active search requests
	
	// total number of search cycles allocated to the manager
	// each update step these are divided equally among all registered path requests
	uint numSearchCyclesPerUpdate = 0;
};

class PathPlanner 
{
public:

	PathPlanner(Entity* owner);

	// Creates an instance of the A* time-sliced search and registers it with the path manager
	bool RequestPathToPosition(iPoint goal);

	// The PathManager calls this to iterate once though the search cycle of the currently
	// asigned search algorithm. When a search is terminated the method messages the owner
	PathfindingStatus CycleOnce() const;

	// Called by an agent after it has been notified that a search has terminated successfully
	vector<iPoint> GetPath();

private:

	Entity* entity; // a pointer to the owner of this class
	iPoint goal = { 0,0 }; // destination that the entity wishes to plan a path to reach

	// GetClosestNodeToPosition
	// a local reference to the navgraph

	vector<iPoint> path;

	j1PathFinding* currentSearch = nullptr;
};

#endif //__j1PATH_MANAGER_H__