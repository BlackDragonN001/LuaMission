#include <math.h>
#include <string.h>

#include "..\Shared\DLLBase.h"
#include "..\Shared\MPVehicles.h"
#include "..\Shared\SubMission.h"
#include "..\Shared\Taunts.h"
#include "..\Shared\StartingVehicles.h"
#include "..\Shared\DLLUtils.h"

#include "StratInstant.h"

extern bool CheckedSVar3;


#define VEHICLE_SPACING_DISTANCE (20.0f)
#define MAX_TEAMS 16
// # of multiple-user teams are possible in Teamplay
#define MAX_MULTIPLAYER_TEAMS 2

// ---------- Scoring Values-- these are delta scores, added to current score --------
const int ScoreForKillingCraft = 5; // User-piloted craft only
const int ScoreForKillingPerson = 10;
const int ScoreForDyingAsCraft = -2;
const int ScoreForDyingAsPerson = -5;
const int ScoreForWinning = 100;
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

// How long a "spawn" kill lasts, in tenth of second ticks. If the
// time since they were spawned to current is less than this, it's a
// spawn kill, and not counted. Todo - Make this an ivar for dm?
#ifdef _DEBUG
const int MaxSpawnKillTime=300; // long time, so we can debug this
#else
const int MaxSpawnKillTime=15; // 15 seconds
#endif

// How much vertical displacement is tolerated from the center's position
const float MaxVerticalDisplacement=10.0f;
// How many times to try and find a suitable location
const int MaxRetries=32;
const float PI=3.141592654f;

// Temporary strings for blasting stuff into for output. NOT to be
// used to store anything you care about.
char TempMsgString[1024];

// Temporary name for blasting ODF names into while building
// them. *not* saved, do *not* count on its contents being valid
// next time the dll is called.
static char TempODFName[64];

// static FILE *stratLog = NULL;

static SubMission *subMission = NULL;


// Don't do much at class creation; do in InitialSetup()
StratInstant::StratInstant(void)
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

}


StratInstant::~StratInstant()
{
	delete subMission;
	subMission = NULL;
}


char* StratInstant::GetNextRandomVehicleODF(int Team)
{
	return GetPlayerODF(Team);
}


void StratInstant::AddObject(Handle h)
{
	// All work handled by this function.
	subMission->AddObject(h, m_CreatingStartingVehicles, m_RecyclerHandles);
}


void StratInstant::DeleteObject(Handle h)
{
	subMission->DeleteObject(h);
}


// Gets the initial player vehicle as selected in the shell
char *StratInstant::GetInitialPlayerVehicleODF(int Team)
{
	return GetPlayerODF(Team);
}


// Given a race identifier, get the pilot back (used during a respawn)
char *StratInstant::GetInitialPlayerPilotODF(char Race)
{
	if(m_RespawnWithSniper)
		strcpy_s(TempODFName, "ispilo"); // With sniper.
	else
		strcpy_s(TempODFName, "isuser_m"); // Note-- this is the sniper-less variant for a respawn
	TempODFName[0]=Race;
	return TempODFName;
}


// Given a race identifier, get the recycler ODF back
char *StratInstant::GetInitialRecyclerODF(char Race)
{
	const char* pContents = DLLUtils::GetCheckedNetworkSvar(5, NETLIST_Recyclers);
	if((pContents != NULL) && (pContents[0] != '\0'))
	{
		strncpy_s(TempODFName, pContents, sizeof(TempODFName)-1);
	}
	else
	{
		strcpy_s(TempODFName, "ivrecy_m");
	}

	TempODFName[0]=Race;
	return TempODFName;
}


// Given a race identifier, get the scavenger ODF back
char *StratInstant::GetInitialScavengerODF(char Race)
{
	strcpy_s(TempODFName, "ivscav");
	TempODFName[0]=Race;
	return TempODFName;
}


// Given a race identifier, get the constructor ODF back
char *StratInstant::GetInitialConstructorODF(char Race)
{
	strcpy_s(TempODFName, "ivcons");
	TempODFName[0]=Race;
	return TempODFName;
}


// Given a race identifier, get the healer (repairbot) ODF back
char *StratInstant::GetInitialHealerODF(char Race)
{
	strcpy_s(TempODFName, "ivserv");
	TempODFName[0]=Race;
	return TempODFName;
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
void StratInstant::SetupTeam(int Team)
{
#ifndef EDITOR
	if((Team<1) || (Team>=MAX_TEAMS))
		return;
	if((IsTeamplayOn()) && (m_TeamIsSetUp[Team]))
		return;

	if(Team == 1)
		DoTaunt(TAUNTS_GameStart);

	char TeamRace=GetRaceOfTeam(Team);
	if(IsTeamplayOn())
		SetMPTeamRace(WhichTeamGroup(Team), TeamRace); // Lock this down to prevent changes.

	Vector Where=GetRandomSpawnpoint(Team);
	// Store position we created them at for later
	m_TeamPos[3*Team+0]=Where.x;
	m_TeamPos[3*Team+1]=Where.y;
	m_TeamPos[3*Team+2]=Where.z;

	// Build recycler some distance away, if it's not preplaced on the map.
	if(GetObjectByTeamSlot(Team, DLL_TEAM_SLOT_RECYCLER) == 0)
	{
		Where=GetPositionNear(Where, VEHICLE_SPACING_DISTANCE, 2*VEHICLE_SPACING_DISTANCE);
		int VehicleH;
		VehicleH=BuildObject(GetInitialRecyclerODF(TeamRace), Team, Where);
		SetRandomHeadingAngle(VehicleH);
		m_RecyclerHandles[Team]=VehicleH;
		SetGroup(VehicleH, 0);
	}

	// Build all optional vehicles for this team.
	Where.x=m_TeamPos[3*Team+0]; // restore default after we modified this for recy above
	Where.y=m_TeamPos[3*Team+1];
	Where.z=m_TeamPos[3*Team+2];
	// Drop skill level while we build things.
	m_CreatingStartingVehicles=true;
	StartingVehicleManager::CreateVehicles(Team, TeamRace, m_StartingVehiclesMask, Where);
	m_CreatingStartingVehicles=false;

	SetScrap(Team, 40);

	if(IsTeamplayOn()) 
		for(int i=GetFirstAlliedTeam(Team);i<=GetLastAlliedTeam(Team);i++) 
			if(i!=Team)
			{
				// Get a new position near the team's central position
				Vector NewPosition=GetPositionNear(Where, AllyMinRadiusAway, AllyMaxRadiusAway);

				// In teamplay, store where offense players were created for respawns later
				m_TeamPos[3*i+0]=NewPosition.x;
				m_TeamPos[3*i+1]=NewPosition.y;
				m_TeamPos[3*i+2]=NewPosition.z;
			} // Loop over allies not the commander

	m_TeamIsSetUp[Team]=true;
#endif // #ifndef EDITOR
}

// Given an index into the Player list, build everything for a given
// single player (i.e. a vehicle of some sorts), and set up the team
// as well as necessary
Handle StratInstant::SetupPlayer(int Team)
{
	Handle PlayerH = 0;
#ifndef EDITOR
	Vector Where;

	if((Team<0) || (Team>=MAX_TEAMS))
		return 0; // Sanity check... do NOT proceed

	m_SpawnedAtTime[Team] = m_ElapsedGameTime; // Note when they spawned in.

	int TeamBlock=WhichTeamGroup(Team);

	if((!IsTeamplayOn()) || (TeamBlock<0))
	{
		// This player is their own commander; set up their equipment.
		SetupTeam(Team);

		// Now put player near his recycler
		Where.x = m_TeamPos[3*Team+0];
		Where.z = m_TeamPos[3*Team+2];
		Where.y = TerrainFindFloor(Where.x, Where.z) + 1.0f;
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
		Where.x = m_TeamPos[3*Team+0];
		Where.z = m_TeamPos[3*Team+2];
		Where.y = TerrainFindFloor(Where.x, Where.z) + 1.0f;
	} // Teamplay setup

	PlayerH=BuildObject(GetPlayerODF(Team), Team, Where);
	SetRandomHeadingAngle(PlayerH);

	// If on team 0 (dedicated server team), make this object gone from the world
	if(!Team)
		MakeInert(PlayerH);

#endif // #ifndef EDITOR
	return PlayerH;
}


void StratInstant::Init(void)
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

	m_DidInit=true;

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
	m_KillLimit = GetVarItemInt("network.session.ivar0");
	m_TotalGameTime = GetVarItemInt("network.session.ivar1");

	m_StartingVehiclesMask = GetVarItemInt("network.session.ivar7");
	m_PointsForAIKill = (GetVarItemInt("network.session.ivar14") != 0);
	m_KillForAIKill = (GetVarItemInt("network.session.ivar15") != 0);
	m_RespawnWithSniper = (GetVarItemInt("network.session.ivar16") != 0);
	m_bIsFriendlyFireOn = (GetVarItemInt("network.session.ivar32") != 0);

	// The BZN has a player in the world. We need to delete them, as
	// this code (either on this machine or remote machines) handles
	// creation of the proper vehicles in the right places for
	// everyone.
	PlayerEntryH = GetPlayerHandle();
	if(PlayerEntryH) 
		RemoveObject(PlayerEntryH);

	// Do all the one-time server side init of varbs. These varbs are
	// saved out and read in on clientside, if saved in the proper
	// place above. This needs to be done after toasting the initial
	// vehicle
	if((ImServer()) || (!IsNetworkOn()))
	{
		m_ElapsedGameTime = 0;
		m_LastTauntPrintedAt=-2000; // Force first message to be printed...
		// Let the taunt system know where to find things it uses.
		InitTaunts(&m_ElapsedGameTime, &m_LastTauntPrintedAt, &m_GameTPS);

		if(!m_RemainingGameTime)
			m_RemainingGameTime = m_TotalGameTime * 60 * m_GameTPS; // convert minutes to 1/10 seconds

	} // Server or no network

	// And build the local player for the server
	int LocalTeamNum=GetLocalPlayerTeamNumber(); // Query this from game
	PlayerH=SetupPlayer(LocalTeamNum);
	SetAsUser(PlayerH, LocalTeamNum);
	AddPilotByHandle(PlayerH);
#endif // #ifndef EDITOR
}


// Called via execute, 1/10 of a second has elapsed. Update everything.
void StratInstant::UpdateGameTime(void)
{
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
	else 
	{
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
}


// Check for absence of recycler & factory, gameover if so.
void StratInstant::ExecuteCheckIfGameOver(void)
{
	// Exit now!...
	if(m_GameOver)
		return;

	const float endDelta = 10.0f;

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
			// it. Thus, we have a sacrificial varb
			Handle TempH=m_RecyclerHandles[i];					
			if((!IsAlive(TempH)) || (TempH==0))
				m_RecyclerHandles[i]=0; // Clear this out for later
			else Functioning=true; 

			// Check buildings as well.
			Handle RecyH = GetObjectByTeamSlot(i, DLL_TEAM_SLOT_RECYCLER);
			if(RecyH)
			{
				Functioning=true;

				// Note deployed location first time it deploys, also
				// every 25.6 seconds. Note - this'll be less than
				// 25.6 seconds if m_GameTPS is > 10, but since this
				// code is so lightweight, and executed in sync on all
				// machines -- m_GameTPS should be set by the exe --
				// then I don't mind not taking m_GameTPS into account
				// here
				if((!m_NotedRecyclerLocation[i]) || (!(m_ElapsedGameTime & 0xFF)))
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
	} // loop over all teams


	// Keep track if we ever had several teams playing. Don't need
	// to check for gameover if so-- 
	if(NumFunctioningTeams > 1)
	{
		m_HadMultipleFunctioningTeams=true;
		return; // Exit function early
	}

	// Easy Gameover case: nobody's got a functioning base. End
	// everything now.  Note - this check is done after the first 1/2
	// of a second of gameplay elapses, just to let everything
	// be spawned in, etc.
#ifndef SERVER
	if((NumFunctioningTeams == 0) && (m_ElapsedGameTime > (m_GameTPS / 2)))
	{
		NoteGameoverByNoBases();
		DoGameover(endDelta);
		m_GameOver=true;
	}
	else
#endif
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
						if(i == 1)
							DoTaunt(TAUNTS_CPURecyDestroyed);
						else if(i == 6)
							DoTaunt(TAUNTS_HumanRecyDestroyed);

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
			} // Teamplay is on
			else { // Non-teamplay, report individual winner
				NoteGameoverByLastWithBase(GetPlayerHandle(1));
				for(i=0;i<MAX_TEAMS;i++)
				{
					if(TeamIsFunctioning[i])
					{
						NoteGameoverByLastWithBase(i);
						AddScore(GetPlayerHandle(i), ScoreForWinning);
					} // Found winner team
				}
			} // Non-teamplay.

			DoGameover(endDelta);
			m_GameOver=true;
		} // Winner
}

void StratInstant::Execute(void)
{
	if (!m_DidInit)
	{
		Init();
	}

#ifndef EDITOR
	// Check for absence of recycler & factory, gameover if so.
	ExecuteCheckIfGameOver();

	// Do this as well...
	UpdateGameTime();

	subMission->Execute(m_TeamIsSetUp, m_RecyclerHandles);
#endif // #ifndef EDITOR
}

bool StratInstant::AddPlayer(DPID id, int Team, bool IsNewPlayer)
{
#ifndef EDITOR
	if (!m_DidInit)
	{
		Init();
	}
	else if (IsNewPlayer)
		DoTaunt(TAUNTS_NewHuman);

	if(IsNewPlayer)
	{
		Handle PlayerH=SetupPlayer(Team);
		SetAsUser(PlayerH, Team);
		AddPilotByHandle(PlayerH);
	}

#endif // #ifndef EDITOR
	return 1; // BOGUS: always assume successful
}


void StratInstant::DeletePlayer(DPID id)
{
	DoTaunt(TAUNTS_LeftHuman);
}


// Rebuilds pilot
EjectKillRetCodes StratInstant::RespawnPilot(Handle DeadObjectHandle, int Team)
{
	Vector Where;

	// Only use safest place if invalid team #
	if((Team<1) || (Team>=MAX_TEAMS))
	{
		Where=GetSafestSpawnpoint();
	}
	else
	{
		Where.x = m_TeamPos[3*Team+0];
		Where.y = m_TeamPos[3*Team+1];
		Where.z = m_TeamPos[3*Team+2];
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
		const float dx = OldPos.x - Where.x;
		const float dz = OldPos.z - Where.z;
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

	// Any humans not on the right team? Make life hell for them.
	const int Grp = WhichTeamGroup(Team);
	if(Grp != 0)
	{
		respawnHeight += 600.0f;
	}

	// Randomize starting position somewhat. This gives a range of +/-
	// RespawnDistanceAwayXZRange
	Where.x += (GetRandomFloat(1.f) - 0.5f) * (2.f * RespawnDistanceAwayXZRange);
	Where.z += (GetRandomFloat(1.f) - 0.5f) * (2.f * RespawnDistanceAwayXZRange);

	// Don't allow a spawn underground - just in case there's a cliff
	// near the starting Where position, we need to keep y above the
	// ground.
	{
		const float curFloor = TerrainFindFloor(Where.x, Where.z) + 2.5f;
		if(Where.y < curFloor)
		{
			Where.y = curFloor;
		}
	}
	Where.y += respawnHeight; // Bounce them in the air to prevent multi-kills
	Where.y += GetRandomFloat(1.f) * RespawnDistanceAwayYRange;

	Handle NewPerson = BuildObject(GetInitialPlayerPilotODF(GetRaceOfTeam(Team)), Team, Where);
	SetAsUser(NewPerson, Team);
	AddPilotByHandle(NewPerson);
	SetRandomHeadingAngle(NewPerson);

	// If on team 0 (dedicated server team), make this object gone from the world
	if(!Team)
	{
		MakeInert(NewPerson);
	}

	return DLLHandled; // Dead pilots get handled by DLL
}


EjectKillRetCodes StratInstant::DeadObject(int DeadObjectHandle, int KillersHandle, bool isDeadPerson, bool isDeadAI)
{
	// Unused - NM 2/10/07
	//	bool KillerIsAI=!IsPlayer(KillersHandle);

	// Get team number of who got waxed.
	const int deadTeam = GetTeamNum(DeadObjectHandle);
	bool isSpawnKill = false;
	// Flip scoring if this is a spawn kill.
	int SpawnKillMultiplier = 1;

	// Give positive or negative points to killer, depending on
	// whether they killed enemy or ally
	{
		const int spawnKillTime = MaxSpawnKillTime * m_GameTPS; // 15 seconds

		// Spawnkill is a non-suicide, on a human by another
		// human.  Added check for !isDeadAI NM 11/25/06 - APC
		// soldiers dying around the same time as their human
		// player would trip this up.
		isSpawnKill = (DeadObjectHandle != KillersHandle) && 
			(!isDeadAI) &&
			(deadTeam > 0) && (deadTeam < MAX_TEAMS) && 
			(m_SpawnedAtTime[deadTeam] > 0) &&
			((m_ElapsedGameTime - m_SpawnedAtTime[deadTeam]) < spawnKillTime);

		if(isSpawnKill)
		{
			SpawnKillMultiplier = -1;
			sprintf_s(TempMsgString, "Spawn kill by %s on %s\n", 
					GetPlayerName(KillersHandle), GetPlayerName(DeadObjectHandle));
			AddToMessagesBox(TempMsgString);
		}

		if((DeadObjectHandle != KillersHandle) && (!IsAlly(DeadObjectHandle, KillersHandle)))
		{
			// Killed enemy...
			if((!isDeadAI) || (m_KillForAIKill))
				AddKills(KillersHandle, SpawnKillMultiplier); // Give them a kill

			if((!isDeadAI) || (m_PointsForAIKill))
			{
				if(isDeadPerson)
				{
					AddScore(KillersHandle, ScoreForKillingPerson * SpawnKillMultiplier);
				}
				else
				{
					AddScore(KillersHandle, ScoreForKillingCraft * SpawnKillMultiplier);
				}
			}
		}
		else
		{
			if((!isDeadAI) || (m_KillForAIKill))
				AddKills(KillersHandle, -SpawnKillMultiplier); // Suicide or teamkill counts as -1 kill

			if((!isDeadAI) || (m_PointsForAIKill))
			{
				if(isDeadPerson)
				{
					AddScore(KillersHandle, -ScoreForKillingPerson * SpawnKillMultiplier);
				}
				else
				{
					AddScore(KillersHandle, -ScoreForKillingCraft * SpawnKillMultiplier);
				}
			}
		}
	}

	// Give points to killee-- this always increases
	AddDeaths(DeadObjectHandle, SpawnKillMultiplier);
	if(isDeadPerson)
	{
		AddScore(DeadObjectHandle, ScoreForDyingAsPerson * SpawnKillMultiplier);
	}
	else
	{
		AddScore(DeadObjectHandle, ScoreForDyingAsCraft * SpawnKillMultiplier);
	}

	// Check to see if we have a m_KillLimit winner
	if((m_KillLimit) && (GetKills(KillersHandle) >= m_KillLimit))
	{
		NoteGameoverByKillLimit(KillersHandle);
		DoGameover(10.0f);
	}

	// Get team number of who got waxed.
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
		if((deadTeam > 0) && (deadTeam < 6))
			DoTaunt(TAUNTS_HumanShipDestroyed);

		// If this was a dead pilot, we need to build another pilot back
		// at base. Otherwise, we just eject a pilot from the
		// craft. [This is strat, nobody gets a craft for free when they
		// lose one.]
		if(isDeadPerson)
			return RespawnPilot(DeadObjectHandle, deadTeam);
		else 
			return DoEjectPilot;
	}
}


EjectKillRetCodes StratInstant::PlayerEjected(Handle DeadObjectHandle)
{
	int deadTeam=GetTeamNum(DeadObjectHandle);
	if(deadTeam==0)
		return DLLHandled; // Invalid team. Do nothing

	// Update Deaths, Kills, Score for this player
	AddDeaths(DeadObjectHandle, 1);
	AddKills(DeadObjectHandle, -1);
	AddScore(DeadObjectHandle, ScoreForDyingAsCraft-ScoreForKillingCraft);

	return DoEjectPilot; // Tell main code to allow the ejection
}


EjectKillRetCodes StratInstant::ObjectKilled(int DeadObjectHandle, int KillersHandle)
{
	bool isDeadAI=!IsPlayer(DeadObjectHandle);

	bool isDeadPerson=IsPerson(DeadObjectHandle);

	// Sanity check for multiworld
	if(GetCurWorld() != 0)
		return DoEjectPilot;

	int deadTeam=GetTeamNum(DeadObjectHandle);
	if(deadTeam==0)
		return DoEjectPilot; // Someone on neutral team always gets default behavior

	if(((deadTeam > 0) && (deadTeam < 6)) && (!isDeadAI))
		DoTaunt(TAUNTS_HumanShipDestroyed);

	// If a person died, respawn them, etc
	return DeadObject(DeadObjectHandle, KillersHandle, isDeadPerson, isDeadAI);
}


EjectKillRetCodes StratInstant::ObjectSniped(int DeadObjectHandle, int KillersHandle)
{
	if(!m_bIsFriendlyFireOn && IsAlly(DeadObjectHandle, KillersHandle))
	{
		// @NMTODO - bump spawn kills here?
	}

	const bool isDeadAI = !IsPlayer(DeadObjectHandle);

	if(GetCurWorld() != 0)
	{
		return DLLHandled;
	}

	// Dead person means we must always respawn a new person
	return DeadObject(DeadObjectHandle, KillersHandle, true, isDeadAI);
}

bool StratInstant::Load(bool missionSave)
{
	EnableHighTPS(m_GameTPS);

	// We're a 1.3 DLL.
	WantBotKillMessages();

	// Make sure we always call this
	StartingVehicleManager::Load(missionSave);

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

		subMission->Load(missionSave);

		// Always do this to hook up clients with the taunt engine as well.
		InitTaunts(&m_ElapsedGameTime, &m_LastTauntPrintedAt, &m_GameTPS);

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

	subMission->Load(missionSave);

	return ret;
}


bool StratInstant::PostLoad(bool missionSave)
{
	if (missionSave)
		return true;

	bool ret = true;

	ConvertHandles(h_array, h_count);

	subMission->PostLoad(missionSave);
	CheckedSVar3=false;

	return ret;
}


bool StratInstant::Save(bool missionSave)
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

	subMission->Save(missionSave);

	return ret;
}


// Notification to the DLL: Passed the current world, shooters handle,
// victim handle, and the ODF string of the ordnance involved in the
// snipe. Returns a code detailing what to do.
//
// !! Note : If DLLs want to do any actions to the world based on this
// PreSnipe callback, they should (1) Ensure curWorld == 0 (lockstep)
// -- do NOTHING if curWorld is != 0, and (2) probably queue up an
// action to do in the next Execute() call.
PreSnipeReturnCodes StratInstant::PreSnipe(const int curWorld, Handle shooterHandle, Handle victimHandle, int ordnanceTeam, char* pOrdnanceODF)
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
PreGetInReturnCodes StratInstant::PreGetIn(const int curWorld, Handle pilotHandle, Handle emptyCraftHandle)
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
	subMission = BuildSubMission();
	return new StratInstant;
}
