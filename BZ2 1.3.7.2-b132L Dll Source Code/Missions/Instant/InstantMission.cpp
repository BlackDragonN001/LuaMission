#include <string.h>

#include "..\Shared\Taunts.h"
#include "InstantMission.h"
#include "..\Shared\TRNAllies.h"

//#include <windows.h>
/*
InstantMission

Just add water!!
*/

// Distance from enemy m_Recycler for enemies to be parked for siege
// mode to be triggered.
const float kfSIEGE_DISTANCE = 250.0f;

const int kRACE_ISDF = 105; // ascii for 'i'
const int kRACE_SCION = 102; // ascii for 'f'

// Base AIP name for stock AIPs. 
const char *kStockAIPNameBase="stock13_";

//#define TEST_INSTANT 1

//
// Errant scav-cleaning code. Added by Nathan Mates 7/20/01.
//
// By the way, this is mostly based on the cleanup of extra ships in
// the DM DLL. 
//

const char *AIPTypeExtensions="0123als"; // AIPType enum is offset in this string table


#if 0//def _DEBUG
#define TEST_SPAWNING 1
#endif

#if TEST_SPAWNING
// Debug code to test spawning lots of objects all the time.

  #ifdef _DEBUG
    #define MAX_TEMP_HANDLES 40
    #define SPAWN_COUNT_PER_EXECUTE 10
  #else
    #define MAX_TEMP_HANDLES 525
    #define SPAWN_COUNT_PER_EXECUTE 75
  #endif

int s_TempHandles[MAX_TEMP_HANDLES];
int s_TempHandlePosition = 0;
#endif

// Constructor
instantMission::instantMission(void)
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

	m_CustomAIPStr = (char*)(&m_CustomAIPInts[0]);
	m_CustomAIPStrLen = sizeof(m_CustomAIPInts);
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
		// doesn't exist, so fallback to ODF2
		strcpy_s(TempODF, ODF2);
		TempODF[0] = (char)aRace;
	}

	Handle h = BuildObject(TempODF, aTeam, Where);
	if((h != 0) && (aGrp >= 0))
		SetGroup(h, aGrp);
	return h;
}


// Sets the current AIP based on CPU's race, and AIP type.
void instantMission::SetCPUAIPlan(AIPType Type)
{
	// Clamp to legal value if not in range
	if((Type<0) || (Type >=MAX_AIP_TYPE))
		Type=AIPType3;

	// Try to keep AI from getting hung up if user rushes them - NM 7/15/05
	if(((Type > AIPType3) && (m_CPUCommBunkerCount == 0)) && (m_LastCPUPlan <= AIPType3))
		return;

	char AIPFile[256];

	// If custom AIPs were specified, use them.
	const char* AIPString = m_CustomAIPStr[0] ? m_CustomAIPStr : kStockAIPNameBase; // Reduces duplicate code. -GBD 6/15/17

	// First pass, try to find an AIP that is designed to use Provides for enemy team, thus it only cares about CPU Race. This makes adding races much easier. -GBD 6/15/17
	sprintf_s(AIPFile, "%s%c%c.aip", AIPString, (char)m_CPUTeamRace, AIPTypeExtensions[Type]);
	// Fallback to old method if none exists. -GBD 6/15/17
	if (!DoesFileExist(AIPFile)) 
		sprintf_s(AIPFile, "%s%c%c%c.aip", AIPString, (char)m_CPUTeamRace, (char)m_HumanTeamRace, AIPTypeExtensions[Type]);

	SetPlan(AIPFile, m_CompTeam);

	m_LastCPUPlan = Type;

	if(m_SetFirstAIP)
		DoTaunt(TAUNTS_Random);
	m_SetFirstAIP=true;
}


// And set up extra race-specific vehicles at pathpoints in the map.
void instantMission::SetupExtraVehicles(void)
{
	int pathCount;
	char **pathNames;
	GetAiPaths(pathCount, pathNames);

	char cCPUTeamRace = (char) m_CPUTeamRace;
	char cHumanTeamRace = (char) m_HumanTeamRace;
	size_t len;

	m_NumCPUScavs=0; // Nothing in this list now.

	for (int i = 0; i < pathCount; ++i)
	{
		char *Label = pathNames[i];
		if(strncmp(Label, "mpi", 3)==0)
		{
			// Starts with MPI. Process it.
			const int MaxODFLen = 64;
			char ODF1[MaxODFLen];
			char ODF2[MaxODFLen];
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
			if(strncmp(Label, "mpic", 4)==0)
			{
				if(ODF1[0] == cCPUTeamRace)
					BuildObject(ODF1, m_CompTeam, Label);
				else if(ODF2[0] == cCPUTeamRace)
					BuildObject(ODF2, m_CompTeam, Label);
			}
			else if(strncmp(Label, "mpiC", 4)==0)
			{
				if(ODF1[0])
				{
					ODF1[0]= cCPUTeamRace;
					BuildObject(ODF1, m_CompTeam, Label);
				}
				else if(ODF2[0])
				{
					ODF2[0] = cCPUTeamRace;
					BuildObject(ODF2, m_CompTeam, Label);
				}
			}
			else if(strncmp(Label, "mpih", 4)==0)
			{ 
				// For the human side
				Handle h = 0;
				if(ODF1[0] == cHumanTeamRace)
				{
					h=BuildObject(ODF1, m_StratTeam, Label);
					SetBestGroup(h);
				}
				else if(ODF2[0] == cHumanTeamRace)
				{
					h=BuildObject(ODF2, m_StratTeam, Label);
					SetBestGroup(h);
				}
			}
			else if(strncmp(Label, "mpiH", 4)==0)
			{
				// For the human side
				Handle h = 0;
				if(ODF1[0])
				{
					ODF1[0] = cHumanTeamRace;
					h=BuildObject(ODF1, m_StratTeam, Label);
					SetBestGroup(h);
				}
				else if(ODF2[0])
				{
					ODF2[0] = cHumanTeamRace;
					h=BuildObject(ODF2, m_StratTeam, Label);
					SetBestGroup(h);
				}
			}
		}
	}
}


void instantMission::Setup(void)
{
	SetAutoGroupUnits(false);

	/*
	Here's where you set the values at the start.  
	*/
	m_StartDone=false;
	m_TimeCount=1;
	m_InspectBunker=false;
	m_SecondObjective=false;
	m_PostBunker=1;
	m_EnemyReinforcementTime=2000;
	m_FriendReinforcementTime=2500;
	m_EndCounter=0;
	m_CompTeam=5;
	m_StratTeam=1;  // the default
	m_DllFlag=GetInstantFlag();
	m_Obj1=(-1);

	m_TurnCounter=0;
	m_NumCPUScavs=0; // Nothing in this list now.


	m_SiegeOn=false;
	m_AntiAssault=false;
	m_SiegeCounter=0;
	m_AssaultCounter=0;
	m_LateGame=false;
	m_HaveArmory=false;

	m_Powerup1=NULL;
	m_Powerup2=NULL;
	m_Powerup3=NULL;
	m_Powerup4=NULL;

	PreloadODF("ivrecy");
	PreloadODF("fvrecy");
	PreloadODF("ivrecycpu");
	PreloadODF("fvrecycpu");

	m_LastTauntPrintedAt=-2000; // Force first message to be printed...
	DoTaunt(TAUNTS_GameStart);
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
		if(IsAround(checkScav) && (GetTeamNum(checkScav) == m_CompTeam))
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


// Note: *NEVER* remove this object from within the AddObject() call.
// It can cause crashes. If you must delete it, wait until the next
// Execute() or something.  NM 3/22/02
void instantMission::AddObject(Handle h)
{
	// Always grab the ODF name out, if possible. Makes life much easier
	// in here.
	char ODFName[64];
	if(GetObjInfo(h, Get_CFG, ODFName))
	{
		// Local varb is now setup... 
	}
	else
	{
		// Failed to get anything. Uhoh. Clear this so our strcmps will be
		// guaranteed to fail.
		ODFName[0] = 0;
	}

	char ObjClass[64];
	GetObjInfo(h, Get_GOClass, ObjClass);

	// Always get one random # in 0..1, for scion shield randomization
	float fRandomNum = GetRandomFloat(1.0f);

	int Team = GetTeamNum(h);

	bool IsCommBunker = (_stricmp(ObjClass, "CLASS_COMMBUNKER") == 0) || 
		(_stricmp(ObjClass, "CLASS_COMMTOWER") == 0);
	if(IsCommBunker && (Team == m_CompTeam))
		++m_CPUCommBunkerCount;

	bool IsRecyVehicle = (_stricmp(ObjClass, "CLASS_RECYCLERVEHICLE") == 0);
	if(IsRecyVehicle)
	{
		// If we're not tracking a m_Recycler vehicle for this team right
		// now, store it.
		if(Team == m_CompTeam)
		{
			if(m_EnemyRecycler == 0)
				m_EnemyRecycler = h;
		}
		if(Team == m_StratTeam)
		{
			m_Recycler = h;
		}
	}

	if(Team == m_CompTeam)
	{
		// Scav cleanup code added NM 7/20/01.

		// Better way to identify scavs added NM 1/29/05 -- checks for
		// regular & hoverscavs, no matter what their ODF is named.
		char ObjClass[64];
		GetObjInfo(h, Get_GOClass, ObjClass);

		if((_stricmp(ObjClass, "CLASS_SCAVENGER") == 0) || 
		   (_stricmp(ObjClass, "CLASS_SCAVENGERH") == 0))
		{
			AddCPUScav(h);
		}

		SetSkill(h, m_Difficulty+1);
		if(_stricmp(ObjClass, "CLASS_ARMORY") == 0)
		{
			m_HaveArmory=true;
		}

		if(m_HaveArmory)
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
#ifdef _DEBUG
		// Nerf for testing.
		SetSkill(h, 0);
		SetMaxHealth(h, 50);
		SetCurHealth(h, 50);
		SetMaxAmmo(h, 50);
		SetCurAmmo(h, 50);
#endif
	} // on computer team
	else
		if((m_MyGoal==0) && (Team == m_StratTeam))
		{

			// if it's artillery bomber/ go to late game tactics
			if((_stricmp(ObjClass, "CLASS_ARTILLERY") == 0) ||
				(_stricmp(ObjClass, "CLASS_BOMBER") == 0))
			{
				if((!m_SiegeOn) && (!m_LateGame))
				{
					m_LateGame=true;
					SetCPUAIPlan(AIPTypeL);
				}
			}

			if( (_stricmp(ObjClass, "CLASS_HOVER") == 0) || // Seems depreciated. -GBD
				(_stricmp(ObjClass, "CLASS_WINGMAN") == 0) || // Use CLASS_WINGMAN now. -GBD
				(_stricmp(ObjClass, "CLASS_MORPHTANK") == 0) ||
				(_stricmp(ObjClass, "CLASS_ASSAULTTANK") == 0) ||
				(_stricmp(ObjClass, "CLASS_SERVICE") == 0) ||
				(_stricmp(ObjClass, "CLASS_WALKER") == 0))
			{
				SetSkill(h, 3-m_Difficulty);
				SetTeamNum(h, 1);
				//			int grp=GetFirstEmptyGroup();			
				//			SetGroup(h, grp);
			}
		}

		if(Team == m_StratTeam)
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
					if(m_CPUTeamRace == kRACE_SCION) { // CPU is scion
						stratchoice = m_TurnCounter%2;
						switch (stratchoice)
						{
						default:
						case 0:
							SetCPUAIPlan(AIPType1);
							break;
						case 1:
							SetCPUAIPlan(AIPType3);
							break;
						case 2:
							SetCPUAIPlan(AIPType2);
							break;
						}
					}
					else { // CPU is human
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
				} // not past AIP 0
			} else
			{
				if((_stricmp(ObjClass, "CLASS_ASSAULTTANK") == 0) ||
					(_stricmp(ObjClass, "CLASS_WALKER") == 0))
				{
					++m_AssaultCounter;
				}
			} // not a recy
		}

		if((!m_PastAIP0) && (m_TurnCounter > (180 * m_GameTPS)))
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
}

void instantMission::DeleteObject(Handle h)
{
	char ObjClass[64];
	GetObjInfo(h, Get_GOClass, ObjClass);

	if(GetTeamNum(h)==m_StratTeam)
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
		if(_stricmp(ObjClass, "CLASS_ARMORY") == 0)
		{
			m_HaveArmory=false;
		}
	}

}

void instantMission::CreateObjectives()
{
	m_GoalVal=GetInstantGoal();
	switch (m_GoalVal)
	{
	case 0:  //ctf
		m_Obj1=BuildObject("apcamr", 0, "flag");
		if(m_Obj1!=NULL)
		{
			SetObjectiveOn(m_Obj1);
			SetObjectiveName(m_Obj1, "Capture Target");
			ClearObjectives();
			AddObjective("inst0101.otf", WHITE, 8.0f);
		}
		break;
	case 1: // attack
		m_Obj1=BuildObject("mbruin01", m_CompTeam, "objective1");
		if(m_Obj1!=NULL)
		{
			SetObjectiveOn(m_Obj1);
			SetObjectiveName(m_Obj1, "Destroy Target");  // later this could be health/ ammo
			ClearObjectives();
			AddObjective("inst0102.otf", WHITE, 8.0f);
			// we need to know if the goal is 
			// protect-destory
		}
		else (m_Obj1=(-1));  // it never existed

		break;
	case 2: // defend
		m_Obj1=BuildObject("mbruin01", 1, "objective2");
		if(m_Obj1!=NULL)
		{
			SetObjectiveOn(m_Obj1);
			SetObjectiveName(m_Obj1, "Protect Target");
			ClearObjectives();
			AddObjective("inst0103.otf", WHITE, 8.0f);
		}
		break;
	}
	/*
	Again, if this is a strategy instant action..
	some variables should be passed
	*/
}

void instantMission::TestObjectives()
{
#if TEST_SPAWNING
	// Do nothing. Spawning recyclers, etc will clear things
	return;
#else
	if((m_EndCounter==0) && (!IsAlive(m_EnemyRecycler)))
	{
		// Fix for mantis #335 - track if recycler was upgraded or
		// otherwise changed handle.
		Handle tempH = GetObjectByTeamSlot(m_CompTeam, DLL_TEAM_SLOT_RECYCLER);
		if(tempH != 0)
		{
			m_EnemyRecycler = tempH;
		}
		else
		{
			DoTaunt(TAUNTS_CPURecyDestroyed);
			SucceedMission(GetTime()+5.0f, "instantw.txt");
			m_EndCounter++;
		}
	}

	if((m_EndCounter==0) && (!IsAlive(m_Recycler)))
	{
		// Fix for mantis #335 - track if recycler was upgraded or
		// otherwise changed handle.
		Handle tempH = GetObjectByTeamSlot(m_StratTeam, DLL_TEAM_SLOT_RECYCLER);
		if(tempH != 0)
		{
			m_Recycler = tempH;
		}
		else
		{
			DoTaunt(TAUNTS_HumanRecyDestroyed);
			FailMission(GetTime()+5.0f, "instantl.txt");
			m_EndCounter++;
		}
	}
#endif
}


#if TEST_SPAWNING
// Helper code to spawn a ton of objects
void TestSpawning()
{
	// List of vehicles possible to spawn, picked randomly below.
	static char* s_ODFsToSpawn[] = {
		"ivscav", "ivrecy", "ivscout", "ivcons", "ivtank",
		"ivserv", "ivatank", "ivwalk",
		"fvscav", "fvrecy", "fvscout", "fvcons", "fvtank",
		"fvserv", "fvatank", "fvwalk"
	};
	const size_t s_NumODFsToSpawn = _countof(s_ODFsToSpawn);
		
	int i;
	// If we overflowed, delete all at once.
	if(s_TempHandlePosition >= MAX_TEMP_HANDLES)
	{
		for(i=0;i<MAX_TEMP_HANDLES;++i)
		{
			RemoveObject(s_TempHandles[i]);
			s_TempHandles[i] = 0;
		}
		s_TempHandlePosition = 0;
	}

	// Spawn SPAWN_COUNT_PER_EXECUTE items now, as long as there's
	// space in s_TempHandles
	for(i=0;i<SPAWN_COUNT_PER_EXECUTE;++i)
	{
		if(s_TempHandlePosition >= MAX_TEMP_HANDLES)
			break;

		Vector where;
		where.x = GetRandomFloat(768.0f) - 384.f;
		where.z = GetRandomFloat(768.0f) - 384.f;
		where.y = TerrainFindFloor(where.x, where.z) + 1.f + GetRandomFloat(10.f);

		size_t odfIndexToSpawn = static_cast<size_t>(GetRandomFloat(static_cast<float>(s_NumODFsToSpawn)));
		if(odfIndexToSpawn >= s_NumODFsToSpawn)
			odfIndexToSpawn = (s_NumODFsToSpawn-1);

		s_TempHandles[s_TempHandlePosition++] = BuildObject(s_ODFsToSpawn[odfIndexToSpawn], 14, where);
	}

}
#endif


void instantMission::Execute(void)
{
#if TEST_SPAWNING
	TestSpawning();

#endif

	m_ElapsedGameTime++;
	m_Player=GetPlayerHandle();
	m_TurnCounter++;
	DoGenericStrategy();
}


bool instantMission::Load(bool missionSave)
{
	// Always do this to hook up clients with the taunt engine as well.
	InitTaunts(&m_ElapsedGameTime, &m_LastTauntPrintedAt, &m_GameTPS, "Computer Team");

	if(missionSave)
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

		Setup();
		return true;
	}

	bool ret = true;

	// bools
	if(b_array != NULL)
		ret=ret && Read(b_array, b_count);

	// floats
	if(f_array != NULL)
		ret=ret && Read(f_array, f_count);

	// Handles
	if(h_array != NULL)
		ret=ret && Read(h_array, h_count);

	// ints
	if(i_array != NULL)
		ret=ret && Read(i_array, i_count);

	return ret;
}

bool instantMission::Save(bool missionSave)
{
	// Always do this to hook up clients with the taunt engine as well.
	InitTaunts(&m_ElapsedGameTime, &m_LastTauntPrintedAt, &m_GameTPS, "Computer Team");

	if(missionSave)
		return true;

	bool ret = true;

	// bools
	if(b_array != NULL)
		ret=ret && Write(b_array, b_count);

	// floats
	if(f_array != NULL)
		ret=ret && Write(f_array, f_count);

	// Handles
	if(h_array != NULL)
		ret=ret && Write(h_array, h_count);

	// ints
	if(i_array != NULL)
		ret=ret && Write(i_array, i_count);

	return ret;
}

bool instantMission::PostLoad(bool missionSave)
{
	if(missionSave)
		return true;

	if(h_array != NULL)
		ConvertHandles(h_array, h_count);

	return true;
}

int Rand(void)
{
	return (int)GetRandomFloat(32768);
}

void instantMission::DoGenericStrategy()
{
	m_TimeCount++;

	if(!m_StartDone)
	{
		m_StartDone=true;

		m_MyGoal=GetInstantGoal();

		// Style notes: we copy out all the options.instant.* varbs we
		// want into class member variables the first chance we get. If
		// the user exits to windows, and reloads a savegame, only these
		// class member variables will be reloaded. The options.* will be
		// lost.

		// true if the user can respawn if died (kill/snipe)
		m_CanRespawn = IFace_GetInteger("options.instant.bool0");

		// Clear this on entry. It might be filled in below.
		memset(m_CustomAIPStr, 0, sizeof(m_CustomAIPStr));

		m_AwareV13 = IFace_GetInteger("options.instant.awarev13");
#ifdef _DEBUG
		m_AwareV13 = true;
#endif
		if(m_AwareV13)
		{

			// Using custom AIPs? Read from string.
			IFace_GetString("options.instant.string0", m_CustomAIPStr, m_CustomAIPStrLen);

			m_CPUTeamRace = IFace_GetInteger("options.instant.hisrace");
			m_HumanTeamRace = IFace_GetInteger("options.instant.myrace");
#ifdef _DEBUG
			if(m_CPUTeamRace == 0)
				m_CPUTeamRace = 'i';
			if(m_HumanTeamRace == 0)
				m_HumanTeamRace = 'i';
#endif

			// Override whatever craft the BZN had for the m_Player, make them
			// a scout of the correct race.
			Handle PlayerH = GetPlayerHandle();
			_ASSERTE(PlayerH != 0);
			int PlayerTeam = GetTeamNum(PlayerH);
			Vector PlayerPos;
			GetPosition(PlayerH, PlayerPos);

			char NewPlayerODF[64];
			sprintf_s(NewPlayerODF, "%cvscout", m_HumanTeamRace);

			char OldODFName[64];
			if(GetObjInfo(h, Get_CFG, OldODFName))
			{
				// Got their cfg. Check if it's a pilot, and change to that if
				// the BZN wanted them to start as that.
				if(OldODFName[1] == 's')
					sprintf_s(NewPlayerODF, "%spilo", m_HumanTeamRace);
			}

			// Remove old object
			RemoveObject(PlayerH);
			// Build new object
			Handle NewPlayerH = BuildObject(NewPlayerODF, PlayerTeam, PlayerPos);

			SetAsUser(NewPlayerH, PlayerTeam);
		}
		else
		{
			// Old way of determining sides
			int mySide=GetInstantMySide();
			if(mySide == 1) { // CPU Scion, Human ISDF
				m_CPUTeamRace = kRACE_SCION;
				m_HumanTeamRace = kRACE_ISDF;
			}
			else
			{
				m_CPUTeamRace = kRACE_ISDF;
				m_HumanTeamRace = kRACE_SCION;
			}
		}

		m_MyForce=GetInstantMyForce();
		m_CompForce=GetInstantCompForce();
		m_Difficulty=GetInstantDifficulty();



		if(m_MyGoal==0)
		{
			m_StratTeam=3;
			Ally(3, 1);
			Ally(1, 3);

			Handle atk = 0;
			if(m_CPUTeamRace == kRACE_SCION)
			{
				atk=BuildObject("fvsent", m_CompTeam, "tankEnemy1");
			}
			else
				atk=BuildObject("ivmisl", m_CompTeam, "tankEnemy1");

			Attack(atk, m_Player);  // spice up the start

			// add power ups here.. or have a function for it.  
		}
		else 
		{
			m_StratTeam=1;
		}

		SetupExtraVehicles();

		// Build initial items for CPU team based on selections. The first
		// ODF listed is preferred, the second one really should be there
		// as a fallback

		// Note: contents of options.instant.string# not copied to member
		// varbs, as recommended above. Why not? It's only used in
		// creating an object, and then that object will save enough data
		// to reload successfully later.
		char CustomCPURecy[64];
		IFace_GetString("options.instant.string2", CustomCPURecy, sizeof(CustomCPURecy));
		if(CustomCPURecy[0]) 
			m_EnemyRecycler=BuildStartingVehicle(m_CompTeam, m_CPUTeamRace, CustomCPURecy, "*vrecy", "RecyclerEnemy");
		else
			m_EnemyRecycler=BuildStartingVehicle(m_CompTeam, m_CPUTeamRace, "*vrecycpu", "*vrecy", "RecyclerEnemy");

		BuildStartingVehicle(m_CompTeam, m_CPUTeamRace, "*vturr", "*vturr", "turretEnemy1");
		BuildStartingVehicle(m_CompTeam, m_CPUTeamRace, "*vturr", "*vturr", "turretEnemy2");

		if(m_CompForce>0)
		{
			BuildStartingVehicle(m_CompTeam, m_CPUTeamRace, "*bspir", "*vturr", "gtow2");
			BuildStartingVehicle(m_CompTeam, m_CPUTeamRace, "*bspir", "*vturr", "gtow3");
			BuildStartingVehicle(m_CompTeam, m_CPUTeamRace, "*vsent", "*vscout", "SentryEnemy1");
			BuildStartingVehicle(m_CompTeam, m_CPUTeamRace, "*vsent", "*vscout", "SentryEnemy2");
		}
		if(m_CompForce>1)
		{
			BuildStartingVehicle(m_CompTeam, m_CPUTeamRace, "*bspir", "*vturr", "gtow4");
			BuildStartingVehicle(m_CompTeam, m_CPUTeamRace, "*bspir", "*vturr", "gtow5");
			BuildStartingVehicle(m_CompTeam, m_CPUTeamRace, "*vtank", "*vtank", "tankEnemy1");
			BuildStartingVehicle(m_CompTeam, m_CPUTeamRace, "*vtank", "*vtank", "tankEnemy2");
			BuildStartingVehicle(m_CompTeam, m_CPUTeamRace, "*vtank", "*vtank", "tankEnemy3");
			BuildStartingVehicle(m_CompTeam, m_CPUTeamRace, "*vtank", "*vtank", "TankEnemy3");
			BuildStartingVehicle(m_CompTeam, m_CPUTeamRace, "*vsent", "*vscout", "SentryEnemy3");
		}
		BuildStartingVehicle(m_CompTeam, m_CPUTeamRace, "*vscav", "*vscav", "ScavengerEnemy");


		// --------------------------------------------------------
		// Build initial items for Human team based on selections

		int grp=GetFirstEmptyGroup();

		// Note: contents of options.instant.string# not copied to member
		// varbs, as recommended above. Why not? It's only used in
		// creating an object, and then that object will save enough data
		// to reload successfully later.

		// If there is a custom human m_Recycler, use that.
		char CustomHumanRecy[64];
		IFace_GetString("options.instant.string1", CustomHumanRecy, sizeof(CustomHumanRecy));
		if(CustomHumanRecy[0]) 
			m_Recycler=BuildStartingVehicle(m_StratTeam, m_HumanTeamRace, CustomHumanRecy, "*vrecy", "Recycler", grp);
		else
			m_Recycler=BuildStartingVehicle(m_StratTeam, m_HumanTeamRace, "*vrecy", "*vrecy", "Recycler", grp);

		grp=GetFirstEmptyGroup();
		BuildStartingVehicle(m_StratTeam, m_HumanTeamRace, "*vscav", "*vscav", "Scavenger", grp);

		grp=GetFirstEmptyGroup();
		BuildStartingVehicle(m_StratTeam, m_HumanTeamRace, "*vturr", "*vturr", "Turret1", grp);
		BuildStartingVehicle(m_StratTeam, m_HumanTeamRace, "*vturr", "*vturr", "Turret2", grp);
		BuildStartingVehicle(m_StratTeam, m_HumanTeamRace, "*vturr", "*vturr", "Turret3", grp);

		grp=GetFirstEmptyGroup();
		BuildStartingVehicle(m_StratTeam, m_HumanTeamRace, "*vscout", "*vscout", "Scout1", grp);
		BuildStartingVehicle(m_StratTeam, m_HumanTeamRace, "*vscout", "*vscout", "Scout2", grp);
		BuildStartingVehicle(m_StratTeam, m_HumanTeamRace, "*vscout", "*vscout", "Scout3", grp);

		grp=GetFirstEmptyGroup();
		BuildStartingVehicle(m_StratTeam, m_HumanTeamRace, "*vtank", "*vtank", "Tank1", grp);
		BuildStartingVehicle(m_StratTeam, m_HumanTeamRace, "*vtank", "*vtank", "Tank2", grp);
		BuildStartingVehicle(m_StratTeam, m_HumanTeamRace, "*vtank", "*vtank", "Tank3", grp);
		if(m_MyForce>1)
		{
			BuildStartingVehicle(m_StratTeam, m_HumanTeamRace, "*vtank", "*vtank", "tank1", grp);
			BuildStartingVehicle(m_StratTeam, m_HumanTeamRace, "*vtank", "*vtank", "tank2", grp);
			BuildStartingVehicle(m_StratTeam, m_HumanTeamRace, "*vtank", "*vtank", "tank3", grp);
		}

		// Only old-style maps w/ opposite races can do thug mode
		if(!m_AwareV13)
		{
			if(m_MyGoal==0)
			{
				// check my team..
				if(m_HumanTeamRace == kRACE_ISDF)
				{
					SetPlan("isdfteam.aip", m_StratTeam);
				}
				else
				{
					SetPlan("scionteam.aip", m_StratTeam);
				}	
			}
		} // Not aware of 1.3

		// If we jumped to AIP 1/3 because there's a preplaced human
		// recy on the map, then don't go back to AIP 0
		if(!m_PastAIP0)
		{
			SetCPUAIPlan(AIPType0);
		}

		// do enemies
		//					ClearObjectives();
		//					AddObjective("isdf0501.otf", WHITE, 5.0f);

		// power ups?
		BuildObject("apammo", 0, "ammo1");
		BuildObject("apammo", 0, "ammo2");
		BuildObject("apammo", 0, "ammo3");
		BuildObject("aprepa", 0, "repair1");
		BuildObject("aprepa", 0, "repair2");
		BuildObject("aprepa", 0, "repair3");
		// objectives .. later make this optional	
		//		CreateObjectives();
		SetScrap(m_CompTeam, 40);
		SetScrap(1, 40);
	}

	int CompForceSkew = m_CompForce * m_GameTPS;
	if(CompForceSkew >= (3 * m_GameTPS))
		CompForceSkew = (3 * m_GameTPS) - 2;

	if(m_TimeCount%((3 * m_GameTPS) - (CompForceSkew))==0)  // this speeds up resources
		AddScrap(m_CompTeam, 1);
	if(m_TimeCount%((10 * m_GameTPS) - (m_MyForce * 2 * m_GameTPS))==0)
		AddScrap(m_StratTeam, 1);
	if(m_TimeCount%m_FriendReinforcementTime==0)
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

	if(m_MyGoal==0)
	{
		m_PowerupCounter++;
		if((m_PowerupCounter % (20 * m_GameTPS))==0)
		{
			if(!IsAround(m_Powerup1))
			{
				m_Powerup1=BuildObject("apshdw", 0, "power1");
			}
			if(!IsAround(m_Powerup2))
			{
				m_Powerup2=BuildObject("apsstb", 0, "power2");
			}
			if(!IsAround(m_Powerup3))
			{
				m_Powerup3=BuildObject("apffld", 0, "power3");
			}
			if(!IsAround(m_Powerup4))
			{
				m_Powerup4=BuildObject("apspln", 0, "power4");
			}
		}
	}

	// AIP switch logic
	if(!m_SiegeOn)
	{
		// Ignore scavs, pilots, and any craft > kfSIEGE_DISTANCE away.
		Handle enemy = GetNearestEnemy(m_EnemyRecycler, true, true, kfSIEGE_DISTANCE);
		if(enemy) 
		{
			// under siege
			m_SiegeCounter++;
		}
		else
		{
			m_SiegeCounter=0;
		}

		if(m_SiegeCounter > (45 * m_GameTPS))
		{
			m_SiegeOn = true;
			SetCPUAIPlan(AIPTypeS);
		}
	}  // !m_SiegeOn
	else
	{
		// Ignore scavs, pilots, and any craft > kfSIEGE_DISTANCE away.
		Handle enemy = GetNearestEnemy(m_EnemyRecycler, true, true, kfSIEGE_DISTANCE);
		if(enemy == 0)
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
	if((!m_LateGame) && (!m_SiegeOn) && (!m_AntiAssault) && (m_AssaultCounter>2))
	{
		m_AntiAssault=true;
		SetCPUAIPlan(AIPTypeA);
	}
	else
	{
		if((m_AntiAssault) && (m_AssaultCounter<3))
		{
			m_AntiAssault=false;
			SetCPUAIPlan(AIPTypeL);
		}
	}

	TestObjectives();
}

EjectKillRetCodes instantMission::PlayerEjected(Handle DeadObjectHandle)
{
	// Always allow user to eject
	return DoEjectPilot;
}

// Function never called - NM 10/30/04
// EjectKillRetCodes instantMission::PlayerKilled(int KillersHandle)
// {
// 	return DoEjectPilot;  // Game over, man.
// }

// Local user died. Do (optional) respawn or fail mission
EjectKillRetCodes instantMission::PlayerDied(int DeadObjectHandle, bool bSniped)
{

	// Player (local or remote human) is dead
	if(!IsPerson(DeadObjectHandle))
	{
		// Craft died. If it wasn't a snipe, then just kick the pilot out
		if(!bSniped)
			return DoEjectPilot;
	}

	// Local m_Player got killed (pilot or snipe). Do respawn as needed

	if(m_CanRespawn && IsAlive(m_Recycler))
	{
		// Respawn them near their m_Recycler, up in the air.
		Vector Where;
		GetPosition(m_Recycler, Where);

		Where=GetPositionNear(Where, 10.0f, 50.0f);
		Where.y += 50.0f;

		char PlayerODF[64];
		sprintf_s(PlayerODF, "%cspilo", m_HumanTeamRace);
		Handle PlayerH=BuildObject(PlayerODF, 1, Where);
		SetAsUser(PlayerH, 1);
		AddPilotByHandle(PlayerH);

		DoTaunt(TAUNTS_HumanShipDestroyed);
	}
	else
	{
		// User can't respawn. Game over, man
		FailMission(GetTime() + 3.0f);
	}

	// Both cases of the above report that we handled things
	return DLLHandled;
}

EjectKillRetCodes instantMission::ObjectKilled(int DeadObjectHandle, int KillersHandle)
{
	if(!IsPlayer(DeadObjectHandle)) { // J.Random AI-controlled Object is toast
		// Should we eject a pilot instead?
		bool bWasDeadPilot = IsPerson(DeadObjectHandle);

		if(!bWasDeadPilot)
			return DoEjectPilot;
		else
			return DLLHandled;
	}
	else
	{
		return PlayerDied(DeadObjectHandle, false);
	}
}

EjectKillRetCodes instantMission::ObjectSniped(int DeadObjectHandle, int KillersHandle)
{
	if(!IsPlayer(DeadObjectHandle))
	{
		// J.Random AI-controlled Object is toast
		return DLLHandled;
	}
	else
	{
		return PlayerDied(DeadObjectHandle, true);
	} // Player dead
}

DLLBase * BuildMission(void)
{
	return new instantMission();
}

