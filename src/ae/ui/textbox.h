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
#include <ae/ui/element.h>
#include <glm/vec4.hpp>

// Forward Declarations
class _Font;

// Classes
class _TextBox : public _Element {

	public:

		_TextBox();
		~_TextBox() override;

		void Update(double FrameTime, const glm::ivec2 &Mouse) override;
		void HandleKeyEvent(const _KeyEvent &KeyEvent) override;
		void HandleTextEvent(const char *Text) override;
		void HandleInput(bool Pressed) override;
		void Render() const override;

		void ResetCursor() { DrawCursor = true; CursorTimer = 0; }

		std::string Text;
		bool Focused;

		const _Font *Font;
		size_t MaxLength;

		bool DrawCursor;
		double CursorTimer;

	private:

};
