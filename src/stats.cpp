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
#include <objects/prop.h>
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
	LoadObjects(STATS_OBJECTS);
	LoadProps(STATS_PROPS);
}

// Destructor
_Stats::~_Stats() {
}

// Object factory
_Object *_Stats::CreateObject(const std::string Identifier, bool IsServer) const {
	const auto &Iterator = Objects.find(Identifier);
	if(Iterator == Objects.end())
		return nullptr;

	const _ObjectStat *ObjectStat = &Iterator->second;

	// Create object
	_Object *Object = new _Object();
	Object->Identifier = Identifier;

	// Create physics
	if(ObjectStat->PhysicsStat) {
		Object->Physics = new _Physics(Object);
		Object->Physics->Radius = ObjectStat->PhysicsStat->Radius;
	}

	// Create controller
	if(ObjectStat->ControllersStat) {
		Object->Controller = new _Controller(Object);
		Object->Controller->Speed = ObjectStat->ControllersStat->Speed;
	}

	// Create animation
	if(ObjectStat->AnimationsStat) {
		Object->Animation = new _Animation(Object);

		// Load animation templates
		for(const auto &Template : ObjectStat->AnimationsStat->Templates)
			Object->Animation->Templates.push_back(Assets.AnimationTemplates[Template]);

		// Set default frame
		Object->Animation->Stop();
		Object->Animation->FramePeriod = 0.07;
	}

	// Create render
	if(IsServer) {
		Object->Physics->Interpolate = false;
	}
	else if(ObjectStat->RendersStat){
		Object->Render = new _Render(Object);
		Object->Render->Icon = Assets.Textures[ObjectStat->RendersStat->Icon];
		Object->Render->Scale = ObjectStat->RendersStat->Scale;
		Object->Render->Z = ObjectStat->RendersStat->Z;
		Object->Render->Layer = ObjectStat->RendersStat->Layer;
	}

	return Object;
}

// Prop factory
_Prop *_Stats::CreateProp(const std::string Identifier) const {
	const auto &Iterator = Props.find(Identifier);
	if(Iterator == Props.end())
		return nullptr;

	const _PropStat &PropStat = Iterator->second;

	// Create prop
	_Prop *Prop = new _Prop(PropStat);
	Prop->Program = Assets.Programs["pos_uv_norm"];
	Prop->Mesh = Assets.Meshes[PropStat.MeshIdentifier];
	Prop->Texture = Assets.Textures[PropStat.TextureIdentifier];

	return Prop;
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

			ObjectStat.ControllersStat = &Controllers[ComponentIdentifier];
			ComponentIdentifier.clear();
		}
		else
			ObjectStat.ControllersStat = nullptr;

		// Load animations
		GetTSVToken(File, ComponentIdentifier);
		if(ComponentIdentifier != "") {
			if(Animations.find(ComponentIdentifier) == Animations.end())
				throw std::runtime_error("Cannot find animation component: " + ComponentIdentifier);

			ObjectStat.AnimationsStat = &Animations[ComponentIdentifier];
			ComponentIdentifier.clear();
		}
		else
			ObjectStat.AnimationsStat = nullptr;

		// Load renders
		GetTSVToken(File, ComponentIdentifier);
		if(ComponentIdentifier != "") {
			if(Renders.find(ComponentIdentifier) == Renders.end())
				throw std::runtime_error("Cannot find render component: " + ComponentIdentifier);

			ObjectStat.RendersStat = &Renders[ComponentIdentifier];
			ComponentIdentifier.clear();
		}
		else
			ObjectStat.RendersStat = nullptr;

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
		File >> PhysicsStat.Radius;

		File.ignore(1024, '\n');

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
		_ControllersStat ControllersStat;
		GetTSVToken(File, ControllersStat.Identifier);
		File >> ControllersStat.Speed;

		File.ignore(1024, '\n');

		// Check for duplicates
		if(Controllers.find(ControllersStat.Identifier) != Controllers.end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + ControllersStat.Identifier);

		// Add row
		Controllers[ControllersStat.Identifier] = ControllersStat;
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
		_AnimationsStat AnimationsStat;
		GetTSVToken(File, AnimationsStat.Identifier);

		// Read animation templates
		bool EndOfLine = false;
		while(!EndOfLine && File.peek() != EOF) {
			std::string Token;
			GetTSVToken(File, Token, &EndOfLine);

			if(Token != "")
				AnimationsStat.Templates.push_back(Token);
		}

		File.ignore(1024, '\n');

		// Check for duplicates
		if(Animations.find(AnimationsStat.Identifier) != Animations.end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + AnimationsStat.Identifier);

		// Add row
		Animations[AnimationsStat.Identifier] = AnimationsStat;
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
		_RendersStat RendersStat;
		GetTSVToken(File, RendersStat.Identifier);
		GetTSVToken(File, RendersStat.Icon);
		File >> RendersStat.Scale;
		File >> RendersStat.Z;
		File >> RendersStat.Layer;

		File.ignore(1024, '\n');

		// Check for duplicates
		if(Renders.find(RendersStat.Identifier) != Renders.end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + RendersStat.Identifier);

		// Add row
		Renders[RendersStat.Identifier] = RendersStat;
	}

	// Close file
	File.close();
}

// Load prop stats
void _Stats::LoadProps(const std::string &Path) {

	// Load file
	std::ifstream File(Path, std::ios::in);
	if(!File)
		throw std::runtime_error("Error loading: " + Path);

	// Skip header
	File.ignore(1024, '\n');

	// Read data
	while(!File.eof() && File.peek() != EOF) {

		// Read row
		_PropStat PropStat;
		GetTSVToken(File, PropStat.Identifier);
		GetTSVToken(File, PropStat.MeshIdentifier);
		GetTSVToken(File, PropStat.TextureIdentifier);
		File >> PropStat.Radius;

		File.ignore(1024, '\n');

		// Check for duplicates
		if(Props.find(PropStat.Identifier) != Props.end())
			throw std::runtime_error("Duplicate entry in file " + Path + ": " + PropStat.Identifier);

		// Add row
		Props[PropStat.Identifier] = PropStat;
	}

	// Close file
	File.close();
}
