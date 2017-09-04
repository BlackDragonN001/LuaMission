#pragma once

#include "..\lua_5_1_5\src\lua.hpp"
#include "..\..\source\fun3d\ScriptUtils.h"
#include "..\Shared\ScriptUtilsExtention.h"

#define lua_pushglobaltable(L) lua_pushvalue(L, LUA_GLOBALSINDEX);

struct lua_State;

extern "C" void luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup);
extern "C" void *luaL_testudata(lua_State *L, int ud, const char *tname);
extern "C" int LuaPrint(lua_State *L);
extern "C" int LuaPanic(lua_State *L);

namespace LuaBindings {
	bool LuaCheckStatus(int status, lua_State *L, const char *format);
	void PushHandle(lua_State *L, Handle h);
	int GetHandle(lua_State *L);
	Handle GetHandle(lua_State *L, int n);
	Handle RequireHandle(lua_State *L, int n);
	int Handle_ToString(lua_State *L);
	bool luaL_optboolean(lua_State *L, int n, int defval);

	// Math Stuff
	Vector *GetVector(lua_State *L, int n);
	Vector *RequireVector(lua_State *L, int n);
	Vector *NewVector(lua_State *L);
	int Vector_Index(lua_State *L);
	int Vector_NewIndex(lua_State *L);
	int Vector_ToString(lua_State *L);
	int Vector_Neg(lua_State *L);
	int Vector_Add(lua_State *L);
	int Vector_Sub(lua_State *L);
	int Vector_Mul(lua_State *L);
	int Vector_Div(lua_State *L);
	int Vector_Eq(lua_State *L);
	int SetVector(lua_State *L);

	Matrix *GetMatrix(lua_State *L, int n);
	Matrix *RequireMatrix(lua_State *L, int n);
	Matrix *NewMatrix(lua_State *L);
	int Matrix_Index(lua_State *L);
	int Matrix_NewIndex(lua_State *L);
	int Matrix_Mul(lua_State *L);
	int Matrix_ToString(lua_State *L);
	int SetMatrix(lua_State *L);

	Quaternion *GetQuaternion(lua_State *L, int n);
	Quaternion *RequireQuaternion(lua_State *L, int n);
	Quaternion *NewQuaternion(lua_State *L);
	int Quaternion_Index(lua_State *L);
	int Quaternion_NewIndex(lua_State *L);
	int Quaternion_ToString(lua_State *L);
	int SetQuaternion(lua_State *L);

	int DotProduct(lua_State *L);
	int CrossProduct(lua_State *L);
	int Normalize(lua_State *L);
	int Length(lua_State *L);
	int LengthSquared(lua_State *L);
	int Distance2D(lua_State *L);
	int Distance2DSquared(lua_State *L);
	int Distance3D(lua_State *L);
	int Distance3DSquared(lua_State *L);

	int BuildAxisRotationMatrix(lua_State *L);
	int BuildPositionRotationMatrix(lua_State *L);
	const Vector zero(0.0f, 0.0f, 0.0f);
	const Vector y_axis(0.0f, 1.0f, 0.0f);
	const Vector z_axis(0.0f, 0.0f, 1.0f);
	int BuildOrthogonalMatrix(lua_State *L);
	int BuildDirectionalMatrix(lua_State *L);

	int SetQuaternion(lua_State *L);
	int Interpolate_Matrix(lua_State *L);
	int Interpolate_Quaternion(lua_State *L);
	int Normalize_Quaternion(lua_State *L);
	int Matrix_to_QuatPos(lua_State *L);
	int Matrix_to_Quaternion(lua_State *L);
	int QuatPos_to_Matrix(lua_State *L);
	int Vector_Transform(lua_State *L);
	int Vector_TransformInv(lua_State *L);
	int Vector_Rotate(lua_State *L);
	int Vector_RotateInv(lua_State *L);
	int Matrix_Inverse(lua_State *L);
	int Build_Yaw_Matrix(lua_State *L);
	int Add_Mult_Vectors(lua_State *L);

	int IsNullVector(lua_State *L);
	int FrontToRadian(lua_State *L);
	int FrontToDegrees(lua_State *L);
	int GetFacingDirection(lua_State *L);
	int GetFacingDrection2D(lua_State *L);

	// Script Util Functions:

	int Make_RGB(lua_State *L);
	int Make_RGBA(lua_State *L);
	/*
	int GetNextRandomVehicleODF(lua_State *L);
	int SetWorld(lua_State *L);
	int ProcessCommand(lua_State *L);
	int SetRandomSeed(lua_State *L);
	*/
	int FailMission(lua_State *L);
	int SucceedMission(lua_State *L);
	int ChangeSide(lua_State *L);
	int AddScrap(lua_State *L);
	int SetScrap(lua_State *L);
	int GetScrap(lua_State *L);
	int AddMaxScrap(lua_State *L);
	int SetMaxScrap(lua_State *L);
	int GetMaxScrap(lua_State *L);
	int GetTime(lua_State *L);
	int GetTug(lua_State *L);
	int HasCargo(lua_State *L);
	int GetDistance(lua_State *L);
	int GetNearestObject(lua_State *L);
	int GetNearestVehicle(lua_State *L);
	int GetNearestBuilding(lua_State *L);
	int GetNearestEnemy(lua_State *L);
	int BuildObject(lua_State *L);
	int RemoveObject(lua_State *L);
	int GetFirstEmptyGroup(lua_State *L);
	int SetGroup(lua_State *L);
	int Attack(lua_State *L);
	int Service(lua_State *L);
	int Goto(lua_State *L);
	int Mine(lua_State *L);
	int Follow(lua_State *L);
	int Defend(lua_State *L);
	int Defend2(lua_State *L);
	int Stop(lua_State *L);
	int Patrol(lua_State *L);
	int Retreat(lua_State *L);
	int GetIn(lua_State *L);
	int Pickup(lua_State *L);
	int Dropoff(lua_State *L);
	int Build(lua_State *L);
	int LookAt(lua_State *L);
	int AllLookAt(lua_State *L);
	int IsOdf(lua_State *L);
	int GetRace(lua_State *L);
	int GetPlayerHandle(lua_State *L);
	int IsAlive(lua_State *L);
	int IsFlying(lua_State *L);
	int IsAliveAndPilot(lua_State *L);
	int IsAround(lua_State *L);
	int InBuilding(lua_State *L);
	int AtTerminal(lua_State *L);
	int GetPosition(lua_State *L);
	int GetPosition2(lua_State *L);
	int GetFront(lua_State *L);
	int SetPosition(lua_State *L);
	int Damage(lua_State *L);
	int GetHealth(lua_State *L);
	int GetCurHealth(lua_State *L);
	int GetMaxHealth(lua_State *L);
	int SetCurHealth(lua_State *L);
	int SetMaxHealth(lua_State *L);
	int AddHealth(lua_State *L);
	int GetAmmo(lua_State *L);
	int GetCurAmmo(lua_State *L);
	int GetMaxAmmo(lua_State *L);
	int SetCurAmmo(lua_State *L);
	int SetMaxAmmo(lua_State *L);
	int AddAmmo(lua_State *L);
	int GetTeamNum(lua_State *L);
	int SetTeamNum(lua_State *L);
	int SetVelocity(lua_State *L);
	int SetControls(lua_State *L);
	int GetWhoShotMe(lua_State *L);
	int GetLastEnemyShot(lua_State *L);
	int GetLastFriendShot(lua_State *L);
	int DefaultAllies(lua_State *L);
	int TeamplayAllies(lua_State *L);
	int Ally(lua_State *L);
	int UnAlly(lua_State *L);
	int IsTeamAllied(lua_State *L);
	int IsAlly(lua_State *L);
	int AudioMessage(lua_State *L);
	int IsAudioMessageDone(lua_State *L);
	int StopAudioMessage(lua_State *L);
	int PreloadAudioMessage(lua_State *L);
	int PurgeAudioMessage(lua_State *L);
	int PreloadMusicMessage(lua_State *L);
	int PurgeMusicMessage(lua_State *L);
	int LoadJukeFile(lua_State *L);
	int SetMusicIntensity(lua_State *L);
	/* // AiPath pointer data type not supported from Lua. -GBD
	AiPath **NewAiPath(lua_State *L);
	AiPath **GetAiPath(lua_State *L, int n);
	int FindAiPath(lua_State *L);
	int FreeAiPath(lua_State *L);
	return 0;
	*/
	int GetAiPaths(lua_State *L);
	int SetPathType(lua_State *L);
	int SetPathOneWay(lua_State *L);
	int SetPathRoundTrip(lua_State *L);
	int SetPathLoop(lua_State *L);
	int SetIndependence(lua_State *L);
	int IsInfo(lua_State *L);
	int StartCockpitTimer(lua_State *L);
	int StartCockpitTimerUp(lua_State *L);
	int StopCockpitTimer(lua_State *L);
	int HideCockpitTimer(lua_State *L);
	int GetCockpitTimer(lua_State *L);
	int StartEarthQuake(lua_State *L);
	int UpdateEarthQuake(lua_State *L);
	int StopEarthQuake(lua_State *L);
	int IsPerson(lua_State *L);
	int GetCurWorld(lua_State *L);
	int GetVarItemStr(lua_State *L);
	int GetVarItemInt(lua_State *L);
	int GetCVarItemInt(lua_State *L);
	int GetCVarItemStr(lua_State *L);
	int PreloadODF(lua_State *L);
	int TerrainFindFloor(lua_State *L);
	int AddPilotByHandle(lua_State *L);
	int PrintConsoleMessage(lua_State *L);
	int GetRandomFloat(lua_State *L);
	int IsDeployed(lua_State *L);
	int Deploy(lua_State *L);
	int IsSelected(lua_State *L);
	int SetWeaponMask(lua_State *L);
	int GiveWeapon(lua_State *L);
	int FireAt(lua_State *L);
	int IsFollowing(lua_State *L);
	int WhoFollowing(lua_State *L);
	int SetUserTarget(lua_State *L);
	int GetUserTarget(lua_State *L);
	int SetPerceivedTeam(lua_State *L);
	int GetCurrentCommand(lua_State *L);
	int GetCurrentWho(lua_State *L);
	int EjectPilot(lua_State *L);
	int HopOut(lua_State *L);
	int KillPilot(lua_State *L);
	int RemovePilot(lua_State *L);
	int HoppedOutOf(lua_State *L);
	int GetCameraPosition(lua_State *L);
	int SetCameraPosition(lua_State *L);
	int ResetCameraPosition(lua_State *L);
	int CalcCRC(lua_State *L);
	int IFace_Exec(lua_State *L);
	int IFace_Activate(lua_State *L);
	int IFace_Deactivate(lua_State *L);
	int IFace_CreateCommand(lua_State *L);
	int IFace_CreateString(lua_State *L);
	int IFace_SetString(lua_State *L);
	int IFace_GetString(lua_State *L);
	int IFace_CreateInteger(lua_State *L);
	int IFace_SetInteger(lua_State *L);
	int IFace_GetInteger(lua_State *L);
	int IFace_SetIntegerRange(lua_State *L);
	int IFace_CreateFloat(lua_State *L);
	int IFace_SetFloat(lua_State *L);
	int IFace_GetFloat(lua_State *L);
	int IFace_ClearListBox(lua_State *L);
	int IFace_AddTextItem(lua_State *L);
	int IFace_GetSelectedItem(lua_State *L);
	int SetSkill(lua_State *L);
	int SetAIP(lua_State *L);
	int LogFloat(lua_State *L);
	int GetInstantMyForce(lua_State *L);
	int GetInstantCompForce(lua_State *L);
	int GetInstantDifficulty(lua_State *L);
	int GetInstantGoal(lua_State *L);
	int GetInstantType(lua_State *L);
	int GetInstantFlag(lua_State *L);
	int GetInstantMySide(lua_State *L);
	/* // Function exists in ScriptUtils.h, but doesn't exist in the .cpp or bzone.lib. No such function.
	int StoppedPlayback(lua_State *L);
	*/
	int CameraReady(lua_State *L);
	int CameraPath(lua_State *L);
	int CameraPathDir(lua_State *L);
	int PanDone(lua_State *L);
	int CameraObject(lua_State *L);
	int CameraFinish(lua_State *L);
	int CameraCancelled(lua_State *L);
	int FreeCamera(lua_State *L);
	int FreeFinish(lua_State *L);
	int FreeCamera(lua_State *L);
	int FreeFinish(lua_State *L);
	int PlayMovie(lua_State *L);
	int StopMovie(lua_State *L);
	int PlayMove(lua_State *L);
	int PlayRecording(lua_State *L);
	int PlaybackVehicle(lua_State *L);
	int SetAnimation(lua_State *L);
	int GetAnimationFrame(lua_State *L);
	int StartAnimation(lua_State *L);
	int MaskEmitter(lua_State *L);
	int StartEmitter(lua_State *L);
	int StopEmitter(lua_State *L);
	/* // Depreciated functions.
	int SaveObjects(lua_State *L);
	int LoadObjects(lua_State *L);
	int IgnoreSync(lua_State *L);
	int IsRecording(lua_State *L);
	*/
	int SetObjectiveOn(lua_State *L);
	int SetObjectiveOff(lua_State *L);
	int SetObjectiveName(lua_State *L);
	int ClearObjectives(lua_State *L);
	long GetColor(const char * const colorname);
	int AddObjective(lua_State *L);
	int IsWithin(lua_State *L);
	int CountUnitsNearObject(lua_State *L);
	int SetAvoidType(lua_State *L);
	int Annoy(lua_State *L);
	int ClearThrust(lua_State *L);
	int SetVerbose(lua_State *L);
	int ClearIdleAnims(lua_State *L);
	int AddIdleAnim(lua_State *L);
	int IsIdle(lua_State *L);
	int CountThreats(lua_State *L);
	int SpawnBirds(lua_State *L);
	int RemoveBirds(lua_State *L);
	int SetColorFade(lua_State *L);
	int StopCheats(lua_State *L);
	int CalcCliffs(lua_State *L);
	int StartSoundEffect(lua_State *L);
	int FindSoundEffect(lua_State *L);
	int StopSoundEffect(lua_State *L);
	int GetObjectByTeamSlot(lua_State *L);
	int IsNetworkOn(lua_State *L);
	int ImServer(lua_State *L);
	int ImDedicatedServer(lua_State *L);
	int IsTeamplayOn(lua_State *L);
	int CountPlayers(lua_State *L);
	int GetRaceOfTeam(lua_State *L);
	int IsPlayer(lua_State *L);
	int GetPlayerName(lua_State *L);
	int WhichTeamGroup(lua_State *L);
	int CountAllies(lua_State *L);
	int GetCommanderTeam(lua_State *L);
	int GetFirstAlliedTeam(lua_State *L);
	int GetLastAlliedTeam(lua_State *L);
	int GetTeamplayRanges(lua_State *L);
	int SetRandomHeadingAngle(lua_State *L);
	int ClearTeamColors(lua_State *L);
	int DefaultTeamColors(lua_State *L);
	int TeamplayTeamColors(lua_State *L);
	int SetTeamColor(lua_State *L);
	int ClearTeamColor(lua_State *L);
	int MakeInert(lua_State *L);
	int GetPositionNear(lua_State *L);
	int GetPlayerODF(lua_State *L);
	int BuildEmptyCraftNear(lua_State *L);
	int GetCircularPos(lua_State *L);
	int GetSafestSpawnpoint(lua_State *L);
	int GetSpawnpoint(lua_State *L);
	int GetSpawnpointHandle(lua_State *L);
	int GetRandomSpawnpoint(lua_State *L);
	int SetTimerBox(lua_State *L);
	int AddToMessagesBox(lua_State *L);
	int GetDeaths(lua_State *L);
	int GetKills(lua_State *L);
	int GetScore(lua_State *L);
	int SetDeaths(lua_State *L);
	int SetKills(lua_State *L);
	int SetScore(lua_State *L);
	int AddDeaths(lua_State *L);
	int AddKills(lua_State *L);
	int AddScore(lua_State *L);
	int SetAsUser(lua_State *L);
	int SetNoScrapFlagByHandle(lua_State *L);
	int ClearNoScrapFlagByHandle(lua_State *L);
	int GetLocalPlayerTeamNumber(lua_State *L);
	int GetLocalPlayerDPID(lua_State *L);
	int FlagSteal(lua_State *L);
	int FlagRecover(lua_State *L);
	int FlagScore(lua_State *L);
	int MoneyScore(lua_State *L);
	int NoteGameoverByTimelimit(lua_State *L);
	int NoteGameoverByKillLimit(lua_State *L);
	int NoteGameoverByScore(lua_State *L);
	int NoteGameoverByLastWithBase(lua_State *L);
	int NoteGameoverByLastTeamWithBase(lua_State *L);
	int NoteGameoverByNoBases(lua_State *L);
	int DoGameover(lua_State *L);
	int SetMPTeamRace(lua_State *L);
	int GetTarget(lua_State *L);
	int IFace_ConsoleCmd(lua_State *L);
	int Network_SetString(lua_State *L);
	int Network_SetInteger(lua_State *L);
	int GetVelocity(lua_State *L);
	/*
	Get_CFG, // Returns the GameObjectClass's cfg string
	Get_ODF, // Returns the ODF of the object
	Get_GOClass_gCfg, // Returns the GameObjectClass's gCfg string (not 100% sure how it differs from the CFG) (It returns the ODF's BaseName. Even reads through ODF inheritence. :) ) -GBD
	Get_EntityType,
	Get_GOClass,
	Get_Weapon0Config,
	Get_Weapon1Config,
	Get_Weapon2Config,
	Get_Weapon3Config,
	Get_Weapon4Config,
	Get_Weapon0ODF,
	Get_Weapon1ODF,
	Get_Weapon2ODF,
	Get_Weapon3ODF,
	Get_Weapon4ODF,
	Get_Weapon0GOClass,
	Get_Weapon1GOClass,
	Get_Weapon2GOClass,
	Get_Weapon3GOClass,
	Get_Weapon4GOClass,
	*/
	int GetObjInfo_CFG(lua_State *L);;
	int GetObjInfo_ODF(lua_State *L);;
	int GetObjInfo_GOClass_gCfg(lua_State *L);;
	int GetObjInfo_EntityType(lua_State *L);;
	int GetObjInfo_GOClass(lua_State *L);;
	int Get_WeaponConfig(lua_State *L);;
	int Get_WeaponODF(lua_State *L);;
	int Get_WeaponGOClass(lua_State *L);;
	int DoesODFExist(lua_State *L);
	int IsAlive2(lua_State *L);
	int IsFlying2(lua_State *L);
	int IsAliveAndPilot2(lua_State *L);
	int TranslateString2(lua_State *L);
	int GetScavengerCurScrap(lua_State *L);
	int GetScavengerMaxScrap(lua_State *L);
	int SetScavengerCurScrap(lua_State *L);
	int SetScavengerMaxScrap(lua_State *L);
	int SelfDamage(lua_State *L);
	int WantBotKillMessages(lua_State *L);
	int EnableHighTPS(lua_State *L);
	int GetLocalUserInspectHandle(lua_State *L);
	int GetLocalUserSelectHandle(lua_State *L);
	int ResetTeamSlot(lua_State *L);
	int GetCategoryTypeOverride(lua_State *L);
	int GetCategoryType(lua_State *L);
	int GetODFHexInt(lua_State *L);
	int GetODFInt(lua_State *L);
	int GetODFLong(lua_State *L);
	int GetODFFloat(lua_State *L);
	int GetODFDouble(lua_State *L);
	int GetODFChar(lua_State *L);
	int GetODFBool(lua_State *L);
	int GetODFString(lua_State *L);;
	int GetODFColor(lua_State *L);
	int GetODFVector(lua_State *L);
	/* // Functions removed, the above functions handle it internally.
	int OpenODF(lua_State *L);
	int CloseODF(lua_State *L);
	*/
	int NoteGameoverWithCustomMessage(lua_State *L);
	int SetBestGroup(lua_State *L);
	int GetGroup(lua_State *L);;
	int GetGroupCount(lua_State *L);
	int SetLifespan(lua_State *L);
	int DoesFileExist(lua_State *L);
	int LoadFile(lua_State *L);
	int StartAudio3D(lua_State *L);
	int StartAudio2D(lua_State *L);
	int IsAudioPlaying(lua_State *L);
	int StopAudio(lua_State *L);
	int PauseAudio(lua_State *L);
	int ResumeAudio(lua_State *L);
	int SetVolume(lua_State *L);
	int SetPan(lua_State *L);
	int SetRate(lua_State *L);
	int GetAudioFileDuration(lua_State *L);
	int IsPlayingLooped(lua_State *L);
	int GetNearestPowerup(lua_State *L);
	int GetNearestPerson(lua_State *L);
	int SetCommand(lua_State *L);
	int SetGravity(lua_State *L);
	int SetAutoGroupUnits(lua_State *L);
	int KickPlayer(lua_State *L);
	int TerrainIsWater(lua_State *L);
	int GetTerrainHeightAndNormal(lua_State *L);
	int WriteToFile(lua_State *L);
	int GetPathPoints(lua_State *L);
	int GetOwner(lua_State *L);
	int SetTarget(lua_State *L);
	int SetOwner(lua_State *L);
	int SetPilotClass(lua_State *L);
	int AllowRandomTracks(lua_State *L);
	int SetFFATeamColors(lua_State *L);
	int SetTeamStratColors(lua_State *L);
	int GetFFATeamColor(lua_State *L);
	int GetFFATeamColorVector(lua_State *L);
	int GetTeamStratColor(lua_State *L);
	int GetTeamStratColorVector(lua_State *L);
	int SwapTeamStratColors(lua_State *L);
	int GetTeamColorsAreFFA(lua_State *L);
	int SetTeamColors(lua_State *L);
	int AddPower(lua_State *L);
	int SetPower(lua_State *L);
	int GetPower(lua_State *L);
	int GetMaxPower(lua_State *L);
	int AddMaxPower(lua_State *L);
	int SetMaxPower(lua_State *L);
	int GetTeamStratIndividualColor(lua_State *L);
	int GetTeamStratIndividualColorVector(lua_State *L);
	int GetMapTRNFilename(lua_State *L);
	int IsNotDeadAndPilot2(lua_State *L);
	int GetLabel(lua_State *L);
	int SetLabel(lua_State *L);
	int GetTap(lua_State *L);
	int SetTap(lua_State *L);
	int GetCurLocalAmmo(lua_State *L);
	int AddLocalAmmo(lua_State *L);
	int GetMaxLocalAmmo(lua_State *L);
	int SetCurLocalAmmo(lua_State *L);
	int GetNetworkListItem(lua_State *L);
	int GetNetworkListCount(lua_State *L);
	int GetTeamRelationship(lua_State *L);
	int HasPilot(lua_State *L);
	int GetPilotClass(lua_State *L);
	int GetBaseScrapCost(lua_State *L);
	int GetActualScrapCost(lua_State *L);
	int PetWatchdogThread(lua_State *L);
	int GetPerceivedTeam(lua_State *L);
	int SetLastCurrentPosition(lua_State *L);
	int GetRemainingLifespan(lua_State *L);
	int GetAllSpawnpoints(lua_State *L);
	int GetPlan(lua_State *L);
	int GetIndependence(lua_State *L);
	int GetSkill(lua_State *L);
	int GetObjectiveName(lua_State *L);
	int GetWeaponMask(lua_State *L);
	int SetInterpolablePosition(lua_State *L);
	int SecondsToTurns(lua_State *L);
	int TurnsToSeconds(lua_State *L);
	int GetLifeSpan(lua_State *L);
	int GetCurrentCommandWhere(lua_State *L);
	int GetAllGameObjectHandles(lua_State *L);
	int GetOmega(lua_State *L);
	int SetOmega(lua_State *L);
	int IsCraftButNotPerson(lua_State *L);
	int IsCraftOrPerson(lua_State *L);
	int IsBuilding(lua_State *L);
	int IsPowerup(lua_State *L);
	int SetCanSnipe(lua_State *L);
	int GetCanSnipe(lua_State *L);
	int WhoIsTargeting(lua_State *L);
	// BZ2 DLL Utility Functions.
	//int DoTaunt(lua_State *L);
	//int SetTauntCPUName(lua_State *L);
	// BZScriptor Backwards Compatability functions.
	int SetAngle(lua_State *L);
	int CameraPos(lua_State *L);
	int CameraOf(lua_State *L);
	int Move(lua_State *L);
	int ReplaceObject(lua_State *L);
	// BZ1 Lua backwards compatability functions.
	int CanCommand(lua_State *L);
	int GetTransform(lua_State *L);
	int SetTransform(lua_State *L);

	// Lua script utils functions
	const luaL_Reg sLuaScriptUtils[] = {
		{ "GetHandle", GetHandle },
		{ "Make_RGB", Make_RGB },
		{ "Make_RGBA", Make_RGBA },
		/*
		{ "GetNextRandomVehicleODF", GetNextRandomVehicleODF },
		{ "SetWorld", SetWorld },
		{ "ProcessCommand", ProcessCommand },
		{ "SetRandomSeed", SetRandomSeed },
		*/
		// BZ2 Script Utils Inlines.
		{ "FailMission", FailMission },
		{ "SucceedMission", SucceedMission },
		{ "ChangeSide", ChangeSide },
		{ "AddScrap", AddScrap },
		{ "SetScrap", SetScrap },
		{ "GetScrap", GetScrap },
		{ "GetMaxScrap", GetMaxScrap },
		{ "GetTime", GetTime },
		{ "GetTug", GetTug },
		{ "HasCargo", HasCargo },
		{ "GetDistance", GetDistance },
		{ "GetNearestObject", GetNearestObject },
		{ "GetNearestVehicle", GetNearestVehicle },
		{ "GetNearestBuilding", GetNearestBuilding },
		{ "GetNearestEnemy", GetNearestEnemy },
		// BZ2 Script Utils Functions.
		{ "BuildObject", BuildObject },
		{ "RemoveObject", RemoveObject },
		{ "GetFirstEmptyGroup", GetFirstEmptyGroup },
		{ "SetGroup", SetGroup },
		{ "Attack", Attack },
		{ "Service", Service },
		{ "Goto", Goto },
		{ "Mine", Mine },
		{ "Follow", Follow },
		{ "Defend", Defend },
		{ "Defend2", Defend2 },
		{ "Stop", Stop },
		{ "Patrol", Patrol },
		{ "Retreat", Retreat },
		{ "GetIn", GetIn },
		{ "Pickup", Pickup },
		{ "Dropoff", Dropoff },
		{ "Build", Build },
		{ "LookAt", LookAt },
		{ "AllLookAt", AllLookAt },
		{ "IsOdf", IsOdf },
		{ "GetRace", GetRace },
		{ "GetPlayerHandle", GetPlayerHandle },
		{ "IsAlive", IsAlive },
		{ "IsFlying", IsFlying },
		{ "IsAliveAndPilot", IsAliveAndPilot },
		{ "IsAround", IsAround },
		{ "InBuilding", InBuilding },
		{ "AtTerminal", AtTerminal },
		{ "GetPosition", GetPosition },
		{ "GetPosition2", GetPosition2 },
		{ "GetFront", GetFront },
		{ "SetPosition", SetPosition },
		{ "Damage", Damage },
		{ "GetHealth", GetHealth },
		{ "GetCurHealth", GetCurHealth },
		{ "GetMaxHealth", GetMaxHealth },
		{ "SetCurHealth", SetCurHealth },
		{ "SetMaxHealth", SetMaxHealth },
		{ "AddHealth", AddHealth },
		{ "GetAmmo", GetAmmo },
		{ "GetCurAmmo", GetCurAmmo },
		{ "GetMaxAmmo", GetMaxAmmo },
		{ "SetCurAmmo", SetCurAmmo },
		{ "SetMaxAmmo", SetMaxAmmo },
		{ "AddAmmo", AddAmmo },
		{ "GetTeamNum", GetTeamNum },
		{ "SetTeamNum", SetTeamNum },
		{ "SetVelocity", SetVelocity },
		{ "SetControls", SetControls },
		{ "GetWhoShotMe", GetWhoShotMe },
		{ "GetLastEnemyShot", GetLastEnemyShot },
		{ "GetLastFriendShot", GetLastFriendShot },
		{ "DefaultAllies", DefaultAllies },
		{ "TeamplayAllies", TeamplayAllies },
		{ "Ally", Ally },
		{ "UnAlly", UnAlly },
		{ "IsTeamAllied", IsTeamAllied },
		{ "IsAlly", IsAlly },
		{ "AudioMessage", AudioMessage },
		{ "IsAudioMessageDone", IsAudioMessageDone },
		{ "StopAudioMessage", StopAudioMessage },
		{ "PreloadAudioMessage", PreloadAudioMessage },
		{ "PurgeAudioMessage", PurgeAudioMessage },
		{ "PreloadMusicMessage", PreloadMusicMessage },
		{ "PurgeMusicMessage", PurgeMusicMessage },
		{ "LoadJukeFile", LoadJukeFile },
		{ "SetMusicIntensity", SetMusicIntensity },
		//{ "FindAiPath", FindAiPath },
		//{ "FreeAiPath", FreeAiPath },
		{ "GetAiPaths", GetAiPaths },
		{ "SetPathType", SetPathType },
		{ "SetIndependence", SetIndependence },
		{ "IsInfo", IsInfo },
		{ "StartCockpitTimer", StartCockpitTimer },
		{ "StartCockpitTimerUp", StartCockpitTimerUp },
		{ "StopCockpitTimer", StopCockpitTimer },
		{ "HideCockpitTimer", HideCockpitTimer },
		{ "GetCockpitTimer", GetCockpitTimer },
		{ "StartEarthQuake", StartEarthQuake },
		{ "UpdateEarthQuake", UpdateEarthQuake },
		{ "StopEarthQuake", StopEarthQuake },
		{ "IsPerson", IsPerson },
		{ "GetCurWorld", GetCurWorld },
		{ "GetVarItemStr", GetVarItemStr },
		{ "GetVarItemInt", GetVarItemInt },
		{ "GetCVarItemInt", GetCVarItemInt },
		{ "GetCVarItemStr", GetCVarItemStr },
		{ "PreloadODF", PreloadODF },
		{ "TerrainFindFloor", TerrainFindFloor },
		{ "AddPilotByHandle", AddPilotByHandle },
		{ "PrintConsoleMessage", PrintConsoleMessage },
		{ "GetRandomFloat", GetRandomFloat },
		{ "IsDeployed", IsDeployed },
		{ "Deploy", Deploy },
		{ "IsSelected", IsSelected },
		{ "SetWeaponMask", SetWeaponMask },
		{ "GiveWeapon", GiveWeapon },
		{ "FireAt", FireAt },
		{ "IsFollowing", IsFollowing },
		{ "WhoFollowing", WhoFollowing },
		{ "SetUserTarget", SetUserTarget },
		{ "GetUserTarget", GetUserTarget },
		{ "SetPerceivedTeam", SetPerceivedTeam },
		{ "GetCurrentCommand", GetCurrentCommand },
		{ "GetCurrentWho", GetCurrentWho },
		{ "EjectPilot", EjectPilot },
		{ "HopOut", HopOut },
		{ "KillPilot", KillPilot },
		{ "RemovePilot", RemovePilot },
		{ "HoppedOutOf", HoppedOutOf },
		{ "GetCameraPosition", GetCameraPosition },
		{ "SetCameraPosition", SetCameraPosition },
		{ "ResetCameraPosition", ResetCameraPosition },
		{ "CalcCRC", CalcCRC },
		{ "IFace_Exec", IFace_Exec },
		{ "IFace_Activate", IFace_Activate },
		{ "IFace_Deactivate", IFace_Deactivate },
		{ "IFace_CreateCommand", IFace_CreateCommand },
		{ "IFace_CreateString", IFace_CreateString },
		{ "IFace_SetString", IFace_SetString },
		{ "IFace_GetString", IFace_GetString },
		{ "IFace_CreateInteger", IFace_CreateInteger },
		{ "IFace_SetInteger", IFace_SetInteger },
		{ "IFace_GetInteger", IFace_GetInteger },
		{ "IFace_SetIntegerRange", IFace_SetIntegerRange },
		{ "IFace_CreateFloat", IFace_CreateFloat },
		{ "IFace_SetFloat", IFace_SetFloat },
		{ "IFace_GetFloat", IFace_GetFloat },
		{ "IFace_ClearListBox", IFace_ClearListBox },
		{ "IFace_AddTextItem", IFace_AddTextItem },
		{ "IFace_GetSelectedItem", IFace_GetSelectedItem },
		{ "SetSkill", SetSkill },
		{ "SetAIP", SetAIP },
		{ "LogFloat", LogFloat },
		{ "GetInstantMyForce", GetInstantMyForce },
		{ "GetInstantCompForce", GetInstantCompForce },
		{ "GetInstantDifficulty", GetInstantDifficulty },
		{ "GetInstantGoal", GetInstantGoal },
		{ "GetInstantType", GetInstantType },
		{ "GetInstantFlag", GetInstantFlag },
		{ "GetInstantMySide", GetInstantMySide },
		//{ "StoppedPlayback", StoppedPlayback }, // No such function.
		{ "CameraReady", CameraReady },
		{ "CameraPath", CameraPath },
		{ "CameraPathDir", CameraPathDir },
		{ "PanDone", PanDone },
		{ "CameraObject", CameraObject },
		{ "CameraFinish", CameraFinish },
		{ "CameraCancelled", CameraCancelled },
		{ "FreeCamera", FreeCamera },
		{ "FreeFinish", FreeFinish },
		{ "PlayMovie", PlayMovie },
		{ "StopMovie", StopMovie },
		{ "PlayMove", PlayMove },
		{ "PlayRecording", PlayRecording },
		{ "PlaybackVehicle", PlaybackVehicle },
		{ "SetAnimation", SetAnimation },
		{ "GetAnimationFrame", GetAnimationFrame },
		{ "StartAnimation", StartAnimation },
		{ "MaskEmitter", MaskEmitter },
		{ "StartEmitter", StartEmitter },
		{ "StopEmitter", StopEmitter },
		//{ "SaveObjects", SaveObjects },
		//{ "LoadObjects", LoadObjects },
		//{ "IgnoreSync", IgnoreSync },
		//{ "IsRecording", IsRecording },
		{ "SetObjectiveOn", SetObjectiveOn },
		{ "SetObjectiveOff", SetObjectiveOff },
		{ "SetObjectiveName", SetObjectiveName },
		{ "ClearObjectives", ClearObjectives },
		{ "AddObjective", AddObjective },
		{ "IsWithin", IsWithin },
		{ "CountUnitsNearObject", CountUnitsNearObject },
		{ "SpawnBirds", SpawnBirds },
		{ "RemoveBirds", RemoveBirds },
		{ "SetColorFade", SetColorFade },
		{ "StopCheats", StopCheats },
		{ "CalcCliffs", CalcCliffs },
		{ "StartSoundEffect", StartSoundEffect },
		{ "FindSoundEffect", FindSoundEffect },
		{ "StopSoundEffect", StopSoundEffect },
		{ "GetObjectByTeamSlot", GetObjectByTeamSlot },
		{ "IsNetGame", IsNetworkOn },
		{ "IsHosting", ImServer },
		{ "IsNetworkOn", IsNetworkOn },
		{ "ImServer", ImServer },
		{ "ImDedicatedServer", ImDedicatedServer },
		{ "IsTeamplayOn", IsTeamplayOn },
		{ "CountPlayers", CountPlayers },
		{ "GetRaceOfTeam", GetRaceOfTeam },
		{ "IsPlayer", IsPlayer },
		{ "GetPlayerName", GetPlayerName },
		{ "WhichTeamGroup", WhichTeamGroup },
		{ "CountAllies", CountAllies },
		{ "GetCommanderTeam", GetCommanderTeam },
		{ "GetFirstAlliedTeam", GetFirstAlliedTeam },
		{ "GetLastAlliedTeam", GetLastAlliedTeam },
		{ "GetTeamplayRanges", GetTeamplayRanges },
		{ "SetRandomHeadingAngle", SetRandomHeadingAngle },
		{ "ClearTeamColors", ClearTeamColors },
		{ "DefaultTeamColors", DefaultTeamColors },
		{ "TeamplayTeamColors", TeamplayTeamColors },
		{ "SetTeamColor", SetTeamColor },
		{ "ClearTeamColor", ClearTeamColor },
		{ "MakeInert", MakeInert },
		{ "GetPositionNear", GetPositionNear },
		{ "GetPlayerODF", GetPlayerODF },
		{ "BuildEmptyCraftNear", BuildEmptyCraftNear },
		{ "GetCircularPos", GetCircularPos },
		{ "GetSafestSpawnpoint", GetSafestSpawnpoint },
		{ "GetSpawnpoint", GetSpawnpoint },
		{ "GetSpawnpointHandle", GetSpawnpointHandle },
		{ "GetRandomSpawnpoint", GetRandomSpawnpoint },
		{ "SetTimerBox", SetTimerBox },
		{ "AddToMessagesBox", AddToMessagesBox },
		{ "GetDeaths", GetDeaths },
		{ "GetKills", GetKills },
		{ "GetScore", GetScore },
		{ "SetDeaths", SetDeaths },
		{ "SetKills", SetKills },
		{ "SetScore", SetScore },
		{ "AddDeaths", AddDeaths },
		{ "AddKills", AddKills },
		{ "AddScore", AddScore },
		{ "SetAsUser", SetAsUser },
		{ "GetTarget", GetTarget },
		{ "IFace_ConsoleCmd", IFace_ConsoleCmd },
		{ "GetVelocity", GetVelocity },
		{ "GetCfg", GetObjInfo_CFG },
		{ "GetOdf", GetObjInfo_ODF },
		{ "GetBase", GetObjInfo_GOClass_gCfg, },
		{ "GetClassSig", GetObjInfo_EntityType },
		{ "GetClassLabel", GetObjInfo_GOClass },
		{ "GetWeaponConfig", Get_WeaponConfig },
		{ "GetWeaponODF", Get_WeaponODF },
		{ "GetWeaponGOClass", Get_WeaponGOClass },
		{ "DoesODFExist", DoesODFExist },
		//{ "IsAlive2", IsAlive },
		//{ "IsFlying2", IsFlying },
		//{ "IsAliveAndPilot2", IsAliveAndPilot },
		{ "TranslateString", TranslateString2 },
		{ "GetScavengerCurScrap", GetScavengerCurScrap },
		{ "GetScavengerMaxScrap", GetScavengerMaxScrap },
		{ "SetScavengerCurScrap", SetScavengerCurScrap },
		{ "SetScavengerMaxScrap", SetScavengerMaxScrap },
		//{ "DamageF", DamageF },
		{ "SelfDamage", SelfDamage },
		{ "WantBotKillMessages", WantBotKillMessages },
		{ "EnableHighTPS", EnableHighTPS },
		{ "GetLocalUserInspectHandle", GetLocalUserInspectHandle },
		{ "GetLocalUserSelectHandle", GetLocalUserSelectHandle },
		{ "ResetTeamSlot", ResetTeamSlot },
		{ "GetCategoryTypeOverride", GetCategoryTypeOverride },
		{ "GetCategoryType", GetCategoryType },
		{ "GetODFHexInt", GetODFBool },
		{ "GetODFInt", GetODFInt },
		{ "GetODFLong", GetODFLong },
		{ "GetODFFloat", GetODFFloat },
		{ "GetODFDouble", GetODFDouble },
		{ "GetODFBool", GetODFBool },
		{ "GetODFString", GetODFString },
		{ "GetODFColor", GetODFColor },
		{ "GetODFVector", GetODFVector },
		//{ "OpenODF", OpenODF },
		//{ "CloseODF", CloseODF },
		{ "NoteGameoverWithCustomMessage", NoteGameoverWithCustomMessage },
		{ "SetBestGroup", SetBestGroup },
		{ "GetGroup", GetGroup },
		{ "GetGroupCount", GetGroupCount },
		{ "SetLifespan", SetLifespan },
		{ "DoesFileExist", DoesFileExist },
		{ "LoadFile", LoadFile },
		{ "StartAudio3D", StartAudio3D },
		{ "StartAudio2D", StartAudio2D },
		{ "IsAudioPlaying", IsAudioPlaying },
		{ "StopAudio", StopAudio },
		{ "PauseAudio", PauseAudio },
		{ "ResumeAudio", ResumeAudio },
		{ "SetVolume", SetVolume },
		{ "SetPan", SetPan },
		{ "SetRate", SetRate },
		{ "GetAudioFileDuration", GetAudioFileDuration },
		{ "IsPlayingLooped", IsPlayingLooped },
		{ "GetNearestPowerup", GetNearestPowerup },
		{ "GetNearestPerson", GetNearestPerson },
		{ "SetCommand", SetCommand },
		{ "SetGravity", SetGravity },
		{ "SetAutoGroupUnits", SetAutoGroupUnits },
		{ "KickPlayer", KickPlayer },
		{ "TerrainIsWater", TerrainIsWater },
		{ "GetTerrainHeightAndNormal", GetTerrainHeightAndNormal },
		{ "WriteToFile", WriteToFile }, //{ "GetOutputPath", GetOutputPath },
		{ "GetPathPoints", GetPathPoints },
		{ "GetOwner", GetOwner },
		{ "SetTarget", SetTarget },
		{ "SetOwner", SetOwner },
		{ "SetPilotClass", SetPilotClass },
		{ "AllowRandomTracks", AllowRandomTracks },
		{ "SetFFATeamColors", SetFFATeamColors },
		{ "SetTeamStratColors", SetTeamStratColors },
		{ "GetFFATeamColor", GetFFATeamColor },
		{ "GetFFATeamColorVector", GetFFATeamColorVector },
		{ "GetTeamStratColor", GetTeamStratColor },
		{ "GetTeamStratColorVector", GetTeamStratColorVector },
		{ "SwapTeamStratColors", SwapTeamStratColors },
		{ "GetTeamColorsAreFFA", GetTeamColorsAreFFA },
		{ "SetTeamColors", SetTeamColors },
		{ "AddPower", AddPower },
		{ "SetPower", SetPower },
		{ "GetPower", GetPower },
		{ "GetMaxPower", GetMaxPower },
		{ "AddMaxPower", AddMaxPower },
		{ "AddMaxScrap", AddMaxScrap },
		{ "SetMaxScrap", SetMaxScrap },
		{ "SetMaxPower", SetMaxPower },
		{ "GetTeamStratIndividualColor", GetTeamStratIndividualColor },
		{ "GetTeamStratIndividualColorVector", GetTeamStratIndividualColorVector },
		{ "GetMapTRNFilename", GetMapTRNFilename },
		{ "IsNotDeadAndPilot2", IsNotDeadAndPilot2 },
		{ "GetLabel", GetLabel },
		{ "SetLabel", SetLabel },
		{ "GetTap", GetTap },
		{ "SetTap", SetTap },
		{ "GetCurLocalAmmo", GetCurLocalAmmo },
		{ "AddLocalAmmo", AddLocalAmmo },
		{ "GetMaxLocalAmmo", GetMaxLocalAmmo },
		{ "SetCurLocalAmmo", SetCurLocalAmmo },
		{ "GetNetworkListItem", GetNetworkListItem },
		{ "GetNetworkListCount", GetNetworkListCount },
		{ "GetTeamRelationship", GetTeamRelationship },
		// 1.3.6.4+
		{ "HasPilot", HasPilot },
		{ "GetPilotClass", GetPilotClass },
		{ "GetBaseScrapCost", GetBaseScrapCost },
		{ "GetActualScrapCost", GetActualScrapCost },
		{ "PetWatchdogThread", PetWatchdogThread },
		{ "GetPerceivedTeam", GetPerceivedTeam },
		{ "SetLastCurrentPosition", SetLastCurrentPosition },
		{ "GetRemainingLifespan", GetRemainingLifespan },
		{ "GetAllSpawnpoints", GetAllSpawnpoints },
		// 1.3.7.0+
		{ "GetPlan", GetPlan },
		{ "GetIndependence", GetIndependence },
		{ "GetSkill", GetSkill },
		{ "GetWeaponMask", GetWeaponMask },
		{ "SetInterpolablePosition", SetInterpolablePosition },
		{ "SecondsToTurns", SecondsToTurns },
		{ "TurnsToSeconds", TurnsToSeconds },
		{ "GetLifeSpan", GetLifeSpan },
		// 1.3.7.2+
		{ "GetCurrentCommandWhere", GetCurrentCommandWhere },
		{ "GetAllGameObjectHandles", GetAllGameObjectHandles },
		{ "GetOmega", GetOmega },
		{ "SetOmega", SetOmega },
		{ "IsCraftButNotPerson", IsCraftButNotPerson },
		{ "IsCraftOrPerson", IsCraftOrPerson },
		{ "IsBuilding", IsBuilding },
		{ "IsPowerup", IsPowerup },
		{ "SetCanSnipe", SetCanSnipe },
		{ "GetCanSnipe", GetCanSnipe },
		{ "WhoIsTargeting", WhoIsTargeting },

		// BZ2 DLL Utility Functions
		//{ "DoTaunt", DoTaunt },
		//{ "SetTauntCPUTeamName", SetTauntCPUTeamName },

		// BZScriptor Backwards Compatability.
		{ "SetAngle", SetAngle },
		{ "CameraPos", CameraPos },
		{ "CameraOf", CameraOf },
		{ "Move", Move },
		{ "ReplaceObject", ReplaceObject },

		//BZ1 Functions/Backwards Compatability.
		// Name Overloads.
		{ "IsValid", IsAround },
		{ "GetAIP", GetPlan },
		{ "GetFloorHeightAndNormal", TerrainFindFloor },
		{ "GetWeaponClass", Get_WeaponConfig },
		//{ "GetClassId", GetObjInfo_EntityType }, //GetClassId }, // BZ1 version returns an int, bz2 returns string, just use Get_EntityType.
//		{ "BuildAt", BuildAt }, // Needs a struct/handle/position saveoff for 1 turn.
		{ "CanCommand", CanCommand },
	//	{ "GetRidOfSomeScrap", GetRidOfSomeScrap },
	//	{ "ClearScrapAround", ClearScrapAround },
	//	{ "ObjectsInRange", ObjectsInRange },
		// Remade functions.
		{ "SetPathOneWay", SetPathOneWay },
		{ "SetPathRoundTrip", SetPathRoundTrip },
		{ "SetPathLoop", SetPathLoop },
		{ "SetTransform", SetTransform },
		{ "GetTransform", GetTransform },

		// Math stuffs.
		{ "SetVector", SetVector },
		{ "DotProduct", DotProduct },
		{ "CrossProduct", CrossProduct },
		{ "Normalize", Normalize },
		{ "Length", Length },
		{ "LengthSquared", LengthSquared },
		{ "Distance2D", Distance2D },
		{ "Distance2DSquared", Distance2DSquared },
		{ "Distance3D", Distance3D },
		{ "Distance3DSquared", Distance3DSquared },
		{ "SetMatrix", SetMatrix },
		{ "BuildAxisRotationMatrix", BuildAxisRotationMatrix },
		{ "BuildPositionRotationMatrix", BuildPositionRotationMatrix },
		{ "BuildOrthogonalMatrix", BuildOrthogonalMatrix },
		{ "BuildDirectionalMatrix", BuildDirectionalMatrix },
		{ "Build_Yaw_Matrix", Build_Yaw_Matrix },
		{ "Interpolate_Matrix", Interpolate_Matrix },
		{ "SetQuaternion", SetQuaternion },
		{ "Interpolate_Quaternion", Interpolate_Quaternion },
		{ "Normalize_Quaternion", Normalize_Quaternion },
		{ "Matrix_to_QuatPos", Matrix_to_QuatPos },
		{ "Matrix_to_Quaternion", Matrix_to_Quaternion },
		{ "QuatPos_to_Matrix", QuatPos_to_Matrix },
		{ "Vector_Transform", Vector_Transform },
		{ "Vector_TransformInv", Vector_TransformInv },
		{ "Vector_Rotate", Vector_Rotate },
		{ "Vector_RotateInv", Vector_RotateInv },
		{ "Matrix_Inverse", Matrix_Inverse },
		{ "Add_Mult_Vectors", Add_Mult_Vectors },
		// Utility Math Functions.
		{ "IsNullVector", IsNullVector },
		{ "FrontToRadian", FrontToRadian },
		{ "FrontToDegrees", FrontToDegrees },
		{ "GetFacingDirection", GetFacingDirection },
		{ "GetFacingDrection2D", GetFacingDrection2D },

		{ NULL, NULL }
	};




	// read a lua value from the file
	bool LoadValue(lua_State *L, bool push);

	// save a Lua value to the file
	bool SaveValue(lua_State *L, int i);
}