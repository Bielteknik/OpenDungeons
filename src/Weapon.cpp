#include "Globals.h"
#include "Functions.h"
#include "Weapon.h"
#include "RenderRequest.h"
#include "Creature.h"

Weapon::Weapon()
{
	sem_init(&meshCreationFinishedSemaphore, 0, 0);
	sem_init(&meshDestructionFinishedSemaphore, 0, 0);
}

void Weapon::createMesh()
{
	if(name.compare("none") == 0)
	{
		return;
	}

	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::createWeapon;
	request->p = this;
	request->p2 = parentCreature;
	request->p3 = &handString;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);
}

void Weapon::destroyMesh()
{
	RenderRequest *request = new RenderRequest;
	request->type = RenderRequest::destroyWeapon;
	request->p = this;
	request->p2 = parentCreature;

	// Add the request to the queue of rendering operations to be performed before the next frame.
	queueRenderRequest(request);

	//FIXME:  This waits forever however it should be enabled to prevent a race condition.
	//sem_wait(&meshDestructionFinishedSemaphore);
}

ostream& operator<<(ostream& os, Weapon *w)
{
	os << w->name << "\t" << w->damage << "\t" << w->range << "\t" << w->defense;

	return os;
}

istream& operator>>(istream& is, Weapon *w)
{
	is >> w->name >> w->damage >> w->range >> w->defense;
	w->meshName = w->name + ".mesh";

	return is;
}

