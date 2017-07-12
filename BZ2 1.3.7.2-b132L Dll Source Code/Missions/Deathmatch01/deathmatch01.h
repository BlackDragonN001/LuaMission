#ifndef DEATHMATCH_01_H__
#define DEATHMATCH_01_H__

#include "..\Shared\DLLBase.h"

class Deathmatch01 : public DLLBase
{
public:

	enum {
		// WARNING: this can't go over 16 for now, as spawnpoints identified by team # now
		//MAX_CHECKPOINTS=MAX_TEAMS,
		MAX_AI_UNITS = 256,

		MAX_ANIMALS = 8,

		// # of vehicles we track for culling to keep the map cleared
		MAX_VEHICLES_TRACKED= (2 * MAX_TEAMS),
	};

private:
	// Variables that are NOT saved & loaded across the network.
	int m_GameTPS; // How many turns per second there are (10, 15, 20, or 30)

	// Variables that ARE saved & loaded across the network.
	int i_first,
		m_TotalGameTime,
		m_ElapsedGameTime,
		m_RemainingGameTime,
		m_KillLimit, // As specified from the shell
		m_MissionType,
		m_RespawnType,
		m_NumVehiclesTracked, 
		m_SpawnedAtTime[MAX_TEAMS],

		// How long a "spawn" kill lasts, in tenth of second ticks. If the
		// time since they were spawned to current is less than this, it's a
		// spawn kill, and not counted. From ivar13
		m_MaxSpawnKillTime, // long time, so we can debug this

		m_LastTauntPrintedAt,
		
		// Variables used during racing
		m_NextRaceCheckpoint[MAX_TEAMS],
		m_TotalCheckpointsCompleted[MAX_TEAMS],
		m_TotalCheckpoints,
		m_LapNumber[MAX_TEAMS],
		m_LastTeamInLead,
		m_TotalRaceLaps,
		m_RaceWinerCount,
		// End of race varbs

		m_AIUnitSkill, // from ivar22, 0..3 or 4==random
		m_NumAIUnits, // Current count of AIs spawned in
		m_MaxAIUnits, // Limit of AIs
		m_NumAnimals, // Current count of AIs spawned in
		m_MaxAnimals, // Limit of AIs
		m_AnimalConfig[16], // HACK - storing a string in here.

		//		AITeams[MAX_AI_UNITS], // Which team # was last assigned to the specified AI unit
		m_PowerupGotoTime[MAX_AI_UNITS], // How much time has elapsed on their quest for a powerup
		m_RabbitTeam,
		m_ForbidRabbitTeam,
		m_RabbitShooterTeam,
		m_RabbitMissingTurns, // how many turns they're MIA

		m_Gravity, // from ivar30
		m_ScoreLimit, // from ivar35, 0=unlimited
		i_last;

	bool
	b_first,
		m_DidOneTimeInit,
		m_FirstTime, m_GameWon, 
		m_Flying[MAX_TEAMS], // Flag saying we need to keep track of a specific player to build a craft when they land
		m_TeamIsSetUp[MAX_TEAMS],
		m_DMSetup,
		m_RaceIsSetup,
		m_HumansVsBots,
		m_RabbitMode,
		m_RabbitWasHuman,
		m_RabbitShooterWasHuman,
		m_AILastWentForPowerup[MAX_AI_UNITS], // flag saying they last went for 
		m_WeenieMode,
		m_ShipOnlyMode,
		m_RespawnAtLowAltitude,
		m_bIsFriendlyFireOn,
		m_bDidGameOverByScore, // Goes true when we have noted a winner by score
		b_last;

	float
	f_first,
		m_TeamPos[3*(MAX_TEAMS+1)], // Where they started out, used in respawning
		//m_SpawnPointPos[3*(MAX_TEAMS+1)], // Used during race
		m_RaceCheckpointRadius,
		f_last;

	Handle
	h_first,
		m_Flag1, m_Flag2, m_Carrier1, m_Carrier2, m_Base1, m_Base2,
		m_EmptyVehicles[MAX_VEHICLES_TRACKED],
		//m_SpawnPointHandles[MAX_TEAMS], // Used during race
		m_AICraftHandles[MAX_AI_UNITS],
		m_AITargetHandles[MAX_AI_UNITS], // Whom each of those is aiming at.
		m_LastPlayerCraftHandle[MAX_TEAMS], // for ShipOnly mode
		m_AnimalHandles[MAX_ANIMALS],
		m_RabbitTargetHandle,
		m_RabbitShooterHandle,
		h_last;

protected:
	bool *b_array;
	int b_count;

	float *f_array;
	int f_count;

	int *h_array;
	int h_count;

	int *i_array;
	int i_count;

public:
	// Calls made from the outside

	// Constructor
	Deathmatch01(void);

	// Housekeeping functions
	bool Load(bool missionSave);
	bool PostLoad(bool missionSave);
	bool Save(bool missionSave);

	// Callback when an object has been built into the world.
	void AddObject(Handle h);

	// Callbacks and helper functions for callbacks
	bool AddPlayer(DPID id, int Team, bool IsNewPlayer);
	void DeletePlayer(DPID id);
	EjectKillRetCodes PlayerEjected(Handle DeadObjectHandle);
	EjectKillRetCodes ObjectKilled(int DeadObjectHandle, int KillersHandle);
	EjectKillRetCodes ObjectSniped(int DeadObjectHandle, int KillersHandle);
	void Execute(void);
	char *GetNextRandomVehicleODF(int Team);

	// Passed the current world, shooters handle, victim handle, and
	// the ODF string of the ordnance involved in the snipe. Returns a
	// code detailing what to do.
	//
	// !! Note : If DLLs want to do any actions to the world based on this
	// PreSnipe callback, they should (1) Ensure curWorld == 0 (lockstep)
	// -- do NOTHING if curWorld is != 0, and (2) probably queue up an
	// action to do in the next Execute() call.
	virtual PreSnipeReturnCodes PreSnipe(const int curWorld, Handle shooterHandle, Handle victimHandle, int ordnanceTeam, char* pOrdnanceODF);

private:
	// Rest of calls are internal functions, thus made private.

	void Init(void);
	void InitialSetup(void);

	EjectKillRetCodes RespawnPilot(Handle DeadObjectHandle,int Team);
	EjectKillRetCodes DeadObject(int DeadObjectHandle, int KillersHandle, bool WasDeadPerson, bool WasDeadAI);

	// Returns true if players can eject (or bail out) of their vehicles
	// depending on the game type, false if they should be killed when
	// they try and do so.
	bool GetCanEject(Handle h);

	// Returns true if players shouldn't be respawned as a pilot, but in
	// a piloted vehicle instead, i.e. during race mode.
	bool GetRespawnInVehicle(void);

	// Given a race identifier, get the flag odf back
	char *GetInitialFlagODF(char Race);

	// Given a race identifier, get the pilot odf back
	char *GetInitialPlayerPilotODF(char Race);

	// Gets the next vehicle ODF for the player on a given team.
	char *GetNextVehicleODF(int TeamNum, bool Randomize);

	// Does the work to set up a team
	void SetupTeam(int Team);

	// Tries to return a good target for the AI unit, orders them to go
	// after it.
	void FindGoodAITarget(const int Index);

	// Sets up the specified AI unit, first time or later.
	void BuildBotCraft(int Index);

	// Sets up the specified animal, first time or later.
	void SetupAnimal(int Index);

	void ObjectifyRabbit(void);

	// For CTF, objectifies the other team's flag
	void ObjectifyFlag(void);

	// This handle is the new rabbit. Target them!
	void SetNewRabbit(const Handle h);

	Handle SetupPlayer(int Team);

	// Collapses the tracked vehicle list so there are no holes (values
	// of 0) in it, puts the updated count in NumVehiclesTracked
	void CrunchEmptyVehicleList(void);

	// Designed to be called from Execute(), updates list of tracked
	// vehicles, killing some if there are too many, and forgetting
	// about any if they've been entered.
	void UpdateEmptyVehicles(void);

	// Adds a vehicle to the tracking list, and kills the oldest
	// unpiloted one if list full
	void AddEmptyVehicle(Handle NewCraft);

	// Flags the appropriately 'next' spawnpoint as the objective
	void ObjectifyRacePoint(void);

	// Helper functions called by Execute() for various game modes.

	// Rabbit-specific execute stuff. Kill da wabbit! Kill da wabbit!
	void ExecuteRabbit(void);
	// Race-specific execute stuff.
	void ExecuteRace(void);
	// For weenie mode
	void ExecuteWeenie(void);
	// Notices what ships all the human players are currently in. If
	// they hop out, then their old craft is pushed onto the empties
	// list. If this is 'ship only' mode, then other penalties are
	// applied.
	void ExecuteTrackPlayers(void);
	// Called via execute, 1/10 of a second has elapsed. Update everything.
	void UpdateGameTime(void);
	// For AI units
	void UpdateAIUnits();
	// For animals
	void UpdateAnimals();

	// Helper function for SetupTeam(), returns an appropriate spawnpoint.
	Vector GetSpawnpointForTeam(int Team);

}; // end of 'class Deathmatch01'

#endif // DEATHMATCH_01_H__
