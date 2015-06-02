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
#include <glm/vec3.hpp>
#include <unordered_map>
#include <string>
#include <vector>

// Forward Declarations
class _Object;
struct _ItemStat;

// Physics template
struct _PhysicsStat {
	std::string Identifier;
};

// Controller template
struct _ControllerStat {
	std::string Identifier;
	float Speed;
};

// Animation template
struct _AnimationStat {
	std::string Identifier;
	std::vector<std::string> Templates;
};

// Render template
struct _RenderStat {
	std::string Identifier;
	std::string ProgramIdentifier;
	std::string TextureIdentifier;
	std::string MeshIdentifier;
	std::string ColorIdentifier;
	int Layer;
	float Scale;
	float Z;
};

// Shape template
struct _ShapeStat {
	std::string Identifier;
	glm::vec3 HalfWidth;
};

// Objects template
struct _ObjectStat {
	std::string Identifier;
	std::string Name;
	const _PhysicsStat *PhysicsStat;
	const _ControllerStat *ControllerStat;
	const _AnimationStat *AnimationStat;
	const _RenderStat *RenderStat;
	const _ShapeStat *ShapeStat;
};

// Classes
class _Stats {

	public:

		_Stats();
		~_Stats();

		_Object *CreateObject(const std::string Identifier, bool IsServer) const;

		std::unordered_map<std::string, _ObjectStat> Objects;

	private:

		void LoadObjects(const std::string &Path);
		void LoadPhysics(const std::string &Path);
		void LoadControllers(const std::string &Path);
		void LoadAnimations(const std::string &Path);
		void LoadRenders(const std::string &Path);
		void LoadShapes(const std::string &Path);

		std::unordered_map<std::string, _PhysicsStat> Physics;
		std::unordered_map<std::string, _ControllerStat> Controllers;
		std::unordered_map<std::string, _AnimationStat> Animations;
		std::unordered_map<std::string, _RenderStat> Renders;
		std::unordered_map<std::string, _ShapeStat> Shapes;

};
