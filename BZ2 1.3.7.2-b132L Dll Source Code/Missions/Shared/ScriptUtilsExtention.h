#ifndef _ScriptUtilsExt_
#define _ScriptUtilsExt_

// Battlezone 2 ScriptUtils Extention written by General BlackDragon, Ken Miller, and Nielk1.

#include "..\..\source\fun3d\ScriptUtils.h"
#include "MathUtils.h"
#include <math.h>
#include <vector>
#include <hash_map>
//#include <algorithm>
//#include <limits.h>
//#include <float.h>
#include <ctype.h>
//#include <time.h>

// Get parellels to RGBA_CREATE().
#define RGBA_GETALPHA(rgb)      ((rgb) >> 24)
#define RGBA_GETRED(rgb)        (((rgb) >> 16) & 0xff)
#define RGBA_GETGREEN(rgb)      (((rgb) >> 8) & 0xff)
#define RGBA_GETBLUE(rgb)       ((rgb) & 0xff)

///*
// Custom RGB Colors.
#define GREY RGB(127,127,127)
#define BLACK RGB(0,0,0)
#define BLUE RGB(0,0,255)
#define YELLOW RGB(255,255,0)
#define PURPLE RGB(255,0,255)
#define CYAN RGB(0,255,255)
#define ALLYBLUE RGB(0,127,255)
#define LAVACOLOR RGB(255,64,0)

#define DKGREY RGB(100,100,100)
#define DKGREEN RGB(0,127,0)
#define DKRED RGB(127,0,0)
#define DKBLUE RGB(0,0,127)
#define DKYELLOW RGB(127,127,0)
//*/

// BZ2 hardcoded values.
#define MAX_TAPS 8 // Max number of vehicle taps.
#define MAX_BASE_SLOTS 9 // Max number of base slots.
#define MAX_GROUPS 10 // Max number of Groups to have. 0 - 9.
#define MAX_HARDPOINTS 5 // This is the max number of hardpoints a ship can have. Maybe it'll go up some day? :)
#define MAX_SPRITES 64 // Max Sprites.
#define MAX_LOCAL_FOGVOLUMES 16 // Max Local Fog volumes.
#define MAX_PROVIDES 32 // Max number of ProvideCount.
#define INVALID_VALUE -1234 // The code returned by the game on Invalid Values, such as GetMaxHealth() of a killed thing/invalid Handle in DeleteObject.
#define HUGE_NUMBER 1e+30 // The distance returned by GetDistance with invalid handles.
// String lengths.
#define MAX_ODF_LENGTH 64 // Max ODF Length.
#define MAX_NAME_LENGTH 256 // Max Objective Name/AIP file Length.
#define MAX_MESSAGE_LENGTH 2048 // Max Length of a message string.
// Math stuff.
#define PI 3.141592654f // Because Pie R Square.
#define DEG_2_RAD 0.0174532925222222f // Degrees to Radians.
//#define RAD_2_DEG 57.2957877f // Radians to Degrees. // Defined in MathUtils.h.

// String for who modified this DLL, for printing in console of misisons. If you make changes, simply add your name here.
static const char *DLLAuthors = "General BlackDragon, Ken Miller, & Nielk1";

///*
// Path Type Enum.
enum PathType {
	ONE_WAY_PATH, // == 0
	ROUND_TRIP_PATH, // == 1
	LOOP_PATH, // == 2 
	BAD_PATH // == 3 -- when it couldn't find a route. Used by HuntTask, Recycle[H]Task
};
//*/
// Independence Enum, cause I can never remember which is which.
enum Independence_Type
{
	Independence_Off,
	Independence_On,
};

// FNV-1a Hash's of Get_EntityType strings, for faster comparisons. (Fowler-Noll-Vo 1a)
enum Hash_EntityType
{
	CLASS_ID_NONE = 0x08ade38c,			//	"CLASS_ID_NONE",		// undefined object (default)
	CLASS_ID_CRAFT = 0xf2070008,		// 	"CLASS_ID_CRAFT",		// hovering vehicle
	CLASS_ID_VEHICLE = 0x27d612aa,		// 	"CLASS_ID_VEHICLE",		// tracked vehicle
	CLASS_ID_TORPEDO = 0xfcbb9395,		// 	"CLASS_ID_TORPEDO",		// miniature vehicle
	CLASS_ID_POWERUP = 0x8f2b1662,		// 	"CLASS_ID_POWERUP",		// power up pod
	CLASS_ID_PERSON = 0xa7a2704b,		// 	"CLASS_ID_PERSON",		// pilot or soldier
	CLASS_ID_ANIMAL = 0x2988eee2,		// 	"CLASS_ID_ANIMAL",		// animal
	CLASS_ID_STRUCT = 0xfea184f3,		// 	"CLASS_ID_STRUCT",		// generic building
	CLASS_ID_BUILDING = 0xc07bf472,		// 	"CLASS_ID_BUILDING",	// important building
	CLASS_ID_SIGN = 0x1f71f513,			// 	"CLASS_ID_SIGN",		// mine object
	CLASS_ID_SCRAP = 0xaff3780b,		// 	"CLASS_ID_SCRAP",		// scrap piece
	CLASS_ID_DEPOSIT = 0xe120e5e8,		// 	"CLASS_ID_DEPOSIT",		// scrap deposit
	CLASS_ID_BEACON = 0x4c47d522,		// 	"CLASS_ID_BEACON",		// nav beacon
	CLASS_ID_PLANT = 0x6c6effbb,		// 	"CLASS_ID_PLANT",		// plant object
	CLASS_ID_TERRAIN = 0xb6789db7,		// 	"CLASS_ID_TERRAIN",		// terrain object
	CLASS_ID_WEAPON = 0xefa88fd6,		// 	"CLASS_ID_WEAPON",		// weapon object
	CLASS_ID_ORDNANCE = 0x5ac64c32,		// 	"CLASS_ID_ORDNANCE",	// ordnance object
	CLASS_ID_EXPLOSION = 0x4366a7c9,	// 	"CLASS_ID_EXPLOSION",	// explosion object
	CLASS_ID_CHUNK = 0x28e135b3,		// 	"CLASS_ID_CHUNK",		// chunk object
	CLASS_ID_CRASH = 0xdc273111,		// 	"CLASS_ID_CRASH",		// crash object
	CLASS_ID_COLLAPSE = 0x9592e289,		// 	"CLASS_ID_COLLAPSE",	// collapsing object
};

// FNV-1a Hash of Get_GOClass strings for faster comparison.
enum Hash_GOClass
{
	CLASS_AIR = 0x04de8a34,						//   "CLASS_AIR" // AirCraft.h
	CLASS_ANCHORROCKET = 0x2503fbf9,			//   "CLASS_ANCHORROCKET" // AnchorRocketClass.h
	CLASS_APC = 0x0da4acce,						//   "CLASS_APC" // APC.h
	CLASS_ARCCANNON = 0xe91cb08d,				//   "CLASS_ARCCANNON" // ArcCannonClass.h
	CLASS_ARMORY = 0x63ae241a,					//   "CLASS_ARMORY" // Armory.h
	CLASS_ARTIFACT = 0x3eb9e3b2,				//   "CLASS_ARTIFACT" // Artifact.h
	CLASS_ARTILLERY = 0xf9df4e76,				//   "CLASS_ARTILLERY" // Artillery.h
	CLASS_ASSAULTTANK = 0x1aefd513,				//   "CLASS_ASSAULTTANK" // AssaultTank.h
	CLASS_ASSAULTHOVER = 0xc864a6ef,			//   "CLASS_ASSAULTHOVER" // AssaultHover.h
	CLASS_BARRACKS = 0xb18ef1c3,				//   "CLASS_BARRACKS" // Barracks.h
	CLASS_BEAM = 0x20cec93f,					//   "CLASS_BEAM"
	CLASS_BLINK = 0x5c09179e,					//   "CLASS_BLINK" // BlinkDeviceClass.h
	CLASS_BOID = 0xa9238ce6,					//   "CLASS_BOID"
	CLASS_BOMBER = 0x4606b887,					//   "CLASS_BOMBER" // Bomber.h
	CLASS_BOMBERBAY = 0x6d0df679,				//   "CLASS_BOMBERBAY" // BomberBay.h
	CLASS_BOUNCEBOMB = 0xfce344e8,				//   "CLASS_BOUNCEBOMB"
	CLASS_BUILDING = 0x98264ce8,				//   "CLASS_BUILDING" // BuildingClass.h
	CLASS_BULLET = 0xd97930d0,					//   "CLASS_BULLET" // BulletClass.h
	CLASS_CANNON = 0xa54f6ffd,					//   "CLASS_CANNON" // CannonClass.h
	CLASS_CANNON_MACHINEGUN = 0x9203c82b,		//   "CLASS_CANNON_MACHINEGUN" // MachineGunClass.h
	CLASS_CANNON_TARGETING = 0x071c0ca1,		//   "CLASS_CANNON_TARGETING" // TargetingGunClass.h
	CLASS_CHARGEGUN = 0xfa593446,				//   "CLASS_CHARGEGUN" // ChargeGunClass.h
	CLASS_COMMBUNKER = 0xabf8e42d,				//   "CLASS_COMMBUNKER" // CommBunker.h
	CLASS_COMMTOWER = 0xfcc7b3dd,				//   "CLASS_COMMTOWER" // CommTower.h
	CLASS_COMMVEHICLE = 0xb6f61ce0,				//   "CLASS_COMMVEHICLE" // CommVehicle.h
	CLASS_CONSTRUCTIONRIG = 0x77f02e21,			//   "CLASS_CONSTRUCTIONRIG" // ConstructionRig.h
	CLASS_CLASS_CRAFT = 0xcf98abe1,				//   "CLASS_CRAFT" // CraftClass.h
	CLASS_CLASS_DAMAGEFIELD = 0x8ac14f6c,		//   "CLASS_DAMAGEFIELD" // DamageFieldClass.h
	CLASS_DAYWRECKER = 0x998a9793,				//   "CLASS_DAYWRECKER" // DayWrecker.h
	CLASS_DEPLOYABLE = 0x22b50b1f,				//   "CLASS_DEPLOYABLE" // Deployable.h
	CLASS_DEPLOYBUILDING = 0xf1656077,			//   "CLASS_DEPLOYBUILDING" // DeployBuilding.h
	CLASS_DEPOSIT = 0xdc5f98c6,					//   "CLASS_DEPOSIT" // Deposit.h
	CLASS_DISPENSER = 0xd566b803,				//   "CLASS_DISPENSER" // DispenserClass.h
	CLASS_EXPLOSION = 0x79dfd297,				//   "CLASS_EXPLOSION" // ExplosionClass.h
	CLASS_EXTRACTOR = 0x490dd218,				//   "CLASS_EXTRACTOR" // Extractor.h
	CLASS_FACTORY = 0xe39ee1ae,					//   "CLASS_FACTORY" // Factory.h
	CLASS_FLAG = 0xf2ecd752,					//   "CLASS_FLAG" // FlagObject.h
	CLASS_FLAREMINE = 0x88799827,				//   "CLASS_FLAREMINE" // FlareMineClass.h
	CLASS_GAMEOBJECT = 0xd840f319,				//   "CLASS_GAMEOBJECT" // GameObjectClass.h
	CLASS_GRENADE = 0x9e8cc54e,					//   "CLASS_GRENADE" // GrenadeClass.h
	CLASS_GRENADE_LASERPOPPER = 0x6255d3b4,		//   "CLASS_GRENADE_LASERPOPPER" // LaserPopperClass.h
	CLASS_GRENADE_POPPER = 0xab9d3e35,			//   "CLASS_GRENADE_POPPER" // PopperClass.h
	//CLASS_GRENADE_RADARPOPPER = 0xdd93bbe7,	// //  "CLASS_GRENADE_RADARPOPPER" // RadarPopperClass.h [Same sig as GRENADE_POPPER, uses that]
	CLASS_HOVER = 0xbb81d932,					//   "CLASS_HOVER" // HoverCraft.h
	CLASS_HOWITZER = 0x25fd9ee0,				//   "CLASS_HOWITZER" // Howitzer.h
	CLASS_I76BUILDING = 0x3a9268d2,				//   "CLASS_I76BUILDING" // Building.h
	CLASS_JAMMER = 0x41fb32ec,					//   "CLASS_JAMMER" // JammerTower.h
	CLASS_JETPACK = 0x834f248a,					//   "CLASS_JETPACK" // JetPackClass.h
	CLASS_KINGOFHILL = 0xa9a32ae5,				//   "CLASS_KINGOFHILL" // KingOfHill.h
	CLASS_LAUNCHER = 0xc3cf9e70,				//   "CLASS_LAUNCHER" // LauncherClass.h
	CLASS_LAUNCHER_IMAGE = 0x9d350762,			//   "CLASS_LAUNCHER_IMAGE" // ImageLauncherClass.h
	CLASS_LAUNCHER_MULTI = 0xd264f7ea,			//   "CLASS_LAUNCHER_MULTI" // MultiLauncherClass.h
	CLASS_LAUNCHER_RADAR = 0xa905d923,			//   "CLASS_LAUNCHER_RADAR" // RadarLauncherClass.h
	CLASS_LAUNCHER_THERMAL = 0x46577898,		//   "CLASS_LAUNCHER_THERMAL" // ThermalLauncherClass.h
	CLASS_LAUNCHER_TORPEDO = 0x2b57e4d0,		//   "CLASS_LAUNCHER_TORPEDO" // TorpedoLauncherClass.h
	//CLASS_LEADER_ROUND = 0xcdc72a4a,			// //  "CLASS_LEADER_ROUND " // LeaderRoundClass.h [Same sig as CLASS_CANNON_TARGETING, returns that]
	CLASS_LOCKSHELL = 0xc878487b,				//   "CLASS_LOCKSHELL" // LockShellClass.h
	CLASS_MAGNETGUN = 0x7eb95e20,				//   "CLASS_MAGNETGUN" // MagnetGunClass.h
	CLASS_MAGNETSHELL = 0x700207a4,				//   "CLASS_MAGNETSHELL" // MagnetShellClass.h
	CLASS_MINE = 0x17900f9b,					//   "CLASS_MINE" // MineClass.h
	CLASS_MINELAYER = 0x32493554,				//   "CLASS_MINELAYER" // Minelayer.h
	CLASS_MINE_MAGNET = 0x6e0a7e06,				//   "CLASS_MINE_MAGNET" // MagnetMineClass.h
	CLASS_MINE_PROXIMITY = 0x3a1b5a15,			//   "CLASS_MINE_PROXIMITY" // ProximityMineClass.h
	CLASS_MINE_TRIP = 0x9558c8af,				//   "CLASS_MINE_TRIP" // TripMineClass.h
	CLASS_MINE_WEAPON = 0x175a2ab0,				//   "CLASS_MINE_WEAPON" // WeaponMineClass.h
	CLASS_MISSILE = 0xe1de9178,					//   "CLASS_MISSILE" // MissileClass.h
	CLASS_MISSILE_IMAGE = 0x778daf1a,			//   "CLASS_MISSILE_IMAGE" // ImageMissileClass.h
	CLASS_MISSILE_LASER = 0x61e8eaac,			//   "CLASS_MISSILE_LASER" // LaserMissileClass.h
	CLASS_MISSILE_RADAR = 0x6a0a773b,			//   "CLASS_MISSILE_RADAR" // RadarMissileClass.h
	CLASS_MISSILE_THERMAL = 0x2bc381e0,			//   "CLASS_MISSILE_THERMAL" // ThermalMissileClass.h
	CLASS_MORPHTANK = 0x1c628d2e,				//   "CLASS_MORPHTANK" // MorphTank.h
	CLASS_MORTAR = 0x3170ea1d,					//   "CLASS_MORTAR" // MortarClass.h
	CLASS_MORTAR_REMOTE = 0xf75934f4,			//   "CLASS_MORTAR_REMOTE" // RemoteDetonatorClass.h
	CLASS_MOTIONSENSOR = 0x6dd673e4,			//   "CLASS_MOTIONSENSOR" // MotionSensor.h
	CLASS_NAVBEACON = 0x33e33cd9,				//   "CLASS_NAVBEACON" // NavBeaconClass.h
	CLASS_OBJECTSPAWN = 0xccf5a072,				//   "CLASS_OBJECTSPAWN" // ObjectSpawn.h
	CLASS_ORDNANCE = 0x0dcf99a8,				//   "CLASS_ORDNANCE" // OrdnanceClass.h
	CLASS_PERSON = 0x5768dca5,					//   "CLASS_PERSON" // PersonClass.h
	CLASS_PLANT = 0xdd054da5,					//   "CLASS_PLANT" // Plant.h // PowerPlant, not foliage plant. -GBD
	CLASS_POWERED = 0xd68d4890,					//   "CLASS_POWERED" // PoweredBuilding.h
	CLASS_POWERUP_CAMERA = 0x8d7c8912,			//   "CLASS_POWERUP_CAMERA" // CameraPod.h
	CLASS_POWERUP_MONEY = 0xb163057b,			//   "CLASS_POWERUP_MONEY" // MoneyPowerup.h
	CLASS_POWERUP_RELOAD = 0xa662aa60,			//   "CLASS_POWERUP_RELOAD" // ServicePowerup.h
	CLASS_POWERUP_REPAIR = 0x4b970310,			//   "CLASS_POWERUP_REPAIR" // ServicePowerup.h
	CLASS_POWERUP_SERVICE = 0x33fe0a68,			//   "CLASS_POWERUP_SERVICE" // ServicePowerup.h
	//CLASS_POWERUP_WEAPON = 0xfa990e15,		// //  "CLASS_POWERUP_WEAPON" // WeaponPowerup.h [Same sig as CLASS_WEAPON, returns that]
	CLASS_PULSESHELL = 0x27cd147d,				//   "CLASS_PULSESHELL" // PulseShellClass.h
	CLASS_RECYCLER = 0x8a0a1ead,				//   "CLASS_RECYCLER" // Recycler.h
	CLASS_RECYCLERVEHICLE = 0xc9cd7077,			//   "CLASS_RECYCLERVEHICLE" // RecyclerVehicle.h
	CLASS_SALVOLAUNCHER = 0xe8ce53bb,			//   "CLASS_SALVOLAUNCHER" // SalvoLauncherClass.h
	CLASS_SATCHELCHARGE = 0xc7788a0e,			//   "CLASS_SATCHELCHARGE" // SatchelCharge.h
	CLASS_SATCHELPACK = 0x2695d937,				//   "CLASS_SATCHELPACK" // SatchelPackClass.h
	CLASS_SAV = 0xa7275daa,						//   "CLASS_SAV" // SAV.h
	CLASS_SCAVENGER = 0x33a01170,				//   "CLASS_SCAVENGER" // Scavenger.h
	CLASS_SCAVENGERH = 0x5cfae8c8,				//   "CLASS_SCAVENGERH" // ScavengerH.h
	CLASS_SCRAP = 0x6ee4c53d,					//   "CLASS_SCRAP" // Scrap.h
	CLASS_SEEKER = 0xd888605f,					//   "CLASS_SEEKER" // SeekerClass.h
	CLASS_SEISMICWAVE = 0xa6dfc78e,				//   "CLASS_SEISMICWAVE" // SeismicWaveClass.h
	CLASS_SERVICE = 0x28e4d573,					//   "CLASS_SERVICE" // ServiceTruck.h
	CLASS_SERVICEH = 0x7b3b7981,				//   "CLASS_SERVICEH" // ServiceTruckH.h
	CLASS_SHIELDTOWER = 0x9a4a949e,				//   "CLASS_SHIELDTOWER" // ShieldTower.h
	CLASS_SHIELDUP = 0xe5a9cafc,				//   "CLASS_SHIELDUP" // ShieldUpgradeClass.h
	CLASS_SIGN = 0x81ed3511,					//   "CLASS_SIGN" // BuildingClass.h
	CLASS_SILO = 0x80f9ff71,					//   "CLASS_SILO" // Silo.h
	CLASS_SNIPERSHELL = 0xaaa6237f,				//   "CLASS_SNIPERSHELL" // SniperShellClass.h
	CLASS_SPAWNBUOY = 0x4a25dbb6,				//   "CLASS_SPAWNBUOY" // SpawnBuoy.h
	CLASS_SPECIAL = 0xc4f1f17b,					//   "CLASS_SPECIAL" // SpecialItemClass.h
	CLASS_SPECIAL_FORCEFIELD = 0x81e62d61,		//   "CLASS_SPECIAL_FORCEFIELD" // ForceFieldClass.h
	CLASS_SPECIAL_IMAGEREFRACT = 0xbefe3de0,	//   "CLASS_SPECIAL_IMAGEREFRACT" // ImageRefractClass.h
	CLASS_SPECIAL_RADARDAMPER = 0x364ce6f5,		//   "CLASS_SPECIAL_RADARDAMPER" // RadarDamperClass.h
	CLASS_SPECIAL_TERRAINEXPOSE = 0x02c0cbed,	//   "CLASS_SPECIAL_TERRAINEXPOSE" // TerrainExposeClass.h
	CLASS_SPRAYBOMB = 0xbe8a7c31,				//   "CLASS_SPRAYBOMB" // SprayBombClass.h
	CLASS_SPRAYBUILDING = 0xf4c70715,			//   "CLASS_SPRAYBUILDING" // SprayBuildingClass.h
	CLASS_SUPPLYDEPOT = 0xdadcb21b,				//   "CLASS_SUPPLYDEPOT" // SupplyDepot.h
	CLASS_TELEPORTAL = 0x7d9b0716,				//   "CLASS_TELEPORTAL" // TelePortalClass.h
	CLASS_TERRAIN = 0x6e47e5f1,					//   "CLASS_TERRAIN" // DummyClass.h
	CLASS_TORPEDO = 0xcd066db7,					//   "CLASS_TORPEDO" // TorpedoClass.h
	CLASS_TRACKEDDEPLOYABLE = 0x07257e15,		//   "CLASS_TRACKEDDEPLOYABLE" // TrackedDeployable.h
	CLASS_TRACKEDVEHICLE = 0x57b8478a,			//   "CLASS_TRACKEDVEHICLE" // TrackedVehicle.h
	CLASS_TUG = 0x04e12224,						//   "CLASS_TUG" // Tug.h
	CLASS_TURRET = 0xdf6dabba,					//   "CLASS_TURRET" // TurretCraft.h - ibgtow/fbspir (Guntower/gunspires)
	CLASS_TURRETTANK = 0x4a7aef92,				//   "CLASS_TURRETTANK" // TurretTank.h - ivturr/fvturr (vehicle turrets)
	CLASS_WALKER = 0xedb121ba,					//   "CLASS_WALKER" // Walker.h
	CLASS_WEAPON = 0x16164674,					//   "CLASS_WEAPON" // WeaponClass.h
	CLASS_WINGMAN = 0x83416d25,					//   "CLASS_WINGMAN" // Wingman.h
	CLASS_UNKNOWN = 0x1418355a,					//   "CLASS_UNKNOWN" // default
};

// Static Const Char lists.
extern const char *CommandList[NUM_CMD];

// Saved open ODF Files on this machine.
struct ODFName { char name[MAX_ODF_LENGTH]; }; // The ODF.
extern stdext::hash_map<unsigned long, ODFName> ODFNameMap;
// Non saved, list of files that code attempted to open but don't exist, such as classlabel strings, e.g. "turret.odf", or "flare.odf"
extern stdext::hash_map<unsigned long, ODFName> ODFNameBlackListMap;
// Saved open Files on this machine.
extern stdext::hash_map<const char*, FILE*> FileNameMap;
typedef stdext::hash_map<const char*, FILE*>::iterator FileMapIt;

// The Matrix. Free Your Mind...
static Matrix Identity_Matrix = Matrix(Vector(1, 0, 0), Vector(0, 1, 0), Vector(0, 0, 1), Vector(0, 0, 0));

// Formats a string with optional parameters, and prints it to the console. 
// Use with %d, %s, %f, %08x, etc, like with sprintf_s().
extern void FormatConsoleMessage(const char *format, ...);

// Writes a message to a file. Returns true if it succeeded, false if it failed. If Append is true, it appends to the existing file, if False then it whipes the existing file before writing. 
// This auotmatically opens the file name and stores it's name. Must call CloseOpenFiles() in destructor or postload.
extern bool FormatFileMessage(const char *Filename, const bool Append, const char *format, ...);
// Formats a message for the MessageBox with color. // Prints the specified string to the messages box (where chat messages, etc go). Color is a RGBA value, use the RGB() or RGBA() macros to make them, e.g. AddToMessagesBox2("Hello, World!", RGB(255,0,255)); // bright purple
extern void FormatToMessagesBox(const char *format, const unsigned long color, ...);

// Get a Vector from a Path point. Code from Nielk1. Height is the Terrain height at the position. Returns Vector(0, 0, 0) if path is invalid.
extern Vector GetVectorFromPath(const char* path, const int point = 0);

// Overload functions that take const parameters.
inline Handle BuildObject(const char* odf, const int Team, const Handle him) { return BuildObject(const_cast<char *>(odf), Team, him); }
inline Handle BuildObject(const char* odf, const int Team, const char *APath) { return BuildObject(const_cast<char *>(odf), Team, const_cast<Name>(APath)); }
inline Handle BuildObject(const char* odf, const int Team, Name APath) { return BuildObject(const_cast<char *>(odf), Team, APath); }
inline Handle BuildObject(const char* odf, const int Team, const AiPath *APath) { return BuildObject(const_cast<char *>(odf), Team, const_cast<AiPath *>(APath)); }
inline Handle BuildObject(const char* odf, const int Team, const Vector pos) { return BuildObject(const_cast<char *>(odf), Team, const_cast<Vector &>(pos)); }
inline Handle BuildObject(const char* odf, const int Team, const Matrix mat) { return BuildObject(const_cast<char *>(odf), Team, const_cast<Matrix &>(mat)); }
inline void SetPilotClass(const Handle obj, const char *odf) { SetPilotClass(obj, const_cast<char *>(odf)); }
inline Handle BuildEmptyCraftNear(const Handle h, const char* ODF, const int Team, const float MinRadiusAway, const float MaxRadiusAway) { return BuildEmptyCraftNear(h, const_cast<char *>(ODF), Team, MinRadiusAway, MaxRadiusAway); }
inline void PrintConsoleMessage(const char* msg) { PrintConsoleMessage(const_cast<char *>(msg)); }
inline bool OpenODF(const char *name) { return OpenODF(const_cast<char *>(name)); }
inline bool DoesODFExist(const char* odf) { return DoesODFExist(const_cast<char *>(odf)); }
inline Handle GetHandle(const char *name) { return GetHandle(const_cast<char *>(name)); }
inline unsigned long CalcCRC(const char *name) { return CalcCRC(const_cast<char *>(name)); }
inline void SetPlan(const char* cfg, const int team = -1) { SetPlan(const_cast<char *>(cfg), team); }
inline Dist GetDistance(const Handle &h1, const Handle &h2) { return misnImport.GetDistanceObject(const_cast<Handle &>(h1), const_cast<Handle &>(h2)); }
inline Dist GetDistance(const Handle h1, const char* path, const int point = 0) { return misnImport.GetDistancePath(const_cast<Handle &>(h1), const_cast<Name>(path), point); }
inline Dist GetDistance(const Handle h1, const AiPath *path, const int point = 0) { return misnImport.GetDistancePathPtr(const_cast<Handle &>(h1), const_cast<AiPath *>(path), point); }
inline Handle GetNearestVehicle(const char* path, const int point) { return GetNearestVehicle(const_cast<Name>(path), point); }
inline void Goto(const Handle me, const char* path, const int priority = 1) { Goto(me, const_cast<Name>(path), priority); }
inline void Mine(const Handle me, const char* path, const int priority = 1) { Mine(me, const_cast<Name>(path), priority); }
inline void Patrol(const Handle me, const char* path, const int priority = 1) { Patrol(me, const_cast<Name>(path), priority); }
inline void Retreat(const Handle me, const char* path, const int priority = 1) { Retreat(me, const_cast<Name>(path), priority); }
inline void Dropoff(const Handle me, const char* path, const int priority = 1) { Dropoff(me, const_cast<Name>(path), priority); }
inline void SetPosition(const Handle h, const char* path) { SetPosition(h, const_cast<Name>(path)); }
inline void SetPathType(const char* path, const PathType pathType) { SetPathType(const_cast<Name>(path), pathType); }
inline bool IsInfo(const char* odf) { return IsInfo(const_cast<Name>(odf)); }
inline void GiveWeapon(const Handle me, const char* weapon) { GiveWeapon(me, const_cast<Name>(weapon)); }
inline void IFace_Exec(const char* n) { IFace_Exec(const_cast<Name>(n)); }
inline void IFace_Activate(const char* n) { IFace_Activate(const_cast<Name>(n)); }
inline void IFace_Deactivate(const char* n) { IFace_Deactivate(const_cast<Name>(n)); }
inline void IFace_CreateCommand(const char* n) { IFace_CreateCommand(const_cast<Name>(n)); }
inline void IFace_CreateString(const char* name, const char* value) { IFace_CreateString(const_cast<Name>(name), const_cast<Name>(value)); }
//inline void IFace_SetString(const char* name, const char* value) { IFace_SetString(const_cast<Name>(name), const_cast<Name>(value)); }
inline void IFace_GetString(const char* name, char* value, int maxSize) { IFace_GetString(const_cast<Name>(name), value, maxSize); }
inline void IFace_CreateInteger(const char* name, const int value) { IFace_CreateInteger(const_cast<Name>(name), value); }
inline void IFace_SetInteger(const char* name, const int value) { IFace_SetInteger(const_cast<Name>(name), value); }
inline int IFace_GetInteger(const char* name) { return IFace_GetInteger(const_cast<Name>(name)); }
inline void IFace_SetIntegerRange(const char* name, const int low, const int high) { IFace_SetIntegerRange(const_cast<Name>(name), low, high); }
inline void IFace_CreateFloat(const char* name, const float value) { IFace_CreateFloat(const_cast<Name>(name), value); }
inline void IFace_SetFloat(const char* name, const float value) { IFace_SetFloat(const_cast<Name>(name), value); }
inline float IFace_GetFloat(const char* name) { return IFace_GetFloat(const_cast<Name>(name)); }
inline void IFace_ClearListBox(const char* name) { IFace_ClearListBox(const_cast<Name>(name)); }
//inline void IFace_AddTextItem(const char* name, const char* value) { IFace_AddTextItem(const_cast<Name>(name), const_cast<Name>(value)); }
inline void IFace_GetSelectedItem(const char* name, char* value, const int maxSize) { IFace_GetSelectedItem(const_cast<Name>(name), value, maxSize); }
inline bool CameraPath(const char* path_name, const int height, const int speed, const Handle target_handle) { return CameraPath(const_cast<Name>(path_name), height, speed, target_handle); }
inline bool CameraPathDir(const char* path_name, const int height, const int speed) { return CameraPathDir(const_cast<Name>(path_name), height, speed); }
inline float SetAnimation(const Handle h, const char* n, const int animType = 0) { return SetAnimation(h, const_cast<Name>(n), animType); }
inline float GetAnimationFrame(const Handle h, const char* n) { return GetAnimationFrame(h, const_cast<Name>(n)); }
inline void SetObjectiveName(const Handle h, const char* n) { SetObjectiveName(h, const_cast<Name>(n)); }
inline void AddObjective(const char* name, const long color, const float showTime = 8.0f) { AddObjective(const_cast<Name>(name), color, showTime); }
inline void AddIdleAnim(const Handle h, const char* anim) { AddIdleAnim(h, const_cast<Name>(anim)); }
inline void SpawnBirds(const int group, const int count, const char* odf, const TeamNum t, const char* path) { SpawnBirds(group, count, const_cast<Name>(odf), t, const_cast<Name>(path)); }
inline void SpawnBirds(const int group, const int count, const char* odf, const TeamNum t, const Handle startObj, const Handle destObj) { SpawnBirds(group, count, const_cast<Name>(odf), t, startObj, destObj); }
inline void CalcCliffs(const char* path) { CalcCliffs(const_cast<Name>(path)); }
inline void TranslateString2(char* Dst, const size_t bufsize, const char* Src) { TranslateString2(Dst, bufsize, const_cast<Name>(Src)); }
inline void TranslateString(char* Dst, const char* Src) { TranslateString2(Dst, sizeof(Dst), const_cast<Name>(Src)); } // Updated to re-direct to TranslateString2.
inline void Network_SetString(const char* name, const char* value) { Network_SetString(const_cast<Name>(name), const_cast<Name>(value)); }
inline void Network_SetInteger(const char* name, const int value) { Network_SetInteger(const_cast<Name>(name), value); }
inline bool GetPathPoints(const char* path, size_t& bufSize, float* pData) { return GetPathPoints(const_cast<Name>(path), bufSize, pData); }
inline void SetPosition(const Handle h, const Matrix &m) { SetPosition(h, m); }
inline void Build(const Handle me, const char* odf, const int priority = 1) { Build(me, const_cast<char *>(odf), priority); }
inline void GiveWeapon(const Handle me, const int slot, const char* weapon) { GiveWeapon(me, slot, const_cast<Name>(weapon)); }

// Function Overloads, these are various name overloads for stock functions that take in different types of arguments by default. Does back end fiddly work for you.
// Sets a handle to the position of another Handle.
inline void SetPosition(const Handle h, const Vector v) { SetVectorPosition(h, v); } // Name Overload for SetVectorPosition.
inline void SetPosition(const Handle h1, const Handle h2) { SetVectorPosition(h1, GetPosition(h2)); }
inline void SetPositionPath(const Handle h1, const char* Path, const int Point = 0) { SetVectorPosition(h1, GetVectorFromPath(Path, Point)); }
// Version that uses a Matrix to take rotation into account.
extern void SetPositionM(const Handle h1, const Handle h2);
// Gets the distance between a Handle and a Vector/Matrix.
inline float GetDistance(const Handle h, const Vector v) { return GetDistance2D(GetPosition(h), v); }
inline float GetDistance(const Handle h, const Matrix m) { return GetDistance2D(GetPosition(h), m.posit); }
// Goto that takes a Matrix/Path Point.
inline void Goto(const Handle h, const Matrix Position, const int Priority = 1) { Goto(h, Position.posit, Priority); }
inline void Goto(const Handle h, const char* Path, const int Point, const int Priority) { Goto(h, GetVectorFromPath(Path, Point), Priority); }
// Command for Mine that take a Handle/Vector/Matrix/Path Point.
//inline void Mine(const Handle h, const Vector Where, const int Priority = 1) { SetCommand(h, CMD_LAY_MINES, Priority, 0, Where); }
inline void Mine(const Handle h, const Matrix Where, const int Priority = 1) { Mine(h, Where.posit, Priority); }
inline void Mine(const Handle me, const Handle him, int Priority = 1) { Mine(me, GetPosition(him), Priority); }
inline void Mine(const Handle h, const char* Path, const int Point, const int Priority) { Mine(h, GetVectorFromPath(Path, Point), Priority); }
// Command for Dropoff that take a Handle/Vector/Matrix/Path Point.
inline void Dropoff(const Handle h, const Matrix Where, const int Priority = 1) { Dropoff(h, Where.posit, Priority); }
inline void Dropoff(const Handle me, const Handle him, const int Priority = 1) { Dropoff(me, GetPosition(him), Priority); }
inline void Dropoff(const Handle h, const char* Path, const int Point, const int Priority) { Dropoff(h, GetVectorFromPath(Path, Point), Priority); }
// Command for Retreat that take a Vector/Matrix/Path Point.
inline void Retreat(const Handle h, const Vector Where, const int Priority = 1) { SetIndependence(h, Independence_Off); Goto(h, Where, Priority); }
inline void Retreat(const Handle h, const Matrix Where, const int Priority = 1) { Retreat(h, Where.posit, Priority); }
inline void Retreat(const Handle h, const char* Path, const int Point, const int Priority) { Retreat(h, GetVectorFromPath(Path, Point), Priority); }
// GetCircularPos.
inline Vector GetCircularPos(const Vector &CenterPos, const float Radius, const float Angle) { Vector Pos(0,0,0);	SetCircularPos(CenterPos, Radius, Angle, Pos); return Pos; }
// Version of StartAudio3D that takes a Vector and a Matrix.
inline DLLAudioHandle StartAudio3D(const char *const name, const Vector Pos, const DLLAudioCategory cat = AUDIO_CAT_UNKNOWN, const bool loop = false) { return StartAudio3D(name, Pos.x, Pos.y, Pos.z, cat, loop); }
inline DLLAudioHandle StartAudio3D(const char *const name, const Matrix Mat, const DLLAudioCategory cat = AUDIO_CAT_UNKNOWN, const bool loop = false) { return StartAudio3D(name, Mat.posit, cat, loop); }
// Version of Setcommand that takes a string for the param. e.g. "SetCommand2(me, CMD_BUILD, 0, "make_me_a_silo", "absilo");
//inline void SetCommand2(Handle me, int cmd, int priority = 0, Handle who = 0, const char* path = NULL, char *param = NULL) { SetCommand(me, cmd, priority, who, path, CalcCRC(param)); }
// Get Position that takes a path point.
inline Vector GetPosition(const char* Path, const int Point = 0) { return GetVectorFromPath(Path, Point); }
inline Vector GetPosition2(const Handle h) { Vector v(0, 0, 0); GetPosition2(h, v); return v; }
// Version that takes a Handle, Matrix, and Path name/point.
inline Vector GetPositionNear(const Handle h, const float MinRadiusAway, const float MaxRadiusAway) { return GetPositionNear(GetPosition(h), MinRadiusAway, MaxRadiusAway); }
inline Vector GetPositionNear(const Matrix m, const float MinRadiusAway, const float MaxRadiusAway) { return GetPositionNear(m.posit, MinRadiusAway, MaxRadiusAway); }
inline Vector GetPositionNear(const char* Path, const int Point, const float MinRadiusAway, const float MaxRadiusAway) { return GetPositionNear(GetVectorFromPath(Path, Point), MinRadiusAway, MaxRadiusAway); }
// Version of CameraObject that takes a Vector for position.
inline bool CameraObject(const Handle object_handle, const Vector pos, const Handle target_handle) { return CameraObject(object_handle, pos.x, pos.y, pos.z, target_handle); }
// Version that takes a Vector, a Matrix, and a Path/Point.
inline float TerrainFindFloor(const Vector pos) { return TerrainFindFloor(pos.x, pos.z); }
inline float TerrainFindFloor(const Matrix mat) { return TerrainFindFloor(mat.posit.x, mat.posit.z); }
inline float TerrainFindFloor(const char* Path, const int Point = 0) { Vector Test = GetVectorFromPath(Path, Point); return TerrainFindFloor(Test.x, Test.z); }
inline float TerrainFindFloor(const Handle h) { return TerrainFindFloor(GetPosition(h)); }
// Version that takes a Matrix and Path/Point.
inline bool TerrainIsWater(const Matrix mat) { return TerrainIsWater(mat.posit); }
inline bool TerrainIsWater(const char* Path, const int Point = 0) { return TerrainIsWater(GetVectorFromPath(Path, Point)); }
inline bool TerrainIsWater(const Handle h) { return TerrainIsWater(GetPosition(h)); }
// Version that takes a Matrix and Path/Point.
inline bool TerrainGetHeightAndNormal(const Matrix& mat, float& outHeight, Vector& outNormal, const bool useWater) { return TerrainGetHeightAndNormal(mat.posit, outHeight, outNormal, useWater); }
inline bool TerrainGetHeightAndNormal(const char* Path, const int Point, float& outHeight, Vector& outNormal, const bool useWater) { return TerrainGetHeightAndNormal(GetVectorFromPath(Path, Point), outHeight, outNormal, useWater); }
inline bool TerrainGetHeightAndNormal(const Handle h, float& outHeight, Vector& outNormal, const bool useWater) { return TerrainGetHeightAndNormal(GetPosition(h), outHeight, outNormal, useWater); }
// Version of SetTeamColor that takes a DWORD Color and Vector Color.
inline void SetTeamColor(const int Team, const DWORD Color) { SetTeamColor(Team, RGBA_GETRED(Color), RGBA_GETGREEN(Color), RGBA_GETBLUE(Color)); }
inline void SetTeamColor(const int Team, const Vector Color) { SetTeamColor(Team, int(Color.x), int(Color.y), int(Color.z)); }
// Version that returns a Vector Color
inline Vector GetFFATeamColor(const TEAMCOLOR_TYPE Type, const int Team) { int r = 0, g = 0, b = 0; GetFFATeamColor(Type, Team, r, g, b); return Vector(float(r),float(g),float(b)); }
inline Vector GetTeamStratColor(const TEAMCOLOR_TYPE Type, const int Team) { int r = 0, g = 0, b = 0; GetTeamStratColor(Type, Team, r, g, b); return Vector(float(r),float(g),float(b)); }
inline Vector GetTeamStratIndividualColor(const TEAMCOLOR_TYPE Type, const int Team) { int r = 0, g = 0, b = 0; GetTeamStratIndividualColor(Type, Team, r, g, b); return Vector(float(r),float(g),float(b)); }
// Version that takes a float for the Height.
inline bool CameraPath(const char* path_name, const float height, const int speed, const Handle target_handle) { return CameraPath(const_cast<Name>(path_name), int(height), speed, target_handle); }

// Custom GetODF functions that Handle opening/closing the ODF, as well as one level of ODF Inheritence. Use Spairingly or by themselves.
extern int GetODFHexInt(const Handle h, const char* block, const char* name, int* value = NULL, const int defval = 0);
extern int GetODFInt(const Handle h, const char* block, const char* name, int* value = NULL, const int defval = 0);
extern int GetODFLong(const Handle h, const char* block, const char* name, long* value = NULL, const long defval = 0);
extern int GetODFFloat(const Handle h, const char* block, const char* name, float* value = NULL, const float defval = 0.0f);
extern int GetODFDouble(const Handle h, const char* block, const char* name, double* value = NULL, const double defval = 0.0);
extern int GetODFChar(const Handle h, const char* block, const char* name, char* value = NULL, const char defval = '\0');
extern int GetODFBool(const Handle h, const char* block, const char* name, bool* value = NULL, const bool defval = false);
extern int GetODFString(const Handle h, const char* block, const char* name, size_t ValueLen = 0, char* value = NULL, const char* defval = NULL);
extern int GetODFColor(const Handle h, const char* block, const char* name, DWORD* value = NULL, const DWORD defval = 0);
extern int GetODFVector(const Handle h, const char* block, const char* name, Vector* value = NULL, const Vector defval = Vector(0.0f, 0.0f, 0.0f));

// Versions that use built-in single level of ODF Inheritence, pass in ODFName as file1, and the classlabel string with .odf appended as file2, and it returns the value. NOTE: These assume the ODF is already Open. 
extern int GetODFHexInt(const char* file1, const char* file2, const char* block, const char* name, int* value = NULL, const int defval = 0); //{ return GetODFHexInt(file1, block, name, value, defval) || GetODFHexInt(file2, block, name, value, defval); }
extern int GetODFInt(const char* file1, const char* file2, const char* block, const char* name, int* value = NULL, const int defval = 0); // { return GetODFInt(file1, block, name, value, defval) || GetODFInt(file2, block, name, value, defval); }
extern int GetODFLong(const char* file1, const char* file2, const char* block, const char* name, long* value = NULL, const long defval = 0); // { return GetODFLong(file1, block, name, value, defval) || GetODFLong(file2, block, name, value, defval); }
extern int GetODFFloat(const char* file1, const char* file2, const char* block, const char* name, float* value = NULL, const float defval = 0.0f); // { return GetODFFloat(file1, block, name, value, defval) || GetODFFloat(file2, block, name, value, defval); }
extern int GetODFDouble(const char* file1, const char* file2, const char* block, const char* name, double* value = NULL, const double defval = 0.0); // { return GetODFDouble(file1, block, name, value, defval) || GetODFDouble(file2, block, name, value, defval); }
extern int GetODFChar(const char* file1, const char* file2, const char* block, const char* name, char* value = NULL, const char defval = '\0'); // { return GetODFChar(file1, block, name, value, defval) || GetODFChar(file2, block, name, value, defval); }
extern int GetODFBool(const char* file1, const char* file2, const char* block, const char* name, bool* value = NULL, const bool defval = false); // { return GetODFBool(file1, block, name, value, defval) || GetODFBool(file2, block, name, value, defval); }
extern int GetODFString(const char* file1, const char* file2, const char* block, const char* name, size_t ValueLen = 0, char* value = NULL, const char* defval = NULL); // { return GetODFString(file1, block, name, ValueLen, value, defval) || GetODFString(file2, block, name, ValueLen, value, defval); }
extern int GetODFColor(const char* file1, const char* file2, const char* block, const char* name, DWORD* value = NULL, const DWORD defval = 0); // { return GetODFColor(file1, block, name, value, defval) || GetODFColor(file2, block, name, value, defval); }
extern int GetODFVector(const char* file1, const char* file2, const char* block, const char* name, Vector* value = NULL, const Vector defval = Vector(0.0f, 0.0f, 0.0f)); // { return GetODFVector(file1, block, name, value, defval) || GetODFVector(file2, block, name, value, defval); }


////////// Things from BZScriptor tool.
// Sets the angle of an object, in centigrees. 
extern void SetAngle(const Handle h, const float Degrees);
// Internal math functions used in setting the Angle of an object in the BuildAngleObject functions, and the SetAngle function.
extern Vector HRotateFront(const Vector Front, const float HAngleDifference);

// Camera Pos function from Bob "BS-er" Stewert, moves the camera between two positions at the incrument speed. Tweaked by GBD and Ken Miller.
extern bool CameraPos(const Handle me, const Handle him, const Vector &PosA, const Vector &PosB, const float Increment);

// Replaces an object with another object, retaining as much information about it as possible, with optional team switching, height offset, replacing of 
// weapons with what it currently has, group, if it can be sniped, and current command info (may not work for all commands).
Handle ReplaceObject(const Handle h, const char *ODF = NULL, const int Team = -1, const float HeightOffset = 0.0f, const int Empty = -1,
	const bool RestoreWeapons = true, const int Group = -1, const int Snipe = -1, const bool KeepCommand = true, const int NewCommand = -1, const Handle NewWho = -1, const Vector NewWhere = Vector(0, 0, 0));
//////////

////////// New things used by BZScriptor Tool functions.
// Returns the current TPS rate in use. (and sets it for the clients)
extern int GetTPS(void);

// Gets a position as a Matrix.
inline Matrix GetMatrixPosition(const Handle h) { Matrix m; GetPosition(h, m); return m; }

// Version that gets/sets a percent.
extern float GetScavengerScrap(const Handle h);
extern void SetScavengerScrap(const Handle h, const float Percent);

// Parellels to GetHealth / GetAmmo, sets them to a % of max.
extern void SetHealth(const Handle h, const float Percent);
extern void SetAmmo(const Handle h, const float Percent);

// IsEmpty, returns true if it is a vehicle and empty, else returns false.
inline bool IsEmpty(const Handle h) { if (!IsAround(h)) return false; else return (!IsNotDeadAndPilot2(h) && !IsPlayer(h)); };

// Counts the number of taps on an object. Special note: If using this in AddObject, set AddObject to true to properly count taps. AddObject creation makes a dummy tap on the end that requires more detailed check to skip.
extern int GetTapCount(const Handle h, const bool IgnoreInvincible = false, const bool AddObject = false);

// This function is similar to GetPilotClass, but 1: Strips off the .odf file extention, if present, and 2: Doesn't return NULL.
extern bool GetPilotConfig(const Handle h, char *ODFName);
// Version that returns a const char *. NOTE: This value gets stomped over the next call, so save it off locally, not via const char * if you plan to use more then one value retrieved by this function.
extern const char *GetPilotConfig(const Handle h);

// Replace an object's weapons, with optional 2nd object ODF. Takes a Handle and an array of PreviousWeapons[MAX_HARDPOINTS]. Returns false if the handle passed in doesn't exist.
// NOTE: The array passed into these MUST have a size of 5. NewWeapons[MAX_HARDPOINTS] is optional, if not present it will restore old weapons from PreviousODF, if present.
extern bool ReplaceWeapons(const Handle h, const char NewWeapons[][MAX_ODF_LENGTH], const float PreviousLocalAmmo[] = NULL, const char *PreviousODF = NULL);

// This function checks the validity of a CFG name retrieved by GetObjInfo Get_CFG/Get_ODF, and cuts off the :## and/or .odf from the end. 
// Useful for checking things built by factories.
extern void CheckODFName(char *ODFName);
//////////


// IsInfo that takes in a handle.
extern bool IsInfo(const Handle h);
// Returns true if Handle me is following Handle him.
extern bool IsFollowing(const Handle me, const Handle him);

// This function checks the validity of an ODF name retrieved by GetObjInfo Get_ODF, and cuts off the .odf from the end. Returns true if successful, otherwise returns false.
extern bool GetODFName(const Handle h, char *ODFName);

// Combines String1 and String2 into the ReturnString value. Returns true if successful, returns false if it failed. This prevents a buffer ovreflow internally. 
extern bool CombineStrings(char *ReturnString, const char *String1, const char *String2, const size_t MaxSize = MAX_ODF_LENGTH);

// Special OpenODF that only opens it if it isn't in the below ODFNames list. 
// Note: If you use this, or the below GetODF* functions, please do NOT use the traditional OpenODF(), and do NOT use the CloseODF() functions.
extern bool OpenODF2(const char *name);

// Closes all ODFs in memory opened via OpenODF2. 
// Note: If you use OpenODF2, or the below GetODF* functions, this MUST be called before the mission closes! Either in the ~Destructor, or in PostRun(). 
extern void CloseOpenODFs(void);

// Opens/Closes and writes a message to a specified file.
extern bool FileOpen(const char *Filename, const bool Append = true);
extern void FileClose(const char *Filename);
extern void CloseOpenFiles();

// fnv-1a hash
inline unsigned int Hash(const unsigned char byte, unsigned int hash = 2166136261u)
{
	hash ^= byte;
	hash *= 16777619u;
	return hash;
}
inline unsigned int Hash(const void *data, size_t len, unsigned int hash = 2166136261u)
{
	const unsigned char * d = static_cast<const unsigned char *>(data);
	const unsigned char * const e = d + len;
	while (d < e)
		hash = Hash(*d++, hash);
	return hash;
}
inline unsigned int Hash(const char *string, unsigned int hash = 2166136261u)
{
	if (string == 0)
		return 0;
	for (const char *s = string; *s != 0; ++s)
		hash = Hash(unsigned char(tolower(*s)), hash);
	return hash;
}

// Public Struct for General Object Info. Functions can use whichever values they want from this.
struct HandleInfo
{
	Handle Tug; // GetTug(h);
	Handle Target; // GetTarget(h);
	Handle CurrWho; // GetCurrentWho(h);
	Handle Tap[MAX_TAPS]; // This object's Taps.
	char ODFName[MAX_ODF_LENGTH]; // ODFName. GetODFName(h); (GetObjInfo(h, Get_CFG w/o :# or Get_ODF w/o .odf)
	char Label[MAX_NAME_LENGTH]; // GetLabel(h);
	char ObjectiveName[MAX_NAME_LENGTH]; // GetObjectiveName(h);, strcpy_s()
	char PilotClass[MAX_ODF_LENGTH]; // GetPilotConfig //GetPilotClass, strcpy_s(), stripoff .odf.
	int Team; // GetTeamNum(h);
	int PerceivedTeam; // GetPerceivedTeam(h);
	long MaxHealth; // GetMaxHealth(h);
	long CurHealth; // GetCurHealth(h);
	long MaxAmmo; // GetMaxAmmo(h);
	long CurAmmo; // GetCurAmmo(h);
	float HealthP; // GetHealth(h);
	float AmmoP; // GetAmmo(h);
	char Weapons[MAX_HARDPOINTS][MAX_ODF_LENGTH]; // for i = 0; ... GetObjInfo(h, Get_Weapon0Config+i
	float LocalAmmo[MAX_HARDPOINTS]; // for i = 0, ... GetCurLocalAmmo(h, i);
	long WeaponMask; // GetWeaponMask(h);
	int CurScrap; // GetScavengerCurScrap(h);
	int MaxScrap; // GetScavengerMaxScrap(h);
	Matrix Position; // GetMatrixPosition(h);
	//Vector Position; // GetPosition(h);
	//Vector GetFront; // GetFront(h);
	Vector Velocity; // GetVelocity(h);
	Vector Omega; // GetOmega(h);
	bool IsDeployed; // IsDeployed(h);
	int ScrapCost; // GetBaseScrapCost(h);
	float LifeSpan; // GetRemainingLifespan(h);
	int Independence; // GetIndependence(h);
	int Skill; // GetSkill(h);
	int Group; // GetGroup(h);
	bool CanSnipe; // GetCanSnipe(h);
	int CurrCommand; // GetCurrentCommand(h);
	Vector CurrWhere; // GetCurrentCommandWhere(h);
	//bool HasPilot; // HasPilot(h); // DEPRECIATED, returns FALSE for buildings/non drivable things. Use IsEmpty instead.
	bool IsPlayer; // IsPlayer(h);
	// New variables.
	float ScrapP; // GetScavengerScrap(h);
	bool IsEmpty; // IsEmpty(h);
	// Custom ODF Variables.
	//int ScrapValue; // For destruction moneys.
	int TapCount; // GetTapCount(h);

	HandleInfo() { memset(this, 0, sizeof(HandleInfo)); };
};

#endif
