/*
 *  CreatureSound.cpp
 *
 *  Created on: 24. feb. 2011
 *  Author: oln
 *
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

#include "CreatureSound.h"

CreatureSound::CreatureSound()
{
    //Reserve space for sound objects
    mSounds.assign(NUM_CREATURE_SOUNDS, sf::Sound());
}

//! \brief Play the wanted sound.
void CreatureSound::play(SoundType type)
{
    mSounds[type].stop();
    mSounds[type].play();
}

void CreatureSound::setPosition(Ogre::Vector3 p)
{
    setPosition(p.x, p.y, p.z);
}

/*! \brief Set the play position for the sound source.
 *
 */
void CreatureSound::setPosition(float x, float y, float z)
{
    SoundEffectsHelper::SoundFXVector::iterator it;
    for (it = mSounds.begin(); it != mSounds.end(); ++it)
    {
        it->setPosition(x, y, z);
    }
}

