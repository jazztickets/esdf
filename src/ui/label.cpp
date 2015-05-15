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
#include <ui/label.h>
#include <graphics.h>
#include <assets.h>
#include <font.h>

// Constructor
_Label::_Label() {
}

// Destructor
_Label::~_Label() {
}

// Render the element
void _Label::Render() const {
	glm::vec4 RenderColor(Color.r, Color.g, Color.b, Color.a*Fade);

	Graphics.SetProgram(Assets.Programs["pos_uv"]);
	Graphics.SetVBO(VBO_NONE);
	if(Texts.size()) {

		// Center box
		float LineHeight = Font->GetMaxHeight() + 2;
		float Y = Bounds.Start.y - (int)((LineHeight * Texts.size() - LineHeight) / 2);
		for(size_t i = 0; i < Texts.size(); i++) {
			Font->DrawText(Texts[i], glm::vec2(Bounds.Start.x, Y), RenderColor, Alignment);

			Y += LineHeight;
		}
	}
	else {
		Font->DrawText(Text, Bounds.Start, RenderColor, Alignment);
	}

	_Element::Render();
}

// Break up text into multiple strings
void _Label::SetWrap(float Width) {

	Texts.clear();
	Font->BreakupString(Text, Width, Texts);
}
