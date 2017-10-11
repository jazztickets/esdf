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
#include <hud.h>
#include <ui/label.h>
#include <assets.h>
#include <graphics.h>
#include <sstream>

// Initialize
_HUD::_HUD() {
}

// Shut down
_HUD::~_HUD() {
}

// Handle mouse
void _HUD::MouseEvent(const _MouseEvent &MouseEvent) {
}

// Update
void _HUD::Update(double FrameTime) {
}

// Draw
void _HUD::Render() {

	// FPS
	std::ostringstream Buffer;
	Buffer << Graphics.FramesPerSecond << " FPS";
	Assets.Labels["label_hud_fps"]->Text = Buffer.str();
	Assets.Labels["label_hud_fps"]->Render();
	Buffer.str("");
}
