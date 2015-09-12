/******************************************************************************
* esdf
* Copyright (C) 2015  Alan Witkowski
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/
#include <objects/shot.h>
#include <objects/object.h>
#include <map.h>
#include <grid.h>
#include <buffer.h>
#include <constants.h>

_Shot::_Shot(_Object *Parent, const _ShotStat &Stats)
:	Parent(Parent) {
}

// Serialize
void _Shot::NetworkSerialize(_Buffer &Buffer) {
	Buffer.Write<glm::vec2>(Position);
	Buffer.Write<float>(Rotation);
}

// Unserialize
void _Shot::NetworkUnserialize(_Buffer &Buffer) {
	Position = Buffer.Read<glm::vec2>();
	Rotation = Buffer.Read<float>();

	CalcDirectionFromRotation();
}

// Serialize update
void _Shot::NetworkSerializeUpdate(_Buffer &Buffer, uint16_t TimeSteps) {
}

// Unserialize update
void _Shot::NetworkUnserializeUpdate(_Buffer &Buffer, uint16_t TimeSteps) {
}

// Update
void _Shot::Update(double FrameTime, uint16_t TimeSteps) {
	_Impact Impact;
	Parent->Map->Grid->CheckBulletCollisions(this, Impact);

	EndPosition = Impact.Position;
}

// Get a direction vector for degrees
void _Shot::CalcDirectionFromRotation() {
	float Radians = (Rotation - 90) / (180.0f / MATH_PI);
	Direction = glm::vec2(std::cos(Radians), std::sin(Radians));
}
