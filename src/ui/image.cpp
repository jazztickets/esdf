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
#include <ui/image.h>
#include <texture.h>
#include <graphics.h>
#include <assets.h>

// Constructor
_Image::_Image(const std::string &Identifier, _Element *Parent, const glm::ivec2 &Offset, const glm::ivec2 &Size, const _Alignment &Alignment, const _Texture *Texture, const glm::vec4 &Color, bool Stretch) :
	_Element(Identifier, Parent, Offset, Size, Alignment, nullptr, false),
	Texture(Texture),
	Color(Color),
	Stretch(Stretch) {
}

// Destructor
_Image::~_Image() {
}

// Render the element
void _Image::Render() const {

	if(Texture) {
		Graphics.SetProgram(Assets.Programs["ortho_pos_uv"]);
		Graphics.SetVBO(VBO_NONE);
		Graphics.DrawImage(Bounds, Texture, Color, Stretch);
	}
	else {
		Graphics.SetProgram(Assets.Programs["ortho_pos"]);
		Graphics.SetVBO(VBO_NONE);
		Graphics.DrawRectangle(Bounds, Color, Stretch);
	}

	// Draw children
	_Element::Render();
}

// Set texture
void _Image::SetTexture(const _Texture *Texture) {
	this->Texture = Texture;
}

// Get texture
const _Texture *_Image::GetTexture() const {
	return Texture;
}
