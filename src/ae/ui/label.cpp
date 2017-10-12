/******************************************************************************
* Copyright (c) 2017 Alan Witkowski
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
*
* 1. The origin of this software must not be misrepresented; you must not
*    claim that you wrote the original software. If you use this software
*    in a product, an acknowledgment in the product documentation would be
*    appreciated but is not required.
* 2. Altered source versions must be plainly marked as such, and must not be
*    misrepresented as being the original software.
* 3. This notice may not be removed or altered from any source distribution.
*******************************************************************************/
#include <ae/ui/label.h>
#include <ae/graphics.h>
#include <ae/assets.h>
#include <ae/font.h>

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
		float LineHeight = 22;
		float Y = Bounds.Start.y - (int)((LineHeight * Texts.size() - LineHeight) / 2);
		for(size_t i = 0; i < Texts.size(); i++) {
			Font->DrawText(Texts[i], glm::vec2(Bounds.Start.x, Y), Alignment, RenderColor);

			Y += LineHeight;
		}
	}
	else {
		Font->DrawText(Text, Bounds.Start, Alignment, RenderColor);
	}

	_Element::Render();
}

// Break up text into multiple strings
void _Label::SetWrap(float Width) {

	Texts.clear();
	//Font->BreakupString(Text, Width, Texts);
}
