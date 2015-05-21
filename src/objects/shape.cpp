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
#include <objects/shape.h>
#include <buffer.h>

// Constructor
_Shape::_Shape(_Object *Parent, const _ShapeStat &Stat) :
	Parent(Parent),
	Stat(Stat),
	Type(CIRCLE) {
}

// Destructor
_Shape::~_Shape() {
}

// Serialize
void _Shape::NetworkSerialize(_Buffer &Buffer) {
}

// Unserialize
void _Shape::NetworkUnserialize(_Buffer &Buffer) {
}

// Serialize update
void _Shape::NetworkSerializeUpdate(_Buffer &Buffer, uint16_t TimeSteps) {
}

// Unserialize update
void _Shape::NetworkUnserializeUpdate(_Buffer &Buffer, uint16_t TimeSteps) {
}