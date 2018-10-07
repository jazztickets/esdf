/******************************************************************************
* esdf
* Copyright (C) 2017  Alan Witkowski
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
#include <objects/animation.h>
#include <objects/object.h>
#include <ae/texture.h>
#include <ae/graphics.h>
#include <ae/assets.h>
#include <iostream>

// Constructor
_Animation::_Animation(_Object *Parent) :
	_Component(Parent),
	Timer(0.0),
	FramePeriod(1.0),
	Reel(0),
	Mode(STOPPED),
	Frame(0),
	LastFrame(-1),
	Direction(1) {
}

// Destructor
_Animation::~_Animation() {
}

// Update
void _Animation::Update(double FrameTime) {
	if(Templates.size() == 0)
		return;

	if(Mode == PLAYING && Timer >= FramePeriod) {
		//std::cout << Parent->ID << " " << Frame << std::endl;
		Timer = 0;
		Frame += Direction;
		if(Frame > Templates[Reel]->EndFrame) {
			if(Templates[Reel]->RepeatType == BOUNCE) {
				Frame = Templates[Reel]->EndFrame - 1;
				Direction = -Direction;
			}
		}
		else if(Frame < Templates[Reel]->StartFrame) {
			if(Templates[Reel]->RepeatType == BOUNCE) {
				Frame = Templates[Reel]->StartFrame + 1;
				Direction = -Direction;
			}
		}
	}

	if(Frame != LastFrame) {
		//std::cout << Frame << std::endl;
		CalculateTextureCoords();
		LastFrame = Frame;
	}

	Timer += FrameTime;
}

// Play an animation
void _Animation::Play(int Reel) {
	if(Mode != PLAYING) {
		//std::cout << "NEW PLAY" << std::endl;
		Mode = PLAYING;
		this->Reel = Reel;
		Frame = Templates[Reel]->DefaultFrame;
		Timer = 0;
		Direction = 1;
	}
}

// Stop
void _Animation::Stop() {
	Mode = STOPPED;
	Frame = Templates[Reel]->DefaultFrame;
}

// Calculate where in the texture to draw the current frame
void _Animation::CalculateTextureCoords() {
	if(!Templates[Reel]->Texture || !Templates[Reel]->Texture->ID)
		return;

	int FrameX = Frame % (Templates[Reel]->FramesPerLine);
	int FrameY = Frame / (Templates[Reel]->FramesPerLine);

	TextureCoords[0] = Templates[Reel]->TextureScale.x * FrameX;
	TextureCoords[1] = Templates[Reel]->TextureScale.y * FrameY;
	TextureCoords[2] = TextureCoords[0] + Templates[Reel]->TextureScale.x;
	TextureCoords[3] = TextureCoords[1] + Templates[Reel]->TextureScale.y;
}
