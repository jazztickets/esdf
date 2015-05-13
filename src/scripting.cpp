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
#include <scripting.h>
#include <stdexcept>

// Constructor
_Scripting::_Scripting() : LuaState(NULL) {

	// Initialize lua object
	LuaState = luaL_newstate();
	luaopen_base(LuaState);
	luaopen_math(LuaState);

	// Register C++ functions used by lua
	lua_register(LuaState, "Test", &TestFunction);
}

// Destructor
_Scripting::~_Scripting() {

	// Close lua state
	if(LuaState != nullptr)
		lua_close(LuaState);
}

// Load a script file
void _Scripting::LoadScript(const std::string &Path) {

	// Load the file
	if(luaL_dofile(LuaState, Path.c_str()) != 0)
		throw std::runtime_error("Failed to load script " + Path + "\n" + std::string(lua_tostring(LuaState, -1)));
}

// Defines a variable in the lua state
void _Scripting::DefineLuaVariable(const char *VariableName, const char *Value) {

	lua_pushstring(LuaState, Value);
	lua_setglobal(LuaState, VariableName);
}

// Calls a lua function by name
void _Scripting::CallFunction(const std::string &FunctionName) {

	// Check for the function name
	lua_getglobal(LuaState, FunctionName.c_str());
	if(!lua_isfunction(LuaState, -1)) {
		lua_pop(LuaState, 1);
		throw std::runtime_error("Failed to call function: " + FunctionName);
	}

	lua_call(LuaState, 0, 0);
}

// API functions
int _Scripting::TestFunction(lua_State *LuaState) {

	return 0;
}
