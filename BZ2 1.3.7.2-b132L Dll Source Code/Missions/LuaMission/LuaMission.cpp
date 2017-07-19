#include "..\..\source\fun3d\ScriptUtils.h"
#include "..\Shared\ScriptUtilsExtention.h"
#include "LuaMission.h"
#include <math.h>
#include <string.h>
#include <iostream>
//#include <shlobj.h>

LuaMission::LuaMission(void)
{
	EnableHighTPS(m_GameTPS);
	AllowRandomTracks(true);
	b_count = &b_last - &b_first - 1;
	b_array = &b_first + 1;

	/*
	f_count = &f_last - &f_first - 1;
	f_array = &f_first + 1;

	h_count = &h_last - &h_first - 1;
	h_array = &h_first + 1;
	*/

	i_count = &i_last - &i_first - 1;
	i_array = &i_first + 1;

	// Zero things out first off. If you don't assign a default value, then it will be assigned whatever value was in that memory it is assigned to use. Essentially it would be filled with random values. We don't want that. This zero's out everything under each array at the very beginning.
	if(i_array)
		memset(i_array, 0, i_count*sizeof(int));
//	if(f_array)
//		memset(f_array, 0, f_count*sizeof(float));
//	if(h_array)
//		memset(h_array, 0, h_count*sizeof(Handle));
	if(b_array)
		memset(b_array, 0, b_count*sizeof(bool));

	L = NULL;
}

LuaMission::~LuaMission()
{
	CloseOpenFiles();

	if (L)
		lua_close(L);
}



DLLBase * BuildMission(void)
{
	return new LuaMission();
}

void LuaMission::Setup(void)
{
	DidSetup = true;

	if (!L)
		return;

	// if the script has a Start function...
	lua_getglobal(L, "Start");
	if (lua_isfunction(L, -1))
	{
		// call the Start function
		LuaBindings::LuaCheckStatus(lua_pcall(L, 0, 0, 0), L, "Lua script Start error: %s");
	}
	else
	{
		lua_pop(L, 1);
	}

}

void LuaMission::InitialSetup(void)
{
	// Setup some initial stuff.
	EnableHighTPS(m_GameTPS);

	FormatConsoleMessage("Battlezone 2 LuaMission DLL created by %s", DLLAuthors);

	SetAutoGroupUnits(false);

	Difficulty = IFace_GetInteger("options.play.difficulty");
	StopScript = GetVarItemInt("network.session.ivar119");

/*
	Here's where you set the values at the start.  
*/

//  bools

//  floats

//  handles

	if(StopScript)
	{
		for(int i = 0; i < MAX_TEAMS; i++)
			for(int x = 0; x < MAX_TEAMS; x++)
				if(i != x)
					Ally(i,x);
	}

	// Load Lua Stuff here?

	// Lua file initial loading. Look for TRN value first, then map name.
	char script_filename[2 * MAX_ODF_LENGTH] = {0};

	if(IsNetworkOn())
	{
		const char *msn_filename = GetVarItemStr("network.session.svar0");
		strcpy_s(script_filename, msn_filename);
	}
	else
	{
		// Look for a script named after the mission file
		const char *TrnFileName = GetMapTRNFilename();
		strcpy_s(script_filename, TrnFileName);
	}

	if (char *dot = strchr(script_filename, '.'))
		*dot = '\0';
	strcat_s(script_filename, ".lua");

	// Skip setup if there isn't a script
	if (!DoesFileExist(script_filename))
	{
		FormatConsoleMessage("No Lua script: '%s'", script_filename);
		return;
	}

	// Create a new Lua state with default memory allocator.
	L = luaL_newstate();
	if (!L)
		return;

	// Load standard libraries
	// (excluding io and os for security; see linit.c)
	luaL_openlibs(L);

	// Register a panic handler
	lua_atpanic(L, LuaPanic);

	// If running in a debugger...
//	if (IsDebuggerPresent())
//	{
		// Hijack the print function
		lua_register(L, "print", LuaPrint);
//	}

	// Register our functions
	// (into the global table)
	lua_pushglobaltable(L);
	luaL_setfuncs(L, LuaBindings::sLuaScriptUtils, 0);

	// Create a metatable for handles
	lua_pushlightuserdata(L, NULL);
    luaL_newmetatable(L, "Handle");
    lua_pushstring(L, "Handle");
    lua_setfield(L, -2, "__type");
	lua_pushcfunction(L, LuaBindings::Handle_ToString);
    lua_setfield(L, -2, "__tostring");
    lua_setmetatable(L, -2);
	
	/*
	// Create a metatable for ParameterDB
	luaL_newmetatable(L, "ParameterDB");
	lua_pushcfunction(L, CloseODF);
	lua_setfield(L, -2, "__gc");
	lua_pop(L, 1);
	*/

	// Create a metatable for Vector
	luaL_newmetatable(L, "Vector");
	lua_pushstring(L, "Vector");
	lua_setfield(L, -2, "__type");
	lua_pushcfunction(L, LuaBindings::Vector_Index);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, LuaBindings::Vector_NewIndex);
	lua_setfield(L, -2, "__newindex");
	lua_pushcfunction(L, LuaBindings::Vector_Neg);
	lua_setfield(L, -2, "__unm");
	lua_pushcfunction(L, LuaBindings::Vector_Add);
	lua_setfield(L, -2, "__add");
	lua_pushcfunction(L, LuaBindings::Vector_Sub);
	lua_setfield(L, -2, "__sub");
	lua_pushcfunction(L, LuaBindings::Vector_Mul);
	lua_setfield(L, -2, "__mul");
	lua_pushcfunction(L, LuaBindings::Vector_Div);
	lua_setfield(L, -2, "__div");
	lua_pushcfunction(L, LuaBindings::Vector_Eq);
	lua_setfield(L, -2, "__eq");
	lua_pushcfunction(L, LuaBindings::Vector_ToString);
	lua_setfield(L, -2, "__tostring");
	// Vector is a struct so it doesn't need __gc
	lua_pop(L, 1);

	// Create a metatable for Matrix
	luaL_newmetatable(L, "Matrix");
	lua_pushstring(L, "Matrix");
	lua_setfield(L, -2, "__type");
	lua_pushcfunction(L, LuaBindings::Matrix_Index);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, LuaBindings::Matrix_NewIndex);
	lua_setfield(L, -2, "__newindex");
	lua_pushcfunction(L, LuaBindings::Matrix_Mul);
	lua_setfield(L, -2, "__mul");
	lua_pushcfunction(L, LuaBindings::Matrix_ToString);
	lua_setfield(L, -2, "__tostring");
	// Matrix is a struct so it doesn't need __gc
	lua_pop(L, 1);

	/*
	// Create a metatable for AiPaths
	luaL_newmetatable(L, "AiPath");
	lua_pushstring(L, "AiPath");
	lua_setfield(L, -2, "__type");
	lua_pushcfunction(L, FreeAiPath);
	lua_setfield(L, -2, "__gc");
	lua_pop(L, 1);
	*/

	// identity matrix
	*LuaBindings::NewMatrix(L) = Identity_Matrix;
	lua_setglobal(L, "IdentityMatrix");

	// ai commands
	lua_createtable(L, NUM_CMD, NUM_CMD);
	for (int cmd = 0; cmd < NUM_CMD; ++cmd)
	{
		lua_pushnumber(L, cmd);
		lua_pushstring(L, CommandList[cmd]);
		lua_pushvalue(L, -1);
		lua_pushvalue(L, -3);
		lua_rawset(L, -5);
		lua_rawset(L, -3);
	}
	lua_setglobal(L, "AiCommand");

	/*
	// get file data
	const char *buf = static_cast<const char *>(UseItem(script_filename));
	long len = GetItemSize(script_filename);
	*/

	char* FileBuffer;
	size_t bufSize = 0;
	LoadFile(script_filename, NULL, bufSize);
	FileBuffer = static_cast<char *>(malloc(bufSize+1));
	LoadFile(script_filename, FileBuffer, bufSize);
	FileBuffer[bufSize] = '\0';

	// load and run the script
	LuaBindings::LuaCheckStatus(luaL_loadbuffer(L, FileBuffer, bufSize, script_filename), L, "Lua script load error: %s") &&
	LuaBindings::LuaCheckStatus(lua_pcall(L, 0, LUA_MULTRET, 0), L, "Lua script run error: %s");

	// release file data
	free(FileBuffer);

	if (!L)
		return;

	FormatConsoleMessage("Lua script: '%s' loaded", script_filename);

	// if the script has an Initial Setup function
	lua_getglobal(L, "InitialSetup");
	if (lua_isfunction(L, -1))
	{
		// call the InitialSetup function.
		LuaBindings::LuaCheckStatus(lua_pcall(L, 1, 0, 0), L, "Lua script InitialSetup error: %s");
	}
	else
	{
		lua_pop(L, 1);
	}
}

bool LuaMission::Load(bool missionSave)
{
	bool ret = true;

	if (missionSave) {
		int i = 0;

		// init bools
		for (i = 0; i < b_count; i++)
			b_array[i] = false;

		/*
		// init floats
		for (i = 0; i < f_count; i++)
			f_array[i] = 99999.0f; // Speical for SP Mission.

		// init handles
		for (i = 0; i < h_count; i++)
			h_array[i] = 0;
		*/

		// init ints
		for (i = 0; i < i_count; i++)
			i_array[i] = 0;

//		Setup();
		return true;
	}

	// bools
	if(b_count)
		ret = ret && Read(b_array, b_count);

	/*
	// floats
	if(f_count)
		ret = ret && Read(f_array, f_count);

	// Handles
	if(h_count)
		ret = ret && Read(h_array, h_count);
	*/

	// ints
	if(i_count)
		ret = ret && Read(i_array, i_count);

	if (L)
	{
		// if the script has a Load function...
		lua_getglobal(L, "Load");
		if (lua_isfunction(L, -1))
		{
			// read argument values from the file
			int count = 0;
			ret = ret && Read(&count, 1);
			for (int i = 0; i < count; ++i)
			{
				ret = ret && LuaBindings::LoadValue(L, true);
			}

			// call the Load function
			LuaBindings::LuaCheckStatus(lua_pcall(L, count, 0, 0), L, "Lua script Load error: %s");
		}
		else
		{
			// skip argument values from the file
			// to prevent a missing Load function
			// from derailing the entire load process
			int count = 0;
			ret = ret && Read(&count, 1);
			for (int i = 0; i < count; ++i)
			{
				ret = ret && LuaBindings::LoadValue(L, false);
			}

			lua_pop(L, 1);
		}
	}

	return ret;
}

bool LuaMission::PostLoad(bool missionSave)
{
	bool ret = true;

	if (missionSave)
		return true;

	//ConvertHandles(h_array, h_count);

	return ret;
}

bool LuaMission::Save(bool missionSave)
{
	bool ret = true;

	if (missionSave)
		return true;
	
	// bools
	if(b_count)
		ret = ret && Write(b_array, b_count);

	/*
	// floats
	if(f_count)
		ret = ret && Write(f_array, f_count);

	// Handles
	if(h_count)
		ret = ret && Write(h_array, h_count);
	*/

	// ints
	if(i_count)
		ret = ret && Write(i_array, i_count);

	if (L)
	{
		// if the script has a Save function...
		lua_getglobal(L, "Save");
		if (lua_isfunction(L, -1))
		{
			// call the Save function
			int level = lua_gettop(L);
			if (LuaBindings::LuaCheckStatus(lua_pcall(L, 0, LUA_MULTRET, 0), L, "Lua script Save error: %s"))
			{
				// write return values to the file
				int count = lua_gettop(L) - level + 1;
				ret = ret && Write(&count, 1);
				for (int i = level; i < level + count; ++i)
				{
					ret = ret && LuaBindings::SaveValue(L, i);
				}
			}
		}
		else
		{
			// write zero return values
			int count = 0;
			ret = ret && Write(&count, 1);

			lua_pop(L, 1);
		}
	}

	return ret;
}

void LuaMission::PreOrdnanceHit(Handle shooterHandle, Handle victimHandle, int ordnanceTeam, char* pOrdnanceODF)
{
	if(!L)
		return;

	// if the script has a PreOrdnanceHit function...
	lua_getglobal(L, "PreOrdnanceHit");
	if (lua_isfunction(L, -1))
	{
		// call the PreOrdnanceHit function
		LuaBindings::PushHandle(L, shooterHandle);
		LuaBindings::PushHandle(L, victimHandle);
		lua_pushinteger(L, ordnanceTeam);
		lua_pushstring(L, pOrdnanceODF);
		LuaBindings::LuaCheckStatus(lua_pcall(L, 4, 0, 0), L, "Lua script PreOrdnanceHit error: %s");
	}
	else
	{
		lua_pop(L, 1);
	}
}

PreGetInReturnCodes LuaMission::PreGetIn(const int curWorld, Handle pilotHandle, Handle emptyCraftHandle)
{
	// Default value;
	PreGetInReturnCodes returnvalue = PREGETIN_ALLOW;

	// Use Default.
	if(!L)
		return returnvalue;

	// if the script has a PreGetIn function...
	lua_getglobal(L, "PreGetIn");
	if (lua_isfunction(L, -1))
	{
		// call the PreGetIn function
		lua_pushnumber(L, curWorld);
		LuaBindings::PushHandle(L, pilotHandle);
		LuaBindings::PushHandle(L, emptyCraftHandle);
		LuaBindings::LuaCheckStatus(lua_pcall(L, 3, 1, 0), L, "Lua script PreGetIn error: %s");

		PreGetInReturnCodes returnvalue2 = PreGetInReturnCodes(luaL_optinteger(L, 1, returnvalue));

		if(returnvalue2 != PREGETIN_ALLOW) // If the Lua return was different then the Default behavior, let Lua Override.
			returnvalue = returnvalue2;

		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
	}

	return returnvalue;
}
PreSnipeReturnCodes LuaMission::PreSnipe(const int curWorld, Handle shooterHandle, Handle victimHandle, int ordnanceTeam, char* pOrdnanceODF)
{
	// Default value.
	PreSnipeReturnCodes returnvalue = PRESNIPE_KILLPILOT;

	if(!L)
		return returnvalue;

	// if the script has a PreSnipe function...
	lua_getglobal(L, "PreSnipe");
	if (lua_isfunction(L, -1))
	{
		// call the PreSnipe function
		lua_pushinteger(L, curWorld);
		LuaBindings::PushHandle(L, shooterHandle);
		LuaBindings::PushHandle(L, victimHandle);
		lua_pushinteger(L, ordnanceTeam);
		lua_pushstring(L, pOrdnanceODF);
		LuaBindings::LuaCheckStatus(lua_pcall(L, 5, 1, 0), L, "Lua script PreSnipe error: %s");
		PreSnipeReturnCodes returnvalue2 = PreSnipeReturnCodes(luaL_optinteger(L, 1, returnvalue));

		if(returnvalue2 != PRESNIPE_KILLPILOT) // If the Lua return was different then the Default behavior, let Lua Override.
			returnvalue = returnvalue2;

		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
	}

	return returnvalue;
}

PrePickupPowerupReturnCodes LuaMission::PrePickupPowerup(const int curWorld, Handle me, Handle powerupHandle)
{
	// Default.
	PrePickupPowerupReturnCodes returnvalue = PREPICKUPPOWERUP_ALLOW;

	if (!L)
		return returnvalue;

	// if the script has a PrePickupPowerup function...
	lua_getglobal(L, "PrePickupPowerup");
	if (lua_isfunction(L, -1))
	{
		// call the PrePickupPowerup function
		lua_pushinteger(L, curWorld);
		LuaBindings::PushHandle(L, me);
		LuaBindings::PushHandle(L, powerupHandle);
		LuaBindings::LuaCheckStatus(lua_pcall(L, 3, 1, 0), L, "Lua script PrePickupPowerup error: %s");
		PrePickupPowerupReturnCodes returnvalue2 = PrePickupPowerupReturnCodes(luaL_optinteger(L, 1, returnvalue));

		if(returnvalue2 != PREPICKUPPOWERUP_ALLOW) // If the Lua return was different then the Default behavior, let Lua Override.
			returnvalue = returnvalue2;

		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
	}

	return returnvalue;
}

// Notification to the DLL: called when a pilot/craft has changed targets
void LuaMission::PostTargetChangedCallback(Handle craft, Handle previousTarget, Handle currentTarget)
{
	// Default handler for this function: no-operation.  If a derived
	// class overrides this function, it can implement custom logic.
	// if the script has an Update function...
	if (!L)
		return;

	lua_getglobal(L, "PostTargetChangeCallback");
	if (lua_isfunction(L, -1))
	{
		// call the PostTargetChangeCallback function
		LuaBindings::PushHandle(L, craft);
		LuaBindings::PushHandle(L, previousTarget);
		LuaBindings::PushHandle(L, currentTarget);
		LuaBindings::LuaCheckStatus(lua_pcall(L, 3, 0, 0), L, "Lua script PostTargetChangeCallback error: %s");
	}
	else
	{
		lua_pop(L, 1);
	}
}

void LuaMission::AddObject(Handle h)
{
	//and this -GBD
	if(StopScript)
		return;

	if (!L)
		return;

	// Add CreateObject in ontop of AddObject, since BZ2 doesn't have CreateObject.
	lua_getglobal(L, "CreateObject");
	if (lua_isfunction(L, -1))
	{
		// call the CreateObject function with the game object handle
		LuaBindings::PushHandle(L, h);
		LuaBindings::LuaCheckStatus(lua_pcall(L, 1, 0, 0), L, "Lua script CreateObject error: %s");
	}
	else
	{
		lua_pop(L, 1);
	}

	// if the script has an AddObject function
	lua_getglobal(L, "AddObject");
	if (lua_isfunction(L, -1))
	{
		// call the AddObject function with the game object handle
		LuaBindings::PushHandle(L, h);
		LuaBindings::LuaCheckStatus(lua_pcall(L, 1, 0, 0), L, "Lua script AddObject error: %s");
	}
	else
	{
		lua_pop(L, 1);
	}
}

void LuaMission::DeleteObject(Handle h)
{
	//and this -GBD
	if(StopScript)
		return;

	if (!L)
		return;

	// if the script has a DeleteObject function
	lua_getglobal(L, "DeleteObject");
	if (lua_isfunction(L, -1))
	{
		// call the DeleteObject function with the game object handle
		LuaBindings::PushHandle(L, h);
		LuaBindings::LuaCheckStatus(lua_pcall(L, 1, 0, 0), L, "Lua script DeleteObject error: %s");
	}
	else
	{
		lua_pop(L, 1);
	}
}

bool LuaMission::AddPlayer(DPID id, int Team, bool IsNewPlayer)
{
	if(!DidSetup)
		Setup();

	if (!L)
		return true;

	bool returnvalue = true;

	// if the script has an AddPlayer function...
	lua_getglobal(L, "AddPlayer");
	if (lua_isfunction(L, -1))
	{
		// call the AddPlayer function
		lua_pushinteger(L, id);
		lua_pushinteger(L, Team);
		lua_pushboolean(L, IsNewPlayer);
		LuaBindings::LuaCheckStatus(lua_pcall(L, 3, 1, 0), L, "Lua script AddPlayer error: %s");
		returnvalue = LuaBindings::luaL_optboolean(L, 1, returnvalue);
		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
	}

	return true;
}

void LuaMission::DeletePlayer(DPID id)
{
	if (!L)
		return;

	// if the script has an Update function...
	lua_getglobal(L, "DeletePlayer");
	if (lua_isfunction(L, -1))
	{
		// call the Update function
		lua_pushinteger(L, id);
		LuaBindings::LuaCheckStatus(lua_pcall(L, 1, 0, 0), L, "Lua script DeletePlayer error: %s");
	}
	else
	{
		lua_pop(L, 1);
	}
}

EjectKillRetCodes LuaMission::PlayerEjected(Handle DeadObjectHandle)
{
	if (!L)
		return DoEjectPilot;

	EjectKillRetCodes returnvalue = DoEjectPilot;

	// if the script has a PlayerEjected function...
	lua_getglobal(L, "PlayerEjected");
	if (lua_isfunction(L, -1))
	{
		// call the PlayerEjected function
		LuaBindings::PushHandle(L, DeadObjectHandle);
		LuaBindings::LuaCheckStatus(lua_pcall(L, 1, 1, 0), L, "Lua script PlayerEjected error: %s");
		returnvalue = EjectKillRetCodes(luaL_optinteger(L, 1, returnvalue));
		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
	}

	return returnvalue;
}

EjectKillRetCodes LuaMission::ObjectKilled(int DeadObjectHandle, int KillersHandle)
{
	if (!L)
	{
		if(!IsPlayer(DeadObjectHandle)) // J.Random AI-controlled Object is toast
			return DLLHandled;
		else // Player dead
			return DoEjectPilot;
	}

	EjectKillRetCodes returnvalue = IsPlayer(DeadObjectHandle) ? DoEjectPilot : DLLHandled;

	// if the script has an ObjectKilled function...
	lua_getglobal(L, "ObjectKilled");
	if (lua_isfunction(L, -1))
	{
		// call the ObjectKilled function
		LuaBindings::PushHandle(L, DeadObjectHandle);
		LuaBindings::PushHandle(L, KillersHandle);
		LuaBindings::LuaCheckStatus(lua_pcall(L, 2, 1, 0), L, "Lua script ObjectKilled error: %s");
		returnvalue = EjectKillRetCodes(luaL_optinteger(L, 1, returnvalue));
		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
	}
	
	return returnvalue;
}

EjectKillRetCodes LuaMission::ObjectSniped(int DeadObjectHandle, int KillersHandle)
{
	if (!L)
	{	
		if(!IsPlayer(DeadObjectHandle)) // J.Random AI-controlled Object is toast
			return DLLHandled;
		else // Player dead
			return DoGameOver; //DoRespawnSafest;
	}

	EjectKillRetCodes returnvalue = IsPlayer(DeadObjectHandle) ? DoGameOver : DLLHandled;

	// if the script has an ObjectSniped function...
	lua_getglobal(L, "ObjectSniped");
	if (lua_isfunction(L, -1))
	{
		// call the ObjectSniped function
		LuaBindings::PushHandle(L, DeadObjectHandle);
		LuaBindings::PushHandle(L, KillersHandle);
		LuaBindings::LuaCheckStatus(lua_pcall(L, 2, 1, 0), L, "Lua script ObjectSniped error: %s");
		returnvalue = EjectKillRetCodes(luaL_optinteger(L, 1, returnvalue));
		lua_pop(L, 1);
	}
	else
	{
		lua_pop(L, 1);
	}

	return returnvalue;
}


void LuaMission::Execute(void)
{
	// Do init first! -GBD
	if (!DidSetup)
		Setup();

	// Count the turns each tick. Multiply or Divide by m_GameTPS to translate into seconds. (divide to turn m_ElapsedGameTime into seconds,
	// Multiply # of seconds by m_GameTPS to turn it into ticks per second.
	m_ElapsedGameTime++;

	if(!StopScript)
	{

/*
	Here is where you put what happens every frame.  
*/

		if (!L)
			return;

		// if the script has an Update function...
		lua_getglobal(L, "Update");
		if (lua_isfunction(L, -1))
		{
			// call the Update function
			LuaBindings::LuaCheckStatus(lua_pcall(L, 0, 0, 0), L, "Lua script Update error: %s");
		}
		else
		{
			lua_pop(L, 1);
		}

	}
}

void LuaMission::PostRun(void)
{
	if(!StopScript)
	{
		if (!L)
			return;

		// if the script has a PostRun function...
		lua_getglobal(L, "PostRun");
		if (lua_isfunction(L, -1))
		{
			// call the PostRun function
			LuaBindings::LuaCheckStatus(lua_pcall(L, 0, 0, 0), L, "Lua script PostRun error: '%s'");
		}
		else
		{
			lua_pop(L, 1);
		}

	}

}