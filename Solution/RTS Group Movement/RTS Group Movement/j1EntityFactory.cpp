#include "Defs.h"
#include "p2Log.h"

#include "j1Module.h"
#include "j1App.h"

#include "j1EntityFactory.h"
#include "j1Render.h"

#include "j1Textures.h"
#include "j1Scene.h"
#include "j1Map.h"
#include "j1Pathfinding.h"
#include "j1Collision.h"
#include "j1Movement.h"
#include "Goal.h"

#include "Entity.h"

#include "Brofiler\Brofiler.h"

j1EntityFactory::j1EntityFactory()
{
	name.assign("entities");
}

// Destructor
j1EntityFactory::~j1EntityFactory()
{
}

bool j1EntityFactory::Awake(pugi::xml_node& config) {

	bool ret = true;

	// Load spritesheets
	pugi::xml_node spritesheets = config.child("spritesheets");
	footmanTexName = spritesheets.child("footman").attribute("name").as_string();
	gruntTexName = spritesheets.child("grunt").attribute("name").as_string();
	crittersTexName = spritesheets.child("critters").attribute("name").as_string();

	// Load animations
	// Footman
	pugi::xml_node footmanAnimations = config.child("animations").child("footman");
	pugi::xml_node currentAnimation;

	/// Walk
	// up
	currentAnimation = footmanAnimations.child("up");
	footmanInfo.up.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.up.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.up.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down
	currentAnimation = footmanAnimations.child("down");
	footmanInfo.down.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.down.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.down.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// right
	currentAnimation = footmanAnimations.child("right");
	footmanInfo.right.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.right.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.right.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// left
	currentAnimation = footmanAnimations.child("left");
	footmanInfo.left.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.left.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.left.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// up-right
	currentAnimation = footmanAnimations.child("upRight");
	footmanInfo.upRight.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.upRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.upRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// up-left
	currentAnimation = footmanAnimations.child("upLeft");
	footmanInfo.upLeft.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.upLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.upLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down-right
	currentAnimation = footmanAnimations.child("downRight");
	footmanInfo.downRight.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.downRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.downRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down-left
	currentAnimation = footmanAnimations.child("downLeft");
	footmanInfo.downLeft.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.downLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.downLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}

	/// Attack
	// attack up
	currentAnimation = footmanAnimations.child("attackUp");
	footmanInfo.attackUp.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.attackUp.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.attackUp.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// attack down
	currentAnimation = footmanAnimations.child("attackDown");
	footmanInfo.attackDown.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.attackDown.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.attackDown.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// attack left
	currentAnimation = footmanAnimations.child("attackLeft");
	footmanInfo.attackLeft.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.attackLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.attackLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// attack right
	currentAnimation = footmanAnimations.child("attackRight");
	footmanInfo.attackRight.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.attackRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.attackRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// attack up-left
	currentAnimation = footmanAnimations.child("attackUpLeft");
	footmanInfo.attackUpLeft.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.attackUpLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.attackUpLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// attack up-right
	currentAnimation = footmanAnimations.child("attackUpRight");
	footmanInfo.attackUpRight.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.attackUpRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.attackUpRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// attack down-left
	currentAnimation = footmanAnimations.child("attackDownLeft");
	footmanInfo.attackDownLeft.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.attackDownLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.attackDownLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// attack down-right
	currentAnimation = footmanAnimations.child("attackDownRight");
	footmanInfo.attackDownRight.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.attackDownRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.attackDownRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// death up
	currentAnimation = footmanAnimations.child("deathUp");
	footmanInfo.deathUp.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.deathUp.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.deathUp.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// death down
	currentAnimation = footmanAnimations.child("deathDown");
	footmanInfo.deathDown.speed = currentAnimation.attribute("speed").as_float();
	footmanInfo.deathDown.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		footmanInfo.deathDown.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}

	// Grunt
	pugi::xml_node gruntAnimations = config.child("animations").child("grunt");

	/// Walk
	// up
	currentAnimation = gruntAnimations.child("up");
	gruntInfo.up.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.up.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.up.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down
	currentAnimation = gruntAnimations.child("down");
	gruntInfo.down.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.down.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.down.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// left
	currentAnimation = gruntAnimations.child("left");
	gruntInfo.left.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.left.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.left.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// right
	currentAnimation = gruntAnimations.child("right");
	gruntInfo.right.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.right.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.right.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// up-left
	currentAnimation = gruntAnimations.child("upLeft");
	gruntInfo.upLeft.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.upLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.upLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// up-right
	currentAnimation = gruntAnimations.child("upRight");
	gruntInfo.upRight.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.upRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.upRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down-left
	currentAnimation = gruntAnimations.child("downLeft");
	gruntInfo.downLeft.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.downLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.downLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down-right
	currentAnimation = gruntAnimations.child("downRight");
	gruntInfo.downRight.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.downRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.downRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}

	/// Attack
	// attack up
	currentAnimation = gruntAnimations.child("attackUp");
	gruntInfo.attackUp.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.attackUp.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.attackUp.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// attack down
	currentAnimation = gruntAnimations.child("attackDown");
	gruntInfo.attackDown.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.attackDown.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.attackDown.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// attack left
	currentAnimation = gruntAnimations.child("attackLeft");
	gruntInfo.attackLeft.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.attackLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.attackLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// attack right
	currentAnimation = gruntAnimations.child("attackRight");
	gruntInfo.attackRight.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.attackRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.attackRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// attack up-left
	currentAnimation = gruntAnimations.child("attackUpLeft");
	gruntInfo.attackUpLeft.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.attackUpLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.attackUpLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// attack up-right
	currentAnimation = gruntAnimations.child("attackUpRight");
	gruntInfo.attackUpRight.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.attackUpRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.attackUpRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// attack down-left
	currentAnimation = gruntAnimations.child("attackDownLeft");
	gruntInfo.attackDownLeft.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.attackDownLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.attackDownLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// attack down-right
	currentAnimation = gruntAnimations.child("attackDownRight");
	gruntInfo.attackDownRight.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.attackDownRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.attackDownRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// attack death up
	currentAnimation = gruntAnimations.child("deathUp");
	gruntInfo.deathUp.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.deathUp.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.deathUp.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// attack death down
	currentAnimation = gruntAnimations.child("deathDown");
	gruntInfo.deathDown.speed = currentAnimation.attribute("speed").as_float();
	gruntInfo.deathDown.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		gruntInfo.deathDown.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}

	// Critter sheep
	pugi::xml_node sheepAnimations = config.child("animations").child("sheep");

	// up
	currentAnimation = sheepAnimations.child("up");
	critterSheepInfo.up.speed = currentAnimation.attribute("speed").as_float();
	critterSheepInfo.up.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterSheepInfo.up.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down
	currentAnimation = sheepAnimations.child("down");
	critterSheepInfo.down.speed = currentAnimation.attribute("speed").as_float();
	critterSheepInfo.down.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterSheepInfo.down.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// left
	currentAnimation = sheepAnimations.child("left");
	critterSheepInfo.left.speed = currentAnimation.attribute("speed").as_float();
	critterSheepInfo.left.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterSheepInfo.left.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// right
	currentAnimation = sheepAnimations.child("right");
	critterSheepInfo.right.speed = currentAnimation.attribute("speed").as_float();
	critterSheepInfo.right.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterSheepInfo.right.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// up-left
	currentAnimation = sheepAnimations.child("upLeft");
	critterSheepInfo.upLeft.speed = currentAnimation.attribute("speed").as_float();
	critterSheepInfo.upLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterSheepInfo.upLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// up-right
	currentAnimation = sheepAnimations.child("upRight");
	critterSheepInfo.upRight.speed = currentAnimation.attribute("speed").as_float();
	critterSheepInfo.upRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterSheepInfo.upRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down-left
	currentAnimation = sheepAnimations.child("downLeft");
	critterSheepInfo.downLeft.speed = currentAnimation.attribute("speed").as_float();
	critterSheepInfo.downLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterSheepInfo.downLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down-right
	currentAnimation = sheepAnimations.child("downRight");
	critterSheepInfo.downRight.speed = currentAnimation.attribute("speed").as_float();
	critterSheepInfo.downRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterSheepInfo.downRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}

	// death up-left
	currentAnimation = sheepAnimations.child("deathUpLeft");
	critterSheepInfo.deathUpLeft.speed = currentAnimation.attribute("speed").as_float();
	critterSheepInfo.deathUpLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterSheepInfo.deathUpLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// death up-right
	currentAnimation = sheepAnimations.child("deathUpRight");
	critterSheepInfo.deathUpRight.speed = currentAnimation.attribute("speed").as_float();
	critterSheepInfo.deathUpRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterSheepInfo.deathUpRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// death down-left
	currentAnimation = sheepAnimations.child("deathDownLeft");
	critterSheepInfo.deathDownLeft.speed = currentAnimation.attribute("speed").as_float();
	critterSheepInfo.deathDownLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterSheepInfo.deathDownLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// death down-right
	currentAnimation = sheepAnimations.child("deathDownRight");
	critterSheepInfo.deathDownRight.speed = currentAnimation.attribute("speed").as_float();
	critterSheepInfo.deathDownRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterSheepInfo.deathDownRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}

	// Critter boar
	pugi::xml_node boarAnimations = config.child("animations").child("boar");

	// up
	currentAnimation = boarAnimations.child("up");
	critterBoarInfo.up.speed = currentAnimation.attribute("speed").as_float();
	critterBoarInfo.up.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterBoarInfo.up.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down
	currentAnimation = boarAnimations.child("down");
	critterBoarInfo.down.speed = currentAnimation.attribute("speed").as_float();
	critterBoarInfo.down.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterBoarInfo.down.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// left
	currentAnimation = boarAnimations.child("left");
	critterBoarInfo.left.speed = currentAnimation.attribute("speed").as_float();
	critterBoarInfo.left.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterBoarInfo.left.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// right
	currentAnimation = boarAnimations.child("right");
	critterBoarInfo.right.speed = currentAnimation.attribute("speed").as_float();
	critterBoarInfo.right.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterBoarInfo.right.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// up-left
	currentAnimation = boarAnimations.child("upLeft");
	critterBoarInfo.upLeft.speed = currentAnimation.attribute("speed").as_float();
	critterBoarInfo.upLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterBoarInfo.upLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// up-right
	currentAnimation = boarAnimations.child("upRight");
	critterBoarInfo.upRight.speed = currentAnimation.attribute("speed").as_float();
	critterBoarInfo.upRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterBoarInfo.upRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down-left
	currentAnimation = boarAnimations.child("downLeft");
	critterBoarInfo.downLeft.speed = currentAnimation.attribute("speed").as_float();
	critterBoarInfo.downLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterBoarInfo.downLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// down-right
	currentAnimation = boarAnimations.child("downRight");
	critterBoarInfo.downRight.speed = currentAnimation.attribute("speed").as_float();
	critterBoarInfo.downRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterBoarInfo.downRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}

	// death up-left
	currentAnimation = boarAnimations.child("deathUpLeft");
	critterBoarInfo.deathUpLeft.speed = currentAnimation.attribute("speed").as_float();
	critterBoarInfo.deathUpLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterBoarInfo.deathUpLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// death up-right
	currentAnimation = boarAnimations.child("deathUpRight");
	critterBoarInfo.deathUpRight.speed = currentAnimation.attribute("speed").as_float();
	critterBoarInfo.deathUpRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterBoarInfo.deathUpRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// death down-left
	currentAnimation = boarAnimations.child("deathDownLeft");
	critterBoarInfo.deathDownLeft.speed = currentAnimation.attribute("speed").as_float();
	critterBoarInfo.deathDownLeft.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterBoarInfo.deathDownLeft.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}
	// death down-right
	currentAnimation = boarAnimations.child("deathDownRight");
	critterBoarInfo.deathDownRight.speed = currentAnimation.attribute("speed").as_float();
	critterBoarInfo.deathDownRight.loop = currentAnimation.attribute("loop").as_bool();
	for (currentAnimation = currentAnimation.child("frame"); currentAnimation; currentAnimation = currentAnimation.next_sibling("frame")) {
		critterBoarInfo.deathDownRight.PushBack({ currentAnimation.attribute("x").as_int(), currentAnimation.attribute("y").as_int(), currentAnimation.attribute("w").as_int(), currentAnimation.attribute("h").as_int() });
	}

	return ret;
}

bool j1EntityFactory::Start()
{
	bool ret = true;

	LOG("Loading entities textures");

	footmanTex = App->tex->Load(footmanTexName.data());
	gruntTex = App->tex->Load(gruntTexName.data());
	crittersTex = App->tex->Load(crittersTexName.data());

	return ret;
}

bool j1EntityFactory::PreUpdate()
{
	BROFILER_CATEGORY(__FUNCTION__, Profiler::Color::Orchid);

	bool ret = true;

	// Spawn entities
	list<Entity*>::const_iterator it = toSpawnEntities.begin();

	while (it != toSpawnEntities.end()) {

		// Move the entity from the waiting list to the active list
		activeDynamicEntities.push_back((DynamicEntity*)(*it));

		fPoint pos = (*it)->GetPos();
		int x = pos.x * App->scene->scale;
		int y = pos.y * App->scene->scale;
		iPoint spawnPos = App->map->WorldToMap(x, y);
		LOG("Spawning entity at tile %d,%d", spawnPos.x, spawnPos.y);

		it++;
	}
	toSpawnEntities.clear();

	return ret;
}

// Called before render is available
bool j1EntityFactory::Update(float dt)
{
	BROFILER_CATEGORY(__FUNCTION__, Profiler::Color::Orchid);

	bool ret = true;

	// Update active entities
	list<DynamicEntity*>::const_iterator it = activeDynamicEntities.begin();

	while (it != activeDynamicEntities.end()) {

		(*it)->Move(dt);
		it++;
	}

	return ret;
}

bool j1EntityFactory::PostUpdate()
{
	BROFILER_CATEGORY(__FUNCTION__, Profiler::Color::Orchid);

	bool ret = true;

	// Dead entities cannot be selected. Remove them from the unitsSelected list
	list<DynamicEntity*>::iterator unit = unitsSelected.begin();

	while (unit != unitsSelected.end()) {

		if ((*unit)->isDead) {

			if (find(unitsSelected.begin(), unitsSelected.end(), *unit) != unitsSelected.end()) {

				(*unit)->isSelected = false;
				unitsSelected.remove(*unit);

				unit = unitsSelected.begin();
				continue;
			}
		}

		unit++;
	}

	// Remove entities
	list<DynamicEntity*>::iterator dynEnt = activeDynamicEntities.begin();

	while (dynEnt != activeDynamicEntities.end()) {

		if ((*dynEnt)->isRemove) {

			delete *dynEnt;
			activeDynamicEntities.erase(dynEnt);

			dynEnt = activeDynamicEntities.begin();
			continue;
		}

		dynEnt++;
	}

	return ret;
}

// Called before quitting
bool j1EntityFactory::CleanUp()
{
	bool ret = true;

	LOG("Freeing all entities");

	// Remove all entities
	list<DynamicEntity*>::const_iterator dynEnt = activeDynamicEntities.begin();

	while (dynEnt != activeDynamicEntities.end()) {
		delete *dynEnt;
		dynEnt++;
	}
	activeDynamicEntities.clear();

	list<Entity*>::const_iterator it = toSpawnEntities.begin();

	while (it != toSpawnEntities.end()) {
		delete *it;
		it++;
	}
	toSpawnEntities.clear();
	unitsSelected.clear();

	// Free all textures
	App->tex->UnLoad(footmanTex);
	App->tex->UnLoad(gruntTex);
	App->tex->UnLoad(crittersTex);

	return ret;
}

void j1EntityFactory::OnCollision(ColliderGroup* c1, ColliderGroup* c2, CollisionState collisionState)
{
	// Check for collisions
	list<DynamicEntity*>::const_iterator it = activeDynamicEntities.begin();

	while (it != activeDynamicEntities.end()) {

		// - SightRadiusCollider and AttackRadiusCollider call their owner as the c1 Collider
		if ((*it)->GetSightRadiusCollider() == c1 || (*it)->GetAttackRadiusCollider() == c1) {

			(*it)->OnCollision(c1, c2, collisionState);
			break;
		}

		it++;
	}
}

void j1EntityFactory::Draw()
{
	BROFILER_CATEGORY(__FUNCTION__, Profiler::Color::Orchid);

	// Blit active entities
	list<DynamicEntity*>::const_iterator it = activeDynamicEntities.begin();

	while (it != activeDynamicEntities.end()) {

		switch ((*it)->dynamicEntityType) {

		case DynamicEntityType_Footman:
			(*it)->Draw(footmanTex);
			break;

		case DynamicEntityType_Grunt:
			(*it)->Draw(gruntTex);
			break;

		case DynamicEntityType_CritterSheep:
		case DynamicEntityType_CritterBoar:
			(*it)->Draw(crittersTex);
			break;
		}

		it++;
	}
}

// Adds a DynamicEntity
DynamicEntity* j1EntityFactory::AddDynamicEntity(DynamicEntityType dynamicEntityType, fPoint pos, iPoint size, uint currLife, uint maxLife, const UnitInfo& unitInfo, const EntityInfo& entityInfo)
{
	switch (dynamicEntityType) {

	case DynamicEntityType_Footman:
	{
		Footman* footman = new Footman(pos, size, currLife, maxLife, unitInfo, (const FootmanInfo&)entityInfo);
		footman->entityType = EntityType_DynamicEntity;
		footman->entitySide = EntitySide_Player;
		footman->dynamicEntityType = DynamicEntityType_Footman;

		toSpawnEntities.push_back((Entity*)footman);
		return (DynamicEntity*)footman;
	}
	break;

	case DynamicEntityType_Grunt:
	{
		Grunt* grunt = new Grunt(pos, size, currLife, maxLife, unitInfo, (const GruntInfo&)entityInfo);
		grunt->entityType = EntityType_DynamicEntity;
		grunt->entitySide = EntitySide_Enemy;
		grunt->dynamicEntityType = DynamicEntityType_Grunt;

		toSpawnEntities.push_back((Entity*)grunt);
		return (DynamicEntity*)grunt;
	}
	break;

	case DynamicEntityType_CritterSheep:
	{
		CritterSheep* critterSheep = new CritterSheep(pos, size, currLife, maxLife, unitInfo, (const CritterSheepInfo&)entityInfo);
		critterSheep->entityType = EntityType_DynamicEntity;
		critterSheep->entitySide = EntitySide_Neutral;
		critterSheep->dynamicEntityType = DynamicEntityType_CritterSheep;

		toSpawnEntities.push_back((Entity*)critterSheep);
		return (DynamicEntity*)critterSheep;
	}
	break;

	case DynamicEntityType_CritterBoar:
	{
		CritterBoar* critterBoar = new CritterBoar(pos, size, currLife, maxLife, unitInfo, (const CritterBoarInfo&)entityInfo);
		critterBoar->entityType = EntityType_DynamicEntity;
		critterBoar->entitySide = EntitySide_Neutral;
		critterBoar->dynamicEntityType = DynamicEntityType_CritterBoar;

		toSpawnEntities.push_back((Entity*)critterBoar);
		return (DynamicEntity*)critterBoar;
	}
	break;

	default:

		return nullptr;
	}
}

// Returns a pointer to the Entity that is on the tile or nullptr
Entity* j1EntityFactory::IsEntityOnTile(iPoint tile, EntityType entityType, EntitySide entitySide) const
{
	list<DynamicEntity*>::const_iterator activeDyn = activeDynamicEntities.begin();

	while (activeDyn != activeDynamicEntities.end()) {
		
		// The unit cannot be dead
		if (!(*activeDyn)->isDead) {

			iPoint entityTile = App->map->WorldToMap((*activeDyn)->GetPos().x, (*activeDyn)->GetPos().y);

			switch (entitySide) {

			case EntitySide_Player:

				if ((*activeDyn)->entitySide == EntitySide_Player)
					if (tile.x == entityTile.x && tile.y == entityTile.y)
						return (Entity*)(*activeDyn);
				break;

			case EntitySide_Enemy:

				if ((*activeDyn)->entitySide == EntitySide_Enemy)
					if (tile.x == entityTile.x && tile.y == entityTile.y)
						return (Entity*)(*activeDyn);
				break;

			case EntitySide_Neutral:

				if ((*activeDyn)->entitySide == EntitySide_Neutral)
					if (tile.x == entityTile.x && tile.y == entityTile.y)
						return (Entity*)(*activeDyn);
				break;

			case EntitySide_NoSide:

				if (tile.x == entityTile.x && tile.y == entityTile.y)
					return (Entity*)(*activeDyn);
				break;
			}
		}

		activeDyn++;
	}

	// TODO: Add StaticEntities (and check them depending on the entityType parameter)

	// We do also need to check the toSpawn list (just in case)
	list<Entity*>::const_iterator toSpawn = toSpawnEntities.begin();

	while (toSpawn != toSpawnEntities.end()) {

		iPoint entityTile = App->map->WorldToMap((*toSpawn)->GetPos().x, (*toSpawn)->GetPos().y);

		switch (entitySide) {

		case EntitySide_Player:

			if ((*toSpawn)->entitySide == EntitySide_Player)
				if (tile.x == entityTile.x && tile.y == entityTile.y)
					return (Entity*)(*toSpawn);
			break;

		case EntitySide_Enemy:

			if ((*toSpawn)->entitySide == EntitySide_Enemy)
				if (tile.x == entityTile.x && tile.y == entityTile.y)
					return (Entity*)(*toSpawn);
			break;

		case EntitySide_Neutral:

			if ((*toSpawn)->entitySide == EntitySide_Neutral)
				if (tile.x == entityTile.x && tile.y == entityTile.y)
					return (Entity*)(*toSpawn);
			break;

		case EntitySide_NoSide:

			if (tile.x == entityTile.x && tile.y == entityTile.y)
				return (Entity*)(*toSpawn);
			break;
		}

		toSpawn++;
	}

	return nullptr;
}

// Selects an Entity
bool j1EntityFactory::SelectEntity(Entity* entity)
{
	bool ret = false;

	if (entity == nullptr)
		return false;

	list<DynamicEntity*>::const_iterator it = activeDynamicEntities.begin();

	while (it != activeDynamicEntities.end()) {

		// Remove entities from the unitsSelected list
		if ((*it) != entity) {
			unitsSelected.remove(GetDynamicEntityByEntity(*it));
			(*it)->isSelected = false;
		}
		it++;
	}

	// If the unit isn't in the unitsSelected list, add it
	if (find(unitsSelected.begin(), unitsSelected.end(), entity) == unitsSelected.end()) {

		DynamicEntity* unit = GetDynamicEntityByEntity(entity);
		unitsSelected.push_back(unit);
		(entity)->isSelected = true;

		ret = true;
	}

	// TODO: Add StaticEntities

	// Update the color of the selection of all entities (Dynamic and Static)
	SetUnitsSelectedColor();

	return ret;
}

// Selects the entities within a rectangle
/// TODO:
/// - If units are selected, buildings cannot be selected
/// - If a building is selected, units cannot be selected
/// · Only 1 building can be selected at a time
void j1EntityFactory::SelectEntitiesWithinRectangle(SDL_Rect rectangleRect, EntitySide entitySide)
{
	list<DynamicEntity*>::const_iterator it = activeDynamicEntities.begin();

	while (it != activeDynamicEntities.end()) {

		// The unit cannot be dead
		if ((*it)->isDead) {

			it++;
			continue;
		}

		if (entitySide == EntitySide_NoSide
			|| (entitySide == EntitySide_Player && (*it)->entitySide == EntitySide_Player)
			|| (entitySide == EntitySide_Enemy && (*it)->entitySide == EntitySide_Enemy)) {

			SDL_Rect entityRect = { (*it)->GetPos().x, (*it)->GetPos().y, (*it)->GetSize().x, (*it)->GetSize().y };

			// If the unit is within the selection:
			if (SDL_HasIntersection(&entityRect, &rectangleRect)) {

				// It there are less units than MAX_UNITS_SELECTED selected:
				if (unitsSelected.size() < MAX_UNITS_SELECTED) {

					// If the unit isn't in the unitsSelected list, add it
					if (find(unitsSelected.begin(), unitsSelected.end(), *it) == unitsSelected.end()) {
						unitsSelected.push_back(GetDynamicEntityByEntity(*it));
						(*it)->isSelected = true;
					}
				}
			}
			else {

				// If the unit is in the unitsSelected list, remove it
				if (find(unitsSelected.begin(), unitsSelected.end(), *it) != unitsSelected.end()) {
					unitsSelected.remove(GetDynamicEntityByEntity(*it));
					(*it)->isSelected = false;
				}
			}
		}

		it++;
	}

	// TODO: Add StaticEntities

	// Update the color of the selection of all entities (Dynamic and Static)
	SetUnitsSelectedColor();
}

// Unselects all entities
void j1EntityFactory::UnselectAllEntities() 
{
	list<DynamicEntity*>::const_iterator it = activeDynamicEntities.begin();

	while (it != activeDynamicEntities.end()) {
	
		if ((*it)->isSelected) {

			(*it)->isSelected = false;

			// If the entity is in the unitsSelected list, remove it
			if (find(unitsSelected.begin(), unitsSelected.end(), *it) != unitsSelected.end())
				unitsSelected.remove(GetDynamicEntityByEntity(*it));
		}

		it++;
	}
}

// Returns a pointer to the DynamicEntity of an Entity
DynamicEntity* j1EntityFactory::GetDynamicEntityByEntity(Entity* entity) const
{
	if (entity->entityType == EntityType_DynamicEntity)
		return (DynamicEntity*)entity;

	return nullptr;
}

// Returns a list with the last units selected
list<DynamicEntity*> j1EntityFactory::GetLastUnitsSelected() const
{
	return unitsSelected;
}

bool j1EntityFactory::RemoveUnitFromUnitsSelected(Entity* entity) 
{
	bool ret = false;

	list<DynamicEntity*>::const_iterator it = find(unitsSelected.begin(), unitsSelected.end(), entity);

	if (it != unitsSelected.end()) {

		(*it)->isSelected = false;

		unitsSelected.remove(*it);
		ret = true;
	}

	return ret;
}

// Updates the selection color of all entities
void j1EntityFactory::SetUnitsSelectedColor()
{
	SDL_Color colors[10] = { ColorYellow, ColorDarkGreen, ColorBrightBlue, ColorOrange, ColorPink, ColorPurple, ColorGrey, ColorBlack, ColorOlive, ColorViolet };
	string colorNames[10] = { "Yellow", "DarkGreen", "BrightBlue", "Orange", "Pink", "Purple", "Grey", "Black", "Olive", "Violet" };

	list<DynamicEntity*>::const_iterator it = activeDynamicEntities.begin();
	uint i = 0;

	while (it != activeDynamicEntities.end())
	{
		// If the unit is selected, change its color
		if ((*it)->isSelected) {

			GetDynamicEntityByEntity(*it)->SetColor(colors[i], colorNames[i]);
			i++;
		}
		else {

			// If the unit is no longer selected, change its color to default white
			GetDynamicEntityByEntity(*it)->SetColor(ColorWhite, "White");
		}

		it++;
	}
}

// Returns the entityInfo (normally read from the config file)
EntityInfo& j1EntityFactory::GetDynamicEntityInfo(DynamicEntityType dynamicEntityType) 
{
	switch (dynamicEntityType) {
	
	case DynamicEntityType_Footman:
		return (EntityInfo&)footmanInfo;
		break;

	case DynamicEntityType_Grunt:
		return (EntityInfo&)gruntInfo;
		break;

	case DynamicEntityType_CritterSheep:
		return (EntityInfo&)critterSheepInfo;
		break;

	case DynamicEntityType_CritterBoar:
		return (EntityInfo&)critterBoarInfo;
		break;
	}
}

bool j1EntityFactory::CommandToUnits(list<DynamicEntity*> units, UnitCommand unitCommand) 
{
	bool ret = false;

	list<DynamicEntity*>::const_iterator it = units.begin();

	while (it != units.end()) {
	
		if ((*it)->SetUnitCommand(unitCommand))
			ret = true;
	
		it++;
	}

	return ret;
}

bool j1EntityFactory::RemoveAllUnitsGoals(list<DynamicEntity*> units) 
{
	bool ret = false;

	list<DynamicEntity*>::const_iterator it = units.begin();

	while (it != units.end()) {

		(*it)->GetBrain()->RemoveAllSubgoals();
		ret = true;

		it++;
	}

	return ret;
}

void j1EntityFactory::InvalidateAttackEntity(Entity* entity)
{
	list<DynamicEntity*>::const_iterator it = activeDynamicEntities.begin();

	while (it != activeDynamicEntities.end()) {

		if ((*it)->IsEntityInTargetsList(entity))

			// The dead entity was a target of another entity
			(*it)->InvalidateTarget(entity);

		// The dead entity may be attacking another unit
		(*it)->RemoveAttackingUnit(entity);

		it++;
	}
}

void j1EntityFactory::InvalidateMovementEntity(Entity* entity) 
{
	list<DynamicEntity*>::const_iterator it = activeDynamicEntities.begin();

	while (it != activeDynamicEntities.end()) {

		if (!(*it)->isDead) {

			if ((*it)->GetSingleUnit()->waitUnit != nullptr) {

				// The dead entity was the waitUnit of another entity
				if ((*it)->GetSingleUnit()->waitUnit->unit == entity)

					(*it)->GetSingleUnit()->ResetUnitCollisionParameters();
			}
		}

		it++;
	}
}

// -------------------------------------------------------------
// -------------------------------------------------------------

bool j1EntityFactory::Load(pugi::xml_node& save)
{
	bool ret = true;

	pugi::xml_node node;

	list<DynamicEntity*>::const_iterator it = activeDynamicEntities.begin();

	while (it != activeDynamicEntities.end()) {
		// Load the entities
		it++;
	}

	return ret;
}

bool j1EntityFactory::Save(pugi::xml_node& save) const
{
	bool ret = true;

	pugi::xml_node node;

	list<DynamicEntity*>::const_iterator it = activeDynamicEntities.begin();

	while (it != activeDynamicEntities.end()) {
		// Save the entities
		it++;
	}

	return ret;
}