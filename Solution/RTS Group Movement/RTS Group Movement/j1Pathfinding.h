#ifndef __j1PATHFINDING_H__
#define __j1PATHFINDING_H__

#include "j1Module.h"
#include "p2Point.h"

#include <list>
#include <vector>
#include <algorithm>
using namespace std;

#define DEFAULT_PATH_LENGTH 50
#define INVALID_WALK_CODE -1

enum DistanceHeuristic {

	DistanceHeuristic_DistanceTo,
	DistanceHeuristic_DistanceNoSqrt,
	DistanceHeuristic_DistanceManhattan
};

enum PathfindingStatus {

	PathfindingStatus_PathFound,
	PathfindingStatus_PathNotFound,
	PathfindingStatus_SearchIncomplete
};

// --------------------------------------------------
// Recommended reading:
// Intro: http://www.raywenderlich.com/4946/introduction-to-a-pathfinding
// Details: http://theory.stanford.edu/~amitp/GameProgramming/
// --------------------------------------------------

class FindActiveTrigger;

// forward declaration
struct PathList;

// ---------------------------------------------------------------------
// Pathnode: Helper struct to represent a node in the path creation
// ---------------------------------------------------------------------
struct PathNode
{
	// Convenient constructors
	PathNode();
	PathNode(float g, float h, const iPoint& pos, const PathNode* parent, const bool diagonal);
	PathNode(const PathNode& node);

	// Fills a list (PathList) of all valid adjacent pathnodes
	uint FindWalkableAdjacents(PathList& list_to_fill) const;
	// Calculates this tile score
	float Score() const;
	// Calculate the F for a specific destination tile
	float CalculateF(bool h = false, const iPoint& destination = { -1,-1 }, DistanceHeuristic distanceHeuristic = DistanceHeuristic_DistanceManhattan);

	// -----------
	float g = 0;
	float h = 0;
	iPoint pos = { 0, 0 };
	const PathNode* parent; // needed to reconstruct the path in the end
	bool diagonal = false;
};

// ---------------------------------------------------------------------
// Helper struct to include a list of path nodes
// ---------------------------------------------------------------------
struct PathList
{
	PathList() {}

	// Looks for a node in this list and returns it's list node or NULL
	const PathNode* Find(const iPoint& point) const;

	// Returns the Pathnode with lowest score in this list or NULL if empty
	const PathNode* GetNodeLowestScore() const;

	// -----------
	// The list itself
	list<PathNode> pathNodeList;
};

// Utility: calculate a specific distance
int CalculateDistance(iPoint origin, iPoint destination, DistanceHeuristic distanceHeuristic = DistanceHeuristic_DistanceManhattan);

// ---------------------------------------------------------------------

class j1PathFinding : public j1Module
{
public:

	j1PathFinding();

	// Destructor
	~j1PathFinding();

	// Called before quitting
	bool CleanUp();

	// Sets up the walkability map
	void SetMap(uint width, uint height, uchar* data);

	// Main function to request a path from A to B
	int CreatePath(const iPoint& origin, const iPoint& destination, DistanceHeuristic distanceHeuristic = DistanceHeuristic_DistanceManhattan);

	// To create the path once finished the search algorithm
	int BacktrackToCreatePath();

	// To request all tiles involved in the last generated path
	const vector<iPoint>* GetLastPath() const;

	// To request the last tile checked by the search algorithm
	iPoint GetLastTile() const;

	// Utility: return true if pos is inside the map boundaries
	bool CheckBoundaries(const iPoint& pos) const;

	// Utility: returns true is the tile is walkable
	bool IsWalkable(const iPoint& pos) const;

	// Utility: return the walkability value of a tile
	int GetTileAt(const iPoint& pos) const;

	// Initialize CycleOnceAStar
	bool InitializeAStar(const iPoint& origin, const iPoint& destination, DistanceHeuristic distanceHeuristic = DistanceHeuristic_DistanceManhattan);

	// CycleOnce A Star
	PathfindingStatus CycleOnceAStar();

	// Initialize CycleOnceDijkstra
	bool InitializeDijkstra(const iPoint& origin, FindActiveTrigger* trigger = nullptr, bool isPathRequested = false);

	// CycleOnce Dijkstra
	PathfindingStatus CycleOnceDijkstra();

private:

	uint width = 0; // size of the map (w)
	uint height = 0; // size of the map (h)
	uchar* walkabilityMap = nullptr; // all map walkability values [0..255]
	DistanceHeuristic distanceHeuristic = DistanceHeuristic_DistanceManhattan; // distance heuristic of choice

	PathList open; // open list of PathNodes
	PathList close; // close list of PathNodes
	vector<iPoint> last_path; // we store the created path here
	iPoint last_tile = { -1,-1 }; // we store the last tile checked here

	// A Star
	iPoint goal = { -1,-1 }; // destination tile

	// Dijkstra
	FindActiveTrigger* trigger = nullptr;
	bool isPathRequested = false;
};

#endif //__j1PATHFINDING_H__