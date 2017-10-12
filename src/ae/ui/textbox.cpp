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
#include <ae/ui/textbox.h>
#include <ae/font.h>
#include <ae/input.h>
#include <SDL_keycode.h>

// Constructor
_TextBox::_TextBox() :
	Focused(false),
	Font(nullptr),
	MaxLength(0),
	DrawCursor(true),
	CursorTimer(0) {
}

// Destructor
_TextBox::~_TextBox() {
}

// Update cursor
void _TextBox::Update(double FrameTime, const glm::ivec2 &Mouse) {
	_Element::Update(FrameTime, Mouse);

	if(Focused) {
		CursorTimer += FrameTime;
		if(CursorTimer > 0.5) {
			CursorTimer = 0;
			DrawCursor = !DrawCursor;
		}
	}
}

// Handle pressed event
void _TextBox::HandleInput(bool Pressed) {
	_Element::HandleInput(Pressed);

	if(HitElement) {
		Focused = true;
		ResetCursor();
	}
	else {
		Focused = false;
	}
}

// Handle key event
void _TextBox::HandleKeyEvent(const _KeyEvent &KeyEvent) {

	if(Focused && KeyEvent.Pressed) {
		if(KeyEvent.Scancode == SDL_SCANCODE_BACKSPACE && Text.length() > 0) {
			Text.erase(Text.length() - 1, 1);
			ResetCursor();
		}
	}
}

// Handle text event
void _TextBox::HandleTextEvent(const char *Text) {
	if(Focused && this->Text.length() < MaxLength && Text[0] >= 32 && Text[0] <= 126) {
		this->Text += Text[0];
		ResetCursor();
	}
}

// Render the element
void _TextBox::Render() const {
	std::string RenderText;
	if(DrawCursor && Focused)
		RenderText = Text + "|";
	else
		RenderText = Text;

	_Element::Render();

	Font->DrawText(RenderText, glm::vec2(Bounds.Start) + glm::vec2(5.0f, 20.0f), LEFT_BASELINE, glm::vec4(1.0f));
}
