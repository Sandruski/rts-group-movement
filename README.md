# RTS Group Movement

I am Sandra Alvarez, student of the Bachelor’s Degree in Video Games by UPC at CITM. 
This content is generated for the second year’s subject Project 2, under supervision of lecturer Ricard Pillosu.

•	My GitHub account: [Sandruski](https://github.com/Sandruski)

## About the Research

This application is the result of a research in how to implement group movement for RTS games. 
It has been done following the tile-based algorithm A* (A-Star) and creating a collision determination and avoidance
system.

<I>It is not recommended to move simultaneously more than 16 units.</I>

•	Website: [RTS Group Movement Website](https://sandruski.github.io/RTS-Group-Movement/)<br>
•	GitHub repository: [RTS Group Movement Repository](https://github.com/Sandruski/RTS-Group-Movement)

## Inputs

### Keyboard

1: spawn a unit of priority 1 on the current mouse tile<br>
2: spawn a unit of priority 2 on the current mouse tile

F1: debug draw units' movement calculations<br>
F2: debug draw units' path<br>
F3: debug draw map collisions

ESC: close the application

### Mouse

Left click on a unit: select the unit<br>
Left click and drag: draw a rectangle. Units within the rectangle will be selected<br>
Left click on a tile without a unit on it: unselect all the selected units<br>
Right click: if units selected, set their group goal

## Tools

- IDE: Microsoft Visual Studio 2017 (language C++)
- SDL 2.0, STL, pugixml 1.8
- Profiler: Brofiler
- Map edition: Tiled
- Graphics edition: Adobe Photoshop

## Assets

All the sprites used belong to the game <I>Warcraft II: Tides of Darkness</I>, hence are property of Blizzard Entertainment.

## Known Bugs

<b>BUG 1.</b> When the user sets a new goal to a group of units, each unit searches for its own goal, which is found by running
a BFS algorithm. Since BFS only expands from a given node (tile), the new tile found may be located on the other side of a
wall. This leads to moving half of the group of units towards the side of the wall the user has clicked and one or two 
units to the opposite side.<br>
<b>BUG 1 PENDING TO SOLVE.</b> The BFS search algorithm will be repalced for a Dijkstra algorithm, which also considers
the cost of the nodes, preferring the low cost nodes. This way, Dijkstra will prefer to expand on the side of the wall the 
user has clicked and avoid jumping on the opposite side.
<b>BUG 2.</b> If many groups are moving at the same time and the user spam-clicks new goals for the units, the application either
slows down or stops for a fraction of second, due to the high computational demand.
