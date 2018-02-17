#ifndef __j1PATHFINDING_H__
#define __j1PATHFINDING_H__

#include "j1Module.h"
#include "p2Point.h"

#include <list>
#include <vector>
using namespace std;

#define DEFAULT_PATH_LENGTH 50
#define INVALID_WALK_CODE 1181
#define INVALID_WALK_CODES t != 1180 && t != 1181 && t != 1182 && t != 1183

enum Distance {
	DISTANCE_TO,
	DISTANCE_NO_SQRT,
	MANHATTAN
};

// --------------------------------------------------
// Recommended reading:
// Intro: http://www.raywenderlich.com/4946/introduction-to-a-pathfinding
// Details: http://theory.stanford.edu/~amitp/GameProgramming/
// --------------------------------------------------

struct PathNode;

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
	int CreatePath(const iPoint& origin, const iPoint& destination, Distance distance_type);

	// To request all tiles involved in the last generated path
	const vector<iPoint>* GetLastPath() const;

	// Utility: return true if pos is inside the map boundaries
	bool CheckBoundaries(const iPoint& pos) const;

	// Utility: returns true is the tile is walkable
	bool IsWalkable(const iPoint& pos) const;

	// Utility: return the walkability value of a tile
	int GetTileAt(const iPoint& pos) const;

private:

	// size of the map
	uint width = 0;
	uint height = 0;
	// all map walkability values [0..255]
	uchar* map = nullptr;
	// we store the created path here
	vector<iPoint> last_path;
};

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
	float CalculateF(const iPoint& destination, Distance distance_type);

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
	// Looks for a node in this list and returns it's list node or NULL
	const PathNode* Find(const iPoint& point) const;

	// Returns the Pathnode with lowest score in this list or NULL if empty
	const PathNode* GetNodeLowestScore() const;

	// -----------
	// The list itself
	list<PathNode*> pathList;
};

// Utility: calculate a specific distance
int CalculateDistance(iPoint origin, iPoint destination, Distance distance_type);



#endif //__j1PATHFINDING_H__