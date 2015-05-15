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
#include <ui/ui.h>
#include <string>
#include <vector>

// Forward Declarations
struct _Style;
struct _KeyEvent;

// Classes
class _Element {

	public:

		_Element();
		_Element(const std::string &Identifier, _Element *Parent, const glm::ivec2 &Offset, const glm::ivec2 &Size, const _Alignment &Alignment, const _Style *Style, bool MaskOutside);
		virtual ~_Element();

		virtual void Update(double FrameTime, const glm::ivec2 &Mouse);

		virtual void CalculateBounds();
		virtual void Render() const;
		virtual void HandleKeyEvent(const _KeyEvent &KeyEvent);
		virtual void HandleTextEvent(const char *Text);
		virtual void HandleInput(bool Pressed);
		_Element *GetClickedElement();

		_Element *AddChild(_Element *Element) { Children.push_back(Element); Element->ID = Children.size()-1; return Element; }
		std::vector<_Element *> &GetChildren() { return Children; }
		void UpdateChildrenOffset(const glm::ivec2 &Update) { ChildrenOffset += Update; CalculateChildrenBounds(); }
		virtual void CalculateChildrenBounds();

		void SetDebug(int Debug);

		void SetAlignment(const _Alignment &Alignment) { this->Alignment = Alignment; CalculateBounds(); }
		const _Alignment &GetAlignment() const { return Alignment; }

		void SetOffset(const glm::ivec2 &Offset) { this->Offset = Offset; CalculateBounds(); }
		const glm::ivec2 &GetOffset() const { return Offset; }

		void SetChildrenOffset(const glm::ivec2 &ChildrenOffset) { this->ChildrenOffset = ChildrenOffset; CalculateChildrenBounds(); }
		const glm::ivec2 &GetChildrenOffset() const { return ChildrenOffset; }

		void SetWidth(int Width) { Size.x = Width; CalculateBounds(); }
		void SetHeight(int Height) { Size.y = Height; CalculateBounds(); }

		// Attributes
		std::string ParentIdentifier;
		_Element *Parent;
		int ID;
		void *UserData;
		std::string Identifier;
		const _Style *Style;
		float Fade;

		// Size
		glm::ivec2 Size;
		_Bounds Bounds;

		// Input
		_Element *HitElement;
		_Element *PressedElement;
		_Element *ReleasedElement;

		glm::ivec2 ChildrenOffset;
		std::vector<_Element *> Children;
		glm::ivec2 Offset;
		_Alignment Alignment;
		bool MaskOutside;
		int Debug;

	protected:

};
