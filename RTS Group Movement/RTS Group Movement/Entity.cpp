#include "j1App.h"
#include "Entity.h"

#include "j1Collision.h"
#include "j1Render.h"
#include "j1Map.h"

Entity::Entity(EntityInfo entityInfo) :entityInfo(entityInfo) 
{
}

Entity::~Entity()
{
	if (collider != nullptr)
		collider->to_delete = true;
}

const Collider* Entity::GetCollider() const
{
	return collider;
}

void Entity::Draw(SDL_Texture* sprites)
{
	/*
	if (animation != nullptr)
		App->render->Blit(texture, position.x, position.y, &(animation->GetCurrentFrame()));
	*/

	if (isSelected)
		DrawSelected();

	DebugDraw(sprites);
}

void Entity::DebugDraw(SDL_Texture* sprites)
{
}

void Entity::DrawSelected() 
{
	const SDL_Rect entitySize = { entityInfo.pos.x, entityInfo.pos.y, entityInfo.size.x, entityInfo.size.y };
	App->render->DrawQuad(entitySize, 255, 255, 255, 255, false);
}

void Entity::OnCollision(Collider* c1, Collider* c2) {}