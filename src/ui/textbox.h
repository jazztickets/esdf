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
#include <glm/vec4.hpp>

// Forward Declarations
class _Font;

// Classes
class _TextBox : public _Element {

	public:

		_TextBox(const std::string &Identifier, _Element *Parent, const glm::ivec2 &Offset, const glm::ivec2 &Size, const _Alignment &Alignment, const _Style *Style, const _Font *Font, size_t MaxLength);
		~_TextBox() override;

		void Update(double FrameTime, const glm::ivec2 &Mouse) override;
		void HandleKeyEvent(const _KeyEvent &KeyEvent) override;
		void HandleTextEvent(const char *Text) override;
		void HandleInput(bool Pressed) override;
		void Render() const override;

		void ResetCursor() { DrawCursor = true; CursorTimer = 0; }

		std::string Text;
		bool Focused;

	private:

		const _Font *Font;
		size_t MaxLength;

		bool DrawCursor;
		double CursorTimer;

};
