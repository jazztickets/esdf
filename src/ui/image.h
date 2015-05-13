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
#include <ui/element.h>
#include <glm/vec4.hpp>

// Forward Declarations
class _Texture;

// Classes
class _Image : public _Element {

	public:

		_Image(const std::string &Identifier, _Element *Parent, const glm::ivec2 &Offset, const glm::ivec2 &Size, const _Alignment &Alignment, const _Texture *Texture, const glm::vec4 &Color, bool Stretch);
		~_Image();

		void SetColor(const glm::vec4 &Color) { this->Color = Color; }
		const glm::vec4 &GetColor() const { return Color; }

		void SetTexture(const _Texture *Texture);
		const _Texture *GetTexture() const;

		void Render() const;

	private:

		const _Texture *Texture;
		glm::vec4 Color;
		bool Stretch;

};
