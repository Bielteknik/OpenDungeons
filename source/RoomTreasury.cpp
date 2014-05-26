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

#include "RoomTreasury.h"

#include "RenderRequest.h"
#include "Tile.h"
#include "RenderManager.h"

#include <string>

RoomTreasury::RoomTreasury()
{
    mType = treasury;
}

void RoomTreasury::absorbRoom(Room *r)
{
    // Start by deleting the Ogre meshes associated with both rooms.
    destroyMesh();
    destroyGoldMeshes();
    r->destroyMesh();

    // Copy over the information about the gold that is stored in the other treasury before we remove its rooms.
    for (unsigned int i = 0; i < r->numCoveredTiles(); ++i)
    {
        Tile *tempTile = r->getCoveredTile(i);
        mGoldInTile[tempTile] = static_cast<RoomTreasury*>(r)->mGoldInTile[tempTile];
        mFullnessOfTile[tempTile] = static_cast<RoomTreasury*>(r)->mFullnessOfTile[tempTile];
    }

    // Use the superclass function to copy over the covered tiles to this room and get rid of them in the other room.
    Room::absorbRoom(r);

    // Recreate the meshes for this new room which contains both rooms.
    createMesh();

    // Recreate the gold indicators which were destroyed when the meshes were cleared.
    createGoldMeshes();
}

bool RoomTreasury::doUpkeep()
{
    // Call the super class Room::doUpkeep() function to do any generic upkeep common to all rooms.
    return Room::doUpkeep();
}

void RoomTreasury::addCoveredTile(Tile* t, double nHP)
{
    Room::addCoveredTile(t, nHP);

    // Only initialize the tile to empty if it has not already been set by the absorbRoom() function.
    if (mGoldInTile.find(t) == mGoldInTile.end())
    {
        mGoldInTile[t] = 0;
        mFullnessOfTile[t] = empty;
    }
}

void RoomTreasury::removeCoveredTile(Tile* t)
{
    Room::removeCoveredTile(t);
    mGoldInTile.erase(t);
    mFullnessOfTile.erase(t);

    //TODO:  When the tile contains gold we need to put it on the map as an item which can be picked up.
}

void RoomTreasury::clearCoveredTiles()
{
    Room::clearCoveredTiles();
    mGoldInTile.clear();
    mFullnessOfTile.clear();
}

int RoomTreasury::getTotalGold()
{
    int tempInt = 0;

    for (std::map<Tile*, int>::iterator itr = mGoldInTile.begin();
            itr != mGoldInTile.end(); ++itr)
        tempInt += (*itr).second;

    return tempInt;
}

int RoomTreasury::emptyStorageSpace()
{
    return numCoveredTiles() * maxGoldWhichCanBeStoredInAChest - getTotalGold();
}

int RoomTreasury::depositGold(int gold, Tile *tile)
{
    int goldDeposited, goldToDeposit = gold, emptySpace;

    // Start by trying to deposit the gold in the requested tile.
    emptySpace = maxGoldWhichCanBeStoredInAChest - mGoldInTile[tile];
    goldDeposited = std::min(emptySpace, goldToDeposit);
    mGoldInTile[tile] += goldDeposited;
    goldToDeposit -= goldDeposited;
    updateMeshesForTile(tile);

    // If there is still gold left to deposit after the first tile, loop over all of the tiles and see if we can put the gold in another tile.
    for (std::map<Tile*, int>::iterator itr = mGoldInTile.begin();
         itr != mGoldInTile.end() && goldToDeposit > 0; ++itr)
    {
        // Store as much gold as we can in this tile.
        emptySpace = maxGoldWhichCanBeStoredInAChest - itr->second;
        goldDeposited = std::min(emptySpace, goldToDeposit);
        itr->second += goldDeposited;
        goldToDeposit -= goldDeposited;
        updateMeshesForTile(itr->first);
    }

    // Return the amount we were actually able to deposit (i.e. the amount we wanted to deposit minus the amount we were unable to deposit).
    return gold - goldToDeposit;
}

int RoomTreasury::withdrawGold(int gold)
{
    int withdrawlAmount = 0;
    for (std::map<Tile*, int>::iterator itr = mGoldInTile.begin();
         itr != mGoldInTile.end(); ++itr)
    {
        // Check to see if the current room tile has enough gold in it to fill the amount we still need to pick up.
        int goldStillNeeded = gold - withdrawlAmount;
        if (itr->second > goldStillNeeded)
        {
            // There is enough to satisfy the request so we do so and exit the loop.
            withdrawlAmount += goldStillNeeded;
            itr->second -= goldStillNeeded;
            updateMeshesForTile(itr->first);
            break;
        }
        else
        {
            // There is not enough to satisfy the request so take everything there is and move on to the next tile.
            withdrawlAmount += itr->second;
            itr->second = 0;
            updateMeshesForTile(itr->first);
        }
    }

    return withdrawlAmount;
}

RoomTreasury::TreasuryTileFullness RoomTreasury::getTreasuryTileFullness(int gold)
{
    if (gold <= 0)
        return empty;

    if (gold <= maxGoldWhichCanBeStoredInABag)
        return bag;

    if (gold <= maxGoldWhichCanBeStoredInAChest)
        return chest;

    return overfull;
}

const char* RoomTreasury::getMeshNameForTreasuryTileFullness(
        TreasuryTileFullness fullness)
{
    switch (fullness)
    {
        // The empty case should really never happen since we shouldn't be creating meshes for an empty tile anyway.
        case empty:
            return "TreasuryTileEmptyError";

        case bag:
            return "GoldBag";

        case chest:
            return "GoldChest";

        case overfull:
            return "TreasuryTileOverfullError";

        default:
            return "TreasuryTileFullnessMeshError";
    }
}

void RoomTreasury::updateMeshesForTile(Tile* t)
{
    TreasuryTileFullness newFullness = getTreasuryTileFullness(mGoldInTile[t]);

    // If the mesh has not changed we do not need to do anything.
    if (mFullnessOfTile[t] == newFullness)
        return;

    // Since the fullness level has changed we need to destroy the existing indicator mesh (if it exists) and create a new one.
    destroyMeshesForTile(t);
    mFullnessOfTile[t] = newFullness;
    createMeshesForTile(t);
}

void RoomTreasury::createMeshesForTile(Tile* t)
{
    // If the tile is empty, there is no indicator mesh to create so we just return.
    if (mFullnessOfTile[t] == empty)
        return;

    std::string indicatorMeshName = getMeshNameForTreasuryTileFullness(mFullnessOfTile[t]);

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::createTreasuryIndicator;
    request->p = t;
    request->p2 = this;
    request->str = indicatorMeshName;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}

void RoomTreasury::destroyMeshesForTile(Tile* t)
{
    // If the tile is empty, there is no indicator mesh to destroy so we just return.
    if (mFullnessOfTile[t] == empty)
        return;

    std::string indicatorMeshName = getMeshNameForTreasuryTileFullness(mFullnessOfTile[t]);

    RenderRequest *request = new RenderRequest;
    request->type = RenderRequest::destroyTreasuryIndicator;
    request->p = t;
    request->p2 = this;
    request->str = indicatorMeshName;

    // Add the request to the queue of rendering operations to be performed before the next frame.
    RenderManager::queueRenderRequest(request);
}

void RoomTreasury::createGoldMeshes()
{
    for (unsigned int i = 0; i < numCoveredTiles(); ++i)
        createMeshesForTile(getCoveredTile(i));
}

void RoomTreasury::destroyGoldMeshes()
{
    for (unsigned int i = 0; i < numCoveredTiles(); ++i)
        destroyMeshesForTile(getCoveredTile(i));
}
