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
#pragma once

// Libraries
#include <glm/vec3.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

// Forward Declarations
class _Object;

// Base stat
struct _Stat {
	_Stat() { }
	virtual ~_Stat() { }
	std::string Identifier;
};

// Physics template
struct _PhysicsStat : public _Stat {
	int CollisionResponse;
};

// Controller template
struct _ControllerStat : public _Stat {
	float Speed;
};

// Animation template
struct _AnimationStat : public _Stat {
	std::vector<std::string> Templates;
};

// Render template
struct _RenderStat : public _Stat {
	std::string ProgramIdentifier;
	std::string TextureIdentifier;
	std::string MeshIdentifier;
	std::string ColorIdentifier;
	int Layer;
	float Scale;
	float Z;
};

// Shape template
struct _ShapeStat : public _Stat {
	glm::vec3 HalfWidth;
};

// Zone template
struct _ZoneStat : public _Stat {
};

// Shot template
struct _ShotStat : public _Stat {
};

// Health template
struct _HealthStat : public _Stat {
	int Health;
};

// Ai template
struct _AiStat : public _Stat {
};

// Objects template
struct _ObjectStat {
	std::string Identifier;
	std::string Name;
	float Lifetime;

	std::unordered_map<std::string, std::shared_ptr<const _Stat>> Components;
};

// Classes
class _Stats {

	public:

		_Stats();
		~_Stats();

		void CreateObject(_Object *Object, const std::string Identifier, bool IsServer) const;

		std::unordered_map<std::string, _ObjectStat> Objects;

	private:

		void LoadObjects(const std::string &Path);
		void LoadComponent(const std::string &Type, const std::string &Path);
		std::shared_ptr<_Stat> LoadComponentType(const std::string &Type, std::ifstream &File);

		std::unordered_map<std::string, std::unordered_map<std::string, std::shared_ptr<_Stat>>> ComponentStats;

};
