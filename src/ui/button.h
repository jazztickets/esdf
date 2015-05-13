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

// Forward Declarations
class _Style;

// Classes
class _Button : public _Element {

	public:

		_Button(const std::string &Identifier, _Element *Parent, const glm::ivec2 &Offset, const glm::ivec2 &Size, const _Alignment &Alignment, const _Style *Style, const _Style *HoverStyle);
		~_Button();

		void HandleInput(bool Pressed);
		void Render() const;

		void SetEnabled(bool Enabled) { this->Enabled = Enabled; }
		bool GetEnabled() const { return Enabled; }

		void SetTextureIndex(int TextureIndex) { this->TextureIndex = TextureIndex; }
		int GetTextureIndex() const { return TextureIndex; }

	private:

		const _Style *HoverStyle;
		bool Enabled;
		int TextureIndex;
};
