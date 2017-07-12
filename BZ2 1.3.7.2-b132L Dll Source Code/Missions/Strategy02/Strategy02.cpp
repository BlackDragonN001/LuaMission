#include "..\Shared\DLLBase.h"
#include "strategy02.h"
#include "..\Shared\StartingVehicles.h"
#include "..\Shared\TRNAllies.h"
#include "..\Shared\DLLUtils.h"

#include <math.h>
#include <string.h>
#include <malloc.h>
#include <algorithm>

#define VEHICLE_SPACING_DISTANCE (20.0f)

// ---------- Scoring Values-- these are delta scores, added to current score --------
const int ScoreDecrementForSpawnKill = 500;
const int ScoreForWinning = 100;

// How long a "spawn" kill lasts, in tenth of second ticks. If the
// time since they were spawned to current is less than this, it's a
// spawn kill, and not counted. Todo - Make this an ivar for dm?
const int MaxSpawnKillTime = 15;

// -----------------------------------------------

// Max distance (in x,z dimensions) on a pilot respawn. Actual value
// used will vary with random numbers, and will be +/- this value
const float RespawnDistanceAwayXZRange = 32.f;
// Max distance (in y dimensions) on a pilot respawn. Actual value
// used will vary with random numbers, and will be [0..this value)
const float RespawnDistanceAwayYRange = 8.f;

// How high up to respawn a pilot
const float RespawnPilotHeight = 200.0f;

// How far allies will be from the commander's position
const float AllyMinRadiusAway = 30.0f;
const float AllyMaxRadiusAway = 60.0f;

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
Strategy02::Strategy02(void)
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


char *Strategy02::GetNextRandomVehicleODF(int Team)
{
	return GetPlayerODF(Team);
}


void Strategy02::AddObject(Handle h)
{
#ifndef EDITOR
	char ODFName[64];
	GetObjInfo(h, Get_CFG, ODFName);

#if 0
#ifdef _DEBUG
	// So we can test gameover logic better
	SetCurHealth(h, 1);
	SetMaxHealth(h, 1);
#endif
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
	bool isTurret = (_stricmp(ObjClass, "CLASS_TURRETTANK") == 0);

	// See if this is a turret, 
	if(isTurret)
	{
		SetSkill(h, UseTurretSkill);
		return;
	}
	else
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

	// Special things for FFA Mode with Thugs. -GBD
	int Team = GetTeamNum(h);
	bool AddToList = true;
	if(!IsTeamplayOn())
	{
		// Scavenger code: added to handle thug scavs in FFA Alliance mode. -GBD
		if((_stricmp(ObjClass, "CLASS_SCAVENGER") == 0) || (_stricmp(ObjClass, "CLASS_SCAVENGERH") == 0))
		{
			AddToList = true;
			for (std::vector<ThugScavengerClass>::iterator iter = ThugScavengerList.begin(); iter != ThugScavengerList.end(); ++iter)
			{
				if(iter->ThugScavengerObject == h)
				{
					AddToList = false;
					break;
				}
			}
			if(AddToList)
			{
				ThugScavengerList.resize(ThugScavengerList.size()+1);
				ThugScavengerClass &sao = ThugScavengerList[ThugScavengerList.size()-1]; // No temporary being created, *much* faster
				memset(&sao, 0, sizeof(ThugScavengerClass)); // Zero things out at start.

				sao.ThugScavengerObject = h;
				sao.ScavengerTeam = Team;
			}
		}
		// Thug code, adding thugs to be thugified. (Thug pilots to switch to Commander's team after 15 seconds)
		else if((_stricmp(ObjClass, "CLASS_PERSON") == 0) && (!IsPlayer(h)))
		{
			for (std::vector<ThugPilotClass>::iterator iter = ThugPilotList.begin(); iter != ThugPilotList.end(); ++iter)
			{
				if(iter->ThugPilotObject == h)
				{
					AddToList = false;
					break;
				}
			}
			if(AddToList)
			{
				ThugPilotList.resize(ThugPilotList.size()+1);
				ThugPilotClass &sao = ThugPilotList[ThugPilotList.size()-1]; // No temporary being created, *much* faster
				memset(&sao, 0, sizeof(ThugPilotClass)); // Zero things out at start.

				sao.ThugPilotObject = h;
				sao.SwitchTickTimer = (15*m_GameTPS); // Set Timer, 15 seconds.
			}
		}
	}
#endif
}

// Gets the initial player vehicle as selected in the shell
char *Strategy02::GetInitialPlayerVehicleODF(int Team)
{
	return GetPlayerODF(Team);
}

// Given a race identifier, get the pilot back (used during a respawn)
char *Strategy02::GetInitialPlayerPilotODF(char Race)
{
	if(m_RespawnWithSniper)
		strcpy_s(TempODFName, "ispilo"); // With sniper.
	else
		strcpy_s(TempODFName, "isuser_m"); // Note-- this is the sniper-less variant for a respawn
	TempODFName[0]=Race;
	return TempODFName;
}

// Given a race identifier, get the recycler ODF back
char *Strategy02::GetInitialRecyclerODF(char Race)
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
char *Strategy02::GetInitialScavengerODF(char Race)
{
	strcpy_s(TempODFName, "ivscav");
	TempODFName[0]=Race;
	return TempODFName;
}

// Given a race identifier, get the constructor ODF back
char *Strategy02::GetInitialConstructorODF(char Race)
{
	strcpy_s(TempODFName, "ivcons");
	TempODFName[0]=Race;
	return TempODFName;
}

// Given a race identifier, get the healer (service truck) ODF back
char *Strategy02::GetInitialHealerODF(char Race)
{
	strcpy_s(TempODFName, "ivserv");
	TempODFName[0]=Race;
	return TempODFName;
}


// Helper function for SetupTeam(), returns an appropriate spawnpoint.
Vector Strategy02::GetSpawnpointForTeam(int Team)
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
void Strategy02::SetupTeam(int Team)
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

	// Build recycler some distance away, if it's not preplaced on the map.
	if(GetObjectByTeamSlot(Team, DLL_TEAM_SLOT_RECYCLER) == 0)
	{
		spawnpointPosition = GetPositionNear(spawnpointPosition, VEHICLE_SPACING_DISTANCE, 2*VEHICLE_SPACING_DISTANCE);
		int VehicleH;
		VehicleH = BuildObject(GetInitialRecyclerODF(TeamRace), Team, spawnpointPosition);
		SetRandomHeadingAngle(VehicleH);
		m_RecyclerHandles[Team] = VehicleH;
		SetGroup(VehicleH, 0);
	}

	// Build all optional vehicles for this team.
	spawnpointPosition.x = m_TeamPos[3*Team+0]; // restore default after we modified this for recy above
	spawnpointPosition.y = m_TeamPos[3*Team+1];
	spawnpointPosition.z = m_TeamPos[3*Team+2];

	// Drop skill level while we build things.
	m_CreatingStartingVehicles=true;
	StartingVehicleManager::CreateVehicles(Team, TeamRace, m_StartingVehiclesMask, spawnpointPosition);
	m_CreatingStartingVehicles=false;

	SetScrap(Team, 40);

	// Modified spawn code to handle FFA Alliances with thugs. -GBD
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
	}
	else // FFA Mode. -GBD
	{
		for(int i = 0; i<MAX_TEAMS; i++)
		{
			if((i!=Team) && (m_AllyTeams[Team] == m_AllyTeams[i]))
			{
				Vector NewPosition = GetPositionNear(spawnpointPosition, AllyMinRadiusAway, AllyMaxRadiusAway);

				m_TeamPos[3*i+0]=NewPosition.x;
				m_TeamPos[3*i+1]=NewPosition.y;
				m_TeamPos[3*i+2]=NewPosition.z;
			}
		}
	}

#if 0//def _DEBUG
	// Test out spawn point safety
	{
		SpawnpointInfo* pSpawnPointInfo;
		size_t i,count = GetAllSpawnpoints(pSpawnPointInfo, 7 - Team);
	}
#endif

	m_TeamIsSetUp[Team] = true;
#endif // #ifndef EDITOR
}

// Given an index into the Player list, build everything for a given
// single player (i.e. a vehicle of some sorts), and set up the team
// as well as necessary
Handle Strategy02::SetupPlayer(int Team)
{
#ifndef EDITOR
	Handle PlayerH = 0;
	Vector spawnpointPosition;
	memset(&spawnpointPosition, 0, sizeof(spawnpointPosition));

	if((Team<0) || (Team>=MAX_TEAMS))
		return 0; // Sanity check... do NOT proceed

	m_SpawnedAtTime[Team] = m_ElapsedGameTime; // Note when they spawned in.

	int TeamBlock=WhichTeamGroup(Team);

	if((!IsTeamplayOn()) || (TeamBlock<0))
	{
		// This player is their own commander; set up their equipment.
		if(!m_ThugFlag[Team]) // If this player is a commander. -GBD
		{
			SetupTeam(Team);
		}

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

	// Added to make your starting pilot match respawning pilot. -GBD
	strcpy_s(TempODFName, "ispilo");
	TempODFName[0] = GetRaceOfTeam(Team);
	SetPilotClass(PlayerH, TempODFName);
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
void Strategy02::Init(void)
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

	// Remvoed m_alliances, now just uses "m_HasAllies[TEAM]" flag set below. -GBD
	/*
	if(IsTeamplayOn())
	{
		m_Alliances = 0; // clear this 
	}

	else 
	*/
	
	// Redone ally code. -GBD
	// Note: currently ivar23 must be >0 for allies to send units to eachother. -GBD
	memset(m_AllyTeams, 0, sizeof(m_AllyTeams));
	if(!IsTeamplayOn())
	{
		// Grab the "Team" numbers. 
		m_AllyTeams[1] = GetVarItemInt("network.session.ivar60");
		m_AllyTeams[2] = GetVarItemInt("network.session.ivar67");
		m_AllyTeams[3] = GetVarItemInt("network.session.ivar68");
		m_AllyTeams[4] = GetVarItemInt("network.session.ivar69");
		m_AllyTeams[5] = GetVarItemInt("network.session.ivar70");
		m_AllyTeams[6] = GetVarItemInt("network.session.ivar71");
		m_AllyTeams[7] = GetVarItemInt("network.session.ivar72");
		m_AllyTeams[8] = GetVarItemInt("network.session.ivar73");
		m_AllyTeams[9] = GetVarItemInt("network.session.ivar74");
		m_AllyTeams[10] = GetVarItemInt("network.session.ivar75");
		m_AllyTeams[11] = GetVarItemInt("network.session.ivar76");
		m_AllyTeams[12] = GetVarItemInt("network.session.ivar77");
		m_AllyTeams[13] = GetVarItemInt("network.session.ivar78");
		m_AllyTeams[14] = GetVarItemInt("network.session.ivar79");
		m_AllyTeams[15] = GetVarItemInt("network.session.ivar80");

		// Thug bools. 0 = Commander, 1 = Thug.
		m_ThugFlag[1] = GetVarItemInt("network.session.ivar81");
		m_ThugFlag[2] = GetVarItemInt("network.session.ivar82");
		m_ThugFlag[3] = GetVarItemInt("network.session.ivar83");
		m_ThugFlag[4] = GetVarItemInt("network.session.ivar84");
		m_ThugFlag[5] = GetVarItemInt("network.session.ivar85");
		m_ThugFlag[6] = GetVarItemInt("network.session.ivar86");
		m_ThugFlag[7] = GetVarItemInt("network.session.ivar87");
		m_ThugFlag[8] = GetVarItemInt("network.session.ivar88");
		m_ThugFlag[9] = GetVarItemInt("network.session.ivar89");
		m_ThugFlag[10] = GetVarItemInt("network.session.ivar90");
		m_ThugFlag[11] = GetVarItemInt("network.session.ivar91");
		m_ThugFlag[12] = GetVarItemInt("network.session.ivar92");
		m_ThugFlag[13] = GetVarItemInt("network.session.ivar93");
		m_ThugFlag[14] = GetVarItemInt("network.session.ivar94");
		m_ThugFlag[15] = GetVarItemInt("network.session.ivar95");

		// Loop over all teams, and ally them if they're set to be allies. 
		for(int x = 1; x < MAX_TEAMS; x++)
		{
			for(int y = 1; y < MAX_TEAMS; y++)
			{
				if((m_AllyTeams[x] > 0) && (x != y) && (m_AllyTeams[x] == m_AllyTeams[y]))
				{
					Ally(x, y);
					m_HasAllies[x] = true;
				}
			}
		}	// Finshed looping.

		// Setup their commander.
		for(int i = 1; i < MAX_TEAMS; i++)
		{
			if(!m_TeamHasCommander[m_AllyTeams[i]]) // This team has no set commander, lets find one. 
			{
				for(int x = 0; x < MAX_TEAMS;x++)
				{
					if((!m_ThugFlag[x]) && (m_AllyTeams[i] == m_AllyTeams[x])) // This is a commander, set them for their team. 
					{
						m_TeamHasCommander[m_AllyTeams[i]] = true;
						break; //found one.
					}
				}

				if(!m_TeamHasCommander[m_AllyTeams[i]]) // Uh oh, no commander set. Pick the first player.
				{
					for(int p = 0; p < MAX_TEAMS; p++)
					{
						if(m_AllyTeams[p] == m_AllyTeams[i]) // This player is on same team. flag them.
						{
							m_ThugFlag[p] = false;
							m_TeamHasCommander[m_AllyTeams[i]] = true;
							break; // found one.
						}
					}
				}
			}
		}
	}

	// Old code. This was such a clunky method :( -GBD
	/*
	else
	{
		m_Alliances = GetVarItemInt("network.session.ivar23");
		switch (m_Alliances)
		{
		// Original Teams
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

		// Mass Pairs
		case 11: // vs 13+14
			Ally(13, 14);
			Ally(14, 13);
		case 12: // vs 11+12
			Ally(11, 12);
			Ally(12, 11);
		case 13: // vs 9+10
			Ally( 9, 10);
			Ally(10,  9);
		case 14: // vs 7+8
			Ally( 7,  8);
			Ally( 8,  7);
		case 15: // 1+2 vs 3+4 vs 5+6
			Ally( 5,  6);
			Ally( 6,  5);
			Ally( 3,  4);
			Ally( 4,  3);
			Ally( 1,  2);
			Ally( 2,  1);
			break;

		// Mass Triads
		case 16: // vs 10+11+12
			Ally(10, 11);
			Ally(10, 12);
			Ally(11, 10);
			Ally(11, 12);
			Ally(12, 10);
			Ally(12, 11);
		case 17: // vs 7+8+9
			Ally( 7,  8);
			Ally( 7,  9);
			Ally( 8,  7);
			Ally( 8,  9);
			Ally( 9,  7);
			Ally( 9,  8);
		case 18: // 1+2+3 vs 4+5+6
			Ally( 4,  5);
			Ally( 4,  6);
			Ally( 5,  4);
			Ally( 5,  6);
			Ally( 6,  4);
			Ally( 6,  5);
			Ally( 1,  2);
			Ally( 1,  3);
			Ally( 2,  1);
			Ally( 2,  3);
			Ally( 3,  1);
			Ally( 3,  2);
			break;

		// Mass Quartets
		case 19: // vs 9+10+11+12
			Ally( 9, 10);
			Ally( 9, 11);
			Ally( 9, 12);
			Ally(10,  9);
			Ally(10, 11);
			Ally(10, 12);
			Ally(11,  9);
			Ally(11, 10);
			Ally(11, 12);
			Ally(12,  9);
			Ally(12, 10);
			Ally(12, 11);
		case 20: // 1+2+3+4 vs 5+6+7+8
			Ally( 5,  6);
			Ally( 5,  7);
			Ally( 5,  8);
			Ally( 6,  5);
			Ally( 6,  7);
			Ally( 6,  8);
			Ally( 7,  5);
			Ally( 7,  6);
			Ally( 7,  8);
			Ally( 8,  5);
			Ally( 8,  6);
			Ally( 8,  7);
			Ally( 1,  2);
			Ally( 1,  3);
			Ally( 1,  4);
			Ally( 2,  1);
			Ally( 2,  3);
			Ally( 2,  4);
			Ally( 3,  1);
			Ally( 3,  2);
			Ally( 3,  4);
			Ally( 4,  1);
			Ally( 4,  2);
			Ally( 4,  3);
			break;

		// Mass Mixed Quartets
		case 21: // vs 9+10+11+12
			Ally( 9, 10);
			Ally( 9, 11);
			Ally( 9, 12);
			Ally(10,  9);
			Ally(10, 11);
			Ally(10, 12);
			Ally(11,  9);
			Ally(11, 10);
			Ally(11, 12);
			Ally(12,  9);
			Ally(12, 10);
			Ally(12, 11);
		case 22: // 1+3+5+7 vs 2+4+6+8
			Ally( 2,  4);
			Ally( 2,  6);
			Ally( 2,  8);
			Ally( 4,  2);
			Ally( 4,  6);
			Ally( 4,  8);
			Ally( 6,  2);
			Ally( 6,  4);
			Ally( 6,  8);
			Ally( 8,  2);
			Ally( 8,  4);
			Ally( 8,  6);
			Ally( 1,  3);
			Ally( 1,  5);
			Ally( 1,  7);
			Ally( 3,  1);
			Ally( 3,  5);
			Ally( 3,  7);
			Ally( 5,  1);
			Ally( 5,  3);
			Ally( 5,  7);
			Ally( 7,  1);
			Ally( 7,  3);
			Ally( 7,  5);
			break;

		// Heptets
		case 23: // 1+2+3+4+5+6+7 vs 8+9+10+11+12+13+14
			Ally( 1,  2);
			Ally( 1,  3);
			Ally( 1,  4);
			Ally( 1,  5);
			Ally( 1,  6);
			Ally( 1,  7);
			Ally( 2,  1);
			Ally( 2,  3);
			Ally( 2,  4);
			Ally( 2,  5);
			Ally( 2,  6);
			Ally( 2,  7);
			Ally( 3,  1);
			Ally( 3,  2);
			Ally( 3,  4);
			Ally( 3,  5);
			Ally( 3,  6);
			Ally( 3,  7);
			Ally( 4,  1);
			Ally( 4,  2);
			Ally( 4,  3);
			Ally( 4,  5);
			Ally( 4,  6);
			Ally( 4,  7);
			Ally( 5,  1);
			Ally( 5,  2);
			Ally( 5,  3);
			Ally( 5,  4);
			Ally( 5,  6);
			Ally( 5,  7);
			Ally( 6,  1);
			Ally( 6,  2);
			Ally( 6,  3);
			Ally( 6,  4);
			Ally( 6,  5);
			Ally( 6,  7);
			Ally( 7,  1);
			Ally( 7,  2);
			Ally( 7,  3);
			Ally( 7,  4);
			Ally( 7,  5);
			Ally( 7,  6);
			Ally( 8,  9);
			Ally( 8, 10);
			Ally( 8, 11);
			Ally( 8, 12);
			Ally( 8, 13);
			Ally( 8, 14);
			Ally( 9,  8);
			Ally( 9, 10);
			Ally( 9, 11);
			Ally( 9, 12);
			Ally( 9, 13);
			Ally( 9, 14);
			Ally(10,  8);
			Ally(10,  9);
			Ally(10, 11);
			Ally(10, 12);
			Ally(10, 13);
			Ally(10, 14);
			Ally(11,  8);
			Ally(11,  9);
			Ally(11, 10);
			Ally(11, 12);
			Ally(11, 13);
			Ally(11, 14);
			Ally(12,  8);
			Ally(12,  9);
			Ally(12, 10);
			Ally(12, 11);
			Ally(12, 13);
			Ally(12, 14);
			Ally(13,  8);
			Ally(13,  9);
			Ally(13, 10);
			Ally(13, 11);
			Ally(13, 12);
			Ally(13, 14);
			Ally(14,  8);
			Ally(14,  9);
			Ally(14, 10);
			Ally(14, 11);
			Ally(14, 12);
			Ally(14, 13);
			break;

		// Mixed Heptets
		case 24: // 1+3+5+7+9+11+13 vs 2+4+6+8+10+12+14
			Ally( 1,  3);
			Ally( 1,  5);
			Ally( 1,  7);
			Ally( 1,  9);
			Ally( 1, 11);
			Ally( 1, 13);
			Ally( 3,  1);
			Ally( 3,  5);
			Ally( 3,  7);
			Ally( 3,  9);
			Ally( 3, 11);
			Ally( 3, 13);
			Ally( 5,  1);
			Ally( 5,  3);
			Ally( 5,  7);
			Ally( 5,  9);
			Ally( 5, 11);
			Ally( 5, 13);
			Ally( 7,  1);
			Ally( 7,  3);
			Ally( 7,  5);
			Ally( 7,  9);
			Ally( 7, 11);
			Ally( 7, 13);
			Ally( 9,  1);
			Ally( 9,  3);
			Ally( 9,  5);
			Ally( 9,  7);
			Ally( 9, 11);
			Ally( 9, 13);
			Ally(11,  1);
			Ally(11,  3);
			Ally(11,  5);
			Ally(11,  7);
			Ally(11,  9);
			Ally(11, 13);
			Ally(13,  1);
			Ally(13,  3);
			Ally(13,  5);
			Ally(13,  7);
			Ally(13,  9);
			Ally(13, 11);
			Ally( 2,  4);
			Ally( 2,  6);
			Ally( 2,  8);
			Ally( 2, 10);
			Ally( 2, 12);
			Ally( 2, 14);
			Ally( 4,  2);
			Ally( 4,  6);
			Ally( 4,  8);
			Ally( 4, 10);
			Ally( 4, 12);
			Ally( 4, 14);
			Ally( 6,  2);
			Ally( 6,  4);
			Ally( 6,  8);
			Ally( 6, 10);
			Ally( 6, 12);
			Ally( 6, 14);
			Ally( 8,  2);
			Ally( 8,  4);
			Ally( 8,  6);
			Ally( 8, 10);
			Ally( 8, 12);
			Ally( 8, 14);
			Ally(10,  2);
			Ally(10,  4);
			Ally(10,  6);
			Ally(10,  8);
			Ally(10, 12);
			Ally(10, 14);
			Ally(12,  2);
			Ally(12,  4);
			Ally(12,  6);
			Ally(12,  8);
			Ally(12, 10);
			Ally(12, 14);
			Ally(14,  2);
			Ally(14,  4);
			Ally(14,  6);
			Ally(14,  8);
			Ally(14, 10);
			Ally(14, 12);
			break;

		// Quintet
		case 25: // 1+2+3+4+5 vs 6+7+8+9+10
			Ally( 1,  2);
			Ally( 1,  3);
			Ally( 1,  4);
			Ally( 1,  5);
			Ally( 2,  1);
			Ally( 2,  3);
			Ally( 2,  4);
			Ally( 2,  5);
			Ally( 3,  1);
			Ally( 3,  2);
			Ally( 3,  4);
			Ally( 3,  5);
			Ally( 4,  1);
			Ally( 4,  2);
			Ally( 4,  3);
			Ally( 4,  5);
			Ally( 5,  1);
			Ally( 5,  2);
			Ally( 5,  3);
			Ally( 5,  4);
			Ally( 6,  7);
			Ally( 6,  8);
			Ally( 6,  9);
			Ally( 6, 10);
			Ally( 7,  6);
			Ally( 7,  8);
			Ally( 7,  9);
			Ally( 7, 10);
			Ally( 8,  6);
			Ally( 8,  7);
			Ally( 8,  9);
			Ally( 8, 10);
			Ally( 9,  6);
			Ally( 9,  7);
			Ally( 9,  8);
			Ally( 9, 10);
			Ally(10,  6);
			Ally(10,  7);
			Ally(10,  8);
			Ally(10,  9);
			break;

		case 26: // 1+3+5+7+9 vs 2+4+6+8+10
			Ally( 1,  3);
			Ally( 1,  5);
			Ally( 1,  7);
			Ally( 1,  9);
			Ally( 3,  1);
			Ally( 3,  5);
			Ally( 3,  7);
			Ally( 3,  9);
			Ally( 5,  1);
			Ally( 5,  3);
			Ally( 5,  7);
			Ally( 5,  9);
			Ally( 7,  1);
			Ally( 7,  3);
			Ally( 7,  5);
			Ally( 7,  9);
			Ally( 9,  1);
			Ally( 9,  3);
			Ally( 9,  5);
			Ally( 9,  7);
			Ally( 2,  4);
			Ally( 2,  6);
			Ally( 2,  8);
			Ally( 2, 10);
			Ally( 4,  2);
			Ally( 4,  6);
			Ally( 4,  8);
			Ally( 4, 10);
			Ally( 6,  2);
			Ally( 6,  4);
			Ally( 6,  8);
			Ally( 6, 10);
			Ally( 8,  2);
			Ally( 8,  4);
			Ally( 8,  6);
			Ally( 8, 10);
			Ally(10,  2);
			Ally(10,  4);
			Ally(10,  6);
			Ally(10,  8);
			break;

		case 0:
		default:
			m_Alliances = 0; // clear this in case it wasn't in 0..3 or 11..26
		}
	} // Teamplay is off
	*/

	m_RecyInvulnerabilityTime = GetVarItemInt("network.session.ivar25");
	m_RecyInvulnerabilityTime *= 60 * m_GameTPS; // convert from minutes to ticks (10 ticks per second)

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
		m_ElapsedGameTime = 0;
		if(!m_RemainingGameTime)
			m_RemainingGameTime = m_TotalGameTime * 60 * m_GameTPS; // convert minutes to 1/10 seconds
	}

	// And build the local player for the server
	int LocalTeamNum=GetLocalPlayerTeamNumber(); // Query this from game
	PlayerH=SetupPlayer(LocalTeamNum);
	SetAsUser(PlayerH, LocalTeamNum);
	AddPilotByHandle(PlayerH);
#endif // #ifndef EDITOR
}

// Called from Execute, m_GameTPS of a second has elapsed. Update everything.
void Strategy02::UpdateGameTime(void)
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

// Old code. -GBD
/*
// Tests if an allied team (A1, A2) has won over the other allied
// team (B1, B2) Sets gameover if true.
void Strategy02::TestAlliedGameover(int A1, int A2, int B1, int B2, bool *TeamIsFunctioning)
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
*/

// If Recycler invulnerability is on, then does the job of it.
void Strategy02::ExecuteRecyInvulnerability(void)
{
	// No need to do anything more...
	if((m_GameOver) || (m_RecyInvulnerabilityTime == 0))
		return;

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

	// Print periodic message about recycler invulnerability time left
	if((m_RecyInvulnerabilityTime == 1) ||
		((m_RecyInvulnerabilityTime % (60 * m_GameTPS)) == 0) ||
		((m_RecyInvulnerabilityTime < (60 * m_GameTPS)) && ((m_RecyInvulnerabilityTime % (15 * m_GameTPS)) == 0)))
	{

		if(m_RecyInvulnerabilityTime == 1)
		{
			sprintf_s(TempMsgString, "Recyclers are now vulnerable. Happy hunting.");
		}
		else if(m_RecyInvulnerabilityTime > (60 * m_GameTPS))
		{
			sprintf_s(TempMsgString, "Recycler invulnerability expires in %d minutes", m_RecyInvulnerabilityTime / (60 * m_GameTPS));
		}
		else
		{
			sprintf_s(TempMsgString, "Recycler invulnerability expires in %d seconds", m_RecyInvulnerabilityTime / m_GameTPS);
		}

		AddToMessagesBox2(TempMsgString, RGB(255, 0, 255)); // bright purple (ARGB)
	}

	// Reduce timer
	--m_RecyInvulnerabilityTime;
}


// New Alliance TeamWinning Code. -GBD
void Strategy02::ExecuteCheckIfGameOver(void)
{
#ifndef EDITOR
	// No need to do anything more...
	if((m_GameOver) || (m_ElapsedGameTime < (m_GameTPS)))
		return;

	// Check for a gameover by no recycler & factory
	int i, NumFunctioningTeams=0;
	bool TeamIsFunctioning[MAX_TEAMS];
	bool AlliesFunctioning[MAX_TEAMS]; // Ally Functioning Flag. -GBD

	memset(TeamIsFunctioning, 0, sizeof(TeamIsFunctioning));
	memset(AlliesFunctioning, 0, sizeof(AlliesFunctioning)); // Memset for ally flag. -GBD

	for(i=0;i<MAX_TEAMS;i++)
	{
		if(m_TeamIsSetUp[i])
		{
			bool Functioning=false; // Assume so for now.
			//AlliesFunctioning[m_AllyTeams[i]] = false; // Assume so for now. -GBD
			
			// Check if recycler vehicle still exists. Side effect to
			// note: IsAliveAndPilot zeroes the handle if pilot missing;
			// that'd be bad for us here if we want to manually remove
			// it. Thus, we have a sacrificial copy of it the game can
			// obliterate w/o hurting anything.
			Handle TempH = m_RecyclerHandles[i];

			// Added to allow recycler upgrading? -GBD
			if(!IsAround(TempH))
				TempH = GetObjectByTeamSlot(i, DLL_TEAM_SLOT_RECYCLER);

			// Use the side effect of IsAlive as well, hence the TempH == 0 -GBD
            if((!TempH) || (!IsAlive(TempH)))
				m_RecyclerHandles[i]=0; // Clear this out for later
			else
				Functioning=true;

			// Set this here. -GBD
			Handle RecyH = 0;

			// Check vehicle if it is around, else, check the buuilding -GBD
			//if(IsAround(TempH)) // This could let you drive it. :) -GBD
			if((IsAlive(TempH)) || (TempH!=0))
				RecyH = TempH;
			else
			{
				RecyH = GetObjectByTeamSlot(i, DLL_TEAM_SLOT_RECYCLER);
				//m_RecyclerHandles[i]=0; // Clear this out for later
			}

			if(!IsAround(RecyH)) // Uh oh, no Recycler? Look for Factory. -GBD
				RecyH = GetObjectByTeamSlot(i, DLL_TEAM_SLOT_FACTORY);

			if(RecyH)
				Functioning=true;
			//else
			//	AlliesFunctioning[m_AllyTeams[i]] = false; // If Allied team is dead, flag as so -GBD

			AlliesFunctioning[m_AllyTeams[i]] = AlliesFunctioning[m_AllyTeams[i]] || Functioning;
			TeamIsFunctioning[i] = Functioning;


			// Moved Respawn check here to look for allies too. -GBD
			// Note deployed location first time it deploys, also every 25.6 seconds
			if((!m_NotedRecyclerLocation[i]) || (!(m_ElapsedGameTime & 0xFF)))
			{
				// Grab out allie's Recy for respawn placement if ours is dead.
				if(!RecyH) // Uh oh, your recy+factory are dead. Look for ones on allied teams.
				{
					for(int x=1;x<MAX_TEAMS;x++) // Loop through all teams. 
					{
						// If the team is set, and it's not the same team as the first loop, and they're allied, then we found one. 
						if((i != x) && (IsTeamAllied(i, x)))
						{
							RecyH = GetObjectByTeamSlot(x, DLL_TEAM_SLOT_RECYCLER);
							if(!RecyH) // Uh oh, no Recycler? Look for Factory.
								RecyH = GetObjectByTeamSlot(x, DLL_TEAM_SLOT_FACTORY);

							if(RecyH)
								break; // We found one, and they're alive. Abort X loop early. 
						}
					}
				}
				

				if(RecyH) // The above functions found something...-GBD
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

	if(!IsTeamplayOn())
	{
		for(i=0;i<MAX_TEAMS;i++)
		{
			if(AlliesFunctioning[i])
				NumFunctioningTeams++;
		}
	}

			// If this team has allies, check to see if any of them are functioning. -GBD
			if(m_HasAllies[i]) // If this team had allies at one point, find allied teams.
			{
				for(int x=1;x<MAX_TEAMS;x++) // Loop through all teams. 
				{
					// If the team is set, and it's not the same team as the first loop, and they're allied, then we found one. 
					if((i != x) && (IsTeamAllied(i, x)))
					{
						if(TeamIsFunctioning[x]) // IF they're functioning, flag the team as allied functionoing.
						{
							AlliesFunctioning[m_AllyTeams[i]] = true; // Flag this internal loop flag.
							break; //x = 16; // We found one, and they're alive. Abort X loop early. 
						}
					}
				} // Done looping over team X
			}
			// If THIS team is functioning, OR Their Allies are functioning. -GBD
			if(m_HasAllies[i]) // If this team had allies at one point.
			{
				if((Functioning) && (!AlliesFunctioning[m_AllyTeams[i]]))
				{
					TeamIsFunctioning[i]=true;
					NumFunctioningTeams++;
					AlliesFunctioning[m_AllyTeams[i]] = true;
				} // You OR Your Allies Have recyclerV, recyclerB or FactoryB
			}// End FFA Play
			else if(Functioning)
			{
				TeamIsFunctioning[i]=true;
				NumFunctioningTeams++;
			} // Has recyclerV, recyclerB or FactoryB
		} // Team is set up
	} // loop over functioning teams

	// Keep track if we ever had several teams playing. Don't need
	// to check for gameover if so-- 
	if(NumFunctioningTeams>1)
	{
		m_HadMultipleFunctioningTeams=true;
		return; // Exit function early
	}

	// Easy Gameover case: nobody's got a functioning base. End everything now.
#ifndef SERVER
	if((NumFunctioningTeams==0) && (m_ElapsedGameTime > (5 * m_GameTPS)))
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
				{
					if(TeamIsFunctioning[i])
					{
						if(WinningTeamgroup == -1)
						{
							WinningTeamgroup = WhichTeamGroup(i);
							NoteGameoverByLastTeamWithBase(WinningTeamgroup);
						}
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
			else // Non-teamplay, report individual winner
			{
				// With alliances, we may not have a winner unless the team
				// remaining isn't allied (or we would have caught it above)
	
				for(i=0;i<MAX_TEAMS;i++) // Start at 0. Also give a more proper message if allies are on. -GBD
				{
					if(AlliesFunctioning[m_AllyTeams[i]]) //if(TeamIsFunctioning[i])
					{
						if(m_HasAllies[i])
						{
							sprintf_s(TempMsgString, "Team %d has won, killed all enemies.", m_AllyTeams[i]); // Tell who won. -GBD
							NoteGameoverWithCustomMessage(TempMsgString); // Custom message ftw. -GBD
							for(int j=0;j<MAX_TEAMS;j++) // Give score to allies. -GBD
							{
								if((i != j) && (IsTeamAllied(i, j)))
									AddScore(GetPlayerHandle(j), ScoreForWinning);
							}
						}
						else
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

void Strategy02::Execute(void)
{
	if (!m_DidInit)
	{
		Init();
	}
#ifndef EDITOR

	// If Recycler invulnerability is on, then does the job of it.
	ExecuteRecyInvulnerability();

	// Check for absence of recycler & factory, gameover if so.
	ExecuteCheckIfGameOver();

	// Do this as well...
	UpdateGameTime();

// If FFA mode is on, run this. -GBD
	if(!IsTeamplayOn())
		ScavengerManagementCode(); // Scavenger Management, yay.

#endif // #ifndef EDITOR
}

bool Strategy02::AddPlayer(DPID id, int Team, bool IsNewPlayer)
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

void Strategy02::DeletePlayer(DPID id)
{
}

// Rebuilds pilot
EjectKillRetCodes Strategy02::RespawnPilot(Handle DeadObjectHandle, int Team)
{
#ifdef EDITOR
	return DLLHandled;
#else
	Vector spawnpointPosition;

	// Only use safest place if invalid team #
	if((Team<1) || (Team>=MAX_TEAMS))
	{
		spawnpointPosition = GetSafestSpawnpoint();
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
EjectKillRetCodes Strategy02::DeadObject(int DeadObjectHandle, int KillersHandle, bool isDeadPerson, bool isDeadAI)
{
#ifdef EDITOR
	return DLLHandled;
#else
	// Get team number of the dead object
	const int deadObjectTeam = GetTeamNum(DeadObjectHandle);

	// An object is a player if it's a local or remotely-controlled player
	// (i.e. not AI)
	const bool deadObjectIsPlayer = IsPlayer(DeadObjectHandle);
	const bool killerObjectIsPlayer = IsPlayer(KillersHandle);

	const TEAMRELATIONSHIP relationship = GetTeamRelationship(DeadObjectHandle, KillersHandle);

	const int deadObjectScrapCost = GetActualScrapCost(DeadObjectHandle);

	// If the dead unit is directly controlled by a player (i.e. not
	// one of their AI units)
	if(deadObjectIsPlayer)
	{
		// Score goes down by the scrap cost of unit that died
		AddScore(DeadObjectHandle, -deadObjectScrapCost);

		// Scoring: One death for players if they die as a pilot.
		if(isDeadPerson)
		{
			AddDeaths(DeadObjectHandle, 1);
		}
	}
	else
	{
		if(m_KillForAIKill)
			AddDeaths(DeadObjectHandle, 1);
		if(m_PointsForAIKill)
			AddScore(DeadObjectHandle, -deadObjectScrapCost);
	}

	// If the killer was a human (directly, not via their AI units), then
	// they get a kill and some score points.
	if(killerObjectIsPlayer)
	{
		if((relationship == TEAMRELATIONSHIP_SAMETEAM) || 
		   (relationship == TEAMRELATIONSHIP_ALLIEDTEAM))
		{
			// Being a jerk to same or allied team loses a kill
			AddKills(KillersHandle, -1);
			// And killer loses score
			AddScore(KillersHandle, -deadObjectScrapCost);
		}
		else
		{
			AddKills(KillersHandle, 1);
			// And, bump their score by the scrap cost of what they just killed
			AddScore(KillersHandle, deadObjectScrapCost);
		}
	}
	else
	{
		if((relationship == TEAMRELATIONSHIP_SAMETEAM) || 
		   (relationship == TEAMRELATIONSHIP_ALLIEDTEAM))
		{
			if(m_KillForAIKill)
				AddKills(KillersHandle, -1);
			if(m_PointsForAIKill)
				AddScore(KillersHandle, -deadObjectScrapCost);
		}
		else
		{
			if(m_KillForAIKill)
				AddKills(KillersHandle, 1);
			if(m_PointsForAIKill)
				AddScore(KillersHandle, deadObjectScrapCost);
		}
	}

	// Was this a spawn kill?
	// Spawnkill is a non-suicide, on a human by another human.  Added
	// check for !isDeadAI NM 11/25/06 - APC soldiers dying around the
	// same time as their human player would trip this up.
	const int spawnKillTime = MaxSpawnKillTime * m_GameTPS; // 15 seconds
	const bool isSpawnKill = (DeadObjectHandle != KillersHandle) && 
		(!isDeadAI) &&
		(deadObjectTeam > 0) && (deadObjectTeam < MAX_TEAMS) && 
		(m_SpawnedAtTime[deadObjectTeam] > 0) &&
		((m_ElapsedGameTime - m_SpawnedAtTime[deadObjectTeam]) < spawnKillTime);

	// If this was a spawnkill, register that on the killers score
	if(isSpawnKill)
	{
		sprintf_s(TempMsgString, "Spawn kill by %s on %s\n", 
				  GetPlayerName(KillersHandle), GetPlayerName(DeadObjectHandle));
		AddToMessagesBox(TempMsgString);
		AddScore(KillersHandle, -ScoreDecrementForSpawnKill);
	}

	// Check to see if we have a m_KillLimit winner
	if((m_KillLimit) && (GetKills(KillersHandle) >= m_KillLimit))
	{
		NoteGameoverByKillLimit(KillersHandle);
		DoGameover(10.0f);
	}

	if(deadObjectTeam == 0)
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
			return RespawnPilot(DeadObjectHandle, deadObjectTeam);
		else 
			return DoEjectPilot;
	}
#endif // #ifndef EDITOR
}

EjectKillRetCodes Strategy02::PlayerEjected(Handle DeadObjectHandle)
{
#ifdef EDITOR
	return DLLHandled;
#else
	int deadObjectTeam=GetTeamNum(DeadObjectHandle);
	if(deadObjectTeam==0)
		return DLLHandled; // Invalid team. Do nothing

	// Tweaked scoring - if a player bails out, no deaths/kills are
	// registered.  But, their score should go down by the scrap cost
	// of the vehicle they just left.
	if(IsPlayer(DeadObjectHandle))
	{
		AddScore(DeadObjectHandle, -GetActualScrapCost(DeadObjectHandle));
	}

	return DoEjectPilot; // Tell main code to allow the ejection
#endif // #ifndef EDITOR
}

EjectKillRetCodes Strategy02::ObjectKilled(int DeadObjectHandle, int KillersHandle)
{
#ifdef EDITOR
	return DLLHandled;
#else
	const bool isDeadAI = !IsPlayer(DeadObjectHandle);
	const bool isDeadPerson = IsPerson(DeadObjectHandle);

	// Sanity check for multiworld
	if(GetCurWorld() != 0)
		return DoEjectPilot;

	const int deadObjectTeam = GetTeamNum(DeadObjectHandle);
	if(deadObjectTeam == 0)
		return DoEjectPilot; // Someone on neutral team always gets default behavior

	// If a person died, respawn them, etc
	return DeadObject(DeadObjectHandle, KillersHandle, isDeadPerson, isDeadAI);
#endif // #ifndef EDITOR
}


EjectKillRetCodes Strategy02::ObjectSniped(int DeadObjectHandle, int KillersHandle)
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

bool Strategy02::Load(bool missionSave)
{
	EnableHighTPS(m_GameTPS);
	SetAutoGroupUnits(false);

	// Make sure we always call this
	StartingVehicleManager::Load(missionSave);

	// We're a 1.3 DLL.
	WantBotKillMessages();

	// Do this for everyone as well.
	ClearObjectives();
	AddObjective("mpobjective_st.otf", WHITE, -1.0f); // negative time means don't change display to show it

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

	// Load for FFA Thug helpers. -GBD
	int size;
	Read(&size, 1);
	ThugScavengerList.resize(size);
	if(size)
		Read(&ThugScavengerList.front(), sizeof(ThugScavengerClass)*size);

	Read(&size, 1);
	ThugPilotList.resize(size);
	if(size)
		Read(&ThugPilotList.front(), sizeof(ThugPilotClass)*size);

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

bool Strategy02::PostLoad(bool missionSave)
{
	if (missionSave)
		return true;

	// PostLoad for FFA Thug helpers. -GBD
	for (std::vector<ThugScavengerClass>::iterator iter = ThugScavengerList.begin(); iter != ThugScavengerList.end(); ++iter)
	{
		ThugScavengerClass &sao = *iter;
		ConvertHandles(&sao.ThugScavengerObject, 1);
	}
	for (std::vector<ThugPilotClass>::iterator iter = ThugPilotList.begin(); iter != ThugPilotList.end(); ++iter)
	{
		ThugPilotClass &sao = *iter;
		ConvertHandles(&sao.ThugPilotObject, 1);
	}

	bool ret = true;

	ConvertHandles(h_array, h_count);

	return ret;
}

bool Strategy02::Save(bool missionSave)
{
	// Make sure we always call this
	StartingVehicleManager::Save(missionSave);

	if (missionSave)
		return true;

	// Save for FFA Thug helpers. -GBD
	int size = int(ThugScavengerList.size());
	Write(&size, 1);
	if(size)
		Write(&ThugScavengerList.front(), sizeof(ThugScavengerClass)*size);

	size = int(ThugPilotList.size());
	Write(&size, 1);
	if(size)
		Write(&ThugPilotList.front(), sizeof(ThugPilotClass)*size);

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
PreSnipeReturnCodes Strategy02::PreSnipe(const int curWorld, Handle shooterHandle, Handle victimHandle, int ordnanceTeam, char* pOrdnanceODF)
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
PreGetInReturnCodes Strategy02::PreGetIn(const int curWorld, Handle pilotHandle, Handle emptyCraftHandle)
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
	return new Strategy02;
}

// Thug Scavenger Management code. -GBD
// This code micro manages FFA Alliance Thug's Scavengers, and pilots, to ensure they get treated properly.
// Scavenger. Handles swapping Team from Thug to Commander when it deploys into a pool.
void Strategy02::ScavengerManagementCode(void)
{
	ThugScavengerList.erase(std::remove_if(ThugScavengerList.begin(), ThugScavengerList.end(), ShouldRemoveThugScavengerObject), ThugScavengerList.end());

	for (std::vector<ThugScavengerClass>::iterator iter = ThugScavengerList.begin(); iter != ThugScavengerList.end(); ++iter)
	{
		int Team = GetTeamNum(iter->ThugScavengerObject);
		//iter->ScavCurrScrap = GetScavengerCurScrap(iter->ThugScavengerObject);
		// First off, is this a commander's team?
		if ((!m_ThugFlag[Team]) && (iter->ScavengerTeam != Team))
		{
			iter->ScavengerTeam = Team;

			//	sprintf_s(TempMsgString, "Set Scavenger on Team: %d Index: %d as Commadner Team", Team, iter-ThugScavengerList.begin());
			//	PrintConsoleMessage(TempMsgString);
		}
		else // Thug team, handle scrap swapping.
		{
			int ScavScrap = GetScavengerCurScrap(iter->ThugScavengerObject);

			if (ScavScrap > 0)
			{
				int CurrScrap = GetScrap(iter->ScavengerTeam);
				int MaxScrap = GetMaxScrap(iter->ScavengerTeam);

				//	if(Logging)
				//		FormatConsoleMessage("Scrap Info for Scavenger on Team: %d Index: %d: ScavScrap: %d CurrScrap: %d MaxScrap: %d", Team, iter-ThugScavengerList.begin(), ScavScrap, CurrScrap, MaxScrap); 

				if ((CurrScrap + ScavScrap) > MaxScrap)
				{
					int TempCurrScrap = (MaxScrap - CurrScrap);
					int TempScavScrap = (ScavScrap - TempCurrScrap);

					AddScrap(iter->ScavengerTeam, TempCurrScrap);
					SetScavengerCurScrap(iter->ThugScavengerObject, TempScavScrap);

					//	sprintf_s(TempMsgString, "Scav Moving Scrap %d from Team: %d to Team: %d, Scrap: %d, %d", TempCurrScrap, Team, iter->ScavengerTeam, GetScrap(Team), GetScrap(iter->ScavengerTeam));
					//	PrintConsoleMessage(TempMsgString);
				}
				else
				{
					AddScrap(iter->ScavengerTeam, ScavScrap);
					SetScavengerCurScrap(iter->ThugScavengerObject, 0);
					//iter->ScavCurrScrap = 0; // Clear this too.

					//	sprintf_s(TempMsgString, "Scav Moving Scrap %d from Team: %d to Team: %d, Scrap: %d, %d", ScavScrap, Team, iter->ScavengerTeam, GetScrap(Team), GetScrap(iter->ScavengerTeam));
					//	PrintConsoleMessage(TempMsgString);
				}
			}

			if ((Team != iter->ScavengerTeam) && (IsDeployed(iter->ThugScavengerObject)))
				SetTeamNum(iter->ThugScavengerObject, iter->ScavengerTeam);
		}
	}

	ThugPilotList.erase(std::remove_if(ThugPilotList.begin(), ThugPilotList.end(), ShouldRemoveThugPilotObject), ThugPilotList.end());

	// Loop over Thug Pilots.
	for (std::vector<ThugPilotClass>::iterator iter = ThugPilotList.begin(); iter != ThugPilotList.end(); ++iter)
	{
		int Team = GetTeamNum(iter->ThugPilotObject);

		if ((m_ThugFlag[Team]) && (iter->SwitchTickTimer <= 0))
		{
			int CommanderTeam = 1;
			for (int x = 1; x<MAX_TEAMS; x++)
			{
				if ((m_AllyTeams[Team] == m_AllyTeams[x]) && (!m_ThugFlag[x]))
					CommanderTeam = x;
			}

			if (Team != CommanderTeam)
				SetTeamNum(iter->ThugPilotObject, CommanderTeam);
		}
		--iter->SwitchTickTimer;
	}
}