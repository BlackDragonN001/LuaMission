#ifndef STRATEGY_02_H__
#define STRATEGY_02_H__

#include "..\Shared\DLLBase.h"
#include <vector>

class Strategy02 : public DLLBase
{
private:
	// WARNING: If you make an array in here based on MAX_PLAYERS, be
	// *SURE* to properly shuffle its contents down in the
	// DeletePlayer() call.  Otherwise, things will be incorrectly
	// credited after someone leaves. Arrays based on MAX_TEAMS are not
	// shuffled by default on player deletion


	// Thug Scavenger Objects. -GBD
	struct ThugScavengerClass {
		Handle ThugScavengerObject; // This ThugScavenger.
		int ScavengerTeam; // Save its team.
		//int ScavCurrScrap; // Save curr scrap for giving switching on MPI.
	};
	std::vector<ThugScavengerClass> ThugScavengerList;
	static bool ShouldRemoveThugScavengerObject(const ThugScavengerClass &entry) { 
		char ObjClass[64] = {0};
		GetObjInfo(entry.ThugScavengerObject, Get_GOClass, ObjClass);
		return !IsAround(entry.ThugScavengerObject) || (_stricmp(ObjClass, "CLASS_EXTRACTOR") == 0) || (_stricmp(ObjClass, "CLASS_SILO") == 0); }

	// Thug Pilot Objects.-GBD
	struct ThugPilotClass {
		Handle ThugPilotObject; // This ThugPilot.
		int SwitchTickTimer; // Timer (in ticks) for switching them to their commander's Team.
	};
	std::vector<ThugPilotClass> ThugPilotList;
	static bool ShouldRemoveThugPilotObject(const ThugPilotClass &entry) { return !IsAround(entry.ThugPilotObject) || entry.SwitchTickTimer < 0; }
	int m_GameTPS; // How many turns per second there are (10, 15, 20, or 30)

	int i_first,
		m_TotalGameTime,
		m_RemainingGameTime,
		m_ElapsedGameTime,
		m_KillLimit, // As specified from the shell
		m_PointsForAIKill, // ivar14
		m_KillForAIKill, // ivar15
		m_RespawnWithSniper, // ivar16
		m_TurretAISkill, // ivar17
		m_NonTurretAISkill, // ivar18
		m_StartingVehiclesMask, // ivar7
		m_SpawnedAtTime[MAX_TEAMS],
		//m_Alliances, // ivar23 // No longer used. -GBD //NOTE: Find engine code that sets allowed TeamSlots based on ivar23, make it behave in FFA the same way it does in TeamPlay. -GBD
		m_RecyInvulnerabilityTime, // ivar25, if >0, this is currently active

		m_AllyTeams[MAX_TEAMS], // New alliance var setting. Yay! -GBD
		m_NumSpawnPoints, // Number of Spawn Points. -GBD
		NumCommanders, 
		NextAutoAssignTeam,
		CommanderTeam[MAX_TEAMS],
		ThugCounter[MAX_TEAMS],

		i_last;

	bool
	b_first,
		m_DidInit,
		m_HadMultipleFunctioningTeams,
		m_TeamIsSetUp[MAX_TEAMS],
		m_NotedRecyclerLocation[MAX_TEAMS], // of deployed recycler
		m_GameOver,
		// Flag that we're creating the staring vehicles, so AI skill levels
		// need to be dropped a notch.
		m_CreatingStartingVehicles,
		m_RespawnAtLowAltitude,
		m_bIsFriendlyFireOn,

		m_HasAllies[MAX_TEAMS], // Flag if they have any allies, so we know to end the game or not. -GBD
		m_ThugFlag[MAX_TEAMS], // Flag for if this player is a commander. -GBD
		m_TeamHasCommander[MAX_TEAMS], // Flag for if we've chosen a commander. -GBD
		//m_SetThugPilotFlag, //Flag to enable/disable the new Thug pilot pregetin call. -GBD
		b_last;

	float
	f_first,
		m_TeamPos[3*(MAX_TEAMS+1)], // Where they started out, used in respawning
		f_last;

	Handle
	h_first,
		m_RecyclerHandles[MAX_TEAMS],
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
	Strategy02(void);

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

	// Notification to the DLL: a sniper shell has hit a piloted
	// craft. The exe passes the current world, shooters handle,
	// victim handle, and the ODF string of the ordnance involved in
	// the snipe. Returns a code detailing what to do.
	//
	// !! Note : If DLLs want to do any actions to the world based on this
	// PreSnipe callback, they should (1) Ensure curWorld == 0 (lockstep)
	// -- do NOTHING if curWorld is != 0, and (2) probably queue up an
	// action to do in the next Execute() call.
	virtual PreSnipeReturnCodes PreSnipe(const int curWorld, Handle shooterHandle, Handle victimHandle, int ordnanceTeam, char* pOrdnanceODF);

	// Notification to the DLL: called when a pilot tries to enter an
	// empty craft, and all other checks (i.e. craft is empty, masks
	// match, etc) have passed. DLLs can prevent that pilot from
	// entering the craft if desired.
	//
	// !! Note : If DLLs want to do any actions to the world based on this
	// PreSnipe callback, they should (1) Ensure curWorld == 0 (lockstep)
	// -- do NOTHING if curWorld is != 0, and (2) probably queue up an
	// action to do in the next Execute() call.
	virtual PreGetInReturnCodes PreGetIn(const int curWorld, Handle pilotHandle, Handle emptyCraftHandle);

private:
	// Rest of calls are internal functions, thus made private.

	// Internal one-time setup
	void Init(void);

	// Gets the initial player vehicle as selected in the shell
	char *GetInitialPlayerVehicleODF(int Team);

	// Given a race identifier, get the pilot back (used during a respawn)
	char *GetInitialPlayerPilotODF(char Race);

	// Given a race identifier, get the recycler ODF back
	char *GetInitialRecyclerODF(char Race);

	// Given a race identifier, get the scavenger ODF back
	char *GetInitialScavengerODF(char Race);

	// Given a race identifier, get the constructor ODF back
	char *GetInitialConstructorODF(char Race);

	// Given a race identifier, get the healer (service truck) ODF back
	char *GetInitialHealerODF(char Race);

	// Sets up the side's commander's extra vehicles, such a recycler or
	// more.  Does *not* create the player vehicle for them. [More notes
	// in .cpp file]
	void SetupTeam(int Team);

	// Given an index into the Player list, build everything for a given
	// single player (i.e. a vehicle of some sorts), and set up the team
	// as well as necessary
	Handle SetupPlayer(int Team);

	// Called from Execute, 1/10 of a second has elapsed. Update everything.
	void UpdateGameTime(void);

	/* // Old code. -GBD
	// Tests if an allied team (A1,A2) has won over the other allied
	// team (B1,B2) Sets gameover if true.
	void TestAlliedGameover(int A1, int A2, int B1, int B2, bool *TeamIsFunctioning);
	*/

	// Check for absence of recycler & factory, gameover if so.
	void ExecuteCheckIfGameOver(void);

	// Called from Execute(). If Recycler invulnerability is on, then
	// does the job of it.
	void ExecuteRecyInvulnerability(void);

	void ScavengerManagementCode(void); // Thug Managment. -GBD

	// Rebuilds pilot
	EjectKillRetCodes	RespawnPilot(Handle DeadObjectHandle,int Team);

	// Helper function for ObjectKilled/Sniped
	EjectKillRetCodes DeadObject(int DeadObjectHandle, int KillersHandle, bool WasDeadPerson, bool WasDeadAI);

	// Helper function for SetupTeam(), returns an appropriate spawnpoint.
	Vector GetSpawnpointForTeam(int Team);
}; // end of 'class Strategy02'

#endif // STRATEGY_02_H__
