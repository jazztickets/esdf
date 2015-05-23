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
#pragma once

// Libraries
#include <glm/vec4.hpp>

// Forward Declarations
class _Object;
class _Program;
class _Texture;
class _Mesh;
class _Buffer;
struct _RenderStat;

// Classes
class _Render {

	public:

		_Render(_Object *Parent, const _RenderStat &Stats);
		~_Render();

		// Network
		void NetworkSerialize(_Buffer &Buffer);
		void NetworkUnserialize(_Buffer &Buffer);

		void Draw3D(double BlendFactor);

		_Object *Parent;
		const _RenderStat &Stats;
		const _Program *Program;
		const _Texture *Texture;
		const _Mesh *Mesh;

		// Attributes
		glm::vec4 Color;
};
