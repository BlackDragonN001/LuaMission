#include "ScriptUtilsExtention.h"
#include <math.h>
#include <memory.h>
#include <malloc.h>
#include <stdarg.h>
#include <share.h>
//#include <time.h>
//#include <ctype.h>


// Battlezone 2 ScriptUtils Extention written by General BlackDragon, Ken Miller, and Nielk1.

// This stores a list of all currently open ODFs for this session.
stdext::hash_map<unsigned long, ODFName> ODFNameMap;
// This stores all the failed attempted open ODFs so we don't try them more then once.
stdext::hash_map<unsigned long, ODFName> ODFNameBlackListMap;
// This stored a list of all currently open Files for this session.
stdext::hash_map<const char*, FILE*> FileNameMap;

// List all AI Commands. !!! Update based on aiCommands.h should it ever change! NOTE: # of entries must == NUM_CMD.
const char *CommandList[NUM_CMD] = { "CMD_NONE", "CMD_SELECT", "CMD_STOP", "CMD_GO", "CMD_ATTACK", "CMD_FOLLOW", "CMD_FORMATION", "CMD_PICKUP",
"CMD_DROPOFF", "CMD_UNDEPLOY", "CMD_DEPLOY", "CMD_NO_DEPLOY", "CMD_GET_REPAIR", "CMD_GET_RELOAD", "CMD_GET_WEAPON", "CMD_GET_CAMERA", "CMD_GET_BOMB",
"CMD_DEFEND", "CMD_RESCUE", "CMD_RECYCLE", "CMD_SCAVENGE", "CMD_HUNT", "CMD_BUILD", "CMD_PATROL", "CMD_STAGE", "CMD_SEND", "CMD_GET_IN",
"CMD_LAY_MINES", "CMD_LOOK_AT", "CMD_SERVICE", "CMD_UPGRADE", "CMD_DEMOLISH", "CMD_POWER", "CMD_BACK", "CMD_DONE", "CMD_CANCEL", "CMD_SET_GROUP",
"CMD_SET_TEAM", "CMD_SEND_GROUP", "CMD_TARGET", "CMD_INSPECT", "CMD_SWITCHTEAM", "CMD_INTERFACE", "CMD_LOGOFF", "CMD_AUTOPILOT", "CMD_MESSAGE",
"CMD_CLOSE", "CMD_MORPH_SETDEPLOYED", "CMD_MORPH_SETUNDEPLOYED", "CMD_MORPH_UNLOCK", "CMD_BAILOUT", "CMD_BUILD_ROTATE", "CMD_CMDPANEL_SELECT",
"CMD_CMDPANEL_DESELECT" };


/////////////////////////////////////////////////////////////
// Things from Bob "BS-ser" Stewert's DLL Scriptor.
/////////////////////////////////////////////////////////////

// Code from Bob "BS-er" Stewart. Slight tweaks by General BlackDragon. (portable_sin/atan2)
Vector HRotateFront(const Vector Front, const float HAngleDifference)
{
	Vector FinalVector(0, 0, 0);
	float Angle = 0.0f;
	float Scale = sqrt(1.0f - Front.y * Front.y);

	// Get the horizontal angle of the initial directional vector.
	Angle = portable_atan2(Front.x, Front.z);

	// Add the rotation amount to the current angle, and convert back into a directional vector.
	Angle += HAngleDifference;
	FinalVector.x = portable_sin(Angle) * Scale;
	FinalVector.y = Front.y;
	FinalVector.z = portable_cos(Angle) * Scale;

	return FinalVector;
}
// Code from Bob "BS-er" Stewart.
void SetAngle(const Handle h, const float Degrees)
{
	if (!IsAround(h))
		return;

	Vector Front = HRotateFront(Vector(0.0f, 0.0f, 1.0f), Degrees * DEG_2_RAD);
	Matrix Position = Build_Directinal_Matrix(GetPosition(h), Front); // GetPosition2?
	SetPosition(h, Position);
	SetVectorPosition(h, Position.posit);
}

// Code originally from Bob "BS-er" Stewart. Re-write by Ken Miller and General BlackDragon.
float CameraTime = -1.0f;
Vector CameraPosition(0, 0, 0);
bool CameraPos(const Handle me, const Handle him, const Vector &PosA, const Vector &PosB, const float Increment)
{
	// Compute the difference vector between the starting and ending positions
	Vector CameraDiffVector = Sub_Vectors(PosB, PosA);
	// Advance the camera position
	float Distance = GetLength3D(CameraDiffVector);

	// Compute the time when the camera shot must end
	if (CameraTime < 0.0f)
	{
		float Time = Distance / (Increment * 0.01f);
		CameraPosition = PosA;
		CameraTime = (GetTime() + Time);
	}
	else
	{
		float Scale = Increment / (Distance * 100 * GetTPS() * 2); // Why is the 2 there? -GBD
		CameraPosition = Add_Mult_Vectors(CameraPosition, CameraDiffVector, Scale); // Increment the Position.
	}

	if ((CameraObject(me, CameraPosition.x, CameraPosition.y, CameraPosition.z, him)) || (GetTime() > CameraTime) || (!IsAround(him)))
	{
		CameraTime = -1.0f;
		return true; // Done here.
	}

	return false;
}

/////////////////////////////////////////////////////////////
// New things :)
/////////////////////////////////////////////////////////////

// Replaces an object with another object, retaining as much information from the original as desired. 
Handle ReplaceObject(const Handle H, const char *ODF, const int Team, const float HeightOffset, const int Empty, const bool RestoreWeapons, const int Group, const int CanSnipe, const bool KeepCommand, const int NewCommand, const Handle NewWho, const Vector NewWhere)
{
	if (!IsAround(H))
		return 0;

	HandleInfo hinfo;
	memset(&hinfo, 0, sizeof(hinfo));

	// Look for custom ODF, if we speicfy a different one, otherwise use this object's ODF.
	if (!ODF)
	{
		GetODFName(H, hinfo.ODFName);
	}
	else
	{
		//strcpy_s(hinfo.ODFName, ODF);
		CombineStrings(hinfo.ODFName, "", ODF);
	}

	// Retrieve all Handle Info.
	hinfo.Tug = GetTug(H);
	hinfo.Target = GetTarget(H);
	hinfo.CurrWho = GetCurrentWho(H);
	for (int x = 0; x < MAX_TAPS; x++)
		hinfo.Tap[x] = GetTap(H, x);
	const char *Label = GetLabel(H);
	//strcpy_s(hinfo.Label, Label);
	CombineStrings(hinfo.Label, "", Label, MAX_MESSAGE_LENGTH);
	const char *Name = GetObjectiveName(H);
	//strcpy_s(hinfo.ObjectiveName, Name);
	CombineStrings(hinfo.ObjectiveName, "", Name, MAX_NAME_LENGTH);
	GetPilotConfig(H, hinfo.PilotClass);
	hinfo.Team = Team >= 0 ? Team : GetTeamNum(H); // If we override Team, do that, or use existing team.
	hinfo.PerceivedTeam = GetPerceivedTeam(H);
	//hinfo.MaxHealth = GetMaxHealth(H); // Just use Percentage, in case we're using different ODFs.
	//hinfo.MaxAmmo = GetMaxAmmo(H);
	//hinfo.CurHealth = GetCurHealth(H);
	//hinfo.CurAmmo = GetCurAmmo(H);
	hinfo.HealthP = GetHealth(H);
	hinfo.AmmoP = GetAmmo(H);
	// Do Weapons.
	for (int x = 0; x < MAX_HARDPOINTS; x++)
	{
		hinfo.LocalAmmo[x] = GetCurLocalAmmo(H, x);
		GetObjInfo(H, ObjectInfoType(Get_Weapon0Config + x), hinfo.Weapons[x]);
	}
	hinfo.WeaponMask = GetWeaponMask(H);
	//hinfo.CurScrap = GetScavengerCurScrap(H); // Just use Percentage, in case we're using different ODFs.
	//hinfo.MaxScrap = GetScavengerMaxScrap(H);
	hinfo.ScrapP = GetScavengerScrap(H);
	hinfo.Position = GetMatrixPosition(H);
	hinfo.Velocity = GetVelocity(H);
	hinfo.Omega = GetOmega(H);
	hinfo.IsDeployed = IsDeployed(H);
	hinfo.LifeSpan = GetRemainingLifespan(H);
	hinfo.Independence = GetIndependence(H);
	hinfo.Skill = GetSkill(H);
	hinfo.Group = Group >= 0 ? Group : GetGroup(H) + 1; // If we override Group, do that, or use existing team.
	hinfo.CanSnipe = CanSnipe >= 0 ? CanSnipe : GetCanSnipe(H); // If we override Group, do that, or use existing team.
	hinfo.CurrCommand = GetCurrentCommand(H);
	switch (hinfo.CurrCommand)
	{
	case CMD_GO:
	case CMD_LAY_MINES:
	case CMD_GET_IN:
	case CMD_DEPLOY:
		hinfo.CurrWhere = GetCurrentCommandWhere(H);
		break;
	}
	//hinfo.HasPilot = HasPilot(H);
	hinfo.IsPlayer = IsPlayer(H);
	// Custom variables.
	hinfo.TapCount = GetTapCount(H);
	hinfo.IsEmpty = Empty >= 0 ? Empty : IsEmpty(H); // If we override Empty, do that, or use existing setting.

	if (HeightOffset != 0)
		hinfo.Position.posit.y += HeightOffset;

	Handle h = 0;

	if (DoesODFExist(hinfo.ODFName))
	{
		// Special: Do we need to restore taps?
		bool DestroyTapsWithParent = true;
		if ((GetODFBool(H, "PoweredBuildingClass", "DestroyTapsWithParent", &DestroyTapsWithParent, true)) ||
			(GetODFBool(H, "TurretCraftClass", "DestroyTapsWithParent", &DestroyTapsWithParent, true)))
		{
			// Got it.
		}

		RemoveObject(H);
		h = BuildObject(hinfo.ODFName, hinfo.Team, hinfo.Position);

		if (hinfo.Tug) // This object was being tugged, try to restore that?
			Pickup(hinfo.Tug, h);
		if (hinfo.Target)
			SetTarget(h, hinfo.Target);
		// If we had taps, and they wern't deleted, re-assign them.
		if (!DestroyTapsWithParent)
		{
			for (int x = 0; x < hinfo.TapCount; x++)
				SetTap(h, x, hinfo.Tap[x]);
		}
		SetLabel(h, hinfo.Label);
		if ((hinfo.PilotClass[0]) && (DoesODFExist(hinfo.PilotClass)))
			SetPilotClass(h, hinfo.PilotClass);
		SetObjectiveName(h, hinfo.ObjectiveName);
		SetPerceivedTeam(h, hinfo.PerceivedTeam);
		//SetMaxHealth(h, hinfo.MaxHealth); // Use percentage below.
		//SetMaxAmmo(h, hinfo.MaxAmmo);
		//SetCurHealth(h, hinfo.CurHealth);
		//SetCurAmmo(h, hinfo.CurAmmo);
		SetHealth(h, hinfo.HealthP);
		SetAmmo(h, hinfo.AmmoP);
		ReplaceWeapons(h, hinfo.Weapons, hinfo.LocalAmmo);
		SetWeaponMask(h, hinfo.WeaponMask);
		//SetScavengerCurScrap(h, hinfo.CurScrap); // Use percentage below.
		//SetScavengerMaxScrap(h, hinfo.MaxScrap);
		SetScavengerScrap(h, hinfo.ScrapP);
		//SetPosition(h, hinfo.Position); // This should match already...
		SetVelocity(h, hinfo.Velocity);
		SetOmega(h, hinfo.Omega);
		if (hinfo.IsDeployed)
			Deploy(h);
		if (hinfo.LifeSpan >= 0)
			SetLifespan(h, hinfo.LifeSpan);
		if (hinfo.Independence >= 0) // Make sure it's a valid unit process, e.g. Not a building > unit transition. (prod units) -GBD
			SetIndependence(h, hinfo.Independence);
		SetSkill(h, hinfo.Skill);
		// Special group behavior, if Group passed into function is 0, don't put it in a group, if -1, use old group, else put it in the specified group.
		if (Group < 0)
			SetBestGroup(h);
		else if (Group > 0)
			SetGroup(h, hinfo.Group - 1);

		SetCanSnipe(h, hinfo.CanSnipe);

		if (hinfo.IsPlayer)
		{
			SetAsUser(h, hinfo.Team);
		}
		else
		{
			if (hinfo.IsEmpty)
			{
				RemovePilotAI(h);
			}
			else
			{
				if (NewCommand >= 0)
					SetCommand(h, NewCommand, 0, NewWho, NewWhere);
				else if (KeepCommand)
					SetCommand(h, hinfo.CurrCommand, 0, hinfo.CurrWho, hinfo.CurrWhere);
			}
		}

		if (RestoreWeapons)
			ReplaceWeapons(h, hinfo.Weapons, hinfo.LocalAmmo, hinfo.ODFName);
	}
	else
	{
		FormatConsoleMessage("ERROR: ReplaceODF '%s' doesn't exist! Fix DLLs!", hinfo.ODFName);
		return H; // Report back the same handle, didn't work.
	}

	return h;
}

// Replace an object's weapons, with optional 2nd object ODF. Takes a Handle and an array of PreviousWeapons[MAX_HARDPOINTS]. Returns false if the handle passed in doesn't exist.
// NOTE: The array passed into these MUST have a size of 5. NewWeapons[MAX_HARDPOINTS] is optional, if not present it will restore old weapons from PreviousODF, if present.
bool ReplaceWeapons(const Handle h, const char NewWeapons[][MAX_ODF_LENGTH], const float PreviousLocalAmmo[], const char *PreviousODF)
{
	char ODFName[MAX_ODF_LENGTH] = { 0 };
	if (!GetObjInfo(h, Get_ODF, ODFName)) // Get the ODF.
		return false;

	bool Logging = false; // GetVarItemInt("network.session.ivar115");

	char OldODFName[MAX_ODF_LENGTH] = { 0 };
	//strcpy_s(OldODFName, PreviousODF ? PreviousODF : ODFName);
	CombineStrings(OldODFName, "", PreviousODF ? PreviousODF : ODFName);

	CheckODFName(OldODFName); // Removes the :# or .odf appended to file name.

	//if (!DoesFileExist(OldODFName))
	//if (_stricmp(OldODFName + strlen(OldODFName) - 4, ".odf") != 0)
	CombineStrings(OldODFName, OldODFName, ".odf"); //strcat_s(OldODFName, ".odf"); // Add .odf to the end.

	// New weapon info.
	char ReplaceWeapon[MAX_HARDPOINTS][MAX_ODF_LENGTH] = { 0 };
	unsigned long ReplaceWeaponClass[MAX_HARDPOINTS] = { 0 };
	bool ReplaceWeaponAssault[MAX_HARDPOINTS] = { 0 };
	char ReplaceWeaponAltName[MAX_HARDPOINTS][MAX_ODF_LENGTH] = { 0 };
	float ReplacementLocalAmmo[MAX_HARDPOINTS] = { 0 };

	int NumHardPointTypes[MAX_HARDPOINTS] = { 0 };
	bool BuildWeapons[MAX_HARDPOINTS] = { 0 };
	char CurrWeapon[MAX_HARDPOINTS][MAX_ODF_LENGTH] = { 0 };
	//char StockWeaponClass[MAX_HARDPOINTS][MAX_ODF_LENGTH] = {0};
	unsigned long CurrWeaponClass[MAX_HARDPOINTS] = { 0 };
	//char WeaponClass[MAX_HARDPOINTS][MAX_ODF_LENGTH] = {0};
	unsigned long PrevWeaponClass[MAX_HARDPOINTS] = { 0 };
	bool PrevWeaponAssault[MAX_HARDPOINTS] = { 0 };
	int PrevWeaponGroup[MAX_HARDPOINTS] = { 0 };
	bool UseGroups[MAX_HARDPOINTS] = { 0 };
	char TempODF2[MAX_ODF_LENGTH] = { 0 }; // Second temp odf, for grabbing out classlabel.

	char TempWeaponODF[MAX_ODF_LENGTH] = { 0 }; // Second temp odf, for grabbing out classlabel.
	char TempWeaponODF2[MAX_ODF_LENGTH] = { 0 }; // Second temp odf, for grabbing out classlabel.

	// Get our old ship's Hardpoint info.
	if (OpenODF2(OldODFName))
	{
		char TempClassString[MAX_ODF_LENGTH] = { 0 };
		if ((GetODFString(OldODFName, "GameObjectClass", "classlabel", MAX_ODF_LENGTH, TempClassString))) //&& (DoesODFExist(TempClassString)))
		{
			CombineStrings(TempODF2, TempClassString, ".odf"); //strcat_s(TempODF2, ".odf"); // Add .odf to the end.
			if (!OpenODF2(TempODF2))
				TempODF2[0] = '\0';

		}

		for (int w = 0; w < MAX_HARDPOINTS; w++)
		{
			char WeaponHard[MAX_ODF_LENGTH] = { 0 };
			char DesiredValue[MAX_ODF_LENGTH] = { 0 };
			// Get Weapon Class.
			sprintf_s(DesiredValue, "weaponHard%d", w + 1);
			//GetODFString(OldODFName, TempODF2, "GameObjectClass", DesiredValue, MAX_ODF_LENGTH, PrevWeaponClass[w]);
			if (GetODFString(OldODFName, TempODF2, "GameObjectClass", DesiredValue, MAX_ODF_LENGTH, WeaponHard))
			{
				if (_strnicmp(WeaponHard, "HP_", 3) != 0)
					continue; // Misformat! No HP_ found! Bail!

				char * pUnderscore2 = strchr(WeaponHard + 3, '_');
				if (pUnderscore2 != NULL)
					*pUnderscore2 = '\0'; // Truncate everything after the 2nd _ on.

				if (strlen(WeaponHard) >= 6) // Only if it's still too long. This handles case "HP_GUN".
					WeaponHard[7] = '\0';

				PrevWeaponClass[w] = Hash(WeaponHard);
			}

			// Get Weapon Assault.
			sprintf_s(DesiredValue, "weaponAssault%d", w + 1);
			GetODFBool(OldODFName, TempODF2, "GameObjectClass", DesiredValue, &PrevWeaponAssault[w]);

			// Get Weapon Groups.
			sprintf_s(DesiredValue, "weaponGroup%d", w + 1);
			if (GetODFInt(OldODFName, TempODF2, "GameObjectClass", DesiredValue, &PrevWeaponGroup[w]))
				UseGroups[w] = true;
		}
	}
	// Look up the current weaponHard's of the handle we're giving weapons to.
	if (OpenODF2(ODFName))
	{
		for (int w = 0; w < MAX_HARDPOINTS; w++)
		{
			char WeaponHard[MAX_ODF_LENGTH] = { 0 };
			char DesiredValue[MAX_ODF_LENGTH] = { 0 };
			sprintf_s(DesiredValue, "weaponHard%d", w + 1);
			//GetODFString(h, "GameObjectClass", DesiredValue, MAX_ODF_LENGTH, CurrWeaponClass[w]);
			if (GetODFString(h, "GameObjectClass", DesiredValue, MAX_ODF_LENGTH, WeaponHard))
			{
				if (_strnicmp(WeaponHard, "HP_", 3) != 0)
					continue; // Misformat! No HP_ found! Bail!

				char * pUnderscore2 = strchr(WeaponHard + 3, '_');
				if (pUnderscore2 != NULL)
					*pUnderscore2 = '\0'; // Truncate everything after the 2nd _ on.

				if (strlen(WeaponHard) >= 6) // Only if it's still too long. This handles case "HP_GUN".
					WeaponHard[7] = '\0';

				CurrWeaponClass[w] = Hash(WeaponHard);
			}
		}
	}
	// If the hardpoints don't match, don't continue.
	for (int c = 0; c < MAX_HARDPOINTS; c++)
	{
		if (PrevWeaponClass[c] != CurrWeaponClass[c]) //if (_strnicmp(PrevWeaponClass[c] + 3, CurrWeaponClass[c] + 3, 4) != 0)
			return false; // Done here.
	}

	// Look up our Replacement Weapon's isAssault, altName, and wpnCategory.
	for (int w = 0; w < MAX_HARDPOINTS; w++)
	{
		//strcpy_s(ReplaceWeapons[x], NewWeapons[x]);
		CombineStrings(ReplaceWeapon[w], "", NewWeapons[w]);
		if (!PreviousLocalAmmo)
			ReplacementLocalAmmo[w] = 1e6; //1.0f;
		else
			ReplacementLocalAmmo[w] = PreviousLocalAmmo[w];

		// Read Each Weapon's Assault Flag/AltName/wpnCategory
		CombineStrings(TempWeaponODF, ReplaceWeapon[w], ".odf"); //sprintf_s(TempWeaponODF, "%s.odf", ReplaceWeapon[w]); // Add .odf to the end.
		if (OpenODF2(TempWeaponODF))
		{
			char TempClassString[MAX_ODF_LENGTH] = { 0 };
			if ((GetODFString(TempWeaponODF, "WeaponClass", "classlabel", MAX_ODF_LENGTH, TempClassString))) //&& (DoesODFExist(TempClassString))) // Grab the classlabel.
			{
				CombineStrings(TempWeaponODF2, TempClassString, ".odf"); //strcat_s(TempWeaponODF2, ".odf"); // Add .odf to the end.
				if (!OpenODF2(TempWeaponODF2))
					TempWeaponODF2[0] = '\0';
			}

			GetODFBool(TempWeaponODF, TempWeaponODF2, "WeaponClass", "isAssault", &ReplaceWeaponAssault[w]);
			GetODFString(TempWeaponODF, TempWeaponODF2, "WeaponClass", "altName", MAX_ODF_LENGTH, ReplaceWeaponAltName[w]);

			char WeaponCat[MAX_ODF_LENGTH] = { 0 };
			if (GetODFString(TempWeaponODF, TempWeaponODF2, "WeaponClass", "wpnCategory", MAX_ODF_LENGTH, WeaponCat))
			{
				char TempWeaponCat[MAX_ODF_LENGTH] = { 0 };
				sprintf_s(TempWeaponCat, WeaponCat);
				CombineStrings(WeaponCat, "HP_", TempWeaponCat);
				ReplaceWeaponClass[w] = Hash(WeaponCat);
			}
		}
	}

	// Get the current ship's weapons.
	for (int i = 0; i < MAX_HARDPOINTS; i++)
		GetObjInfo(h, ObjectInfoType(Get_Weapon0Config + i), CurrWeapon[i]);

	//!!-- Get current ship's hardpoint Assault/Group #'s?

	// Count how many hardpoints are in each weaponGroup.
	for (int c = 0; c < MAX_HARDPOINTS; c++)
	{
		//for(int s = c; s < MAX_HARDPOINTS; s++)
		for (int s = 0; s < MAX_HARDPOINTS; s++)
		{
			//	if ((PrevWeaponClass[c][0]) && (PrevWeaponClass[s][0]) && // Both Weapons Classes exist.
			//		(_strnicmp(PrevWeaponClass[c] + 3, PrevWeaponClass[s] + 3, 4) == 0)) // Weapon Classes Match.
			if ((PrevWeaponClass[c]) && (PrevWeaponClass[s]) && (PrevWeaponClass[c] == PrevWeaponClass[s])) // Weapon Classes Match.
			{
				bool Skip = false;
				for (int t = 0; t < MAX_HARDPOINTS; t++)
				{
					/*
					[ARM] Ultraken: SplitWeapons[2][MAX_HARDPOINTS][MAX_ODF_LENGTH] maybe
					[ARM] Ultraken: So, for each entry in NewWeapons, open that ODF, get its altClass, and open that ODF.
					[ARM] Ultraken: Take the altClass ODF, get its isAssault flag, and assign altClass to the corresponding slot in SplitWeapons.
					[ARM] Ultraken: Then take the original ODF, get its isAssault flag, and assign the original to the corresponding slot in SplitWeapons.
					[ARM] Ultraken: You can then use those SplitWeapons indexed by the isAssault of that hardpoint
					[ARM] Ultraken: *shrug*
					[ARM] Ultraken: well, that's what WeaponPowerup does internally
					*/

					//if(((!UseGroups[c]) && (!UseGroups[t]) && (_strnicmp(PrevWeaponClass[s] + 3, PrevWeaponClass[t] + 3, 4) == 0)) && // Both are not using groups, and classes match.
					if (((!UseGroups[c]) && (!UseGroups[t]) && (PrevWeaponClass[s] == PrevWeaponClass[t])) && // Both are not using groups, and classes match.
						(((PrevWeaponAssault[c] == PrevWeaponAssault[t]) && (NumHardPointTypes[t] > 0) && (t < c)) || // Assault matches, number of hardpoints > 0, and t < c.
						((ReplaceWeaponAssault[t]) && (!ReplaceWeaponAltName[t][0]) && (t > s) && (!PrevWeaponAssault[s]) && (_stricmp(ReplaceWeapon[s], CurrWeapon[s]) == 0)))) // Make this skip a previous non assault hardpoint if this weapon is Assault Only.
					{
						if (Logging)
							FormatConsoleMessage("Assault Matchs, Weapon:%d, CompareWeapon:%d, NumHardTypesS: %d, T < C Skipping S. WeaponClassT: '%d'", c, t, NumHardPointTypes[t], PrevWeaponClass[t]);

						Skip = true;
						break;
					}
				}
				if (Skip)
					continue;

				if (
					// Restrict to, If assault matches, and Group Matches, then restrict to 1, else don't !numhardpointtypes.
					// Works for BZ1 stuff. (Groups)
					((((PrevWeaponGroup[c] != PrevWeaponGroup[s]) && (s <= c)) || ((PrevWeaponGroup[c] == PrevWeaponGroup[s]) && ((UseGroups[c]) || (UseGroups[s])) && (s == c))) || // Both dont match, or we our ourselves. count this as a new group.
					// BZ2 Assault combinations.
					((PrevWeaponAssault[c] != PrevWeaponAssault[s]) && (!UseGroups[c]) && (!UseGroups[s]) && (NumHardPointTypes[c] < NumHardPointTypes[s])) || // Weapon assaults dont match, but group overrides.
					// BZ2 Grouped Combat Weapons. (works for isdf rocket tank).
					((PrevWeaponAssault[c] == PrevWeaponAssault[s]) && (!UseGroups[c]) && (!UseGroups[s]) && (s >= c) && (NumHardPointTypes[c] <= NumHardPointTypes[s]))))//&& (NumHardPointTypes[s] < 1) && (NumHardPointTypes[c] < 1)) // Weapon assaults match and groups match, treat as 1 group.
					++NumHardPointTypes[c];

				if (Logging)
				{
					if (((PrevWeaponGroup[c] != PrevWeaponGroup[s]) && (s <= c)) || ((PrevWeaponGroup[c] == PrevWeaponGroup[s]) && ((UseGroups[c]) || (UseGroups[s])) && (s == c)))
						FormatConsoleMessage("Using Groups, Groups Dont Match, S<C, or We are Ourselves, Weapon:%d, CompareWeapon:%d", c, s);
					else if ((PrevWeaponAssault[c] != PrevWeaponAssault[s]) && (!UseGroups[c]) && (!UseGroups[s]) && (NumHardPointTypes[c] < NumHardPointTypes[s]))
						FormatConsoleMessage("Assault NOT Matches,Weapon:%d, CompareWeapon:%d, NumHardTypesC: %d < NumHardTypesS: %d", c, s, NumHardPointTypes[c], NumHardPointTypes[s]);
					else if ((PrevWeaponAssault[c] == PrevWeaponAssault[s]) && (!UseGroups[c]) && (!UseGroups[s]) && (s >= c)) // && (NumHardPointTypes[s] < 1) && (NumHardPointTypes[c] < 1))
						FormatConsoleMessage("Assault Matchs, Weapon:%d, CompareWeapon:%d, NumHardTypesS: %d, NumHardTypesC: %d", c, s, NumHardPointTypes[s], NumHardPointTypes[c]);
				}
			}
		}
	}

	/* // Ken's suggestion for optimizing the loop, Didn't work when 1st hp_cann == same, and 2nd hp_cann == different.
	for (int c = 1; c < MAX_HARDPOINTS; c++)
	{
	for (int s = 0; s < c; s++)
	{
	*/
	for (int c = 0; c < MAX_HARDPOINTS; c++)
	{
		for (int s = 0; s < MAX_HARDPOINTS; s++)
		{
			if (Logging)
				FormatConsoleMessage("Weapon Class C: %d is: '%d', Weapon Class S: %d Is: '%d'", c, PrevWeaponClass[c], s, PrevWeaponClass[s]);

			if ((PrevWeaponClass[c] != 0) &&  // Weapon Class is not null.
				(PrevWeaponClass[s] != 0) &&  // Weapon Compare class is not null.
				//(_strnicmp(PrevWeaponClass[c] + 3, PrevWeaponClass[s] + 3, 4) == 0) && // Weapon Classes Match.
				(PrevWeaponClass[c] == PrevWeaponClass[s]) && // Weapon Classes Match.
				(PrevWeaponClass[c] == ReplaceWeaponClass[c]) && // Weapon is for this type of Hardpoint. !!= New 11/28/16 Test me.
				((c != s) || (_stricmp(ReplaceWeapon[c], CurrWeapon[c]) != 0)) && // This is not the same as itself, or a different weapon then we used to have here.
				((_stricmp(ReplaceWeapon[c], CurrWeapon[c]) != 0) || // New Weapon does not match Original Weapon.
				((s > 0) && (_stricmp(ReplaceWeapon[s], CurrWeapon[s]) != 0)))) // 2nd+ New Weapon doesn't match Original Weapon.
			{
				if (Logging)
					FormatConsoleMessage("Build Weapons C: Weapon Class C: %d is: '%d', Weapon Class S: %d Is: '%d'", c, PrevWeaponClass[c], s, PrevWeaponClass[s]);

				BuildWeapons[c] = true;
			}
		}
	}

	// Handle the weapon replacement. This must go in reverse order. -GBD
	for (int x = 4; x > -1; x--)
	{
		if (Logging)
			FormatConsoleMessage("Index: %d Original Weapon: '%s', New Weapon: '%s', Number of Hardpoints: %d", x, CurrWeapon[x], ReplaceWeapon[x], NumHardPointTypes[x]);

		if ((ReplaceWeapon[x][0]) && (BuildWeapons[x])) // || (_stricmp(ReplaceWeapons[x], StockWeapon[x]) != 0))
		{
			for (int t = 0; t < NumHardPointTypes[x]; t++)
			{
				GiveWeapon(h, ReplaceWeapon[x]);

				if (Logging)
					FormatConsoleMessage("Giving Weapon: '%s', index: %d", ReplaceWeapon[x], x);
			}
		}
	}
	for (int x = 0; x < MAX_HARDPOINTS; x++)
		SetCurLocalAmmo(h, ReplacementLocalAmmo[x], x);

	return true; // Assume it worked...
}

// Conforms an ODFName retrieved with Get_CFG to cutoff the :## appended to it if it was built by a factory. Also removes the . if there's a .odf attached.
void CheckODFName(char *ODFName)
{
	if (!ODFName)
		return; // Null pointer, bad. return! return!

	if (!DoesODFExist(ODFName))
	{
		if (char *dot = strchr(ODFName, ':'))
			*dot = '\0';

		if (!DoesODFExist(ODFName))
		{
			if (char *dot = strrchr(ODFName, '.'))
				*dot = '\0';
		}
	}
}

// Gets the Pilot CFG of the object.
bool GetPilotConfig(const Handle h, char *ODFName)
{
	if (!ODFName)
		return false; // Null pointer, bad. return! return!

	if (const char *pilotcfg = GetPilotConfig(h))
		//strcpy_s(ODFName, sizeof(pilotcfg), pilotcfg);
		CombineStrings(ODFName, "", pilotcfg);
	else
		return false;

	return true;
}
// Version that returns a char *.
char TempGetPilotConfig[MAX_ODF_LENGTH];
const char *GetPilotConfig(const Handle h)
{
	memset(TempGetPilotConfig, 0, sizeof(TempGetPilotConfig)); // Clear this before use.

	if (const char *pilotOdf = GetPilotClass(h))
	{
		strcpy_s(TempGetPilotConfig, pilotOdf);
		if (char *dot = strrchr(TempGetPilotConfig, '.'))
			*dot = '\0';
	} // else { return NULL; } // Don't return NULL, since we don't want a char * null pointer, instead return an empty string, "".

	return TempGetPilotConfig;
}

// Gets the current percentage of scrap inside the scavenger.
float GetScavengerScrap(const Handle h)
{
	int MaxScrap = GetScavengerMaxScrap(h);
	if (MaxScrap <= 0)
		return -1.0f; // Not a Scavvy Thing.

	return float(GetScavengerCurScrap(h) / MaxScrap);
}
// Sets the current percentage of scrap in a Scav.
void SetScavengerScrap(const Handle h, const float Percent)
{
	int MaxScrap = GetScavengerMaxScrap(h);
	if (MaxScrap <= 0)
		return; // Not a Scavvy Thing.

	float NewP = clamp(Percent, 0.0f, 1.0f);
	SetScavengerCurScrap(h, int(MaxScrap * NewP));
}

// Parellel to GetHealth, Sets the health as a % of MaxHealth.
void SetHealth(const Handle h, const float Percent)
{
	//	if(!IsAround(h))
	//		return;
	long Health = GetMaxHealth(h);

	if (Health == -1234)
		return; // Invalid Handle, bail.

	float TempP = clamp(Percent, 0.0f, 1.0f);
	SetCurHealth(h, long(Health * TempP));
}

// Parellel to GetAmmo, Sets the ammo as a % of MaxAmmo.
void SetAmmo(const Handle h, const float Percent)
{
	//	if(!IsAround(h))
	//		return;
	long Ammo = GetMaxAmmo(h);

	if (Ammo == -1234)
		return; // Invalid Handle, bail.

	float TempP = clamp(Percent, 0.0f, 1.0f);
	SetCurAmmo(h, long(Ammo * TempP));
}

// Gets the number of Taps a handle has, optionally excluding invulnerable ones.
int GetTapCount(const Handle h, const bool IgnoreInvincible, const bool AddObject)
{
	int Taps = 0;
	for (int x = 0; x < MAX_TAPS; x++)
	{
		Handle TempTap = GetTap(h, x);
		if (!AddObject)
		{
			if ((TempTap) && ((!IgnoreInvincible) || (GetMaxHealth(TempTap) > 0)))
				++Taps;
		}
		else
		{
			// Special note: It appears AddObject adds a dummy tap to the end of the tap list. This was causing if(TapH) to pass true and save an invalid thing. -GBD
			//	if(TapH) // BAD! Returns true for a second (dummy?) handle of ffffffff (or -1)???? Unsafe. Used ODFName existance to verify tap reality. -GBD
			char TapODFName[MAX_ODF_LENGTH] = { 0 };
			GetObjInfo(TempTap, Get_ODF, TapODFName);
			if (TapODFName[0])
				++Taps;
		}
	}
	return Taps;
}


// Formatted console message.
void FormatConsoleMessage(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	char tempstr[MAX_MESSAGE_LENGTH] = {0};
	vsnprintf_s(tempstr, sizeof(tempstr), sizeof(tempstr), format, ap);
	PrintConsoleMessage(tempstr);
	va_end(ap);
}

// Like AddToMessagesBox2, but takes a formatted string.
void FormatToMessagesBox(const char *format, const unsigned long color, ...)
{
	va_list ap;
	va_start(ap, color);
	char tempstr[MAX_MESSAGE_LENGTH] = { 0 };
	vsnprintf_s(tempstr, sizeof(tempstr), sizeof(tempstr), format, ap);
	AddToMessagesBox2(tempstr, color);
	va_end(ap);
}

// Opens a file and remembers the file name for later closure.
bool FileOpen(const char *Filename, const bool Append)
{
	// already open
	if(FileNameMap.find(Filename) != FileNameMap.end())
	{
		errno_t Error = _wfreopen_s(&FileNameMap[Filename], NULL, Append ? L"a+" : L"w+", FileNameMap[Filename]);
		if(Error)
		{
			char ErrorMessage[MAX_ODF_LENGTH] = {0};
			strerror_s(ErrorMessage, Error);
			FormatConsoleMessage(ErrorMessage);
			return false;
		}
		return true;
	}

	// get the output path
	wchar_t* pRootOutputDir = NULL;
	size_t bufSize = 0;
	GetOutputPath(bufSize, pRootOutputDir);
	wchar_t *pData = static_cast<wchar_t *>(alloca(bufSize*sizeof(wchar_t)));
	if(GetOutputPath(bufSize, pData))
	{
		// generate a full path name
		wchar_t outPath[MAX_MESSAGE_LENGTH] = {0};

		/*
		wchar_t MyDocumentsPath[MAX_MESSAGE_LENGTH] = { 0 };
		SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, MyDocumentsPath);
		swprintf_s(outPath, (_wcsnicmp(pData, MyDocumentsPath, wcslen(MyDocumentsPath)) == 0) ? L"%slogs/%S" : L"%saddon/%S", pData, Filename);
		*/
		swprintf_s(outPath, L"%s%S", pData, Filename); //"%slogs/%S"
		FILE *file = _wfsopen(outPath, Append ? L"a+" : L"w+", _SH_DENYWR); //NULL;

		// try to open the file for append
		if (file)
		{
			// opened successfully
			FileNameMap[Filename] = file;
			return true;
		}
		else
		{
			FormatConsoleMessage("ERROR: Could not open File: '%s'", outPath);
		}
	}
	return false;
}

// Formatted Logfile message. Writes a char string to a log file.
bool FormatFileMessage(const char *Filename, const bool Append, const char *format, ...)
{
	if (FileOpen(Filename, Append))
	{
		va_list ap;
		va_start(ap, format);
		FILE *file = FileNameMap[Filename];
		fprintf(file, format, ap);
		fputs("\n", file);
		va_end(ap);
		return true;
	}
	return false;
}

// Closes a specific file.
void FileClose(const char *Filename)
{
	FileMapIt it = FileNameMap.find(Filename);
	if(it != FileNameMap.end())
	{
		fclose(it->second);
		FileNameMap.erase(it);
	}
}

// Close all open files.
void CloseOpenFiles()
{
	FileMapIt it = FileNameMap.begin();
	while(it != FileNameMap.end())
	{
		if (it->second)
			fclose(it->second);

		it = FileNameMap.erase(it);
	}
}


// Get Vector from a Path point: By Nielk1/Ken Miller.
Vector GetVectorFromPath(const char* path, const int point)
{
	// New code from Ken Miller.
	Vector retVal(0, 0, 0);
	size_t bufSize = 0;

	if (path)
		GetPathPoints(path, bufSize, NULL);

	// Bad Path, abort!
	if(!bufSize)
		return retVal;

	if(point < UnsignedToSigned(bufSize))
	{
		float *pData = static_cast<float *>(_alloca(sizeof(float) * 2 * bufSize));
		if(GetPathPoints(path, bufSize, pData)) 
			retVal = Vector(pData[2*point+0], TerrainFindFloor(pData[2*point+0], pData[2*point+1]), pData[2*point+1]); 
	}

	return retVal;
}

// Returns the current TPS (And sets it for entire game).
int GetTPS(void)
{
	int TPS = 10; // Default.
	EnableHighTPS(TPS);
	return TPS;
}

// IsInfo that uses a handle. No need to know exact ODF name :)
bool IsInfo(const Handle h)
{
	char ODFName[MAX_ODF_LENGTH] = { 0 };
	if (!GetODFName(h, ODFName))
		return false;

	return IsInfo(ODFName); //GetLocalUserInspectHandle() == h
}

// Is something following something?
bool IsFollowing(const Handle me, const Handle him)
{
	if (!IsAround(him)) // IsAround(me) checked by WhoFollowing. :)
		return false;

	return WhoFollowing(me) == him;
}


// Conforms an ODFName retrieved with Get_ODF to cutoff the .odf.
bool GetODFName(const Handle h, char *ODFName)
{
	if (!ODFName)
		return false; // Null pointer, bad. return! return!

	if (!GetObjInfo(h, Get_ODF, ODFName))
		return false; // Invalid Handle.

	if (char *dot = strrchr(ODFName, '.'))
		*dot = '\0';

	return true;
}

// Combines two strings into one. Used to append things to a string safely.
bool CombineStrings(char *ReturnString, const char *String1, const char *String2, const size_t MaxSize)
{
	if ((!String1) || (!String2))
		return false; // Null pointer, bad. return! return!

	if (ReturnString == String2)
	{
		FormatConsoleMessage("ERROR: '%s' cannot append '%s', Return and String2 is itself!", String1, String2);
		return false; // Bad! Pointer string overwrite!
	}

	if ((strlen(String1) + strlen(String2)) > MaxSize)
	{
		FormatConsoleMessage("ERROR: '%s' cannot append '%s', length too long. Make ODF Name shorter!", String1, String2);
		return false;
	}

	if (ReturnString == String1)
		strcat_s(ReturnString, MaxSize, String2);
	else
		sprintf_s(ReturnString, MaxSize, "%s%s", String1, String2);

	return true;
}
// Version that returns a char *.
char TempCombineStrings[MAX_ODF_LENGTH];
const char *CombineStrings(const char *String1, const char *String2)
{
	memset(TempCombineStrings, 0, sizeof(TempCombineStrings)); // Clear this before use.
	CombineStrings(TempCombineStrings, String1, String2);

	return TempCombineStrings;
}

// Checks to see if this ODF is already open, then opens it and adds it to the list of currently open odfs. Note: You MUST call CloseOpenODFs() at the end of the mission, in the ~Destructor or PostRun.
bool OpenODF2(const char *name)
{
//	if(!name || !name[0] || name[0] == '.')
//		FormatConsoleMessage("Found ya!");

	if((!name[0]) || (name[0] == '.'))
		return false;

	// get the CRC hash of the name.
	unsigned long nameHash = CalcCRC(name);

	// check if we already tried and failed.
	if (ODFNameBlackListMap.find(nameHash) != ODFNameBlackListMap.end())
		return false; // previously tried and failed.

	// check if the name is already open.
	if(ODFNameMap.find(nameHash) != ODFNameMap.end())
		return true; // already open.

//	if (!DoesFileExist(name)) // Too laggy.
//		return false; // Nope.avi

	/*
	char TempName[MAX_ODF_LENGTH] = { 0 };
	for (int i = 0; i < sizeof(ClassLabelList) / sizeof(ClassLabelList[0]); i++)
	{
		sprintf_s(TempName, "%s.odf", ClassLabelList[i]);
		if (_stricmp(name, TempName) == 0)
		{
			FormatConsoleMessage("ALERT!");
		}
	}
	FormatConsoleMessage("Open ODF Opening: '%s'", name);
	*/

	PetWatchdogThread();
	// open the file.
	if(OpenODF(name))
	{
		PetWatchdogThread();
		// add it to the open files.
		strcpy_s(ODFNameMap[nameHash].name, name);
		return true;
	}

	// add it to the failed files.
	strcpy_s(ODFNameBlackListMap[nameHash].name, name);

	return false; // file not found.
}

// Closes all ODFs opened by OpenODF. Note: This MUST be called at the end of the mission, in the ~Destructor or PostRun().
void CloseOpenODFs(void)
{
	// Close all ODFs we've opened this game.
	for (stdext::hash_map<unsigned long, ODFName>::iterator iter = ODFNameMap.begin(); iter != ODFNameMap.end(); ++iter)
		CloseODF(iter->second.name);

	ODFNameMap.clear();
}

// Versions that use built-in single level of ODF Inheritence, pass in ODFName as file1, and the classlabel string with .odf appended as file2, and it returns the value. NOTE: These assume the ODF is already Open. 
// Modified to also do fully recursive, though the original single level intended use is still faster. -GBD
int GetODFHexInt(const char* file1, const char* file2, const char* block, const char* name, int* value, const int defval)
{
	if ((file1 && GetODFHexInt(file1, block, name, value, defval)) || (file2 && GetODFHexInt(file2, block, name, value, defval)))
		return true; // It was in the first 2 odfs passed in, woohoo!

	// Nope, go fully recursive.
	// First step: Does this odf have the .odf appended?
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if (file2)
		(_stricmp(file2 + strlen(file2) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file2) : CombineStrings(TempODF, file2, ".odf");
	else
		(_stricmp(file1 + strlen(file1) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file1) : CombineStrings(TempODF, file1, ".odf");

	char TempODF2[MAX_ODF_LENGTH] = { 0 };
	while (OpenODF2(TempODF)) // Go all the way until you can't go any further.
	{
		if (GetODFHexInt(TempODF, block, name, value, defval))
			return true; // Got it.

		if ((!GetODFString(TempODF, "GameObjectClass", "classlabel", MAX_ODF_LENGTH, TempODF2)) || (!DoesODFExist(TempODF2)))
			return false; // No "Classlabel" string in ODF, abort.
		CombineStrings(TempODF2, TempODF2, ".odf"); // Add .odf to the end.

		if (_stricmp(TempODF, TempODF2) == 0)
			return false; // This ODF is the same as it's Classlabel. Abort!

		strcpy_s(TempODF, TempODF2); // Copy TempODF2 into TempODF to try again.
	}
	return false; // Nope, didn't find it.
}
int GetODFInt(const char* file1, const char* file2, const char* block, const char* name, int* value, const int defval)
{
	if ((file1 && GetODFInt(file1, block, name, value, defval)) || (file2 && GetODFInt(file2, block, name, value, defval)))
		return true;

	// Nope, go fully recursive.
	// First step: Does this odf have the .odf appended?
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if (file2)
		(_stricmp(file2 + strlen(file2) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file2) : CombineStrings(TempODF, file2, ".odf");
	else
		(_stricmp(file1 + strlen(file1) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file1) : CombineStrings(TempODF, file1, ".odf");

	char TempODF2[MAX_ODF_LENGTH] = { 0 };
	while (OpenODF2(TempODF)) // Go all the way until you can't go any further.
	{
		if (GetODFInt(TempODF, block, name, value, defval))
			return true; // Got it.

		if ((!GetODFString(TempODF, "GameObjectClass", "classlabel", MAX_ODF_LENGTH, TempODF2)) || (!DoesODFExist(TempODF2)))
			return false; // No "Classlabel" string in ODF, abort.
		CombineStrings(TempODF2, TempODF2, ".odf"); // Add .odf to the end.

		if (_stricmp(TempODF, TempODF2) == 0)
			return false; // This ODF is the same as it's Classlabel. Abort!

		strcpy_s(TempODF, TempODF2); // Copy TempODF2 into TempODF to try again.
	}
	return false; // Nope, didn't find it.
}
int GetODFLong(const char* file1, const char* file2, const char* block, const char* name, long* value, const long defval)
{
	if ((file1 && GetODFLong(file1, block, name, value, defval)) || (file2 && GetODFLong(file2, block, name, value, defval)))
		return true;

	// Nope, go fully recursive.
	// First step: Does this odf have the .odf appended?
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if (file2)
		(_stricmp(file2 + strlen(file2) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file2) : CombineStrings(TempODF, file2, ".odf");
	else
		(_stricmp(file1 + strlen(file1) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file1) : CombineStrings(TempODF, file1, ".odf");

	char TempODF2[MAX_ODF_LENGTH] = { 0 };
	while (OpenODF2(TempODF)) // Go all the way until you can't go any further.
	{
		if (GetODFLong(TempODF, block, name, value, defval))
			return true; // Got it.

		if ((!GetODFString(TempODF, "GameObjectClass", "classlabel", MAX_ODF_LENGTH, TempODF2)) || (!DoesODFExist(TempODF2)))
			return false; // No "Classlabel" string in ODF, abort.
		CombineStrings(TempODF2, TempODF2, ".odf"); // Add .odf to the end.

		if (_stricmp(TempODF, TempODF2) == 0)
			return false; // This ODF is the same as it's Classlabel. Abort!

		strcpy_s(TempODF, TempODF2); // Copy TempODF2 into TempODF to try again.
	}
	return false; // Nope, didn't find it.
}
int GetODFFloat(const char* file1, const char* file2, const char* block, const char* name, float* value, const float defval)
{
	if ((file1 && GetODFFloat(file1, block, name, value, defval)) || (file2 && GetODFFloat(file2, block, name, value, defval)))
		return true;

	// Nope, go fully recursive.
	// First step: Does this odf have the .odf appended?
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if (file2)
		(_stricmp(file2 + strlen(file2) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file2) : CombineStrings(TempODF, file2, ".odf");
	else
		(_stricmp(file1 + strlen(file1) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file1) : CombineStrings(TempODF, file1, ".odf");

	char TempODF2[MAX_ODF_LENGTH] = { 0 };
	while (OpenODF2(TempODF)) // Go all the way until you can't go any further.
	{
		if (GetODFFloat(TempODF, block, name, value, defval))
			return true; // Got it.

		if ((!GetODFString(TempODF, "GameObjectClass", "classlabel", MAX_ODF_LENGTH, TempODF2)) || (!DoesODFExist(TempODF2)))
			return false; // No "Classlabel" string in ODF, abort.
		CombineStrings(TempODF2, TempODF2, ".odf"); // Add .odf to the end.

		if (_stricmp(TempODF, TempODF2) == 0)
			return false; // This ODF is the same as it's Classlabel. Abort!

		strcpy_s(TempODF, TempODF2); // Copy TempODF2 into TempODF to try again.
	}
	return false; // Nope, didn't find it.
}
int GetODFDouble(const char* file1, const char* file2, const char* block, const char* name, double* value, const double defval)
{
	if ((file1 && GetODFDouble(file1, block, name, value, defval)) || (file2 && GetODFDouble(file2, block, name, value, defval)))
		return true;

	// Nope, go fully recursive.
	// First step: Does this odf have the .odf appended?
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if (file2)
		(_stricmp(file2 + strlen(file2) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file2) : CombineStrings(TempODF, file2, ".odf");
	else
		(_stricmp(file1 + strlen(file1) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file1) : CombineStrings(TempODF, file1, ".odf");

	char TempODF2[MAX_ODF_LENGTH] = { 0 };
	while (OpenODF2(TempODF)) // Go all the way until you can't go any further.
	{
		if (GetODFDouble(TempODF, block, name, value, defval))
			return true; // Got it.

		if ((!GetODFString(TempODF, "GameObjectClass", "classlabel", MAX_ODF_LENGTH, TempODF2)) || (!DoesODFExist(TempODF2)))
			return false; // No "Classlabel" string in ODF, abort.
		CombineStrings(TempODF2, TempODF2, ".odf"); // Add .odf to the end.

		if (_stricmp(TempODF, TempODF2) == 0)
			return false; // This ODF is the same as it's Classlabel. Abort!

		strcpy_s(TempODF, TempODF2); // Copy TempODF2 into TempODF to try again.
	}
	return false; // Nope, didn't find it.
}
int GetODFChar(const char* file1, const char* file2, const char* block, const char* name, char* value, const char defval)
{
	if ((file1 && GetODFChar(file1, block, name, value, defval)) || (file2 && GetODFChar(file2, block, name, value, defval)))
		return true;

	// Nope, go fully recursive.
	// First step: Does this odf have the .odf appended?
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if (file2)
		(_stricmp(file2 + strlen(file2) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file2) : CombineStrings(TempODF, file2, ".odf");
	else
		(_stricmp(file1 + strlen(file1) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file1) : CombineStrings(TempODF, file1, ".odf");

	char TempODF2[MAX_ODF_LENGTH] = { 0 };
	while (OpenODF2(TempODF)) // Go all the way until you can't go any further.
	{
		if (GetODFChar(TempODF, block, name, value, defval))
			return true; // Got it.

		if ((!GetODFString(TempODF, "GameObjectClass", "classlabel", MAX_ODF_LENGTH, TempODF2)) || (!DoesODFExist(TempODF2)))
			return false; // No "Classlabel" string in ODF, abort.
		CombineStrings(TempODF2, TempODF2, ".odf"); // Add .odf to the end.

		if (_stricmp(TempODF, TempODF2) == 0)
			return false; // This ODF is the same as it's Classlabel. Abort!

		strcpy_s(TempODF, TempODF2); // Copy TempODF2 into TempODF to try again.
	}
	return false; // Nope, didn't find it.
}
int GetODFBool(const char* file1, const char* file2, const char* block, const char* name, bool* value, const bool defval)
{
	if ((file1 && GetODFBool(file1, block, name, value, defval)) || (file2 && GetODFBool(file2, block, name, value, defval)))
		return true;

	// Nope, go fully recursive.
	// First step: Does this odf have the .odf appended?
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if (file2)
		(_stricmp(file2 + strlen(file2) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file2) : CombineStrings(TempODF, file2, ".odf");
	else
		(_stricmp(file1 + strlen(file1) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file1) : CombineStrings(TempODF, file1, ".odf");

	char TempODF2[MAX_ODF_LENGTH] = { 0 };
	while (OpenODF2(TempODF)) // Go all the way until you can't go any further.
	{
		if (GetODFBool(TempODF, block, name, value, defval))
			return true; // Got it.

		if ((!GetODFString(TempODF, "GameObjectClass", "classlabel", MAX_ODF_LENGTH, TempODF2)) || (!DoesODFExist(TempODF2)))
			return false; // No "Classlabel" string in ODF, abort.
		CombineStrings(TempODF2, TempODF2, ".odf"); // Add .odf to the end.

		if (_stricmp(TempODF, TempODF2) == 0)
			return false; // This ODF is the same as it's Classlabel. Abort!

		strcpy_s(TempODF, TempODF2); // Copy TempODF2 into TempODF to try again.
	}
	return false; // Nope, didn't find it.
}
int GetODFString(const char* file1, const char* file2, const char* block, const char* name, size_t ValueLen, char* value, const char* defval)
{
	if ((file1 && GetODFString(file1, block, name, ValueLen, value, defval)) || (file2 && GetODFString(file2, block, name, ValueLen, value, defval)))
		return true;

	// Nope, go fully recursive.
	// First step: Does this odf have the .odf appended?
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if (file2)
		(_stricmp(file2 + strlen(file2) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file2) : CombineStrings(TempODF, file2, ".odf");
	else
		(_stricmp(file1 + strlen(file1) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file1) : CombineStrings(TempODF, file1, ".odf");

	char TempODF2[MAX_ODF_LENGTH] = { 0 };
	while (OpenODF2(TempODF)) // Go all the way until you can't go any further.
	{
		if (GetODFString(TempODF, block, name, ValueLen, value, defval))
			return true; // Got it.

		if ((!GetODFString(TempODF, "GameObjectClass", "classlabel", MAX_ODF_LENGTH, TempODF2)) || (!DoesODFExist(TempODF2)))
			return false; // No "Classlabel" string in ODF, abort.
		CombineStrings(TempODF2, TempODF2, ".odf"); // Add .odf to the end.

		if (_stricmp(TempODF, TempODF2) == 0)
			return false; // This ODF is the same as it's Classlabel. Abort!

		strcpy_s(TempODF, TempODF2); // Copy TempODF2 into TempODF to try again.
	}
	return false; // Nope, didn't find it.
}
int GetODFColor(const char* file1, const char* file2, const char* block, const char* name, DWORD* value, const DWORD defval)
{
	if ((file1 && GetODFColor(file1, block, name, value, defval)) || (file2 && GetODFColor(file2, block, name, value, defval)))
		return true;

	// Nope, go fully recursive.
	// First step: Does this odf have the .odf appended?
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if (file2)
		(_stricmp(file2 + strlen(file2) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file2) : CombineStrings(TempODF, file2, ".odf");
	else
		(_stricmp(file1 + strlen(file1) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file1) : CombineStrings(TempODF, file1, ".odf");

	char TempODF2[MAX_ODF_LENGTH] = { 0 };
	while (OpenODF2(TempODF)) // Go all the way until you can't go any further.
	{
		if (GetODFColor(TempODF, block, name, value, defval))
			return true; // Got it.

		if ((!GetODFString(TempODF, "GameObjectClass", "classlabel", MAX_ODF_LENGTH, TempODF2)) || (!DoesODFExist(TempODF2)))
			return false; // No "Classlabel" string in ODF, abort.
		CombineStrings(TempODF2, TempODF2, ".odf"); // Add .odf to the end.

		if (_stricmp(TempODF, TempODF2) == 0)
			return false; // This ODF is the same as it's Classlabel. Abort!

		strcpy_s(TempODF, TempODF2); // Copy TempODF2 into TempODF to try again.
	}
	return false; // Nope, didn't find it.
}
int GetODFVector(const char* file1, const char* file2, const char* block, const char* name, Vector* value, const Vector defval)
{
	if ((file1 && GetODFVector(file1, block, name, value, defval)) || (file2 && GetODFVector(file2, block, name, value, defval)))
		return true;

	// Nope, go fully recursive.
	// First step: Does this odf have the .odf appended?
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if (file2)
		(_stricmp(file2 + strlen(file2) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file2) : CombineStrings(TempODF, file2, ".odf");
	else
		(_stricmp(file1 + strlen(file1) - 4, ".odf") == 0) ? CombineStrings(TempODF, "", file1) : CombineStrings(TempODF, file1, ".odf");

	char TempODF2[MAX_ODF_LENGTH] = { 0 };
	while (OpenODF2(TempODF)) // Go all the way until you can't go any further.
	{
		if (GetODFVector(TempODF, block, name, value, defval))
			return true; // Got it.

		if ((!GetODFString(TempODF, "GameObjectClass", "classlabel", MAX_ODF_LENGTH, TempODF2)) || (!DoesODFExist(TempODF2)))
			return false; // No "Classlabel" string in ODF, abort.
		CombineStrings(TempODF2, TempODF2, ".odf"); // Add .odf to the end.

		if (_stricmp(TempODF, TempODF2) == 0)
			return false; // This ODF is the same as it's Classlabel. Abort!

		strcpy_s(TempODF, TempODF2); // Copy TempODF2 into TempODF to try again.
	}
	return false; // Nope, didn't find it.
}
// Custom GetODF functions that OpenODF2 the ODF, and include ODF Inhertience.
int GetODFHexInt(const Handle h, const char* block, const char* name, int* value, const int defval)
{
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if ((!GetObjInfo(h, Get_ODF, TempODF)) || (!block) || (!name))
		return false;

	return GetODFHexInt(TempODF, NULL, block, name, value, defval);
}
int GetODFInt(const Handle h, const char* block, const char* name, int* value, const int defval)
{
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if ((!GetObjInfo(h, Get_ODF, TempODF)) || (!block) || (!name))
		return false;

	return GetODFInt(TempODF, NULL, block, name, value, defval);
}
int GetODFLong(const Handle h, const char* block, const char* name, long* value, const long defval)
{
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if ((!GetObjInfo(h, Get_ODF, TempODF)) || (!block) || (!name))
		return false;

	return GetODFLong(TempODF, NULL, block, name, value, defval);
}
int GetODFFloat(const Handle h, const char* block, const char* name, float* value, const float defval)
{
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if ((!GetObjInfo(h, Get_ODF, TempODF)) || (!block) || (!name))
		return false;

	return GetODFFloat(TempODF, NULL, block, name, value, defval);
}
int GetODFDouble(const Handle h, const char* block, const char* name, double* value, const double defval)
{
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if ((!GetObjInfo(h, Get_ODF, TempODF)) || (!block) || (!name))
		return false;

	return GetODFDouble(TempODF, NULL, block, name, value, defval);
}
int GetODFChar(const Handle h, const char* block, const char* name, char* value, const char defval)
{
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if ((!GetObjInfo(h, Get_ODF, TempODF)) || (!block) || (!name))
		return false;

	return GetODFChar(TempODF, NULL, block, name, value, defval);
}
int GetODFBool(const Handle h, const char* block, const char* name, bool* value, const bool defval)
{
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if ((!GetObjInfo(h, Get_ODF, TempODF)) || (!block) || (!name))
		return false;

	return GetODFBool(TempODF, NULL, block, name, value, defval);
}
int GetODFString(const Handle h, const char* block, const char* name, size_t ValueLen, char* value, const char* defval)
{
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if ((!GetObjInfo(h, Get_ODF, TempODF)) || (!block) || (!name))
		return false;

	return GetODFString(TempODF, NULL, block, name, ValueLen, value, defval);
}
int GetODFColor(const Handle h, const char* block, const char* name, DWORD* value, const DWORD defval)
{
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if ((!GetObjInfo(h, Get_ODF, TempODF)) || (!block) || (!name))
		return false;

	return GetODFColor(TempODF, NULL, block, name, value, defval);
}
int GetODFVector(const Handle h, const char* block, const char* name, Vector* value, const Vector defval)
{
	char TempODF[MAX_ODF_LENGTH] = { 0 };
	if ((!GetObjInfo(h, Get_ODF, TempODF)) || (!block) || (!name))
		return false;

	return GetODFVector(TempODF, NULL, block, name, value, defval);
}