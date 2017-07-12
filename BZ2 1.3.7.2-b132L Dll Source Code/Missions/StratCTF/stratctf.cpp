#include "..\Shared\DLLBase.h"
#include "stratctf.h"
#include "..\Shared\StartingVehicles.h"
#include "..\Shared\TRNAllies.h"
#include "..\Shared\DLLUtils.h"

#include <math.h>
#include <string.h>
#include <malloc.h>

#define VEHICLE_SPACING_DISTANCE (20.0f)

// Set this to 0 to have vulnerable recyclers once again
#define VULNERABLE_RECY 1

// How close the goal needs to be to the recycler to win
const float kfGOAL_SCORE_DIST = 60.0f;

// How far the goal can be moved before users get notified
const float kfGOAL_MOVE_DIST = 10.0f;

// ---------- Scoring Values-- these are delta scores, added to current score --------
const int ScoreForKillingCraft = 5; // User-piloted craft only
const int ScoreForKillingPerson = 10;
const int ScoreForDyingAsCraft = -2;
const int ScoreForDyingAsPerson = -5;
const int ScoreForWinning = 10000;

// How long a "spawn" kill lasts, in tenth of second ticks. If the
// time since they were spawned to current is less than this, it's a
// spawn kill, and not counted. Todo - Make this an ivar for dm?
const int MaxSpawnKillTime=15;

// -----------------------------------------------

// Max distance (in x,z dimensions) on a pilot respawn. Actual value
// used will vary with random numbers, and will be +/- this value
const float RespawnDistanceAwayXZRange = 32.f;
// Max distance (in y dimensions) on a pilot respawn. Actual value
// used will vary with random numbers, and will be [0..this value)
const float RespawnDistanceAwayYRange = 8.f;

// How high up to respawn a pilot
const float RespawnPilotHeight=200.0f;

// How far allies will be from the commander's position
const float AllyMinRadiusAway=30.0f;
const float AllyMaxRadiusAway=60.0f;

// How long (in seconds) from noticing gameover, to the actual kicking
// out back to the shell.
const float endDelta = 10.0f;

// Tuning distances for GetSpawnpointForTeam()

// Friendly spawnpoint: Max distance away for ally
const float FRIENDLY_SPAWNPOINT_MAX_ALLY = 100.f;
// Friendly spawnpoint: Min distance away for enemy
const float FRIENDLY_SPAWNPOINT_MIN_ENEMY = 400.f;

// Random spawnpoint: min distance away for enemy
const float RANDOM_SPAWNPOINT_MIN_ENEMY = 450.f;


// Temporary strings for blasting stuff into for output. NOT to be
// used to store anything you care about.
static char TempMsgString[1024];

// Temporary name for blasting ODF names into while building
// them. *not* saved, do *not* count on its contents being valid
// next time the dll is called.
static char TempODFName[64];


// Don't do much in constructor; do in InitialSetup()
StratCTF::StratCTF(void)
{
	EnableHighTPS(m_GameTPS);
	AllowRandomTracks(true); // If the user wants random music, we're fine with that.

	b_count = &b_last - &b_first - 1;
	b_array = &b_first + 1;

	f_count = &f_last - &f_first - 1;
	f_array = &f_first +1;

	h_array = &h_first + 1;
	h_count = &h_last - &h_first - 1;

	i_count = &i_last - &i_first - 1;
	i_array = &i_first + 1;
	TRNAllies::SetupTRNAllies(GetMapTRNFilename());
}

char *StratCTF::GetNextRandomVehicleODF(int Team)
{
	return GetPlayerODF(Team);
}

void StratCTF::AddObject(Handle h)
{
#ifndef EDITOR
	char ODFName[64];
	GetObjInfo(h, Get_CFG, ODFName);

#if 0//def _DEBUG
	// So we can test gameover logic better
	SetCurHealth(h, 1);
	SetMaxHealth(h, 1);
#endif

	// Changed NM 10/19/01 - all AI is at skill 3 now by default.
	// Changed NM 4/13/02 - except when starting things up. Then we drop a notch.


	int UseTurretSkill=m_TurretAISkill;
	int UseNonTurretSkill=m_NonTurretAISkill;
#if 0
	if(m_CreatingStartingVehicles)
	{
		if(UseTurretSkill >0)
			UseTurretSkill--;  // bottoms out at zero.
		if(UseNonTurretSkill >0)
			UseNonTurretSkill--;  // bottoms out at zero.
	}
#endif

	// Note: this is the proper way to determine if a handle points to a
	// turret. NM 1/29/05
	char ObjClass[64];
	GetObjInfo(h, Get_GOClass, ObjClass);
	bool IsTurret = (_stricmp(ObjClass, "CLASS_TURRETTANK") == 0);

	// See if this is a turret, 
	if(IsTurret)
	{
		SetSkill(h, UseTurretSkill);
		return;
	} else
	{
		// Not a turret. Use regular skill level
		SetSkill(h, UseNonTurretSkill);
	}

	// Also see if this is a new recycler vehicle (e.g. user upgraded
	// recycler building to vehicle)
	bool IsRecyVehicle = (_stricmp(ObjClass, "CLASS_RECYCLERVEHICLE") == 0);
	if(IsRecyVehicle)
	{
		int Team = GetTeamNum(h);
		// If we're not tracking a recycler vehicle for this team right
		// now, store it.
		if(m_RecyclerHandles[Team] == 0)
			m_RecyclerHandles[Team] = h;
	}
#endif
}

// Gets the initial player vehicle as selected in the shell
char *StratCTF::GetInitialPlayerVehicleODF(int Team)
{
	return GetPlayerODF(Team);
}

// Given a race identifier, get the pilot back (used during a respawn)
char *StratCTF::GetInitialPlayerPilotODF(char Race)
{
	if(m_RespawnWithSniper)
		strcpy_s(TempODFName, "ispilo"); // With sniper.
	else
		strcpy_s(TempODFName, "isuser_m"); // Note-- this is the sniper-less variant for a respawn
	TempODFName[0]=Race;
	return TempODFName;
}


// Given a race identifier, get the recycler ODF back
char *StratCTF::GetInitialRecyclerODF(char Race)
{
	const char* pContents = DLLUtils::GetCheckedNetworkSvar(5, NETLIST_Recyclers);
	if((pContents != NULL) && (pContents[0] != '\0'))
	{
		strcpy_s(TempODFName, pContents);
	}
	else
	{
		strcpy_s(TempODFName, "ivrecy_m");
	}
	TempODFName[0]=Race;
	return TempODFName;
}


// Given a race identifier, get the scavenger ODF back
char *StratCTF::GetInitialScavengerODF(char Race)
{
	strcpy_s(TempODFName, "ivscav");
	TempODFName[0]=Race;
	return TempODFName;
}

// Given a race identifier, get the constructor ODF back
char *StratCTF::GetInitialConstructorODF(char Race)
{
	strcpy_s(TempODFName, "ivcons");
	TempODFName[0]=Race;
	return TempODFName;
}

// Given a race identifier, get the healer (service truck) ODF back
char *StratCTF::GetInitialHealerODF(char Race)
{
	strcpy_s(TempODFName, "ivserv");
	TempODFName[0]=Race;
	return TempODFName;
}


// Helper function for SetupTeam(), returns an appropriate spawnpoint.
Vector StratCTF::GetSpawnpointForTeam(int Team)
{
	Vector spawnpointPosition;
	spawnpointPosition.x = spawnpointPosition.y = spawnpointPosition.z = 0.f;

	// Pick a random, ideally safe spawnpoint.
	SpawnpointInfo* pSpawnPointInfo;
	size_t i,count = GetAllSpawnpoints(pSpawnPointInfo, Team);

	// Designer didn't seem to put any spawnpoints on the map :(
	if(count == 0)
	{
		return spawnpointPosition;
	}

	// First pass: see if a spawnpoint exists with this team #
	//
	// Note: using a temporary array allocated on stack to keep track
	// of indices.
	size_t* pIndices = reinterpret_cast<size_t*>(_alloca(count * sizeof(size_t)));
	memset(pIndices, 0, count * sizeof(size_t));
	size_t indexCount = 0;
	for(i=0; i<count; ++i)
	{
		if(pSpawnPointInfo[i].m_Team == Team)
		{
			pIndices[indexCount++] = i;
		}
	}

	// Did we find any spawnpoints in the above search? If so,
	// randomize out of that list and return that
	if(indexCount > 0)
	{
		size_t index = 0;
		// Might be unnecessary, but make sure we return a valid index
		// in [0,indexCount)
		do
		{
			index = static_cast<size_t>(GetRandomFloat(static_cast<float>(indexCount)));
		} while(index >= indexCount);
		return pSpawnPointInfo[pIndices[index]].m_Position;
	}

	// Second pass: build up a list of spawnpoints that appear to have
	// allies close, randomly pick one of those.
	indexCount = 0;
	for(i=0; i<count; ++i)
	{
		if(((pSpawnPointInfo[i].m_DistanceToClosestSameTeam < FRIENDLY_SPAWNPOINT_MAX_ALLY) ||
			(pSpawnPointInfo[i].m_DistanceToClosestAlly < FRIENDLY_SPAWNPOINT_MAX_ALLY)) &&
		   (pSpawnPointInfo[i].m_DistanceToClosestEnemy >= FRIENDLY_SPAWNPOINT_MIN_ENEMY))
		{
			pIndices[indexCount++] = i;
		}
	}

	// Did we find any spawnpoints in the above search? If so,
	// randomize out of that list and return that
	if(indexCount > 0)
	{
		size_t index = 0;
		// Might be unnecessary, but make sure we return a valid index
		// in [0,indexCount)
		do
		{
			index = static_cast<size_t>(GetRandomFloat(static_cast<float>(indexCount)));
		} while(index >= indexCount);
		return pSpawnPointInfo[pIndices[index]].m_Position;
	}

	// Third pass: Make up a list of spawnpoints that appear to have
	// no enemies close.
	indexCount = 0;
	for(i=0; i<count; ++i)
	{
		if(pSpawnPointInfo[i].m_DistanceToClosestEnemy >= RANDOM_SPAWNPOINT_MIN_ENEMY)
		{
			pIndices[indexCount++] = i;
		}
	}

	// Did we find any spawnpoints in the above search? If so,
	// randomize out of that list and return that
	if(indexCount > 0)
	{
		size_t index = 0;
		// Might be unnecessary, but make sure we return a valid index
		// in [0,indexCount)
		do
		{
			index = static_cast<size_t>(GetRandomFloat(static_cast<float>(indexCount)));
		} while(index >= indexCount);
		return pSpawnPointInfo[pIndices[index]].m_Position;
	}

	// If here, all spawnpoints have an enemy within
	// RANDOM_SPAWNPOINT_MIN_ENEMY.  Fallback to old code.
	return GetRandomSpawnpoint(Team);
}


// Sets up the side's commander's extra vehicles, such a recycler or
// more.  Does *not* create the player vehicle for them, 
// however. [That's to be done in SetupPlayer.]  Safe to be called
// multiple times for each player on that team
//
// If Teamplay is off, this function is called once per player.
//
// If Teamplay is on, this function is called only on the
// _defensive_ team number for an alliance. 
void StratCTF::SetupTeam(int Team)
{
#ifndef EDITOR
	if((Team<1) || (Team>=MAX_TEAMS))
		return;
	if(m_TeamIsSetUp[Team])
		return;

	char TeamRace=GetRaceOfTeam(Team);
	if(IsTeamplayOn())
		SetMPTeamRace(WhichTeamGroup(Team), TeamRace); // Lock this down to prevent changes.

	Vector spawnpointPosition = GetSpawnpointForTeam(Team);

	// Store position we created them at for later
	m_TeamPos[3*Team+0] = spawnpointPosition.x;
	m_TeamPos[3*Team+1] = spawnpointPosition.y;
	m_TeamPos[3*Team+2] = spawnpointPosition.z;

	// Build recycler some distance away
	spawnpointPosition = GetPositionNear(spawnpointPosition, VEHICLE_SPACING_DISTANCE, 2*VEHICLE_SPACING_DISTANCE);

	int VehicleH;
	VehicleH = BuildObject(GetInitialRecyclerODF(TeamRace), Team, spawnpointPosition);
	SetRandomHeadingAngle(VehicleH);
	m_RecyclerHandles[Team] = VehicleH;
	SetGroup(VehicleH, 0);

	// Build all optional vehicles for this team.
	spawnpointPosition.x = m_TeamPos[3*Team+0]; // restore default after we modified this for recy above
	spawnpointPosition.y = m_TeamPos[3*Team+1];
	spawnpointPosition.z = m_TeamPos[3*Team+2];

	// Drop skill level while we build things.
	m_CreatingStartingVehicles=true;
	StartingVehicleManager::CreateVehicles(Team, TeamRace, m_StartingVehiclesMask, spawnpointPosition);
	m_CreatingStartingVehicles=false;

	SetScrap(Team, 40);

	if(IsTeamplayOn()) 
	{
		for(int i=GetFirstAlliedTeam(Team); i <= GetLastAlliedTeam(Team); i++) 
		{
			if(i!=Team)
			{
				// Get a new position near the team's central position
				const Vector pos = GetPositionNear(spawnpointPosition, AllyMinRadiusAway, AllyMaxRadiusAway);

				// In teamplay, store where offense players were created for respawns later
				m_TeamPos[3*i+0] = pos.x;
				m_TeamPos[3*i+1] = pos.y;
				m_TeamPos[3*i+2] = pos.z;
			} // Loop over allies not the commander
		}
		m_TeamIsSetUp[Team]=true;
	}

#if 0//def _DEBUG
	// Test out spawn point safety
	{
		SpawnpointInfo* pSpawnPointInfo;
		size_t i,count = GetAllSpawnpoints(pSpawnPointInfo, 7 - Team);
	}
#endif

	if(Team == GetLocalPlayerTeamNumber())
	{
		ClearObjectives();
		AddObjective("stratctf.otf", WHITE, -1.0f);
		AddToMessagesBox2("Pick up the goal, return it to your recycler to win.", RGB(255, 0, 255));
	}
#endif // #ifndef EDITOR
}

// Given an index into the Player list, build everything for a given
// single player (i.e. a vehicle of some sorts), and set up the team
// as well as necessary
Handle StratCTF::SetupPlayer(int Team)
{
#ifndef EDITOR
	Handle PlayerH = 0;
	Vector spawnpointPosition;
	memset(&spawnpointPosition, 0, sizeof(spawnpointPosition));

	if((Team<0) || (Team>=MAX_TEAMS))
		return 0; // Sanity check... do NOT proceed

	m_SpawnedAtTime[Team]=m_ElapsedGameTime; // Note when they spawned in.

	int TeamBlock=WhichTeamGroup(Team);

	if((!IsTeamplayOn()) || (TeamBlock<0))
	{
		// This player is their own commander; set up their equipment.
		SetupTeam(Team);

		// Now put player near his recycler
		spawnpointPosition.x = m_TeamPos[3*Team+0];
		spawnpointPosition.z = m_TeamPos[3*Team+2];
		spawnpointPosition.y = TerrainFindFloor(spawnpointPosition.x, spawnpointPosition.z) + 2.5f;
	}
	else
	{
		// Teamplay. Gotta put them near their defensive player. Also, 
		// always ensure the recycler/etc has been set up for this
		// team if we know who they are
		SetupTeam(GetCommanderTeam(Team));

		// SetupTeam will fill in the m_TeamPos[] array of positions
		// for both the commander and offense players, so read out the
		// results
		spawnpointPosition.x = m_TeamPos[3*Team+0];
		spawnpointPosition.z = m_TeamPos[3*Team+2];
		spawnpointPosition.y = TerrainFindFloor(spawnpointPosition.x, spawnpointPosition.z) + 2.5f;
	} // Teamplay setup

	PlayerH = BuildObject(GetPlayerODF(Team), Team, spawnpointPosition);
	SetRandomHeadingAngle(PlayerH);

	// If on team 0 (dedicated server team), make this object gone from the world
	if(!Team)
		MakeInert(PlayerH);

	return PlayerH;
#else
	return 0;
#endif // #ifndef EDITOR
}

// Internal one-time setup
void StratCTF::Init(void)
{
	Handle PlayerH = 0;
	Handle PlayerEntryH = 0;

	// Ensure variables pre-init'd to zero at the start

	if(i_array)
		memset(i_array, 0, i_count*sizeof(int));
	if(f_array)
		memset(f_array, 0, f_count*sizeof(float));
	if(h_array)
		memset(h_array, 0, h_count*sizeof(Handle));
	if(b_array)
		memset(b_array, 0, b_count*sizeof(bool));
#ifndef EDITOR

	StartingVehicleManager::Init();

	// Read from the map's .trn file whether we respawn at altitude or
	// not.
	{
		const char* pMapName = GetVarItemStr("network.session.svar0");
		char mapTrnFile[128];
		strcpy_s(mapTrnFile, pMapName);
		char* pDot = strrchr(mapTrnFile, '.');
		if(pDot)
		{
			*pDot = 0;
			strcat_s(mapTrnFile, ".trn");
			OpenODF(mapTrnFile);
			GetODFBool(mapTrnFile, "DLL", "RespawnAtLowAltitude", &m_RespawnAtLowAltitude, false);
			CloseODF(mapTrnFile);
		}
	}

	// Set up some variables based on how things appear in the world
	m_DidInit=true;
	m_KillLimit = GetVarItemInt("network.session.ivar0");
	m_TotalGameTime = GetVarItemInt("network.session.ivar1");
	// Skip ivar2-- player limit. Assume the netmgr takes care of that.
	// ivar4 (vehicle prefs 1) read elsewhere.
	m_StartingVehiclesMask = GetVarItemInt("network.session.ivar7");

	m_bIsFriendlyFireOn = (GetVarItemInt("network.session.ivar32") != 0);

	// Override the bitfields set in the shell, so that you start with
	// 2 turrets, or nothing.
	if(m_StartingVehiclesMask)
		m_StartingVehiclesMask = 3; 

	m_PointsForAIKill=(GetVarItemInt("network.session.ivar14") != 0);
	m_KillForAIKill=(GetVarItemInt("network.session.ivar15") != 0);
	m_RespawnWithSniper=(GetVarItemInt("network.session.ivar16") != 0);

	m_TurretAISkill = GetVarItemInt("network.session.ivar17");
	if(m_TurretAISkill<0)
		m_TurretAISkill=0;
	else if(m_TurretAISkill>3)
		m_TurretAISkill=3;
	m_NonTurretAISkill = GetVarItemInt("network.session.ivar18");
	if(m_NonTurretAISkill<0)
		m_NonTurretAISkill=0;
	else if(m_NonTurretAISkill>3)
		m_NonTurretAISkill=3;

	if(IsTeamplayOn())
	{
		m_Alliances = 0; // clear this 
	}
	else
	{
		m_Alliances = GetVarItemInt("network.session.ivar23");
		switch (m_Alliances)
		{
		case 1: // 1+2 vs 3+4
			Ally(1, 2);
			Ally(2, 1);
			Ally(3, 4);
			Ally(4, 3);
			break;

		case 2: // 1+3 vs 2+4
			Ally(1, 3);
			Ally(3, 1);
			Ally(2, 4);
			Ally(4, 2);
			break;

		case 3: // 1+4 vs 2+3
			Ally(1, 4);
			Ally(4, 1);
			Ally(2, 3);
			Ally(3, 2);
			break;

		case 0:
		default:
			m_Alliances = 0; // clear this in case it wasn't in 0..3
		}
	} // Teamplay is off

#if VULNERABLE_RECY
	m_RecyInvulnerabilityTime = GetVarItemInt("network.session.ivar25");
	m_RecyInvulnerabilityTime *= 60 * m_GameTPS; // convert from minutes to ticks
#endif

	// The BZN has a player in the world. We need to delete them, as
	// this code (either on this machine or remote machines) handles
	// creation of the proper vehicles in the right places for
	// everyone.
	PlayerEntryH=GetPlayerHandle();
	if(PlayerEntryH) 
		RemoveObject(PlayerEntryH);

	// Do all the one-time server side init of varbs. These varbs are
	// saved out and read in on clientside, if saved in the proper
	// place above. This needs to be done after toasting the initial
	// vehicle
	if((ImServer()) || (!IsNetworkOn()))
	{
		m_ElapsedGameTime=0;
		if(!m_RemainingGameTime)
			m_RemainingGameTime = m_TotalGameTime * 60 * m_GameTPS; // convert minutes to 1/10 seconds
	}

	// And build the local player for the server
	int LocalTeamNum=GetLocalPlayerTeamNumber(); // Query this from game
	PlayerH=SetupPlayer(LocalTeamNum);
	SetAsUser(PlayerH, LocalTeamNum);
	AddPilotByHandle(PlayerH);

	{
		const char* pContents = DLLUtils::GetCheckedNetworkSvar(11, NETLIST_STCTFGoals);
		if(pContents && pContents[0])
			m_GoalHandle = BuildObject((char *)pContents, 0, "stctf_goal"); // neutral team
		else
			m_GoalHandle = BuildObject("stctf_goal", 0, "stctf_goal"); // Fallback to default ODF neutral team
	}

	if(!m_GoalHandle)
	{
		AddToMessagesBox2("WARNING! Goal configuration is invalid. Game will be over very soon!", RGB(255, 0, 255));
	}

	// Store position so that we don't get false-positive goal moved
	// messages.
	Vector GoalVector = GetPosition(m_GoalHandle);
	m_GoalPos[0] = GoalVector.x;
	m_GoalPos[1] = GoalVector.y;
	m_GoalPos[2] = GoalVector.z;

#endif // #ifndef EDITOR
}

// Called from Execute, 1/10 of a second has elapsed. Update everything.
void StratCTF::UpdateGameTime(void)
{
#ifndef EDITOR
	m_ElapsedGameTime++;

	// Are we in a time limited game?
	if(m_RemainingGameTime>0)
	{
		m_RemainingGameTime--;
		if(!(m_RemainingGameTime % m_GameTPS))
		{
			// Convert tenth-of-second ticks to hour/minutes/seconds format.
			int Seconds = m_RemainingGameTime / m_GameTPS;
			int Minutes = Seconds / 60;
			int Hours = Minutes / 60;
			// Lop seconds and minutes down to 0..59 once we've grabbed
			// non-remainder out.
			Seconds %= 60;
			Minutes %= 60;

			if(Hours)
				sprintf_s(TempMsgString, "Time Left %d:%02d:%02d\n", Hours, Minutes, Seconds);
			else
				sprintf_s(TempMsgString, "Time Left %d:%02d\n", Minutes, Seconds);
			SetTimerBox(TempMsgString);

			// Also print this out more visibly at important times....
			if(Hours == 0)
			{
				// Every 5 minutes down to 10 minutes, then every minute
				// thereafter.
				if((Seconds == 0) && ((Minutes <= 10) || (!(Minutes % 5))))
					AddToMessagesBox(TempMsgString);
				else
				{
					// Every 5 seconds when under a minute is remaining.
					if((Minutes == 0) && (!(Seconds % 5)))
						AddToMessagesBox(TempMsgString);
				}
			}
		}

		// Game over due to timeout?
		if(!m_RemainingGameTime)
		{
			NoteGameoverByTimelimit();
			DoGameover(10.0f);
		}

	}
	else { 
		// Infinite time game. Periodically update ingame clock.
		if(!(m_ElapsedGameTime % m_GameTPS))
		{

			// Convert tenth-of-second ticks to hour/minutes/seconds format.
			int Seconds = m_ElapsedGameTime / m_GameTPS;
			int Minutes = Seconds / 60;
			int Hours = Minutes / 60;
			// Lop seconds and minutes down to 0..59 once we've grabbed
			// non-remainder out.
			Seconds %= 60;
			Minutes %= 60;

			if(Hours)
				sprintf_s(TempMsgString, "Mission Time %d:%02d:%02d\n", Hours, Minutes, Seconds);
			else
				sprintf_s(TempMsgString, "Mission Time %d:%02d\n", Minutes, Seconds);
			SetTimerBox(TempMsgString);
		}
	}
#endif // #ifndef EDITOR
}

// Tests if an allied team (A1, A2) has won over the other allied
// team (B1, B2) Sets gameover if true.
void StratCTF::TestAlliedGameover(int A1, int A2, int B1, int B2, bool *TeamIsFunctioning)
{
#ifndef EDITOR
	// No need to do anything more...
	if(m_GameOver)
		return;

	// Either A1 or A2 is still around, either B1 or B2 was
	// set up, and both enemies are gone = win
	if(((TeamIsFunctioning[A1]) || (TeamIsFunctioning[A2])) &&
		((m_TeamIsSetUp[B1]) || (m_TeamIsSetUp[B2])) && 
		((!TeamIsFunctioning[B1]) && (!TeamIsFunctioning[B2])))
	{
		// Give points
		AddScore(GetPlayerHandle(A1), ScoreForWinning);
		AddScore(GetPlayerHandle(A2), ScoreForWinning);
		// We can only report 1 winner, default to team A1 if they're
		// still around
		if(TeamIsFunctioning[A1])
			NoteGameoverByLastWithBase(GetPlayerHandle(A1));
		else
			NoteGameoverByLastWithBase(GetPlayerHandle(A2));

		DoGameover(endDelta);
		m_GameOver=true;
	}
#endif // #ifndef EDITOR
}

// If Recycler invulnerability is on, then does the job of it.
void StratCTF::ExecuteRecyInvulnerability(void)
{
#if VULNERABLE_RECY
	if((m_GameOver) || (m_RecyInvulnerabilityTime == 0))
		return;
#else
	if((m_GameOver)) // || (m_RecyInvulnerabilityTime == 0))
		return;
#endif

	int i;
	Handle recyHandle = 0; // for this team, either vehicle or building
	for(i=0;i<MAX_TEAMS;i++)
	{
		if(m_TeamIsSetUp[i])
		{

			// Check if recycler vehicle still exists. Side effect to
			// note: IsAliveAndPilot zeroes the handle if pilot missing;
			// that'd be bad for us here if we want to manually remove
			// it. Thus, we have a sacrificial copy of it the game can
			// obliterate w/o hurting anything.
			Handle TempH = m_RecyclerHandles[i];
			if((!IsAlive(TempH)) || (TempH==0))
				recyHandle = GetObjectByTeamSlot(i, DLL_TEAM_SLOT_RECYCLER);
			else
				recyHandle = m_RecyclerHandles[i];

			if(recyHandle && DLLUtils::IsRecycler(recyHandle))
			{
				SetCurHealth(recyHandle, GetMaxHealth(recyHandle));
			}

		} // m_TeamIsSetUp[i]
	} // i loop over all teams

#if VULNERABLE_RECY
	// Print periodic message about recycler invulnerability time left
	if((m_RecyInvulnerabilityTime == 1) ||
		((m_RecyInvulnerabilityTime % (60 * m_GameTPS)) == 0) ||
		((m_RecyInvulnerabilityTime < (60 * m_GameTPS)) && ((m_RecyInvulnerabilityTime % (15 * m_GameTPS)) == 0)))
	{

		if(m_RecyInvulnerabilityTime == 1)
			sprintf_s(TempMsgString, "Recyclers are now vulnerable. Happy hunting.");
		else if(m_RecyInvulnerabilityTime > (60 * m_GameTPS))
			sprintf_s(TempMsgString, "Recycler invulnerability expires in %d minutes", m_RecyInvulnerabilityTime / (60 * m_GameTPS));
		else
			sprintf_s(TempMsgString, "Recycler invulnerability expires in %d seconds", m_RecyInvulnerabilityTime / m_GameTPS);
		AddToMessagesBox2(TempMsgString, RGB(255, 0, 255)); // bright purple (ARGB)
	}

	// Reduce timer
	--m_RecyInvulnerabilityTime;
#endif
}


// Check for absence of recycler & factory, gameover if so.
void StratCTF::ExecuteCheckIfGameOver(void)
{
#ifndef EDITOR
	// No need to do anything more...
	if(m_GameOver)
		return;

	// Check for a gameover by no recycler & factory
	int i, NumFunctioningTeams=0;
	bool TeamIsFunctioning[MAX_TEAMS];

	memset(TeamIsFunctioning, 0, sizeof(TeamIsFunctioning));

	for(i=0;i<MAX_TEAMS;i++)
	{
		if(m_TeamIsSetUp[i])
		{
			bool Functioning=false; // Assume so for now.

			// Check if recycler vehicle still exists. Side effect to
			// note: IsAliveAndPilot zeroes the handle if pilot missing;
			// that'd be bad for us here if we want to manually remove
			// it. Thus, we have a sacrificial copy of it the game can
			// obliterate w/o hurting anything.
			Handle TempH=m_RecyclerHandles[i];					
			if((!IsAlive(TempH)) || (TempH==0))
				m_RecyclerHandles[i]=0; // Clear this out for later
			else
				Functioning=true; 

			// Check buildings as well.
			Handle RecyH = GetObjectByTeamSlot(i, DLL_TEAM_SLOT_RECYCLER);
			if(RecyH)
			{
				Functioning=true;

				// Note deployed location first time it deploys, also every 25.6 seconds
				if((!m_NotedRecyclerLocation[i]) || (!(m_ElapsedGameTime % 0xFF)))
				{
					m_NotedRecyclerLocation[i] = true;
					Vector RecyPos;
					GetPosition(RecyH, RecyPos);
					m_TeamPos[3*i+0] = RecyPos.x;
					m_TeamPos[3*i+1] = RecyPos.y;
					m_TeamPos[3*i+2] = RecyPos.z;
					// Apply this to thugs also.
					if(IsTeamplayOn())
					{
						for(int jj=GetFirstAlliedTeam(i);jj<=GetLastAlliedTeam(i);jj++)
						{
							// In teamplay, store where offense players were created for respawns later
							m_TeamPos[3*jj+0]=RecyPos.x;
							m_TeamPos[3*jj+1]=RecyPos.y;
							m_TeamPos[3*jj+2]=RecyPos.z;
						} // Loop over allies not the commander
					}
				} // Note deployed location
			}
			else
			{
				if(GetObjectByTeamSlot(i, DLL_TEAM_SLOT_FACTORY)!=0)
					Functioning=true;
			}

			if(Functioning)
			{
				TeamIsFunctioning[i]=true;
				NumFunctioningTeams++;
			} // Has recyclerV, recyclerB or FactoryB
		} // Loop over all set up teams
	}

	// If m_Alliances is on, then check the special cases. Basically, 
	// if all the allied teams disappear, but a non-allied team (5+)
	// is still around, the logic below should catch that.
	if(m_Alliances)
	{
		// Flag if non-allied teams are still operating
		bool NonAlliedTeamsFunctional=false;
		for(i=5;i<MAX_TEAMS;i++)
			if(TeamIsFunctioning[i])
				NonAlliedTeamsFunctional=true;

		// Allied teams can win only if the non-allied teams are all gone.
		if(!NonAlliedTeamsFunctional)
		{
			// Ok, now test the various combos.
			switch (m_Alliances)
			{
			case 1: // 1+2 vs 3+4
					// Test both cases of winners
				TestAlliedGameover(1, 2, 3, 4, TeamIsFunctioning);
				TestAlliedGameover(3, 4, 1, 2, TeamIsFunctioning);
				break;

			case 2: // 1+3 vs 2+4
				TestAlliedGameover(1, 3, 2, 4, TeamIsFunctioning);
				TestAlliedGameover(2, 4, 1, 3, TeamIsFunctioning);
				break;

			case 3: // 1+4 vs 2+3
				TestAlliedGameover(1, 4, 2, 3, TeamIsFunctioning);
				TestAlliedGameover(2, 3, 1, 4, TeamIsFunctioning);
				break;

			} // switch (m_Alliances)
		} // no non-allied teams are on

		// Exit asap if we set this.
		if(m_GameOver)
			return;
	} // m_Alliances on

	// Keep track if we ever had several teams playing. Don't need
	// to check for gameover if so-- 
	if(NumFunctioningTeams>1)
	{
		m_HadMultipleFunctioningTeams=true;
		return; // Exit function early
	}

	// Easy Gameover case: nobody's got a functioning base. End everything now.
#ifndef SERVER
	if((NumFunctioningTeams==0) && (m_ElapsedGameTime>50))
	{
		NoteGameoverByNoBases();
		DoGameover(endDelta);
		m_GameOver=true;
	}
	else
#endif
	{
		if((m_HadMultipleFunctioningTeams) && (NumFunctioningTeams==1))
		{
			// Ok, at one point we had >1 teams playing, now we've got
			// 1. They're the winner.

			// In teamplay, report the team as the winner
			if(IsTeamplayOn())
			{
				int WinningTeamgroup=-1;
				for(i=0;i<MAX_TEAMS;i++)
					if(TeamIsFunctioning[i])
					{
						if(WinningTeamgroup == -1)
						{
							WinningTeamgroup=WhichTeamGroup(i);
							NoteGameoverByLastTeamWithBase(WinningTeamgroup);
						}
					}

				// Also, give all players on winning team points...
				for(i=0;i<MAX_TEAMS;i++)
				{
					if(WhichTeamGroup(i) == WinningTeamgroup)
						AddScore(GetPlayerHandle(i), ScoreForWinning);
				}
				DoGameover(endDelta);
				m_GameOver=true;
			} // Teamplay is on
			else { // Non-teamplay, report individual winner
				// With alliances, we may not have a winner unless the team
				// remaining isn't allied (or we would have caught it above)
				int FirstTeam=0;
				if(m_Alliances)
					FirstTeam = 5;
				for(i=FirstTeam;i<MAX_TEAMS;i++)
				{
					if(TeamIsFunctioning[i])
					{
						NoteGameoverByLastWithBase(GetPlayerHandle(i));
						AddScore(GetPlayerHandle(i), ScoreForWinning);
						DoGameover(endDelta);
						m_GameOver=true;
					} // Found winner team
				}
			} // Non-teamplay.

		} // Winner
	}
#endif // #ifndef EDITOR
}

// Updates the strat-ctf goal.
void StratCTF::ExecuteST_CTF(void)
{
#ifndef EDITOR
	//	Handle PlayerH=GetPlayerHandle();

	// If the goal's gone, then bail asap.
	if((!m_GoalHandle) && (!m_GameOver))
	{
		NoteGameoverByNoBases();
		AddToMessagesBox2("Strat-CTF : goal is dead. Game over. :(", RGB(255, 0, 255));
		DoGameover(10.0f);
		m_GameOver = true;
	}

	Handle NewGoalTug = GetTug(m_GoalHandle);

	if(m_GoalTug != NewGoalTug)
	{

		Handle LocalPlayerHandle = GetPlayerHandle();

		if(NewGoalTug == 0)
		{
			AddToMessagesBox2("Goal dropped by tug!", RGB(255, 0, 255));
		}
		else
		{
			int TugTeam = GetTeamNum(NewGoalTug);
			Handle PlayerH = GetPlayerHandle(TugTeam);
			if(PlayerH)
			{
				if(IsAlly(PlayerH, LocalPlayerHandle))
				{
					sprintf_s(TempMsgString, "Goal has been tugged by %s. Get it to your recycler!", GetPlayerName(PlayerH));
				}
				else
				{
					sprintf_s(TempMsgString, "Goal has been tugged by %s. Prevent it from getting to their recycler!", GetPlayerName(PlayerH));
				}
			}
			else 
				sprintf_s(TempMsgString, "Goal has been tugged by unknown team!");
			AddToMessagesBox2(TempMsgString, RGB(255, 0, 255));

			// Also, constantly store position so that we don't get false-positive
			// goal moved messages.
			Vector GoalVector = GetPosition(m_GoalHandle);
			m_GoalPos[0] = GoalVector.x;
			m_GoalPos[1] = GoalVector.y;
			m_GoalPos[2] = GoalVector.z;
		}

		// Store tug of goal.
		m_GoalTug = NewGoalTug;
	}

	// Periodically re-objectify goal, and notify users if the goal has moved
	if((m_ElapsedGameTime % 50) == 0)
	{
		SetObjectiveOff(m_GoalHandle);
		SetObjectiveOn(m_GoalHandle);
		SetObjectiveName(m_GoalHandle, "Goal");

		// Notify players if goal has moved but isn't being tugged
		if(m_GoalTug == 0)
		{
			Vector GoalVector = GetPosition(m_GoalHandle);
			float fMoveDist = ((GoalVector.x - m_GoalPos[0]) * (GoalVector.x - m_GoalPos[0])) + 
				((GoalVector.z - m_GoalPos[2]) * (GoalVector.z - m_GoalPos[2]));
			if(fMoveDist > kfGOAL_MOVE_DIST)
			{

				// Tweak NM 9/17/05 - don't print 'goal is moving' in first 30
				// seconds of game. This prevents spurious messages if the
				// mapmaker placed it on a slope.
				if(m_ElapsedGameTime > 300) 
					AddToMessagesBox2("Goal is moving!", RGB(255, 0, 255));

				// Store current position.
				m_GoalPos[0] = GoalVector.x;
				m_GoalPos[1] = GoalVector.y;
				m_GoalPos[2] = GoalVector.z;
			}
		} // No m_GoalTug
	} // periodic 

	// Keep goal alive.
	AddHealth(m_GoalHandle, 1<<28);

	// See if any team has won.
	if(!m_GameOver)
	{
		int i;
		Handle RecyHandle = 0; // for this team, either vehicle or building
		for(i=0;i<MAX_TEAMS;i++)
		{
			if(m_TeamIsSetUp[i])
			{
				Handle TempH = m_RecyclerHandles[i];
				if((!IsAlive(TempH)) || (TempH==0))
					RecyHandle = GetObjectByTeamSlot(i, DLL_TEAM_SLOT_RECYCLER);
				else
					RecyHandle = m_RecyclerHandles[i];

				Dist GoalDist = GetDistance(m_GoalHandle, RecyHandle);
				if(GoalDist < kfGOAL_SCORE_DIST)
				{
					AddScore(GetPlayerHandle(i), ScoreForWinning);
					NoteGameoverByScore(i);
					AddToMessagesBox2("Strat-CTF has been won!", RGB(255, 0, 255));
					DoGameover(10.0f);
					m_GameOver = true;
				} // close enough to win
			} // Team[i] set up
		} // loop over teams
	} // !m_GameOver
#endif
}

void StratCTF::Execute(void)
{
	if (!m_DidInit)
	{
		Init();
	}
#ifndef EDITOR

	// Updates the strat-ctf goal.
	ExecuteST_CTF();

	// If Recycler invulnerability is on, then does the job of it.
	ExecuteRecyInvulnerability();

	// Check for absence of recycler & factory, gameover if so.
	ExecuteCheckIfGameOver();

	// Do this as well...
	UpdateGameTime();
#endif // #ifndef EDITOR
}

bool StratCTF::AddPlayer(DPID id, int Team, bool IsNewPlayer)
{
#ifndef EDITOR
	if (!m_DidInit)
	{
		Init();
	}

	if(IsNewPlayer)
	{
		Handle PlayerH=SetupPlayer(Team);
		SetAsUser(PlayerH, Team);
		AddPilotByHandle(PlayerH);
	}

#endif // #ifndef EDITOR
	return 1; // BOGUS: always assume successful
}

void StratCTF::DeletePlayer(DPID id)
{
}

// Rebuilds pilot
EjectKillRetCodes StratCTF::RespawnPilot(Handle DeadObjectHandle, int Team)
{
#ifdef EDITOR
	return DLLHandled;
#else
	Vector spawnpointPosition;

	// Only use safest place if invalid team #
	if((Team<1) || (Team>=MAX_TEAMS))
	{
		spawnpointPosition=GetSafestSpawnpoint();
	}
	else
	{
		// Use last noted position for the team
		spawnpointPosition.x = m_TeamPos[3*Team+0];
		spawnpointPosition.y = m_TeamPos[3*Team+1];
		spawnpointPosition.z = m_TeamPos[3*Team+2];
		m_SpawnedAtTime[Team] = m_ElapsedGameTime; // Note when they spawned in.
	}

	Vector OldPos;
	// As this object was just killed, gotta use the slower search for its
	// position.
	GetPosition2(DeadObjectHandle, OldPos);

	// Find out how far we are away from starting location... use
	// default if couldn't get position of DeadObjectHandle
	float respawnHeight = RespawnPilotHeight;
	if((fabsf(OldPos.x) > 0.01f) && (fabsf(OldPos.z) > 0.01f))
	{
		// Position valid. Use it.
		const float dx = OldPos.x - spawnpointPosition.x;
		const float dz = OldPos.z - spawnpointPosition.z;
		// How far this person died from where we'll respawn them
		const float distanceAway = sqrtf((dx * dx) + (dz * dz));
		if(distanceAway < 100.f)
		{
			respawnHeight = 35.f; // 1.2 used 25.f here
		}
		else
		{
			// Min of 40, max varies by # of allies. More penalty for
			// dying far away from your team
			const int numAllies = DLLUtils::CountAlliedPlayers(Team);
			respawnHeight = 30.f + (sqrtf(distanceAway) * 1.25f);
			const float minRespawnHeight = 40.0f;
			const float maxRespawnHeight = 72.0f + (15.0f * numAllies);

			if(respawnHeight < minRespawnHeight)
				respawnHeight = minRespawnHeight;
			else if(respawnHeight > maxRespawnHeight)
				respawnHeight = maxRespawnHeight;
		}
	}
	if(m_RespawnAtLowAltitude)
	{
		respawnHeight = 2.0f;
	}

	// Randomize starting position somewhat. This gives a range of +/-
	// RespawnDistanceAwayXZRange
	spawnpointPosition.x += (GetRandomFloat(1.f) - 0.5f) * (2.f * RespawnDistanceAwayXZRange);
	spawnpointPosition.z += (GetRandomFloat(1.f) - 0.5f) * (2.f * RespawnDistanceAwayXZRange);

	// Don't allow a spawn underground - just in case there's a cliff
	// near the starting spawnpointPosition position, we need to keep y above the
	// ground.
	{
		const float curFloor = TerrainFindFloor(spawnpointPosition.x, spawnpointPosition.z) + 2.5f;
		if(spawnpointPosition.y < curFloor)
		{
			spawnpointPosition.y = curFloor;
		}
	}
	spawnpointPosition.y += respawnHeight; // Bounce them in the air to prevent multi-kills
	spawnpointPosition.y += GetRandomFloat(1.f) * RespawnDistanceAwayYRange;

	Handle NewPerson = BuildObject(GetInitialPlayerPilotODF(GetRaceOfTeam(Team)), Team, spawnpointPosition);
	SetAsUser(NewPerson, Team);
	AddPilotByHandle(NewPerson);
	SetRandomHeadingAngle(NewPerson);

	// If on team 0 (dedicated server team), make this object gone from the world
	if(!Team)
	{
		MakeInert(NewPerson);
	}

	return DLLHandled; // Dead pilots get handled by DLL
#endif // #ifndef EDITOR
}

// Helper function for ObjectKilled/Sniped
EjectKillRetCodes StratCTF::DeadObject(int DeadObjectHandle, int KillersHandle, bool isDeadPerson, bool isDeadAI)
{
#ifdef EDITOR
	return DLLHandled;
#else

	// STCTF changes - if goal is dead, then really penalize its killer
	if(DeadObjectHandle == m_GoalHandle)
	{
		int KillerTeam = GetTeamNum(KillersHandle);
		if(!KillerTeam)
		{
			AddToMessagesBox2("Goal killed by unknown team.", RGB(255, 0, 255));
		}
		else
		{
			Handle PlayerH = GetPlayerHandle(KillerTeam);
			if(PlayerH)
			{
				sprintf_s(TempMsgString, "Goal has been killed %s. Bad move.", GetPlayerName(PlayerH));
				AddScore(GetPlayerHandle(KillerTeam), -ScoreForWinning);
			}
			else
			{
				sprintf_s(TempMsgString, "Goal has been killed by AI team");
			}
		}
	}

	//	bool KillerIsAI=!IsPlayer(KillersHandle);

	// Get team number of who got waxed.
	int deadTeam=GetTeamNum(DeadObjectHandle);
	bool isSpawnKill=false;
	// Flip scoring if this is a spawn kill.
	int SpawnKillMultiplier=1;

	// Give positive or negative points to killer, depending on
	// whether they killed enemy or ally
	if(1) //!KillerIsAI)
	{
		int spawnKillTime = MaxSpawnKillTime * m_GameTPS; // 15 seconds

		// Spawnkill is a non-suicide, on a human by another human.
		// Added check for !isDeadAI NM 11/25/06 - APC soldiers dying
		// around the same time as their human player would trip this
		// up.
		isSpawnKill = (DeadObjectHandle != KillersHandle) && 
			(!isDeadAI) &&
			(deadTeam > 0) && (deadTeam < MAX_TEAMS) && 
			(m_SpawnedAtTime[deadTeam] > 0) &&
			((m_ElapsedGameTime - m_SpawnedAtTime[deadTeam]) < spawnKillTime);

		if(isSpawnKill)
		{
			SpawnKillMultiplier=-1;
			sprintf_s(TempMsgString, "Spawn kill by %s on %s\n", 
				GetPlayerName(KillersHandle), GetPlayerName(DeadObjectHandle));
			AddToMessagesBox(TempMsgString);
		}

		if((DeadObjectHandle != KillersHandle) && (!IsAlly(DeadObjectHandle, KillersHandle)))
		{
			// Killed enemy...
			if((!isDeadAI) || (m_KillForAIKill))
				AddKills(KillersHandle, 1*SpawnKillMultiplier); // Give them a kill

			if((!isDeadAI) || (m_PointsForAIKill))
			{
				if(isDeadPerson)
					AddScore(KillersHandle, ScoreForKillingPerson*SpawnKillMultiplier);
				else
					AddScore(KillersHandle, ScoreForKillingCraft*SpawnKillMultiplier);
			}
		}
		else
		{
			if((!isDeadAI) || (m_KillForAIKill))
				AddKills(KillersHandle, -1*SpawnKillMultiplier); // Suicide or teamkill counts as -1 kill

			if((!isDeadAI) || (m_PointsForAIKill))
			{
				if(isDeadPerson)
					AddScore(KillersHandle, -ScoreForKillingPerson*SpawnKillMultiplier);
				else
					AddScore(KillersHandle, -ScoreForKillingCraft*SpawnKillMultiplier);
			}
		}
	} // Killer was a human

	if(1) //!isDeadAI)
	{
		// Give points to killee-- this always increases
		AddDeaths(DeadObjectHandle, 1*SpawnKillMultiplier);
		if(isDeadPerson)
			AddScore(DeadObjectHandle, ScoreForDyingAsPerson*SpawnKillMultiplier);
		else
			AddScore(DeadObjectHandle, ScoreForDyingAsCraft*SpawnKillMultiplier);
	}

	// Check to see if we have a m_KillLimit winner
	if((m_KillLimit) && (GetKills(KillersHandle)>=m_KillLimit))
	{
		NoteGameoverByKillLimit(KillersHandle);
		DoGameover(10.0f);
	}

	if(deadTeam==0)
		return DoEjectPilot; // Someone on neutral team always gets default behavior

	if(isDeadAI)
	{
		// Snipe?
		if(isDeadPerson)
			return DLLHandled;
		else // Nope. Eject.
			return DoEjectPilot;
	}
	else  // Not DeadAI, i.e. a human!
	{
		// If this was a dead pilot, we need to build another pilot back
		// at base. Otherwise, we just eject a pilot from the
		// craft. [This is strat, nobody gets a craft for free when they
		// lose one.]
		if(isDeadPerson)
			return RespawnPilot(DeadObjectHandle, deadTeam);
		else 
			return DoEjectPilot;
	}
#endif // #ifndef EDITOR
}

EjectKillRetCodes StratCTF::PlayerEjected(Handle DeadObjectHandle)
{
#ifdef EDITOR
	return DLLHandled;
#else
	int deadTeam=GetTeamNum(DeadObjectHandle);
	if(deadTeam==0)
		return DLLHandled; // Invalid team. Do nothing

	// Update Deaths, Kills, Score for this player
	AddDeaths(DeadObjectHandle, 1);
	AddKills(DeadObjectHandle, -1);
	AddScore(DeadObjectHandle, ScoreForDyingAsCraft-ScoreForKillingCraft);

	return DoEjectPilot; // Tell main code to allow the ejection
#endif // #ifndef EDITOR
}

EjectKillRetCodes StratCTF::ObjectKilled(int DeadObjectHandle, int KillersHandle)
{
#ifdef EDITOR
	return DLLHandled;
#else
	bool isDeadAI=!IsPlayer(DeadObjectHandle);

	bool isDeadPerson=IsPerson(DeadObjectHandle);

	// Sanity check for multiworld
	if(GetCurWorld() != 0)
		return DoEjectPilot;

	int deadTeam=GetTeamNum(DeadObjectHandle);
	if(deadTeam==0)
		return DoEjectPilot; // Someone on neutral team always gets default behavior

	// If a person died, respawn them, etc
	return DeadObject(DeadObjectHandle, KillersHandle, isDeadPerson, isDeadAI);
#endif // #ifndef EDITOR
}


EjectKillRetCodes StratCTF::ObjectSniped(int DeadObjectHandle, int KillersHandle)
{
#ifdef EDITOR
	return DLLHandled;
#else
	bool isDeadAI=!IsPlayer(DeadObjectHandle);

	if(GetCurWorld() != 0)
	{
		return DLLHandled;
	}

	// Dead person means we must always respawn a new person
	return DeadObject(DeadObjectHandle, KillersHandle, true, isDeadAI);
#endif // #ifndef EDITOR
}

bool StratCTF::Load(bool missionSave)
{
	EnableHighTPS(m_GameTPS);
	SetAutoGroupUnits(false);

	// Make sure we always call this
	StartingVehicleManager::Load(missionSave);

	// We're a 1.3 DLL.
	WantBotKillMessages();

	// Do this for everyone as well.
	ClearObjectives();
	AddObjective("stratctf.otf", WHITE, -1.0f); // negative time means don't change display to show it

	if (missionSave)
	{
		int i;

		// init bools
		if((b_array) && (b_count))
			for (i = 0; i < b_count; i++)
				b_array[i] = false;

		// init floats
		if((f_array) && (f_count))
			for (i = 0; i < f_count; i++)
				f_array[i] = 0.0f;

		// init handles
		if((h_array) && (h_count))
			for (i = 0; i < h_count; i++)
				h_array[i] = 0;

		// init ints
		if((i_array) && (i_count))
			for (i = 0; i < i_count; i++)
				i_array[i] = 0;

		return true;
	}

	bool ret = true;

	// bools
	if (b_array != NULL)
		Read(b_array, b_count);

	// floats
	if (f_array != NULL)
		Read(f_array, f_count);

	// Handles
	if (h_array != NULL)
		Read(h_array, h_count);

	// ints
	if (i_array != NULL)
		Read(i_array, i_count);

	return ret;
}

bool StratCTF::PostLoad(bool missionSave)
{
	if (missionSave)
		return true;

	bool ret = true;

	ConvertHandles(h_array, h_count);

	return ret;
}

bool StratCTF::Save(bool missionSave)
{
	// Make sure we always call this
	StartingVehicleManager::Save(missionSave);

	if (missionSave)
		return true;

	bool ret = true;

	// bools
	if (b_array != NULL)
		Write(b_array, b_count);

	// floats
	if (f_array != NULL)
		Write(f_array, f_count);

	// Handles
	if (h_array != NULL)
		Write(h_array, h_count);

	// ints
	if (i_array != NULL)
		Write(i_array, i_count);

	return ret;
}


// Notification to the DLL: a sniper shell has hit a piloted
// craft. The exe passes the current world, shooters handle, victim
// handle, and the ODF string of the ordnance involved in the
// snipe. Returns a code detailing what to do.
//
// !! Note : If DLLs want to do any actions to the world based on this
// PreSnipe callback, they should (1) Ensure curWorld == 0 (lockstep)
// -- do NOTHING if curWorld is != 0, and (2) probably queue up an
// action to do in the next Execute() call.
PreSnipeReturnCodes StratCTF::PreSnipe(const int curWorld, Handle shooterHandle, Handle victimHandle, int ordnanceTeam, char* pOrdnanceODF)
{
	// If Friendly Fire is off, then see if we should disallow the snipe
	if(!m_bIsFriendlyFireOn)
	{
		const TEAMRELATIONSHIP relationship = GetTeamRelationship(shooterHandle, victimHandle);
		if(//(relationship == TEAMRELATIONSHIP_SAMETEAM) || 
		   (relationship == TEAMRELATIONSHIP_ALLIEDTEAM))
		{
			// Allow snipes of items on team 0/perceived team 0, as long
			// as they're not a local/remote player
			if(IsPlayer(victimHandle) || (GetTeamNum(victimHandle) != 0))
			{
				return PRESNIPE_ONLYBULLETHIT;
			}
		}
		// Friendly fire is off, and we're about to kill the pilot of the hit object.
		// Set its team to 0
		SetPerceivedTeam(victimHandle, 0);
	}

	// If we got here, allow the pilot to be killed
	return PRESNIPE_KILLPILOT;
}


// Notification to the DLL: called when a pilot tries to enter an
// empty craft, and all other checks (i.e. craft is empty, masks
// match, etc) have passed. DLLs can prevent that pilot from entering
// the craft if desired.
//
// !! Note : If DLLs want to do any actions to the world based on this
// PreSnipe callback, they should (1) Ensure curWorld == 0 (lockstep)
// -- do NOTHING if curWorld is != 0, and (2) probably queue up an
// action to do in the next Execute() call.
PreGetInReturnCodes StratCTF::PreGetIn(const int curWorld, Handle pilotHandle, Handle emptyCraftHandle)
{
	// Is this an AI pilot getting into an allied craft? If so, change
	// the pilot's team to be the same as the craft, so that the craft
	// remains with its owning team. If a local or remotely-controlled
	// player tries to get in, behave as before.
	const TEAMRELATIONSHIP relationship = GetTeamRelationship(pilotHandle, emptyCraftHandle);
	if((relationship == TEAMRELATIONSHIP_ALLIEDTEAM) &&
	   (!IsPlayer(pilotHandle)))
	{
		SetTeamNum(pilotHandle, GetTeamNum(emptyCraftHandle));
	}

	// Always allow the entry
	return PREGETIN_ALLOW;
}


DLLBase *BuildMission(void)
{
	return new StratCTF;
}
