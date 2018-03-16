#include "Defs.h"
#include "p2Log.h"

#include "j1App.h"
#include "j1PathFinding.h"
#include "j1EntityFactory.h"
#include "j1Map.h"

#include "Brofiler\Brofiler.h"

j1PathFinding::j1PathFinding() : j1Module(), walkabilityMap(NULL), last_path(DEFAULT_PATH_LENGTH), width(0), height(0)
{
	name.assign("pathfinding");
}

// Destructor
j1PathFinding::~j1PathFinding()
{
	RELEASE_ARRAY(walkabilityMap);
}

// Called before quitting
bool j1PathFinding::CleanUp()
{
	LOG("Freeing pathfinding library");

	last_path.clear();
	RELEASE_ARRAY(walkabilityMap);
	return true;
}

// Sets up the walkability map
void j1PathFinding::SetMap(uint width, uint height, uchar* data)
{
	this->width = width;
	this->height = height;

	RELEASE_ARRAY(walkabilityMap);
	walkabilityMap = new uchar[width*height];
	memcpy(walkabilityMap, data, width*height);
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
	return INVALID_WALK_CODE && t > 0;
}

// Utility: return the walkability value of a tile
int j1PathFinding::GetTileAt(const iPoint& pos) const
{
	if (CheckBoundaries(pos))
		return walkabilityMap[(pos.y*width) + pos.x];

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
	list<PathNode>::const_iterator item = pathNodeList.begin();

	while (item != pathNodeList.end())
	{
		if ((*item).pos == point)
			return &(*item);
		item++;
	}
	
	return NULL;
}

// PathList ------------------------------------------------------------------------
// Returns the Pathnode with lowest score in this list or NULL if empty
// ---------------------------------------------------------------------------------
const PathNode* PathList::GetNodeLowestScore() const
{
	const PathNode* ret = NULL;
	float min = INT_MAX;

	list<PathNode>::const_reverse_iterator item = pathNodeList.rbegin();

	while (item != pathNodeList.rend())
	{
		if ((*item).Score() < min)
		{
			min = (*item).Score();
			ret = &(*item);
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
	uint before = list_to_fill.pathNodeList.size();

	cell.create(pos.x, pos.y + 1);
	if (App->pathfinding->IsWalkable(cell))
		list_to_fill.pathNodeList.push_back(PathNode(-1, -1, cell, this));

	// south
	cell.create(pos.x, pos.y - 1);
	if (App->pathfinding->IsWalkable(cell))
		list_to_fill.pathNodeList.push_back(PathNode(-1, -1, cell, this));

	// east
	cell.create(pos.x + 1, pos.y);
	if (App->pathfinding->IsWalkable(cell))
		list_to_fill.pathNodeList.push_back(PathNode(-1, -1, cell, this));

	// west
	cell.create(pos.x - 1, pos.y);
	if (App->pathfinding->IsWalkable(cell))
		list_to_fill.pathNodeList.push_back(PathNode(-1, -1, cell, this));

	// north-west
	cell.create(pos.x + 1, pos.y - 1);
	if (App->pathfinding->IsWalkable(cell))
		list_to_fill.pathNodeList.push_back(PathNode(-1, -1, cell, this, true));

	// south-west
	cell.create(pos.x - 1, pos.y - 1);
	if (App->pathfinding->IsWalkable(cell))
		list_to_fill.pathNodeList.push_back(PathNode(-1, -1, cell, this, true));

	// north-west
	cell.create(pos.x + 1, pos.y + 1);
	if (App->pathfinding->IsWalkable(cell))
		list_to_fill.pathNodeList.push_back(PathNode(-1, -1, cell, this, true));

	// south-est
	cell.create(pos.x - 1, pos.y + 1);
	if (App->pathfinding->IsWalkable(cell))
		list_to_fill.pathNodeList.push_back(PathNode(-1, -1, cell, this, true));

	return list_to_fill.pathNodeList.size();
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
float PathNode::CalculateF(const iPoint& destination, DistanceHeuristic distanceHeuristic)
{
	if (diagonal)
		g = parent->g + 1.7f;
	else
		g = parent->g + 1.0f;

	h = CalculateDistance(pos, destination, distanceHeuristic);

	return g + h;
}

// ----------------------------------------------------------------------------------
// Actual A* algorithm: return number of steps in the creation of the path or -1 ----
// ----------------------------------------------------------------------------------
int j1PathFinding::CreatePath(const iPoint& origin, const iPoint& destination, DistanceHeuristic distanceHeuristic)
{	
	BROFILER_CATEGORY(__FUNCTION__, Profiler::Color::Orchid);

	last_path.clear();
	int ret = 0;

	// If origin or destination are not walkable, return -1
	if (!IsWalkable(origin) || !IsWalkable(destination))
		ret = -1;
	else {

		// Create two lists: open, close

		// Add the origin tile to open
		PathNode originNode(0, CalculateDistance(origin, destination, distanceHeuristic), origin, nullptr);
		open.pathNodeList.push_back(originNode);

		// Iterate while we have tile in the open list
		while (open.pathNodeList.size() > 0) {

			// Move the lowest score cell from open list to the closed list
			PathNode* curr = (PathNode*)open.GetNodeLowestScore();
			close.pathNodeList.push_back(*curr);

			// Erase element from list -----
			list<PathNode>::iterator it = open.pathNodeList.begin();
			while (it != open.pathNodeList.end()) {

				if (&(*it) == &(*curr))
					break;
				it++;
			}
			open.pathNodeList.erase(it);
			// Erase element from list -----

			// If we just added the destination, we are done!
			// Backtrack to create the final path
			if (close.pathNodeList.back().pos == destination) {
				
				for (PathNode iterator = close.pathNodeList.back(); iterator.parent != nullptr;
					iterator = *close.Find(iterator.parent->pos)) {
					
					last_path.push_back(iterator.pos);
				}

				last_path.push_back(close.pathNodeList.front().pos);

				// Flip() the path when you are finish
				reverse(last_path.begin(), last_path.end());

				ret = last_path.size();

				return ret;
				
			}
			else {
				// Fill a list of all adjancent nodes
				PathList neighbors;
				close.pathNodeList.back().FindWalkableAdjacents(neighbors);

				// Iterate adjancent nodes:
				list<PathNode>::iterator iterator = neighbors.pathNodeList.begin();

				while (iterator != neighbors.pathNodeList.end()) {
					// ignore nodes in the closed list
					if (close.Find((*iterator).pos) != NULL) {
						iterator++;
						continue;
					}

					(*iterator).CalculateF(destination, distanceHeuristic);
					// If it is already in the open list, check if it is a better path (compare G)
					if (open.Find((*iterator).pos) != NULL) {

						// If it is a better path, Update the parent
						PathNode open_node = *open.Find((*iterator).pos);
						if ((*iterator).g < open_node.g)
							open_node.parent = (*iterator).parent;
					}
					else {
						// If it is NOT found, calculate its F and add it to the open list
						open.pathNodeList.push_back(*iterator);
					}
					iterator++;
				}
				neighbors.pathNodeList.clear();
			}
		}
	}

	return ret;
}

int CalculateDistance(iPoint origin, iPoint destination, DistanceHeuristic distanceHeuristic)
{
	int distance = 0;

	switch (distanceHeuristic) {
	case DistanceHeuristic_DistanceTo:
		distance = origin.DistanceTo(destination);
		break;
	case DistanceHeuristic_DistanceNoSqrt:
		distance = origin.DistanceNoSqrt(destination);
		break;
	case DistanceHeuristic_DistanceManhattan:
		distance = origin.DistanceManhattan(destination);
		break;
	}

	return distance;
}

bool j1PathFinding::InitializeAStar(const iPoint& origin, const iPoint& destination, DistanceHeuristic distanceHeuristic)
{
	// If origin or destination are not walkable, return false
	if (!IsWalkable(origin) || !IsWalkable(destination))
		return false;

	goal = destination;
	this->distanceHeuristic = distanceHeuristic;

	last_path.clear();

	// Add the origin tile to open
	PathNode originNode(0, CalculateDistance(origin, destination, distanceHeuristic), origin, nullptr);
	open.pathNodeList.push_back(originNode);

	return true;
}

PathfindingStatus j1PathFinding::CycleOnceAStar() 
{
	// If the open list is empty, the path has not been found
	if (open.pathNodeList.size() == 0)
		return PathfindingStatus_PathNotFound;

	// Move the lowest score cell from open list to the closed list
	PathNode* curr = (PathNode*)open.GetNodeLowestScore();

	/// Push_back the lowest score cell to the closed list
	close.pathNodeList.push_back(*curr);

	// If the current node is the goal, the path has been found
	if (curr->pos == goal) {

		// Backtrack to create the final path
		for (PathNode iterator = close.pathNodeList.back(); iterator.parent != nullptr;
			iterator = *close.Find(iterator.parent->pos)) {

			last_path.push_back(iterator.pos);
		}

		last_path.push_back(close.pathNodeList.front().pos);

		// Flip the path
		reverse(last_path.begin(), last_path.end());

		return PathfindingStatus_PathFound;
	}

	/// Erase the lowest score cell from the open list
	list<PathNode>::iterator it = open.pathNodeList.begin();
	while (it != open.pathNodeList.end()) {

		if (&(*it) == &(*curr))
			break;
		it++;
	}
	open.pathNodeList.erase(it);

	// Fill a list of all adjancent nodes
	PathList neighbors;
	close.pathNodeList.back().FindWalkableAdjacents(neighbors);

	list<PathNode>::iterator iterator = neighbors.pathNodeList.begin();

	while (iterator != neighbors.pathNodeList.end()) {

		// Ignore nodes in the closed list
		if (close.Find((*iterator).pos) != NULL) {
			iterator++;
			continue;
		}

		(*iterator).CalculateF(goal, DistanceHeuristic_DistanceManhattan);

		// If it is already in the open list, check if it is a better path (compare G)
		if (open.Find((*iterator).pos) != NULL) {

			// If it is a better path, Update the parent
			PathNode open_node = *open.Find((*iterator).pos);
			if ((*iterator).g < open_node.g)
				open_node.parent = (*iterator).parent;
		}
		else {

			// If it is NOT found, calculate its F and add it to the open list
			open.pathNodeList.push_back(*iterator);
		}

		iterator++;
	}
	neighbors.pathNodeList.clear();

	// There are still nodes to explore
	return PathfindingStatus_SearchIncomplete;
}

// Initialize CycleOnceDijkstra
bool j1PathFinding::InitializeDijkstra(const iPoint& origin, DistanceHeuristic distanceHeuristic) 
{
	// If origin is not walkable, return false
	if (!IsWalkable(origin))
		return false;

	start = origin;
	this->distanceHeuristic = distanceHeuristic;

	last_path.clear();

	// Add the origin tile to the priorityQueue
	iPointPriority curr;
	curr.point = origin;
	curr.priority = curr.point.DistanceManhattan(origin);
	priorityQueue.push(curr);

	return true;
}

// CycleOnce Dijkstra
PathfindingStatus j1PathFinding::CycleOnceDijkstra() 
{
	// If the open list is empty, the path has not been found
	if (priorityQueue.size() == 0)
		return PathfindingStatus_TileNotFound;

	iPointPriority curr;

	// Pop the first element of the priorityQueue
	curr = priorityQueue.top();
	priorityQueue.pop();

	iPoint neighbors[8];
	neighbors[0].create(curr.point.x + 1, curr.point.y + 0);
	neighbors[1].create(curr.point.x + 0, curr.point.y + 1);
	neighbors[2].create(curr.point.x - 1, curr.point.y + 0);
	neighbors[3].create(curr.point.x + 0, curr.point.y - 1);
	neighbors[4].create(curr.point.x + 1, curr.point.y + 1);
	neighbors[5].create(curr.point.x + 1, curr.point.y - 1);
	neighbors[6].create(curr.point.x - 1, curr.point.y + 1);
	neighbors[7].create(curr.point.x - 1, curr.point.y - 1);

	for (uint i = 0; i < 8; ++i)
	{
		if (App->pathfinding->IsWalkable(neighbors[i])) {

			/*
				iPointPriority priorityNeighbors;
				priorityNeighbors.point = neighbors[i];
				priorityNeighbors.priority = neighbors[i].DistanceManhattan(start);

				int g = cost_so_far[curr.point.x][curr.point.y] + neighbors[i].DistanceManhattan(start);

				if (find(visited.begin(), visited.end(), neighbors[i]) == visited.end() || g < cost_so_far[neighbors[i].x][neighbors[i].y]) {

					cost_so_far[neighbors[i].x][neighbors[i].y] = g;

					priorityQueue.push(priorityNeighbors);

					visited.push_back(neighbors[i]);
				}
			}
			*/
		}
	}

	return PathfindingStatus_SearchIncomplete;
}

iPoint j1PathFinding::GetLastTile() const 
{
	return last_tile;
}