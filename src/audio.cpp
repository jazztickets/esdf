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
#include <audio.h>
#include <vorbis/vorbisfile.h>
#include <vector>
#include <stdexcept>
#include <constants.h>
#include <glm/gtx/norm.hpp>

// Globals
_Audio Audio;

// Initializes the audio system
void _Audio::Init(bool Enabled) {

	// Set audio enabled
	this->Enabled = Enabled;
	if(!Enabled)
		return;

	// Create device
	ALCdevice *Device = alcOpenDevice(nullptr);
	if(Device == nullptr) {
		Enabled = false;
		throw std::runtime_error("Unable to create audio device");
	}

	// Create context
	ALCcontext *Context = alcCreateContext(Device, nullptr);

	// Set active context
	alcMakeContextCurrent(Context);

	// Clear code
	alGetError();

	// Set orientation
	SetDirection(glm::vec2(0, -1));
}

// Closes the audio system
void _Audio::Close() {
	if(!Enabled)
		return;

	// Free loaded sounds
	FreeAllBuffers();

	// Get active context
	ALCcontext *Context = alcGetCurrentContext();

	// Get device for active context
	ALCdevice *Device = alcGetContextsDevice(Context);

	// Disable context
	alcMakeContextCurrent(nullptr);

	// Free context
	alcDestroyContext(Context);

	// Close device
	alcCloseDevice(Device);

	Enabled = false;
}

// Loads an ogg file into memory
bool _Audio::LoadBuffer(const std::string &Name, const std::string &File, float Volume, int Limit) {
	if(!Enabled)
		return true;

	// Get path
	std::string Path = File;

	// Find existing buffer in map
	if(Buffers.find(Name) != Buffers.end())
		return true;

	// Open vorbis stream
	OggVorbis_File VorbisStream;
	int ReturnCode = ov_fopen(Path.c_str(), &VorbisStream);
	if(ReturnCode != 0) {
		printf("AudioClass::LoadBuffer - Unable to load %s\n", Path.c_str());
		return false;
	}

	// Get vorbis file info
	vorbis_info *Info = ov_info(&VorbisStream, -1);

	// Create new buffer
	_AudioBuffer AudioBuffer;
	switch(Info->channels) {
		case 1:
			AudioBuffer.Format = AL_FORMAT_MONO16;
		break;
		case 2:
			AudioBuffer.Format = AL_FORMAT_STEREO16;
		break;
		default:
			printf("AudioClass::LoadBuffer - Unsupported # of channels for %s\n", Path.c_str());
			return false;
		break;
	}

	// Set volume
	AudioBuffer.Volume = Volume;
	AudioBuffer.Limit = Limit;

	// Alloc some memory for the samples
	std::vector<char> Data;

	// Decode vorbis file
	long BytesRead;
	char Buffer[4096];
	int BitStream;
	do {
		BytesRead = ov_read(&VorbisStream, Buffer, 4096, 0, 2, 1, &BitStream);
		Data.insert(Data.end(), Buffer, Buffer + BytesRead);
	} while(BytesRead > 0);

	// Create buffer
	alGenBuffers(1, &AudioBuffer.ID);
	alBufferData(AudioBuffer.ID, AudioBuffer.Format, &Data[0], (ALsizei)Data.size(), Info->rate);

	// Close vorbis file
	ov_clear(&VorbisStream);

	// Add to map
	Buffers[Name] = AudioBuffer;

	return true;
}

// Get a loaded buffer
const _AudioBuffer *_Audio::GetBuffer(const std::string &Name) {
	if(!Enabled)
		return nullptr;

	// Find buffer in map
	const auto &BuffersIterator = Buffers.find(Name);
	if(BuffersIterator == Buffers.end())
		return nullptr;

	return &BuffersIterator->second;
}

// Free all loaded buffers
void _Audio::FreeAllBuffers() {
	if(!Enabled)
		return;

	// Iterate over map
	for(auto BuffersIterator = Buffers.begin(); BuffersIterator != Buffers.end(); ++BuffersIterator) {
		_AudioBuffer &Buffer = BuffersIterator->second;

		alDeleteBuffers(1, &Buffer.ID);
	}

	Buffers.clear();
}

// Play an audio source and add it to the audio manager
void _Audio::Play(_AudioSource *AudioSource, const glm::vec2 &Position) {
	if(!Enabled) {

		// TODO cleanup
		delete AudioSource;
		return;
	}

	if(!AudioSource->GetAudioBuffer())
		return;

	float DistanceSquared = glm::distance2(Position, GetListenerPosition());
	if(AudioSource->IsRelative() || DistanceSquared <= MAX_AUDIO_DISTANCE_SQUARED) {
		SourcesPlaying[AudioSource->GetAudioBuffer()->ID].Count++;

		if(AudioSource->GetAudioBuffer()->Limit > 0 && SourcesPlaying[AudioSource->GetAudioBuffer()->ID].Count > AudioSource->GetAudioBuffer()->Limit) {
			for(auto Iterator = Sources.begin(); Iterator != Sources.end(); ++Iterator) {
				_AudioSource *Source = *Iterator;
				if(Source->IsPlaying() && Source->GetAudioBuffer()->ID == AudioSource->GetAudioBuffer()->ID) {
					alSourceStop(Source->GetID());
					break;
				}
			}
		}

		Sources.push_back(AudioSource);
		AudioSource->SetPosition(Position);
		AudioSource->Play();
	}
	else {
		delete AudioSource;
	}
}

// Update all audio sources
void _Audio::Update(double FrameTime) {
	if(!Enabled)
		return;

	// Update sources
	for(auto Iterator = Sources.begin(); Iterator != Sources.end(); ) {
		_AudioSource *Source = *Iterator;
		bool NeedsDelete = false;

		// Check conditions for deletion
		if(!Source->IsPlaying()) {
			NeedsDelete = true;
		}
		else if(!Source->IsRelative()) {

			float DistanceSquared = glm::distance2(Source->GetPosition(), GetListenerPosition());
			if(DistanceSquared > MAX_AUDIO_DISTANCE_SQUARED)
				NeedsDelete = true;
		}

		// Delete source
		if(NeedsDelete) {
			SourcesPlaying[Source->GetAudioBuffer()->ID].Count--;
			if(SourcesPlaying[Source->GetAudioBuffer()->ID].Count <= 0)
				SourcesPlaying.erase(Source->GetAudioBuffer()->ID);
			delete Source;
			Iterator = Sources.erase(Iterator);
		}
		else {
			++Iterator;
		}
	}

	//for(SourcesPlayingIterator = SourcesPlaying.begin(); SourcesPlayingIterator != SourcesPlaying.end(); ++SourcesPlayingIterator)
	//	std::cout << SourcesPlayingIterator->first << ": " << SourcesPlayingIterator->second.Count << std::endl;
}

// Set position of listener
void _Audio::SetPosition(const glm::vec2 &Position) {
	if(!Enabled)
		return;

	alListener3f(AL_POSITION, Position.x, 10, Position.y);
}

// Get listener position
glm::vec2 _Audio::GetListenerPosition() {
	float Position[3];
	alGetListener3f(AL_POSITION, &Position[0], &Position[1], &Position[2]);

	return glm::vec2(Position[0], Position[2]);
}

// Sets the listener direction
void _Audio::SetDirection(const glm::vec2 &Direction) {
	if(!Enabled)
		return;

	float Orientation[6] = { Direction.x, 0, Direction.y, 0.0f, 1.0f, 0.0f };
	alListenerfv(AL_ORIENTATION, Orientation);
}

// Set gain
void _Audio::SetGain(float Value) {
	if(!Enabled)
		return;

	alListenerf(AL_GAIN, Value);
}

// Create an audio source
_AudioSource::_AudioSource(const _AudioBuffer *Buffer, bool Relative, bool Loop, float MinGain, float MaxGain, float ReferenceDistance, float RollOff) {
	Loaded = false;
	AudioBuffer = nullptr;
	if(Buffer) {

		AudioBuffer = Buffer;

		// Create source
		alGenSources(1, &ID);

		// Assign buffer to source
		alSourcei(ID, AL_BUFFER, Buffer->ID);
		alSourcef(ID, AL_GAIN, Buffer->Volume);
		alSourcef(ID, AL_MIN_GAIN, MinGain);
		alSourcef(ID, AL_MAX_GAIN, 1.0f);
		alSourcef(ID, AL_REFERENCE_DISTANCE, ReferenceDistance);
		alSourcef(ID, AL_MAX_DISTANCE, 100.0f);
		alSourcef(ID, AL_ROLLOFF_FACTOR, RollOff);
		alSourcei(ID, AL_LOOPING, Loop);
		alSourcei(ID, AL_SOURCE_RELATIVE, Relative);

		Loaded = true;
	}
}

// Free audio source
_AudioSource::~_AudioSource() {
	if(Loaded) {

		// Create source
		alDeleteSources(1, &ID);
		Loaded = false;
	}
}

// Play
void _AudioSource::Play() {
	if(Loaded) {

		// Get state
		ALint State;
		alGetSourcei(ID, AL_SOURCE_STATE, &State);

		// If already playing, stop
		if(State == AL_PLAYING)
			alSourceStop(ID);

		// Play sound
		alSourcePlay(ID);
	}
}

// Stop
void _AudioSource::Stop() {
	if(Loaded) {
		alSourceStop(ID);
	}
}

// Returns true if the source is playing
bool _AudioSource::IsPlaying() {
	ALenum State;

	alGetSourcei(ID, AL_SOURCE_STATE, &State);

	return State == AL_PLAYING;
}

// Returns true if the source is relative
bool _AudioSource::IsRelative() {
	ALenum State;

	alGetSourcei(ID, AL_SOURCE_RELATIVE, &State);

	return State == AL_TRUE;
}

// Set pitch
void _AudioSource::SetPitch(float Value) {
	if(Loaded) {
		alSourcef(ID, AL_PITCH, Value);
	}
}

// Set gain
void _AudioSource::SetGain(float Value) {
	if(Loaded) {
		alSourcef(ID, AL_GAIN, Value);
	}
}

// Set position
void _AudioSource::SetPosition(const glm::vec2 &Position) {
	if(Loaded) {
		alSource3f(ID, AL_POSITION, Position.x, 0, Position.y);
	}
}

// Get source position
glm::vec2 _AudioSource::GetPosition() {
	if(!Loaded)
		return glm::vec2(0);

	float Position[3];
	alGetSource3f(ID, AL_POSITION, &Position[0], &Position[1], &Position[2]);

	return glm::vec2(Position[0], Position[2]);
}
