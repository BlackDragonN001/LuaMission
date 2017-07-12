#ifndef MPINSTANT_H__
#define MPINSTANT_H__

#include "..\shared\SubMission.h"

enum AIPType
{
	AIPType0 = 0, 
	AIPType1, 
	AIPType2, 
	AIPType3, 
	AIPTypeA, 
	AIPTypeL, 
	AIPTypeS, 
	MAX_AIP_TYPE, 
};

class instantMission : public SubMission
{
	enum
	{
		// Number of CPU Scavs that'll trigger a cleanup
		MAX_CPU_SCAVS = 15,
	};

public:
	instantMission(void);
	//	~instantMission();

	// Based on player races and type, set AIPlan
	void SetCPUAIPlan(AIPType Type);

	// And set up extra race-specific vehicles at pathpoints in the map.
	void SetupExtraVehicles(void);

	// Builds a starting vehicle on the specified team with either ODF1
	// (preferred, if found) or ODF2 as its name, modified by the
	// race. Sets group, if specified. Returns the handle
	Handle BuildStartingVehicle(int Team, int Race, char *ODF1, char *ODF2, char *Where, int aGrp = -1);

	// h points to a CPU scav (regular or hover). Add it to the lists,
	// delete some older ones as needed. Same rules as for AddObject--
	// NEVER call RemoveObject(scavHandle) during this function.
	void AddCPUScav(Handle scavHandle);

	virtual void AddObject(Handle h, bool IsStartingVehicles, Handle *m_RecyclerHandles);
	virtual void DeleteObject(Handle h);
	virtual void Setup(void);
	virtual void CreateObjectives();
	virtual void TestObjectives();
	virtual void Execute(bool *pTeamIsSetUp, Handle *m_RecyclerHandles);
	virtual void DoGenericStrategy(bool *pTeamIsSetUp, Handle *m_RecyclerHandles);
	virtual EjectKillRetCodes PlayerEjected(void);
	virtual EjectKillRetCodes PlayerKilled(int KillersHandle);

private:
	// bools
	bool
		b_first, 
		m_StartDone, 
		m_SiegeOn, 
		m_AntiAssault, 
		m_LateGame, 
		m_HaveArmory, 
		m_SetFirstAIP, 
		m_IraqiMode, 
		m_PastAIP0, // true when they've built a recyler building, or we get bored
		b_last;

	// floats
	float
		f_first, 
		f, 
		f_last;

	// handles
	Handle
		h_first, 
// 		h, 
// 		obj1, 
// 		bunker, 
// 		bunker2, 
// 		player, 
// 		foe1, 
// 		foe2, 
// 		foe3, 
// 		walker, 
// 		tug, 
		m_HumanRecycler, 
		m_CPURecycler, 
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
		//		i, 
		m_SiegeCounter, 
		m_AssaultCounter, 
		//		next_wave, 
		//		next_spawn, 
		m_TimeCount, 
		m_CPUReinforcementTime, 
		m_HumanReinforcementTime, 
		m_CPUTeamNumber, 
		//		powerup_counter, 
		m_TurnCounter, 
		//		myGoal, // always 2 now - NM 10/20/06
		m_NumHumans, 
		//		difficulty, 
		m_NumCPUScavs, 
		m_TurretAISkill, // ivar17
		m_NonTurretAISkill, // ivar18
		m_CPUTurretAISkill, // ivar21
		m_CPUNonTurretAISkill, // ivar22
		m_HumanForce, 
		m_CPUForce, 
		m_CPUTeamRace, 
		m_HumanTeamRace, 
		m_FirstAIPSwitchTime, // ivar26
		m_CPUCommBunkerCount,
		m_LastCPUPlan,
		i_last;
};

#endif
