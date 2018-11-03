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

#ifndef BASEMONSTER_H
#define BASEMONSTER_H




#include "cbase.h"

#include "basetoggle.h"

//Moved to here from monsters.h.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "skill.h"



// CHECKLOCALMOVE result types 
#define	LOCALMOVE_INVALID					0 // move is not possible
#define LOCALMOVE_INVALID_DONT_TRIANGULATE	1 // move is not possible, don't try to triangulate
#define LOCALMOVE_VALID						2 // move is possible

// Hit Group standards
#define	HITGROUP_GENERIC	0
#define	HITGROUP_HEAD		1
#define	HITGROUP_CHEST		2
#define	HITGROUP_STOMACH	3
#define HITGROUP_LEFTARM	4	
#define HITGROUP_RIGHTARM	5
#define HITGROUP_LEFTLEG	6
#define HITGROUP_RIGHTLEG	7


// Monster Spawnflags
#define	SF_MONSTER_WAIT_TILL_SEEN		1// spawnflag that makes monsters wait until player can see them before attacking.
#define	SF_MONSTER_GAG					2 // no idle noises from this monster
#define SF_MONSTER_HITMONSTERCLIP		4


//FLAG OVERVIEW:

//2^0 = 1:	NO, waitTillSeen
//2^1 = 2:	NO, gag
//2^3 = 8:	OPEN!
//2^4 = 16:	NO, PRISONER
//2^5 = 32: NO, squadleader (for non-squad monsters, perhaps open, but also used by turrets)
//2^6 = 64: CAREFUL, BEST NOT
//2^7, 8, 9:	NO
//2^10 = 1024: OPEN!
//2^11 = 2048: possibly no.
//beyond?  test to be certain...   some readings, if in the "int" rage of 2^0 - 2^31 (inclusive), are still cut high-wards (that is, just plain not sent / transferred to here for seeing)

//in short: 2^3 = 8 and 2^10 = 1024 are the best spots for custom flags.







//MODDD - "8" is now for a possible unique entity-only flag.  OR, this is specialty 2.0:
//										8
#define SF_MONSTER_APACHE_CINBOUNDS		1024
//whether the stuka has a strong preference for the ground (off) or the ceiling (on), snapping to its preferred type if close enough.  Will hover in place if not close enough to either.
#define SF_MONSTER_STUKA_ONGROUND		8
//whether a scientist (only, yet) spawns with the dirty skin (alpha models) or not.  off = no, on = yes.
#define SF_MONSTER_TALKMONSTER_BLOODY	8
//causes an HGrunt to not allow itself to be replaced with an HAssault (commando version) at map / level start.
//By default, one hgrunt in a squad is selected to be replaced with an HAssault if no other HAssault is found at mapstart.
#define SF_HGRUNT_DISALLOW_PROMOTION 8
//Used to tell a spawned weapon (snark, chumtoad) not to replace itself with the walkable version. This is also to stop the version spawned by a walkable from wanting to spawn.
#define SF_PICKUP_NOREPLACE 8
//This monster was spawned by a player using a throwable weapon, such as snarks or chumtoads.
#define SF_MONSTER_THROWN 1024
//For tripmines, same bit is unused there.
#define SF_MONSTER_DYNAMIC 1024




//ALSO: it appears this is consistently set for entities spawned by the player (or anything spawned outside of map-start, maybe?).  Marking, not sure why it wasn't before.
#define SF_MONSTER_DYNAMICSPAWN			(1<<30)
//NOTICE!!!!!! THIS PLACE,  (1<<30), IS SHARED BY AN EXISTING FLAG IN cbase.h:
//#define	SF_NORESPAWN	( 1 << 30 )// !!!set this bit on guns and stuff that should never respawn.


//NOTE: the spot "32" (2^5) can be occupied by squadmonsters to mean "squadleader" (see "SF_SQUADMONSTER_LEADER" of "squadmonster.cpp").
///////////////////////////////////////////////////////////////////////////////////////


#define SF_MONSTER_PRISONER				16 // monster won't attack anyone, no one will attacke him.
//										32
//										64
#define	SF_MONSTER_WAIT_FOR_SCRIPT		128 //spawnflag that makes monsters wait to check for attacking until the script is done or they've been attacked
#define SF_MONSTER_PREDISASTER			256	//this is a predisaster scientist or barney. Influences how they speak.
#define SF_MONSTER_FADECORPSE			512 // Fade out corpse after death
#define SF_MONSTER_FALL_TO_GROUND		0x80000000    //This flag does the opposite of what it says. It's presence STOPS a monster from snapping to the ground at creation. WHat?
                                                      //A more accurate name would be "SF_MONSTER_DONT_SNAP_TO_GROUND". By default they do (snap to ground).
													  //^AKA, 31st power of 2 (1<<31)

// specialty spawnflags
#define SF_MONSTER_TURRET_AUTOACTIVATE	32
#define SF_MONSTER_TURRET_STARTINACTIVE	64
#define SF_MONSTER_WAIT_UNTIL_PROVOKED	64 // don't attack the player unless provoked



// MoveToOrigin stuff
#define		MOVE_START_TURN_DIST	64 // when this far away from moveGoal, start turning to face next goal
#define		MOVE_STUCK_DIST			32 // if a monster can't step this far, it is stuck.


// MoveToOrigin stuff
#define		MOVE_NORMAL				0// normal move in the direction monster is facing
#define		MOVE_STRAFE				1// moves in direction specified, no matter which way monster is facing

// spawn flags 256 and above are already taken by the engine
extern void UTIL_MoveToOrigin( edict_t* pent, const Vector &vecGoal, float flDist, int iMoveType ); 

Vector VecCheckToss ( entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float flGravityAdj = 1.0 );
Vector VecCheckThrow ( entvars_t *pev, const Vector &vecSpot1, Vector vecSpot2, float flSpeed, float flGravityAdj = 1.0 );
extern DLL_GLOBAL Vector		g_vecAttackDir;
extern DLL_GLOBAL CONSTANT float g_flMeleeRange;
extern DLL_GLOBAL CONSTANT float g_flMediumRange;
extern DLL_GLOBAL CONSTANT float g_flLongRange;
extern void EjectBrass (const Vector &vecOrigin, const Vector &vecVelocity, float rotation, int model, int soundtype );
extern void ExplodeModel( const Vector &vecOrigin, float speed, int model, int count );

BOOL FBoxVisible ( entvars_t *pevLooker, entvars_t *pevTarget );
BOOL FBoxVisible ( entvars_t *pevLooker, entvars_t *pevTarget, Vector &vecTargetOrigin, float flSize = 0.0 );





//NOTICE - "monster to monster" relationship constnats moved to cbase.h because base entities have some relationship-related methods.

// these bits represent the monster's memory
#define MEMORY_CLEAR					0
#define bits_MEMORY_PROVOKED			( 1 << 0 )// right now only used for houndeyes.
#define bits_MEMORY_INCOVER				( 1 << 1 )// monster knows it is in a covered position.
#define bits_MEMORY_SUSPICIOUS			( 1 << 2 )// Ally is suspicious of the player, and will move to provoked more easily
#define bits_MEMORY_PATH_FINISHED		( 1 << 3 )// Finished monster path (just used by big momma for now)
#define bits_MEMORY_ON_PATH				( 1 << 4 )// Moving on a path
#define bits_MEMORY_MOVE_FAILED			( 1 << 5 )// Movement has already failed
#define bits_MEMORY_FLINCHED			( 1 << 6 )// Has already flinched
#define bits_MEMORY_KILLED				( 1 << 7 )// HACKHACK -- remember that I've already called my Killed()

//#define bits_MEMORY_COVER_RECENTFAIL	( 1 << 8) //MODDD - cut for now. New memory flag to signify that a recent request for cover has failed and not to try that again.

#define bits_MEMORY_CUSTOM4				( 1 << 28 )	// Monster-specific memory
#define bits_MEMORY_CUSTOM3				( 1 << 29 )	// Monster-specific memory
#define bits_MEMORY_CUSTOM2				( 1 << 30 )	// Monster-specific memory
#define bits_MEMORY_CUSTOM1				( 1 << 31 )	// Monster-specific memory




#define RANDOMWANDER_TRIES 4






// trigger conditions for scripted AI
// these MUST match the CHOICES interface in halflife.fgd for the base monster
enum 
{
	AITRIGGER_NONE = 0,
	AITRIGGER_SEEPLAYER_ANGRY_AT_PLAYER,
	AITRIGGER_TAKEDAMAGE,
	AITRIGGER_HALFHEALTH,
	AITRIGGER_DEATH,
	AITRIGGER_SQUADMEMBERDIE,
	AITRIGGER_SQUADLEADERDIE,
	AITRIGGER_HEARWORLD,
	AITRIGGER_HEARPLAYER,
	AITRIGGER_HEARCOMBAT,
	AITRIGGER_SEEPLAYER_UNCONDITIONAL,
	AITRIGGER_SEEPLAYER_NOT_IN_COMBAT,
};
/*
		0 : "No Trigger"
		1 : "See Player"
		2 : "Take Damage"
		3 : "50% Health Remaining"
		4 : "Death"
		5 : "Squad Member Dead"
		6 : "Squad Leader Dead"
		7 : "Hear World"
		8 : "Hear Player"
		9 : "Hear Combat"
*/


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////









//
// generic Monster
//
class CBaseMonster : public CBaseToggle
{
private:
		int					m_afConditions;

		//MODDD - new, for the m_afConditionsFrame instead. This var is similar to m_afConditions but retains conditions throughout one frame of gamelogic.
		//        That is, schedule changes don't reset this. But the starting "RunTask" call in monsterstate.cpp "MaintainSchedule" resets this.
		int m_afConditionsFrame;

public:
		typedef enum
		{
			SCRIPT_PLAYING = 0,		// Playing the sequence
			SCRIPT_WAIT,				// Waiting on everyone in the script to be ready
			SCRIPT_CLEANUP,					// Cancelling the script / cleaning up
			SCRIPT_WALK_TO_MARK,
			SCRIPT_RUN_TO_MARK,
		} SCRIPTSTATE;


		//new
		BOOL disableEnemyAutoNode;

		
		//MODDD - new
		CBaseMonster(void);
		virtual BOOL usesSoundSentenceSave(void);
		BOOL canSetAnim;
		BOOL m_fNewScheduleThisFrame;

		BOOL canDrawDebugSurface;

		static int monsterIDLatest;
		int monsterID;

		Vector m_vecEnemyLKP_prev;

		BOOL signalActivityUpdate;
		BOOL forceFlyInterpretation;

		//void smartResize();
		virtual BOOL getHasPathFindingMod();
		virtual BOOL getHasPathFindingModA();
		//this is okay to be virtual, yes?
		virtual BOOL NoFriendlyFireImp(const Vector& startVec, const Vector& endVec);
		virtual BOOL forceIdleFrameReset(void);

		virtual void onNewRouteNode(void);
		
		virtual void setPhysicalHitboxForDeath(void);
		virtual void DeathAnimationStart(void);
		virtual void DeathAnimationEnd(void);
		virtual void onDeathAnimationEnd(void);

		virtual BOOL isOrganic(void);
		virtual int LookupActivityFiltered(int NewAcitivty);
		virtual int LookupActivity(int NewActivity);
		virtual int LookupActivityHeaviest(int NewActivity);

		virtual void OnTakeDamageSetConditions(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType, int bitsDamageTypeMod);

		//MODDD - okay
		Vector debugVector1;
		Vector debugVector2;
		Vector debugVector3;
		Vector debugVector4;
		BOOL debugVectorsSet;
		BOOL debugFailColor;
		BOOL debugVectorPrePass;
		int debugVectorMode;

		BOOL forceNoDrop;

		BOOL fApplyTempVelocity;
		Vector velocityApplyTemp;

		BOOL drawPathConstant;
		BOOL drawFieldOfVisionConstant;




		//MODDD - new, pertaining to "takeDamage".
		BOOL blockDamage;
		BOOL buddhaMode;
		BOOL blockTimedDamage;

		//MODDD - new, to block state changes when pulled up.
		//possible glitch that enemies just stay stuck in a standing animation while pulled up?  This may solve that.
		BOOL barnacleLocked;
		MONSTERSTATE		queuedMonsterState;// monster's current state



		BOOL targetIsDeadException;

		BOOL canRangedAttack1;
		BOOL canRangedAttack2;
		


		virtual void setAnimationSmart(const char* arg_animName);
		virtual void setAnimationSmart(const char* arg_animName, float arg_frameRate);
		virtual void setAnimationSmart(int arg_animIndex, float arg_frameRate);
		virtual void setAnimationSmartAndStop(const char* arg_animName);
		virtual void setAnimationSmartAndStop(const char* arg_animName, float arg_frameRate);
		virtual void setAnimationSmartAndStop(int arg_animIndex, float arg_frameRate);
		
		virtual BOOL usesAdvancedAnimSystem(void);

		void setAnimation(char* animationName);
		void setAnimation(char* animationName, BOOL forceException);
		void setAnimation(char* animationName, BOOL forceException, BOOL forceLoopsProperty);
		void setAnimation(char* animationName, BOOL forceException, BOOL forceLoopsProperty, int extraLogic);

		virtual BOOL isSizeGiant(void);
		virtual float getBarnaclePulledTopOffset(void);

		virtual float getBarnacleForwardOffset(void);
		virtual float getBarnacleAnimationFactor(void);





	
		// these fields have been added in the process of reworking the state machine. (sjb)
		EHANDLE				m_hEnemy;		 // the entity that the monster is fighting.
		EHANDLE				m_hTargetEnt;	 // the entity that the monster is trying to reach
		
		EHANDLE				m_hOldEnemy[ MAX_OLD_ENEMIES ];
		Vector				m_vecOldEnemy[ MAX_OLD_ENEMIES ];
		//It shall use the stack!
		//Use this variable to record the most recent addition to take instead.
		int m_intOldEnemyNextIndex;

		



		float				m_flFieldOfView;// width of monster's field of view ( dot product )
		float				m_flWaitFinished;// if we're told to wait, this is the time that the wait will be over.
		float				m_flMoveWaitFinished;

		Activity			m_Activity;// what the monster is doing (animation)
		Activity			m_IdealActivity;// monster should switch to this activity
		
		int					m_LastHitGroup; // the last body region that took damage
		
		MONSTERSTATE		m_MonsterState;// monster's current state
		MONSTERSTATE		m_IdealMonsterState;// monster should change to this state
	
		int					m_iTaskStatus;
		Schedule_t			*m_pSchedule;
		int					m_iScheduleIndex;

		WayPoint_t			m_Route[ ROUTE_SIZE ];	// Positions of movement
		int					m_movementGoal;			// Goal that defines route
		int					m_iRouteIndex;			// index into m_Route[]
		float				m_moveWaitTime;			// How long I should wait for something to move

		Vector				m_vecMoveGoal; // kept around for node graph moves, so we know our ultimate goal
		Activity			m_movementActivity;	// When moving, set this activity

		int					m_iAudibleList; // first index of a linked list of sounds that the monster can hear.
		int					m_afSoundTypes;

		Vector				m_vecLastPosition;// monster sometimes wants to return to where it started after an operation.

		int					m_iHintNode; // this is the hint node that the monster is moving towards or performing active idle on.

		int					m_afMemory;

		int					m_iMaxHealth;// keeps track of monster's maximum health value (for re-healing, etc)

	Vector				m_vecEnemyLKP;// last known position of enemy. (enemy's origin)

	int					m_cAmmoLoaded;		// how much ammo is in the weapon (used to trigger reload anim sequences)

	int					m_afCapability;// tells us what a monster can/can't do.

	float				m_flNextAttack;		// cannot attack again until this time

	int					m_bitsDamageType;	// what types of damage has monster (player) taken
	//MODDD - complimentary.
	int					m_bitsDamageTypeMod;

	//MODDD - new.
	////////////////////////////////////////////////////////////////////////////////////////////
	BOOL				m_rgbTimeBasedFirstFrame[CDMG_TIMEBASED];
	float				m_tbdPrev;				// Time-based damage timer

	//MODDD - canned.
	//BOOL usingGermanModel;

	//MODDD - new var
	virtual BOOL hasSeeEnemyFix(void);
	virtual BOOL getForceAllowNewEnemy(CBaseEntity* pOther);

	virtual void tempMethod(void);

	virtual BOOL needsMovementBoundFix(void);
	virtual void cheapKilled(void);
	virtual void cheapKilledFlier(void);
	virtual void OnKilledSetTouch(void);
	virtual int getLoopingDeathSequence(void);
	static Schedule_t* flierDeathSchedule(void);
	virtual BOOL getMovementCanAutoTurn(void);

	virtual BOOL getGermanModelRequirement(void);
	virtual const char* getGermanModel(void);
	virtual const char* getNormalModel(void);
	virtual void Activate(void);
	virtual void Spawn(void);


	//MODDD
	virtual BOOL skipSpawnStuckCheck(void){return FALSE;};

	

	//pastable:
	/*
	virtual const char* getGermanModel();
	virtual const char* getNormalModel();
	
	const char* CDeadScientist::getGermanModel(){
		return "models/g_scientist.mdl";
	}
	const char* CDeadScientist::getNormalModel(){
		return "models/scientist.mdl";
	}
	*/





	//int		Save( CSave &save );
	//int		Restore( CRestore &restore );
	//static	TYPEDESCRIPTION m_SaveData[];

	
#define PARALYZE_DURATION	30		// number of 2 second intervals to take damage
#define PARALYZE_DAMAGE		0.0		// damage to take each 2 second interval

#define NERVEGAS_DURATION	16
#define NERVEGAS_DAMAGE		5.0

#define POISON_DURATION		25
#define POISON_DAMAGE		2.0

#define RADIATION_DURATION	50
#define RADIATION_DAMAGE	1.0

#define ACID_DURATION		10
#define ACID_DAMAGE			5.0

#define SLOWBURN_DURATION	2
#define SLOWBURN_DAMAGE		1.0

#define SLOWFREEZE_DURATION	1.0
#define SLOWFREEZE_DAMAGE	3.0





#define PARALYZE_DURATION_EASY	21
#define PARALYZE_DURATION_MEDIUM	30
#define PARALYZE_DURATION_HARD	39
#define PARALYZE_DAMAGE		0.0

#define NERVEGAS_DURATION_EASY	12
#define NERVEGAS_DURATION_MEDIUM	16
#define NERVEGAS_DURATION_HARD	20
#define NERVEGAS_DAMAGE		5.0

#define POISON_DURATION_EASY		18
#define POISON_DURATION_MEDIUM		25
#define POISON_DURATION_HARD		32
#define POISON_DAMAGE		2.0

#define RADIATION_DURATION_EASY	35
#define RADIATION_DURATION_MEDIUM	50
#define RADIATION_DURATION_HARD	65
#define RADIATION_DAMAGE	1.0

#define ACID_DURATION_EASY		7
#define ACID_DURATION_MEDIUM		10
#define ACID_DURATION_HARD		13
#define ACID_DAMAGE			5.0

#define SLOWBURN_DURATION_EASY	1
#define SLOWBURN_DURATION_MEDIUM	2
#define SLOWBURN_DURATION_HARD	3
#define SLOWBURN_DAMAGE		1.0

#define SLOWFREEZE_DURATION_EASY	1.0
#define SLOWFREEZE_DURATION_MEDIUM	1.0
#define SLOWFREEZE_DURATION_HARD	2.0
#define SLOWFREEZE_DAMAGE	3.0

#define BLEEDING_DURATION_EASY	11.0
#define BLEEDING_DURATION_MEDIUM	15.0
#define BLEEDING_DURATION_HARD	19.0
#define BLEEDING_DAMAGE 1.0



	

	static float paralyzeDuration;
	static float nervegasDuration;
	static float poisonDuration;
	static float radiationDuration;
	static float acidDuration;
	static float slowburnDuration;
	static float slowfreezeDuration;
	static float bleedingDuration;



	//CBaseMonster();
	
	int convert_itbd_to_damage(int i);
	virtual void CheckTimeBasedDamage(void);
	//void PreThink(void);
	//virtual void Think(void);

	void attemptResetTimedDamage(BOOL forceReset);
	

	////////////////////////////////////////////////////////////////////////////////////////////

	

	BYTE				m_rgbTimeBasedDamage[CDMG_TIMEBASED];

	int					m_lastDamageAmount;// how much damage did monster (player) last take
											// time based damage counters, decr. 1 per 2 seconds
	int					m_bloodColor;		// color of blood particless

	int					m_failSchedule;				// Schedule type to choose if current schedule fails

	float				m_flHungryTime;// set this is a future time to stop the monster from eating for a while. 

	float				m_flDistTooFar;	// if enemy farther away than this, bits_COND_ENEMY_TOOFAR set in CheckEnemy
	float				m_flDistLook;	// distance monster sees (Default 2048)

	int					m_iTriggerCondition;// for scripted AI, this is the condition that will cause the activation of the monster's TriggerTarget
	string_t			m_iszTriggerTarget;// name of target that should be fired. 

	Vector				m_HackedGunPos;	// HACK until we can query end of gun

// Scripted sequence Info
	SCRIPTSTATE			m_scriptState;		// internal cinematic state
	CCineMonster		*m_pCine;

	virtual int		Save( CSave &save ); 
	virtual int		Restore( CRestore &restore );

	//MODDD - new
	void PostRestore(void);
	
	static	TYPEDESCRIPTION m_SaveData[];

	void KeyValue( KeyValueData *pkvd );

// monster use function
	void EXPORT			MonsterUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void EXPORT			CorpseUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

// overrideable Monster member functions
	
	virtual int	 BloodColor( void ) { return m_bloodColor; }

	virtual CBaseMonster *MyMonsterPointer( void ) { return this; }
	virtual void Look ( int iDistance );// basic sight function for monsters
	virtual void RunAI ( void );// core ai function!	
	void Listen ( void );


	//MODDD - changed, made more strict. Any problems with this or scripted ent's? Check Nihilanth.
	//virtual BOOL	IsAlive( void ) { return (pev->deadflag != DEAD_DEAD); }
	virtual BOOL	IsAlive( void ) { return (pev->deadflag == DEAD_NO); }

	//MODDD NEW - alternate version for special cases. See notes on this in cbase.h .
	virtual BOOL	IsAlive_FromAI( CBaseMonster* whoWantsToKnow ) { return (pev->deadflag == DEAD_NO || (pev->deadflag == DEAD_DYING && !recognizablyDead ) ) && pev->health > 0; }

	virtual BOOL	ShouldFadeOnDeath( void );

// Basic Monster AI functions
	virtual float ChangeYaw ( int speed );
	float VecToYaw( Vector vecDir );
	float FlYawDiff ( void ); 

	float DamageForce( float damage );




// stuff written for new state machine
//MODDD NOTE - ...The meaning of the above comment shall forever be lost to time.
	virtual void MonsterThink( void );
	virtual void MonsterThinkPreMOD( void );

	virtual void heardBulletHit(entvars_t* pevShooter);
	virtual void wanderAway(const Vector& toWalkAwayFrom);
	

	//MODDD - send through a filter first, it will redirect to "MonsterThink" after.  Think of it as an injection for testing.
	//REVERTED!
	void EXPORT	CallMonsterThink( void ) { this->MonsterThink(); }
	//void EXPORT	CallMonsterThink( void ) { this->MonsterThinkPreMOD(); }



	//NOTICE: beware of IRelationship's implementation in dlls/mpstubb.cpp if changing. By default it is identical to monster.cpp's implementation
	virtual int IRelationship ( CBaseEntity *pTarget );


	virtual void MonsterInit ( void );
	virtual void MonsterInitDead( void );	// Call after animation/pose is set up
	virtual void BecomeDead( void );
	void EXPORT CorpseFallThink( void );

	void EXPORT MonsterInitThink ( void );
	virtual void StartMonster ( void );
	virtual CBaseEntity* BestVisibleEnemy ( void );// finds best visible enemy for attack
	virtual BOOL FInViewCone ( CBaseEntity *pEntity );// see if pEntity is in monster's view cone
	virtual BOOL FInViewCone ( Vector *pOrigin );// see if given location is in monster's view cone
	virtual void HandleAnimEvent( MonsterEvent_t *pEvent );


	virtual int CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist );// check validity of a straight move through space
	
	virtual int CheckLocalMoveHull ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist );// check validity of a straight move through space
	
	
	virtual void Move( float flInterval = 0.1 );

	virtual void MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );
	virtual BOOL ShouldAdvanceRoute( float flWaypointDist, float flInterval );

	virtual Activity GetStoppedActivity( void ) { return ACT_IDLE; }
	virtual void Stop( void ) { m_IdealActivity = GetStoppedActivity(); }

	// This will stop animation until you call ResetSequenceInfo() at some point in the future
	inline void StopAnimation( void ) { pev->framerate = 0; }

	// these functions will survey conditions and set appropriate conditions bits for attack types.
	virtual BOOL CheckRangeAttack1( float flDot, float flDist );
	virtual BOOL CheckRangeAttack2( float flDot, float flDist );
	virtual BOOL CheckMeleeAttack1( float flDot, float flDist );
	virtual BOOL CheckMeleeAttack2( float flDot, float flDist );


	

	BOOL FHaveSchedule( void );
	BOOL FScheduleValid ( void );
	void ClearSchedule( void );
	BOOL FScheduleDone ( void );
	void ChangeSchedule ( Schedule_t *pNewSchedule );
	void NextScheduledTask ( void );
	Schedule_t *ScheduleInList( const char *pName, Schedule_t **pList, int listCount );

	virtual Schedule_t *ScheduleFromName( const char *pName );
	static Schedule_t *m_scheduleList[];

	void MaintainSchedule ( void );
	virtual void StartTask ( Task_t *pTask );
	virtual void RunTask ( Task_t *pTask );
	virtual Schedule_t *GetScheduleOfType( int Type );
	virtual Schedule_t *GetSchedule( void );
	
	virtual void ScheduleChange( void ); //MODDD - a little default behavior now, see defaultai.cpp.
	virtual Schedule_t* GetStumpedWaitSchedule(void);
	// virtual int CanPlaySequence( void ) { return ((m_pCine == NULL) && (m_MonsterState == MONSTERSTATE_NONE || m_MonsterState == MONSTERSTATE_IDLE || m_IdealMonsterState == MONSTERSTATE_IDLE)); }
	virtual int CanPlaySequence( BOOL fDisregardState, int interruptLevel );
	virtual int CanPlaySentence( BOOL fDisregardState ) { return IsAlive(); }
	virtual void PlaySentence( const char *pszSentence, float duration, float volume, float attenuation );
	virtual void PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener );

	virtual void SentenceStop( void );


	Task_t *GetTask ( void );
	virtual MONSTERSTATE GetIdealState ( void );
	



	BOOL deadSetActivityBlock;

	//MODDD 
	int getTaskNumber(void);
	const char* getScheduleName(void);

	virtual void SetActivity ( Activity NewActivity );
	//MODDD
	//virtual void SetActivity ( Activity NewActivity, BOOL forceReset );
	

	virtual BOOL allowedToSetActivity(void);
	BOOL tryGetTaskID(void);
	const char* tryGetScheduleName(void);

	//MODDD - edited.
	void SetSequenceByIndex(int iSequence);
	void SetSequenceByName(char* szSequence);
	void SetSequenceByIndex(int iSequence, float flFramerateMulti);
	void SetSequenceByName(char* szSequence, float flFramerateMulti);
	void SetSequenceByIndex(int iSequence, BOOL safeReset);
	void SetSequenceByName(char* szSequence, BOOL safeReset);
	void SetSequenceByIndex(int iSequence, float flFramerateMulti, BOOL safeReset);
	void SetSequenceByName(char* szSequence, float flFramerateMulti, BOOL safeReset);

	void SetSequenceByIndexForceLoops(int iSequence, BOOL forceLoops);
	void SetSequenceByNameForceLoops(char* szSequence, BOOL forceLoops);
	void SetSequenceByIndexForceLoops(int iSequence, float flFramerateMulti, BOOL forceLoops);
	void SetSequenceByNameForceLoops(char* szSequence, float flFramerateMulti, BOOL forceLoops);
	void SetSequenceByIndexForceLoops(int iSequence, BOOL safeReset, BOOL forceLoops);
	void SetSequenceByNameForceLoops(char* szSequence, BOOL safeReset, BOOL forceLoops);
	void SetSequenceByIndexForceLoops(int iSequence, float flFramerateMulti, BOOL safeReset, BOOL forceLoops);
	void SetSequenceByNameForceLoops(char* szSequence, float flFramerateMulti, BOOL safeReset, BOOL forceLoops);






	void SetState ( MONSTERSTATE State );
	

	//MODDD - new.
	void reportNetName(void);

	virtual void ReportAIState( void );

	void CheckAttacks ( CBaseEntity *pTarget, float flDist );
	virtual int CheckEnemy ( CBaseEntity *pEnemy );

	//MODDD - helper
	void refreshStack(void);

	void PushEnemy( CBaseEntity *pEnemy, Vector &vecLastKnownPos );
	BOOL PopEnemy( void );
	void DrawFieldOfVision(void);

	BOOL FGetNodeRoute ( Vector vecDest );
	
	//MODDD - not inline as of now.  I need my breakpoints.
	//inline
	void TaskComplete( void ) {
		if ( !HasConditions(bits_COND_TASK_FAILED) ){
			m_iTaskStatus = TASKSTATUS_COMPLETE;
		}
	}
	void MovementComplete( void );
	
	//For now not inline, harder to debug inlines (use break points).
	//inline
	void TaskFail( void ) {
		SetConditions(bits_COND_TASK_FAILED);

		//if(FClassnameIs(this->pev, "monster_scientist")){
		if(monsterID == 4){
			//break point!
			const char* imLazy = (m_pSchedule!=NULL)?m_pSchedule->pName:"NULL!";
			int x = 7;
		}

	}
	inline void TaskBegin( void ) { m_iTaskStatus = TASKSTATUS_RUNNING; }
	int TaskIsRunning( void );
	inline int TaskIsComplete( void ) { return (m_iTaskStatus == TASKSTATUS_COMPLETE); }
	inline int MovementIsComplete( void ) { return (m_movementGoal == MOVEGOAL_NONE); }

	int IScheduleFlags ( void );
	//MODDD - made virtual.
	virtual BOOL FRefreshRoute( void );
	virtual BOOL FRefreshRouteChaseEnemySmart(void);
	BOOL FRouteClear ( void );

	void RouteSimplify( CBaseEntity *pTargetEnt );

	//MODDD - easier to access.
	static void DrawRoute( entvars_t *pev, WayPoint_t *m_Route, int m_iRouteIndex, int r, int g, int b );




	void AdvanceRoute ( float distance, float flInterval );
	virtual BOOL FTriangulate ( const Vector &vecStart , const Vector &vecEnd, float flDist, CBaseEntity *pTargetEnt, Vector *pApex );

	
	//MODDD - made virtual.
	virtual void MakeIdealYaw( Vector vecTarget );

	virtual void SetYawSpeed ( void ) { return; };// allows different yaw_speeds for each activity
	
	BOOL BuildRoute ( const Vector &vecGoal, int iMoveFlag, CBaseEntity *pTarget );
	BOOL BuildRouteSimple ( const Vector &vecGoal, int iMoveFlag, CBaseEntity *pTarget );

	virtual BOOL BuildNearestRoute ( Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist );
	virtual BOOL BuildNearestRouteSimple ( Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist );





	int RouteClassify( int iMoveFlag );
	void InsertWaypoint ( Vector vecLocation, int afMoveFlags );
	
	BOOL FindLateralCover ( const Vector &vecThreat, const Vector &vecViewOffset );
	virtual BOOL FindCover ( Vector vecThreat, Vector vecViewOffset, float flMinDist, float flMaxDist );
	virtual BOOL FValidateCover ( const Vector &vecCoverLocation ) { return TRUE; };
	virtual float CoverRadius( void ) { return 784; } // Default cover radius

	virtual BOOL FCanCheckAttacks ( void );
	virtual void CheckAmmo( void ) { return; };
	virtual int IgnoreConditions ( void );
	

	//MODDD - any calls to set / clear conditions now also apply to m_afConditionsFrame. It is reset per frame instead of per schedule change.
	inline void	SetConditions( int iConditions ) {
		m_afConditions |= iConditions;
		m_afConditionsFrame |= iConditions;
	}
	inline void	ClearConditions( int iConditions ) {
		m_afConditions &= ~iConditions;
		m_afConditionsFrame &= ~iConditions;
	}

	inline BOOL HasConditions( int iConditions ) { if ( m_afConditions & iConditions ) return TRUE; return FALSE; }
	inline BOOL HasAllConditions( int iConditions ) { if ( (m_afConditions & iConditions) == iConditions ) return TRUE; return FALSE; }

	//MODDD - new
	inline BOOL HasConditionsFrame( int iConditions ) { if ( m_afConditionsFrame & iConditions ) return TRUE; return FALSE; }
	inline BOOL HasAllConditionsFrame( int iConditions ) { if ( (m_afConditionsFrame & iConditions) == iConditions ) return TRUE; return FALSE; }

	inline void clearAllConditions(void){
		m_afConditions = 0;
		m_afConditionsFrame = 0;
	}


	virtual BOOL FValidateHintType( short sHint );
	int FindHintNode ( void );
	virtual BOOL FCanActiveIdle ( void );
	virtual void SetTurnActivity ( void );
	float FLSoundVolume ( CSound *pSound );

	BOOL MoveToNode( Activity movementAct, float waitTime, const Vector &goal );
	BOOL MoveToTarget( Activity movementAct, float waitTime );
	BOOL MoveToLocation( Activity movementAct, float waitTime, const Vector &goal );
	BOOL MoveToEnemy( Activity movementAct, float waitTime );

	// Returns the time when the door will be open
	float	OpenDoorAndWait( entvars_t *pevDoor );

	virtual void testMethod(void);

	//MODDD - 
	virtual BOOL interestedInBait(int arg_classID);
	virtual SCHEDULE_TYPE getHeardBaitSoundSchedule(CSound* pSound);
	virtual SCHEDULE_TYPE _getHeardBaitSoundSchedule(CSound* pSound);
	virtual SCHEDULE_TYPE getHeardBaitSoundSchedule();


	virtual int ISoundMask( void );
	virtual CSound* PBestSound ( void );
	virtual CSound* PBestScent ( void );
	virtual float HearingSensitivity( void ) { return 1.0; };

	BOOL FBecomeProne ( void );
	virtual void BarnacleVictimBitten( entvars_t *pevBarnacle );
	virtual void BarnacleVictimReleased( void );

	//MODDD - virtual. This can be hardcoded now.
	virtual void SetEyePosition ( void );

	BOOL FShouldEat( void );// see if a monster is 'hungry'
	void Eat ( float flFullDuration );// make the monster 'full' for a while.

	//MODDD - new version that can expect the 2nd bitmask.
	CBaseEntity *CheckTraceHullAttack( float flDist, int iDamage, int iDmgType );
	CBaseEntity *CheckTraceHullAttack( float flDist, int iDamage, int iDmgType, int iDmgTypeMod );
	CBaseEntity *CheckTraceHullAttack( const Vector vecStartOffset, float flDist, int iDamage, int iDmgType );
	CBaseEntity *CheckTraceHullAttack( const Vector vecStartOffset, float flDist, int iDamage, int iDmgType, int iDmgTypeMod );
	BOOL FacingIdeal( void );
	BOOL FacingIdeal(float argDegreeTolerance);

	BOOL FCheckAITrigger( void );// checks and, if necessary, fires the monster's trigger target. 
	BOOL NoFriendlyFire( void );

	BOOL BBoxFlat( void );

	// PrescheduleThink 
	virtual void PrescheduleThink( void ) { return; };

	BOOL GetEnemy ( void );
	void MakeDamageBloodDecal ( int cCount, float flNoise, TraceResult *ptr, const Vector &vecDir );
	
	
	//MODDD CRITICAL
	//NEVER BEEN VIRTUAL.  OH DEAR.
	//LETS OPEN UP PANDAROA's BOX, SHALL WE???
	GENERATE_TRACEATTACK_PROTOTYPE_VIRTUAL
	GENERATE_TAKEDAMAGE_PROTOTYPE_VIRTUAL



	// combat functions
	float UpdateTarget ( entvars_t *pevTarget );
	virtual Activity GetDeathActivity ( void );
	Activity GetSmallFlinchActivity( void );

	GENERATE_KILLED_PROTOTYPE_VIRTUAL
	//virtual void Killed( entvars_t *pevAttacker, int iGib );
	
	virtual BOOL DetermineGibHeadBlock(void);

	GENERATE_GIBMONSTER_PROTOTYPE_VIRTUAL
	GENERATE_GIBMONSTERGIB_PROTOTYPE_VIRTUAL
	GENERATE_GIBMONSTERSOUND_PROTOTYPE_VIRTUAL
	GENERATE_GIBMONSTEREND_PROTOTYPE_VIRTUAL

	
	BOOL		 ShouldGibMonster( int iGib );
	
	//MODDD - removed. Merged with GibMonster
	//void		 CallGibMonster( void );
	//void		 CallGibMonster( BOOL gibsSpawnDecals );


	//MODDD
	void cleanDelete(void);


	virtual BOOL	HasHumanGibs( void );
	virtual BOOL	HasAlienGibs( void );
	virtual void	FadeMonster( void );	// Called instead of GibMonster() when gibs are disabled
	                                        //MODDD - little out of date comment above. Actually GibMonster is called in anticipation of possibly gibbing a monster.
	                                        //        GibMonster will call FadeMonster if the monster is to be deleted without spawning any gibs.

	Vector ShootAtEnemy( const Vector &shootOrigin );
	Vector ShootAtEnemyMod( const Vector &shootOrigin );
	virtual Vector BodyTarget( const Vector &posSrc ) { return Center( ) * 0.75 + EyePosition() * 0.25; };		// position to shoot at
	//MODDD
	virtual Vector BodyTargetMod( const Vector &posSrc ) { return Center( ) * 0.75 + EyePosition() * 0.25; };		// position to shoot at

	virtual	Vector GetGunPosition( void );
	virtual Vector GetGunPositionAI(void);

	virtual int TakeHealth( float flHealth, int bitsDamageType );
	


	//MODDD
	virtual void setModel(void);
	virtual void setModel(const char* m);
	virtual BOOL getMonsterBlockIdleAutoUpdate(void);



	//MODDD - DAMMIT WHY (were) YOU NOT VIRTUAL.
	GENERATE_DEADTAKEDAMAGE_PROTOTYPE_VIRTUAL

	//~see implementation in combat.cpp



	//MODDD - versions that support 2nd damage bitmask, "bitsDamageTypeMod".
	void RadiusDamageAutoRadius(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType );
	void RadiusDamageAutoRadius(entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType, int bitsDamageTypeMod );
	//MODDD - 2nd version in basemonster.h moved to weapons.h to be treated as more of a universal utility.
	//...restoring these versions for compatability with the way things were and there are places that don't include "weapons.h". Identical and call to the global version as before.
	void RadiusDamageAutoRadius(Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType );
	void RadiusDamageAutoRadius(Vector vecSrc, entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int iClassIgnore, int bitsDamageType, int bitsDamageTypeMod  );
	
	
	virtual int		IsMoving( void ) { return m_movementGoal != MOVEGOAL_NONE; }

	void RouteClear( void );
	void RouteNew( void );
	
	virtual void DeathSound ( void ) { return; };
	virtual void AlertSound ( void ) { return; };
	virtual void IdleSound ( void ) { return; };
	virtual void PainSound ( void ) { return; };
	
	virtual void StopFollowing( BOOL clearSchedule ) {}

	inline void	Remember( int iMemory ) { m_afMemory |= iMemory; }
	inline void	Forget( int iMemory ) { m_afMemory &= ~iMemory; }
	inline BOOL HasMemory( int iMemory ) { if ( m_afMemory & iMemory ) return TRUE; return FALSE; }
	inline BOOL HasAllMemories( int iMemory ) { if ( (m_afMemory & iMemory) == iMemory ) return TRUE; return FALSE; }

	BOOL ExitScriptedSequence( );
	BOOL CineCleanup( );


	CBaseEntity* DropItem ( char *pszItemName, const Vector &vecPos, const Vector &vecAng );// drop an item.


	//MODDD - new.
	virtual void removeFromPoweredUpCommandList(CBaseMonster* argToRemove);
	virtual void forceNewEnemy(CBaseEntity* argIssuing, CBaseEntity* argNewEnemy, BOOL argPassive);

	virtual void setPoweredUpOff(void);
	virtual void setPoweredUpOn(CBaseMonster* argPoweredUpCauseEnt, float argHowLong );
	virtual void forgetForcedEnemy(CBaseMonster* argIssuing, BOOL argPassive);

	virtual void startReanimation(void);
	virtual void EndOfRevive(int preReviveSequence);

	virtual float MoveYawDegreeTolerance(void);
	int BloodColorRedFilter(void);
	int CanUseGermanModel(void);

	BOOL attemptFindCoverFromEnemy(Task_t* pTask);
	WayPoint_t* GetGoalNode(void);

	virtual void ReportGeneric(void);


};



#endif // BASEMONSTER_H
