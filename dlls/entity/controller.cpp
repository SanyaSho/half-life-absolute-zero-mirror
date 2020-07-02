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


#pragma once


//=========================================================
// CONTROLLER
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "basemonster.h"
#include "effects.h"
#include "schedule.h"
#include "weapons.h"
#include "squadmonster.h"

//Separated.
#include "controller_zap_ball.h"
#include "controller_head_ball.h"

#include "defaultai.h"


EASY_CVAR_EXTERN(animationFramerateMulti)





//Sequences, in the order they appear in the model. Some sequences have the same display name and so should just
//be referenced by order (numbered index).
enum controller_sequence{  //key: frames, FPS
	SEQ_CONTROLLER_ATTACK1,
	SEQ_CONTROLLER_ATTACK2,
	SEQ_CONTROLLER_THROW,
	SEQ_CONTROLLER_IDLE2,
	SEQ_CONTROLLER_BLOCK,
	SEQ_CONTROLLER_SHOOT,
	SEQ_CONTROLLER_FLINCH1,
	SEQ_CONTROLLER_FLINCH2,
	SEQ_CONTROLLER_FALL,
	SEQ_CONTROLLER_FORWARD,
	SEQ_CONTROLLER_BACKWARD,
	SEQ_CONTROLLER_UP,
	SEQ_CONTROLLER_DOWN,
	SEQ_CONTROLLER_RIGHT,
	SEQ_CONTROLLER_LEFT,
	SEQ_CONTROLLER_FLOAT,  //SAME?
	SEQ_CONTROLLER_WALK,   //SAME?
	SEQ_CONTROLLER_RUN,    //SAME?
	SEQ_CONTROLLER_DIE1
	

};




//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define CONTROLLER_AE_HEAD_OPEN		1
#define CONTROLLER_AE_BALL_SHOOT	2
#define CONTROLLER_AE_SMALL_SHOOT	3
#define CONTROLLER_AE_POWERUP_FULL	4
#define CONTROLLER_AE_POWERUP_HALF	5


//MODDD - too bad you were unused.
//#define CONTROLLER_FLINCH_DELAY			2		// at most one flinch every n secs

class CController : public CSquadMonster
{
public:
	CController(void);

	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn( void );
	void Precache( void );
	void SetYawSpeed( void );
	int  Classify ( void );
	void HandleAnimEvent( MonsterEvent_t *pEvent );

	void RunAI( void );
	BOOL CheckRangeAttack1 ( float flDot, float flDist );	// balls
	BOOL CheckRangeAttack2 ( float flDot, float flDist );	// head
	BOOL CheckMeleeAttack1 ( float flDot, float flDist );	// block, throw


	//MODDD - new.
	void SetObjectCollisionBox( void )
	{
		if(pev->deadflag != DEAD_NO){
			pev->absmin = pev->origin + Vector(-64, -64, 0);
			pev->absmax = pev->origin + Vector(64, 64, 50);
		}else{

			CBaseMonster::SetObjectCollisionBox();

		}
	}


	Schedule_t* GetSchedule ( void );
	Schedule_t* GetScheduleOfType ( int Type );
	void StartTask ( Task_t *pTask );
	void RunTask ( Task_t *pTask );
	CUSTOM_SCHEDULES;

	void Stop( void );
	void Move ( float flInterval );
	int  CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist );
	void MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval );
	void SetActivity ( Activity NewActivity );
	BOOL ShouldAdvanceRoute( float flWaypointDist, float flInterval );
	int LookupFloat( );

	//MODDD - unused as-is variable, too bad.
	//float m_flNextFlinch;

	float m_flShootTime;
	float m_flShootEnd;

	void PainSound( void );
	void AlertSound( void );
	void IdleSound( void );
	void AttackSound( void );
	void DeathSound( void );

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pDeathSounds[];

	
	GENERATE_TRACEATTACK_PROTOTYPE
	GENERATE_TAKEDAMAGE_PROTOTYPE

	GENERATE_KILLED_PROTOTYPE

	GENERATE_GIBMONSTER_PROTOTYPE

	
	int getLoopingDeathSequence(void);
	

	CSprite *m_pBall[2];	// hand balls
	int m_iBall[2];			// how bright it should be
	float m_iBallTime[2];	// when it should be that color
	int m_iBallCurrent[2];	// current brightness

	Vector m_vecEstVelocity;

	Vector m_velocity;
	int m_fInCombat;
};

#if REMOVE_ORIGINAL_NAMES != 1
	LINK_ENTITY_TO_CLASS( monster_alien_controller, CController );
#endif


#if EXTRA_NAMES > 0
	LINK_ENTITY_TO_CLASS( controller, CController );

	#if EXTRA_NAMES == 2
		LINK_ENTITY_TO_CLASS( monster_controller, CController );
		LINK_ENTITY_TO_CLASS( alien_controller, CController );
	#endif

#endif



TYPEDESCRIPTION	CController::m_SaveData[] = 
{
	DEFINE_ARRAY( CController, m_pBall, FIELD_CLASSPTR, 2 ),
	DEFINE_ARRAY( CController, m_iBall, FIELD_INTEGER, 2 ),
	DEFINE_ARRAY( CController, m_iBallTime, FIELD_TIME, 2 ),
	DEFINE_ARRAY( CController, m_iBallCurrent, FIELD_INTEGER, 2 ),
	DEFINE_FIELD( CController, m_vecEstVelocity, FIELD_VECTOR ),
};
IMPLEMENT_SAVERESTORE( CController, CSquadMonster );


const char *CController::pAttackSounds[] = 
{
	"controller/con_attack1.wav",
	"controller/con_attack2.wav",
	"controller/con_attack3.wav",
};

const char *CController::pIdleSounds[] = 
{
	"controller/con_idle1.wav",
	"controller/con_idle2.wav",
	"controller/con_idle3.wav",
	"controller/con_idle4.wav",
	"controller/con_idle5.wav",
};

const char *CController::pAlertSounds[] = 
{
	"controller/con_alert1.wav",
	"controller/con_alert2.wav",
	"controller/con_alert3.wav",
};

const char *CController::pPainSounds[] = 
{
	"controller/con_pain1.wav",
	"controller/con_pain2.wav",
	"controller/con_pain3.wav",
};

const char *CController::pDeathSounds[] = 
{
	"controller/con_die1.wav",
	"controller/con_die2.wav",
};


//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int CController :: Classify ( void )
{
	return	CLASS_ALIEN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CController :: SetYawSpeed ( void )
{
	int ys;

	ys = 120;

#if 0
	switch ( m_Activity )
	{
	}
#endif

	pev->yaw_speed = ys;
}




GENERATE_TRACEATTACK_IMPLEMENTATION(CController)
{
	GENERATE_TRACEATTACK_PARENT_CALL(CSquadMonster);
}

GENERATE_TAKEDAMAGE_IMPLEMENTATION(CController)
{
	// HACK HACK -- until we fix this.
	if ( IsAlive() )
		PainSound();
	//MODDD NOTE - ...is this redundant now? TODO: check this out and remove it if this PainSound() call is not needed anymore.
		
	return GENERATE_TAKEDAMAGE_PARENT_CALL(CSquadMonster);
}


GENERATE_KILLED_IMPLEMENTATION(CController)
{
	// shut off balls
	/*
	m_iBall[0] = 0;
	m_iBallTime[0] = gpGlobals->time + 4.0;
	m_iBall[1] = 0;
	m_iBallTime[1] = gpGlobals->time + 4.0;
	*/

	// fade balls
	if (m_pBall[0])
	{
		m_pBall[0]->SUB_StartFadeOut();
		m_pBall[0] = NULL;
	}
	if (m_pBall[1])
	{
		m_pBall[1]->SUB_StartFadeOut();
		m_pBall[1] = NULL;
	}

	GENERATE_KILLED_PARENT_CALL(CSquadMonster);
	
	//MODDD - safety. Must fall once killed, just in case.
	pev->movetype = MOVETYPE_TOSS;

}



GENERATE_GIBMONSTER_IMPLEMENTATION(CController)
{
	// delete balls
	if (m_pBall[0])
	{
		UTIL_Remove( m_pBall[0] );
		m_pBall[0] = NULL;
	}
	if (m_pBall[1])
	{
		UTIL_Remove( m_pBall[1] );
		m_pBall[1] = NULL;
	}
	GENERATE_GIBMONSTER_PARENT_CALL(CSquadMonster);
}







void CController :: PainSound( void )
{
	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_ARRAY_FILTERED( CHAN_VOICE, pPainSounds ); 
}	

void CController :: AlertSound( void )
{
	EMIT_SOUND_ARRAY_FILTERED( CHAN_VOICE, pAlertSounds ); 
}

void CController :: IdleSound( void )
{
	EMIT_SOUND_ARRAY_FILTERED( CHAN_VOICE, pIdleSounds ); 
}

void CController :: AttackSound( void )
{
	EMIT_SOUND_ARRAY_FILTERED( CHAN_VOICE, pAttackSounds ); 
}

void CController :: DeathSound( void )
{
	EMIT_SOUND_ARRAY_FILTERED( CHAN_VOICE, pDeathSounds ); 
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CController :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case CONTROLLER_AE_HEAD_OPEN:
		{
			Vector vecStart, angleGun;
			
			GetAttachment( 0, vecStart, angleGun );
			
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_ELIGHT );
				WRITE_SHORT( entindex( ) + 0x1000 );		// entity, attachment
				WRITE_COORD( vecStart.x );		// origin
				WRITE_COORD( vecStart.y );
				WRITE_COORD( vecStart.z );
				WRITE_COORD( 1 );	// radius
				WRITE_BYTE( 255 );	// R
				WRITE_BYTE( 192 );	// G
				WRITE_BYTE( 64 );	// B
				WRITE_BYTE( 20 );	// life * 10
				WRITE_COORD( -32 ); // decay
			MESSAGE_END();

			m_iBall[0] = 192;
			m_iBallTime[0] = gpGlobals->time + atoi( pEvent->options ) / 15.0;
			m_iBall[1] = 255;
			m_iBallTime[1] = gpGlobals->time + atoi( pEvent->options ) / 15.0;

		}
		break;

		case CONTROLLER_AE_BALL_SHOOT:
		{
			Vector vecStart, angleGun;
			
			GetAttachment( 0, vecStart, angleGun );

			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_ELIGHT );
				WRITE_SHORT( entindex( ) + 0x1000 );		// entity, attachment
				WRITE_COORD( 0 );		// origin
				WRITE_COORD( 0 );
				WRITE_COORD( 0 );
				WRITE_COORD( 32 );	// radius
				WRITE_BYTE( 255 );	// R
				WRITE_BYTE( 192 );	// G
				WRITE_BYTE( 64 );	// B
				WRITE_BYTE( 10 );	// life * 10
				WRITE_COORD( 32 ); // decay
			MESSAGE_END();

			CBaseMonster *pBall = (CBaseMonster*)Create( "controller_head_ball", vecStart, pev->angles, edict() );

			pBall->pev->velocity = Vector( 0, 0, 32 );
			pBall->m_hEnemy = m_hEnemy;

			m_iBall[0] = 0;
			m_iBall[1] = 0;
		}
		break;

		case CONTROLLER_AE_SMALL_SHOOT:
		{
			AttackSound( );
			m_flShootTime = gpGlobals->time;
			m_flShootEnd = m_flShootTime + atoi( pEvent->options ) / 15.0;
		}
		break;
		case CONTROLLER_AE_POWERUP_FULL:
		{
			m_iBall[0] = 255;
			m_iBallTime[0] = gpGlobals->time + atoi( pEvent->options ) / 15.0;
			m_iBall[1] = 255;
			m_iBallTime[1] = gpGlobals->time + atoi( pEvent->options ) / 15.0;
		}
		break;
		case CONTROLLER_AE_POWERUP_HALF:
		{
			m_iBall[0] = 192;
			m_iBallTime[0] = gpGlobals->time + atoi( pEvent->options ) / 15.0;
			m_iBall[1] = 192;
			m_iBallTime[1] = gpGlobals->time + atoi( pEvent->options ) / 15.0;
		}
		break;
		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

CController::CController(){


}

//=========================================================
// Spawn
//=========================================================
void CController :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/controller.mdl");
	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 64 ));

	pev->classname = MAKE_STRING("monster_alien_controller");

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_FLY;
	pev->flags			|= FL_FLY;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health			= gSkillData.controllerHealth;
	pev->view_ofs		= Vector( 0, 0, -2 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_FULL;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	MonsterInit();

	

}

extern int global_useSentenceSave;
//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CController :: Precache()
{
	PRECACHE_MODEL("models/controller.mdl");

	global_useSentenceSave = TRUE;

	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND_ARRAY( pDeathSounds );

	global_useSentenceSave = FALSE;

	PRECACHE_MODEL( "sprites/xspark4.spr");

	UTIL_PrecacheOther( "controller_energy_ball" );
	UTIL_PrecacheOther( "controller_head_ball" );
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================


// Chase enemy schedule
Task_t tlControllerChaseEnemy[] = 
{
	{ TASK_GET_PATH_TO_ENEMY,	(float)128		},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0		},

};

Schedule_t slControllerChaseEnemy[] =
{
	{ 
		tlControllerChaseEnemy,
		ARRAYSIZE ( tlControllerChaseEnemy ),
		bits_COND_NEW_ENEMY			|
		bits_COND_TASK_FAILED,
		0,
		"ControllerChaseEnemy"
	},
};



Task_t	tlControllerStrafe[] =
{
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_GET_PATH_TO_ENEMY,		(float)128					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_WAIT,					(float)1					},
};

Schedule_t	slControllerStrafe[] =
{
	{ 
		tlControllerStrafe,
		ARRAYSIZE ( tlControllerStrafe ), 
		bits_COND_NEW_ENEMY,
		0,
		"ControllerStrafe"
	},
};


Task_t	tlControllerTakeCover[] =
{
	{ TASK_WAIT,					(float)0.2					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_WAIT_FOR_MOVEMENT,		(float)0					},
	{ TASK_WAIT,					(float)1					},
};

Schedule_t	slControllerTakeCover[] =
{
	{ 
		tlControllerTakeCover,
		ARRAYSIZE ( tlControllerTakeCover ), 
		bits_COND_NEW_ENEMY,
		0,
		"ControllerTakeCover"
	},
};


Task_t	tlControllerFail[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		},
	{ TASK_WAIT_PVS,			(float)0		},
};

Schedule_t	slControllerFail[] =
{
	{
		tlControllerFail,
		ARRAYSIZE ( tlControllerFail ),
		0,
		0,
		"ControllerFail"
	},
};



DEFINE_CUSTOM_SCHEDULES( CController )
{
	slControllerChaseEnemy,
	slControllerStrafe,
	slControllerTakeCover,
	slControllerFail,
};

IMPLEMENT_CUSTOM_SCHEDULES( CController, CSquadMonster );



//=========================================================
// StartTask
//=========================================================
void CController :: StartTask ( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		
	case TASK_RANGE_ATTACK1:
		CSquadMonster :: StartTask ( pTask );
		break;
	case TASK_GET_PATH_TO_ENEMY_LKP:
		{
			CBaseEntity* enemyTest;

			//MODDD - send along the enemy if we still have one.
			if(m_hEnemy != NULL){
				enemyTest = m_hEnemy.GetEntity();
			}else{
				enemyTest = NULL;
			}

			//MODDD NOTE - the interesting (new? different? unique?) thing here is sending the distance between the enemyLKP and my origin... not that, the PLUS 1024 here.
			//             why isn't that elsewhere? isn't there some possibility of a route needing to go outwards or out of the way a bit? Must not come up much.
			//             Is this kind of situation more likely for flyers then?  Questions questions.
			//  Anyways, we are sending along the moveflag (TO_ENEMY) and a reference to the enemy, if it exists. The more information the merrier for pathfinding.
			if (BuildNearestRoute( m_vecEnemyLKP, pev->view_ofs, pTask->flData, (m_vecEnemyLKP - pev->origin).Length() + 1024, bits_MF_TO_ENEMY, enemyTest ))
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToEnemyLKP failed!!\n" );
				TaskFail();
			}
			break;
		}
	case TASK_GET_PATH_TO_ENEMY:
		{
			CBaseEntity *pEnemy = m_hEnemy;

			if ( pEnemy == NULL )
			{
				TaskFail();
				return;
			}

			//MODDD - like above, sending more info. Moveflag and enemy reference which looks to be guaranteed (task fails if we don't have one or it expired)
			if (BuildNearestRoute( pEnemy->pev->origin, pEnemy->pev->view_ofs, pTask->flData, (pEnemy->pev->origin - pev->origin).Length() + 1024, bits_MF_TO_ENEMY, pEnemy ))
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToEnemy failed!!\n" );
				TaskFail();
			}
			break;
		}
		
	default:
		{
		CSquadMonster :: StartTask ( pTask );
		break;
		}
	}
}

//MODDD - old location of "Intersect" top-level method.  Moved to util.cpp to be shared by other files too (like stukabat)
//ALSO: method renamed to "UTIL_Intersect".  Should there be any other method called "Intersect", it shouldn't interfere with this.

int CController::LookupFloat( )
{
	//m_velocity = Vector(0, 0, 0);

	//only do this change if the current sequence is over, we really don't need
	//to rapidly set to "up" between movements.
	if (m_velocity.Length( ) < 32.0)
	{

		if(m_fSequenceFinished){
			return LookupSequence( "up" );
		}else{
			//don't affect the squence at all.
			return -1;
		}
	}

	UTIL_MakeAimVectors( pev->angles );
	float x = DotProduct( gpGlobals->v_forward, m_velocity );
	float y = DotProduct( gpGlobals->v_right, m_velocity );
	float z = DotProduct( gpGlobals->v_up, m_velocity );

	if (fabs(x) > fabs(y) && fabs(x) > fabs(z))
	{
		if (x > 0)
			return LookupSequence( "forward");
		else
			return LookupSequence( "backward");
	}
	else if (fabs(y) > fabs(z))
	{
		/*
		if (y > 0)
			return LookupSequence( "right");
		else
			return LookupSequence( "left");
		*/
		//MODDD - swapping. If it's supposed to look like swimming through the air,
		//shouldn't the push be away from the direction you want to move?
		if (y > 0)
			return LookupSequence( "left");
		else
			return LookupSequence( "right");
	}
	else
	{
		if (z > 0)
			return LookupSequence( "up");
		else
			return LookupSequence( "down");
	}
}


//=========================================================
// RunTask 
//=========================================================
void CController :: RunTask ( Task_t *pTask )
{

	if (m_flShootEnd > gpGlobals->time)
	{
		Vector vecHand, vecAngle;
		
		GetAttachment( 2, vecHand, vecAngle );
	
		while (m_flShootTime < m_flShootEnd && m_flShootTime < gpGlobals->time)
		{
			Vector vecSrc = vecHand + pev->velocity * (m_flShootTime - gpGlobals->time);
			Vector vecDir;
			
			if (m_hEnemy != NULL)
			{
				if (HasConditions( bits_COND_SEE_ENEMY ))
				{
					m_vecEstVelocity = m_vecEstVelocity * 0.5 + m_hEnemy->pev->velocity * 0.5;
				}
				else
				{
					m_vecEstVelocity = m_vecEstVelocity * 0.8;
				}
				vecDir = UTIL_Intersect( vecSrc, m_hEnemy->BodyTarget( pev->origin ), m_vecEstVelocity, gSkillData.controllerSpeedBall );
				float delta = 0.03490; // +-2 degree
				vecDir = vecDir + Vector( RANDOM_FLOAT( -delta, delta ), RANDOM_FLOAT( -delta, delta ), RANDOM_FLOAT( -delta, delta ) ) * gSkillData.controllerSpeedBall;

				vecSrc = vecSrc + vecDir * (gpGlobals->time - m_flShootTime);
				CBaseMonster *pBall = (CBaseMonster*)Create( "controller_energy_ball", vecSrc, pev->angles, edict() );
				pBall->pev->velocity = vecDir;
			}
			m_flShootTime += 0.2;
		}

		if (m_flShootTime > m_flShootEnd)
		{
			m_iBall[0] = 64;
			m_iBallTime[0] = m_flShootEnd;
			m_iBall[1] = 64;
			m_iBallTime[1] = m_flShootEnd;
			m_fInCombat = FALSE;
		}
	}

	switch ( pTask->iTask )
	{
	case TASK_WAIT_FOR_MOVEMENT:
	case TASK_WAIT:
	case TASK_WAIT_FACE_ENEMY:
	case TASK_WAIT_PVS:
		MakeIdealYaw( m_vecEnemyLKP );
		ChangeYaw( pev->yaw_speed );

		//if (m_fSequenceFinished)
		//MODDD - "m_fSequenceFinishedSinceLoop" is better a ttimes now. m_fSequenceFinished is ticked if this is the very frame it finished on.
		if(m_fSequenceFinishedSinceLoop)
		{
			m_fInCombat = FALSE;
		}

		CSquadMonster :: RunTask ( pTask );

		if (!m_fInCombat)
		{
			if (HasConditions ( bits_COND_CAN_RANGE_ATTACK1 ))
			{
				pev->sequence = LookupActivity( ACT_RANGE_ATTACK1 );
				pev->frame = 0;
				ResetSequenceInfo( );
				m_fInCombat = TRUE;
			}
			else if (HasConditions ( bits_COND_CAN_RANGE_ATTACK2 ))
			{
				pev->sequence = LookupActivity( ACT_RANGE_ATTACK2 );
				pev->frame = 0;
				ResetSequenceInfo( );
				m_fInCombat = TRUE;
			}
			//MODDD - extra check. If I'm supposed to be stopped don't even try this.
			//        I pick idle animations fine, right?
			else if(this->IsMoving())
			{
				int iFloat = LookupFloat( );
				//if (m_fSequenceFinished || iFloat != pev->sequence)
				//MODDD - if it loops, don't force a reset. It just
				//        messes up interpolation and resets the animation after
				//        already going into the second automatic restart a tiny
				//        bit.
				if (iFloat != -1 && iFloat != pev->sequence)
				{
					pev->sequence = iFloat;
					pev->frame = 0;
					ResetSequenceInfo( );
				}
			}
		}
	break;


	default: 
		CSquadMonster :: RunTask ( pTask );
		break;
	}
}


//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CController :: GetSchedule ( void )
{
	switch	( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
		break;

	case MONSTERSTATE_ALERT:
		break;

	case MONSTERSTATE_COMBAT:
	{
		//MODDD NOTE - uhhh......  ????? ............ ?????????????????????????
		//Vector vecTmp = UTIL_Intersect( Vector( 0, 0, 0 ), Vector( 100, 4, 7 ), Vector( 2, 10, -3 ), 20.0 );

		// dead enemy
		if ( HasConditions ( bits_COND_LIGHT_DAMAGE ) )
		{
			// m_iFrustration++;
		}
		if ( HasConditions ( bits_COND_HEAVY_DAMAGE ) )
		{
			// m_iFrustration++;
		}
	}
	break;
	//MODDD - not that interesting actually.
	case MONSTERSTATE_DEAD:
	{
		return GetScheduleOfType( SCHED_DIE );
	}
	break;


	}//END OF switch
	return CSquadMonster :: GetSchedule();
}



//=========================================================
//=========================================================
Schedule_t* CController :: GetScheduleOfType ( int Type ) 
{
	// ALERT( at_console, "%d\n", m_iFrustration );
	switch	( Type )
	{
	case SCHED_CHASE_ENEMY:
		return slControllerChaseEnemy;
	case SCHED_RANGE_ATTACK1:
		return slControllerStrafe;
	case SCHED_RANGE_ATTACK2:
	case SCHED_MELEE_ATTACK1:
	case SCHED_MELEE_ATTACK2:
	case SCHED_TAKE_COVER_FROM_ENEMY:
		return slControllerTakeCover;
	case SCHED_FAIL:
		return slControllerFail;
	case SCHED_DIE:{
		return flyerDeathSchedule();
	break;}


	}//END OF switch
	

	return CBaseMonster :: GetScheduleOfType( Type );
}






//MODDD NOTE - so why does it pick one or the other with such broad, sometimes
//             overlapping conditions?
//  If rangedAttack1 is given the Okay, it runs, period.  That is distance is
//  at least 256.
//  But if it is not okay, rangedAttack2 gets the chance.
//  It requires a distance of at least 64 instead.
//  So long story short, attack1 always gets picked if further away than 256.
//  But between a distance of 64 and 256, Attack2 gets picked instead.
//  (and obviously neither if the distance exceeds 2048)

// Attack1 is a bunch of smaller dumb zap balls,
// Attack2 is the bigger homing head ball.

//=========================================================
// CheckRangeAttack1  - shoot a bigass energy ball out of their head
//
//=========================================================
BOOL CController :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDot > 0.5 && flDist > 256 && flDist <= 2048 )
	{
		return TRUE;
	}
	return FALSE;
}


BOOL CController :: CheckRangeAttack2 ( float flDot, float flDist )
{
	if ( flDot > 0.5 && flDist > 64 && flDist <= 2048 )
	{
		return TRUE;
	}
	return FALSE;
}


BOOL CController :: CheckMeleeAttack1 ( float flDot, float flDist )
{
	return FALSE;
}


void CController :: SetActivity ( Activity NewActivity )
{
	CBaseMonster::SetActivity( NewActivity );

	switch ( m_Activity)
	{
	case ACT_WALK:
		m_flGroundSpeed = 100;
		break;
	default:
		m_flGroundSpeed = 100;
		break;
	}
}



//=========================================================
// RunAI
//=========================================================
void CController :: RunAI( void )
{
	CBaseMonster :: RunAI();
	Vector vecStart, angleGun;

	if ( HasMemory( bits_MEMORY_KILLED ) )
		return;

	for (int i = 0; i < 2; i++)
	{
		if (m_pBall[i] == NULL)
		{
			m_pBall[i] = CSprite::SpriteCreate( "sprites/xspark4.spr", pev->origin, TRUE );
			m_pBall[i]->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
			m_pBall[i]->SetAttachment( edict(), (i + 3) );
			m_pBall[i]->SetScale( 1.0 );
		}

		float t = m_iBallTime[i] - gpGlobals->time;
		if (t > 0.1)
			t = 0.1 / t;
		else
			t = 1.0;

		m_iBallCurrent[i] += (m_iBall[i] - m_iBallCurrent[i]) * t;

		m_pBall[i]->SetBrightness( m_iBallCurrent[i] );

		GetAttachment( i + 2, vecStart, angleGun );
		UTIL_SetOrigin( m_pBall[i]->pev, vecStart );
		
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_ELIGHT );
			WRITE_SHORT( entindex( ) + 0x1000 * (i + 3) );		// entity, attachment
			WRITE_COORD( vecStart.x );		// origin
			WRITE_COORD( vecStart.y );
			WRITE_COORD( vecStart.z );
			WRITE_COORD( m_iBallCurrent[i] / 8 );	// radius
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 192 );	// G
			WRITE_BYTE( 64 );	// B
			WRITE_BYTE( 5 );	// life * 10
			WRITE_COORD( 0 ); // decay
		MESSAGE_END();
	}
}


extern void DrawRoute( entvars_t *pev, WayPoint_t *m_Route, int m_iRouteIndex, int r, int g, int b );

void CController::Stop( void ) 
{ 

	//MODDD - shouldn't this also be a signal to zero my velocity?
	m_velocity = Vector(0, 0, 0);

	m_IdealActivity = GetStoppedActivity(); 
}


#define DIST_TO_CHECK	200
void CController :: Move ( float flInterval ) 
{
	float	flWaypointDist;
	float	flCheckDist;
	float	flDist;// how far the lookahead check got before hitting an object.
	float	flMoveDist;
	Vector		vecDir;
	Vector		vecApex;
	CBaseEntity	*pTargetEnt;

	// Don't move if no valid route
	if ( FRouteClear() )
	{
		ALERT( at_aiconsole, "Tried to move with no route!\n" );
		TaskFail();
		return;
	}
	
	if ( m_flMoveWaitFinished > gpGlobals->time )
		return;

// Debug, test movement code
#if 0
//	if ( CVAR_GET_FLOAT("stopmove" ) != 0 )
	{
		if ( m_movementGoal == MOVEGOAL_ENEMY )
			RouteSimplify( m_hEnemy );
		else
			RouteSimplify( m_hTargetEnt );
		FRefreshRoute();
		return;
	}
#else
// Debug, draw the route
//	DrawRoute( pev, m_Route, m_iRouteIndex, 0, 0, 255 );
#endif

	// if the monster is moving directly towards an entity (enemy for instance), we'll set this pointer
	// to that entity for the CheckLocalMove and Triangulate functions.
	pTargetEnt = NULL;

	if (m_flGroundSpeed == 0)
	{
		m_flGroundSpeed = 100;
		// TaskFail( );
		// return;
	}


	//MODDD - added pev->framerate and the CVar.
	flMoveDist = m_flGroundSpeed * pev->framerate * EASY_CVAR_GET(animationFramerateMulti) * flInterval;

	do 
	{
		// local move to waypoint.
		vecDir = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Normalize();
		flWaypointDist = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length();
		
		// MakeIdealYaw ( m_Route[ m_iRouteIndex ].vecLocation );
		// ChangeYaw ( pev->yaw_speed );

		// if the waypoint is closer than CheckDist, CheckDist is the dist to waypoint
		if ( flWaypointDist < DIST_TO_CHECK )
		{
			flCheckDist = flWaypointDist;
		}
		else
		{
			flCheckDist = DIST_TO_CHECK;
		}
		
		if ( (m_Route[ m_iRouteIndex ].iType & (~bits_MF_NOT_TO_MASK)) == bits_MF_TO_ENEMY )
		{
			// only on a PURE move to enemy ( i.e., ONLY MF_TO_ENEMY set, not MF_TO_ENEMY and DETOUR )
			pTargetEnt = m_hEnemy;
		}
		else if ( (m_Route[ m_iRouteIndex ].iType & ~bits_MF_NOT_TO_MASK) == bits_MF_TO_TARGETENT )
		{
			pTargetEnt = m_hTargetEnt;
		}

		// !!!BUGBUG - CheckDist should be derived from ground speed.
		// If this fails, it should be because of some dynamic entity blocking this guy.
		// We've already checked this path, so we should wait and time out if the entity doesn't move
		flDist = 0;
		if ( CheckLocalMove ( pev->origin, pev->origin + vecDir * flCheckDist, pTargetEnt, &flDist ) != LOCALMOVE_VALID )
		{
			CBaseEntity *pBlocker;

			// Can't move, stop
			Stop();
			// Blocking entity is in global trace_ent
			pBlocker = CBaseEntity::Instance( gpGlobals->trace_ent );
			if (pBlocker)
			{
				DispatchBlocked( edict(), pBlocker->edict() );
			}
			if ( pBlocker && m_moveWaitTime > 0 && pBlocker->IsMoving() && !pBlocker->IsPlayer() && (gpGlobals->time-m_flMoveWaitFinished) > 3.0 )
			{
				// Can we still move toward our target?
				if ( flDist < m_flGroundSpeed )
				{
					// Wait for a second
					m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime;
	//				ALERT( at_aiconsole, "Move %s!!!\n", STRING( pBlocker->pev->classname ) );
					return;
				}
			}
			else 
			{
				// try to triangulate around whatever is in the way.
				if ( FTriangulate( pev->origin, m_Route[ m_iRouteIndex ].vecLocation, flDist, pTargetEnt, &vecApex ) )
				{
					InsertWaypoint( vecApex, bits_MF_TO_DETOUR );
					RouteSimplify( pTargetEnt );
				}
				else
				{
	 			    ALERT ( at_aiconsole, "Couldn't Triangulate\n" );
					Stop();
					if ( m_moveWaitTime > 0 )
					{
						FRefreshRoute();
						m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime * 0.5;
					}
					else
					{
						TaskFail();
						ALERT( at_aiconsole, "Failed to move!\n" );
						//ALERT( at_aiconsole, "%f, %f, %f\n", pev->origin.z, (pev->origin + (vecDir * flCheckDist)).z, m_Route[m_iRouteIndex].vecLocation.z );
					}
					return;
				}
			}
		}

		// UNDONE: this is a hack to quit moving farther than it has looked ahead.
		if (flCheckDist < flMoveDist)
		{
			MoveExecute( pTargetEnt, vecDir, flCheckDist / m_flGroundSpeed );

			// ALERT( at_console, "%.02f\n", flInterval );
			AdvanceRoute( flWaypointDist, flInterval );
			flMoveDist -= flCheckDist;
		}
		else
		{
			MoveExecute( pTargetEnt, vecDir, flMoveDist / m_flGroundSpeed );

			if ( ShouldAdvanceRoute( flWaypointDist - flMoveDist, flInterval ) )
			{
				AdvanceRoute( flWaypointDist, flInterval );
			}
			flMoveDist = 0;
		}

		if ( MovementIsComplete() )
		{
			Stop();
			RouteClear();
		}
	} while (flMoveDist > 0 && flCheckDist > 0);

	// cut corner?
	if (flWaypointDist < 128)
	{
		if ( m_movementGoal == MOVEGOAL_ENEMY )
			RouteSimplify( m_hEnemy );
		else
			RouteSimplify( m_hTargetEnt );
		FRefreshRoute();

		if (m_flGroundSpeed > 100)
			m_flGroundSpeed -= 40;
	}
	else
	{
		if (m_flGroundSpeed < 400)
			m_flGroundSpeed += 10;
	}
}



//MODDD - added flInterval, but it looks like controllers never really cared about anything too precise here.
BOOL CController:: ShouldAdvanceRoute( float flWaypointDist, float flInterval )
{
	if ( flWaypointDist <= 32  )
	{
		return TRUE;
	}

	return FALSE;
}


int CController :: CheckLocalMove ( const Vector &vecStart, const Vector &vecEnd, CBaseEntity *pTarget, float *pflDist )
{
	TraceResult tr;

	UTIL_TraceHull( vecStart + Vector( 0, 0, 32), vecEnd + Vector( 0, 0, 32), dont_ignore_monsters, large_hull, edict(), &tr );

	// ALERT( at_console, "%.0f %.0f %.0f : ", vecStart.x, vecStart.y, vecStart.z );
	// ALERT( at_console, "%.0f %.0f %.0f\n", vecEnd.x, vecEnd.y, vecEnd.z );

	if (pflDist)
	{
		*pflDist = ( (tr.vecEndPos - Vector( 0, 0, 32 )) - vecStart ).Length();// get the distance.
	}

	// ALERT( at_console, "check %d %d %f\n", tr.fStartSolid, tr.fAllSolid, tr.flFraction );
	if (tr.fStartSolid || tr.flFraction < 1.0)
	{
		if ( pTarget && pTarget->edict() == gpGlobals->trace_ent )
			return LOCALMOVE_VALID;
		return LOCALMOVE_INVALID;
	}

	return LOCALMOVE_VALID;
}


void CController::MoveExecute( CBaseEntity *pTargetEnt, const Vector &vecDir, float flInterval )
{
	if ( m_IdealActivity != m_movementActivity )
		m_IdealActivity = m_movementActivity;

	// ALERT( at_console, "move %.4f %.4f %.4f : %f\n", vecDir.x, vecDir.y, vecDir.z, flInterval );

	// float flTotal = m_flGroundSpeed * pev->framerate * flInterval;
	// UTIL_MoveToOrigin ( ENT(pev), m_Route[ m_iRouteIndex ].vecLocation, flTotal, MOVE_STRAFE );

	m_velocity = m_velocity * 0.8 + m_flGroundSpeed * vecDir * 0.2;

	UTIL_MoveToOrigin ( ENT(pev), pev->origin + m_velocity, m_velocity.Length() * flInterval, MOVE_STRAFE );
	
}


int CController::getLoopingDeathSequence(void){
	return SEQ_CONTROLLER_FALL;
}



//MODDD - two types of balls moved to their own files, controller_zap_ball and controller_head_ball .h / .cpp.
//        Other monsters may include and use them as well.
