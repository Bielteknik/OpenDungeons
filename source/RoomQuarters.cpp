/*
 *  Copyright (C) 2011-2014  OpenDungeons Team
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "RoomQuarters.h"

#include "ODServer.h"
#include "Tile.h"
#include "GameMap.h"
#include "RoomObject.h"
#include "Creature.h"

RoomQuarters::RoomQuarters()
{
    mType = quarters;
}

void RoomQuarters::absorbRoom(Room *r)
{
    // Start by deleting the Ogre meshes associated with both rooms.
    destroyMesh();
    destroyBedMeshes();
    r->destroyMesh();
    //Added a check here - didn't look safe - oln 17/03/2011
    if(r->getType() == quarters)
    {
        static_cast<RoomQuarters*>(r)->destroyBedMeshes();
    }

    // Copy over the information about the creatures that are sleeping in the other quarters before we remove its rooms.
    for (unsigned int i = 0; i < r->numCoveredTiles(); ++i)
    {
        Tile *tempTile = r->getCoveredTile(i);

        if (static_cast<RoomQuarters*>(r)->mCreatureSleepingInTile[tempTile] != NULL)
            std::cout << "\nCreature sleeping in tile " << tempTile << "\n"
                    << static_cast<RoomQuarters*>(r)->mCreatureSleepingInTile[tempTile];
        else
            std::cout << "\nCreature sleeping in tile " << tempTile << "\nNULL";

        std::cout << "\n";
        mCreatureSleepingInTile[tempTile] = static_cast<RoomQuarters*>(r)->mCreatureSleepingInTile[tempTile];

        if (static_cast<RoomQuarters*>(r)->mBedOrientationForTile.find(tempTile)
                != static_cast<RoomQuarters*>(r)->mBedOrientationForTile.end())
            mBedOrientationForTile[tempTile] = static_cast<RoomQuarters*>(r)->mBedOrientationForTile[tempTile];
    }

    // Use the superclass function to copy over the covered tiles to this room and get rid of them in the other room.
    Room::absorbRoom(r);

    // Recreate the meshes for this new room which contains both rooms.
    createMesh();

    createRoomObjectMeshes();
}

bool RoomQuarters::doUpkeep()
{
    // Call the super class Room::doUpkeep() function to do any generic upkeep common to all rooms.
    return Room::doUpkeep();
}

void RoomQuarters::addCoveredTile(Tile* t, double nHP)
{
    Room::addCoveredTile(t, nHP);

    // Only initialize the tile to NULL if it is a tile being added to a new room.  If it is being absorbed
    // from another room the map value will already have been set and we don't want to override it.
    if (mCreatureSleepingInTile.find(t) == mCreatureSleepingInTile.end())
        mCreatureSleepingInTile[t] = NULL;
}

void RoomQuarters::removeCoveredTile(Tile* t)
{
    if (mCreatureSleepingInTile[t] != NULL)
    {
        Creature *c = mCreatureSleepingInTile[t];
        if (c != NULL) // This check is probably redundant but I don't think it is a problem.
        {
            // Inform the creature that it no longer has a place to sleep.
            c->setHomeTile(NULL);

            // Loop over all the tiles in this room and if they are slept on by creature c then set them back to NULL.
            for (std::map<Tile*, Creature*>::iterator itr = mCreatureSleepingInTile.begin();
                 itr != mCreatureSleepingInTile.end(); ++itr)
            {
                if (itr->second == c)
                    itr->second = NULL;
            }
        }

        //roomObjects[t]->destroyMesh();
    }

    mCreatureSleepingInTile.erase(t);
    mBedOrientationForTile.erase(t);
    Room::removeCoveredTile(t);
}

void RoomQuarters::clearCoveredTiles()
{
    Room::clearCoveredTiles();
    mCreatureSleepingInTile.clear();
}

std::vector<Tile*> RoomQuarters::getOpenTiles()
{
    std::vector<Tile*> returnVector;

    for (std::map<Tile*, Creature*>::iterator itr = mCreatureSleepingInTile.begin();
         itr != mCreatureSleepingInTile.end(); ++itr)
    {
        if (itr->second == NULL)
            returnVector.push_back(itr->first);
    }

    return returnVector;
}

bool RoomQuarters::claimTileForSleeping(Tile *t, Creature *c)
{
    // Check to see if there is already a creature which has claimed this tile for sleeping.
    if (mCreatureSleepingInTile[t] == NULL)
    {
        double xDim, yDim, rotationAngle;
        bool normalDirection, spaceIsBigEnough = false;

        // Check to see whether the bed should be situated x-by-y or y-by-x tiles.
        if (tileCanAcceptBed(t, c->getDefinition()->getBedDim1(), c->getDefinition()->getBedDim2()))
        {
            normalDirection = true;
            spaceIsBigEnough = true;
            xDim = c->getDefinition()->getBedDim1();
            yDim = c->getDefinition()->getBedDim2();
            rotationAngle = 0.0;
        }

        if (!spaceIsBigEnough && tileCanAcceptBed(t, c->getDefinition()->getBedDim2(), c->getDefinition()->getBedDim1()))
        {
            normalDirection = false;
            spaceIsBigEnough = true;
            xDim = c->getDefinition()->getBedDim2();
            yDim = c->getDefinition()->getBedDim1();
            rotationAngle = 90.0;
        }

        if (spaceIsBigEnough)
        {
            // Mark all of the affected tiles as having this creature sleeping in them.
            for (int i = 0; i < xDim; ++i)
            {
                for (int j = 0; j < yDim; ++j)
                {
                    Tile *tempTile = getGameMap()->getTile(t->x + i, t->y + j);
                    mCreatureSleepingInTile[tempTile] = c;
                }
            }

            mBedOrientationForTile[t] = normalDirection;

            const CreatureDefinition* def = c->getDefinition();
            assert(def);

            loadRoomObject(def->getBedMeshName(), t, t->x + xDim / 2.0 - 0.5, t->y
                           + yDim / 2.0 - 0.5, rotationAngle)->createMesh();

            return true;
        }
    }

    return false;
}

bool RoomQuarters::releaseTileForSleeping(Tile *t, Creature *c)
{
    if (mCreatureSleepingInTile[t] == NULL)
        return false;

    // Loop over all the tiles in this room and if they are slept on by creature c then set them back to NULL.
    for (std::map<Tile*, Creature*>::iterator itr = mCreatureSleepingInTile.begin();
         itr != mCreatureSleepingInTile.end(); ++itr)
    {
        if (itr->second == c)
            itr->second = NULL;
    }

    mBedOrientationForTile.erase(t);

    mRoomObjects[t]->destroyMesh();

    return true;
}

Tile* RoomQuarters::getLocationForBed(int xDim, int yDim)
{
    // Force the dimensions to be positive.
    if (xDim < 0)
        xDim *= -1;
    if (yDim < 0)
        yDim *= -1;

    // Check to see if there is even enough space available for the bed.
    std::vector<Tile*> tempVector = getOpenTiles();
    unsigned int area = xDim * yDim;
    if (tempVector.size() < area)
        return NULL;

    // Randomly shuffle the open tiles in tempVector so that the quarters are filled up in a random order.
    std::random_shuffle(tempVector.begin(), tempVector.end());

    // Loop over each of the open tiles in tempVector and for each one, check to see if it
    for (unsigned int i = 0; i < tempVector.size(); ++i)
    {
        if (tileCanAcceptBed(tempVector[i], xDim, yDim))
            return tempVector[i];
    }

    // We got to the end of the open tile list without finding an open tile for the bed so return NULL to indicate failure.
    return NULL;
}

bool RoomQuarters::tileCanAcceptBed(Tile *tile, int xDim, int yDim)
{
    //TODO: This function could be made more efficient by making it take the list of open tiles as an argument so if it is called repeatedly the tempTiles vecotor below only has to be computed once in the calling function rather than N times in this function.

    // Force the dimensions to be positive.
    if (xDim < 0)
        xDim *= -1;
    if (yDim < 0)
        yDim *= -1;

    // If either of the dimensions is 0 just return true, since the bed takes no space.  This should never really happen anyway.
    if (xDim == 0 || yDim == 0)
        return true;

    // If the tile is invalid or not part of this room then the bed cannot be placed in this room.
    if (tile == NULL || tile->getCoveringRoom() != this)
        return false;

    // Create a 2 dimensional array of booleans initially all set to false.
    std::vector<std::vector<bool> > tileOpen(xDim);
    for (int i = 0; i < xDim; ++i)
    {
        tileOpen[i].resize(yDim, false);
    }

    // Now loop over the list of all the open tiles in this quarters.  For each tile, if it falls within
    // the xDim by yDim area from the starting tile we set the corresponding tileOpen entry to true.
    std::vector<Tile*> tempTiles = getOpenTiles();
    for (unsigned int i = 0; i < tempTiles.size(); ++i)
    {
        int xDist = tempTiles[i]->x - tile->x;
        int yDist = tempTiles[i]->y - tile->y;
        if (xDist >= 0 && xDist < xDim && yDist >= 0 && yDist < yDim)
            tileOpen[xDist][yDist] = true;
    }

    // Loop over the tileOpen array and check to see if every value has been set to true, if it has then
    // we can place the a bed of the specified dimensions with its corner at the specified starting tile.
    bool returnValue = true;
    for (int i = 0; i < xDim; ++i)
    {
        for (int j = 0; j < yDim; ++j)
        {
            returnValue = returnValue && tileOpen[i][j];
        }
    }

    return returnValue;
}

void RoomQuarters::destroyBedMeshes()
{
    destroyRoomObjectMeshes();
}
