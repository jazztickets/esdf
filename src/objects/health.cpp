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
#include <objects/health.h>
#include <stats.h>
#include <buffer.h>

// Constructor
_Health::_Health(_Object *Parent, const _HealthStat *Stat) :
	_Component(Parent),
	MaxHealth(Stat->Health),
	Health(Stat->Health) {
}

// Destructor
_Health::~_Health() {
}

// Serialize
void _Health::NetworkSerialize(_Buffer &Buffer) {
	Buffer.Write<int>(MaxHealth);
	Buffer.Write<int>(Health);
}

// Unserialize
void _Health::NetworkUnserialize(_Buffer &Buffer) {
	MaxHealth = Buffer.Read<int>();
	Health = Buffer.Read<int>();
}
