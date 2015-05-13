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
#include <vector>

// Forward Declarations
class _Font;

// Classes
class _Label : public _Element {

	public:

		_Label(const std::string &Identifier, _Element *Parent, const glm::ivec2 &Offset, const glm::ivec2 &Size, const _Alignment &Alignment, const _Font *Font, const glm::vec4 &Color, const std::string &Text);
		~_Label() override;

		void Render() const override;

		void SetWrap(float Width);

		// Attributes
		const _Font *Font;
		std::string Text;
		glm::vec4 Color;

	private:

		std::vector<std::string> Texts;

};
