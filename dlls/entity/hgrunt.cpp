/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// hgrunt
//=========================================================

//=========================================================
// Hit groups!
//=========================================================
/*
  1 - Head
  2 - Stomach
  3 - Gun
*/


//MODDD TODO - should the deathSound be cutoff if gibbed? Shouting "Medic!" shortly after exploding does not make much sense.
//             Could make cutting off the deathsounud a general thing for all monsters too, or something to enable / disable for monsters per type.


//NOTICE - ACT_SIGNAL3, the retreat sequence, is completely unused!


#include "extdll.h"
#include "plane.h"
#include "util.h"
#include "cbase.h"
#include "basemonster.h"
#include "schedule.h"
#include "util_model.h"
#include "squadmonster.h"
#include "weapons.h"
#include "talkmonster.h"
#include "soundent.h"
#include "effects.h"
#include "customentity.h"

//MODDD - temporary for debugging.
#include "player.h"

EASY_CVAR_EXTERN(hgruntBrassEjectForwardOffset)
EASY_CVAR_EXTERN(gruntsCanHaveMP5Grenade)
EASY_CVAR_EXTERN(animationFramerateMulti)
EASY_CVAR_EXTERN(noFlinchOnHard)
EASY_CVAR_EXTERN(hgruntSpeedMulti)
EASY_CVAR_EXTERN(hgruntForceStrafeFireAnim)
EASY_CVAR_EXTERN(hgruntLockRunAndGunTime)
EASY_CVAR_EXTERN(sv_germancensorship)
EASY_CVAR_EXTERN(hgruntAllowStrafeFire)
EASY_CVAR_EXTERN(hgruntTinyClip)
EASY_CVAR_EXTERN(hgruntStrafeAlwaysHasAmmo)
EASY_CVAR_EXTERN(hgruntRunAndGunDistance)
EASY_CVAR_EXTERN(thatWasntPunch)
EASY_CVAR_EXTERN(hgruntPrintout)
EASY_CVAR_EXTERN(hgruntRunAndGunDotMin)
//EASY_CVAR_EXTERN(testVar)
EASY_CVAR_EXTERN(hgruntLockStrafeTime)
EASY_CVAR_EXTERN(hgruntMovementDeltaCheck)
EASY_CVAR_EXTERN(hgruntStrafeAnimSpeedMulti)
EASY_CVAR_EXTERN(hgruntRunAndGunAnimSpeedMulti)
extern BOOL globalPSEUDO_germanModel_hgruntFound;
EASY_CVAR_EXTERN(hgruntAllowGrenades)





//=========================================================
// monster-specific DEFINE's
//=========================================================

//MODDD - new constant.  Only use one head.
#define FORCE_ONE_HEAD 1

//MODDD - new constant.  Force spawning with MP5.
#define FORCE_MP5 1

//MODDD - changed for debugging.
#define GRUNT_CLIP_SIZE					36 // how many bullets in a clip? - NOTE: 3 round burst sound, so keep as 3 * x!

#define GRUNT_VOL						0.35		// volume of grunt sounds
#define GRUNT_ATTN						ATTN_NORM	// attenutation of grunt sentences
#define HGRUNT_LIMP_HEALTH				20
//MODDD - as-is.  apparently unused.  ?
#define HGRUNT_DMG_HEADSHOT				( DMG_BULLET | DMG_CLUB )	// damage types that can kill a grunt with a single headshot.

//MODDD - now 1.
//#define HGRUNT_NUM_HEADS				2 // how many grunt heads are there?
#define HGRUNT_NUM_HEADS				1

#define HGRUNT_MINIMUM_HEADSHOT_DAMAGE	15 // must do at least this much damage in one shot to head to score a headshot kill
#define HGRUNT_SENTENCE_VOLUME			(float)0.35 // volume of grunt sentences

#define HGRUNT_9MMAR				( 1 << 0)
#define HGRUNT_HANDGRENADE			( 1 << 1)
#define HGRUNT_GRENADELAUNCHER		( 1 << 2)
#define HGRUNT_SHOTGUN				( 1 << 3)

#define HEAD_GROUP					1
#define HEAD_GRUNT					0
#define HEAD_COMMANDER				1
#define HEAD_SHOTGUN				2
#define HEAD_M203					3

/*
//ORIGINAL
#define GUN_GROUP					2
#define GUN_MP5						0
#define GUN_SHOTGUN					1
#define GUN_NONE					2
*/

//ADVICE
#define GUN_GROUP					2
#define GUN_MP5						0
#define GUN_SHOTGUN					0
#define GUN_NONE					1

/*
//SUGGESTION? for me before the hgrunt model edit I guess
#define GUN_GROUP					2
#define GUN_MP5						0
#define GUN_SHOTGUN					0
#define GUN_NONE					2
*/

//=========================================================
// monster-specific conditions
//=========================================================
#define bits_COND_GRUNT_NOFIRE	( bits_COND_SPECIAL1 )


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define HGRUNT_AE_RELOAD		( 2 )
#define HGRUNT_AE_KICK			( 3 )
#define HGRUNT_AE_BURST1		( 4 )
#define HGRUNT_AE_BURST2		( 5 )
#define HGRUNT_AE_BURST3		( 6 )
#define HGRUNT_AE_GREN_TOSS		( 7 )
#define HGRUNT_AE_GREN_LAUNCH	( 8 )
#define HGRUNT_AE_GREN_DROP		( 9 )
#define HGRUNT_AE_CAUGHT_ENEMY	( 10) // grunt established sight with an enemy (player only) that had previously eluded the squad.
#define HGRUNT_AE_DROP_GUN		( 11) // grunt (probably dead) is dropping his mp5.



extern DLL_GLOBAL int	g_iSkillLevel;
extern Schedule_t slGruntSweep[];

int g_fGruntQuestion;				// true if an idle grunt asked a question. Cleared when someone answers.



//sequences in the model. Some sequences have the same display name and so should just be referenced by order
//(numbered index).
enum hgrunt_sequence{  //key: frames, FPS
	SEQ_HGRUNT_WALK1,
	SEQ_HGRUNT_WALK2,
	SEQ_HGRUNT_RUN,
	SEQ_HGRUNT_CRAWL,
	SEQ_HGRUNT_VICTORY_DANCE,
	SEQ_HGRUNT_COWER,
	SEQ_HGRUNT_SM_FLINCH,
	SEQ_HGRUNT_LEFT_LEGS_SM_FLINCH,
	SEQ_HGRUNT_RIGHT_LEGS_SM_FLINCH,
	SEQ_HGRUNT_RIGHT_ARM_FLINCH,
	SEQ_HGRUNT_LEFT_ARM_FLINCH,
	SEQ_HGRUNT_LAUNCH_GRENADE,
	SEQ_HGRUNT_THROW_GRENADE,
	SEQ_HGRUNT_IDLE1,
	SEQ_HGRUNT_IDLE2,
	SEQ_HGRUNT_COMBAT_IDLE,
	SEQ_HGRUNT_FRONT_KICK,
	SEQ_HGRUNT_CROUCHING_IDLE,
	SEQ_HGRUNT_CROUCHING_WAIT,
	SEQ_HGRUNT_CROUCHING_MP5,
	SEQ_HGRUNT_STANDING_MP5,
	SEQ_HGRUNT_RELOAD_MP5,
	SEQ_HGRUNT_CROUCHING_SHOTGUN,
	SEQ_HGRUNT_STANDING_SHOTGUN,
	SEQ_HGRUNT_RELOAD_SHOTGUN,
	SEQ_HGRUNT_ADVANCE_SIGNAL,
	SEQ_HGRUNT_FLANK_SIGNAL,
	SEQ_HGRUNT_RETREAT_SIGNAL,
	SEQ_HGRUNT_DROP_GRENADE,
	SEQ_HGRUNT_LIMPING_WALK,
	SEQ_HGRUNT_LIMPING_RUN,
	SEQ_HGRUNT_180L,
	SEQ_HGRUNT_180R,
	SEQ_HGRUNT_STRAFE_LEFT,
	SEQ_HGRUNT_STRAFE_RIGHT,
	SEQ_HGRUNT_STRAFE_LEFT_FIRE,
	SEQ_HGRUNT_STRAFE_RIGHT_FIRE,
	SEQ_HGRUNT_RUN_AND_GUN,
	SEQ_HGRUNT_DIE_BACK_1,
	SEQ_HGRUNT_DIE_FORWARD,
	SEQ_HGRUNT_DIE_SIMPLE,
	SEQ_HGRUNT_DIE_BACKWARDS,
	SEQ_HGRUNT_DIE_HEADSHOT,  //TODO - specifcally call for perhaps??
	SEQ_HGRUNT_DIE_GUTSHOT,
	//...rest continued at barnacled1.  This is fine.

};



//=========================================================
// monster-specific schedule types
//=========================================================
enum
{
	SCHED_GRUNT_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE,// move to a location to set up an attack against the enemy. (usually when a friendly is in the way).
	SCHED_GRUNT_COVER_AND_RELOAD,
	SCHED_GRUNT_SWEEP,
	SCHED_GRUNT_FOUND_ENEMY,
	SCHED_GRUNT_REPEL,
	SCHED_GRUNT_REPEL_ATTACK,
	SCHED_GRUNT_REPEL_LAND,
	SCHED_GRUNT_WAIT_FACE_ENEMY,
	SCHED_GRUNT_TAKECOVER_FAILED,// special schedule type that forces analysis of conditions and picks the best possible schedule to recover from this type of failure.
	SCHED_GRUNT_ELOF_FAIL,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_GRUNT_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_GRUNT_SPEAK_SENTENCE,
	TASK_GRUNT_CHECK_FIRE,
	//MODDD
	TASK_HGRUNT_PICK_STRAFE_ACT,
	TASK_HGRUNT_STRAFEPATH,
};



class CHGrunt : public CSquadMonster
{
public:
	CHGrunt(void);

	BOOL hgruntMoveAndShootDotProductPass;

	void EXPORT tempStrafeTouch(CBaseEntity *pOther);
	void EXPORT		partyUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	
	float strafeFailTime;
	float runAndGunFailTime;

	Vector vecPrevOrigin;
	float nextPositionCheckTime;

	BOOL runAndGun;
	Vector lastHeadHit;

	int getClipSize(void);

	void updateStrafeStatus(void);
	void onNewRouteNode(void);

	BOOL outOfAmmoStrafeFireBlock(void);
	BOOL hgruntAllowStrafe(void);
	BOOL hgruntAllowStrafeFire(void);
	BOOL getIsStrafeLocked(void);
	BOOL canDoOpportunisticStrafe(void);
	float findCoverInDirection(const Vector& arg_vecStart, const float& arg_vecDistanceCompete, const Vector& arg_inDir, const float& arg_maxDist, Vector* arg_vecDirFeedback);
	float findCoverInDirection(const Vector& arg_vecStart, const float& arg_vecDistanceCompete, const Vector& arg_inDir, const float& arg_maxDist, Vector* arg_vecDirFeedback, BOOL canTryAlternateDegrees);

	float lastStrafeCoverCheck;

	BOOL missingHeadPossible;
	BOOL friendlyFireStrafeBlock;
	BOOL strafeCanFire;

	void onAnimationLoop(void);

	int runAndGunSequenceID;

	int tempStrafeAct;

	int strafeMode;
	int idealStrafeMode;
	//-1 = not strafing.
	//0 = strafe left.
	//1 = strafe left + fire.
	//2 = strafe right.
	//3 = strafe right + fire.

	void MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );

	//MODDD
	static const char *pAttackHitSounds[];
	//MODDD - override here, for testing.
	void MonsterThink(void);
	
	//MODDD - new. See whether a headshot was dealt to the corpse.
	GENERATE_DEADTAKEDAMAGE_PROTOTYPE
	
	GENERATE_KILLED_PROTOTYPE

	BOOL usesAdvancedAnimSystem(void);
	int LookupActivityHard(int activity);
	int tryActivitySubstitute(int activity);
	void MakeIdealYaw( Vector vecTarget );
	void startReanimation(void);

	void moveAnimUpdate(void);
	
	//MODDD - NEW.  Overriding 
	virtual void StartMonster(void);
	
	void Spawn( void );
	void Precache( void );
	void SetYawSpeed ( void );
	int  Classify ( void );
	BOOL isOrganic(void){return !CanUseGermanModel();}
	Vector EyePosition(void);
	Vector EyeOffset(void);
	Vector BodyTarget(const Vector &posSrc);
	Vector BodyTargetMod(const Vector &posSrc);
	BOOL getMovementCanAutoTurn(void);
	
	BOOL getGermanModelRequirement(void);
	const char* getGermanModel(void);
	const char* getNormalModel(void);

	int ISoundMask ( void );

	//MODDD
	void HandleEventQueueEvent(int arg_eventID);

	void HandleAnimEvent( MonsterEvent_t *pEvent );
	BOOL FCanCheckAttacks ( void );
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );
	BOOL CheckRangeAttack2 ( float flDot, float flDist );
	void CheckAmmo ( void );
	void SetActivity ( Activity NewActivity );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	void DeathSound( void );
	void PainSound( void );
	void IdleSound ( void );
	Vector GetGunPosition( void );
	Vector GetGunPositionAI(void);
	void Shoot ( void );
	void Shotgun ( void );
	void PrescheduleThink ( void );
	
	BOOL DetermineGibHeadBlock(void);
	
	GENERATE_GIBMONSTER_PROTOTYPE

	void SpeakSentence( void );

	int Save( CSave &save );
	int Restore( CRestore &restore );

	//MODDD - moved to CBaseMonster and renamed HumanKick to be callable by other monsters.
	//CBaseEntity	*Kick( void );

	Schedule_t	*GetSchedule( void );
	Schedule_t  *GetScheduleOfType ( int Type );

	//MODDD
	
	GENERATE_TRACEATTACK_PROTOTYPE
	GENERATE_TAKEDAMAGE_PROTOTYPE

	int IRelationship ( CBaseEntity *pTarget );

	//MODDD - new.
	void SetObjectCollisionBox( void )
	{
		//easyForcePrintLine("I FALLLLLLLLLLLL %d", pev->deadflag);
		if(pev->deadflag != DEAD_NO){
			pev->absmin = pev->origin + Vector(-65, -65, 0);
			pev->absmax = pev->origin + Vector(65, 65, 72);
			//pev->absmin = pev->origin + Vector(-4, -4, 0);
			//pev->absmax = pev->origin + Vector(4, 4, 4);
		}else{
			CSquadMonster::SetObjectCollisionBox();
		}
	}

	BOOL FOkToSpeak( void );
	void JustSpoke( void );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	// checking the feasibility of a grenade toss is kind of costly, so we do it every couple of seconds,
	// not every server frame.
	float m_flNextGrenadeCheck;
	float m_flNextPainTime;
	float m_flLastEnemySightTime;

	Vector	m_vecTossVelocity;

	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;// only put on the handsign show in the squad's first encounter.
	int	m_cClipSize;

	int m_voicePitch;

	int	m_iBrassShell;
	int	m_iShotgunShell;

	int	m_iSentence;

	static const char *pGruntSentences[];

	//MODDD - see comments below at implementation.
	//int SquadRecruit( int searchRadius, int maxMembers );

	//OH YOU GREASY, CHEEKY LITTLE guy.  I.  WILL.  HURT.  YOU.   (PlayerUse  uses this to judge whether or not this is worth sending a "touch" to).
	int ObjectCaps( void ) { return CSquadMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }

	/*
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) 
	{ 
		easyForcePrintLine("WELL IS HE????? %d %s", m_pfnUse == NULL, pCaller!=NULL?pCaller->getClassname():"blankcaller?");
		if (m_pfnUse) 
			(this->*m_pfnUse)( pActivator, pCaller, useType, value );
	}
	*/

	BOOL canResetBlend0(void);
	BOOL onResetBlend0(void);
	void checkHeadGore(void);
	void checkHeadGore(int iGib );

};


#if REMOVE_ORIGINAL_NAMES != 1
	LINK_ENTITY_TO_CLASS( monster_human_grunt, CHGrunt );
#endif


#if EXTRA_NAMES > 0
	LINK_ENTITY_TO_CLASS( human_grunt, CHGrunt );

	#if EXTRA_NAMES == 2
		LINK_ENTITY_TO_CLASS( hgrunt, CHGrunt );
	#endif

#endif




TYPEDESCRIPTION	CHGrunt::m_SaveData[] =
{
	DEFINE_FIELD( CHGrunt, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CHGrunt, m_flNextPainTime, FIELD_TIME ),
//	DEFINE_FIELD( CHGrunt, m_flLastEnemySightTime, FIELD_TIME ), // don't save, go to zero
	DEFINE_FIELD( CHGrunt, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CHGrunt, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGrunt, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGrunt, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGrunt, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CHGrunt, m_voicePitch, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iBrassShell, FIELD_INTEGER ),
//  DEFINE_FIELD( CShotgun, m_iShotgunShell, FIELD_INTEGER ),
	DEFINE_FIELD( CHGrunt, m_iSentence, FIELD_INTEGER ),

	//MODDD - new
	DEFINE_FIELD( CHGrunt, idealStrafeMode, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CHGrunt, CSquadMonster );

const char *CHGrunt::pGruntSentences[] =
{
	"HG_GREN", // grenade scared grunt
	"HG_ALERT", // sees player
	"HG_MONSTER", // sees monster
	"HG_COVER", // running to cover
	"HG_THROW", // about to throw grenade
	"HG_CHARGE",  // running out to get the enemy
	"HG_TAUNT", // say rude things
};

enum
{
	HGRUNT_SENT_NONE = -1,
	HGRUNT_SENT_GREN = 0,
	HGRUNT_SENT_ALERT,
	HGRUNT_SENT_MONSTER,
	HGRUNT_SENT_COVER,
	HGRUNT_SENT_THROW,
	HGRUNT_SENT_CHARGE,
	HGRUNT_SENT_TAUNT,
} HGRUNT_SENTENCE_TYPES;




const char *CHGrunt::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};



void CHGrunt :: partyUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{

	//easyForcePrintLine("????? %d %d %d", EASY_CVAR_GET(thatWasntPunch), pCaller, pCaller!=NULL?pCaller->IsPlayer():-1 );
	if ( EASY_CVAR_GET(thatWasntPunch) == 1 && pCaller != NULL && pCaller->IsPlayer() )
	{
		//sentence.
		int choice = RANDOM_LONG(0, 4);

		switch(choice){
		case 0:
			EMIT_SOUND_DYN( edict(), CHAN_VOICE, "!HG_SUCKS", 0.87, ATTN_NORM, 0, m_voicePitch);
		break;
		case 1:
			EMIT_SOUND_DYN( edict(), CHAN_VOICE, "!HG_CIVVIES", 0.87, ATTN_NORM, 0, m_voicePitch);
		break;
		case 2:
			EMIT_SOUND_DYN( edict(), CHAN_VOICE, "!HG_MEME", 0.9, ATTN_NORM, 0, m_voicePitch);
		break;
		case 3:
			EMIT_SOUND_DYN( edict(), CHAN_VOICE, "!HG_MEMEB", 0.9, ATTN_NORM, 0, m_voicePitch);
		break;
		case 4:
			EMIT_SOUND_DYN( edict(), CHAN_VOICE, "!hgrunt_gr_pain2", 1.00, ATTN_NORM, 0, m_voicePitch);
		break;
		}
		
		
	}

}

int CHGrunt::getClipSize(void){

	if(EASY_CVAR_GET(hgruntTinyClip) != 1){
		return GRUNT_CLIP_SIZE;
	}else{
		return 3;
	}

}




//MODDD - CUSTOM SCHEDULE: strafe to the immediate left or right.
//        ~A Preliminary check should be done to see if it is possible to strafe left or right, and mark this as a move-goal before calling this schedule.



Task_t	tlhgruntStrafeToLocation[] =
{
	//DISREGARD YOU.  GO TO HELL AND ROT FOREVER YOU RANCID LITTLE BUTTMUNCHER.
	//{ TASK_STOP_MOVING,				(float)0					},

	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	//!!! RELY ON MOVEGOALLOC FROM SCRIPT!
	{ TASK_HGRUNT_PICK_STRAFE_ACT,			(float)0		},
	{ TASK_HGRUNT_STRAFEPATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	//that okay?
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	//that okay?
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slhgruntStrafeToLocation[] =
{
	{
		tlhgruntStrafeToLocation,
		ARRAYSIZE ( tlhgruntStrafeToLocation ),

		/*
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,
		*/
		0,

		bits_SOUND_DANGER,
		"HGruntStrafeToLocation"
	}
};















//=========================================================
// Speak Sentence - say your cued up sentence.
//
// Some grunt sentences (take cover and charge) rely on actually
// being able to execute the intended action. It's really lame
// when a grunt says 'COVER ME' and then doesn't move. The problem
// is that the sentences were played when the decision to TRY
// to move to cover was made. Now the sentence is played after
// we know for sure that there is a valid path. The schedule
// may still fail but in most cases, well after the grunt has
// started moving.
//=========================================================
void CHGrunt :: SpeakSentence( void )
{
	if ( m_iSentence == HGRUNT_SENT_NONE )
	{
		// no sentence cued up.
		return;
	}

	if (FOkToSpeak())
	{
		SENTENCEG_PlayRndSz( ENT(pev), pGruntSentences[ m_iSentence ], HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
		JustSpoke();
	}
}

//=========================================================
// IRelationship - overridden because Alien Grunts are
// Human Grunt's nemesis.
//=========================================================
int CHGrunt::IRelationship ( CBaseEntity *pTarget )
{
	if(EASY_CVAR_GET(thatWasntPunch) == 1){
		//I just don't give a damn man
		return R_NO;
	}
	if ( FClassnameIs( pTarget->pev, "monster_alien_grunt" ) || ( FClassnameIs( pTarget->pev,  "monster_gargantua" ) ) )
	{
		return R_NM;
	}
	return CSquadMonster::IRelationship( pTarget );
}

//NOTICE - the HGrunt needs to be able to force gibs to leave blood impact decals (or not) and still determine the "spawn head" logic in the same way.
BOOL CHGrunt::DetermineGibHeadBlock(void){
	//if bodygroup #1 is set to 1 (headless), spawnHeadBlock should be TRUE, to block spawning a head.
	BOOL spawnHeadBlock = (GetBodygroup(1) == 1);
	return spawnHeadBlock;
}

//=========================================================
// GibMonster - make gun fly through the air.
//=========================================================
GENERATE_GIBMONSTER_IMPLEMENTATION(CHGrunt)
{
	Vector	vecGunPos;
	Vector	vecGunAngles;

	//MODDD - okay, Valve?  WHY use actual numbers if you have named constants?
	//if ( GetBodygroup( 2 ) != 2 )     //(changed to what is clearly meant below)
	if ( GetBodygroup( GUN_GROUP ) != GUN_NONE )
	{// throw a gun if the grunt has one
		GetAttachment( 0, vecGunPos, vecGunAngles );

		//MODDD - force to none even here. Gibbing removes this model instantly, but if this model fades out instead, from
		//german censorship refusing to gub, an hgrunt fading out while holding a gun that already dropped a gun will look strange.
		SetBodygroup( GUN_GROUP, GUN_NONE );

		CBaseEntity *pGun;

#if FORCE_MP5 == 1
		//always drop an mp5, since the original code would've if this grunt didn't have a shotgun (and the shotgun won't be equipped).

		pGun = DropItem( "weapon_9mmAR", vecGunPos, vecGunAngles );

#else
		if (FBitSet( pev->weapons, HGRUNT_SHOTGUN ))
		{
			pGun = DropItem( "weapon_shotgun", vecGunPos, vecGunAngles );
		}
		else
		{
			pGun = DropItem( "weapon_9mmAR", vecGunPos, vecGunAngles );
		}
#endif

		if ( pGun )
		{
			pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
			pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}

#if FORCE_MP5 == 1
		//don't do anything here.  Never drop ARgrenades if we can never have them equipped.
		//...unless we allowed "mp5grenades" (same as ARgrenades) again.
		if(EASY_CVAR_GET(gruntsCanHaveMP5Grenade) == 1){
			if (FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER ))
			{
				pGun = DropItem( "ammo_ARgrenades", vecGunPos, vecGunAngles );
				if ( pGun )
				{
					pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
					pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
				}
			}
		}
#else
		if (FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER ))
		{
			pGun = DropItem( "ammo_ARgrenades", vecGunPos, vecGunAngles );
			if ( pGun )
			{
				pGun->pev->velocity = Vector (RANDOM_FLOAT(-100,100), RANDOM_FLOAT(-100,100), RANDOM_FLOAT(200,300));
				pGun->pev->avelocity = Vector ( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
		}
#endif
	}

	//CBaseMonster::GibMonster(spawnHeadBlock, gibsSpawnDecal );
	GENERATE_GIBMONSTER_PARENT_CALL(CSquadMonster);
}



//=========================================================
// ISoundMask - Overidden for human grunts because they
// hear the DANGER sound that is made by hand grenades and
// other dangerous items.
//=========================================================
int CHGrunt :: ISoundMask ( void )
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_PLAYER	|
			bits_SOUND_DANGER	|
			//MODDD - new
			bits_SOUND_BAIT;
}

//=========================================================
// someone else is talking - don't speak
//=========================================================
BOOL CHGrunt :: FOkToSpeak( void )
{
// if someone else is talking, don't speak
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
		return FALSE;

	if ( pev->spawnflags & SF_MONSTER_GAG )
	{
		if ( m_MonsterState != MONSTERSTATE_COMBAT )
		{
			// no talking outside of combat if gagged.
			return FALSE;
		}
	}

	// if player is not in pvs, don't speak
//	if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
//		return FALSE;

	return TRUE;
}

//=========================================================
//=========================================================
void CHGrunt :: JustSpoke( void )
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(1.5, 2.0);
	m_iSentence = HGRUNT_SENT_NONE;
}

//=========================================================
// PrescheduleThink - this function runs after conditions
// are collected and before scheduling code is run.
//=========================================================
void CHGrunt :: PrescheduleThink ( void )
{
	if ( InSquad() && m_hEnemy != NULL )
	{
		if ( HasConditions ( bits_COND_SEE_ENEMY ) )
		{
			// update the squad's last enemy sighting time.
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		}
		else
		{
			if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
			{
				// been a while since we've seen the enemy
				MySquadLeader()->m_fEnemyEluded = TRUE;
			}
		}
	}
}

//=========================================================
// FCanCheckAttacks - this is overridden for human grunts
// because they can throw/shoot grenades when they can't see their
// target and the base class doesn't check attacks if the monster
// cannot see its enemy.
//
// !!!BUGBUG - this gets called before a 3-round burst is fired
// which means that a friendly can still be hit with up to 2 rounds.
// ALSO, grenades will not be tossed if there is a friendly in front,
// this is a bad bug. Friendly machine gun fire avoidance
// will unecessarily prevent the throwing of a grenade as well.
//=========================================================
BOOL CHGrunt :: FCanCheckAttacks ( void )
{
	if ( !HasConditions( bits_COND_ENEMY_TOOFAR ) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CHGrunt :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	CBaseMonster *pEnemy;

	if ( m_hEnemy != NULL )
	{
		pEnemy = m_hEnemy->MyMonsterPointer();

		if ( !pEnemy )
		{
			return FALSE;
		}
	}

	if ( flDist <= 64 && flDot >= 0.7	&&
		 pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		 pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON )
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// CheckRangeAttack1 - overridden for HGrunt, cause
// FCanCheckAttacks() doesn't disqualify all attacks based
// on whether or not the enemy is occluded because unlike
// the base class, the HGrunt can attack when the enemy is
// occluded (throw grenade over wall, etc). We must
// disqualify the machine gun attack if the enemy is occluded.
//=========================================================
BOOL CHGrunt :: CheckRangeAttack1 ( float flDot, float flDist )
{

	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5 && NoFriendlyFire() )
	{
		TraceResult	tr;

		if ( !m_hEnemy->IsPlayer() && flDist <= 64 )
		{
			// kick nonclients, but don't shoot at them.
			return FALSE;
		}

		Vector vecSrc = GetGunPositionAI();

		// verify that a bullet fired from the gun will hit the enemy before the world.
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTargetMod(vecSrc), ignore_monsters, ignore_glass, ENT(pev), &tr);

		if ( tr.flFraction == 1.0 )
		{
			return TRUE;
		}
	}

	return FALSE;
}

//=========================================================
// CheckRangeAttack2 - this checks the Grunt's grenade
// attack.
//=========================================================
BOOL CHGrunt :: CheckRangeAttack2 ( float flDot, float flDist )
{

	if(EASY_CVAR_GET(hgruntAllowGrenades) == 0){
		return FALSE;
	}

#if FORCE_MP5 == 1

	if(EASY_CVAR_GET(gruntsCanHaveMP5Grenade) == 1){
		if (! FBitSet(pev->weapons, (HGRUNT_HANDGRENADE | HGRUNT_GRENADELAUNCHER)))
		{
			return FALSE;
		}
	}else{
		//just count hand grenades.
		if (! FBitSet(pev->weapons, (HGRUNT_HANDGRENADE)))
		{
			return FALSE;
		}
	}

#else

	if (! FBitSet(pev->weapons, (HGRUNT_HANDGRENADE | HGRUNT_GRENADELAUNCHER)))
	{
		return FALSE;
	}

#endif

	// if the grunt isn't moving, it's ok to check.
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	// assume things haven't changed too much since last time
	if (gpGlobals->time < m_flNextGrenadeCheck )
	{
		return m_fThrowGrenade;
	}

	if ( !FBitSet ( m_hEnemy->pev->flags, FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 && m_vecEnemyLKP.z > pev->absmax.z  )
	{
		//!!!BUGBUG - we should make this check movetype and make sure it isn't FLY? Players who jump a lot are unlikely to
		// be grenaded.
		// don't throw grenades at anything that isn't on the ground!
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}

	Vector vecTarget;

	if (FBitSet( pev->weapons, HGRUNT_HANDGRENADE))
	{
		// find feet
		if (RANDOM_LONG(0,1))
		{
			// magically know where they are
			vecTarget = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
		}
		else
		{
			// toss it to where you last saw them
			vecTarget = m_vecEnemyLKP;
		}
		// vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		// vecTarget = vecTarget + m_hEnemy->pev->velocity * 2;
	}
	else
	{
		// find target
		// vecTarget = m_hEnemy->BodyTarget( pev->origin );
		vecTarget = m_vecEnemyLKP + (m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin);
		// estimate position
		if (HasConditions( bits_COND_SEE_ENEMY))
			vecTarget = vecTarget + ((vecTarget - pev->origin).Length() / gSkillData.hgruntGrenadeSpeed) * m_hEnemy->pev->velocity;
	}

	// are any of my squad members near the intended grenade impact area?
	if ( InSquad() )
	{
		if (SquadMemberInRange( vecTarget, 256 ))
		{
			// crap, I might blow my own guy up. Don't throw a grenade and don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
			m_fThrowGrenade = FALSE;
		}
	}

	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		// crap, I don't want to blow myself up
		m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		m_fThrowGrenade = FALSE;
		return m_fThrowGrenade;
	}


	if (FBitSet( pev->weapons, HGRUNT_HANDGRENADE))
	{
		Vector vecToss = VecCheckToss( pev, GetGunPositionAI(), vecTarget, 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}
	else
	{
		Vector vecToss = VecCheckThrow( pev, GetGunPositionAI(), vecTarget, gSkillData.hgruntGrenadeSpeed, 0.5 );

		if ( vecToss != g_vecZero )
		{
			m_vecTossVelocity = vecToss;

			// throw a hand grenade
			m_fThrowGrenade = TRUE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 0.3; // 1/3 second.
		}
		else
		{
			// don't throw
			m_fThrowGrenade = FALSE;
			// don't check again for a while.
			m_flNextGrenadeCheck = gpGlobals->time + 1; // one full second.
		}
	}



	return m_fThrowGrenade;
}





//MODDD - HEADSHOT!
//=========================================================
// TraceAttack - make sure we're not taking it in the helmet
//=========================================================
GENERATE_TRACEATTACK_IMPLEMENTATION(CHGrunt)
{
	//easyForcePrintLine("HGrunt:: TraceAttack says I took %.2f damage.", flDamage);


	if(ptr->iHitgroup == 11 || ptr->iHitgroup == HITGROUP_HEAD && flDamage >= gSkillData.plrDmg357 && GetBodygroup(1) != 1){
		//a python round (at least)?  Remember that...

		missingHeadPossible = TRUE;
		lastHeadHit = ptr->vecEndPos;

	}else{
		missingHeadPossible = FALSE;
	}


	// check for helmet shot
	if (ptr->iHitgroup == 11)
	{
		// make sure we're wearing one
		//MODDD - Dangit Valve!  Use those constants!  That wasn't so bad though.
		//if (GetBodygroup( 1 ) == HEAD_GRUNT && (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB)))
		if (GetBodygroup( HEAD_GROUP ) == HEAD_GRUNT && (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB)))
		{
			// absorb damage
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
				if(useBulletHitSound){*useBulletHitSound=FALSE;} //deny it.
				useBloodEffect = FALSE;  //don't now.
			}
		}
		// it's head shot anyways
		ptr->iHitgroup = HITGROUP_HEAD;
	}

	//easyForcePrintLine("HGrunt:: TraceAttack ended with %.2f damage.", flDamage);
	GENERATE_TRACEATTACK_PARENT_CALL(CSquadMonster);
}




GENERATE_DEADTAKEDAMAGE_IMPLEMENTATION(CHGrunt){
	checkHeadGore();
	return GENERATE_DEADTAKEDAMAGE_PARENT_CALL(CSquadMonster);
}


GENERATE_KILLED_IMPLEMENTATION(CHGrunt){
	SetUse(NULL);

	//easyForcePrintLine("HGrunt: KILLED. iGib: %d", iGib);

	//head or helmet?
	checkHeadGore(iGib);


	GENERATE_KILLED_PARENT_CALL(CSquadMonster);
}



//=========================================================
// TakeDamage - overridden for the grunt because the grunt
// needs to forget that he is in cover if he's hurt. (Obviously
// not in a safe place anymore).
//=========================================================
GENERATE_TAKEDAMAGE_IMPLEMENTATION(CHGrunt)
{
	Forget( bits_MEMORY_INCOVER );

	//is this okay?
	//m_IdealActivity != m_movementActivity

	//return 0;

	//HACK -
	if(flDamage < gSkillData.plrDmg357){
		//less than python damage?  We've been scammed!
		missingHeadPossible = FALSE;
	}

	



	if(bitsDamageType & DMG_BLAST && m_Activity == ACT_COWER){
		//reduce explosive damage while ducking in cover.

		if(flDamage >= 50){
			//Not as effective, too much damage. Assuming too direct or forceful of an explosion.
			flDamage *= 0.7;
			//at 50 damage, that's 35 adjusted points of damage dealt.

		}else if(flDamage > 15){
			//Formula - scale the factor applied to damage (closer to 1 is less reduction, closer to 0 is more reduction).
			//As in, more damage is scaled less than what was less damage to begin with. 40 damage is less affected than 30 damage. This makes 

			// 0.1, 0.7
			
			//in:  out: factor:  equation:
			//50 -> 35	0.7		(0.3 + ?)*50 = 35
			//40 -> 24	0.6     (0.3 + ?)*40 = 24
			//30 -> 15  0.5     (0.3 + ?)*30 = 15
			//20 -> 8   0.4		(0.3 + ?)*20 = 8
			//<10-> 3  0.3		(0.3 + ?)*10 = 3?
			/*
			(0.3 + ?)*10 = 3

			(0.3 + R)*X = Y

			0.3X + RX = Y

			RX = Y + -0.3X
			R = (Y + -0.3X) / X
			R = Y/X + -0.3


			50, 0.4
			40, 0.3
			30, 0.2
			20, 0.1
			10, 0

			slope = (y2 - y1) / (x2 - x1)
			slope = ydelta / xdelta
			ydelta = 0.3 - 0.2 = 0.1
			xdelta = 30 - 20   =  10
			slope = ydelta / xdelta
			slope = 0.1 / 10
			slope = 0.01

			y = mx + b
			y = 0.01x + b

			0.2 = 0.01*30 + b

			b = -0.1

			50, 35 = 0.4
			40, 24 = 0.3
			30, 15 = 0.2
			20,  8 = 0.1
			10,  3 = 0
			*/

			//flDamage = (flDamage * ((((flDamage - 15)/35 )*0.6)+0.1)  );

			//float damageFactor = 0.01*flDamage + -0.1 + 0.3;
			//flDamage = (damageFactor)*flDamage;

			flDamage = (0.01*flDamage + 0.2)*flDamage;


			
			
		}else{
			//tiny.
			flDamage *= 0.3;
		}
		/*else if(flDamage > 40){
			//ok.
			flDamage *= 0.6;
		}else{
			//good distance? Doesn't hurt as much.
			flDamage *= 0.2;
		}
		*/

	}//END OF blast damage and act_cower (ducking for cover) check.


	//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("WHUT %d :: ACT ideal:%d, cur:%d.", this->m_movementGoal, m_IdealActivity, m_movementActivity));

	//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("PASSSSS???? %d %d %d", pev->deadflag == DEAD_NO, m_pSchedule != slhgruntStrafeToLocation, (m_IdealActivity != m_movementActivity || this->m_movementGoal == MOVEGOAL_NONE)) );






	if(
		m_MonsterState != MONSTERSTATE_PRONE &&
		lastStrafeCoverCheck <= gpGlobals->time &&
		pev->deadflag == DEAD_NO &&
		m_pSchedule != slhgruntStrafeToLocation &&
		(
		m_IdealActivity != m_movementActivity ||
		this->m_movementGoal == MOVEGOAL_NONE
		)
		&&
		//This ensures the damage isn't coming from a continual source (poison, radiation).
		//Moving sideways in response to taking damage from a likely out-of-sight enemy is... odd looking.
		!(bitsDamageTypeMod & (DMG_TIMEDEFFECT|DMG_TIMEDEFFECTIGNORE) )
	)
	{
		//standing?

		lastStrafeCoverCheck = gpGlobals->time + 0.8f;


		//0, 3
		BOOL randomPass;
		//randomPass = -1;

		float healthRatio = pev->health / pev->max_health;

		if(healthRatio < 0.49 || flDamage > gSkillData.plrDmg357){
			//always.
			randomPass = 0;
		}else{

			randomPass = RANDOM_LONG(0,  (int) ((healthRatio - 0.49)/0.51 * 4.4 )   );

		}


		if(randomPass == 0 && m_hEnemy != NULL){

			Vector vecForward;
			Vector vecRight;

			//why not??
			//UTIL_MakeVectorsPrivate ( pev->angles, vecForward, vecRight, NULL );

			vecForward = (m_hEnemy->pev->origin - pev->origin).Normalize();
			vecRight = CrossProduct(vecForward, Vector(0, 0, 1) ).Normalize();


			TraceResult trLeft;
			TraceResult trRight;
			//eyes?
			Vector vecStart = pev->origin;
			Vector vecDestRight = vecStart + vecRight * 500;
			Vector vecDestLeft = vecStart + vecRight * -500;

			float vecStartDistance = (vecStart - m_hEnemy->pev->origin).Length();


			Vector vecLeftBestDir;
			Vector vecRightBestDir;
			float bestCoverLeftDist = findCoverInDirection(vecStart, vecStartDistance, -vecRight, 500, &vecLeftBestDir);
			float bestCoverRightDist = findCoverInDirection(vecStart, vecStartDistance, vecRight, 500, &vecRightBestDir);


			int decision = 0;
			//0 is failed, 1 is left, 2 is right.

			//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("BEST COVER!!! %.2f %.2f", bestCoverLeftDist, bestCoverRightDist) );

			if(bestCoverLeftDist == -1 && bestCoverRightDist == -1){
				//no cover.  Fail.
			}else if(bestCoverLeftDist == bestCoverRightDist){
				//roll for it.
				if(RANDOM_LONG(0, 1) == 0){
					decision = 1;
				}else{
					decision = 2;
				}
			}else if(bestCoverRightDist == -1){
				decision = 1;
			}else if(bestCoverLeftDist == -1){
				decision = 2;
			}else if(bestCoverLeftDist < bestCoverRightDist){
				decision = 1;
			}else if(bestCoverRightDist < bestCoverLeftDist){
				decision = 2;
			}else{
				//that just ain't right
				EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("HGRUNT:::  OMG  WTF  BBQ" ) );
			}


			BOOL pathOkay = FALSE;
			BOOL pathAttempt = FALSE;

			float lengthGoneWith;
			Vector dirChoice;

			if(decision == 1){
				//going left!
				lengthGoneWith = bestCoverLeftDist * 1;
				dirChoice = vecLeftBestDir;

				//well, strafes are... kinda weird.  This works, just go with that.
				tempStrafeAct = ACT_STRAFE_RIGHT;

				idealStrafeMode = 2;
				updateStrafeStatus();

				RouteClear();
				this->m_movementGoal = MOVEGOAL_LOCATION;
				m_vecMoveGoal = vecStart + dirChoice * lengthGoneWith;
				pathOkay = FRefreshRoute();
				pathAttempt = TRUE;
				//good enough for run_path?
			}else if(decision == 2){
				//going right!
				lengthGoneWith = bestCoverRightDist * 1;
				dirChoice = vecRightBestDir;

				tempStrafeAct = ACT_STRAFE_LEFT;

				idealStrafeMode = 0;
				updateStrafeStatus();

				RouteClear();
				this->m_movementGoal = MOVEGOAL_LOCATION;
				m_vecMoveGoal = vecStart + dirChoice * lengthGoneWith;
				pathOkay = FRefreshRoute();
				pathAttempt = TRUE;
				//good enough for run_path?
			}else{
				//nothing...
			}

			if(pathOkay && !(m_Route[ m_iRouteIndex ].iType & bits_MF_IS_GOAL) ){
				//this path is not okay!  Can not have multiple nodes.
				pathOkay = FALSE;
			}


			if(m_hEnemy->IsPlayer()){
				//CBasePlayer* tempPlayer = static_cast<CBasePlayer*>(m_hEnemy.Get()->pvPrivateData-> );
				CBaseEntity* thing;
				//CBasePlayer* tempPlayer = (CBaseEntity::Instance(m_hEnemy)) ;
				CBasePlayer* tempPlayer = GetClassPtr((CBasePlayer *)m_hEnemy->pev) ;
				tempPlayer->debugDrawVectRecentGive1 = pev->origin + Vector(0, 0, 10);
				tempPlayer->debugDrawVectRecentGive2 = m_vecMoveGoal + Vector(0, 0, 10);

				//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("OW!!!@@@@@@@@@@@@@@@@@@@@@@ %.2f %d", lengthGoneWith, pathOkay) );


			}//END OF if(m_hEnemy->IsPlayer())


			//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("TESETTT %d %.2f %.2f %.2f", decision, bestCoverLeftDist, bestCoverRightDist, (m_vecMoveGoal - pev->origin).Length()) );


			if(pathOkay){
				//created a path?


				this->SetTouch(&CHGrunt::tempStrafeTouch);
				ChangeSchedule(slhgruntStrafeToLocation);
			}else{
				if(pathAttempt){
					//cleanup just to be safe.
					RouteClear();
				}
			}

		}//END OF random check

	}//END OF stopped? check.


	return GENERATE_TAKEDAMAGE_PARENT_CALL(CSquadMonster);
}//END OF TakeDamage




float CHGrunt::findCoverInDirection(const Vector& arg_vecStart, const float& arg_vecDistanceCompete, const Vector& arg_inDir, const float& arg_maxDist, Vector* arg_vecDirFeedback){
	return findCoverInDirection(arg_vecStart, arg_vecDistanceCompete, arg_inDir, arg_maxDist, arg_vecDirFeedback, TRUE);
}

float CHGrunt::findCoverInDirection(const Vector& arg_vecStart, const float& arg_vecDistanceCompete, const Vector& arg_inDir, const float& arg_maxDist, Vector* arg_vecDirFeedback, BOOL canTryAlternateDegrees){

	TraceResult tr1;
	//-1 means could not find any cover in that direction.
	float bestCoverDistance = -1;
	float bestCoverDistancePre = -1;

	Vector vecDest = arg_vecStart + arg_inDir * arg_maxDist;

	UTIL_TraceLine(arg_vecStart, vecDest, dont_ignore_monsters, ENT(pev), &tr1);

	float distanceHit = arg_maxDist * tr1.flFraction;
	//float distanceHit = arg_maxDist;


	if(distanceHit >= 60){

		for(int i = 60; i < distanceHit; i+= 30){
			TraceResult trSeeTest;
			//suppose this place...
			Vector vecStartTest = EyePosition() + arg_inDir * i;
			Vector vecDestTest = m_hEnemy->EyePosition() + Vector(0, 0, 10);

			UTIL_TraceLine( vecStartTest, vecDestTest, ignore_monsters, ENT(pev), &trSeeTest);

			if(trSeeTest.flFraction == 1.0 || trSeeTest.pHit == m_hEnemy.Get() ){
				//hit the enemy from the supposed location?  This is not cover.  Try more.
			}else{
				//One more check...
				UTIL_TraceLine( Vector(vecStartTest.x, vecStartTest.y, pev->origin.z + pev->maxs.z * 1.02f), vecDestTest, ignore_monsters, ENT(pev), &trSeeTest);

				if(trSeeTest.flFraction == 1.0 || trSeeTest.pHit == m_hEnemy.Get() ){
					//hit the enemy from the supposed location?  This is not cover.  Try more.
				}else{
					//Lastly...
					float thisDistance = (vecStartTest - m_hEnemy->pev->origin).Length();
					//this shouldn't bring me much closer at most.
					if(arg_vecDistanceCompete - 60 < thisDistance){
						//closest cover found!  Add some padding.
						bestCoverDistancePre = i + 33;
						break;
					}
				}

			}

		}//END OF loop.



	//NEW: including checkLocalMove.

		/*
		if(arg_vecDirFeedback != NULL){
			//feedback the start direction.
			*arg_vecDirFeedback = arg_inDir;
		}
		bestCoverDistance = bestCoverDistancePre;
	return bestCoverDistance;
	*/

	if(bestCoverDistancePre == -1){
		//failed there?  Not a great chance of success.
		return -1;
	}

	if(CheckLocalMove(arg_vecStart, arg_vecStart + arg_inDir * bestCoverDistancePre, NULL, NULL) == LOCALMOVE_VALID){
		//go ahead!
		if(arg_vecDirFeedback != NULL){
			//feedback the start direction.
			*arg_vecDirFeedback = arg_inDir;
		}
		bestCoverDistance = bestCoverDistancePre;
	}else{
		if(canTryAlternateDegrees){
			//a few more tries...
			Vector vecDegUp = getRotatedVectorAboutZAxis(arg_inDir, 9);
			Vector vecDegDown = getRotatedVectorAboutZAxis(arg_inDir, -9);
			float bestCoverDistanceUp = findCoverInDirection(arg_vecStart, arg_vecDistanceCompete, vecDegUp, arg_maxDist * 1.11, NULL, FALSE);
			float bestCoverDistanceDown = findCoverInDirection(arg_vecStart, arg_vecDistanceCompete, vecDegDown, arg_maxDist * 1.11, NULL, FALSE);


			if(bestCoverDistanceUp == -1 && bestCoverDistanceDown == -1){
				//nope.
			}else if(bestCoverDistanceDown == -1){
				bestCoverDistance = bestCoverDistanceUp;
				if(arg_vecDirFeedback != NULL){*arg_vecDirFeedback = vecDegUp;}
			}else if(bestCoverDistanceUp == -1){
				bestCoverDistance = bestCoverDistanceDown;
				if(arg_vecDirFeedback != NULL){*arg_vecDirFeedback = vecDegDown;}
			}else if(bestCoverDistanceDown == bestCoverDistanceUp){
				//Roll for it!
				int roll = RANDOM_LONG(0, 1);
				if(roll == 0){
					bestCoverDistance = bestCoverDistanceDown;
					if(arg_vecDirFeedback != NULL){*arg_vecDirFeedback = vecDegDown;}
				}else{
					bestCoverDistance = bestCoverDistanceUp;
					if(arg_vecDirFeedback != NULL){*arg_vecDirFeedback = vecDegUp;}
				}

			}else if(bestCoverDistanceDown < bestCoverDistanceUp){
				bestCoverDistance = bestCoverDistanceDown;
				if(arg_vecDirFeedback != NULL){*arg_vecDirFeedback = vecDegDown;}
			}else if(bestCoverDistanceUp < bestCoverDistanceDown){
				bestCoverDistance = bestCoverDistanceUp;
				if(arg_vecDirFeedback != NULL){*arg_vecDirFeedback = vecDegUp;}
			}else{
				EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("POTATOES!!!!!!! POTATOES!!!!!!!!!!!!!"));
			}

		}//END OF if(canTryAlternateDegrees)
	}//END OF else

	}//END OF total distance check from left.


	//safety.
	if(bestCoverDistance > 12){
		return bestCoverDistance - 5;
	}else{
		return -1;
	}

}





void CHGrunt::tempStrafeTouch(CBaseEntity *pOther){
	EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("HGRUNT: tempStrafeTouch TOUCHED %s", pOther != NULL ? STRING(pOther->pev->classname) : "NULL"));
	this->SetTouch(NULL);
	TaskFail();
}





//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHGrunt :: SetYawSpeed ( void )
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 150;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 120;
		break;
	case ACT_RANGE_ATTACK2:
		ys = 120;
		break;
	case ACT_MELEE_ATTACK1:
		ys = 120;
		break;
	case ACT_MELEE_ATTACK2:
		ys = 120;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		ys = 30;
		break;
	default:
		ys = 90;
		break;
	}


	if(m_IdealActivity == m_movementActivity && (m_movementActivity == ACT_STRAFE_LEFT || m_movementActivity == ACT_STRAFE_RIGHT) ){
		//if strafing, turn fast.
		ys = 200;
	}

	pev->yaw_speed = ys;
}

void CHGrunt :: IdleSound( void )
{
	if (FOkToSpeak() && (g_fGruntQuestion || RANDOM_LONG(0,1)))
	{
		if (!g_fGruntQuestion)
		{
			// ask question or make statement
			switch (RANDOM_LONG(0,2))
			{
			case 0: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "HG_CHECK", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fGruntQuestion = 1;
				break;
			case 1: // question
				SENTENCEG_PlayRndSz(ENT(pev), "HG_QUEST", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				g_fGruntQuestion = 2;
				break;
			case 2: // statement
				SENTENCEG_PlayRndSz(ENT(pev), "HG_IDLE", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
		}
		else
		{
			switch (g_fGruntQuestion)
			{
			case 1: // check in
				SENTENCEG_PlayRndSz(ENT(pev), "HG_CLEAR", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			case 2: // question
				SENTENCEG_PlayRndSz(ENT(pev), "HG_ANSWER", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch);
				break;
			}
			g_fGruntQuestion = 0;
		}
		JustSpoke();
	}
}


//MODDD - IMPORTANT!  keep in mind for barney's reload (to be added)
//=========================================================
// CheckAmmo - overridden for the grunt because he actually
// uses ammo! (base class doesn't)
//=========================================================
void CHGrunt :: CheckAmmo ( void )
{
	if ( m_cAmmoLoaded <= 0 )
	{
		SetConditions(bits_COND_NO_AMMO_LOADED);
	}
}

//=========================================================
// Classify - indicates this monster's place in the
// relationship table.
//=========================================================
int CHGrunt :: Classify ( void )
{
	return	CLASS_HUMAN_MILITARY;
}



Vector CHGrunt::EyePosition(void){

	//If crouching in the animation, the center of the hitbox (and eyes) go down a little.
	if(
	  pev->sequence == SEQ_HGRUNT_COWER ||
	  pev->sequence == SEQ_HGRUNT_CRAWL ||
	  pev->sequence == SEQ_HGRUNT_CROUCHING_MP5 ||
	  pev->sequence == SEQ_HGRUNT_RELOAD_MP5 ||
	  pev->sequence == SEQ_HGRUNT_CROUCHING_SHOTGUN ||
	  pev->sequence == SEQ_HGRUNT_RELOAD_SHOTGUN ||
	  pev->sequence == SEQ_HGRUNT_LIMPING_RUN ||
	  pev->sequence == SEQ_HGRUNT_CROUCHING_IDLE ||
	  pev->sequence == SEQ_HGRUNT_CROUCHING_WAIT
	)
	{
		return pev->origin + Vector(pev->view_ofs.x, pev->view_ofs.y, pev->view_ofs.z - 18);
	}else{
		//default behavior works... minus a little.
		return pev->origin + Vector(pev->view_ofs.x, pev->view_ofs.y, pev->view_ofs.z - 2);
	}
};

Vector CHGrunt::EyeOffset(void){

	//If crouching in the animation, the center of the hitbox (and eyes) go down a little.
	if(
	  pev->sequence == SEQ_HGRUNT_COWER ||
	  pev->sequence == SEQ_HGRUNT_CRAWL ||
	  pev->sequence == SEQ_HGRUNT_CROUCHING_MP5 ||
	  pev->sequence == SEQ_HGRUNT_RELOAD_MP5 ||
	  pev->sequence == SEQ_HGRUNT_CROUCHING_SHOTGUN ||
	  pev->sequence == SEQ_HGRUNT_RELOAD_SHOTGUN ||
	  pev->sequence == SEQ_HGRUNT_LIMPING_RUN ||
	  pev->sequence == SEQ_HGRUNT_CROUCHING_IDLE ||
	  pev->sequence == SEQ_HGRUNT_CROUCHING_WAIT
	)
	{
		return Vector(pev->view_ofs.x, pev->view_ofs.y, pev->view_ofs.z - 18);
	}else{
		//default behavior works... minus a little.
		return Vector(pev->view_ofs.x, pev->view_ofs.y, pev->view_ofs.z - 2);
	}
};


Vector CHGrunt::BodyTarget(const Vector &posSrc){
	//MODDD - also would checking for some ACTs help ease the sequence checks?

	//If crouching in the animation, the center of the hitbox (and eyes) go down a little.
	if(
	  pev->sequence == SEQ_HGRUNT_COWER ||
	  pev->sequence == SEQ_HGRUNT_CRAWL ||
	  pev->sequence == SEQ_HGRUNT_CROUCHING_MP5 ||
	  pev->sequence == SEQ_HGRUNT_RELOAD_MP5 ||
	  pev->sequence == SEQ_HGRUNT_CROUCHING_SHOTGUN ||
	  pev->sequence == SEQ_HGRUNT_RELOAD_SHOTGUN ||
	  pev->sequence == SEQ_HGRUNT_LIMPING_RUN ||
	  pev->sequence == SEQ_HGRUNT_CROUCHING_IDLE ||
	  pev->sequence == SEQ_HGRUNT_CROUCHING_WAIT
	)
	{
		Vector typicalCenter = CSquadMonster::BodyTarget(posSrc);
		return Vector(typicalCenter.x, typicalCenter.y, typicalCenter.z - 8);
	}else{
		//default behavior works.
		return CSquadMonster::BodyTarget(posSrc);
	}
};

Vector CHGrunt::BodyTargetMod(const Vector &posSrc){

	//If crouching in the animation, the center of the hitbox (and eyes) go down a little.
	if(
	  pev->sequence == SEQ_HGRUNT_COWER ||
	  pev->sequence == SEQ_HGRUNT_CRAWL ||
	  pev->sequence == SEQ_HGRUNT_CROUCHING_MP5 ||
	  pev->sequence == SEQ_HGRUNT_RELOAD_MP5 ||
	  pev->sequence == SEQ_HGRUNT_CROUCHING_SHOTGUN ||
	  pev->sequence == SEQ_HGRUNT_RELOAD_SHOTGUN ||
	  pev->sequence == SEQ_HGRUNT_LIMPING_RUN ||
	  pev->sequence == SEQ_HGRUNT_CROUCHING_IDLE ||
	  pev->sequence == SEQ_HGRUNT_CROUCHING_WAIT
	)
	{
		Vector typicalCenter = CSquadMonster::BodyTarget(posSrc);
		return Vector(typicalCenter.x, typicalCenter.y, typicalCenter.z - 6);
	}else{
		//default behavior works.
		return CSquadMonster::BodyTarget(posSrc);
	}

	// eh.  couldn't we have done this to begin with to piggyback off of BodyTarget without anything else?
	//return CHGrunt::BodyTarget(posSrc);
};











BOOL CHGrunt::getMovementCanAutoTurn(void){
	if(strafeMode == -1){
		return TRUE;
	}else{
		//strafing? forbid the auto turn, can get really confusing to deal with.
		return FALSE;
	}
}//END OF getMovementCanAutoTurn




BOOL CHGrunt::getGermanModelRequirement(void){
	return globalPSEUDO_germanModel_hgruntFound;
}
const char* CHGrunt::getGermanModel(void){
	return "models/g_hgrunt.mdl";
}
const char* CHGrunt::getNormalModel(void){
	return "models/hgrunt.mdl";
}




//MODDD - CHGrunt Kick has been moved to CBaseMonster to be callable by any other monster like HAssassins. And renamed to HumanKick.

//=========================================================
// GetGunPosition	return the end of the barrel
//=========================================================

//MODDD NOTE - Strangely the hgrunt doesn't seem to have an attachment just for the end of the gun.
//Maybe attachments 0 or 1 are ok? 2 says it's the head. Not sure.
Vector CHGrunt :: GetGunPosition( )
{
	if (m_fStanding )
	{
		return pev->origin + Vector( 0, 0, 60 );
	}
	else
	{
		return pev->origin + Vector( 0, 0, 48 );
	}
}
Vector CHGrunt::GetGunPositionAI(){

	return GetGunPosition();
}


//=========================================================
// Shoot
//=========================================================
void CHGrunt :: Shoot ( void )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemyMod( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);

	//MODDD NOTE - this pushed the point of brass ejection back by a constant 24 in the past. Letting this CVar handle it instead.
	EjectBrass ( vecShootOrigin + vecShootDir * EASY_CVAR_GET(hgruntBrassEjectForwardOffset), vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_MP5 ); // shoot +-5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!


	/*
	//MODDD - no, handle this any frame while firing instead.
	//MODDD - look more consistently when aiming.
	Vector vecShootDirMod = ShootAtEnemyMod( vecShootOrigin);

	Vector angDir = UTIL_VecToAngles( vecShootDirMod );
	SetBlending( 0, angDir.x );
	*/
}


//=========================================================
// Shoot
//=========================================================
void CHGrunt :: Shotgun ( void )
{

#if FORCE_MP5 == 1
	//do the default "Shoot" method instead (I assume it is more appropriate for firing with an mp5 instead)
	Shoot();

#else
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemyMod( vecShootOrigin );

	UTIL_MakeVectors ( pev->angles );

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40,90) + gpGlobals->v_up * RANDOM_FLOAT(75,200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass ( vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL);
	FireBullets(gSkillData.hgruntShotgunPellets, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0 ); // shoot +-7.5 degrees

	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!

	//Vector angDir = UTIL_VecToAngles( vecShootDir );
	//SetBlending( 0, angDir.x );

#endif

}


BOOL CHGrunt::outOfAmmoStrafeFireBlock(void){

	if(EASY_CVAR_GET(hgruntStrafeAlwaysHasAmmo) == 1){
		//can't block.
		return FALSE;
	}

	return (m_cAmmoLoaded <= 0);


}

//strafing of any kind?.
BOOL CHGrunt::hgruntAllowStrafe(void){

	//No CVar.
	//return TRUE;

	if(m_pSchedule == slhgruntStrafeToLocation || g_iSkillLevel >= 2){
		//any but easy.
		return TRUE;
	}else{
		return FALSE;
	}

}

BOOL CHGrunt::hgruntAllowStrafeFire(void){

	//CVAR:
	//EASY_CVAR_GET(hgruntAllowStrafeFire)

	/*
	if(EASY_CVAR_GET(hgruntAllowStrafeFire) != -1){
		EASY_CVAR_GET(hgruntAllowStrafeFire) == 1;
	}
	*/

	if( (m_pSchedule == slhgruntStrafeToLocation || g_iSkillLevel >= 3) && hgruntMoveAndShootDotProductPass == TRUE && HasConditions(bits_COND_SEE_ENEMY) ){
		//on hard, yes.
		return TRUE;
	}else{
		return FALSE;
	}
}


BOOL CHGrunt::canDoOpportunisticStrafe(void){


	if(m_pSchedule == slhgruntStrafeToLocation || g_iSkillLevel >= 2){
		//on all but easy, yes.
		return TRUE;
	}else{
		return FALSE;
	}

}

BOOL CHGrunt::getIsStrafeLocked(void){

	//CVAR:
	//EASY_CVAR_GET(hgruntLockStrafe)

	//that schedule locks the strafe.
	if(m_pSchedule == slhgruntStrafeToLocation || (EASY_CVAR_GET(hgruntLockStrafeTime) != 0 && (EASY_CVAR_GET(hgruntLockStrafeTime) <= 0 || ( strafeFailTime != -1 && gpGlobals->time <= strafeFailTime  ) )  ) ){
		return TRUE;
	}else{
		return FALSE;
	}

}


void CHGrunt :: MonsterThink ( void ){



	/*
	EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine( "yey %d", test()) ) ;
	*/

	//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("HGRUNT REPORT: %s : %d ROUTEINDEX: %d", getScheduleName(), getTaskNumber(), m_iRouteIndex);


	//Tried this, surprisingly unaffected by the player flashlight. Probably by any dynamic light sources (not constant from the map).
	//easyForcePrintLine("MY LIGHT LEVEL: %d", GETENTITYILLUM( ENT( pev ) ) );

	/*
	pev->iuser1 = 5;
	pev->iuser1 = 6;
	pev->iuser1 = 7;
	pev->iuser1 = 8;
	
	pev->fuser1 = 25.2f;
	pev->fuser2 = 26.2f;
	pev->fuser3 = 27.2f;
	pev->fuser4 = 28.2f;

	*/


		if(EASY_CVAR_GET(thatWasntPunch) == 1 && this->m_fSequenceFinished){

			switch(RANDOM_LONG(0, 32)){
			case 0:this->SetSequenceByName("converse1");break;
			case 1:this->SetSequenceByName("converse2");break;
			case 2:this->SetSequenceByName("trackwave");break;
			case 3:this->SetSequenceByName("trackwave");break;
			case 4:this->SetSequenceByName("trackwave");break;
			case 5:this->SetSequenceByName("WM_button");break;
			case 6:this->SetSequenceByName("dragleft");break;
			case 7:this->SetSequenceByName("dragright");break;
			case 8:this->SetSequenceByName("repel_land");break;
			case 9:this->SetSequenceByName("repel_land");break;
			case 10:this->SetSequenceByName("repel_jump");break;
			case 11:this->SetSequenceByName("repel_die");break;
			case 12:this->SetSequenceByName("barnacled1");break;
			case 13:this->SetSequenceByName("barnacled2");break;
			case 14:this->SetSequenceByName("barnacled3");break;
			case 15:this->SetSequenceByName("barnacled4");break;
			case 16:this->SetSequenceByName("advance_signal");break;
			case 17:this->SetSequenceByName("flank_signal");break;
			case 18:this->SetSequenceByName("retreat_signal");break;
			case 19:this->SetSequenceByName("drop_grenade");break;
			case 20:this->SetSequenceByName("frontkick");break;
			case 21:this->SetSequenceByName("frontkick");break;
			case 22:this->SetSequenceByName("idle3");break;
			case 23:this->SetSequenceByName("idle1");break;
			case 24:this->SetSequenceByName("victorydance");break;
			case 25:this->SetSequenceByName("smflinch");break;
			case 26:this->SetSequenceByName("leftlegsmflinch");break;
			case 27:this->SetSequenceByName("rightlegsmflinch");break;
			case 28:this->SetSequenceByName("leftarmflinch");break;
			case 29:this->SetSequenceByName("rightarmflinch");break;
			case 30:this->SetSequenceByName("get_smashed");break;
			case 31:this->SetSequenceByName("plunger");break;
			case 32:this->SetSequenceByName("pipetoss");break;
			}//END OF switch
		}

		if(m_fSequenceFinished){
			/*
			if(m_idealActivity == m_movementActivity && m_idealActivity == ACT_RUN && runAndGun){
				if(pev->sequence != runAndGunSequenceID){

				}

			}
			*/
		}
	//MODDD - see if we can use the strafing anim.

	BOOL noStrafeForYou = FALSE;


	//conditions that make it impossible to strafe.
	if(hgruntAllowStrafe() == FALSE || m_movementGoal == MOVEGOAL_ENEMY || m_movementGoal == MOVEGOAL_TARGETENT){
		//can't strafe at all?  Chasing an enemy (as opposed to getting to a location alone)? Disregard everything here.


		//...we may not necessarily want to run. Just force the movement activity if we happen to be strafing.
		
		if(m_movementGoal == MOVE_STRAFE){
			m_movementActivity = ACT_RUN;
		}
		//


		idealStrafeMode = -1;
		strafeMode = -1;

		noStrafeForYou = TRUE;
	}

	/*
	//No, this is awkward.
	if(this->m_pSchedule == slhgruntStrafeToLocation){
		//
		//not the usual strafe script, this does not cancel the strafe in action.
		//Just let it proceed.
		noStrafeForYou = TRUE;

	}
	*/




	//!NOTE!!!  enemy null check right here...
	if(pev->deadflag == DEAD_NO && m_hEnemy != NULL && (this->m_IdealActivity == m_movementActivity) && this->m_movementActivity == ACT_RUN || this->m_movementActivity == ACT_STRAFE_LEFT || this->m_movementActivity == ACT_STRAFE_RIGHT ){


		Vector vecDirToPointNorm = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Normalize();
		Vector vecTowardEnemyNorm = ( m_hEnemy->pev->origin - pev->origin).Normalize();

		float distanceToEnemy2D = (pev->origin - m_hEnemy->pev->origin).Length2D();
		float dotProductStraight = DotProduct ( vecDirToPointNorm, vecTowardEnemyNorm);

		BOOL isFacingEnemy = UTIL_IsFacing(this->pev, m_hEnemy->pev->origin, 1-EASY_CVAR_GET(hgruntRunAndGunDotMin));
		
		//easyForcePrintLine("COND CHECK::: %d && (%.2f > %.2f),,, isFacing? %d", HasConditions(bits_COND_SEE_ENEMY), dotProductStraight, EASY_CVAR_GET(hgruntRunAndGunDotMin), isFacingEnemy);

		//if(UTIL_IsFacing(this->pev, m_hEnemy->pev->origin, 0.1  ) ){
		if(this->m_pSchedule != slhgruntStrafeToLocation &&
			(EASY_CVAR_GET(hgruntRunAndGunDistance) != -1) && 
			m_movementActivity == ACT_RUN &&
			distanceToEnemy2D > EASY_CVAR_GET(hgruntRunAndGunDistance) &&
			HasConditions(bits_COND_SEE_ENEMY) &&
			dotProductStraight > EASY_CVAR_GET(hgruntRunAndGunDotMin)

			//???

			//NoFriendlyFire()
		){

			hgruntMoveAndShootDotProductPass = TRUE;

			//If I am directly facing the enemy en route, we're going to RUN. AND. GUN.
			m_movementActivity = ACT_RUN;
			idealStrafeMode = -1;
			strafeMode = -1;
			runAndGun = TRUE;

			runAndGunFailTime = gpGlobals->time + EASY_CVAR_GET(hgruntLockRunAndGunTime);


			//running and gunning gets priority... for now.
			noStrafeForYou = TRUE;
		}else{

			hgruntMoveAndShootDotProductPass = FALSE;

			if(runAndGun && gpGlobals->time > runAndGunFailTime){
				//fail!
				runAndGun = FALSE;
			}

			/*
			if(EASY_CVAR_GET(testVar) == 1){
				easyForcePrintLine("***hgrunt-%d: NO RUN&GUN:  actIsRun:%d, cvar_run&gundist:%.2f, cvar_run&gundot: %.2f, distance2d:%.2f, dotprod:%.2f",
				monsterID,
				m_movementActivity == ACT_RUN,
				EASY_CVAR_GET(hgruntRunAndGunDistance),
				EASY_CVAR_GET(hgruntRunAndGunDotMin),
				distanceToEnemy2D,
				dotProductStraight
				);
			}
			*/
		}
		


		
		/*
		if(runAndGun){
			if(dotProductStraight > EASY_CVAR_GET(hgruntRunAndGunDotMin) && HasConditions(bits_COND_SEE_ENEMY) ){
				//just reusing this var.
				hgruntMoveAndShootDotProductPass = TRUE;
			}else{
				hgruntMoveAndShootDotProductPass = FALSE;
			}
		}
		*/

		if(!noStrafeForYou){

			//check for opportunity to strafe...

			/*
			Vector vecForward;
			Vector vecRight;
			UTIL_MakeVectorsPrivate ( pev->angles, vecForward, vecRight, NULL );
			*/


			//My "right" isn't necessarily what we want.
			//Vector2D vecRight2D = vecRight.Make2D().Normalize();



			//Vector vecDirToPoint = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin );
			////////////////Vector vecDirToPointNorm = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Normalize();
			//Vector2D vec2DirToPoint = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Make2D().Normalize();
			//Vector vecDirToPoint2DPseudo = Vector( vecDirToPoint.x, vecDirToPoint.y, 0 ).Normalize();


			//right of dir to point!
			//Vector2D vec2DirToPointRight = CrossProduct( ( m_Route[ 0 ].vecLocation - pev->origin ), Vector(0, 0, 1) ).Make2D() ;
			Vector vec2DirToPointRight = CrossProduct( vecDirToPointNorm, Vector(0, 0, 1) ).Normalize();

			//Vector vecTowardEnemy = ( m_hEnemy->pev->origin - pev->origin);
			////////////////Vector vecTowardEnemyNorm = ( m_hEnemy->pev->origin - pev->origin).Normalize();
			//Vector2D vecTowardEnemy2D = ( m_hEnemy->pev->origin - pev->origin).Make2D().Normalize();
			//Vector vecTowardEnemy2DPseudo = Vector(vecTowardEnemy.x, vecTowardEnemy.y, 0).Normalize();

			//UTIL_drawLineFrame(pev->origin + Vector(0,0,6), pev->origin + vec2DirToPointRight * 27 + Vector(0,0,6), 9, 255, 0, 0);
			//UTIL_drawLineFrame(pev->origin + Vector(0,0,6), pev->origin + vecTowardEnemyNorm * 27 + Vector(0,0,6), 9, 0, 255, 0);
			//UTIL_drawLineFrame(pev->origin + Vector(0,0,6), m_vecMoveGoal + Vector(0,0,6), 9, 255, 255, 0);






			//UTIL_drawLineFrame(pev->origin, m_Route[ 0 ].vecLocation, 9, 0, 0, 255);



			//QUESTION: is my "right" or "left" (opposite of right) similar to the vector towards the "enemy"?  If so, this is a good position to strafe.


			//my "right" is too arbitrary.
			//float dotProduct = DotProduct ( vecTowardEnemy, vecRight2D ) ;

			//IF right of where I'm going...
			float dotProduct = DotProduct ( vec2DirToPointRight, vecTowardEnemyNorm ) ;

			BOOL mayStrafeFire = FALSE;


			strafeCanFire = TRUE;

			float dotProductCutoff = 0.9;


			/*
			if(EASY_CVAR_GET(testVar) == 24){
				//(EASY_CVAR_GET(hgruntRunAndGunDistance) != -1) &&  m_movementActivity == ACT_RUN && distanceToEnemy2D > EASY_CVAR_GET(hgruntRunAndGunDistance) && dotProductStraight > EASY_CVAR_GET(hgruntRunAndGunDotMin)

				EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("***HGRUNT PRINTOUT24::: movementact:%d, canDoOppStrafe:%d, cvar_ragdis:%.2f, cvar_ragdot:%.2f, distance2D:%.2f, dotproductstr:%.2f, dotproductnorm:%.2f ",
					canDoOpportunisticStrafe(),
					m_movementActivity,
					EASY_CVAR_GET(hgruntRunAndGunDistance),
					EASY_CVAR_GET(hgruntRunAndGunDotMin),
					distanceToEnemy2D,
					dotProductStraight,
					dotProduct));
			}
			*/


			//easyForcePrintLine("ARE YOU A fine fellow %d %d %.2f %.2f %.2f", this->m_pSchedule != slhgruntStrafeToLocation, !canDoOpportunisticStrafe(), distanceToEnemy2D,dotProduct,dotProductCutoff);
			
			
			//if(this->m_pSchedule != slhgruntStrafeToLocation){
			
				//if we're too close, or dotproduct isn't close enough to 1 or -1, don't try a strafe.
				//Also, don't allow strafing if we're unable to do so opportunistically (can do if forced by schedule though).
				if( (this->m_pSchedule != slhgruntStrafeToLocation && !canDoOpportunisticStrafe()) || distanceToEnemy2D < 80 || (dotProduct <= dotProductCutoff && dotProduct >= -dotProductCutoff) ){
				
					strafeCanFire = FALSE;
					//easyForcePrintLine("CAN I please STOP %.2f %.2f", gpGlobals->time, strafeFailTime);

					if(getIsStrafeLocked() != TRUE ){
						m_movementActivity = ACT_RUN;
						idealStrafeMode = -1;
						strafeMode = -1;

					}
					if(idealStrafeMode == 1){
						idealStrafeMode = 0;
					}else if(idealStrafeMode == 3){
						idealStrafeMode = 2;
					}

				}else if ( dotProduct > 0.9 )
				{

					strafeFailTime = gpGlobals->time + EASY_CVAR_GET(hgruntLockStrafeTime);


					mayStrafeFire = TRUE;
					// strafe right
					//m_IdealActivity = ACT_STRAFE_RIGHT;
					m_movementActivity = ACT_STRAFE_RIGHT;

					//easyForcePrintLine("????? %d %d %d", outOfAmmoStrafeFireBlock(), friendlyFireStrafeBlock, hgruntAllowStrafeFire());
					if( outOfAmmoStrafeFireBlock() || friendlyFireStrafeBlock || hgruntAllowStrafeFire() == FALSE){
						//no ammo, no fire.  OR in violation of friendly-fire.
						idealStrafeMode = 2;
					}else{
						idealStrafeMode = 3;
					}

					//idealStrafeMode = 2 + 1;
					//strafeMode = 2 + 1;

				}else if( dotProduct < -0.9)
				{

					mayStrafeFire = TRUE;
				
					strafeFailTime = gpGlobals->time + EASY_CVAR_GET(hgruntLockStrafeTime);

					// strafe left
					//m_IdealActivity = ACT_STRAFE_LEFT;
					m_movementActivity = ACT_STRAFE_LEFT;

					if(outOfAmmoStrafeFireBlock() || friendlyFireStrafeBlock || hgruntAllowStrafeFire() == FALSE  ){
						//no ammo, no fire.  OR in violation of friendly-fire.
						idealStrafeMode = 0;
					}else{
						idealStrafeMode = 1;
					}


					//idealStrafeMode = 0 + 1;
					//strafeMode = 0 + 1;
				}else{




					//WHUT
					EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("HGRUNT: potatoes.  ???" ));
				}
				//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("IDEAL STR: %d : %d %d", strafeCanFire, strafeMode, idealStrafeMode) );

				//strafin'?  cool.


			//}//END OF the (this->m_pSchedule == slhgruntStrafeToLocation) check

			
			if(m_Activity == ACT_STRAFE_RIGHT){
				if(dotProduct > 0.92){
					hgruntMoveAndShootDotProductPass = TRUE;
				}else{
					hgruntMoveAndShootDotProductPass = FALSE;
				}
			}
			if(m_Activity == ACT_STRAFE_LEFT){
				if(dotProduct < -0.92){
					hgruntMoveAndShootDotProductPass = TRUE;
				}else{
					hgruntMoveAndShootDotProductPass = FALSE;
				}
			}

		}//END OF (!noStrafeForYou)
		else{

			/*
			if(EASY_CVAR_GET(testVar) == 24){

				EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("***!hgrunt NOSTRAFEFORYOU??? schedCheck:%d, skillLevelPass:%d, movegoalpass: %d", m_pSchedule != slhgruntStrafeToLocation, g_iSkillLevel >= 2, m_movementGoal == MOVEGOAL_ENEMY || m_movementGoal == MOVEGOAL_TARGETENT));
			}
			*/

		}
	}//END OF any edit check
	else{

		/*
		if(runAndGun == TRUE){
			runAndGun = FALSE;
		}
		if(idealStrafeMode != -1 ){
			m_movementActivity = ACT_RUN;
			//SetActivity(ACT_RUN);
			idealStrafeMode = -1;
			strafeMode = -1;
		}
		*/

		/*
		if(EASY_CVAR_GET(testVar) == 1){
			easyForcePrintLine("***!hgrunt-%d: INVALID PRE??? hasEnemy:%d, actPass:%d, idealAct:%d, moveAct:%d",
				monsterID,
				m_hEnemy!=NULL,
				( (this->m_IdealActivity == m_movementActivity) && this->m_movementActivity == ACT_RUN || this->m_movementActivity == ACT_STRAFE_LEFT || this->m_movementActivity == ACT_STRAFE_RIGHT ),
				m_IdealActivity,
				m_movementActivity
				);

			//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("***!hgrunt INVALID MOVE??? allowStr:%d, movgoalCheck:%d, schedCheck:%d, m_hEnemyNotnull:%d, idealact:%d, movementact:%d, actpass?:%d", hgruntAllowStrafe(), (m_movementGoal == MOVEGOAL_ENEMY || m_movementGoal == MOVEGOAL_TARGETENT), this->m_pSchedule == slhgruntStrafeToLocation, !noStrafeForYou, (m_hEnemy!=NULL), this->m_IdealActivity, m_movementActivity, (this->m_IdealActivity == m_movementActivity) && this->m_movementActivity == ACT_RUN || this->m_movementActivity == ACT_STRAFE_LEFT || this->m_movementActivity == ACT_STRAFE_RIGHT ));
		}
		*/
	}
	CBaseMonster::MonsterThink();
}



void CHGrunt::HandleEventQueueEvent(int arg_eventID){
	
	/*
	MonsterEvent_t eee;

	switch(arg_eventID){
	case 0:
		eee.event = 4;
	break;
	case 1:
		eee.event = 5;
	break;

	}
	
	HandleAnimEvent(&eee);
	*/

	//easyForcePrintLine("DO I EVER GET CALLED?????");
	return;

}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CHGrunt :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	if(EASY_CVAR_GET(thatWasntPunch) == 1){
		return;
	}
	//pev->renderfx |= 128;
	//return;

	//easyForcePrintLine("ATTT THE %d", pEvent->event);
	
	//can flash if needed.
	pev->renderfx &= ~NOMUZZLEFLASH;


	if(pEvent->event >= 4 && pEvent->event <= 6){
		//(pEvent->event >= 4 && pEvent->event <= 6)

		//burst-fire events can be blocked.
		//if(m_IdealActivity == m_movementActivity && (m_movementActivity == ACT_STRAFE_LEFT || m_movementActivity == ACT_STRAFE_RIGHT)  ){
		

		//easyForcePrintLine("m_Activity:%d runAndGun:%d", m_Activity, runAndGun);
		///easyForcePrintLine(" m_IdealActivity:%d m_movementActivity:%d", m_IdealActivity, m_movementActivity);
		if(m_Activity == ACT_STRAFE_LEFT || m_Activity == ACT_STRAFE_RIGHT  ){

			/*
			if(idealStrafeMode != strafeMode){
				pev->renderfx |= 128;
				return;
			}
			*/
			
			if(!hgruntMoveAndShootDotProductPass){
				pev->renderfx |= NOMUZZLEFLASH;
				return;
			}
			if(m_Activity != m_IdealActivity){
				//trying to change?  can't fire.
				pev->renderfx |= NOMUZZLEFLASH;
				return;
			}


			//easyForcePrintLine("ASSSSSSSSSS %d %d %d %d", hgruntAllowStrafeFire() == FALSE, !strafeCanFire, outOfAmmoStrafeFireBlock(), friendlyFireStrafeBlock);
			if(hgruntAllowStrafeFire() == FALSE || !strafeCanFire || outOfAmmoStrafeFireBlock() || friendlyFireStrafeBlock){
				//no firing if out of ammo or obligated not to.

				//code that means, do not do flash animations.  They're likely following soon.
				pev->renderfx |= NOMUZZLEFLASH;
				return;
			}
		}else if(m_Activity == ACT_RUN && runAndGun){

			//no ammo check?   TODO  perhaps?
			//easyForcePrintLine("WHAZ GOIN ON YO %d", hgruntMoveAndShootDotProductPass);
			if(!hgruntMoveAndShootDotProductPass){
				pev->renderfx |= NOMUZZLEFLASH;
				return;
			}
		}
	}




	//pev->renderfx |= NOMUZZLEFLASH;

	Vector	vecShootDir;
	Vector	vecShootOrigin;


	//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("HGRUNT: HANDLEANIMEVENT: %d", pEvent->event) );

	switch( pEvent->event )
	{
		case HGRUNT_AE_DROP_GUN:
			{
			// if reversed, we're being revived.
			if (pev->framerate > 0) {
				Vector	vecGunPos;
				Vector	vecGunAngles;

				GetAttachment(0, vecGunPos, vecGunAngles);

				// switch to body group with no gun.
				SetBodygroup(GUN_GROUP, GUN_NONE);
	#if FORCE_MP5 == 1
				//From the else statement below (meaning, this grunt does not have a shot gun), it appears a grunt with no weapon
				//will also drop mp5 ammo.  So, since it is not possible to have a shotgun (FORCE_MP5), always drop mp5 ammo.

				DropItem( "weapon_9mmAR", vecGunPos, vecGunAngles );

				if(EASY_CVAR_GET(gruntsCanHaveMP5Grenade) == 1){
					if (FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER ))
					{
						DropItem( "ammo_ARgrenades", BodyTarget( pev->origin ), vecGunAngles );
					}
				}

	#else

				// now spawn a gun.
				if (FBitSet( pev->weapons, HGRUNT_SHOTGUN ))
				{
					 DropItem( "weapon_shotgun", vecGunPos, vecGunAngles );
				}
				else
				{
					 DropItem( "weapon_9mmAR", vecGunPos, vecGunAngles );
				}
				if (FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER ))
				{
					DropItem( "ammo_ARgrenades", BodyTarget( pev->origin ), vecGunAngles );
				}
	#endif
			}//END OF pev->framerate check

		}
		break;

		case HGRUNT_AE_RELOAD:
			EMIT_SOUND_FILTERED( ENT(pev), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM );
			m_cAmmoLoaded = getClipSize();
			ClearConditions(bits_COND_NO_AMMO_LOADED);
		break;

		case HGRUNT_AE_GREN_TOSS:
		{
			UTIL_MakeVectors( pev->angles );
			// CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 34 + Vector (0, 0, 32), m_vecTossVelocity, 3.5 );
			CGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 3.5 );

			m_fThrowGrenade = FALSE;
			m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.
			// !!!LATER - when in a group, only try to throw grenade if ordered.
		}
		break;

		case HGRUNT_AE_GREN_LAUNCH:
		{

#if FORCE_MP5 == 1

			//no mp5 grenades, unless we specifically allow them.
			if(EASY_CVAR_GET(gruntsCanHaveMP5Grenade) == 1){
				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
				CGrenade::ShootContact( pev, GetGunPosition(), m_vecTossVelocity );
				m_fThrowGrenade = FALSE;
				if (g_iSkillLevel == SKILL_HARD)
					m_flNextGrenadeCheck = gpGlobals->time + RANDOM_FLOAT( 2, 5 );// wait a random amount of time before shooting again
				else
					m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.

			}


#else
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM);
			CGrenade::ShootContact( pev, GetGunPosition(), m_vecTossVelocity );
			m_fThrowGrenade = FALSE;
			if (g_iSkillLevel == SKILL_HARD)
				m_flNextGrenadeCheck = gpGlobals->time + RANDOM_FLOAT( 2, 5 );// wait a random amount of time before shooting again
			else
				m_flNextGrenadeCheck = gpGlobals->time + 6;// wait six seconds before even looking again to see if a grenade can be thrown.

#endif

		}
		break;

		case HGRUNT_AE_GREN_DROP:
		{
			UTIL_MakeVectors( pev->angles );
			CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 + gpGlobals->v_up * 6, g_vecZero, 3 );

		}
		break;

		case HGRUNT_AE_BURST1:
		{

			//easyForcePrintLine("HGRUNT%d WHAT IS THIS TOMfoolery A. sched:%s task:%d seq:%d frame:%.2f", monsterID, this->getScheduleName(), this->getTaskNumber(), pev->sequence, pev->frame);


#if FORCE_MP5 == 1

			//Assume mp5.
			Shoot();


			// the first round of the three round burst plays the sound and puts a sound in the world sound list.
			if ( RANDOM_LONG(0,1) )
			{
				EMIT_SOUND_FILTERED( ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun1.wav", 1, ATTN_NORM );
			}
			else
			{
				EMIT_SOUND_FILTERED( ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun2.wav", 1, ATTN_NORM );
			}
#else
			if ( FBitSet( pev->weapons, HGRUNT_9MMAR ))
			{
				Shoot();

				// the first round of the three round burst plays the sound and puts a sound in the world sound list.
				if ( RANDOM_LONG(0,1) )
				{
					EMIT_SOUND_FILTERED( ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun1.wav", 1, ATTN_NORM );
				}
				else
				{
					EMIT_SOUND_FILTERED( ENT(pev), CHAN_WEAPON, "hgrunt/gr_mgun2.wav", 1, ATTN_NORM );
				}
			}
			else
			{
				Shotgun( );

				EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM );
			}

#endif


			CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		}
		break;

		case HGRUNT_AE_BURST2:
		case HGRUNT_AE_BURST3:
			//easyForcePrintLine("HGRUNT%d WHAT IS THIS TOMfoolery B. sched:%s task:%d seq:%d frame:%.2f", monsterID, this->getScheduleName(), this->getTaskNumber(), pev->sequence, pev->frame);
			Shoot();
			break;

		case HGRUNT_AE_KICK:
		{
			CBaseEntity *pHurt = HumanKick();

			if ( pHurt )
			{

				// SOUND HERE!
				UTIL_MakeVectors( pev->angles );
				if(!pHurt->blocksImpact()){
					pHurt->pev->punchangle.x = 15;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
				}
				pHurt->TakeDamage( pev, pev, gSkillData.hgruntDmgKick, DMG_CLUB );

				//derp.
				//EMIT_SOUND_FILTERED( ENT(pev), CHAN_WEAPON, "hgrunt/gr_pain3.wav", 1, ATTN_NORM );
				EMIT_SOUND_FILTERED ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			}else{
				//play woosh sound?
				playStandardMeleeAttackMissSound();
			}
		}
		break;

		case HGRUNT_AE_CAUGHT_ENEMY:
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz(ENT(pev), "HG_ALERT", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
				 JustSpoke();
			}

		}

		default:
			CSquadMonster::HandleAnimEvent( pEvent );
		break;
	}
}

CHGrunt::CHGrunt(void){

	hgruntMoveAndShootDotProductPass = FALSE;

	strafeFailTime = -1;
	runAndGunFailTime = -1;

	runAndGunSequenceID = -1;

	strafeMode = -1;
	idealStrafeMode = -1;
	friendlyFireStrafeBlock = FALSE;
	missingHeadPossible = FALSE;

	//set before calling the strafe schedule.
	tempStrafeAct = 0;

	lastStrafeCoverCheck = -1;
	runAndGun = FALSE;

}




//MODDD - new.  Copy of SquadMonster's StartMonster to have a huge custom section that had to be nested in there.
// Keep that in mind for future changes to squadmonster.cpp's startMonster, or just make it a proper vent, like
//   onPostStartSquadCheck
// ...it's just awkward to because this is the only class that would use that event.
void CHGrunt::StartMonster( void )
{
	// And yes, skip CSquadMonster's StartMonster.  The whole point of this is to replace that completely,
	// don't invoke it.
	CBaseMonster :: StartMonster();

	if ( ( m_afCapability & bits_CAP_SQUAD ) && !InSquad() )
	{
		if ( !FStringNull( pev->netname ) )
		{
			BOOL possibleExceptionToRule = FALSE;
			if(EASY_CVAR_GET(leaderlessSquadAllowed) == 1 && !this->alreadyDoneNetnameLeaderCheck){
				alreadyDoneNetnameLeaderCheck = TRUE;
				//or will have soon...
				//Check all of netname.
				possibleExceptionToRule = checkLeaderlessSquadByNetname();
			}
			//MODDD - do a check.   See if any member has the "SF_SQUADMONSTER_LEADER" flag set.
			//EASY_CVAR_GET(leaderlessSquadAllowed) == 0
			// if I have a groupname, I can only recruit if I'm flagged as leader
			//NOTE::: joining is now possible, but the old behavior is probably okay.
			if ( !possibleExceptionToRule && !( pev->spawnflags & SF_SQUADMONSTER_LEADER ) )
			{
				return;
			}
		}

		if(skipSquadStartup){
			//it appears this is completely unnecessaray, but oh well.  (being assigned to a squad right at creation already disables this whole startup block)
			EASY_CVAR_PRINTIF_PRE(squadmonsterPrintout, easyForcePrintLine("STARTUP SKIPPED!"));
			return;
		}

		// try to form squads now.
		int iSquadSize = SquadRecruit( 1024, 4 );

		if ( iSquadSize )
		{
		  ALERT ( at_aiconsole, "Squad of %d %s formed\n", iSquadSize, STRING( pev->classname ) );
		}

		//easyForcePrintLine("ARE YOU bros SERIOUS RIGHT NOW %.2f %d %d", EASY_CVAR_GET(altSquadRulesRuntime), FStringNull(pev->netname), iSquadSize);
		//If "altSquadRule" is something (1 or 2), and is either 2 OR, if 1, also spawned in real-time (not coming from the map), try joining a squad.
		if( EASY_CVAR_GET(altSquadRulesRuntime) > 0 && (EASY_CVAR_GET(altSquadRulesRuntime) == 2 || FStringNull( pev->netname ) ) ){
			if(iSquadSize == 1){
				
				if(EASY_CVAR_GET(monsterSpawnPrintout) == 1){
				easyForcePrintLine("ITS MONDAY my acquaintance!  Get myself a squad... %s:%d", this->getClassname(), this->monsterID);
				}
				//found no other unassigned members to start a squad with.  Try joining instead?
				SquadJoin(1024, 4);
			}
		}

		//if ( IsLeader() && FClassnameIs ( pev, "monster_human_grunt" ) ){

		//	//MODDD - removed, as grunts no longer have other possible heads (just the gasmask)
		//	//SetBodygroup( 1, 1 ); // UNDONE: truly ugly hack
		//	
		//	pev->skin = 0;
		//	//unsure if the skin thing is necessary anymore.
		//	
		//}
		
		
		
		//////////////////////////////////////////////////////////////////////////////////////////////
		//START OF HGRUNTS CUSTOM SCRIPT
		//////////////////////////////////////////////////////////////////////////////////////////////
		CSquadMonster* testLeader = MySquadLeader();

		if(EASY_CVAR_GET(monsterSpawnPrintout) == 1){
			EASY_CVAR_PRINTIF_PRE(squadmonsterPrintout, easyForcePrintLine("STARTMONSTER for SQUADMONSTER CALLED: info? %s %d %d", STRING(pev->classname), this->SquadCount(), testLeader != NULL));
		}
		
		if (testLeader != NULL )
		{
			BOOL forbidLeaderChange = FALSE;
			CSquadMonster* eligibleChange = NULL;
			CSquadMonster* selectedLeader = NULL;
			//First, a check.  Is any squad member already a "hassault"?  If so, make that one the leader.
			for (int i = 0; i < MAX_SQUAD_MEMBERS-1; i++){
				if (testLeader->m_hSquadMember[i] != NULL){
					CSquadMonster *pMember = testLeader->MySquadMember(i);
					if (pMember){
						EASY_CVAR_PRINTIF_PRE(squadmonsterPrintout, easyForcePrintLine("CHECKING %d; %s", i, STRING(pMember->pev->classname)) );
						if(pMember->disableLeaderChange == TRUE){
							//For now, if any member has this variable set, do NOT allow any chance of the change.
							//EASY_CVAR_PRINTIF_PRE(squadmonsterPrintout, easyForcePrintLine("Leader checking blocked!"));
							//forbidLeaderChange = TRUE;
							//...nah, just mean "THIS" hgrunt can't be the leader.
							continue;
						}
						if(eligibleChange == NULL && pMember->disableLeaderChange == FALSE){
							//This can become an "hassault" in case the current choice can't (and needs to be changed).
							eligibleChange = pMember;
						}
						if(selectedLeader == NULL && FClassnameIs(pMember->pev, "monster_human_assault") ){
							//Make this the leader!
							selectedLeader = pMember;
							//break;
						};
					}
				}
			}//END OF for(...)

			if(!forbidLeaderChange){
				EASY_CVAR_PRINTIF_PRE(squadmonsterPrintout, easyForcePrintLine("SELECTED LEADER: %d", selectedLeader == NULL));
				if(selectedLeader != NULL){
					//make this "hassault" that was found the leader.
					ChangeLeader(testLeader, selectedLeader);
					testLeader = selectedLeader;
				}//END OF if(selectedLeader != NULL)
				else if(this->SquadCount() >= 3){
					//no "hassaults" and the squad is of at least 3 members?  Make the current leading grunt an "hassault" (create one in his place) and transfer squad-data over.

					//We MAY way to turn this "hgrunt" into a "hassault" though?  (leaders of hgrunts become hassault now)
					
					//remember me...   nevermind.
					//CSquadMonster* startingLeader = testLeader;
					//CSquadMonster* toDelete = testLeader;

					//First, a check.  Starting with the current leader, try to find one that doesn't have the block on:
					if(testLeader->disableLeaderChange){

						if(eligibleChange){
							
							ChangeLeader(testLeader, eligibleChange);
							testLeader = eligibleChange;

						}else{
							//nothing can be done, no leader change.
							return;
						}
					}
					CBaseEntity *pReplacement = CBaseEntity::CreateManual("monster_human_assault", testLeader->pev->origin, testLeader->pev->angles, NULL);
					
					CSquadMonster* tempReplacementSqdPntr = NULL;
					if( (tempReplacementSqdPntr = pReplacement->MySquadMonsterPointer()) == NULL){
						//???
						return;
					}
					// What??   Why here when we're doing DispatchSpawn later?
					//DispatchSpawn( pReplacement->edict() );

					tempReplacementSqdPntr->m_MonsterState = testLeader->m_MonsterState;
					tempReplacementSqdPntr->m_IdealMonsterState = testLeader->m_IdealMonsterState;
					tempReplacementSqdPntr->m_Activity = testLeader->m_Activity;
					tempReplacementSqdPntr->m_IdealActivity = testLeader->m_IdealActivity;
					tempReplacementSqdPntr->m_movementActivity = testLeader->m_movementActivity;
					//tempReplacementSqdPntr->m_pSchedule = testLeader->m_pSchedule;
					//tempReplacementSqdPntr->m_iScheduleIndex = testLeader->m_iScheduleIndex;

					//VITAL STEPS... i think?
					pReplacement->pev->netname = testLeader->pev->netname;
					tempReplacementSqdPntr->skipSquadStartup = TRUE;

					//If the hgrunt being replaced had some vital role in the map, he needs to maintain that.
					tempReplacementSqdPntr->pev->target = testLeader->pev->target;
					tempReplacementSqdPntr->pev->targetname = testLeader->pev->targetname;

					DispatchSpawn( pReplacement->edict() );

					//easyForcePrintLine("HIS SCHED: %s schedind: %d", testLeader->getScheduleName(), testLeader->m_iScheduleIndex);
					//tempReplacementSqdPntr->ChangeSchedule(testLeader->m_pSchedule);
					//tempReplacementSqdPntr->m_iScheduleIndex = testLeader->m_iScheduleIndex;
					
						//293
					//easyForcePrintLine("Squad replacement. Who am I? %s:%d  Being replaced by: %s:%d", testLeader->getClassname(), testLeader->monsterID, tempReplacementSqdPntr->getClassname(), tempReplacementSqdPntr->monsterID); 
					
					CSquadMonster* pReplacementSquadRef = (CSquadMonster*) pReplacement;
					
					pReplacementSquadRef->skipSquadStartup = TRUE;
					
					//should remove all ties b/w me and all squaddies.
					//this->SquadRemove(this);
					
					//The replacement is the leader now.
					pReplacementSquadRef->m_hSquadLeader = pReplacementSquadRef;
					
					for (int i = 0; i < MAX_SQUAD_MEMBERS-1; i++)
					{
						if (testLeader->m_hSquadMember[i] != NULL)
						{
							//healthy default for this squad slot of the replacement.
							pReplacementSquadRef->m_hSquadMember[i] = NULL;
							
							CSquadMonster *pMember = testLeader->MySquadMember(i);
							if (pMember)
							{
								EASY_CVAR_PRINTIF_PRE(squadmonsterPrintout, easyForcePrintLine("EDITED THAT MEMBER: %d", i));
								pMember->m_hSquadLeader = pReplacementSquadRef;
								pReplacementSquadRef->m_hSquadMember[i] = pMember;
							}else{
								EASY_CVAR_PRINTIF_PRE(squadmonsterPrintout, easyForcePrintLine("NO SQUAD MEMBER???? %d", i));
							}
							testLeader->m_hSquadMember[i] = NULL;

							//m_hSquadMember[i] = pAdd;
							//pAdd->m_hSquadLeader = this;
							//return TRUE;
						}
					}

					testLeader->m_hSquadLeader = NULL;

					EASY_CVAR_PRINTIF_PRE(squadmonsterPrintout, easyForcePrintLine("CONVERTED::::"));
					for (int i2 = 0; i2 < MAX_SQUAD_MEMBERS-1; i2++)
					{	CSquadMonster *pMember = testLeader->MySquadMember(i2);
						if (pMember){
							EASY_CVAR_PRINTIF_PRE(squadmonsterPrintout, easyForcePrintLine("wat i: %d insq: %d count: %d leadernotnull: %d ", i2, pMember->MySquadMonsterPointer()->InSquad(), pMember->MySquadMonsterPointer()->SquadCount(), (pMember->MySquadMonsterPointer()->MySquadLeader() != NULL)));

						}
					}
					//return FALSE;
					UTIL_Remove(testLeader);	
				}//END OF else OF if(selectedLeader != NULL)
			}//END OF if(!forbidLeaderChange)
		}//END OF testLeader check
		//////////////////////////////////////////////////////////////////////////////////////////////
		//END OF HGRUNTS CUSTOM SCRIPT
		//////////////////////////////////////////////////////////////////////////////////////////////
		
		
	}//END OF has bits_CAP_SQUAD and not already in a squad check
}//END OF StartMonster



//=========================================================
// Spawn
//=========================================================
void CHGrunt :: Spawn()
{
	Precache( );

	//MODDD - if this spawnflag is in place...
	if(pev->spawnflags & SF_HGRUNT_DISALLOW_PROMOTION){
		//can't change into an HAssault.
		this->disableLeaderChange = TRUE;
	}

	pev->classname = MAKE_STRING("monster_human_grunt");

	setModel(); //"models/hgrunt.mdl"  argument unused when there's a german model equivalent.
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor = BloodColorRedFilter();
	pev->effects		= 0;
	pev->health			= gSkillData.hgruntHealth;
	m_flFieldOfView		= 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime	= gpGlobals->time;
	m_iSentence			= HGRUNT_SENT_NONE;

	m_afCapability		= bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	

	m_fEnemyEluded		= FALSE;
	m_fFirstEncounter	= TRUE;// this is true when the grunt spawns, because he hasn't encountered an enemy yet.

	//MODDD NOTE - this has no point. GetGunPosition was the only method that used this and it was already overridden as-is to use different coords.
	m_HackedGunPos = Vector ( 0, 0, 55 );

#if FORCE_MP5 == 1
	//pev->weapons = HGRUNT_9MMAR | HGRUNT_GRENADELAUNCHER;
	//pev->weapons = HGRUNT_9MMAR | HGRUNT_HANDGRENADE;

	if(EASY_CVAR_GET(gruntsCanHaveMP5Grenade) == 1 ){
		if(pev->weapons == 0 || pev->weapons == HGRUNT_SHOTGUN || pev->weapons == (HGRUNT_SHOTGUN | HGRUNT_HANDGRENADE) ){
			//No shotguns, only the HGrunt_9MMAR.
			pev->weapons = HGRUNT_9MMAR | HGRUNT_HANDGRENADE;
			//Notice that if it used to be the 9MMAR w/ grenade launcher, we don't mind, and we leave it.
		}
	}else{
		//Always use the 9MMAR (mp5 weapon).
		pev->weapons = HGRUNT_9MMAR | HGRUNT_HANDGRENADE;
	}

#else

	//would default to the mp5 if no weapon is present.
	if (pev->weapons == 0)
	{
		// initialize to original values

		//MODDD - this used to not be commented out.  The two commented out parts below were though.
		//UNDONE!
		pev->weapons = HGRUNT_9MMAR | HGRUNT_HANDGRENADE;

		//Changing to use the mp5.
		//No.
		//pev->weapons = HGRUNT_9MMAR;

		// pev->weapons = HGRUNT_SHOTGUN;
		// pev->weapons = HGRUNT_9MMAR | HGRUNT_GRENADELAUNCHER;
	}

#endif


#if FORCE_MP5 == 1
	//mp5 is the default.  Leave as is to always have an mp5's clipsize (else statement)
	m_cClipSize = getClipSize();
	SetBodygroup( GUN_GROUP, GUN_MP5 );
#else

	if (FBitSet( pev->weapons, HGRUNT_SHOTGUN ))
	{
		SetBodygroup( GUN_GROUP, GUN_SHOTGUN );
		m_cClipSize		= 8;
	}
	else
	{
		//MODDD - wait.. why not set the GUN_GROUP to GUN_MP5 then??
		// Yes GUN_MP5 happens to be 0, just seems sloppy not to do this still.
		// If the model changes and GUN_MP5 is any other number, the assumption that
		// GUN_MP5 is the default from not setting anything goes out the window.
		SetBodygroup(GUN_GROUP, GUN_MP5);
		m_cClipSize		= GRUNT_CLIP_SIZE;
	}
#endif

	m_cAmmoLoaded		= getClipSize();

	if (RANDOM_LONG( 0, 99 ) < 80)
		pev->skin = 0;	// light skin
	else
		pev->skin = 1;	// dark skin




#if FORCE_ONE_HEAD == 1
	//This seems to be the default "gas mask".

	//SetBodygroup( HEAD_GROUP, HEAD_GRUNT );
	//TEST....
	SetBodygroup( HEAD_GROUP, HEAD_GRUNT );


	//easyPrint("SetBodygroup %d\n", HEAD_GRUNT);
	//easyPrint("SPAWNED WITH HEAD %d\n", pev->body);


	//pev->body = 0;

#else
	SetBodygroup( HEAD_GROUP, HEAD_M203 );

	if (FBitSet( pev->weapons, HGRUNT_SHOTGUN ))
	{
		SetBodygroup( HEAD_GROUP, HEAD_SHOTGUN);
	}
	else if (FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER ))
	{
		pev->skin = 1; // alway dark skin
	}

#endif

	CTalkMonster::g_talkWaitTime = 0;

	MonsterInit();


	/*
#if FORCE_MP5 == 1
	//do it again.
	SetBodygroup( HEAD_GROUP, HEAD_GRUNT );
	SetBodygroup( GUN_GROUP, GUN_MP5 );
#endif
	*/


	//MODDD
	if(runAndGunSequenceID == -1){
		runAndGunSequenceID = LookupSequence("runandgun");
	}

	
	SetUse( &CHGrunt::partyUse );

}




extern int global_useSentenceSave;
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHGrunt :: Precache()
{
	PRECACHE_MODEL("models/hgrunt.mdl");

	global_useSentenceSave = TRUE;

	PRECACHE_SOUND( "hgrunt/gr_mgun1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_mgun2.wav" );

	PRECACHE_SOUND( "hgrunt/gr_die1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die3.wav" );

	PRECACHE_SOUND( "hgrunt/gr_pain1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain3.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain4.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain5.wav" );

	PRECACHE_SOUND( "hgrunt/gr_reload1.wav" );

	PRECACHE_SOUND( "weapons/glauncher.wav", TRUE ); //player precache, can't skip.
	PRECACHE_SOUND( "weapons/sbarrel1.wav", TRUE ); //same.

	PRECACHE_SOUND("zombie/claw_miss2.wav");// because we use the basemonster SWIPE animation event

	//MODDD - can now play "Strike" sounds on hitting something with the kick.
	PRECACHE_SOUND("zombie/claw_strike1.wav");
	PRECACHE_SOUND("zombie/claw_strike2.wav");
	PRECACHE_SOUND("zombie/claw_strike3.wav");

	precacheStandardMeleeAttackMissSounds(); //MODDD - lazy lazy.

	global_useSentenceSave = FALSE;









	
	// get voice pitch
	if (RANDOM_LONG(0,1))
		m_voicePitch = 109 + RANDOM_LONG(0,7);
	else
		m_voicePitch = 100;




	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell
	m_iShotgunShell = PRECACHE_MODEL ("models/shotgunshell.mdl");









	//ALSO precache "HAssault", since HGrunts may become them.
	/////////////////////////////////////////////////////////////////////////////////////////////
	PRECACHE_MODEL("models/hassault.mdl");
	m_iBrassShell = PRECACHE_MODEL ("models/shell.mdl");// brass shell

	global_useSentenceSave = TRUE;



	
	PRECACHE_SOUND("hassault/hw_spin.wav");	
	PRECACHE_SOUND("hassault/hw_spinup.wav");	
	PRECACHE_SOUND("hassault/hw_spindown.wav");	
	PRECACHE_SOUND("hassault/hw_shoot1.wav");	
	PRECACHE_SOUND("hassault/hw_shoot2.wav");
	PRECACHE_SOUND("hassault/hw_shoot3.wav");
	PRECACHE_SOUND("hassault/hw_gun4.wav");


	PRECACHE_SOUND("zombie/claw_miss1.wav");
	PRECACHE_SOUND("zombie/zo_attack1.wav");
	PRECACHE_SOUND("zombie/zo_attack2.wav");

	//MODDD - probably don't actually need to (not needed for sentences, which these already were in, stand-alone)?  Verify this though.
	//Yes, these sounds are never called directly. The HG_IDLE sentencegroup (0-2) already uses them.
	//PRECACHE_SOUND("hgrunt/gr_idle1.wav");
	//PRECACHE_SOUND("hgrunt/gr_idle2.wav");
	//PRECACHE_SOUND("hgrunt/gr_idle3.wav");

	PRECACHE_SOUND("hassault/hw_alert.wav");

	global_useSentenceSave = FALSE;
	/////////////////////////////////////////////////////////////////////////////////////////////





}

//=========================================================
// start task
//=========================================================
void CHGrunt :: StartTask ( Task_t *pTask )
{


	if(monsterID == 1){
		int x = 0;
	}
	
	//return;
	EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("HGRUNT STARTTASK eeee %s %d", getScheduleName(), getTaskNumber()));

	//NOTICE - the method TaskBegin in basemonster.h, called by schedule.cpp's routinely called "MaintainSchedule" right before starting a task (StartTask here),
	//         already sets m_iTaskSatus to TASKSTATUS_RUNNING.
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch ( pTask->iTask )
	{

	case TASK_GRUNT_CHECK_FIRE:
		if ( !NoFriendlyFire() )
		{
			SetConditions( bits_COND_GRUNT_NOFIRE );
		}
		TaskComplete();
		break;

	case TASK_GRUNT_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;

	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		// grunt no longer assumes he is covered if he moves
		Forget( bits_MEMORY_INCOVER );
		CSquadMonster ::StartTask( pTask );
		break;

	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;

	case TASK_GRUNT_FACE_TOSS_DIR:
		break;

	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CSquadMonster :: StartTask( pTask );
		if (pev->movetype == MOVETYPE_FLY)
		{
			m_IdealActivity = ACT_GLIDE;
		}
		break;
	//MODDD
	case TASK_HGRUNT_PICK_STRAFE_ACT:
		//just use the temp var.
		m_movementActivity = (Activity)(int)tempStrafeAct;
		TaskComplete();
	break;
	//MODDD
	case TASK_HGRUNT_STRAFEPATH:

		m_IdealActivity = m_movementActivity;
		TaskComplete();

		//MoveToLocation( m_movementActivity, 2, this->m_vecMoveGoal );

	break;

	default:
		CSquadMonster :: StartTask( pTask );
		break;
	}
}




BOOL CHGrunt::usesAdvancedAnimSystem(void){
	return TRUE;
}



void CHGrunt::moveAnimUpdate(void){

	//???
	/*
	switch(strafeMode){
		case 0:
			return LookupSequence("strafeleft");
		break;
		case 1:
			return LookupSequence("strafeleft_fire");
		break;
		case 2:
			return LookupSequence("straferight");
		break;
		case 3:
			return LookupSequence("straferight_fire");
		break;
	}//END OF switch(strafeMode)

	*/
}


void CHGrunt::updateStrafeStatus(void){


	EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("STATUS:::??? %d %d %d", friendlyFireStrafeBlock, !strafeCanFire, hgruntAllowStrafeFire() == FALSE));

	//on getting a strafe anim, check for NoFriendlyFire.
	if ( friendlyFireStrafeBlock || !strafeCanFire || hgruntAllowStrafeFire() == FALSE ){
		//there is friendly fire!
		//can't fire.  nope.
		if(idealStrafeMode == 1){
			idealStrafeMode = 0;
		}
		if(idealStrafeMode == 3){
			idealStrafeMode = 2;
		}
	}else{

		//no friendly, see if we can fire...
		friendlyFireStrafeBlock = FALSE;
		if( !outOfAmmoStrafeFireBlock() && idealStrafeMode == 0  ){
			idealStrafeMode = 1;
		}
		if( !outOfAmmoStrafeFireBlock() && idealStrafeMode == 2 ){
			idealStrafeMode = 3;
		}
	}

	//IS THAT OKAY?
	//strafeMode = idealStrafeMode;



	if(EASY_CVAR_GET(hgruntForceStrafeFireAnim) == 1){
		if(idealStrafeMode == 0){
			idealStrafeMode = 1;
			//strafeMode = 1;
		}
		if(idealStrafeMode == 2){
			idealStrafeMode = 3;
			//strafeMode = 3;
		}
	}



}


void CHGrunt::onAnimationLoop(void){

	EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("I have looped: %d ", m_IdealActivity == m_movementActivity && (m_movementActivity == ACT_STRAFE_LEFT || m_movementActivity == ACT_STRAFE_RIGHT)));

	//check: should we change animation?
	if(pev->deadflag == DEAD_NO && m_IdealActivity == m_movementActivity ){
		if( (m_movementActivity == ACT_STRAFE_LEFT || m_movementActivity == ACT_STRAFE_RIGHT)){

			updateStrafeStatus();
			//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("HOW THE quack %d %d", strafeMode, idealStrafeMode ));
			if(strafeMode == -1 || strafeMode != idealStrafeMode){
				strafeMode = idealStrafeMode;

				//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("ANIMATION CHANGE!!!! A" ));
				int iSequence = LookupActivityHard (m_IdealActivity);
				ResetSequenceInfo( );
				SetYawSpeed();
				//m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

				// Set to the desired anim, or default anim if the desired is not present
				if ( iSequence > ACTIVITY_NOT_AVAILABLE )
				{
					if ( pev->sequence != iSequence || !m_fSequenceLoops )
					{
						pev->frame = 0;
					}

						//do so.
				}

			}

		}//END OF movement activity being either strafe... check.
		else if(m_movementActivity == ACT_RUN){
			//update the run anim?

			//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("SOOOOOOO??? %d    %d", runAndGunSequenceID, runAndGun));
			if(runAndGun && pev->sequence != runAndGunSequenceID || (!runAndGun && pev->sequence == runAndGunSequenceID) ){
				int iSequence = LookupActivityHard (m_IdealActivity);

				pev->sequence = iSequence;

				ResetSequenceInfo( );
				SetYawSpeed();
				//m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

				// Set to the desired anim, or default anim if the desired is not present
				//if ( iSequence > ACTIVITY_NOT_AVAILABLE )
				{
					//if ( pev->sequence != iSequence || !m_fSequenceLoops )
					{
						pev->frame = 0;
					}
					//do so.
				}
			}

		}//END OF movement activity being run... check.
	}//END OF (ideal activity & movement activity checks)

}


int CHGrunt::LookupActivityHard(int activity){

	pev->framerate = 1;
	m_flFramerateSuggestion = 1;

	resetEventQueue();

	//HACK - just force it for now,  animation is seamless for the most part?
	//....nah.
	//strafeMode = idealStrafeMode;

	//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("WWWHHHHHYYYYYY %d", activity));
	BOOL tempFriendlyFire;
	//easyForcePrintLine("LookupActivityHard: %d", activity);

	switch(activity){
		
		case ACT_TURN_RIGHT:
		case ACT_TURN_LEFT:
		{
			if(m_pSchedule == slGruntSweep){
				//turn slower.
				m_flFramerateSuggestion = 0.92;
			}
		break;}
		
		case ACT_STRAFE_LEFT:
		case ACT_STRAFE_RIGHT:
			//TODO - check. Does this autoloop or not?? I don't think so...
			//easyForcEPrintLine("STRAFE ACT CALLED");
			tempFriendlyFire = !NoFriendlyFire();

			if(tempFriendlyFire){
				friendlyFireStrafeBlock = TRUE;
			}

			//do checks here too just in case?
			updateStrafeStatus();



			if(strafeMode == -1){
				//get ideal!
				strafeMode = idealStrafeMode;

			}
			//strafeMode = idealStrafeMode;
			//hgruntLock
			//IS THAT OKAY?
			strafeMode = idealStrafeMode;

			//return CBaseAnimating::LookupActivity(ACT_RUN);
			//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("GAHHH %d", strafeMode));
			switch(strafeMode){
				case 0:
					//return LookupSequence("straferight");
					m_flFramerateSuggestion = EASY_CVAR_GET(hgruntStrafeAnimSpeedMulti);
					return LookupSequence("straferight");
				break;
				case 1:
					//this->animEventQueuePush(3.0f / 30.0f, 0);
					//this->animEventQueuePush(7.5f / 30.0f, 1);
					//this->animEventQueuePush(11.0f / 30.0f, 2);
					m_flFramerateSuggestion = EASY_CVAR_GET(hgruntStrafeAnimSpeedMulti);
					return LookupSequence("straferight_fire");
				break;
				case 2:
					//return LookupSequence("strafeleft");
					m_flFramerateSuggestion = EASY_CVAR_GET(hgruntStrafeAnimSpeedMulti);
					return LookupSequence("strafeleft");
				break;
				case 3:
					//this->animEventQueuePush(7.0f / 30.0f, 0);
					m_flFramerateSuggestion = EASY_CVAR_GET(hgruntStrafeAnimSpeedMulti);
					return LookupSequence("strafeleft_fire");
				break;
				default:
					//???
					EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("HGRUNT SEVERE ERROR B: Strafe failed to fetch anim!"));
					return CBaseAnimating::LookupActivity(activity);
				break;
			}//END OF switch(strafeMode)

		break;
		case ACT_RUN:
			//are we strafing?  If so, substitute!

					//return LookupSequence("strafeleft");
			//return LookupSequence("strafeleft");



			//strafeMode = idealStrafeMode;


			if(strafeMode >= 0){
				//???
			}

			if(runAndGun == TRUE){

				
				//Also needs to see if firing would inflict friendly fire.
				if(!NoFriendlyFire()){
					friendlyFireStrafeBlock = TRUE;
				}else{
					//runnin' and gunnin instead!
					/*
					this->animEventQueuePush(3.0f / 30.0f, 0);
					this->animEventQueuePush(11.0f / 30.0f, 1);
					*/

					m_flFramerateSuggestion = EASY_CVAR_GET(hgruntRunAndGunAnimSpeedMulti);
					return LookupSequence("runandgun");
				}
			}
			return CBaseAnimating::LookupActivity(activity);
		break;
		case ACT_WALK:
			//??

			return CBaseAnimating::LookupActivity(activity);
		break;
	}

	return CBaseAnimating::LookupActivity(activity);
}//END OF LookupActivityHard


int CHGrunt::tryActivitySubstitute(int activity){

	switch(activity){
		case ACT_STRAFE_LEFT:
		case ACT_STRAFE_RIGHT:
			//are we strafing?  If so, substitute!

			//return CBaseAnimating::LookupActivity(ACT_RUN);

			switch(strafeMode){
				case 0:
					//return LookupSequence("straferight");
					return LookupSequence("strafeleft");
				break;
				case 1:
					return LookupSequence("strafeleft_fire");
				break;
				case 2:
					//return LookupSequence("strafeleft");
					return LookupSequence("straferight");
				break;
				case 3:
					return LookupSequence("straferight_fire");
				break;
				default:
					//???
					EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("HGRUNT SEVERE ERROR A: Strafe failed to fetch anim!"));
					return CBaseAnimating::LookupActivity(activity);
				break;
			}

		break;
		case ACT_RUN:

			//return LookupSequence("strafeleft");
			//return LookupSequence("strafeleft");

			if(runAndGun == TRUE){
				return LookupSequence("runandgun");
			}

			return CBaseAnimating::LookupActivity(activity);
		break;
	}

	
	return CBaseAnimating::LookupActivity(activity);
}//END OF tryActivitySubstitute



//=========================================================
// MakeIdealYaw - gets a yaw value for the caller that would
// face the supplied vector. Value is stuffed into the monster's
// ideal_yaw
//=========================================================
void CHGrunt :: MakeIdealYaw( Vector vecTarget )
{
	Vector	vecProjection;

	//MODDD - this kind of immitation "Strafe" is too crude and causes issues with the AI sometimes, like fiddling-around near the end point of a path..
	// strafing monster needs to face 90 degrees away from its goal

	//UTIL_drawLineFrame(pev->origin + Vector(0, 0, 3), (vecTarget + Vector(0, 0, 3) ), 14, 255, 0, 255 );
	Vector vecTargetRight = CrossProduct( (vecTarget - pev->origin).Normalize() , Vector(0, 0, 1)).Normalize();
	Vector origin2DOnly = Vector(pev->origin.x, pev->origin.y, 0);

	if ( m_IdealActivity == m_movementActivity && m_movementActivity == ACT_STRAFE_LEFT )
	{
		//UTIL_drawLineFrame(pev->origin + Vector(0, 0, 3), (pev->origin + vecTargetRight*25 + Vector(0, 0, 3) ), 14, 0, 255, 124 );
		//SMELLS LIKE TEEN bee
		pev->ideal_yaw = UTIL_VecToYaw(-vecTargetRight);
		return;

		vecProjection.x = -vecTarget.y;
		vecProjection.y = vecTarget.x;
		vecProjection.z = 0;

		//UTIL_drawLineFrame(pev->origin, (vecProjection + Vector(0, 0, 3) ), 14, 0, 255, 124 );
		pev->ideal_yaw = UTIL_VecToYaw( vecProjection - pev->origin );
	}
	else if ( m_IdealActivity == m_movementActivity && m_movementActivity == ACT_STRAFE_RIGHT)
	{
		//UTIL_drawLineFrame(pev->origin + Vector(0, 0, 3), (pev->origin + vecTargetRight*25 + Vector(0, 0, 3) ), 14, 255, 0, 124 );
		//SMELLS LIKE TEEN bee
		pev->ideal_yaw = UTIL_VecToYaw(vecTargetRight);
		return;

		vecProjection.x = vecTarget.y;
		vecProjection.y = vecTarget.x;
		vecProjection.z = 0;

		//UTIL_drawLineFrame(pev->origin, (vecProjection + Vector(0, 0, 3) ), 14, 255, 0, 124 );
		pev->ideal_yaw = UTIL_VecToYaw( vecProjection - pev->origin );
	}
	else
	{
		vecProjection.x = vecTarget.x;
		vecProjection.y = vecTarget.y;
		vecProjection.z = 0;

		//UTIL_drawLineFrame(pev->origin, (vecProjection + Vector(0, 0, 3) ), 14, 255, 255, 255 );
		pev->ideal_yaw = UTIL_VecToYaw ( vecTarget - pev->origin );
	}
}//END OF MakeIdealYaw





void CHGrunt::startReanimation(void) {
	CBaseMonster::startReanimation();


	//SetBodygroup(GUN_GROUP, GUN_MP5);



}//END OF startReanimation
















//=========================================================
// RunTask
//=========================================================
void CHGrunt :: RunTask ( Task_t *pTask )
{

	//easyForcePrintLine("IM HGRUNT AND THIS IS %s : %d", this->getScheduleName(), pTask->iTask);


	if(pTask->iTask != TASK_WAIT_FOR_MOVEMENT){
		//mark that we aren't keeping track of this.  Cheap way for now.
		nextPositionCheckTime = -1;
	}

	switch ( pTask->iTask )
	{


	//can't hurt doing either?
	case TASK_RANGE_ATTACK1:
	case TASK_PLAY_SEQUENCE_FACE_ENEMY:
	{

		lookAtEnemy_pitch();
		CSquadMonster::RunTask(pTask);
	break;}
	case TASK_WAIT_FOR_MOVEMENT:
		


		if(EASY_CVAR_GET(hgruntMovementDeltaCheck) == 1){
			if(nextPositionCheckTime == -1){
				nextPositionCheckTime = gpGlobals->time + 1.0f;
			}else{
			
				if(gpGlobals->time > nextPositionCheckTime){

					float delta = (vecPrevOrigin - pev->origin).Length() ;
					EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("DELTA IS?? %.2f", delta));
					if(delta > 1){ //|| EASY_CVAR_GET(hgruntSpeedMulti) <= 0.3f){
						//pass.  Just keep going.
						nextPositionCheckTime = gpGlobals->time + 1.0f;
					}else{
						//no good!  Stop!
						//TaskFail();
						nextPositionCheckTime = -1;
						return;
					}
				}
			}
		}//END OF EASY_CVAR_GET(hgruntMovementDeltaCheck) ...


		vecPrevOrigin = pev->origin;

		CSquadMonster::RunTask(pTask);
	break;
	case TASK_GRUNT_FACE_TOSS_DIR:
		{
			// project a point along the toss vector and turn to face that point.
			MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
			ChangeYaw( pev->yaw_speed );

			if ( FacingIdeal() )
			{
				m_iTaskStatus = TASKSTATUS_COMPLETE;
			}
			break;
		}
	default:
		{
			CSquadMonster :: RunTask( pTask );
			break;
		}
	}




}




void CHGrunt::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
//	float flYaw = UTIL_VecToYaw ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin );// build a yaw that points to the goal.
//	WALK_MOVE( ENT(pev), flYaw, m_flGroundSpeed * flInterval, WALKMOVE_NORMAL );
	if ( m_IdealActivity != m_movementActivity )
		m_IdealActivity = m_movementActivity;


	EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine( "MONSTERID: %d MY GROUNDSPEED IS %.2f", this->monsterID, m_flGroundSpeed));
	




	float flTotal = m_flGroundSpeed * pev->framerate * EASY_CVAR_GET(animationFramerateMulti) * flInterval * EASY_CVAR_GET(hgruntSpeedMulti);
	float flStep;

	//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("WHAT IS THIS junk??? %.2f %.2f %.2f %.2f %d", m_flGroundSpeed, pev->framerate, flInterval, EASY_CVAR_GET(hgruntSpeedMulti), strafeMode));

	while (flTotal > 0.001)
	{
		// don't walk more than 16 units or stairs stop working
		flStep = min( 16.0, flTotal );


		if(strafeMode == -1){
			UTIL_MoveToOrigin ( ENT(pev), m_Route[ m_iRouteIndex ].vecLocation, flStep, MOVE_NORMAL );
		}else{
			UTIL_MoveToOrigin ( ENT(pev), m_Route[ m_iRouteIndex ].vecLocation, flStep, MOVE_STRAFE );
		}


		//UTIL_MoveToOrigin ( ENT(pev), m_Route[ m_iRouteIndex ].vecLocation, flStep, MOVE_STRAFE );
		flTotal -= flStep;
	}
	// ALERT( at_console, "dist %f\n", m_flGroundSpeed * pev->framerate * flInterval );
}








//=========================================================
// PainSound
//=========================================================
void CHGrunt :: PainSound ( void )
{
	if ( gpGlobals->time > m_flNextPainTime )
	{


		//MODDD NOTE - looks like this HG_PAIN sentence got canned.
#if 0
		if ( RANDOM_LONG(0,99) < 5 )
		{
			// pain sentences are rare
			if (FOkToSpeak())
			{
				SENTENCEG_PlayRndSz(ENT(pev), "HG_PAIN", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, PITCH_NORM);
				JustSpoke();
				return;
			}
		}
#endif
		switch ( RANDOM_LONG(0,6) )
		{
		case 0:
			EMIT_SOUND_FILTERED( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain3.wav", 1, ATTN_NORM );
			break;
		case 1:
			EMIT_SOUND_FILTERED( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain4.wav", 1, ATTN_NORM );
			break;
		case 2:
			EMIT_SOUND_FILTERED( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain5.wav", 1, ATTN_NORM );
			break;
		case 3:
			EMIT_SOUND_FILTERED( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain1.wav", 1, ATTN_NORM );
			break;
		case 4:
			EMIT_SOUND_FILTERED( ENT(pev), CHAN_VOICE, "hgrunt/gr_pain2.wav", 1, ATTN_NORM );
			break;
		}

		m_flNextPainTime = gpGlobals->time + 1;
	}
}

//=========================================================
// DeathSound
//=========================================================
void CHGrunt :: DeathSound ( void )
{


	/*
	//MODDD.  Different place now.
	switch ( RANDOM_LONG(0,2) )
	{
	case 0:
		EMIT_SOUND_FILTERED( ENT(pev), CHAN_VOICE, "hgrunt/gr_die1.wav", 1, ATTN_IDLE );
		break;
	case 1:
		EMIT_SOUND_FILTERED( ENT(pev), CHAN_VOICE, "hgrunt/gr_die2.wav", 1, ATTN_IDLE );
		break;
	case 2:
		EMIT_SOUND_FILTERED( ENT(pev), CHAN_VOICE, "hgrunt/gr_die3.wav", 1, ATTN_IDLE );
		break;
	}
	*/



	if(GetBodygroup(1) == 1){
		//headless.
		EMIT_SOUND_FILTERED( ENT(pev), CHAN_VOICE, "common/bodysplat.wav", 1, ATTN_IDLE, 0, 108, FALSE );

	}else{
		switch ( RANDOM_LONG(0,2) )
		{
		case 0:
			EMIT_SOUND_FILTERED( ENT(pev), CHAN_VOICE, "hgrunt/gr_die1.wav", 1, ATTN_IDLE );
			break;
		case 1:
			EMIT_SOUND_FILTERED( ENT(pev), CHAN_VOICE, "hgrunt/gr_die2.wav", 1, ATTN_IDLE );
			break;
		case 2:
			EMIT_SOUND_FILTERED( ENT(pev), CHAN_VOICE, "hgrunt/gr_die3.wav", 1, ATTN_IDLE );
			break;
		}
	}


}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================





//=========================================================
// GruntFail
//=========================================================
Task_t	tlGruntFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slGruntFail[] =
{
	{
		tlGruntFail,
		ARRAYSIZE ( tlGruntFail ),
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2,
		0,
		"Grunt Fail"
	},
};

//=========================================================
// Grunt Combat Fail
//=========================================================
Task_t	tlGruntCombatFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY,		(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slGruntCombatFail[] =
{
	{
		tlGruntCombatFail,
		ARRAYSIZE ( tlGruntCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Grunt Combat Fail"
	},
};

//=========================================================
// Victory dance!
//=========================================================
Task_t	tlGruntVictoryDance[] =
{
	{ TASK_STOP_MOVING,						(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_WAIT,							(float)1.5					},
	{ TASK_GET_PATH_TO_ENEMY_CORPSE,		(float)0					},
	{ TASK_WALK_PATH,						(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0					},
	{ TASK_FACE_ENEMY,						(float)0					},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE	},
};

Schedule_t	slGruntVictoryDance[] =
{
	{
		tlGruntVictoryDance,
		ARRAYSIZE ( tlGruntVictoryDance ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"GruntVictoryDance"
	},
};

//=========================================================
// Establish line of fire - move to a position that allows
// the grunt to attack.
//=========================================================
Task_t tlGruntEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_GRUNT_ELOF_FAIL	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0						},
	{ TASK_GRUNT_SPEAK_SENTENCE,(float)0						},
	{ TASK_RUN_PATH,			(float)0						},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0						},

	//MODDD - extra step.
	//{ TASK_CHECK_STUMPED,	(float)0						},
	//Actually no. If we get to the goal and don't see the enemy, do a sweep first. Seeing them will interrupt the sweep.
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_SWEEP	},
};

Schedule_t slGruntEstablishLineOfFire[] =
{
	{
		tlGruntEstablishLineOfFire,
		ARRAYSIZE ( tlGruntEstablishLineOfFire ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK1	|
		
		//MODDD - removing this as an interrupter.
		//Being interrupted by being able to throw grenades, even if not in a grenade-throwing role, is potentially annoying.
		//bits_COND_CAN_RANGE_ATTACK2	|

		bits_COND_CAN_MELEE_ATTACK2	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntEstablishLineOfFire"
	},
};

//=========================================================
// GruntFoundEnemy - grunt established sight with an enemy
// that was hiding from the squad.
//=========================================================
Task_t	tlGruntFoundEnemy[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_SIGNAL1			},
};

Schedule_t	slGruntFoundEnemy[] =
{
	{
		tlGruntFoundEnemy,
		ARRAYSIZE ( tlGruntFoundEnemy ),
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntFoundEnemy"
	},
};

//=========================================================
// GruntCombatFace Schedule
//=========================================================
Task_t	tlGruntCombatFace1[] =
{
	{ TASK_STOP_MOVING,				0							},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_FACE_ENEMY,				(float)0					},
	//MODDD - now faces the enemy instead, good to keep up to date during this waiting time.
	{ TASK_WAIT_FACE_ENEMY,					(float)1.5					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_SWEEP	},
};

Schedule_t	slGruntCombatFace[] =
{
	{
		tlGruntCombatFace1,
		ARRAYSIZE ( tlGruntCombatFace1 ),
		bits_COND_NEW_ENEMY				|
		bits_COND_ENEMY_DEAD			|
		bits_COND_CAN_RANGE_ATTACK1		|
		bits_COND_CAN_RANGE_ATTACK2,
		0,
		//MODDD - why named so generic? "Combat Face"? what?
		"hgrunt Combat Face"
	},
};

//=========================================================
// Suppressing fire - don't stop shooting until the clip is
// empty or grunt gets hurt.
//=========================================================
Task_t	tlGruntSignalSuppress[] =
{
	{ TASK_STOP_MOVING,					0						},
	{ TASK_FACE_IDEAL,					(float)0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_SIGNAL2		},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
	{ TASK_FACE_ENEMY,					(float)0				},
	{ TASK_GRUNT_CHECK_FIRE,			(float)0				},
	{ TASK_RANGE_ATTACK1,				(float)0				},
};

Schedule_t	slGruntSignalSuppress[] =
{
	{
		tlGruntSignalSuppress,
		ARRAYSIZE ( tlGruntSignalSuppress ),
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"SignalSuppress"
	},
};

Task_t	tlGruntSuppress[] =
{
	{ TASK_STOP_MOVING,			0							},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0					},
	{ TASK_RANGE_ATTACK1,		(float)0					},
};

Schedule_t	slGruntSuppress[] =
{
	{
		tlGruntSuppress,
		ARRAYSIZE ( tlGruntSuppress ),
		bits_COND_ENEMY_DEAD		|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Suppress"
	},
};


//=========================================================
// grunt wait in cover - we don't allow danger or the ability
// to attack to break a grunt's run to cover schedule, but
// when a grunt is in cover, we do want them to attack if they can.
//=========================================================
Task_t	tlGruntWaitInCover[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_ACTIVITY,			(float)ACT_IDLE				},
	{ TASK_WAIT_FACE_ENEMY,			(float)1					},
};

Schedule_t	slGruntWaitInCover[] =
{
	{
		tlGruntWaitInCover,
		ARRAYSIZE ( tlGruntWaitInCover ),
		bits_COND_NEW_ENEMY			|
		bits_COND_HEAR_SOUND		|
		bits_COND_CAN_RANGE_ATTACK1	|
		bits_COND_CAN_RANGE_ATTACK2	|
		bits_COND_CAN_MELEE_ATTACK1	|
		bits_COND_CAN_MELEE_ATTACK2,

		bits_SOUND_DANGER,
		"GruntWaitInCover"
	},
};

//=========================================================
// run to cover.
// !!!BUGBUG - set a decent fail schedule here.
//=========================================================
Task_t	tlGruntTakeCover1[] =
{
	{ TASK_STOP_MOVING,				(float)0							},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_GRUNT_TAKECOVER_FAILED	},
	{ TASK_WAIT,					(float)0.2							},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0							},
	{ TASK_GRUNT_SPEAK_SENTENCE,	(float)0							},
	{ TASK_RUN_PATH,				(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0							},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER			},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_WAIT_FACE_ENEMY	},
};

Schedule_t	slGruntTakeCover[] =
{
	{
		tlGruntTakeCover1,
		ARRAYSIZE ( tlGruntTakeCover1 ),
		0,
		0,
		"TakeCover"
	},
};

//=========================================================
// drop grenade then run to cover.
//=========================================================
Task_t	tlGruntGrenadeCover1[] =
{
	{ TASK_STOP_MOVING,						(float)0							},

	//MODDD - bug in the base game found? Holy moly.
	//        This isn't a minimum or maximum distance, this ends up setting m_flMoveWaitFinished, a time typically used
	//        to deny movement such as waiting for a door to open.  So this makes the HGrunt want to stand still for 99 seconds
	//        next time if it is not cleared between then.  Don't look at me.
	//        And see the TASK_CLEAR_MOVE_WAIT below? That resets m_flMoveWaitFinished.
	//        But I bet it got interrupted if this schedule were to fail... even though it has no fail conditions?
	//        It doesn't make failure completely impossible though. Other tasks like TASK_FIND_FAR_NODE_COVER_FROM_ENEMY
	//        could very well fail and leave the m_flMoveWaitFinished uncleared to perma-pause whatever future schedule 
	//        calls "Move" in monster. I don't see what the purpose of settting m_flMoveWaitFinished was to begin with.
	//        And the cherry on top? HGrunt is the only one, and in this one schedule, the only ever use of "TASK_CLEAR_MOVE_WAIT".
	//        So nowhere else even manipulates m_flMoveWaitFinshed by schedule like this, at least not so drastically (99 seconds).
	//        More importantly, this is the only place TASK_FIND_COVER_FROM_ENEMY gets a non-zero extra float parameter at all.
	//{ TASK_FIND_COVER_FROM_ENEMY,			(float)99							},
	{ TASK_FIND_COVER_FROM_ENEMY,			(float)0							},

	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY,	(float)384							},
	{ TASK_PLAY_SEQUENCE,					(float)ACT_SPECIAL_ATTACK1			},
	//MODDD - unnecessary now.{ TASK_CLEAR_MOVE_WAIT,					(float)0							},
	{ TASK_RUN_PATH,						(float)0							},
	{ TASK_WAIT_FOR_MOVEMENT,				(float)0							},
	//MODDD NOTE - not a "TAKE_COVER_FROM_SOUND" or from origin since we've... dropped a live grenade?
	//OK then.
	{ TASK_SET_SCHEDULE,					(float)SCHED_GRUNT_WAIT_FACE_ENEMY	},
};

Schedule_t	slGruntGrenadeCover[] =
{
	{
		tlGruntGrenadeCover1,
		ARRAYSIZE ( tlGruntGrenadeCover1 ),
		0,
		0,
		"GrenadeCover"
	},
};


//MODDD NOTE - copy of the exact same comment above? ok then.
//=========================================================
// drop (THROW?) grenade then run to cover.
//=========================================================
Task_t	tlGruntTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY,						(float)0							},
	{ TASK_RANGE_ATTACK2, 					(float)0							},
	{ TASK_SET_SCHEDULE,					(float)SCHED_TAKE_COVER_FROM_ENEMY	},
};

Schedule_t	slGruntTossGrenadeCover[] =
{
	{
		tlGruntTossGrenadeCover1,
		ARRAYSIZE ( tlGruntTossGrenadeCover1 ),
		0,
		0,
		"TossGrenadeCover"
	},
};

//=========================================================
// hide from the loudest sound source (to run from grenade)
//=========================================================
Task_t	tlGruntTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE,			(float)SCHED_COWER			},// duck and cover if cannot move from explosion
	{ TASK_STOP_MOVING,					(float)0					},
	{ TASK_FIND_COVER_FROM_BEST_SOUND,	(float)0					},
	{ TASK_RUN_PATH,					(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,			(float)0					},
	{ TASK_REMEMBER,					(float)bits_MEMORY_INCOVER	},
	{ TASK_TURN_LEFT,					(float)179					},
};

Schedule_t	slGruntTakeCoverFromBestSound[] =
{
	{
		tlGruntTakeCoverFromBestSound,
		ARRAYSIZE ( tlGruntTakeCoverFromBestSound ),
		0,
		0,
		"GruntTakeCoverFromBestSound"
	},
};

//=========================================================
// Grunt reload schedule
//=========================================================
Task_t	tlGruntHideReload[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_RELOAD			},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH,				(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_REMEMBER,				(float)bits_MEMORY_INCOVER	},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RELOAD			},
};

Schedule_t slGruntHideReload[] =
{
	{
		tlGruntHideReload,
		ARRAYSIZE ( tlGruntHideReload ),
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"GruntHideReload"
	}
};



//=========================================================
// Do a turning sweep of the area
//=========================================================
//MODDD NOTICE - the end of this may be a good place for a TASK_CHECK_STUMPED.  But be careful.
Task_t	tlGruntSweep[] =
{

	/*
	//ACTUALLY changed a little more. Now looks +- 90 degrees instead.
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	{ TASK_TURN_LEFT,			(float)179	},
	{ TASK_WAIT,				(float)1	},
	*/

	
	{ TASK_TURN_LEFT_FORCE_ACT,			(float)89	},
	{ TASK_WAIT,				(float)1.5	},
	{ TASK_TURN_RIGHT_FORCE_ACT,			(float)179	},
	{ TASK_WAIT,				(float)1.5	},
	{ TASK_TURN_LEFT_FORCE_ACT,			(float)90	},
	{ TASK_WAIT,				(float)1.0	},
	
	//MODDD - now see if re-routing is necessary, perhaps after staring into space for a little.
	{ TASK_CHECK_STUMPED,	(float)0						},
};

Schedule_t	slGruntSweep[] =
{
	{
		tlGruntSweep,
		ARRAYSIZE ( tlGruntSweep ),

		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_CAN_RANGE_ATTACK1	|
		
		//MODDD - perhaps not.
		//bits_COND_CAN_RANGE_ATTACK2	|
		
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD		|// sound flags
		bits_SOUND_DANGER		|
		bits_SOUND_PLAYER,

		"Grunt Sweep"
	},
};

//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlGruntRangeAttack1A[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_CROUCH },
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slGruntRangeAttack1A[] =
{
	{
		tlGruntRangeAttack1A,
		ARRAYSIZE ( tlGruntRangeAttack1A ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_HEAR_SOUND		|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_NO_AMMO_LOADED,

		bits_SOUND_DANGER,
		"Range Attack1A"
	},
};


//=========================================================
// primary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlGruntRangeAttack1B[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,(float)ACT_IDLE_ANGRY  },
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_GRUNT_CHECK_FIRE,	(float)0		},
	{ TASK_RANGE_ATTACK1,		(float)0		},
};

Schedule_t	slGruntRangeAttack1B[] =
{
	{
		tlGruntRangeAttack1B,
		ARRAYSIZE ( tlGruntRangeAttack1B ),
		bits_COND_NEW_ENEMY			|
		bits_COND_ENEMY_DEAD		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_ENEMY_OCCLUDED	|
		bits_COND_NO_AMMO_LOADED	|
		bits_COND_GRUNT_NOFIRE		|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER,
		"Range Attack1B"
	},
};

//=========================================================
// secondary range attack. Overriden because base class stops attacking when the enemy is occluded.
// grunt's grenade toss requires the enemy be occluded.
//=========================================================
Task_t	tlGruntRangeAttack2[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_GRUNT_FACE_TOSS_DIR,		(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_RANGE_ATTACK2	},
	{ TASK_SET_SCHEDULE,			(float)SCHED_GRUNT_WAIT_FACE_ENEMY	},// don't run immediately after throwing grenade.
};

Schedule_t	slGruntRangeAttack2[] =
{
	{
		tlGruntRangeAttack2,
		ARRAYSIZE ( tlGruntRangeAttack2 ),
		0,
		0,
		"RangeAttack2"
	},
};


//=========================================================
// repel
//=========================================================
Task_t	tlGruntRepel[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_GLIDE 	},
};

Schedule_t	slGruntRepel[] =
{
	{
		tlGruntRepel,
		ARRAYSIZE ( tlGruntRepel ),
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER,
		"Repel"
	},
};


//=========================================================
// repel
//=========================================================
Task_t	tlGruntRepelAttack[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_FLY 	},
};

Schedule_t	slGruntRepelAttack[] =
{
	{
		tlGruntRepelAttack,
		ARRAYSIZE ( tlGruntRepelAttack ),
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Repel Attack"
	},
};

//=========================================================
// repel land
//=========================================================
Task_t	tlGruntRepelLand[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LAND	},
	{ TASK_GET_PATH_TO_LASTPOSITION,(float)0				},
	{ TASK_RUN_PATH,				(float)0				},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0				},
	{ TASK_CLEAR_LASTPOSITION,		(float)0				},
};

Schedule_t	slGruntRepelLand[] =
{
	{
		tlGruntRepelLand,
		ARRAYSIZE ( tlGruntRepelLand ),
		bits_COND_SEE_ENEMY			|
		bits_COND_NEW_ENEMY			|
		bits_COND_LIGHT_DAMAGE		|
		bits_COND_HEAVY_DAMAGE		|
		bits_COND_HEAR_SOUND,

		bits_SOUND_DANGER			|
		bits_SOUND_COMBAT			|
		bits_SOUND_PLAYER,
		"Repel Land"
	},
};


DEFINE_CUSTOM_SCHEDULES( CHGrunt )
{
	slGruntFail,
	slGruntCombatFail,
	slGruntVictoryDance,
	slGruntEstablishLineOfFire,
	slGruntFoundEnemy,
	slGruntCombatFace,
	slGruntSignalSuppress,
	slGruntSuppress,
	slGruntWaitInCover,
	slGruntTakeCover,
	slGruntGrenadeCover,
	slGruntTossGrenadeCover,
	slGruntTakeCoverFromBestSound,
	slGruntHideReload,
	slGruntSweep,
	slGruntRangeAttack1A,
	slGruntRangeAttack1B,
	slGruntRangeAttack2,
	slGruntRepel,
	slGruntRepelAttack,
	slGruntRepelLand,
	//MODDD
	slhgruntStrafeToLocation,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHGrunt, CSquadMonster );

//=========================================================
// SetActivity
//=========================================================
void CHGrunt :: SetActivity ( Activity NewActivity )
{
	//MODDD - good idea?
	signalActivityUpdate = FALSE;


	int iSequence = ACTIVITY_NOT_AVAILABLE;
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	//To differentiate between what's used for animation and what activity we're actually getting set to.
	//This is less confusing to the system for knowing not to reget the same ACT_IDLE_ANGRY over and over again because it isn't ACT_IDLE for instance.
	//This can be changed independently of NewActivity and used separately.
	//...Actually, nevermind this, we're just going to use activityToAnimateFor as the new actiivty to set anyways.
	//Looks like the issue was caused by not setting both m_Actiivty and m_IdealActivity, only one or the other causes problems.
	Activity activityToAnimateFor = NewActivity;


	switch ( activityToAnimateFor)
	{
	case ACT_RANGE_ATTACK1:
		// grunt is either shooting standing or shooting crouched

#if FORCE_MP5 == 1

		//assume using mp5.
		if ( m_fStanding )
		{
			// get aimable sequence
			iSequence = LookupSequence( "standing_mp5" );
		}
		else
		{
			// get crouching shoot
			iSequence = LookupSequence( "crouching_mp5" );
		}

#else

		if (FBitSet( pev->weapons, HGRUNT_9MMAR))
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "standing_mp5" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouching_mp5" );
			}
		}
		else
		{
			if ( m_fStanding )
			{
				// get aimable sequence
				iSequence = LookupSequence( "standing_shotgun" );
			}
			else
			{
				// get crouching shoot
				iSequence = LookupSequence( "crouching_shotgun" );
			}
		}
#endif

		break;
	case ACT_RANGE_ATTACK2:
		// grunt is going to a secondary long range attack. This may be a thrown
		// grenade or fired grenade, we must determine which and pick proper sequence

#if FORCE_MP5 == 1

		//can launch hand grenade at least, I think?
		if(EASY_CVAR_GET(gruntsCanHaveMP5Grenade) == 1 ){
			if ( pev->weapons & HGRUNT_HANDGRENADE )
			{
				// get toss anim
				iSequence = LookupSequence( "throwgrenade" );
			}
			else
			{
				// get launch anim
				iSequence = LookupSequence( "launchgrenade" );
			}
		}else{
			if ( pev->weapons & HGRUNT_HANDGRENADE )
			{
				// get toss anim
				iSequence = LookupSequence( "throwgrenade" );
			}

		}

#else
		if ( pev->weapons & HGRUNT_HANDGRENADE )
		{
			// get toss anim
			iSequence = LookupSequence( "throwgrenade" );
		}
		else
		{
			// get launch anim
			iSequence = LookupSequence( "launchgrenade" );
		}

#endif

		break;
	case ACT_RUN:
		if ( pev->health <= HGRUNT_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivityHard ( ACT_RUN_HURT );
		}
		else
		{
			iSequence = LookupActivityHard ( activityToAnimateFor );
		}
		break;
	case ACT_WALK:
		if ( pev->health <= HGRUNT_LIMP_HEALTH )
		{
			// limp!
			iSequence = LookupActivityHard ( ACT_WALK_HURT );
		}
		else
		{
			iSequence = LookupActivityHard ( activityToAnimateFor );
		}
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
		{
			activityToAnimateFor = ACT_IDLE_ANGRY;
		}
		iSequence = LookupActivityHard ( activityToAnimateFor );
		break;
	default:

		//MODDD - use new system.
		//iSequence = LookupActivity ( activityToAnimateFor );
		iSequence = LookupActivityHard (activityToAnimateFor);

		break;
	}


	m_Activity = activityToAnimateFor; // Go ahead and set this so it doesn't keep trying when the anim is not present
	
	//why was this not here...?
	m_IdealActivity = m_Activity;


	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence > ACTIVITY_NOT_AVAILABLE )
	{
		if ( pev->sequence != iSequence || !m_fSequenceLoops )
		{
			pev->frame = 0;
		}


		//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("ANIMATION CHANGE!!!! D"));
		pev->sequence		= iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo( );
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT ( at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), activityToAnimateFor );
		pev->sequence		= 0;	// Set to the reset anim (if it's there)
	}
}

//=========================================================
// Get Schedule!
//=========================================================
Schedule_t *CHGrunt :: GetSchedule( void )
{


	//return CBaseMonster::GetSchedule();

	// clear old sentence
	m_iSentence = HGRUNT_SENT_NONE;

	// flying? If PRONE, barnacle has me. IF not, it's assumed I am rapelling.
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if (pev->flags & FL_ONGROUND)
		{
			// just landed
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType ( SCHED_GRUNT_REPEL_LAND );
		}
		else
		{
			// repel down a rope,
			if ( m_MonsterState == MONSTERSTATE_COMBAT )
				return GetScheduleOfType ( SCHED_GRUNT_REPEL_ATTACK );
			else
				return GetScheduleOfType ( SCHED_GRUNT_REPEL );
		}
	}


	//easyForcePrintLine("HGRUNT: HEAR???");
	// grunts place HIGH priority on running away from danger sounds.
	if ( HasConditions(bits_COND_HEAR_SOUND) )
	{
		//easyForcePrintLine("HGRUNT: YES");

		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound)
		{
			if (pSound->m_iType & bits_SOUND_DANGER)
			{
				//easyForcePrintLine("HGRUNT: FUK???");
				// dangerous sound nearby!

				//!!!KELLY - currently, this is the grunt's signal that a grenade has landed nearby,
				// and the grunt should find cover from the blast
				// good place for "SHIT!" or some other colorful verbal indicator of dismay.
				// It's not safe to play a verbal order here "Scatter", etc cause
				// this may only affect a single individual in a squad.

				if (FOkToSpeak())
				{
					SENTENCEG_PlayRndSz( ENT(pev), "HG_GREN", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
			}
			/*
			if (!HasConditions( bits_COND_SEE_ENEMY ) && ( pSound->m_iType & (bits_SOUND_PLAYER | bits_SOUND_COMBAT) ))
			{
				MakeIdealYaw( pSound->m_vecOrigin );
			}
			*/

			//MODDD - can listen for bait.
			
			//easyForcePrintLine("HGRUNT: TRUUU???");
			SCHEDULE_TYPE baitSched = getHeardBaitSoundSchedule(pSound);

			if(baitSched != SCHED_NONE){
				return GetScheduleOfType ( baitSched );
			}



		}
	}//END OF hear sound check


	switch	( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

// new enemy
			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				if ( InSquad() )
				{
					MySquadLeader()->m_fEnemyEluded = FALSE;

					if ( !IsLeader() )
					{
						return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
					}
					else
					{
						//!!!KELLY - the leader of a squad of grunts has just seen the player or a
						// monster and has made it the squad's enemy. You
						// can check pev->flags for FL_CLIENT to determine whether this is the player
						// or a monster. He's going to immediately start
						// firing, though. If you'd like, we can make an alternate "first sight"
						// schedule where the leader plays a handsign anim
						// that gives us enough time to hear a short sentence or spoken command
						// before he starts pluggin away.
						if (FOkToSpeak())// && RANDOM_LONG(0,1))
						{
							if ((m_hEnemy != NULL) && m_hEnemy->IsPlayer())
								// player
								SENTENCEG_PlayRndSz( ENT(pev), "HG_ALERT", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
							else if ((m_hEnemy != NULL) &&
									(m_hEnemy->Classify() != CLASS_PLAYER_ALLY) &&
									(m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE) &&
									(m_hEnemy->Classify() != CLASS_MACHINE))
								// monster
								SENTENCEG_PlayRndSz( ENT(pev), "HG_MONST", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);

							JustSpoke();
						}

						if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
						{
							return GetScheduleOfType ( SCHED_GRUNT_SUPPRESS );
						}
						else
						{
							return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
						}
					}
				}
			}
// no ammo
			else if ( HasConditions ( bits_COND_NO_AMMO_LOADED ) )
			{
				//!!!KELLY - this individual just realized he's out of bullet ammo.
				// He's going to try to find cover to run to and reload, but rarely, if
				// none is available, he'll drop and reload in the open here.
				return GetScheduleOfType ( SCHED_GRUNT_COVER_AND_RELOAD );
			}

			//MODDD - heavy damage must flinch.
			else if(HasConditions(bits_COND_HEAVY_DAMAGE)){
				return GetScheduleOfType(SCHED_BIG_FLINCH);
			}

// damaged just a little
			else if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
			{
				// if hurt:
				// 90% chance of taking cover
				// 10% chance of flinch.
				int iPercent = RANDOM_LONG(0,99);

				if ( iPercent <= 90 && m_hEnemy != NULL )
				{
					// only try to take cover if we actually have an enemy!

					//!!!KELLY - this grunt was hit and is going to run to cover.
					if (FOkToSpeak()) // && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_COVER", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = HGRUNT_SENT_COVER;
						//JustSpoke();
					}
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
				//MODDD - condition required before flinching (or rather, specifically forbidden).
				else if(!(EASY_CVAR_GET(noFlinchOnHard)==1 && g_iSkillLevel==SKILL_HARD))
				{
					return GetScheduleOfType( SCHED_SMALL_FLINCH );
				}
			}
// can kick
			else if ( HasConditions ( bits_COND_CAN_MELEE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_MELEE_ATTACK1 );
			}
// can grenade launch
#if FORCE_MP5 == 1
			//can't use mp5 grenades if FORCE_MP5 is on... unless the cvar "gruntsCanHaveMP5Grenade" says so.
			else if (EASY_CVAR_GET(gruntsCanHaveMP5Grenade) == 1 && FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER) && HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			{
				// shoot a grenade if you can
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
#else
			else if ( FBitSet( pev->weapons, HGRUNT_GRENADELAUNCHER) && HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			{
				// shoot a grenade if you can
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}


#endif

// can shoot
			else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				if ( InSquad() )
				{
					// if the enemy has eluded the squad and a squad member has just located the enemy
					// and the enemy does not see the squad member, issue a call to the squad to waste a
					// little time and give the player a chance to turn.
					if ( MySquadLeader()->m_fEnemyEluded && !HasConditions ( bits_COND_ENEMY_FACING_ME ) )
					{
						MySquadLeader()->m_fEnemyEluded = FALSE;
						return GetScheduleOfType ( SCHED_GRUNT_FOUND_ENEMY );
					}
				}

				if ( OccupySlot ( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					// try to take an available ENGAGE slot
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				else if ( HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					// throw a grenade if can and no engage slots are available
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else
				{
					// hide!
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
				}
			}
// can't see enemy
			else if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
			{
				if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					//!!!KELLY - this grunt is about to throw or fire a grenade at the player. Great place for "fire in the hole"  "frag out" etc
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "HG_THROW", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				else if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
				{
					//!!!KELLY - grunt cannot see the enemy and has just decided to
					// charge the enemy's position.
					if (FOkToSpeak())// && RANDOM_LONG(0,1))
					{
						//SENTENCEG_PlayRndSz( ENT(pev), "HG_CHARGE", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						m_iSentence = HGRUNT_SENT_CHARGE;
						//JustSpoke();
					}

					return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
				}
				else
				{
					//!!!KELLY - grunt is going to stay put for a couple seconds to see if
					// the enemy wanders back out into the open, or approaches the
					// grunt's covered position. Good place for a taunt, I guess?
					if (FOkToSpeak() && RANDOM_LONG(0,1))
					{
						SENTENCEG_PlayRndSz( ENT(pev), "HG_TAUNT", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return GetScheduleOfType( SCHED_STANDOFF );
				}
			}

			if(monsterID==1){
				easyForcePrintLine("I AM YEAH %d : %d", HasConditions( bits_COND_SEE_ENEMY ), HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) );
			}

			if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ) )
			{
				return GetScheduleOfType ( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
			}
		}
	}



	// no special cases here, call the base class
	Schedule_t* sched = CSquadMonster::GetSchedule();

	//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("HGRUNT: SCHED OUTSOURCED eeee!!! %s", sched ));

	return sched;
}

//=========================================================
//=========================================================
Schedule_t* CHGrunt :: GetScheduleOfType ( int Type )
{


	EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("HGRUNT%d GetSchedOfType: %d", monsterID, Type));

	if(monsterID == 1){
		int x = 4;
	}

	//MODDD - new schedule?  Forget the touch method.
	this->SetTouch(NULL);

	//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("HGRUNT%d GetScheduleOfType %d", monsterID, Type ));

	//MODDD - Maybe another time!
	//EASY_CVAR_PRINTIF_PRE(hgruntPrintout, easyForcePrintLine("GRUNT SCHED %d", Type));
	switch	( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		{
			if ( InSquad() )
			{
				//"CAN_RANGE_ATTACK2" is blocked if hgruntAllowGrenades is off.
				if ( g_iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				{
					if (FOkToSpeak())
					{
						SENTENCEG_PlayRndSz( ENT(pev), "HG_THROW", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch);
						JustSpoke();
					}
					return slGruntTossGrenadeCover;
				}
				else
				{
					return &slGruntTakeCover[ 0 ];
				}
			}
			else
			{
				//an AllowGrenades value of 0 guarantees just taking cover instead.
				if ( EASY_CVAR_GET(hgruntAllowGrenades) == 0 || RANDOM_LONG(0,1) )
				{
					return &slGruntTakeCover[ 0 ];
				}
				else
				{
					return &slGruntGrenadeCover[ 0 ];
				}
			}
		}
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		{
			return &slGruntTakeCoverFromBestSound[ 0 ];
		}
	case SCHED_GRUNT_TAKECOVER_FAILED:
		{
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			{
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
			}

			return GetScheduleOfType ( SCHED_FAIL );
		}
		break;
	case SCHED_GRUNT_ELOF_FAIL:
		{
			// human grunt is unable to move to a position that allows him to attack the enemy.
			return GetScheduleOfType ( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		break;
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		{
			return &slGruntEstablishLineOfFire[ 0 ];
		}
		break;
	case SCHED_RANGE_ATTACK1:
		{
			// randomly stand or crouch
			if (RANDOM_LONG(0,9) == 0)
				m_fStanding = RANDOM_LONG(0,1);

			if (m_fStanding)
				return &slGruntRangeAttack1B[ 0 ];
			else
				return &slGruntRangeAttack1A[ 0 ];
		}
	case SCHED_RANGE_ATTACK2:
		{
			return &slGruntRangeAttack2[ 0 ];
		}
	case SCHED_COMBAT_FACE:
		{
			return &slGruntCombatFace[ 0 ];
		}
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		{
			return &slGruntWaitInCover[ 0 ];
		}
	case SCHED_GRUNT_SWEEP:
		{
			return &slGruntSweep[ 0 ];
		}
	case SCHED_GRUNT_COVER_AND_RELOAD:
		{
			return &slGruntHideReload[ 0 ];
		}
	case SCHED_GRUNT_FOUND_ENEMY:
		{
			return &slGruntFoundEnemy[ 0 ];
		}
	case SCHED_VICTORY_DANCE:
		{
			if ( InSquad() )
			{
				if ( !IsLeader() )
				{
					return &slGruntFail[ 0 ];
				}
			}

			return &slGruntVictoryDance[ 0 ];
		}
	case SCHED_GRUNT_SUPPRESS:
		{
			if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
			{
				m_fFirstEncounter = FALSE;// after first encounter, leader won't issue handsigns anymore when he has a new enemy
				return &slGruntSignalSuppress[ 0 ];
			}
			else
			{
				return &slGruntSuppress[ 0 ];
			}
		}
	case SCHED_FAIL:
		{
			if ( m_hEnemy != NULL )
			{
				// grunt has an enemy, so pick a different default fail schedule most likely to help recover.
				return &slGruntCombatFail[ 0 ];
			}

			return &slGruntFail[ 0 ];
		}
	case SCHED_GRUNT_REPEL:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slGruntRepel[ 0 ];
		}
	case SCHED_GRUNT_REPEL_ATTACK:
		{
			if (pev->velocity.z > -128)
				pev->velocity.z -= 32;
			return &slGruntRepelAttack[ 0 ];
		}
	case SCHED_GRUNT_REPEL_LAND:
		{
			return &slGruntRepelLand[ 0 ];
		}
	default:
		{
			return CSquadMonster :: GetScheduleOfType ( Type );
		}
	}
}

//=========================================================
// CHGruntRepel - when triggered, spawns a monster_human_grunt
// repelling down a line.
//=========================================================

class CHGruntRepel : public CBaseMonster
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	int m_iSpriteTexture;	// Don't save, precache
};

#if REMOVE_ORIGINAL_NAMES != 1
	LINK_ENTITY_TO_CLASS( monster_grunt_repel, CHGruntRepel );
#endif

#if EXTRA_NAMES > 0
	LINK_ENTITY_TO_CLASS( hgrunt_repel, CHGruntRepel );

	#if EXTRA_NAMES == 2
		LINK_ENTITY_TO_CLASS( monster_human_grunt_repel, CHGruntRepel );
		LINK_ENTITY_TO_CLASS( monster_hgrunt_repel, CHGruntRepel );
		LINK_ENTITY_TO_CLASS( grunt_repel, CHGruntRepel );
		LINK_ENTITY_TO_CLASS( human_grunt_repel, CHGruntRepel );
	#endif

#endif


void CHGruntRepel::Spawn( void )
{
	Precache( );

	pev->classname = MAKE_STRING("monster_grunt_repel");

	pev->solid = SOLID_NOT;

	/////////////////////////////////////////////////////////////////////////
	//  just in case..? Wait how is all this other stuff missing? There never was even a blood set call in here.
	//  OH. This monster actually lasts the initial frame, despawns and creates a grunt that begins sliding down
	//    a rope in its place.
	//m_bloodColor = BloodColorRedFilter();
	/////////////////////////////////////////////////////////////////////////

	SetUse( &CHGruntRepel::RepelUse );
}

void CHGruntRepel::Precache( void )
{
	UTIL_PrecacheOther( "monster_human_grunt" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );
}

void CHGruntRepel::RepelUse ( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -4096.0), dont_ignore_monsters, ENT(pev), &tr);
	/*
	if ( tr.pHit && Instance( tr.pHit )->pev->solid != SOLID_BSP)
		return NULL;
	*/

	//MODDD - not good enough.  We need to intervene before the "Spawn" method of the newly created HGrunt gets called.
	//CBaseEntity *pEntity = Create( "monster_human_grunt", pev->origin, pev->angles );
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CBaseEntity *pEntity;
	//while(TRUE)
	//{
	//MODDD - have a method I made that does "Create" without the DispatchSpawn call.
	pEntity = CBaseEntity::CreateManual("monster_human_grunt", pev->origin, pev->angles, NULL);
	
	pEntity->MySquadMonsterPointer()->disableLeaderChange = TRUE;

	DispatchSpawn( pEntity->edict() );

	//break;
	//}//END OF while(true) ... just to be skippable at any point.
	//  ..wait, what was I thinking.  Why did I even make that while loop.    ah well.
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	CBaseMonster *pGrunt = pEntity->MyMonsterPointer( );

	pGrunt->pev->movetype = MOVETYPE_FLY;
	pGrunt->pev->velocity = Vector( 0, 0, RANDOM_FLOAT( -196, -128 ) );
	pGrunt->SetActivity( ACT_GLIDE );
	// UNDONE: position?
	pGrunt->m_vecLastPosition = tr.vecEndPos;

	CBeam *pBeam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
	pBeam->PointEntInit( pev->origin + Vector(0,0,112), pGrunt->entindex() );
	pBeam->SetFlags( BEAM_FSOLID );
	pBeam->SetColor( 255, 255, 255 );
	pBeam->SetThink( &CBaseEntity::SUB_Remove );
	pBeam->pev->nextthink = gpGlobals->time + -4096.0 * tr.flFraction / pGrunt->pev->velocity.z + 0.5;

	UTIL_Remove( this );
}


//=========================================================
// DEAD HGRUNT PROP
//=========================================================
class CDeadHGrunt : public CBaseMonster
{
public:
	void Spawn( void );
	int Classify ( void ) { return	CLASS_HUMAN_MILITARY; }
	BOOL isOrganic(void){return !CanUseGermanModel();}
	
	BOOL getGermanModelRequirement(void);
	const char* getGermanModel(void);
	const char* getNormalModel(void);


	void KeyValue( KeyValueData *pkvd );

	int m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[3];
};

char *CDeadHGrunt::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

void CDeadHGrunt::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}


#if REMOVE_ORIGINAL_NAMES != 1
	LINK_ENTITY_TO_CLASS( monster_hgrunt_dead, CDeadHGrunt );
#endif

#if EXTRA_NAMES > 0
	LINK_ENTITY_TO_CLASS( hgrunt_dead, CDeadHGrunt );

	#if EXTRA_NAMES == 2
		LINK_ENTITY_TO_CLASS( monster_human_grunt_dead, CDeadHGrunt );
		LINK_ENTITY_TO_CLASS( human_grunt_dead, CDeadHGrunt );
	#endif

#endif


//=========================================================
// ********** DeadHGrunt SPAWN **********
//=========================================================
void CDeadHGrunt :: Spawn( void )
{
	PRECACHE_MODEL("models/hgrunt.mdl");
	setModel(); //"models/hgrunt.mdl"  argument unused when there's a german model equivalent.

	pev->classname = MAKE_STRING("monster_hgrunt_dead");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor = BloodColorRedFilter();

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );

	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead hgrunt with bad pose\n" );
	}

	// Corpses have less health
	pev->health			= 8;

#if FORCE_ONE_HEAD == 1
	//easyPrint("!!!!!!!!!!! hgrunt CORPSE pev->body: %d\n", pev->body);

	//NOTICE: all use "HEAD_GRUNT" now.
	switch( pev->body )
		{
		case 0: // Grunt with Gun
			pev->body = 0;
			pev->skin = 0;
			SetBodygroup( HEAD_GROUP, HEAD_GRUNT );
			SetBodygroup( GUN_GROUP, GUN_MP5 );
			break;
		case 1: // Commander with Gun
			pev->body = 0;
			pev->skin = 0;
			SetBodygroup( HEAD_GROUP, HEAD_GRUNT );
			SetBodygroup( GUN_GROUP, GUN_MP5 );
			break;
		case 2: // Grunt no Gun
			pev->body = 0;
			pev->skin = 0;
			SetBodygroup( HEAD_GROUP, HEAD_GRUNT );
			SetBodygroup( GUN_GROUP, GUN_NONE );
			break;
		case 3: // Commander no Gun
			pev->body = 0;
			pev->skin = 0;
			SetBodygroup( HEAD_GROUP, HEAD_GRUNT );
			SetBodygroup( GUN_GROUP, GUN_NONE );
			break;
		}

#else
	// map old bodies onto new bodies
		switch( pev->body )
		{
		case 0: // Grunt with Gun
			pev->body = 0;
			pev->skin = 0;
			SetBodygroup( HEAD_GROUP, HEAD_GRUNT );
			SetBodygroup( GUN_GROUP, GUN_MP5 );
			break;
		case 1: // Commander with Gun
			pev->body = 0;
			pev->skin = 0;
			SetBodygroup( HEAD_GROUP, HEAD_COMMANDER );
			SetBodygroup( GUN_GROUP, GUN_MP5 );
			break;
		case 2: // Grunt no Gun
			pev->body = 0;
			pev->skin = 0;
			SetBodygroup( HEAD_GROUP, HEAD_GRUNT );
			SetBodygroup( GUN_GROUP, GUN_NONE );
			break;
		case 3: // Commander no Gun
			pev->body = 0;
			pev->skin = 0;
			SetBodygroup( HEAD_GROUP, HEAD_COMMANDER );
			SetBodygroup( GUN_GROUP, GUN_NONE );
			break;
		}
#endif


	MonsterInitDead();

	if(isOrganicLogic()){
		//MODDD - emit a stench that eaters will pick up.
		CSoundEnt::InsertSound ( bits_SOUND_CARCASS, pev->origin, 384, SOUND_NEVER_EXPIRE );
	}
}



BOOL CDeadHGrunt::getGermanModelRequirement(void){
	return globalPSEUDO_germanModel_hgruntFound;
}
const char* CDeadHGrunt::getGermanModel(void){
	return "models/g_hgrunt.mdl";
}
const char* CDeadHGrunt::getNormalModel(void){
	return "models/hgrunt.mdl";
}


void CHGrunt::onNewRouteNode(void){
	//stop strafing if we were.

	if(m_IdealActivity == m_movementActivity && (m_movementActivity == ACT_STRAFE_LEFT || m_movementActivity == ACT_STRAFE_RIGHT) ){
		m_movementActivity = ACT_RUN;
		strafeMode = -1;
		idealStrafeMode = -1;
	}
}



//MODDD - yes.
BOOL CHGrunt::canResetBlend0(){
	return TRUE;
}

BOOL CHGrunt::onResetBlend0(void){

	lookAtEnemy_pitch();
	return TRUE;

	/*
	//easyForcePrintLine("HOW IT GO   %d", (m_hEnemy!=NULL));
	if (m_hEnemy == NULL)
	{
		return FALSE;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemyMod( vecShootOrigin );

	//UTIL_MakeVectors ( pev->angles );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	//easyForcePrintLine("ANG:::: %.2f", angDir.x);
	
	SetBlending( 0, angDir.x );

	return TRUE;
	*/
}


void CHGrunt::checkHeadGore( ){
	CHGrunt::checkHeadGore(GIB_ALWAYS);  //just a cheap way to make it draw decal blood on impact (at least not block it).
}
void CHGrunt::checkHeadGore(int iGib ){
	
	BOOL gibsSpawnDecals = !(iGib == GIB_ALWAYS_NODECAL);

	if(m_LastHitGroup == HITGROUP_HEAD || m_LastHitGroup == 11){
		//Took enough damage on last trace-hit?
		if(missingHeadPossible && EASY_CVAR_GET(sv_germancensorship) != 1 && GetBodygroup(1) != 1){
			//missing head!... if german censorship isn't on.
			//that is, heads --> Headless
			SetBodygroup( 1, 1 );

			//FOUNTAIN O BLOOD
			UTIL_BloodStream(lastHeadHit, UTIL_RandomBloodVector(), BloodColor(), RANDOM_LONG(70, 80) );

			//EMIT_SOUND_FILTERED( ENT(pev), CHAN_VOICE, "!SC_SCREAM_TRU0", 1, ATTN_IDLE );
			//playedSoundAlready = TRUE;

			CGib::SpawnHeadGib(this->pev, gibsSpawnDecals);
		}

	}
}//END OF checkHeadGore()
