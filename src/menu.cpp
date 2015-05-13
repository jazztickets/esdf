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
#include <menu.h>
#include <constants.h>
#include <input.h>
#include <actions.h>
#include <graphics.h>
#include <assets.h>
#include <ui/element.h>
#include <ui/label.h>
#include <ui/button.h>
#include <ui/image.h>
#include <ui/textbox.h>
#include <config.h>
#include <framework.h>
#include <states/client.h>
#include <states/null.h>
#include <sstream>
#include <SDL_mouse.h>

_Menu Menu;

const std::string InputBoxPrefix = "button_options_input_";
const std::string PlayerButtonPrefix = "button_singleplayer_slot";
const std::string PlayerColorButtonPrefix = "button_new_color";

const std::string KEYLABEL_IDENTIFIERS[] = {
	"label_options_config_up",
	"label_options_config_down",
	"label_options_config_left",
	"label_options_config_right",
	"label_options_config_fire",
	"label_options_config_aim",
	"label_options_config_use",
	"label_options_config_inventory",
	"label_options_config_reload",
	"label_options_config_weaponswitch",
	"label_options_config_medkit",
};

const char *COLORS[] = {
	"black",
	"red",
	"green",
	"blue",
};

// Constructor
_Menu::_Menu() {
	State = STATE_NONE;
	CurrentLayout = nullptr;
	Background = nullptr;
	OptionsState = OPTION_NONE;
	SinglePlayerState = SINGLEPLAYER_NONE;
	PreviousClickTimer = 0.0;
}

// Initialize
void _Menu::InitTitle() {
	Assets.GetLabel("game_version")->SetText(GAME_VERSION_STRING);
	Graphics.ShowCursor(true);

	//ackground = Assets.GetImage("menu_bg");
	CurrentLayout = Assets.GetElement("menu_title");

	State = STATE_TITLE;
}

// Init tutorial
void _Menu::InitTutorial() {
	//Framework.ChangeState(&ClientState);
	//State = STATE_NONE;
}

// Init single player
void _Menu::InitSinglePlayer() {
	CurrentLayout = Assets.GetElement("menu_singleplayer");

	RefreshSaveSlots();
	for(int i = 0; i < SAVE_COUNT; i++)
		SaveSlots[i]->SetEnabled(false);
	SelectedColor = 0;
	SelectedSlot = -1;

	SinglePlayerState = SINGLEPLAYER_NONE;
	State = STATE_SINGLEPLAYER;
}

// Options
void _Menu::InitOptions() {
	CurrentLayout = Assets.GetElement("menu_options");

	RefreshInputLabels();
	CurrentAction = -1;

	OptionsState = OPTION_NONE;
	State = STATE_OPTIONS;
}

// In-game menu
void _Menu::InitInGame() {
	CurrentLayout = Assets.GetElement("menu_ingame");

	Graphics.ShowCursor(true);
	Background = nullptr;

	State = STATE_INGAME;
}

// Return to play
void _Menu::InitPlay() {

	Graphics.ShowCursor(false);
	CurrentLayout = nullptr;

	State = STATE_NONE;
}

// Init new player popup
void _Menu::InitNewPlayer() {
	_TextBox *Name = Assets.GetTextBox("textbox_new_name");
	Name->SetFocused(true);
	Name->SetText("");

	// Deselect previous elements
	for(int i = 0; i < COLOR_COUNT; i++) {
		std::stringstream Buffer;
		Buffer << PlayerColorButtonPrefix << i;

		ColorButtons[i] = Assets.GetButton(Buffer.str());
		ColorButtons[i]->SetEnabled(false);
		ColorButtons[i]->SetID(i);
	}

	SelectedColor = 0;
	ColorButtons[SelectedColor]->SetEnabled(true);

	CurrentLayout = Assets.GetElement("menu_new");
	SinglePlayerState = SINGLEPLAYER_NEW_PLAYER;
}

// Play the game
void _Menu::LaunchGame() {
	//Save.GetPlayer(SelectedSlot)->Load();
	//PlayState.SetPlayer(Save.GetPlayer(SelectedSlot));
	//ClientState.SetLevel("");
	//ClientState.SetTestMode(false);
	//ClientState.SetFromEditor(false);
	//Framework.ChangeState(&ClientState);

	//SaveSlots[SelectedSlot]->SetEnabled(false);
	//State = STATE_NONE;
}

// Shutdown
void _Menu::Close() {
}

// Handle key event
void _Menu::KeyEvent(const _KeyEvent &KeyEvent) {
	if(CurrentLayout)
		CurrentLayout->HandleKeyEvent(KeyEvent);

	switch(State) {
		case STATE_TITLE: {
			if(KeyEvent.Pressed && KeyEvent.Key == SDL_SCANCODE_ESCAPE)
				Framework.SetDone(true);
		} break;
		case STATE_SINGLEPLAYER: {

			if(SinglePlayerState == SINGLEPLAYER_NONE) {
				if(KeyEvent.Pressed && KeyEvent.Key == SDL_SCANCODE_ESCAPE)
					InitTitle();
			}
			else {
				if(KeyEvent.Pressed) {
					if(KeyEvent.Key == SDL_SCANCODE_ESCAPE)
						CancelCreate();
					else if(KeyEvent.Key == SDL_SCANCODE_RETURN)
						CreatePlayer();
				}
			}
		} break;
		case STATE_OPTIONS: {
			if(OptionsState == OPTION_NONE) {
				if(KeyEvent.Pressed && KeyEvent.Key == SDL_SCANCODE_ESCAPE) {
					Config.Save();
					if(Framework.GetState() == &ClientState)
						InitInGame();
					else
						InitTitle();
				}
			}
			else {
				if(KeyEvent.Pressed) {
					RemapInput(_Input::KEYBOARD, KeyEvent.Key);
				}
			}
		} break;
		case STATE_INGAME: {
			if(KeyEvent.Pressed && KeyEvent.Key == SDL_SCANCODE_ESCAPE)
				InitPlay();
		} break;
		default:
		break;
	}
}

// Handle text
void _Menu::TextEvent(const char *Text) {
	if(CurrentLayout)
		CurrentLayout->HandleTextEvent(Text);
}

// Handle mouse event
void _Menu::MouseEvent(const _MouseEvent &MouseEvent) {
	if(!CurrentLayout)
		return;

	// Accepting new action input
	switch(State) {
		case STATE_OPTIONS: {
			if(OptionsState == OPTION_ACCEPT_INPUT) {
				if(MouseEvent.Pressed) {
					RemapInput(_Input::MOUSE_BUTTON, MouseEvent.Button);
					return;
				}
			}
		} break;
		default:
		break;
	}

	if(MouseEvent.Button == SDL_BUTTON_LEFT)
		CurrentLayout->HandleInput(MouseEvent.Pressed);

	// Get clicked element
	_Element *Clicked = CurrentLayout->GetClickedElement();
	if(Clicked) {
		bool DoubleClick = false;
		if(PreviousClick == Clicked && PreviousClickTimer < MENU_DOUBLECLICK_TIME) {
			PreviousClick = nullptr;
			DoubleClick = true;
		}
		else
			PreviousClick = Clicked;
		PreviousClickTimer = 0.0;

		switch(State) {
			case STATE_TITLE: {
				if(Clicked->GetIdentifier() == "button_title_tutorial") {
					InitTutorial();
				}
				else if(Clicked->GetIdentifier() == "button_title_single") {
					InitSinglePlayer();
				}
				else if(Clicked->GetIdentifier() == "button_title_options") {
					InitOptions();
				}
				else if(Clicked->GetIdentifier() == "button_title_exit") {
					Framework.SetDone(true);
				}
			} break;
			case STATE_SINGLEPLAYER: {
				if(SinglePlayerState == SINGLEPLAYER_NONE) {

					if(Clicked->GetIdentifier() == "button_singleplayer_delete") {
						if(SelectedSlot != -1) {
							//Save.DeletePlayer(SelectedSlot);
							RefreshSaveSlots();

							SaveSlots[SelectedSlot]->SetEnabled(false);
							SelectedSlot = -1;
						}
					}
					else if(Clicked->GetIdentifier() == "button_singleplayer_play") {
						//if(SelectedSlot != -1 && Save.GetPlayer(SelectedSlot)) {
						//	LaunchGame();
						//}
					}
					else if(Clicked->GetIdentifier() == "button_singleplayer_back") {
						InitTitle();
					}
					else if(Clicked->GetIdentifier().substr(0, PlayerButtonPrefix.size()) == PlayerButtonPrefix) {

						// Deselect previous slot
						if(SelectedSlot != -1)
							SaveSlots[SelectedSlot]->SetEnabled(false);

						// Set up create player screen
						//if(!Save.GetPlayer(Clicked->GetID())) {
							InitNewPlayer();
						//}

						SelectedSlot = Clicked->GetID();
						SaveSlots[SelectedSlot]->SetEnabled(true);

						if(DoubleClick) {
							LaunchGame();
						}
					}
				}
				else {
					if(Clicked->GetIdentifier().substr(0, PlayerColorButtonPrefix.size()) == PlayerColorButtonPrefix) {
						if(SelectedColor != -1)
							ColorButtons[SelectedColor]->SetEnabled(false);

						SelectedColor = Clicked->GetID();
						ColorButtons[SelectedColor]->SetEnabled(true);
					}
					else if(Clicked->GetIdentifier() == "button_new_create") {
						CreatePlayer();
					}
					else if(Clicked->GetIdentifier() == "button_new_cancel") {
						CancelCreate();
					}
				}
			} break;
			case STATE_OPTIONS: {
				if(OptionsState == OPTION_NONE) {
					if(Clicked->GetIdentifier() == "button_options_defaults") {
						Config.LoadDefaultInputBindings();
						RefreshInputLabels();
					}
					else if(Clicked->GetIdentifier() == "button_options_save") {
						Config.Save();
						if(Framework.GetState() == &ClientState)
							InitInGame();
						else
							InitTitle();
					}
					else if(Clicked->GetIdentifier() == "button_options_cancel") {
						Config.Load();
						if(Framework.GetState() == &ClientState)
							InitInGame();
						else
							InitTitle();
					}
					else if(Clicked->GetIdentifier().substr(0, InputBoxPrefix.size()) == InputBoxPrefix) {
						OptionsState = OPTION_ACCEPT_INPUT;
						CurrentAction = Clicked->GetID();
						Assets.GetLabel("menu_options_accept_text_action")->SetText(Actions.GetName(CurrentAction));
					}
				}
			} break;
			case STATE_INGAME: {
				if(Clicked->GetIdentifier() == "button_ingame_resume") {
					InitPlay();
				}
				else if(Clicked->GetIdentifier() == "button_ingame_options") {
					InitOptions();
				}
				else if(Clicked->GetIdentifier() == "button_ingame_menu") {
					Framework.ChangeState(&NullState);
				}
			} break;
			default:
			break;
		}
	}
}

// Update phase
void _Menu::Update(double FrameTime) {
	PreviousClickTimer += FrameTime;

	if(CurrentLayout && OptionsState == OPTION_NONE) {
		CurrentLayout->Update(FrameTime, Input.GetMouse());
	}

	switch(State) {
		case STATE_TITLE: {
		} break;
		case STATE_SINGLEPLAYER: {
			for(int i = 0; i < SAVE_COUNT; i++) {
				/*_Object *Player = Save.GetPlayer(i);
				if(Player) {
					Player->SetChangedPosition(true);
					Player->UpdateAnimationState(FrameTime);
				}*/
			}
		} break;
		case STATE_OPTIONS: {
		} break;
		default:
		break;
	}
}

// Draw phase
void _Menu::Render() {
	Graphics.Setup2D();

	if(Background)
		Background->Render();

	switch(State) {
		case STATE_TITLE: {
			if(CurrentLayout)
				CurrentLayout->Render();
			Assets.GetLabel("game_version")->Render();
		} break;
		case STATE_OPTIONS: {
			if(CurrentLayout)
				CurrentLayout->Render();

			if(OptionsState == OPTION_ACCEPT_INPUT) {
				Graphics.FadeScreen(MENU_ACCEPTINPUT_FADE);
				Assets.GetElement("menu_popup")->Render();
			}
		} break;
		case STATE_SINGLEPLAYER: {
			Assets.GetElement("menu_singleplayer")->Render();

			/*
			for(int i = 0; i <= _Save::SLOT_9; i++) {
				_Object *Player = Save.GetPlayer(i);
				if(Player)
					Player->Render2D(SaveSlots[i]->GetBounds().GetMidPoint());
			}
			*/
			if(SinglePlayerState == SINGLEPLAYER_NEW_PLAYER) {
				Graphics.FadeScreen(MENU_ACCEPTINPUT_FADE);
				if(CurrentLayout)
					CurrentLayout->Render();
			}

		} break;
		case STATE_INGAME: {
			if(CurrentLayout)
				CurrentLayout->Render();
		} break;
		default:
		break;
	}
}

// Refreshes the save slots after player creation
void _Menu::RefreshSaveSlots() {

	// Load save slots
	for(int i = 0; i < SAVE_COUNT; i++) {
		std::stringstream Buffer;
		Buffer << "menu_singleplayer_slot" << i << "_text";
		_Label *SlotLabel = Assets.GetLabel(Buffer.str());
		Buffer.str("");

		SlotLabel->SetText("Empty Slot");

		Buffer << PlayerButtonPrefix << i;
		SaveSlots[i] = Assets.GetButton(Buffer.str());
		SaveSlots[i]->SetID(i);
	}
}

// Refreshes the input map labels
void _Menu::RefreshInputLabels() {
	for(size_t i = 0; i < LABEL_COUNT; i++) {
		InputLabels[i] = Assets.GetLabel(KEYLABEL_IDENTIFIERS[i]);
		InputLabels[i]->SetText(Actions.GetInputNameForAction(i));
		InputLabels[i]->GetParent()->SetID(i);
	}
}

// Cancel create screen
void _Menu::CancelCreate() {
	CurrentLayout = Assets.GetElement("menu_singleplayer");
	SinglePlayerState = SINGLEPLAYER_NONE;

	SaveSlots[SelectedSlot]->SetEnabled(false);
}

// Handle player creation
void _Menu::CreatePlayer() {
	if(Assets.GetTextBox("textbox_new_name")->GetText().length() == 0)
		return;

	CurrentLayout = Assets.GetElement("menu_singleplayer");
	SinglePlayerState = SINGLEPLAYER_NONE;

	if(SelectedSlot != -1) {
		//Save.CreateNewPlayer(SelectedSlot, Assets.GetTextBox("textbox_new_name")->GetText(), COLORS[SelectedColor]);
		RefreshSaveSlots();
	}
}

// Remap a key/button
void _Menu::RemapInput(int InputType, int Input) {
	OptionsState = OPTION_NONE;
	if(InputType == _Input::KEYBOARD && Input == SDL_SCANCODE_ESCAPE)
		return;

	// Remove duplicate keys/buttons
	for(int i = 0; i < _Actions::COUNT; i++) {
		if(Actions.GetInputForAction(InputType, i) == Input) {
			Actions.ClearMappingsForAction(InputType, i);
		}
	}

	// Clear out existing action
	Actions.ClearMappingsForAction(_Input::KEYBOARD, CurrentAction);
	Actions.ClearMappingsForAction(_Input::MOUSE_BUTTON, CurrentAction);

	// Add new binding
	Actions.AddInputMap(InputType, Input, CurrentAction, false);

	// Update menu labels
	RefreshInputLabels();
}
