#include "Defs.h"
#include "p2Log.h"
#include "j1App.h"
#include "j1PathFinding.h"
#include "j1EntityFactory.h"
#include "j1Map.h"
#include "Brofiler\Brofiler.h"

j1PathFinding::j1PathFinding() : j1Module(), map(NULL), last_path(DEFAULT_PATH_LENGTH), width(0), height(0)
{
	name.assign("pathfinding");
}

// Destructor
j1PathFinding::~j1PathFinding()
{
	RELEASE_ARRAY(map);
}

// Called before quitting
bool j1PathFinding::CleanUp()
{
	LOG("Freeing pathfinding library");

	last_path.clear();
	RELEASE_ARRAY(map);
	return true;
}

// Sets up the walkability map
void j1PathFinding::SetMap(uint width, uint height, uchar* data)
{
	this->width = width;
	this->height = height;

	RELEASE_ARRAY(map);
	map = new uchar[width*height];
	memcpy(map, data, width*height);

}

// Utility: return true if pos is inside the map boundaries
bool j1PathFinding::CheckBoundaries(const iPoint& pos) const
{
	return (pos.x >= 0 && pos.x <= (int)width &&
		pos.y >= 0 && pos.y <= (int)height);
}

// Utility: returns true is the tile is walkable
bool j1PathFinding::IsWalkable(const iPoint& pos) const
{
	int t = GetTileAt(pos);
	return INVALID_WALK_CODES && t >= 0;
}

// Utility: return the walkability value of a tile
int j1PathFinding::GetTileAt(const iPoint& pos) const
{
	uint tile = App->map->collisionLayer->data[(pos.y*width) + pos.x];
	if (CheckBoundaries(pos))
		return tile;

	return INVALID_WALK_CODE;
}

// To request all tiles involved in the last generated path
const vector<iPoint>* j1PathFinding::GetLastPath() const
{
	return &last_path;
}

// PathList ------------------------------------------------------------------------
// Looks for a node in this list and returns it's list node or NULL
// ---------------------------------------------------------------------------------
const PathNode* PathList::Find(const iPoint& point) const
{
	list<PathNode*>::const_iterator item = pathList.begin();

	while (item != pathList.end())
	{
		if ((*item)->pos == point)
			return *item;
		item++;
	}
	
	return nullptr;
}

// PathList ------------------------------------------------------------------------
// Returns the Pathnode with lowest score in this list or NULL if empty
// ---------------------------------------------------------------------------------
const PathNode* PathList::GetNodeLowestScore() const
{
	const PathNode* ret = NULL;
	float min = INT_MAX;

	list<PathNode*>::const_reverse_iterator item = pathList.rbegin();

	while (item != pathList.rend())
	{
		if ((*item)->Score() < min)
		{
			min = (*item)->Score();
			ret = *item;
		}
		item++;
	}

	return ret;
}

// PathNode -------------------------------------------------------------------------
// Convenient constructors
// ----------------------------------------------------------------------------------
PathNode::PathNode() : g(-1), h(-1), pos(-1, -1), parent(NULL)
{}

PathNode::PathNode(float g, float h, const iPoint& pos, const PathNode* parent, const bool diagonal = false) : g(g), h(h), pos(pos), parent(parent), diagonal(diagonal)
{}

PathNode::PathNode(const PathNode& node) : g(node.g), h(node.h), pos(node.pos), parent(node.parent)
{}

// PathNode -------------------------------------------------------------------------
// Fills a list (PathList) of all valid adjacent pathnodes
// ----------------------------------------------------------------------------------
uint PathNode::FindWalkableAdjacents(PathList& list_to_fill) const
{
	iPoint cell;
	uint before = list_to_fill.pathList.size();

	cell.create(pos.x, pos.y + 1);
	if (App->pathfinding->IsWalkable(cell))
		list_to_fill.pathList.push_back(&PathNode(-1, -1, cell, this));

	// south
	cell.create(pos.x, pos.y - 1);
	if (App->pathfinding->IsWalkable(cell))
		list_to_fill.pathList.push_back(&PathNode(-1, -1, cell, this));

	// east
	cell.create(pos.x + 1, pos.y);
	if (App->pathfinding->IsWalkable(cell))
		list_to_fill.pathList.push_back(&PathNode(-1, -1, cell, this));

	// west
	cell.create(pos.x - 1, pos.y);
	if (App->pathfinding->IsWalkable(cell))
		list_to_fill.pathList.push_back(&PathNode(-1, -1, cell, this));

	// north-west
	cell.create(pos.x + 1, pos.y - 1);
	if (App->pathfinding->IsWalkable(cell))
		list_to_fill.pathList.push_back(&PathNode(-1, -1, cell, this, true));

	// south-west
	cell.create(pos.x - 1, pos.y - 1);
	if (App->pathfinding->IsWalkable(cell))
		list_to_fill.pathList.push_back(&PathNode(-1, -1, cell, this, true));

	// north-west
	cell.create(pos.x + 1, pos.y + 1);
	if (App->pathfinding->IsWalkable(cell))
		list_to_fill.pathList.push_back(&PathNode(-1, -1, cell, this, true));

	// south-est
	cell.create(pos.x - 1, pos.y + 1);
	if (App->pathfinding->IsWalkable(cell))
		list_to_fill.pathList.push_back(&PathNode(-1, -1, cell, this, true));

	return list_to_fill.pathList.size();
}

// PathNode -------------------------------------------------------------------------
// Calculates this tile score
// ----------------------------------------------------------------------------------
float PathNode::Score() const
{
	return g + h;
}

// PathNode -------------------------------------------------------------------------
// Calculate the F for a specific destination tile
// ----------------------------------------------------------------------------------
float PathNode::CalculateF(const iPoint& destination, Distance distance_type)
{
	g = parent->g + 1;
	if (this->diagonal)
		g = parent->g + 1.7f;
	h = CalculateDistance(pos, destination, distance_type);

	return g + h;
}

// ----------------------------------------------------------------------------------
// Actual A* algorithm: return number of steps in the creation of the path or -1 ----
// ----------------------------------------------------------------------------------
int j1PathFinding::CreatePath(const iPoint& origin, const iPoint& destination, Distance distance_type)
{	
	last_path.clear();
	int ret = 0;

	// TODO 1: if origin or destination are not walkable, return -1
	if (!IsWalkable(origin) || !IsWalkable(destination))
		ret = -1;
	else {

		// TODO 2: Create two lists: open, close
		PathList open;
		PathList close;

		// Add the origin tile to open
		PathNode originNode(0, CalculateDistance(origin, destination, distance_type), origin, nullptr);
		open.pathList.push_back(&originNode);

		// Iterate while we have tile in the open list
		while (open.pathList.size() > 0) {

			// TODO 3: Move the lowest score cell from open list to the closed list
			PathNode* curr = (PathNode*)open.GetNodeLowestScore();

			close.pathList.push_back(curr);
			close.pathList.remove(curr);

			// TODO 4: If we just added the destination, we are done!
			// Backtrack to create the final path
			if (close.pathList.back()->pos == destination) {
				
				for (const PathNode* iterator = close.pathList.back(); iterator->parent != nullptr; 
					iterator = close.Find(iterator->parent->pos)) {
					
					last_path.push_back(iterator->pos);
				}

				last_path.push_back(close.pathList.front()->pos);

				// Use the Pathnode::parent and Flip() the path when you are finish
				
				// Flip the path
				vector<iPoint> ret_path = last_path;
				last_path.clear();

				vector<iPoint>::const_iterator it = ret_path.begin();
				while (it != ret_path.end()) {
					last_path.push_back(*it);
					it++;
				}

				ret = last_path.size();

				return ret;
				
			}
			else {
				// TODO 5: Fill a list of all adjancent nodes
				PathList neighbors;
				close.pathList.back()->FindWalkableAdjacents(neighbors);

				// TODO 6: Iterate adjancent nodes:
				list<PathNode*>::iterator iterator = neighbors.pathList.begin();

				while (iterator != neighbors.pathList.end()) {
					// ignore nodes in the closed list
					if (close.Find((*iterator)->pos) != NULL) {
						iterator++;
						continue;
					}

					(*iterator)->CalculateF(destination, distance_type);
					// If it is already in the open list, check if it is a better path (compare G)
					if (open.Find((*iterator)->pos) != NULL) {

						// If it is a better path, Update the parent
						PathNode open_node = *open.Find((*iterator)->pos);
						if ((*iterator)->g < open_node.g)
							open_node.parent = (*iterator)->parent;
					}
					else {
						// If it is NOT found, calculate its F and add it to the open list
						open.pathList.push_back(*iterator);
					}
					iterator++;
				}
				neighbors.pathList.clear();
			}
		}
	}

	return ret;
}

int CalculateDistance(iPoint origin, iPoint destination, Distance distance_type)
{
	int distance = 0;

	switch (distance_type) {
	case DISTANCE_TO:
		distance = origin.DistanceTo(destination);
		break;
	case DISTANCE_NO_SQRT:
		distance = origin.DistanceNoSqrt(destination);
		break;
	case MANHATTAN:
		distance = origin.DistanceManhattan(destination);
		break;
	}

	return distance;
}
