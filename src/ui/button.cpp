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
#include <ui/button.h>
#include <ui/label.h>
#include <ui/style.h>
#include <assets.h>
#include <texture.h>
#include <atlas.h>
#include <graphics.h>

// Constructor
_Button::_Button(const std::string &Identifier, _Element *Parent, const glm::ivec2 &Offset, const glm::ivec2 &Size, const _Alignment &Alignment, const _Style *Style, const _Style *HoverStyle)
:	_Element(Identifier, Parent, Offset, Size, Alignment, Style, false),
	HoverStyle(HoverStyle),
	Enabled(false),
	TextureIndex(0) {

}

// Destructor
_Button::~_Button() {
}

// Render the element
void _Button::Render() const {

	if(Style) {

		if(Style->Texture) {
			Graphics.SetProgram(Style->Program);
			Graphics.SetVBO(VBO_NONE);
			Graphics.DrawImage(Bounds, Style->Texture, Style->TextureColor, Style->Stretch);
		}
		else if(Style->Atlas) {
			Graphics.SetProgram(Style->Program);
			Graphics.SetVBO(VBO_NONE);
			Graphics.DrawAtlas(Bounds, Style->Atlas->Texture, Style->Atlas->GetTextureCoords(TextureIndex), Style->TextureColor);
		}
		else {
			Graphics.SetProgram(Assets.Programs["ortho_pos"]);
			Graphics.SetVBO(VBO_NONE);
			Graphics.DrawRectangle(Bounds, Style->BackgroundColor, true);
			Graphics.DrawRectangle(Bounds, Style->BorderColor, false);
		}
	}

	// Draw hover texture
	if(HoverStyle && (Enabled || HitElement)) {

		if(HoverStyle->Texture) {
			Graphics.SetProgram(HoverStyle->Program);
			Graphics.SetVBO(VBO_NONE);
			Graphics.DrawImage(Bounds, HoverStyle->Texture, HoverStyle->TextureColor, Style->Stretch);
		}
		else {
			Graphics.SetProgram(Assets.Programs["ortho_pos"]);
			Graphics.SetVBO(VBO_NONE);
			if(HoverStyle->HasBackgroundColor)
				Graphics.DrawRectangle(Bounds, HoverStyle->BackgroundColor, true);

			if(HoverStyle->HasBorderColor)
				Graphics.DrawRectangle(Bounds, HoverStyle->BorderColor, false);
		}
	}

	// Render all children
	for(size_t i = 0; i < Children.size(); i++) {
		Children[i]->Render();
	}
}

// Handle pressed event
void _Button::HandleInput(bool Pressed) {
	if(HitElement) {
	}
}
