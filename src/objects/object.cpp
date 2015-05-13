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
#include <objects/object.h>
#include <objects/physics.h>
#include <objects/controller.h>
#include <objects/animation.h>
#include <objects/render.h>
#include <objects/item.h>
#include <constants.h>
#include <buffer.h>

// Constructor
_Object::_Object() :
	Physics(nullptr),
	Controller(nullptr),
	Animation(nullptr),
	Render(nullptr),
	Item(nullptr),
	Peer(nullptr),
	Map(nullptr),
	Log(nullptr),
	Deleted(false),
	SendUpdate(false),
	TileChanged(false),
	ID(0),
	Identifier(""),
	Name(""),
	GridType(0) {

}

// Destructor
_Object::~_Object() {
	delete Physics;
	delete Controller;
	delete Animation;
	delete Render;
	delete Item;
}

// Update
void _Object::Update(double FrameTime, uint16_t TimeSteps) {
	if(Physics && !Physics->ClientSidePrediction)
		Physics->Update(FrameTime, TimeSteps);

	if(Animation)
		Animation->Update(FrameTime);
}

void _Object::Serialize(_Buffer &Buffer) {

}

// Serialize
void _Object::NetworkSerialize(_Buffer &Buffer) {
	Buffer.WriteString(Identifier.c_str());
	Buffer.Write<uint16_t>(ID);
	Buffer.Write<char>(GridType);

	if(Controller)
		Controller->NetworkSerialize(Buffer);

	if(Physics)
		Physics->NetworkSerialize(Buffer);
}

// Unserialize
void _Object::NetworkUnserialize(_Buffer &Buffer) {
	GridType = Buffer.Read<char>();

	if(Controller)
		Controller->NetworkUnserialize(Buffer);

	if(Physics)
		Physics->NetworkUnserialize(Buffer);
}

// Serialize update
void _Object::NetworkSerializeUpdate(_Buffer &Buffer, uint16_t TimeSteps) {
	Buffer.Write<uint16_t>(ID);

	if(Controller)
		Controller->NetworkSerializeUpdate(Buffer, TimeSteps);

	if(Physics)
		Physics->NetworkSerializeUpdate(Buffer, TimeSteps);
}

// Unserialize update
void _Object::NetworkUnserializeUpdate(_Buffer &Buffer, uint16_t TimeSteps) {

	if(Controller)
		Controller->NetworkUnserializeUpdate(Buffer, TimeSteps);

	if(Physics)
		Physics->NetworkUnserializeUpdate(Buffer, TimeSteps);
}
