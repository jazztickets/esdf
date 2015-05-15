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
#include <string>
#include <ui/ui.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

// Forward Declarations
class _Texture;
class _Element;
class _Label;
class _Image;
class _Font;
class _Object;
class _Item;
struct _MouseEvent;

class _HUD {

	public:

		_HUD();
		~_HUD();

		void MouseEvent(const _MouseEvent &MouseEvent);

		void Update(double FrameTime, float Radius);

		void Render();
		void RenderCharacterScreen();
		void RenderItemInfo(_Item *Item, int DrawX, int DrawY);
		void UpdateSkillInfo(int Skill, int DrawX, int DrawY);
		void RenderCrosshair(const glm::vec2 &Position);
		void RenderDeathScreen();

		void SetLastEntityHit(_Object *Entity);

		bool IsDragging() const { return CursorItem != nullptr; }

		void SetCursorOverItem(_Item *CursorOverItem) { this->CursorOverItem = CursorOverItem; }
		_Item *GetCursorOverItem() { return CursorOverItem; }

		void SetInventoryOpen(bool Value);
		bool GetInventoryOpen() { return InventoryOpen; }

		void ShowTextMessage(const std::string &Message, double Time);
		void ShowMessageBox(const std::string &Message, double Time);
		double GetMessageBoxTimer() { return MessageBoxTimer; }

		void SetPlayer(_Object *Player) { this->Player = Player; }

	private:

		void DrawIndicator(const std::string &String, float Percent=0.0f, _Texture *Texture=nullptr);
		void DrawHUDWeapon(const _Item *Weapon, _Element *Element, _Image *Image, _Label *Label);
		void DrawItemCount(_Item *Item, int X, int Y);

		// State
		_Object *Player;
		bool InventoryOpen;

		// UI
		_Element *DragStart;
		_Item *CursorItem;
		_Item *CursorOverItem;
		glm::ivec2 ClickOffset;
		int CursorSkill;

		// Displays
		_Object *LastEntityHit;
		double LastEntityHitTimer;
		float CrosshairScale;

		// Messages
		double MessageTimer;
		double MessageBoxTimer;

};
