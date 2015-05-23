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
#include <stats.h>
#include <objects/object.h>
#include <objects/physics.h>
#include <objects/controller.h>
#include <objects/animation.h>
#include <objects/render.h>
#include <objects/shape.h>
#include <assets.h>
#include <utils.h>
#include <constants.h>
#include <fstream>
#include <stdexcept>
#include <iostream>

// Constructor
_Stats::_Stats() {
	LoadPhysics(STATS_PHYSICS);
	LoadControllers(STATS_CONTROLLERS);
	LoadAnimations(STATS_ANIMATIONS);
	LoadRenders(STATS_RENDERS);
	LoadShapes(STATS_SHAPES);
	LoadObjects(STATS_OBJECTS);
}

// Destructor
_Stats::~_Stats() {
}

// Object factory
_Object *_Stats::CreateObject(const std::string Identifier, bool IsServer) const {
	const auto &Iterator = Objects.find(Identifier);
	if(Iterator == Objects.end())
		return nullptr;

	const _ObjectStat &ObjectStat = Iterator->second;

	// Create object
	_Object *Object = new _Object();
	Object->Identifier = Identifier;
	Object->Name = ObjectStat.Name;

	// Create physics
	if(ObjectStat.PhysicsStat) {
		Object->Physics = new _Physics(Object);
	}

	// Create controller
	if(ObjectStat.ControllerStat) {
		Object->Controller = new _Controller(Object, *ObjectStat.ControllerStat);
	}

	// Create animation
	if(ObjectStat.AnimationStat) {
		Object->Animation = new _Animation(Object);

		// Load animation templates
		for(const auto &Template : ObjectStat.AnimationStat->Templates)
			Object->Animation->Templates.push_back(Assets.AnimationTemplates[Template]);

		// Set default frame
		Object->Animation->Stop();
		Object->Animation->FramePeriod = 0.07;
	}

	// Create render
	if(IsServer) {
		Object->Physics->Interpolate = false;
	}
	else if(ObjectStat.RenderStat) {
		Object->Render = new _Render(Object, *ObjectStat.RenderStat);
		Object->Render->Program = Assets.Programs[ObjectStat.RenderStat->ProgramIdentifier];
		Object->Render->Texture = Assets.Textures[ObjectStat.RenderStat->TextureIdentifier];
		Object->Render->Mesh = Assets.Meshes[ObjectStat.RenderStat->MeshIdentifier];
	}

	// Create shape
	if(ObjectStat.ShapeStat) {
		Object->Shape = new _Shape(Object, *ObjectStat.ShapeStat);
	}

	return Object;
}

// Load object stats
void _Stats::LoadObjects(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(1024, '\n');

	// Read data
	while(!File.eof() && File.peek() != EOF) {

		// Read row
		_ObjectStat ObjectStat;
		GetTSVToken(File, ObjectStat.Identifier);
		GetTSVToken(File, ObjectStat.Name);

		std::string ComponentIdentifier;

		// Load physics
		GetTSVToken(File, ComponentIdentifier);
		if(ComponentIdentifier != "") {
			if(Physics.find(ComponentIdentifier) == Physics.end())
				throw std::runtime_error("Cannot find physics component: " + ComponentIdentifier);

			ObjectStat.PhysicsStat = &Physics[ComponentIdentifier];
			ComponentIdentifier.clear();
		}
		else
			ObjectStat.PhysicsStat = nullptr;

		// Load controllers
		GetTSVToken(File, ComponentIdentifier);
		if(ComponentIdentifier != "") {
			if(Controllers.find(ComponentIdentifier) == Controllers.end())
				throw std::runtime_error("Cannot find controller component: " + ComponentIdentifier);

			ObjectStat.ControllerStat = &Controllers[ComponentIdentifier];
			ComponentIdentifier.clear();
		}
		else
			ObjectStat.ControllerStat = nullptr;

		// Load animations
		GetTSVToken(File, ComponentIdentifier);
		if(ComponentIdentifier != "") {
			if(Animations.find(ComponentIdentifier) == Animations.end())
				throw std::runtime_error("Cannot find animation component: " + ComponentIdentifier);

			ObjectStat.AnimationStat = &Animations[ComponentIdentifier];
			ComponentIdentifier.clear();
		}
		else
			ObjectStat.AnimationStat = nullptr;

		// Load renders
		GetTSVToken(File, ComponentIdentifier);
		if(ComponentIdentifier != "") {
			if(Renders.find(ComponentIdentifier) == Renders.end())
				throw std::runtime_error("Cannot find render component: " + ComponentIdentifier);

			ObjectStat.RenderStat = &Renders[ComponentIdentifier];
			ComponentIdentifier.clear();
		}
		else
			ObjectStat.RenderStat = nullptr;

		// Load shapes
		GetTSVToken(File, ComponentIdentifier);
		if(ComponentIdentifier != "") {
			if(Shapes.find(ComponentIdentifier) == Shapes.end())
				throw std::runtime_error("Cannot find shape component: " + ComponentIdentifier);

			ObjectStat.ShapeStat = &Shapes[ComponentIdentifier];
			ComponentIdentifier.clear();
		}
		else
			ObjectStat.ShapeStat = nullptr;

		// Check for duplicates
		if(ComponentIdentifier != "" && Objects.find(ObjectStat.Identifier) != Objects.end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + ObjectStat.Identifier);

		// Add row
		Objects[ObjectStat.Identifier] = ObjectStat;
	}

	// Close file
	File.close();
}

// Load physics components
void _Stats::LoadPhysics(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(1024, '\n');

	// Read data
	while(!File.eof() && File.peek() != EOF) {

		// Read row
		_PhysicsStat PhysicsStat;
		GetTSVToken(File, PhysicsStat.Identifier);
		//File >> PhysicsStat.;

		//File.ignore(1024, '\n');

		// Check for duplicates
		if(Physics.find(PhysicsStat.Identifier) != Physics.end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + PhysicsStat.Identifier);

		// Add row
		Physics[PhysicsStat.Identifier] = PhysicsStat;
	}

	// Close file
	File.close();
}

// Load controller components
void _Stats::LoadControllers(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(1024, '\n');

	// Read data
	while(!File.eof() && File.peek() != EOF) {

		// Read row
		_ControllerStat ControllerStat;
		GetTSVToken(File, ControllerStat.Identifier);
		File >> ControllerStat.Speed;

		File.ignore(1024, '\n');

		// Check for duplicates
		if(Controllers.find(ControllerStat.Identifier) != Controllers.end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + ControllerStat.Identifier);

		// Add row
		Controllers[ControllerStat.Identifier] = ControllerStat;
	}

	// Close file
	File.close();
}

// Load animation components
void _Stats::LoadAnimations(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(1024, '\n');

	// Read data
	while(!File.eof() && File.peek() != EOF) {

		// Read row
		_AnimationStat AnimationStat;
		GetTSVToken(File, AnimationStat.Identifier);

		// Read animation templates
		bool EndOfLine = false;
		while(!EndOfLine && File.peek() != EOF) {
			std::string Token;
			GetTSVToken(File, Token, &EndOfLine);

			if(Token != "")
				AnimationStat.Templates.push_back(Token);
		}

		File.ignore(1024, '\n');

		// Check for duplicates
		if(Animations.find(AnimationStat.Identifier) != Animations.end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + AnimationStat.Identifier);

		// Add row
		Animations[AnimationStat.Identifier] = AnimationStat;
	}

	// Close file
	File.close();
}

// Load render components
void _Stats::LoadRenders(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(1024, '\n');

	// Read data
	while(!File.eof() && File.peek() != EOF) {

		// Read row
		_RenderStat RenderStat;
		GetTSVToken(File, RenderStat.Identifier);
		GetTSVToken(File, RenderStat.ProgramIdentifier);
		GetTSVToken(File, RenderStat.TextureIdentifier);
		GetTSVToken(File, RenderStat.MeshIdentifier);
		File >> RenderStat.Scale;
		File >> RenderStat.Z;
		File >> RenderStat.Layer;

		File.ignore(1024, '\n');

		// Check for duplicates
		if(Renders.find(RenderStat.Identifier) != Renders.end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + RenderStat.Identifier);

		// Add row
		Renders[RenderStat.Identifier] = RenderStat;
	}

	// Close file
	File.close();
}

// Load shape components
void _Stats::LoadShapes(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(1024, '\n');

	// Read data
	while(!File.eof() && File.peek() != EOF) {

		// Read row
		_ShapeStat ShapeStat;
		GetTSVToken(File, ShapeStat.Identifier);
		File >> ShapeStat.HalfWidth[0];
		File >> ShapeStat.HalfWidth[1];

		File.ignore(1024, '\n');

		// Check for duplicates
		if(Shapes.find(ShapeStat.Identifier) != Shapes.end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + ShapeStat.Identifier);

		// Add row
		Shapes[ShapeStat.Identifier] = ShapeStat;
	}

	// Close file
	File.close();
}
