# RTS Group Movement

I am <a href="https://www.linkedin.com/in/sandra-alvarez-45453215b/">Sandra Alvarez</a>, student of the <a href="https://www.citm.upc.edu/ing/estudis/graus-videojocs/">Bachelor’s Degree in Video Games by UPC at CITM</a>. This content is generated for the second year’s subject Project 2, under supervision of lecturer <a href="https://es.linkedin.com/in/ricardpillosu">Ricard Pillosu</a>.

<I>NOTE: although this research focuses mainly on real-time strategy (RTS) games, the methods described can easily be applied to games of other genres.</I>

## Intro to the subject

“Game-unit pathfinding is something that most players never notice until it doesn’t work quite right, and then that minor issue becomes a rage-inducing, end-of-the-world problem” says Patrick Wyatt, a game programmer who worked on <I>StarCraft</I>.

Moving troops in groups via complex terrain is a challenge that many games have to deal with, specially real-time strategy games, where hundreds of units compete for the same destinations. The main question is, how do we make units cooperate with each other, so they move around without causing a terrible chaos? While the first part of the answer is pathfinding, the technique of finding paths from one location to another, the second part is movement. Without controlling the movement, units would behave individually, without taking in account the behaviors of the rest of the units. Here is, then, where coordinated movement comes in.

Coordinated movement is the result of combining two processes. The first one is pathfinding, while the second one is movement, the execution of a given path.

## How different games have approach it

Before we start to implement our coordinated movement system, we should understand precisely how coordinated movement works. To do so, there is no better way than to take a look at some existing games that have it. First, we will analyse how it is felt in the game (from the player's perspective). Then, we will dive deeper to investigate the method/s behind it.

### Command & Conquer: Tiberian Dawn (1995)

- Units always calculate the shortest route possible, even if this means getting through an enemy's trap. Thus, if units are shot while moving, they don't fire back.
- When a harvester attempts to return to the base while another harvester is going out to collect resources, if their routes share the same narrow path, the two of them will sometimes meet. If they do, they will turn twice (each time continuing to block each other's progress), center their orientation, and finally move right through each other.

<iframe width="740" height="590" src="http://www.youtube.com/watch?v=AGtd0KkOvG4&t=6m40s" frameborder="0" allowfullscreen></iframe>

The shortest distance is determined with algebra, not calculus. Consequently, the resulting path only takes into account the distance from point A to point B, ignoring any obstacles. This makes units overlap each other when they move, but if they do overlap, they will spread out again when reaching their destination.

### Warcraft I (1994) and II (1995)

- Every time a group of defensive units (still units) is attacked, the individual units will move further and further away.
- The armies head straight for their destination, going left or right when they hit something, then giving up after a while.
- It can take a unit 45 seconds to go from one end of the map to the other. In that time, new structures may have been built, making the original path invalid, or trees may have been chopped down, making the original path a very poor choice.
- Units often block each other. E.g.: peasants heading to the gold mine block peasants returning from it, because the only valid path for each group is blocked by the other group.

<I>Warcraft</I> engine is optimized to draw 32x32 pixel square tiles made of 16 8x8 pixel square cells (orthogonal perspective). The camera perspective of <I>Warcraft I</I> and <I>Warcraft II</I> is almost top-down, so the edges of the objects (buildings, etc.) are either horizontal or vertical. This leads to easy pathfinding, because each 32x32 tile is either passable or un-passable.

However, some tiles seem passable but actually are not. For example, the barracks building artwork does not fill completely the 96x96 area it sits on, and it leaves two tiles that seem passable but actually are not.

[](Images/Warcraft2.jpg)
<I>Warcraft II map with 32x32 tiles. The two tiles that seem passable but actually are not are drawn in red</I>

### StarCraft (1998)

- Units stop when they detect an object in front and politely wait for the object to move before continuing along the path.
- Units do not walk on top of each other, even if this means taking longer to get from point A to point B. Consequently, units remain visually separated.
- Harvesting units (Terran SVC, Zerg drone, Protoss probe) would get jammed up trying to harvest crystals or vespene gas (hereafter "minerals") and they would grind to a halt, because they are constantly running headlong into other harvesters traveling in the opposite direction. To avoid this situation, they ignore collisions with other units, so they can operate efficiently.

<iframe width="740" height="590" src="https://www.youtube.com/watch?v=0oJPPCaQeD4" frameborder="0" allowfullscreen></iframe>
  
<I>StarCraft</I> was built on the <I>Warcraft</I> engine (orthogonal perspective), but along the way the development team switched to isometric artwork to make the game more visually attractive. However, the terrain engine was not re-engineered to use isometric tiles. In order for pathfinding to work properly it was necessary to increase the resolution of the pathfinding map. Now, each 8x8 tile was either passable or unpassable. The increase in the size of the pathfinding map by a factor of 16 involved more computational effort when searching for a path. In addition, diagonal edges drawn in the artwork split many of the square tiles unevenly, making it hard to determine whether a tile should be passable or not.

Because the project was always two months from launch, there was no time to re-engine the terrain engine, so the pathfinding code had to be made to work. The pathfinding code, then, turned into a gigantic state-machine which handled all the tricky edge-cases.

[](Images/StarCraft.jpg)
<I>StarCraft map with 8x8 cells. The red line cuts each 8x8 cell into an irregular shape</I>

### StarCraft II: Wings of Liberty (2010)

- Units of all sizes find their way to destinations, without overlapping each other and without stopping.

<iframe width="740" height="590" src="https://www.youtube.com/watch?v=LztRm_bXGcc" frameborder="0" allowfullscreen></iframe>

### Supreme Commander 2 (2010)

- Smooth flow of the units.

<iframe width="740" height="590" src="https://www.youtube.com/watch?v=bovlsENv1g4" frameborder="0" allowfullscreen></iframe>

## Methods to approach it (used by the previous games)

### Tile-based algorithm A* (A-Star)

**Pathfinding technique:** A*<br><br>
The games <I>Command & Conquer: Tiberian Dawn</I>, <I>Warcraft I: Orcs & Humans</I>, <I>Warcraft II: Tides of Darkness</I>, and <I>StarCraft</I> base their group movement on the tile-based algorithm A* (A-Star). The primitive A* is the most common pathfinding algorithm used by the first RTS games such as the named, which had to deal with much lower processing power. 

**Movement behavior:** set of rules<br><br>
Since the pathfinding algorithm A* only takes into account the terrain (and, if modified, the objects of the map), it has to be complemented by a set of rules, which vary depending on the game and its needs. For example, in <I>Warcraft II</I>, a rule says that if a unit runs into other units and cannot slide past them, it will repath an alternate route. This works fine for a samll number of units, but when trying to navigate a large number of units through a narrow passage, a few will inevitably run into the units ahead of them and find another path.
  
As seen, those rules are very limited. In some situations, they force the games to sacrify more natural behaviors to make the whole system work. From the limitations behind the tile-based algorithm A* when dealing with group movement, it came out the Flocking System with Flow Fields.

### Flow Fields + Flocking (or Swarm) Behavior

The games <I>StarCraft II: Wings of Liberty</I> and <I>Supreme Commander 2</I>, and the great majority of modern RTS games use a Flocking System with Flow Fields to maintain fluid control of large groups of units. A local dynamic Flow Field is generated around each unit. The Flow Fields of the units are combined together before adjusting the units' movements.

**Pathfinding technique:** Flow Fields<br><br>
Flow Fields are an alternate way of doing pathfinding which works better for larger groups of units. A Flow Field is a grid where each grid square has a directional vector. This vector should be pointed in the direction of the most efficient way to get to the destination, while avoiding static obstacles.

**Movement behavior:** Flocking (or Swarm)<br><br>
The flocking model was defined by Craig Reynolds, an artificial life and computer graphics expert. Flocks, by definition, are a group of birds traveling together. Reynolds called the generic simulated flocking entities "boids", creating the Boids artificial life simulation (1986). The basic flocking model consists of three simple steering behaviors (separation, alignment and cohesion) which describe how an individual boid moves based on the positions and velocities of its nearby flockmates. As a result, entities in a flock (or boids) travel at roughly the same speed and form a cohesive group without strict arrangement.

The algorithm finds the fewest amount of waypoints and allows autonomous steering behaviour for units to smoothly hug their way around obstacles and its immediate neighbors. Logically, every unit has sensors which, when colliding with another unit, notify the first unit to turn in an appropriate direction to avoid the other unit.

## How we are going to approach it

From the two methods described, we will follow the first one. That means that we are going to implement a set of rules which allows individual units to execute paths (already found by the A* pathfinding algorithm) while moving together as a group.

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
Check out how <I>Rise of Nations</I> (2003) does formations! When you click on your destination, if you hold the mouse button down and drag, small circles are drawn on the tiles, showing the formation that the selected units would made.
PLUS: it also has smart systems where the melee is in front, the ranged behind, with artillery behind them.


