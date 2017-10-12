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
#pragma once

// Libraries
#include <ae/ui/ui.h>
#include <string>
#include <vector>

// Forward Declarations
struct _Style;
struct _KeyEvent;

// Classes
class _Element {

	public:

		_Element();
		virtual ~_Element();

		virtual void Update(double FrameTime, const glm::ivec2 &Mouse);

		virtual void CalculateBounds();
		virtual void Render() const;
		virtual void HandleKeyEvent(const _KeyEvent &KeyEvent);
		virtual void HandleTextEvent(const char *Text);
		virtual void HandleInput(bool Pressed);
		_Element *GetClickedElement();

		void UpdateChildrenOffset(const glm::ivec2 &Update) { ChildrenOffset += Update; CalculateChildrenBounds(); }
		void CalculateChildrenBounds();

		void SetDebug(int Debug);
		void SetOffset(const glm::ivec2 &Offset) { this->Offset = Offset; CalculateBounds(); }
		void SetWidth(int Width) { Size.x = Width; CalculateBounds(); }
		void SetHeight(int Height) { Size.y = Height; CalculateBounds(); }

		// Attributes
		size_t GlobalID;
		std::string Identifier;
		std::string ParentIdentifier;
		_Element *Parent;
		void *UserData;

		int MaskOutside : 1;
		int Debug : 4;

		// Graphics
		const _Style *Style;
		float Fade;

		// Layout
		_Bounds Bounds;
		_Alignment Alignment;
		glm::ivec2 Size;
		glm::ivec2 Offset;

		// Input
		_Element *HitElement;
		_Element *PressedElement;
		_Element *ReleasedElement;

		// Children
		std::vector<_Element *> Children;
		glm::ivec2 ChildrenOffset;

	protected:

};
