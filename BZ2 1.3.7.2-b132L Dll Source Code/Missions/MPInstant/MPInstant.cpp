#include "MPInstant.h"

#include <string.h>
#include "..\Shared\Taunts.h"
#include "..\Shared\TRNAllies.h"
#include "..\Shared\DLLUtils.h"

/*
InstantMission

Just add water!!
*/

// Distance from enemy recycler for enemies to be parked for siege
// mode to be triggered.
const float kfSIEGE_DISTANCE = 250.0f;

// #define TEST_INSTANT 1

//const int myForce = 1;
//const int compForce = 2;
const int difficulty = 2;

//

#ifdef _DEBUG
FILE *logfpout = NULL;
static void OpenLogfile(void)
{
	if(logfpout == NULL)
		logfpout = fopen("c:\\mpinstant.txt", "w");
}
#endif

// Errant scav-cleaning code. Added by Nathan Mates 7/20/01.


// Number of scavs (vehicles) that'll be left around after a cleanup.
const int MIN_CPU_SCAVS_AFTER_CLEANUP = 5;

const char *AIPTypeExtensions = "0123als"; // AIPType above is offset in this string table

char CustomAIPNameBase[256];
bool CheckedSVar3 = false;


// Sees if this is a m_HumanRecycler vehicle ODF, and returns the race
// ('a'..'z') if true. Returns 0 if not a m_HumanRecycler.
static int IsRecyclerODF(Handle h)
{
	if(h == 0)
	{
		return 0;
	}

	char ODFName[64];
	GetObjInfo(h, Get_CFG, ODFName);

	char ObjClass[64];
	GetObjInfo(h, Get_GOClass, ObjClass);

	if((_stricmp(ObjClass, "CLASS_RECYCLERVEHICLE") == 0) || (_stricmp(ObjClass, "CLASS_RECYCLER") == 0))
		return ODFName[0];

	// fallback: not a m_HumanRecycler.
	return 0;
}

instantMission::instantMission()
{
	EnableHighTPS(m_GameTPS);
	AllowRandomTracks(true); // If the user wants random music, we're fine with that.
	b_count = &b_last - &b_first - 1;
	b_array = &b_first + 1;

	f_count = &f_last - &f_first - 1;
	f_array = &f_first + 1;

	h_count = &h_last - &h_first - 1;
	h_array = &h_first + 1;

	i_count = &i_last - &i_first - 1;
	i_array = &i_first + 1;

	if(i_array)
		memset(i_array, 0, i_count*sizeof(int));
	if(f_array)
		memset(f_array, 0, f_count*sizeof(float));
	if(h_array)
		memset(h_array, 0, h_count*sizeof(Handle));
	if(b_array)
		memset(b_array, 0, b_count*sizeof(bool));
	TRNAllies::SetupTRNAllies(GetMapTRNFilename());
}


// Builds a starting vehicle on the specified team with either ODF1
// (preferred, if found) or ODF2 as its name, modified by the
// race. Sets group, if specified. Returns the handle
Handle instantMission::BuildStartingVehicle(int aTeam, int aRace, char *ODF1, char *ODF2, char *Where, int aGrp)
{
	char TempODF[64];

	strcpy_s(TempODF, ODF1);
	TempODF[0] = (char)aRace;
	if(!DoesODFExist(TempODF))
	{
		// fallback to ODF2
		strcpy_s(TempODF, ODF2);
		TempODF[0] = (char)aRace;
	}

	Handle h = BuildObject(TempODF, aTeam, Where);
	if((h != 0) && (aGrp >= 0))
		SetGroup(h, aGrp);
	return h;
}


// And set up extra race-specific vehicles at pathpoints in the map.
void instantMission::SetupExtraVehicles(void)
{
#ifndef EDITOR
	int pathCount;
	char **pathNames;
	GetAiPaths(pathCount, pathNames);

	char cCPUTeamRace = (char) m_CPUTeamRace;
	char cHumanTeamRace = (char) m_HumanTeamRace;
	size_t len;

	m_NumCPUScavs = 0; // Nothing in this list now.

	for (int i = 0; i < pathCount; ++i)
	{
		char *Label = pathNames[i];
		if(strncmp(Label, "mpi", 3) == 0)
		{
			// Starts with MPI. Process it.

			const int MaxODFLen = 64;
			char ODF1[MaxODFLen];
			char ODF2[MaxODFLen];

			// Prezap string contents
			// MUST pre-clear these - compiler can't tell that ODF2 is
			// sometimes used uninitialized. NM 3/9/07
			memset(ODF1, 0, sizeof(ODF1));
			memset(ODF2, 0, sizeof(ODF2));

			char *pUnderscore = strchr(Label, '_');
			if(pUnderscore == NULL)
				continue; // Misformat! No _ found! Bail!

			char *pUnderscore2 = strchr(pUnderscore+1, '_');
			if(pUnderscore2 == NULL)
			{
				// Only 1 underscore in the name, e.g. (mpic#_ivturr). Copy
				// everything after it to the first name.
				strcpy_s(ODF1, pUnderscore+1);
			}
			else
			{
				// 2 underscores in the name, e.g. (mpic#_ivturr_fvturr). Copy
				// everything between them to ODF1, rest to ODF2.
				size_t len = (pUnderscore2 - pUnderscore) - 1;
				if(len > (sizeof(ODF2) - 1))
					len = (sizeof(ODF2) - 1);
				strncpy_s(ODF1, pUnderscore+1, len);
				strcpy_s(ODF2, pUnderscore2+1);
			}

			len = strlen(ODF1);
			if((len > 0) && (ODF1[len-1] == '_'))
				ODF1[len-1] = '\0';
			len = strlen(ODF2);
			if((len > 0) && (ODF2[len-1] == '_'))
				ODF2[len-1] = '\0';

			// For the CPU?
			if(strncmp(Label, "mpic", 4) == 0)
			{
				if(ODF1[0] == cCPUTeamRace)
					BuildObject(ODF1, m_CPUTeamNumber, Label);
				else if(ODF2[0] == cCPUTeamRace)
					BuildObject(ODF2, m_CPUTeamNumber, Label);
			}
			else if(strncmp(Label, "mpiC", 4) == 0)
			{
				if(ODF1[0])
				{
					ODF1[0] = cCPUTeamRace;
					BuildObject(ODF1, m_CPUTeamNumber, Label);
				}
				else if(ODF2[0])
				{
					ODF2[0] = cCPUTeamRace;
					BuildObject(ODF2, m_CPUTeamNumber, Label);
				}
			}
			else if(strncmp(Label, "mpih", 4) == 0)
			{
				// For the human side
				Handle h = 0;
				if(ODF1[0] == cHumanTeamRace)
				{
					h = BuildObject(ODF1, m_StratTeam, Label);
					SetBestGroup(h);
				}
				else if(ODF2[0] == cHumanTeamRace)
				{
					h = BuildObject(ODF2, m_StratTeam, Label);
					SetBestGroup(h);
				}
			}
			else if(strncmp(Label, "mpiH", 4) == 0)
			{
				// For the human side
				Handle h = 0;
				if(ODF1[0])
				{
					ODF1[0] = cHumanTeamRace;
					h = BuildObject(ODF1, m_StratTeam, Label);
					SetBestGroup(h);
				}
				else if(ODF2[0])
				{
					ODF2[0] = cHumanTeamRace;
					h = BuildObject(ODF2, m_StratTeam, Label);
					SetBestGroup(h);
				}
			}
		}
	}
#endif // #ifndef EDITOR
}


void instantMission::SetCPUAIPlan(AIPType Type)
{
#ifndef EDITOR
#if 0//def _DEBUG
	OpenLogfile();
	fprintf(logfpout, "SetCPUAIPlan(%d)\n", Type);
#endif

	if(!CheckedSVar3)
	{
		CheckedSVar3 = true;
		const char* pContents = DLLUtils::GetCheckedNetworkSvar(3, NETLIST_AIPs);
		if((pContents == NULL) || (pContents[0] == '\0'))
		{
			// Use the default 1.3 naming scheme if not specified.
			strcpy_s(CustomAIPNameBase, "stock13_");
		}
		else
		{
			strcpy_s(CustomAIPNameBase, pContents);
		}
	}

	if((Type<0) || (Type >= MAX_AIP_TYPE))
		Type = AIPType3; // Default

	// Try to keep AI from getting hung up if user rushes them - NM 7/15/05
	if(((Type > AIPType3) && (m_CPUCommBunkerCount == 0)) && (m_LastCPUPlan <= AIPType3))
		return;

	char AIPFile[256];

	// First pass, try to find an AIP that is designed to use Provides for enemy team, thus it only cares about CPU Race. This makes adding races much easier. -GBD 6/15/17
	sprintf_s(AIPFile, "%s%c%c.aip", CustomAIPNameBase, (char)m_CPUTeamRace, AIPTypeExtensions[Type]);
	// Fallback to old method if none exists. -GBD 6/15/17
	if (!DoesFileExist(AIPFile))
		sprintf_s(AIPFile, "%s%c%c%c.aip", CustomAIPNameBase, (char)m_CPUTeamRace, (char)m_HumanTeamRace, AIPTypeExtensions[Type]);

	SetPlan(AIPFile, m_CPUTeamNumber);

	m_LastCPUPlan = Type;

	if(m_SetFirstAIP)
		DoTaunt(TAUNTS_Random);
	m_SetFirstAIP = true;
#endif // #ifndef EDITOR
}


void instantMission::Setup(void)
{
	SetAutoGroupUnits(false);
#ifndef EDITOR
	/*
	Here's where you set the values at the start.  
	*/

	static bool DidInit = false;
	if(!DidInit)
	{
		m_TurretAISkill = GetVarItemInt("network.session.ivar17");
		m_TurretAISkill = clamp(m_TurretAISkill, 0, 3);

		m_NonTurretAISkill = GetVarItemInt("network.session.ivar18");
		m_NonTurretAISkill = clamp(m_NonTurretAISkill, 0, 3);

		m_CPUTurretAISkill = GetVarItemInt("network.session.ivar21");
		m_CPUTurretAISkill = clamp(m_CPUTurretAISkill, 0, 3);

		m_CPUNonTurretAISkill = GetVarItemInt("network.session.ivar22");
		m_CPUNonTurretAISkill = clamp(m_CPUNonTurretAISkill, 0, 3);

		int StartingVehicles = GetVarItemInt("network.session.ivar7");
		m_IraqiMode = (m_TurretAISkill == 3) && (m_NonTurretAISkill == 3) && 
			(m_CPUTurretAISkill == 0) && (m_CPUNonTurretAISkill == 0) && (StartingVehicles == 7);

		m_HumanForce = GetVarItemInt("network.session.ivar23");
		m_HumanForce = clamp(m_HumanForce, 0, 3);

		m_CPUForce = GetVarItemInt("network.session.ivar24");
		m_CPUForce = clamp(m_CPUForce, 0, 3);

		m_FirstAIPSwitchTime = GetVarItemInt("network.session.ivar26");
		if(m_FirstAIPSwitchTime == 0)
			m_FirstAIPSwitchTime = 180;
		if(m_FirstAIPSwitchTime > 0)
			m_FirstAIPSwitchTime *= m_GameTPS; // convert from seconds to sim ticks
	}
	m_NumHumans = CountPlayers();
#endif // #ifndef EDITOR

	m_StartDone = false;
	m_TimeCount = 1;
	m_CPUReinforcementTime = 2000;
	m_HumanReinforcementTime = 2500;
	m_CPUTeamNumber = 6;
	m_StratTeam = 1;  // the default
	//	obj1 = (-1);

	m_TurnCounter = 0;

	m_SiegeOn = false;
	m_AntiAssault = false;
	m_SiegeCounter = 0;
	m_AssaultCounter = 0;
	m_LateGame = false;
	m_HaveArmory = false;

	m_Powerup1 = NULL;
	m_Powerup2 = NULL;
	m_Powerup3 = NULL;
	m_Powerup4 = NULL;

	//	myGoal = 2;
}


// h points to a CPU scav (regular or hover). Add it to the lists,
// delete some older ones as needed. Same rules as for AddObject--
// NEVER call RemoveObject(scavHandle) during this function.
void instantMission::AddCPUScav(Handle scavHandle)
{
	// Ok, adding in another scav. Add to list (and don't trash
	// memory) This is a fast operation in all cases.
	if(m_NumCPUScavs < MAX_CPU_SCAVS)
		m_CPUScavList[m_NumCPUScavs++] = scavHandle;

	// If our lists aren't full, then we're done from this function.
	if(m_NumCPUScavs < MAX_CPU_SCAVS)
		return;

	// Spice things up.
	DoTaunt(TAUNTS_Random);

	// If we get here, it's cleanup time. First pass - cull CPU scav
	// list of any dead ones, ones that changed teams, or ones that
	// aren't scavs anymore.
	Handle newScavList[MAX_CPU_SCAVS];
	int newScavListCount = 0;
	memset(newScavList, 0, sizeof(newScavList));
	int i;

	for(i = 0; i < m_NumCPUScavs; i++)
	{
		bool keepIt = false;
		// Do quick checks first
		Handle checkScav = m_CPUScavList[i];
		if(IsAround(checkScav) && (GetTeamNum(checkScav) == m_CPUTeamNumber))
		{
			char ObjClass[64];
			GetObjInfo(checkScav, Get_GOClass, ObjClass);
			keepIt = (_stricmp(ObjClass, "CLASS_SCAVENGER") == 0) || (_stricmp(ObjClass, "CLASS_SCAVENGERH") == 0);
		}
		// Always keep the scavHandle passed in to this function.
		keepIt |= (checkScav == scavHandle);

		if(keepIt)
		{
			newScavList[newScavListCount++] = checkScav;
		}
	} // i loop over m_NumCPUScavs

	// Copy culled list back into master list.
	m_NumCPUScavs = newScavListCount;
	memcpy(m_CPUScavList, newScavList, sizeof(m_CPUScavList));

	// Now, if there's still too many scavs, kill from the youngest
	// scavs. (m_CPUScavList[0] is the oldest scav). Theoretically,
	// the oldest scavs are the furthest from the m_HumanRecycler, so
	// killing them shouldn't affect the gameplay too much. You could
	// probably improve this algorithm by keeping, say 10-15 scavs,
	// that are more than 150 meters from the m_HumanRecycler. That is
	// left as an exercise for the reader. :)

	while(m_NumCPUScavs > MIN_CPU_SCAVS_AFTER_CLEANUP)
	{
		// Find newest scav that isn't the one that just got created.
		i = m_NumCPUScavs - 1; // in C, arrays count from 0, so last valid element is at [count-1]
		while((i > 0) && (m_CPUScavList[i] == scavHandle))
			--i;
		if(m_CPUScavList[i] == scavHandle)
			break; // extra sanity check.

		// Don't generate a scrapfield the AI might try and
		// build more scavs to pick up and loop infinitely.
		SetNoScrapFlagByHandle(m_CPUScavList[i]);
		SelfDamage(m_CPUScavList[i], 1e28f);

		// Forget about this scav, shuffle array down.
		for(; i<m_NumCPUScavs; ++i)
			m_CPUScavList[i] = m_CPUScavList[i+1];
		--m_NumCPUScavs;
		m_CPUScavList[m_NumCPUScavs] = 0; // zap what used to be in last slot.
	}
}


// Note: within this AddObject() function body, *NEVER* call
// RemoveObject(h), i.e. what was passed in. That *will* cause
// problems. If you must delete it, stash that Handle somewhere, 
// and call RemoveObject next Execute() cycle.
void instantMission::AddObject(Handle h, bool IsStartingVehicles, Handle *pRecyclerHandles)
{
#ifndef EDITOR
	// Always grab the ODF name out, if possible. Makes life much easier
	// in here.
	char ODFName[64];
	GetObjInfo(h, Get_CFG, ODFName);

	// Always get one random # in 0..1, for scion shield randomization
	float fRandomNum = GetRandomFloat(1.0f);

	// Need to watch this the first few turns, as we're not called
	// immediately anymore - NM 6/18/05
	if (m_TurnCounter < 2)
	{
		// monitor what objects get built by the mpstrat dll
		if (GetTeamNum(h) == 1)
		{

			// Is this the human's m_HumanRecycler? If so, note it.
			int HumanRecyRace = IsRecyclerODF(h);
			if(HumanRecyRace)
			{
				m_HumanTeamRace = HumanRecyRace;
				m_HumanRecycler = h;
			}

			// Always get CPU race the first timestep also.
			int ShellRace = GetVarItemInt("network.session.ivar13");
			if(ShellRace)
			{
				m_CPUTeamRace = ShellRace;
			}
			else
			{
				Handle h = GetObjectByTeamSlot(m_CPUTeamNumber, DLL_TEAM_SLOT_RECYCLER);
				if(h != 0)
				{
					int cpuRace = IsRecyclerODF(h);
					if(cpuRace != 0)
					{
						m_CPUTeamRace = cpuRace;
					}
				}
				else
				{
					m_CPUTeamRace = 'i';
				}
			}

#if 0//def _DEBUG
			OpenLogfile();
			fprintf(logfpout, "m_HumanTeamRace = '%c' . m_CPUTeamRace = '%c'\n", m_HumanTeamRace, m_CPUTeamRace);
#endif

		}
	}

	// Note: this is the proper way to determine if a handle points to a
	// turret. NM 1/29/05
	char ObjClass[64];
	GetObjInfo(h, Get_GOClass, ObjClass);
	bool IsTurret = (_stricmp(ObjClass, "CLASS_TURRETTANK") == 0);
	bool IsCommBunker = (_stricmp(ObjClass, "CLASS_COMMBUNKER") == 0) || 
		(_stricmp(ObjClass, "CLASS_COMMTOWER") == 0);
	if(IsCommBunker && (GetTeamNum(h) == m_CPUTeamNumber))
		++m_CPUCommBunkerCount;

	// Set skill levels for humans
	if (GetTeamNum(h)!= m_CPUTeamNumber)
	{

		int UseTurretSkill = m_TurretAISkill;
		int UseNonTurretSkill = m_NonTurretAISkill;
#if 0
		if(IsStartingVehicles)
		{
			if(UseTurretSkill >0)
				UseTurretSkill--;  // bottoms out at zero.
			if(UseNonTurretSkill >0)
				UseNonTurretSkill--;  // bottoms out at zero.
		}
#endif

		if(IsTurret)
			SetSkill(h, UseTurretSkill);
		else
			SetSkill(h, UseNonTurretSkill);
	}

	// Also see if this is a new m_HumanRecycler vehicle (e.g. user upgraded
	// m_HumanRecycler building to vehicle)
	if(pRecyclerHandles != NULL)
	{
		bool IsRecyVehicle = (_stricmp(ObjClass, "CLASS_RECYCLERVEHICLE") == 0);
		if(IsRecyVehicle)
		{
			int Team = GetTeamNum(h);
			// If we're not tracking a m_HumanRecycler vehicle for this team right
			// now, store it.
			if(pRecyclerHandles[Team] == 0)
				pRecyclerHandles[Team] = h;
		}
	}

	// CPU unit added. Do stuff.
	if (GetTeamNum(h) == m_CPUTeamNumber)
	{

		int UseTurretSkill = m_CPUTurretAISkill;
		int UseNonTurretSkill = m_CPUNonTurretAISkill;
		if(IsStartingVehicles)
		{
			if(UseTurretSkill >0)
				UseTurretSkill--;  // bottoms out at zero.
			if(UseNonTurretSkill >0)
				UseNonTurretSkill--;  // bottoms out at zero.
		}

		if(IsTurret)
			SetSkill(h, UseTurretSkill);
		else
			SetSkill(h, UseNonTurretSkill);

		if(m_IraqiMode)
		{
			// Nerf CPU team for quick testing.
			SetCurHealth(h, 1);
			SetCurAmmo(h, 1);
			SetMaxHealth(h, 1);
			SetMaxAmmo(h, 1);
			SetSkill(h, 0);
		}

		// Better way to identify scavs added NM 1/29/05 -- checks for
		// regular & hoverscavs, no matter what their ODF is named.
		char ObjClass[64];
		GetObjInfo(h, Get_GOClass, ObjClass);

		if((_stricmp(ObjClass, "CLASS_SCAVENGER") == 0) || 
		   (_stricmp(ObjClass, "CLASS_SCAVENGERH") == 0))
		{
			AddCPUScav(h);
		}

		// Changed NM 10/19/01 - all AI is at skill 3 now by default (at top of this function)
		//		SetSkill(h, difficulty+1);

		/*
		if (//(!IsAlive(h)) && 
		((IsOdf(h, "fvtank")) || (IsOdf(h, "fvarch"))))
		{
		for (int temp = 0;temp<MAX_GOODIES;temp++)
		{
		if (goodies[temp] == NULL)
		{
		goodies[temp] = h;
		break;
		}
		}
		}
		*/

		if (_stricmp(ObjClass, "CLASS_ARMORY") == 0)
		{
			m_HaveArmory = true;
		}

		if (m_HaveArmory)
		{
			if(_strnicmp(ODFName, "ivtank", 6) == 0)
			{
				GiveWeapon(h, "gspstab_c");
			}
			if(_strnicmp(ODFName, "fvtank", 6) == 0)
			{
				GiveWeapon(h, "garc_c");   // what a crutch!  
				//				GiveWeapon(h, "gshield");
			}

			// Randomize shields somewhat. - NM 5/24/03
			if(_strnicmp(ODFName, "fv", 2) == 0)
			{
				if(fRandomNum < 0.3f)
					GiveWeapon(h, "gshield");
				else if(fRandomNum < 0.6f)
					GiveWeapon(h, "gabsorb");
				else if(fRandomNum < 0.9f)
					GiveWeapon(h, "gdeflect");
			}
		}
	} // Computer team.
	else
#if 0  // myGoal always 2 now - NM 10/28/06
		if ((myGoal == 0) && (GetTeamNum(h) == m_StratTeam))
		{

			// if it's artillery bomber/ go to late game tactics
			if((_stricmp(ObjClass, "CLASS_ARTILLERY") == 0) ||
				(_stricmp(ObjClass, "CLASS_BOMBER") == 0))
			{
				if ((!m_SiegeOn) && (!m_LateGame))
				{
					m_LateGame = true;
					SetCPUAIPlan(AIPTypeL);
				}
			}

			// Um, what's this code supposed to be doing? Turned off for now -
			// NM 9/9/02
			// 			if ((_stricmp(ObjClass, "CLASS_HOVER") == 0) ||
			// 				(_stricmp(ObjClass, "CLASS_MORPHTANK") == 0) ||
			// 				(_stricmp(ObjClass, "CLASS_ASSAULTTANK") == 0) ||
			// 				(_stricmp(ObjClass, "CLASS_SERVICE") == 0) ||
			// 				(_stricmp(ObjClass, "CLASS_WALKER") == 0))
			// 		{
			// 			// Changed NM 10/19/01 - all AI is at skill 3 now by default (at top of this function)
			// 			//			SetSkill(h, 3-difficulty);

			// 			SetTeamNum(h, 1);
			// 			//			int grp = GetFirstEmptyGroup();			
			// 			//			SetGroup(h, grp);
			// 		}

		}
#endif

		if (GetTeamNum(h) == m_StratTeam)
		{
			if(_stricmp(ObjClass, "CLASS_RECYCLER") == 0)
			{
				if(!m_PastAIP0)
				{
					m_PastAIP0 = true;
					/*
					SetAip here based on m_TurnCounter
					*/

					int stratchoice = m_TurnCounter%2;
					if (m_CPUTeamRace == 'f')
					{
						//				stratchoice = m_TurnCounter%3; // Seems to give problems - NM 7/5/03
						switch (stratchoice)
						{
						default:
						case 0:
#ifdef TEST_INSTANT
							SetObjectiveOn(h);
							SetObjectiveName(h, "scioninst1");
#endif
							SetCPUAIPlan(AIPType1);
							break;
						case 1:
#ifdef TEST_INSTANT
							SetObjectiveOn(h);
							SetObjectiveName(h, "scioninst3");
#endif
							SetCPUAIPlan(AIPType3);
							break;
						case 2:
#ifdef TEST_INSTANT
							SetObjectiveOn(h);
							SetObjectiveName(h, "scioninst2");
#endif
							SetCPUAIPlan(AIPType2);
							break;
						}
					}
					else
					{
						switch (stratchoice)
						{
						default:
						case 0:
#ifdef TEST_INSTANT
							SetObjectiveOn(h);
							SetObjectiveName(h, "isdfinst1");
#endif
							SetCPUAIPlan(AIPType1);
							break;
						case 1:
#ifdef TEST_INSTANT
							SetObjectiveOn(h);
							SetObjectiveName(h, "isdfinst3");
#endif
							SetCPUAIPlan(AIPType3);
							break;
						}

					}
				} // not past AIP0
			}
			else
			{
				if((_stricmp(ObjClass, "CLASS_ASSAULTTANK") == 0) ||
					(_stricmp(ObjClass, "CLASS_WALKER") == 0))
				{
					++m_AssaultCounter;
				}
			}
		} // on strat team

		if((!m_PastAIP0) && (m_FirstAIPSwitchTime > 0) && (m_TurnCounter > m_FirstAIPSwitchTime))
		{
			m_PastAIP0 = true;
			int stratchoice = m_TurnCounter%2;
			switch (stratchoice % 2)
			{
			default:
			case 0:
				SetCPUAIPlan(AIPType1);
				break;
			case 1:
				SetCPUAIPlan(AIPType3);
				break;
			}
		}
#endif // #ifndef EDITOR
}

void instantMission::DeleteObject(Handle h)
{
#ifndef EDITOR
	char ObjClass[64];
	GetObjInfo(h, Get_GOClass, ObjClass);

	if (GetTeamNum(h) == m_StratTeam)
	{
		// unit exists. Check its type.
		if((_stricmp(ObjClass, "CLASS_ASSAULTTANK") == 0) ||
			(_stricmp(ObjClass, "CLASS_WALKER") == 0))
		{
			--m_AssaultCounter;
			if(m_AssaultCounter < 0)
				m_AssaultCounter = 0;
		}
	}
	else
	{
		bool IsCommBunker = (_stricmp(ObjClass, "CLASS_COMMBUNKER") == 0) || 
			(_stricmp(ObjClass, "CLASS_COMMTOWER") == 0);
		if(IsCommBunker)
			--m_CPUCommBunkerCount;


		// CPU team.
		if (_stricmp(ObjClass, "CLASS_ARMORY") == 0)
		{
			m_HaveArmory = false;
		}
	}
#endif // #ifndef EDITOR
}

void instantMission::CreateObjectives()
{
}

void instantMission::TestObjectives()
{
	// game end conditions will be handled by the strat code
}

void instantMission::Execute(bool *pTeamIsSetUp, Handle *pRecyclerHandles)
{
	//	player = GetPlayerHandle();
	m_TurnCounter++;
	DoGenericStrategy(pTeamIsSetUp, pRecyclerHandles);

	// Any humans not on the right team? Make life hell for them.
	if((m_TurnCounter % (10 * m_GameTPS)) == 0)
	{

		int i;
		for(i = 0; i<MAX_TEAMS; i++)
		{
			Handle h = GetPlayerHandle(i);
			if(h)
			{
				int Grp = WhichTeamGroup(i);
				if(Grp != 0)
				{
					Damage(h, (1<<28)+1);
					AddToMessagesBox("MPI is limited to 5 humans! No joining the CPU team!");
				}
			}
		}
	}

}



int Rand(void)
{
	return (int)GetRandomFloat(32768);
}

void instantMission::DoGenericStrategy(bool *pTeamIsSetUp, Handle *pRecyclerHandles)
{
#ifndef EDITOR
	//#ifdef BLAHXBLAH
	//	int a;
	m_TimeCount++;
	// Must wait for Human's m_HumanRecycler to be created before we have
	// everything ready to go. - NM 6/18/05
	if ((!m_StartDone) && m_HumanTeamRace)
	{
		m_StratTeam = 1;
		//		m_HumanTeamRace = GetRaceOfTeam(m_StratTeam);

		m_StartDone = true;

		SetupExtraVehicles();

		// Build initial items for CPU team based on selections. The first
		// ODF listed is preferred, the second one really should be there
		// as a fallback

		// Build m_HumanRecycler, if it's not preplaced on the map.
		m_CPURecycler = GetObjectByTeamSlot(m_CPUTeamNumber, DLL_TEAM_SLOT_RECYCLER);
		if(m_CPURecycler == 0)
		{
			char startRecyODF[64];
			const char* pContents = DLLUtils::GetCheckedNetworkSvar(12, NETLIST_Recyclers);
			if((pContents != NULL) && (pContents[0] != '\0'))
				strncpy_s(startRecyODF, pContents, sizeof(startRecyODF)-1);
			else
				strcpy_s(startRecyODF, "*vrecycpu");

			startRecyODF[0] = (char)m_CPUTeamRace;
			if(!DoesODFExist(startRecyODF))
			{
				strcpy_s(startRecyODF, "*vrecycpu");
			}

			m_CPURecycler = BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, startRecyODF, "*vrecy", "RecyclerEnemy");
		}
		pRecyclerHandles[m_CPUTeamNumber] = m_CPURecycler;
		pTeamIsSetUp[m_CPUTeamNumber] = true;

		BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vturr", "*vturr", "turretEnemy1");
		BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vturr", "*vturr", "turretEnemy2");

		if (m_CPUForce>0)
		{
			BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*bspir", "*vturr", "gtow2");
			BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*bspir", "*vturr", "gtow3");
			BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vsent", "*vscout", "SentryEnemy1");
			BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vsent", "*vscout", "SentryEnemy2");
		}
		if (m_CPUForce>1)
		{
			BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*bspir", "*vturr", "gtow4");
			BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*bspir", "*vturr", "gtow5");
			BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vtank", "*vtank", "tankEnemy1");
			BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vtank", "*vtank", "tankEnemy2");
			BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vtank", "*vtank", "tankEnemy3");
			BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vtank", "*vtank", "TankEnemy3");
			BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vsent", "*vscout", "SentryEnemy3");
		}
		BuildStartingVehicle(m_CPUTeamNumber, m_CPUTeamRace, "*vscav", "*vscav", "ScavengerEnemy");

		// 		int grp = GetFirstEmptyGroup();			
		// 		SetGroup(m_HumanRecycler, grp);
#if 0  // myGoal always 2 now - NM 10/20/06
		if ( myGoal == 0)
		{
			// check my team..
			if(m_HumanTeamRace == 'i')
			{
				SetPlan("isdfteam.aip", m_StratTeam);
			}
			else
			{
				SetPlan("scionteam.aip", m_StratTeam);
			}	
		}
#endif

		// If we jumped to AIP 1/3 because there's a preplaced human
		// recy on the map, then don't go back to AIP 0
		if(!m_PastAIP0)
		{
			SetCPUAIPlan(AIPType0);
		}

		// do enemies
		//					ClearObjectives();
		//					AddObjective("isdf0501.otf", WHITE, 5.0f);

		// because turrets suck
		//		BuildObject("fvscout", m_CPUTeamNumber, "turretEnemy1");
		if (m_CPUForce>0)
		{
			if (m_CPUTeamRace == 'f')
			{
				// CPU is Scion
				BuildObject("fvtank", m_CPUTeamNumber, "TankEnemy1");
				BuildObject("fvtank", m_CPUTeamNumber, "TankEnemy2");
				BuildObject("fvsent", m_CPUTeamNumber, "SentryEnemy1");
				BuildObject("fvsent", m_CPUTeamNumber, "SentryEnemy2");
				if (m_CPUForce>1)
				{
					BuildObject("fvtank", m_CPUTeamNumber, "TankEnemy3");
					BuildObject("fvsent", m_CPUTeamNumber, "SentryEnemy3");	
				}
			}
			else
			{
				// CPU is ISDF
				BuildObject("ivtank", m_CPUTeamNumber, "TankEnemy1");
				BuildObject("ivtank", m_CPUTeamNumber, "TankEnemy2");
				BuildObject("ivscout", m_CPUTeamNumber, "SentryEnemy1");
				BuildObject("ivscout", m_CPUTeamNumber, "SentryEnemy2");
				if (m_CPUForce>1)
				{
					BuildObject("ivtank", m_CPUTeamNumber, "TankEnemy3");
					BuildObject("ivscout", m_CPUTeamNumber, "SentryEnemy3");	
				}
			}
		}

		//		BuildObject("ivscout", m_StratTeam, "Scout1");
		//		BuildObject("ivscout", m_StratTeam, "Scout2");
		//		BuildObject("ivscout", m_StratTeam, "Scout3");
		//		BuildObject("ivscav", m_StratTeam, "Scavenger");

		if (m_CPUTeamRace == 'f') // CPU is Scion
			BuildObject("fvscav", m_CPUTeamNumber, "ScavengerEnemy");
		else
			BuildObject("ivscav", m_CPUTeamNumber, "ScavengerEnemy");

		// power ups?
		BuildObject("apammo", 0, "ammo1");
		BuildObject("apammo", 0, "ammo2");
		BuildObject("apammo", 0, "ammo3");
		BuildObject("aprepa", 0, "repair1");
		BuildObject("aprepa", 0, "repair2");
		BuildObject("aprepa", 0, "repair3");
		// objectives .. later make this optional	
		//		CreateObjectives();
		SetScrap(m_CPUTeamNumber, 60);
		SetScrap(1, 40);
	}

	if ((m_TimeCount % m_CPUReinforcementTime) == 0)
	{
		//		AddScrap(m_CPUTeamNumber, 5);
		if (m_CPUForce>0)
		{
			//			BuildObject("fvtank", m_CPUTeamNumber, "TankEnemy1");
			//			BuildObject("fvtank", m_CPUTeamNumber, "TankEnemy2");
			//			BuildObject("fvsent", m_CPUTeamNumber, "SentryEnemy1");
			//			BuildObject("fvsent", m_CPUTeamNumber, "SentryEnemy2");
			//			AddScrap(m_CPUTeamNumber, 10);
			if (m_CPUForce>1)
			{
				//				BuildObject("fvtank", m_CPUTeamNumber, "TankEnemy3");
				//				BuildObject("fvsent", m_CPUTeamNumber, "SentryEnemy3");	
				AddScrap(m_CPUTeamNumber, 10);
			}
		}
	}

	if ((m_TimeCount % (10 * m_GameTPS)) == 0) 
	{
		// Evaluate this every 10 seconds
		m_NumHumans = CountPlayers();
	}

	// Avoid divide by zero if we set compforce to 3.
	int CompForceSkew = m_CPUForce * m_GameTPS;
	if(CompForceSkew >= (3 * m_GameTPS))
		CompForceSkew = (3 * m_GameTPS) - 2;

	if((m_TimeCount % ((3 * m_GameTPS) - CompForceSkew)) == 0)  // this speeds up resources
		AddScrap(m_CPUTeamNumber, 1);
	if (m_TimeCount % (m_GameTPS - m_NumHumans + 1) == 0)
		AddScrap(m_CPUTeamNumber, 1);
	if ((m_HumanForce) && (m_TimeCount%((10 * m_GameTPS)-(m_HumanForce*20)) == 0))
		AddScrap(m_StratTeam, 1);

	/*

	// An extra scrap boost? Why/how is this needed NM 3/5/02

	CompForceSkew = (m_CPUForce*10)-m_NumHumans+1;
	if(CompForceSkew >= 30)
	CompForceSkew = 28;

	if (m_TimeCount%(30-CompForceSkew) == 0)  // this speeds up resources
	AddScrap(m_CPUTeamNumber, 1);
	*/

	//	if (m_TimeCount%(100-(m_HumanForce*20)) == 0)
	//		AddScrap(m_StratTeam, 1);
	if (m_TimeCount % m_HumanReinforcementTime == 0)
	{
		/*
		BuildObject("ivtank", m_StratTeam, "Tank1");
		BuildObject("ivtank", m_StratTeam, "Tank2");
		BuildObject("ivtank", m_StratTeam, "Tank3");
		BuildObject("ivturr", m_StratTeam, "Turret1");
		BuildObject("ivturr", m_StratTeam, "Turret2");
		BuildObject("ivturr", m_StratTeam, "Turret3");
		BuildObject("ivscout", m_StratTeam, "Scout1");
		BuildObject("ivscout", m_StratTeam, "Scout2");
		BuildObject("ivscout", m_StratTeam, "Scout3");
		*/
		// power ups?
		BuildObject("apammo", 0, "ammo1");
		BuildObject("apammo", 0, "ammo2");
		BuildObject("apammo", 0, "ammo3");
		BuildObject("aprepa", 0, "repair1");
		BuildObject("aprepa", 0, "repair2");
		BuildObject("aprepa", 0, "repair3");

	}


#if 0  // myGoal always 2 - NM 10/20/06
	if (myGoal == 0)
	{
		powerup_counter++;
		if (powerup_counter%200 == 0)
		{
			if (!IsAround(m_Powerup1))
			{
				m_Powerup1 = BuildObject("apshdw", 0, "power1");
			}
			if (!IsAround(m_Powerup2))
			{
				m_Powerup2 = BuildObject("apsstb", 0, "power2");
			}
			if (!IsAround(m_Powerup3))
			{
				m_Powerup3 = BuildObject("apffld", 0, "power3");
			}
			if (!IsAround(m_Powerup4))
			{
				m_Powerup4 = BuildObject("apspln", 0, "power4");
			}
		}
	}
#endif

	// Unified code to determine closest enemy handle, distance to CPU
	// m_HumanRecycler or factory (if recy dead)
	Handle closestEnemy = 0;
	float closestEnemyDist = 1e30f; // big distance!

	// Find nearest enemy to m_HumanRecycler/factory (in that order)
	Handle RecyH = GetObjectByTeamSlot(m_CPUTeamNumber, DLL_TEAM_SLOT_RECYCLER);
	if(RecyH)
	{
		// Ignore scavs, pilots, and any craft > kfSIEGE_DISTANCE away.
		closestEnemy = GetNearestEnemy(RecyH, true, true, kfSIEGE_DISTANCE);
	} // m_HumanRecycler exists

	if(closestEnemy != 0)
	{
		// RecyH must be not-0 to get in here, see above.
		closestEnemyDist = GetDistance(closestEnemy, RecyH);
	}
	else
	{
		// Recyler doesn't exist, or closest to m_HumanRecycler is a scav. Check
		// around factory
		closestEnemy = 0; // redundant, but makes logic easier to follow
		Handle FactoryH = GetObjectByTeamSlot(m_CPUTeamNumber, DLL_TEAM_SLOT_FACTORY);
		if(FactoryH)
		{
			// Ignore scavs, pilots, and any craft > kfSIEGE_DISTANCE away.
			closestEnemy = GetNearestEnemy(FactoryH, true, true, kfSIEGE_DISTANCE);
		} // FactoryH exists

		if(closestEnemy != 0)
		{
			// RecyH must be not-0 to get in here, see above.
			closestEnemyDist = GetDistance(closestEnemy, FactoryH);
		} // Have closestEnemy
	} // Factory check

	// AIP switch logic
	if (!m_SiegeOn)
	{
		if(closestEnemy)
		{
			// under siege
			m_SiegeCounter++;
		}
		else
		{
			m_SiegeCounter = 0;
		}

		const int kiSIEGE_TIME = 45 * m_GameTPS; // 45 seconds
		if (m_SiegeCounter > kiSIEGE_TIME)
		{
			m_SiegeOn = true;
			SetCPUAIPlan(AIPTypeS);
		}
	}  // !m_SiegeOn
	else
	{
		// m_SiegeOn mustbe true to get here.
		if(closestEnemy == 0)
		{
			// no longer under siege
			SetCPUAIPlan(AIPTypeL);
			// Move out only when it succeeds.
			if(m_LastCPUPlan == AIPTypeL)
			{
				m_SiegeOn = false;
				m_SiegeCounter = 0;
			}
		}
	}  // m_SiegeOn

	// assault_test
	if ((!m_LateGame) && (!m_SiegeOn) && (!m_AntiAssault) && (m_AssaultCounter>2))
	{
		m_AntiAssault = true;
		SetCPUAIPlan(AIPTypeA);
	}
	else
	{
		if ((m_AntiAssault) && (m_AssaultCounter<3))
		{
			m_AntiAssault = false;
			SetCPUAIPlan(AIPTypeL);
		}
	}

	TestObjectives();
#endif // #ifndef EDITOR
}

EjectKillRetCodes instantMission::PlayerEjected(void)
{
	return DoEjectPilot;

}


EjectKillRetCodes instantMission::PlayerKilled(int /*KillersHandle*/)
{
	return DoEjectPilot;  // Game over, man.
}


SubMission* BuildSubMission(void)
{
	return new instantMission();
}

