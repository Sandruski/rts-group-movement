#include "Defs.h"
#include "p2Log.h"

#include "j1App.h"
#include "j1PathFinding.h"
#include "j1PathManager.h"

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

void j1PathManager::UpdateSearches() 
{
	// RESTRICT THE PATH MANAGER TO A SPECIFIC AMOUNT OF TIME!

	int numSearchCyclesRemaining = numSearchCyclesPerUpdate;

	// Iterate through the search requests until either all requests have been
	// fulfilled or there are no search cycles remaining for this update step
	
	list<PathPlanner*>::const_iterator currPath = searchRequests.begin();
	list<PathPlanner*>::const_iterator auxCurrPath;

	while (numSearchCyclesRemaining-- && searchRequests.size() > 0) {

		// Make one search cycle of this path request
		int result = (*currPath)->CycleOnce();

		// If the search has terminated, remove this path from the list
		if (result == PathfindingStatus_PathFound || result == PathfindingStatus_PathNotFound) {
			auxCurrPath = currPath++;
			searchRequests.erase(currPath);
			currPath = auxCurrPath;
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

PathfindingStatus PathPlanner::CycleOnce() const
{
	PathfindingStatus result = currentSearch->CycleOnce();

	// Let the bot know of the failure to find a path
	if (result == PathfindingStatus_PathNotFound) {

	}
	// Let the bot know a path has been found
	else if (result == PathfindingStatus_PathFound) {

	}

	return result;
}