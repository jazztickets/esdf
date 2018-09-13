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
#include <glm/vec4.hpp>

// Forward Declarations
struct _RenderStat;

namespace ae {
	class _Program;
	class _Mesh;
	class _Texture;
}

// Classes
class _Render : public _Component {

	public:

		enum DebugType {
			DEBUG_NETWORK = 0x01,
			DEBUG_HISTORY = 0x02,
			DEBUG_ID      = 0x04,
			DEBUG_ALL     = 0xFF,
		};

		_Render(_Object *Parent, const _RenderStat *Stats);
		~_Render();

		// Network
		void NetworkSerialize(ae::_Buffer &Buffer) override;
		void NetworkUnserialize(ae::_Buffer &Buffer) override;

		void Draw3D(double BlendFactor);

		const _RenderStat *Stats;
		const ae::_Program *Program;
		const ae::_Texture *Texture;
		const ae::_Mesh *Mesh;

		// Attributes
		glm::vec4 Color;
		uint8_t Debug;
};
