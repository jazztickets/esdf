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
#pragma once

// Libraries
#include <objects/component.h>
#include <glm/vec2.hpp>
#include <string>
#include <vector>

// Forward Declarations
class _Texture;

// Animation template struct
struct _AnimationTemplate {
	std::string Identifier;
	const _Texture *Texture;
	int FramesPerLine;
	glm::vec2 TextureScale;
	glm::ivec2 FrameSize;
	int StartFrame;
	int EndFrame;
	int DefaultFrame;
	int RepeatType;
};

// Classes
class _Animation : public _Component {

	public:

		// States
		enum PlayType {
			STOPPED,
			PLAYING,
			PAUSED
		};

		enum RepeatType {
			STOP,
			WRAP,
			BOUNCE
		};

		_Animation(_Object *Parent);
		~_Animation();

		void Update(double FrameTime, uint16_t TimeSteps) override;
		void Play(int Reel);
		void Stop();
		void CalculateTextureCoords();

		std::vector<const _AnimationTemplate *> Templates;
		float TextureCoords[8];
		double Timer;
		double FramePeriod;
		int Mode;
		int Reel;
		int Frame;
		int LastFrame;
		int Direction;

};
