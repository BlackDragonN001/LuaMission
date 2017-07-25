#ifndef INSTANTMISSION_H__
#define INSTANTMISSION_H__

#include "..\Shared\DLLBase.h"

class instantMission : public DLLBase
{
private:
	enum AIPType {
		AIPType0=0,
		AIPType1,
		AIPType2,
		AIPType3,
		AIPTypeA,
		AIPTypeL,
		AIPTypeS,
		MAX_AIP_TYPE,
	};

	enum {
		// Number of CPU Scavs that'll trigger a cleanup
		MAX_CPU_SCAVS=15,
		
		// Number of scavs (vehicles) that'll be left around after a cleanup.
		MIN_CPU_SCAVS_AFTER_CLEANUP=5,

		MAX_GOODIES=40,
	};

	// bools
	bool
		b_first,
		m_StartDone,
		m_InspectBunker,
		m_SecondObjective,
		m_PickingUp[MAX_GOODIES],
		m_Captured[MAX_GOODIES],
		m_SiegeOn,
		m_AntiAssault,
		m_LateGame,
		m_HaveArmory,
		m_SetFirstAIP,
		m_PastAIP0, // true when they've built a recyler building, or we get bored
		m_CanRespawn, // true if the user can respawn if died (kill/snipe)

		b_last;

	// floats
	float
		f_first,
		f,
		f_last;

	// handles
	Handle
		h_first,
		h,
		m_Obj1,
		m_Bunker,
		m_Bunker2,
		m_Player,
		m_Foe1,
		m_Foe2,
		m_Foe3,
		m_Walker,
		m_Tug,
		m_Recycler, // Human's m_Recycler
		m_EnemyRecycler, // CPU's m_Recycler
		m_Powerup1,
		m_Powerup2,
		m_Powerup3,
		m_Powerup4,
		m_CPUScavList[MAX_CPU_SCAVS], // List of CPU scav handles, used during cleanup.

		h_last;

	int m_GameTPS; // How many turns per second there are (10, 15, 20, or 30)

	// integers
	int
		i_first,
		m_StratTeam,
		m_DllFlag,
		i,
		m_SiegeCounter,
		m_AssaultCounter,
		m_NextWave,
		m_NextSpawn,
		m_TimeCount,
		m_PostBunker,
		m_EnemyReinforcementTime,
		m_FriendReinforcementTime,
		m_EndCounter,
		m_CompTeam,
		m_PowerupCounter,
		m_TurnCounter,
		m_MyForce,
		m_MyGoal,
		//		mySide, // replaced by m_CPUTeamRace,m_HumanTeamRace
		m_AwareV13,
		m_CPUTeamRace,
		m_HumanTeamRace,

		m_CompForce,
		m_Difficulty,
		m_GoalVal,
		m_NumCPUScavs,

		m_ElapsedGameTime,
		m_LastTauntPrintedAt,

		m_CPUCommBunkerCount,

		m_LastCPUPlan,

		// UGLY CODE ALERT -- storing a string in an array of ints. Done
		// because that simplifies the load/save code quite a lot.
		// Avert your eyes if you're with the coding style police.
		m_CustomAIPInts[8], // == 64 bytes 

		i_last;

protected:
	bool *b_array;
	int b_count;

	float *f_array;
	int f_count;

	int *h_array;
	int h_count;

	int *i_array;
	int i_count;

private:
	char *m_CustomAIPStr; // Points to m_CustomAIPInts . Use this instead
	int m_CustomAIPStrLen; // size, in bytes, of this buffer

public:
	// Calls made from the outside

	// Constructor
	instantMission(void);

	// Housekeeping functions
	bool Load(bool missionSave);
	bool PostLoad(bool missionSave);
	bool Save(bool missionSave);

	// Callback when an object has been built into the world.
	void AddObject(Handle h);
	void DeleteObject(Handle h);

	EjectKillRetCodes PlayerEjected(Handle DeadObjectHandle);
	EjectKillRetCodes ObjectKilled(int DeadObjectHandle, int KillersHandle);
	EjectKillRetCodes ObjectSniped(int DeadObjectHandle, int KillersHandle);
	void Execute(void);
	virtual void DoGenericStrategy();

private:
	// Rest of calls are internal functions, thus made private.

	virtual void Setup(void);
	virtual void CreateObjectives();
	virtual void TestObjectives();

	// Sets the current AIP based on passed-in AIP type, which sides these are
	void SetCPUAIPlan(AIPType Type);

	// And set up extra race-specific vehicles at pathpoints in the map.
	void SetupExtraVehicles(void);

	// Builds a starting vehicle on the specified team with either ODF1
	// (preferred, if found) or ODF2 as its name, modified by the
	// race. Sets group, if specified. Returns the handle
	Handle BuildStartingVehicle(int Team, int Race, char *ODF1, char *ODF2, char *Where, int aGrp = -1);

	// Local user died. Do (optional) respawn or fail mission
	EjectKillRetCodes PlayerDied(int DeadObjectHandle, bool bSniped);

	// h points to a CPU scav (regular or hover). Add it to the lists,
	// delete some older ones as needed. Same rules as for AddObject--
	// NEVER call RemoveObject(scavHandle) during this function.
	void AddCPUScav(Handle scavHandle);

}; // end of 'class instantMission'

#endif // INSTANTMISSION_H__
