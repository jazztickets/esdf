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
#include <framework.h>
#include <states/null.h>
#include <states/convert.h>
#include <states/client.h>
#include <states/editor.h>
#include <states/benchmark.h>
#include <states/dedicated.h>
#include <ae/network.h>
#include <config.h>
#include <ae/graphics.h>
#include <ae/input.h>
#include <ae/actions.h>
#include <ae/audio.h>
#include <ae/state.h>
#include <ae/framelimit.h>
#include <stdexcept>
#include <constants.h>
#include <ae/assets.h>
#include <ae/util.h>
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

	// Get window settings
	ae::_WindowSettings WindowSettings;
	WindowSettings.WindowTitle = "esdf";
	WindowSettings.Fullscreen = Config.Fullscreen;
	WindowSettings.Vsync = Config.Vsync;
	WindowSettings.Size = Config.WindowSize;
	WindowSettings.Position = glm::ivec2(SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	//bool AudioEnabled = Config.AudioEnabled;
	//int MSAA = Config.MSAA;
	uint16_t NetworkPort = Config.NetworkPort;

	// Process arguments
	std::string Token;
	int TokensRemaining;
	for(int i = 1; i < ArgumentCount; i++) {
		Token = std::string(Arguments[i]);
		TokensRemaining = ArgumentCount - i - 1;

		if(Token == "-fullscreen") {
		}
		else if(Token == "-window") {
		}
		else if(Token == "-w" && TokensRemaining > 0) {
			WindowSettings.Size.x = atoi(Arguments[++i]);
		}
		else if(Token == "-h" && TokensRemaining > 0) {
			WindowSettings.Size.y = atoi(Arguments[++i]);
		}
		else if(Token == "-wx" && TokensRemaining > 0) {
			WindowSettings.Position.x = atoi(Arguments[++i]);
		}
		else if(Token == "-wy" && TokensRemaining > 0) {
			WindowSettings.Position.y = atoi(Arguments[++i]);
		}
		else if(Token == "-vsync" && TokensRemaining > 0) {
			WindowSettings.Vsync = atoi(Arguments[++i]);
		}
		else if(Token == "-msaa" && TokensRemaining > 0) {
			//MSAA = atoi(Arguments[++i]);
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
			NetworkPort = ae::ToNumber<uint16_t>(Arguments[++i]);
		}
		else if(Token == "-benchmark") {
			State = &BenchmarkState;
		}
		else if(Token == "-dedicated") {
			State = &DedicatedState;
		}
	}

	// Initialize network subsystem
	ae::_Network::InitializeSystem();

	// Run dedicated server
	if(State == &DedicatedState) {
		LoadAssets(true);
		FrameLimit = new ae::_FrameLimit(120.0, false);

		DedicatedState.SetNetworkPort(NetworkPort);
	}
	else if(State == &ConvertState) {
	}
	else {

		// Open log file
		Log.Open((Config.ConfigPath + "client.log").c_str());

		// Initialize SDL
		if(SDL_Init(SDL_INIT_VIDEO) < 0)
			throw std::runtime_error("Failed to initialize SDL");

		// Initialize audio
		//Audio.Init(AudioEnabled && Config.AudioEnabled);
		//Audio.SetSoundVolume(Config.SoundVolume);
		//Audio.SetMusicVolume(Config.MusicVolume);

		// Set up subsystems
		ae::Graphics.Init(WindowSettings);
		ae::Graphics.SetDepthTest(false);
		ae::Graphics.SetDepthMask(false);
		LoadAssets(false);
		ae::Graphics.SetStaticUniforms();

		ClientState.SetConnectPort(NetworkPort);
		ClientState.SetLog(&Log);

		FrameLimit = new ae::_FrameLimit(Config.MaxFPS, Config.Vsync);
	}

	Timer = SDL_GetPerformanceCounter();
}

// Shutdown
void _Framework::Close() {

	// Close the current state
	if(State)
		State->Close();

	ae::Assets.Close();
	delete FrameLimit;

	ae::_Network::CloseSystem();

	ae::Graphics.Close();

	if(SDL_WasInit(SDL_INIT_VIDEO))
		SDL_Quit();
}

// Requests a state change
void _Framework::ChangeState(ae::_State *RequestedState) {
	this->RequestedState = RequestedState;
	FrameworkState = CLOSE;
}

// Updates the current state and manages the state stack
void _Framework::Update() {

	// Get frame time
	double FrameTime = (SDL_GetPerformanceCounter() - Timer) / (double)SDL_GetPerformanceFrequency();
	Timer = SDL_GetPerformanceCounter();

	// Get events from SDL
	SDL_PumpEvents();
	ae::Input.Update(FrameTime);

	// Loop through events
	SDL_Event Event;
	while(SDL_PollEvent(&Event)) {
		if(State && FrameworkState == UPDATE) {
			switch(Event.type) {
				case SDL_KEYDOWN:
				case SDL_KEYUP: {
					if(!GlobalKeyHandler(Event)) {

						ae::_KeyEvent KeyEvent("", Event.key.keysym.scancode, Event.type == SDL_KEYDOWN, Event.key.repeat);
						State->HandleKey(KeyEvent);
						if(!Event.key.repeat) {
							ae::Actions.InputEvent(State, ae::_Input::KEYBOARD, Event.key.keysym.scancode, Event.type == SDL_KEYDOWN);
						}
					}
				} break;
				case SDL_TEXTINPUT: {
					ae::_KeyEvent KeyEvent(Event.text.text, 0, 1, 1);
					State->HandleKey(KeyEvent);
				} break;
				case SDL_MOUSEMOTION: {
					State->HandleMouseMove(glm::ivec2(Event.motion.xrel, Event.motion.yrel));
				} break;
				case SDL_MOUSEBUTTONDOWN:
				case SDL_MOUSEBUTTONUP: {
					ae::_MouseEvent MouseEvent(glm::ivec2(Event.motion.x, Event.motion.y), Event.button.button, Event.type == SDL_MOUSEBUTTONDOWN);
					State->HandleMouseButton(MouseEvent);
					ae::Actions.InputEvent(State, ae::_Input::MOUSE_BUTTON, Event.button.button, Event.type == SDL_MOUSEBUTTONDOWN);
				} break;
				case SDL_MOUSEWHEEL: {
					State->HandleMouseWheel(Event.wheel.y);
				} break;
				case SDL_WINDOWEVENT:
					if(Event.window.event)
						State->HandleWindow(Event.window.event);
				break;
				case SDL_QUIT:
					State->HandleQuit();
				break;
			}
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

	//Audio.Update(FrameTime * Config.TimeScale);
	ae::Graphics.Flip(FrameTime);

	if(FrameLimit)
		FrameLimit->Update();
}

// Handles global hotkeys
int _Framework::GlobalKeyHandler(const SDL_Event &Event) {

	// Handle alt-enter
	if(Event.type == SDL_KEYDOWN) {
		if((Event.key.keysym.mod & KMOD_ALT) && Event.key.keysym.scancode == SDL_SCANCODE_RETURN) {
			if(!Event.key.repeat)
				//Menu.SetFullscreen(!Config.Fullscreen);

			return 1;
		}
		else if((Event.key.keysym.mod & KMOD_CTRL) && Event.key.keysym.scancode == SDL_SCANCODE_S) {
			if(!Event.key.repeat) {
				if(Config.SoundVolume > 0.0f)
					Config.SoundVolume = 0.0f;
				else
					Config.SoundVolume = 1.0f;

				//Config.Save();
				//Audio.SetSoundVolume(Config.SoundVolume);
			}

			return 1;
		}
		else if((Event.key.keysym.mod & KMOD_CTRL) && Event.key.keysym.scancode == SDL_SCANCODE_M) {
			if(!Event.key.repeat) {
				if(Config.MusicVolume > 0.0f)
					Config.MusicVolume = 0.0f;
				else
					Config.MusicVolume = 1.0f;

				//Config.Save();
				//Audio.SetMusicVolume(Config.MusicVolume);
			}

			return 1;
		}
	}

	return 0;
}

// Load assets
void _Framework::LoadAssets(bool Server) {

	ae::Assets.LoadTextureDirectory("textures/editor/", Server);
	ae::Assets.LoadTextureDirectory("textures/tiles/", Server);
	ae::Assets.LoadTextureDirectory("textures/menu/", Server);
	ae::Assets.LoadTextureDirectory("textures/blocks/", Server, true, true);
	ae::Assets.LoadTextureDirectory("textures/props/", Server, true, true);
	ae::Assets.LoadAnimations("tables/animations.tsv", Server);
	ae::Assets.LoadLayers("tables/layers.tsv");

	if(!Server) {
		ae::Assets.LoadPrograms("tables/programs.tsv");
		ae::Assets.LoadFonts("tables/fonts.tsv", false);
		ae::Assets.LoadMeshDirectory("meshes/");
		ae::Assets.LoadColors("tables/colors.tsv");
		ae::Assets.LoadStyles("tables/styles.tsv");
		ae::Assets.LoadUI("tables/ui.xml");
		ae::Assets.LoadFonts("tables/fonts.tsv");
	}
}
