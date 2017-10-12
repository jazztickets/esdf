/******************************************************************************
* esdf
* Copyright (C) 2017  Alan Witkowski
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
#pragma once

// Libraries
#include <objects/component.h>

// Forward Declarations
struct _HealthStat;

// Classes
class _Health : public _Component {

	public:

		_Health(_Object *Parent, const _HealthStat *Stats);
		~_Health();

		// Network
		void NetworkSerialize(_Buffer &Buffer) override;
		void NetworkUnserialize(_Buffer &Buffer) override;

		// Attributes
		int MaxHealth;
		int Health;

};
