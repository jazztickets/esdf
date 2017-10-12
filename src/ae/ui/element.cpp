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
#include <ae/ui/element.h>
#include <ae/ui/style.h>
#include <ae/graphics.h>
#include <ae/input.h>
#include <ae/assets.h>
#include <constants.h>

const glm::vec4 DebugColors[] = { COLOR_CYAN, COLOR_YELLOW, COLOR_RED, COLOR_GREEN, COLOR_BLUE };
const int DebugColorCount = sizeof(DebugColors) / sizeof(glm::vec4);

// Constructor
_Element::_Element() :
	GlobalID(0),
	Parent(nullptr),
	UserData(nullptr),
	MaskOutside(false),
	Debug(0),
	Style(nullptr),
	Fade(1.0f),
	HitElement(nullptr),
	PressedElement(nullptr),
	ReleasedElement(nullptr) {

}

// Destructor
_Element::~_Element() {
}

// Handle key event
void _Element::HandleKeyEvent(const _KeyEvent &KeyEvent) {

	// Pass event to children
	for(size_t i = 0; i < Children.size(); i++)
		Children[i]->HandleKeyEvent(KeyEvent);
}

// Handle text event
void _Element::HandleTextEvent(const char *Text) {

	// Pass event to children
	for(size_t i = 0; i < Children.size(); i++)
		Children[i]->HandleTextEvent(Text);
}

// Handle a press event
void _Element::HandleInput(bool Pressed) {

	// Pass event to children
	for(size_t i = 0; i < Children.size(); i++)
		Children[i]->HandleInput(Pressed);

	// Set pressed element
	if(Pressed)
		PressedElement = HitElement;

	// Get released element
	if(!Pressed && PressedElement && HitElement) {
		ReleasedElement = PressedElement;
		PressedElement = nullptr;
	}
}

// Get the element that was clicked and released
_Element *_Element::GetClickedElement() {
	if(HitElement == ReleasedElement)
		return HitElement;

	return nullptr;
}

// Handle mouse movement
void _Element::Update(double FrameTime, const glm::ivec2 &Mouse) {
	HitElement = nullptr;
	ReleasedElement = nullptr;

	// Test element first
	if(Bounds.PointInside(Mouse)) {
		HitElement = this;
	}
	else if(MaskOutside) {
		HitElement = nullptr;
		return;
	}

	// Test children
	for(size_t i = 0; i < Children.size(); i++) {
		Children[i]->Update(FrameTime, Mouse);
		if(Children[i]->HitElement)
			HitElement = Children[i]->HitElement;
	}
}

// Calculate the screen space bounds for the element
void _Element::CalculateBounds() {
	Bounds.Start = Offset;

	// Handle horizontal alignment
	switch(Alignment.Horizontal) {
		case _Alignment::CENTER:
			if(Parent)
				Bounds.Start.x += Parent->Size.x / 2;
			Bounds.Start.x -= Size.x / 2;
		break;
		case _Alignment::RIGHT:
			if(Parent)
				Bounds.Start.x += Parent->Size.x;
			Bounds.Start.x -= Size.x;
		break;
	}

	// Handle vertical alignment
	switch(Alignment.Vertical) {
		case _Alignment::MIDDLE:
			if(Parent)
				Bounds.Start.y += Parent->Size.y / 2;
			Bounds.Start.y -= Size.y / 2;
		break;
		case _Alignment::BOTTOM:
			if(Parent)
				Bounds.Start.y += Parent->Size.y;
			Bounds.Start.y -= Size.y;
		break;
	}

	// Offset from parent
	if(Parent)
		Bounds.Start += Parent->Bounds.Start + Parent->ChildrenOffset;

	// Set end
	Bounds.End = Bounds.Start + Size;

	// Update children
	CalculateChildrenBounds();
}

// Update children bounds
void _Element::CalculateChildrenBounds() {

	// Update children
	for(size_t i = 0; i < Children.size(); i++)
		Children[i]->CalculateBounds();
}

// Render the element
void _Element::Render() const {

	if(MaskOutside) {
		Graphics.SetProgram(Assets.Programs["ortho_pos"]);
		Graphics.EnableStencilTest();
		Graphics.DrawMask(Bounds);
	}

	if(Style) {
		Graphics.SetProgram(Style->Program);
		Graphics.SetVBO(VBO_NONE);
		if(Style->HasBackgroundColor) {
			glm::vec4 RenderColor(Style->BackgroundColor);
			RenderColor.a *= Fade;
			Graphics.SetColor(RenderColor);
			Graphics.DrawRectangle(Bounds, true);
		}

		if(Style->HasBorderColor) {
			glm::vec4 RenderColor(Style->BorderColor);
			RenderColor.a *= Fade;
			Graphics.SetColor(RenderColor);
			Graphics.DrawRectangle(Bounds, false);
		}
	}

	// Render all children
	for(size_t i = 0; i < Children.size(); i++) {
		Children[i]->Fade = Fade;
		Children[i]->Render();
	}

	if(MaskOutside)
		Graphics.DisableStencilTest();

	if(Debug && Debug-1 < DebugColorCount) {
		Graphics.SetProgram(Assets.Programs["ortho_pos"]);
		Graphics.SetVBO(VBO_NONE);
		Graphics.SetColor(DebugColors[1]);
		Graphics.DrawRectangle(Bounds.Start, Bounds.End);
	}

}

// Set the debug, and increment for children
void _Element::SetDebug(int Debug) {
	this->Debug = Debug;

	for(size_t i = 0; i < Children.size(); i++)
		Children[i]->SetDebug(Debug + 1);
}