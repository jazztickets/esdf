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
#include <al.h>
#include <alc.h>
#include <glm/vec2.hpp>
#include <map>
#include <list>
#include <string>

// Struct for OpenAL buffers
struct _AudioBuffer {
	ALuint ID;
	ALenum Format;
	float Volume;
	int Limit;
};

struct _SourcePlaying {
	_SourcePlaying() { Count = 0; }
	int Count;
};

// Class for OpenAL sources
class _AudioSource {

	public:

		_AudioSource(const _AudioBuffer *Buffer, bool Relative=false, bool Loop=false, float MinGain=0.0f, float MaxGain=1.0f, float ReferenceDistance=10.0f, float RollOff=2.5f);
		~_AudioSource();

		void Play();
		void Stop();
		void SetPitch(float Value);
		void SetGain(float Value);
		void SetPosition(const glm::vec2 &Position);
		glm::vec2 GetPosition();
		bool IsPlaying();
		bool IsRelative();
		ALuint GetID() { return ID; }
		const _AudioBuffer *GetAudioBuffer() const { return AudioBuffer; }

	private:

		bool Loaded;
		const _AudioBuffer *AudioBuffer;
		ALuint ID;
};

// Classes
class _Audio {

	public:

		_Audio() { Enabled = false; }

		void Init(bool Enabled);
		void Close();

		bool IsEnabled() { return Enabled; }

		// Buffers
		bool LoadBuffer(const std::string &Name, const std::string &File, float Volume=1.0f, int Limit=0);
		const _AudioBuffer *GetBuffer(const std::string &Name);
		void FreeAllBuffers();

		// 3D Audio
		void SetPosition(const glm::vec2 &Position);
		void SetDirection(const glm::vec2 &Direction);
		void SetGain(float Value);
		glm::vec2 GetListenerPosition();

		// Sources
		void Play(_AudioSource *AudioSource, const glm::vec2 &Position);
		void Update(double FrameTime);

	private:

		// State
		bool Enabled;

		// Buffers
		std::map<std::string, _AudioBuffer> Buffers;
		std::map<ALuint, _SourcePlaying> SourcesPlaying;

		// Sources
		std::list<_AudioSource *> Sources;
};

extern _Audio Audio;
