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
#include <string>

// Forward Declarations
class _Texture;
class _Atlas;
class _Program;

// Classes
class _Style {

	public:

		_Style(const std::string &Identifier,
			   bool HasBackgroundColor,
			   bool HasBorderColor,
			   const glm::vec4 &BackgroundColor,
			   const glm::vec4 &BorderColor,
			   const _Program *Program,
			   const _Texture *Texture,
			   const _Atlas *Atlas,
			   const glm::vec4 &TextureColor,
			   bool Stretch)
			:
			Identifier(Identifier),
			BackgroundColor(BackgroundColor),
			BorderColor(BorderColor),
			HasBackgroundColor(HasBackgroundColor),
			HasBorderColor(HasBorderColor),
			Program(Program),
			Texture(Texture),
			Atlas(Atlas),
			TextureColor(TextureColor),
			Stretch(Stretch) {
		}

		~_Style() { }

		// Attributes
		std::string Identifier;

		// Colors
		glm::vec4 BackgroundColor;
		glm::vec4 BorderColor;
		bool HasBackgroundColor;
		bool HasBorderColor;

		// Shaders
		const _Program *Program;
		const _Texture *Texture;
		const _Atlas *Atlas;
		glm::vec4 TextureColor;

		bool Stretch;

	private:

};
