#include <stdio.h>
#include <math.h>
#include <string.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "moonscript/moonscript.h"

#define CODE(...) #__VA_ARGS__

int luaopen_lpeg (lua_State *L);

lua_State* L;
int default_stack_size;

const char* moonscript_to_lua_src = CODE(
	return require('moonscript.base').to_lua(...)
);

const char* execute_moonscript_src = CODE(
	local fn, err = require('moonscript.base').loadstring(...)

	if not fn then
		return err
	end
	return fn()
);

// put whatever is on top of stack into package.loaded under name something is
// already there
void setloaded(lua_State* l, char* name) {
	int top = lua_gettop(l);
	lua_getglobal(l, "package");
	lua_getfield(l, -1, "loaded");
	lua_getfield(l, -1, name);
	if (lua_isnil(l, -1)) {
		lua_pop(l, 1);
		lua_pushvalue(l, top);
		lua_setfield(l, -2, name);
	}

	lua_settop(l, top);
}

void setup_lua() {
	if (L) {
		return;
	}

	L = luaL_newstate();
	luaL_openlibs(L);

	luaopen_lpeg(L);
	setloaded(L, "lpeg");

	if (luaL_loadbuffer(L, (const char *)moonscript_lua, moonscript_lua_len, "moonscript.lua") != LUA_OK) {
		fprintf(stderr, "Failed to load moonscript.lua\n");
		return;
	}
	lua_call(L, 0, 0);

	default_stack_size = lua_gettop(L);
}


const char* run_moonscript(const char* input) {
	setup_lua();
	lua_pop(L, lua_gettop(L) - default_stack_size);

	if (luaL_loadbuffer(L, execute_moonscript_src, strlen(execute_moonscript_src), "execute_moonscript") != LUA_OK) {
		return "Failed to load code compiler";
	}

	lua_pushstring(L, input);
	int res = lua_pcall(L, 1, 1, 0);

	// both return value and error is returned
	return lua_tostring(L, -1);
}

const char* compile_moonscript(const char* input) {
	setup_lua();
	lua_pop(L, lua_gettop(L) - default_stack_size);

	if (luaL_loadbuffer(L, moonscript_to_lua_src, strlen(moonscript_to_lua_src), "moonscript_to_lua") != LUA_OK) {
		return "Failed to load code compiler";
	}

	lua_pushstring(L, input);
	int res = lua_pcall(L, 1, 2, 0);

	// this will return either:
	// 1. compiled code
	// 2. exception error
	// 3. compile error
	//
	// We currently make no destinction between compiled code an an error :(

	if (lua_isstring(L, -1)) {
		return lua_tostring(L, -1);
	} else {
		return lua_tostring(L, -2);
	}
}
