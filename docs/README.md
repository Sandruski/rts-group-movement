# RTS Group Movement

I am <a href="https://www.linkedin.com/in/sandra-alvarez-45453215b/">Sandra Alvarez</a>, student of the <a href="https://www.citm.upc.edu/ing/estudis/graus-videojocs/">Bachelor’s Degree in Video Games by UPC at CITM</a>. This content is generated for the second year’s subject Project 2, under supervision of lecturer <a href="https://es.linkedin.com/in/ricardpillosu">Ricard Pillosu</a>.

<I>NOTE: although this research focuses mainly on real-time strategy (RTS) games, the methods described can easily be applied to games of other genres.</I>

## Intro to the subject

“Game-unit pathfinding is something that most players never notice until it doesn’t work quite right, and then that minor issue becomes a rage-inducing, end-of-the-world problem” says Patrick Wyatt, a game programmer who worked on <I>StarCraft</I>.

Moving troops in groups via complex terrain is a challenge that many games have to deal with, specially real-time strategy games, where hundreds of units compete for the same destinations. The main question is, how do we make units cooperate with each other, so they move around without causing a terrible chaos? While the first part of the answer is pathfinding, the technique of finding paths from one location to another, the second part is movement. Without controlling the movement, units would behave individually, without taking in account the behaviors of the rest of the units. Here is, then, where coordinated movement comes in.

Coordinated movement is the result of combining two processes. The first one is pathfinding, while the second one is movement, the execution of a given path.

## How different games have approach it

Before we start to implement our coordinated movement system, we should understand precisely how coordinated movement works. To do so, there is no better way than to take a look at some existing games that use it. For each game, first, we will analyse how it is felt from the player’s perspective, and then, we will dive deeper to investigate the method behind it.

### Command & Conquer: Tiberian Dawn

- Units always calculate the shortest route possible, even if this means getting through an enemy's trap. Thus, if units are shot while moving, they don't fire back.
- When a harvester attempts to return to the base while another harvester is going out to collect resources, if their routes share the same narrow path, the two of them will sometimes meet. If they do, they will turn twice (each time continuing to block each other's progress), center their orientation, and finally move right through each other.

<iframe width="740" height="590" src="http://www.youtube.com/watch?v=AGtd0KkOvG4&t=6m40s" frameborder="0" allowfullscreen></iframe>

<b>Method used:</b>
The shortest distance is determined with algebra, not calculus. Consequently, the resulting path only takes into account the distance from point A to point B, ignoring any obstacles.

### Warcraft I and II

- Every time a group of defensive units (still units) is attacked, the individual units will move further and further away.
- The armies head straight for their destination, going left or right when they hit something, then giving up after a while.
- It can take a unit 45 seconds to go from one end of the map to the other. In that time, new structures may have been built, making the original path invalid, or trees may have been chopped down, making the original path a very poor choice.
- Units often block each other. E.g.: peasants heading to the gold mine block peasants returning from it, because the only valid path for each group is blocked by the other group.

<b>Method used:</b>A* search algorithm
Warcraft engine is optimized to draw 32x32 pixel square tiles made of 16 8x8 pixel square cells. The camera perspective of Warcraft I and Warcraft II is almost top-down, so the edges of the objects (buildings, etc.) are either horizontal or vertical. This leads to easy pathfinding, because each 32x32 tile is either passable or un-passable. 

However, some tiles seem passable but actually are not. For example, the barracks building artwork does not fill completely the 96x96 area it sits on, and it leaves two tiles that seem passable but actually are not.

### StarCraft

In StarCraft, units stop when they detect an object in front and politely wait for the object to move before continuing along the path.
Units never walk on top of each other, even if this means taking longer to get from point A to point B. Consequently, units remain visually separated. The harvesting units (Terran SVC, Zerg drone, Protoss probe) would get jammed up trying to harvest crystals or vespene gas (hereafter "minerals") and they would grind to a halt.

<b>Method used:</b>A* search algorithm
The A* search algorithm led some problems in leading with collision, since it only takes into account the terrain and the buildings. If it runs into other units and can't slide past them it will repath an alternate route. This works fine for a samll number of units, but if you try to navigate a large number of units through a narrow passage inevitably a few will run into the units ahead of them and find another path. Those harvesters are commuting between the minerals and their base so they're constantly running headlong into other harvesters traveling in the opposite direction. Given enough harvesters in a small space it's entirely possible that some get jammed in and are unable to move until the mineral deposit is mined out. This was resolved by making harvesters ignore collisions with other units, so harvesters operate efficiently.

StarCraft was built on the Warcraft engine, but along the way the development team switched StarCraft to isometric artwork to make the game more visually attractive. However, the underlying terrain engine was not re-engineered to use isometric tiles, only the artwork was redrawn.

The new camera perspective looked great but in order for pathfinding to work properly it was necessary to increase the resolutuon of the pathfinding map: now each 8x8 tile was either passable or unpassable, increasing the size of the pathfinding map by a factor of 16. While the finer resolution enabled more units to be squeezed onto the map, it also meant that searching for a path around the map would require substantially more computational effort to search the larger pathing space.
Pathfinding was now more challenging because diagonal edges drawn in the artwork split many of the square tiles unevenly, making it hard tgo determine wheter a tile should be passable or not.

Because the project was always two months from launch, it was inconceivable that there was enough time to re-engineer the terrain enginge to make pathfinding easier, so the pathfinding code just had to be made to work. To handle all the tricky edge-cases, the pathinf code exploded into a gigantic state-machine which encoded all sorts of specialized "get me out of here" hacks.

### StarCraft II: Wings of Liberty

<b>Method used:</b>

## How we are going to approach it
In the following article, we are going to focus on ways to execute a path that’s already been found.

### Simple Movement Algorithm

### Collision Determination System

How do we avoid them lining up?
How do they finish the path? (avoid all to the same spot)

## Links to more documentation

## TODOs

### Homework (optional)

If this research caught your eye and you want to keep practising, I suggest you to try to add one or all of the following improvements to the group movement system.

1.
2. Formations.
Check out how <I>Rise of Nations</I> does formations! When you click on your destination, if you hold the mouse button down and drag, small circles are drawn on the tiles, showing the formation that the selected units would made.
PLUS: it also has smart systems where the melee is in front, the ranged behind, with artillery behind them.


