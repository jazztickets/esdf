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
#include <framework.h>
#include <states/null.h>
#include <states/convert.h>
#include <states/client.h>
#include <states/editor.h>
#include <states/benchmark.h>
#include <states/dedicated.h>
#include <network/network.h>
#include <config.h>
#include <graphics.h>
#include <input.h>
#include <actions.h>
#include <audio.h>
#include <state.h>
#include <framelimit.h>
#include <stdexcept>
#include <constants.h>
#include <assets.h>
#include <stats.h>
#include <save.h>
#include <SDL.h>

// Global instance
_Framework Framework;

// Initialize
void _Framework::Init(int ArgumentCount, char **Arguments) {
	RequestedState = nullptr;
	FrameLimit = nullptr;
	Done = false;
	TimeStepAccumulator = 0.0;
	TimeStep = GAME_TIMESTEP;
	FrameworkState = INIT;
	State = &EditorState;

	//bool AudioEnabled = Config.AudioEnabled;
	bool Fullscreen = Config.Fullscreen;
	glm::ivec2 WindowSize = Config.WindowSize;
	glm::ivec2 WindowPosition(SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	int MSAA = Config.MSAA;
	int Vsync = Config.Vsync;
	uint16_t NetworkPort = Config.NetworkPort;

	// Process arguments
	std::string Token;
	int TokensRemaining;
	for(int i = 1; i < ArgumentCount; i++) {
		Token = std::string(Arguments[i]);
		TokensRemaining = ArgumentCount - i - 1;

		if(Token == "-fullscreen") {
			Fullscreen = true;
		}
		else if(Token == "-window") {
			Fullscreen = false;
		}
		else if(Token == "-w" && TokensRemaining > 0) {
			WindowSize.x = atoi(Arguments[++i]);
		}
		else if(Token == "-h" && TokensRemaining > 0) {
			WindowSize.y = atoi(Arguments[++i]);
		}
		else if(Token == "-wx" && TokensRemaining > 0) {
			WindowPosition.x = atoi(Arguments[++i]);
		}
		else if(Token == "-wy" && TokensRemaining > 0) {
			WindowPosition.y = atoi(Arguments[++i]);
		}
		else if(Token == "-vsync" && TokensRemaining > 0) {
			Vsync = atoi(Arguments[++i]);
		}
		else if(Token == "-msaa" && TokensRemaining > 0) {
			MSAA = atoi(Arguments[++i]);
		}
		else if(Token == "-noaudio") {
			//AudioEnabled = false;
		}
		else if(Token == "-editor") {
			State = &EditorState;
			if(TokensRemaining && Arguments[i+1][0] != '-')
				EditorState.SetMapFilename(Arguments[++i]);
		}
		else if(Token == "-level" && TokensRemaining > 0) {
			ClientState.SetLevel(Arguments[++i]);
			ClientState.SetTestMode(true);

			State = &ClientState;
		}
		else if(Token == "-connect" && TokensRemaining > 0) {
			State = &ClientState;
			ClientState.SetHostAddress(Arguments[++i]);
			ClientState.SetRunServer(false);
		}
		else if(Token == "-port" && TokensRemaining > 0) {
			NetworkPort = atoi(Arguments[++i]);
		}
		else if(Token == "-benchmark") {
			State = &BenchmarkState;
		}
		else if(Token == "-dedicated") {
			State = &DedicatedState;
		}
	}

	// Initialize network subsystem
	_Network::InitializeSystem();

	// Run dedicated server
	if(State == &DedicatedState) {
		Assets.Init(true);
		Stats = new _Stats();
		FrameLimit = new _FrameLimit(120.0, false);

		DedicatedState.SetNetworkPort(NetworkPort);
		DedicatedState.SetStats(Stats);
	}
	else if(State == &ConvertState) {
	}
	else {

		// Open log file
		Log.Open((Config.ConfigPath + "client.log").c_str());

		// Initialize SDL
		if(SDL_Init(SDL_INIT_VIDEO) < 0)
			throw std::runtime_error("Failed to initialize SDL");

		// Get fullscreen size
		Config.SetDefaultFullscreenSize();

		// Set up subsystems
		Graphics.Init(WindowSize, WindowPosition, Vsync, MSAA, Config.Anisotropy, Fullscreen, &Log);

		// Load assets
		Assets.Init(false);
		Stats = new _Stats();
		Graphics.SetStaticUniforms();

		ClientState.SetConnectPort(NetworkPort);
		ClientState.SetLog(&Log);
		ClientState.SetStats(Stats);
		EditorState.SetStats(Stats);

		FrameLimit = new _FrameLimit(Config.MaxFPS, Config.Vsync);
	}

	Timer = SDL_GetPerformanceCounter();
}

// Shutdown
void _Framework::Close() {

	// Close the current state
	if(State)
		State->Close();

	Assets.Close();
	delete Stats;
	delete FrameLimit;

	_Network::CloseSystem();

	Graphics.Close();

	if(SDL_WasInit(SDL_INIT_VIDEO))
		SDL_Quit();
}

// Update input
void _Framework::Update() {

	// Get frame time
	double FrameTime = (SDL_GetPerformanceCounter() - Timer) / (double)SDL_GetPerformanceFrequency();
	Timer = SDL_GetPerformanceCounter();

	// Get events from SDL
	SDL_PumpEvents();
	Input.Update(FrameTime);

	// Loop through events
	SDL_Event Event;
	while(SDL_PollEvent(&Event)) {
		switch(Event.type){
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				if(!Event.key.repeat) {

					// Toggle fullscreen
					if(Event.type == SDL_KEYDOWN && (Event.key.keysym.mod & KMOD_ALT) && Event.key.keysym.scancode == SDL_SCANCODE_RETURN)
						Graphics.ToggleFullScreen(Config.WindowSize, Config.FullscreenSize);
					else if(State && FrameworkState == UPDATE) {
						_KeyEvent KeyEvent(Event.key.keysym.scancode, Event.type == SDL_KEYDOWN);
						State->KeyEvent(KeyEvent);
						Actions.InputEvent(_Input::KEYBOARD, Event.key.keysym.scancode, Event.type == SDL_KEYDOWN);
					}
				}
				else {
					if(State && FrameworkState == UPDATE && Event.key.keysym.scancode == SDL_SCANCODE_BACKSPACE) {
						_KeyEvent KeyEvent(Event.key.keysym.scancode, Event.type == SDL_KEYDOWN);
						State->KeyEvent(KeyEvent);
					}
				}
			break;
			case SDL_TEXTINPUT:
				if(State)
					State->TextEvent(Event.text.text);
			break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				if(State && FrameworkState == UPDATE) {
					_MouseEvent MouseEvent(glm::ivec2(Event.motion.x, Event.motion.y), Event.button.button, Event.type == SDL_MOUSEBUTTONDOWN);
					State->MouseEvent(MouseEvent);
					Actions.InputEvent(_Input::MOUSE_BUTTON, Event.button.button, Event.type == SDL_MOUSEBUTTONDOWN);
				}
			break;
			case SDL_MOUSEWHEEL:
				if(State)
					State->MouseWheelEvent(Event.wheel.y);
			break;
			case SDL_WINDOWEVENT:
				if(Event.window.event)
					State->WindowEvent(Event.window.event);
			break;
			case SDL_QUIT:
				Done = true;
			break;
		}
	}

	switch(FrameworkState) {
		case INIT: {
			if(State) {
				State->Init();
				FrameworkState = UPDATE;
			}
			else
				Done = true;
		} break;
		case UPDATE: {
			TimeStepAccumulator += FrameTime;
			while(TimeStepAccumulator >= TimeStep) {
				State->Update(TimeStep);
				TimeStepAccumulator -= TimeStep;
			}
			State->Render(TimeStepAccumulator / TimeStep);
		} break;
		case CLOSE: {
			if(State)
				State->Close();

			State = RequestedState;
			FrameworkState = INIT;
		} break;
	}

	Graphics.Flip(FrameTime);

	if(FrameLimit)
		FrameLimit->Update();
}

// Change states
void _Framework::ChangeState(_State *RequestedState) {
	this->RequestedState = RequestedState;
	FrameworkState = CLOSE;
}
