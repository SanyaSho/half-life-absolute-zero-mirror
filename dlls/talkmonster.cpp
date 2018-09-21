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
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include "basemonster.h"
#include	"schedule.h"
#include	"talkmonster.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"soundent.h"
#include	"animation.h"



//MODDD
extern float global_peopleStrobe;
extern float global_wildHeads;
extern float global_raveEffectSpawnInterval;
extern float global_pissedNPCs;
extern float global_thatWasntPunch;
extern float global_NPCsTalkMore;
extern float global_barneyPrintouts;
extern BOOL globalPSEUDO_iCanHazMemez;

extern unsigned short g_sFreakyLight;

EASY_CVAR_EXTERN(playerFollowerMax)


//=========================================================
// Talking monster base class
// Used for scientists and barneys
//=========================================================
float	CTalkMonster::g_talkWaitTime = 0;		// time delay until it's ok to speak: used so that two NPCs don't talk at once





// NOTE: m_voicePitch & m_szGrp should be fixed up by precache each save/restore

TYPEDESCRIPTION	CTalkMonster::m_SaveData[] = 
{
	DEFINE_FIELD( CTalkMonster, m_bitsSaid, FIELD_INTEGER ),
	DEFINE_FIELD( CTalkMonster, m_nSpeak, FIELD_INTEGER ),

	// Recalc'ed in Precache()
	//	DEFINE_FIELD( CTalkMonster, m_voicePitch, FIELD_INTEGER ),
	//	DEFINE_FIELD( CTalkMonster, m_szGrp, FIELD_??? ),
	DEFINE_FIELD( CTalkMonster, m_useTime, FIELD_TIME ),
	DEFINE_FIELD( CTalkMonster, m_iszUse, FIELD_STRING ),
	DEFINE_FIELD( CTalkMonster, m_iszUnUse, FIELD_STRING ),
	DEFINE_FIELD( CTalkMonster, m_flLastSaidSmelled, FIELD_TIME ),
	DEFINE_FIELD( CTalkMonster, m_flStopTalkTime, FIELD_TIME ),
	DEFINE_FIELD( CTalkMonster, m_hTalkTarget, FIELD_EHANDLE ),

	DEFINE_FIELD( CTalkMonster, m_flPlayerDamage, FIELD_FLOAT ),
	//DEFINE_FIELD( CTalkMonster, forgiveSuspiciousTime, FIELD_TIME ),  no need really?  happens so often.
	DEFINE_FIELD( CTalkMonster, forgiveSomePlayerDamageTime, FIELD_TIME ),
	


};
//IMPLEMENT_SAVERESTORE( CTalkMonster, CBaseMonster );


int CTalkMonster::Save( CSave &save )
{
	if ( !CBaseMonster::Save(save) )
		return 0;
	return save.WriteFields( "CTalkMonster", this, m_SaveData, ARRAYSIZE(m_SaveData) );
}
int CTalkMonster::Restore( CRestore &restore )
{
	//setModelCustom();

	if ( !CBaseMonster::Restore(restore) )
		return 0;
	
	if(this->m_flPlayerDamage > 0 && forgiveSomePlayerDamageTime == -1){
		forgiveSomePlayerDamageTime = gpGlobals->time + 5;
	}

	return restore.ReadFields( "CTalkMonster", this, m_SaveData, ARRAYSIZE(m_SaveData) );
}







//MODDD - new, placeholder.
int CTalkMonster::getMadSentencesMax(void){
	return 0;
}
int CTalkMonster::getMadInterSentencesMax(void){
	return 0;
}



// array of friend names
char *CTalkMonster::m_szFriends[TLK_CFRIENDS] = 
{
	"monster_barney",
	"monster_scientist",
	"monster_sitting_scientist",
};


//=========================================================
// AI Schedules Specific to talking monsters
//=========================================================

Task_t	tlIdleResponse[] =
{
	{ TASK_SET_ACTIVITY,	(float)ACT_IDLE	},// Stop and listen
	{ TASK_WAIT,			(float)0.5		},// Wait until sure it's me they are talking to
	{ TASK_TLK_EYECONTACT,	(float)0		},// Wait until speaker is done
	{ TASK_TLK_RESPOND,		(float)0		},// Wait and then say my response
	{ TASK_TLK_IDEALYAW,	(float)0		},// look at who I'm talking to
	{ TASK_FACE_IDEAL,		(float)0		}, 
	{ TASK_SET_ACTIVITY,	(float)ACT_SIGNAL3	},
	{ TASK_TLK_EYECONTACT,	(float)0		},// Wait until speaker is done
};

Schedule_t	slIdleResponse[] =
{
	{ 
		tlIdleResponse,
		ARRAYSIZE ( tlIdleResponse ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"Idle Response"

	},
};

Task_t	tlIdleSpeak[] =
{
	{ TASK_TLK_SPEAK,		(float)0		},// question or remark
	{ TASK_TLK_IDEALYAW,	(float)0		},// look at who I'm talking to
	{ TASK_FACE_IDEAL,		(float)0		}, 
	{ TASK_SET_ACTIVITY,	(float)ACT_SIGNAL3	},
	{ TASK_TLK_EYECONTACT,	(float)0		},
	{ TASK_WAIT_RANDOM,		(float)0.5		},
};

Schedule_t	slIdleSpeak[] =
{
	{ 
		tlIdleSpeak,
		ARRAYSIZE ( tlIdleSpeak ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_CLIENT_PUSH	|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"Idle Speak"
	},
};

Task_t	tlIdleSpeakWait[] =
{
	{ TASK_SET_ACTIVITY,	(float)ACT_SIGNAL3	},// Stop and talk
	{ TASK_TLK_SPEAK,		(float)0		},// question or remark
	{ TASK_TLK_EYECONTACT,	(float)0		},// 
	{ TASK_WAIT,			(float)2		},// wait - used when sci is in 'use' mode to keep head turned
};

Schedule_t	slIdleSpeakWait[] =
{
	{ 
		tlIdleSpeakWait,
		ARRAYSIZE ( tlIdleSpeakWait ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_CLIENT_PUSH	|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"Idle Speak Wait"
	},
};

Task_t	tlIdleHello[] =
{
	{ TASK_SET_ACTIVITY,	(float)ACT_SIGNAL3	},// Stop and talk
	{ TASK_TLK_HELLO,		(float)0		},// Try to say hello to player
	{ TASK_TLK_EYECONTACT,	(float)0		},
	{ TASK_WAIT,			(float)0.5		},// wait a bit
	{ TASK_TLK_HELLO,		(float)0		},// Try to say hello to player
	{ TASK_TLK_EYECONTACT,	(float)0		},
	{ TASK_WAIT,			(float)0.5		},// wait a bit
	{ TASK_TLK_HELLO,		(float)0		},// Try to say hello to player
	{ TASK_TLK_EYECONTACT,	(float)0		},
	{ TASK_WAIT,			(float)0.5		},// wait a bit
	{ TASK_TLK_HELLO,		(float)0		},// Try to say hello to player
	{ TASK_TLK_EYECONTACT,	(float)0		},
	{ TASK_WAIT,			(float)0.5		},// wait a bit

};

Schedule_t	slIdleHello[] =
{
	{ 
		tlIdleHello,
		ARRAYSIZE ( tlIdleHello ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_CLIENT_PUSH	|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT,
		"Idle Hello"
	},
};

Task_t	tlIdleStopShooting[] =
{
	{ TASK_TLK_STOPSHOOTING,	(float)0		},// tell player to stop shooting friend
	// { TASK_TLK_EYECONTACT,		(float)0		},// look at the player
};

Schedule_t	slIdleStopShooting[] =
{
	{ 
		tlIdleStopShooting,
		ARRAYSIZE ( tlIdleStopShooting ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND,
		0,
		"Idle Stop Shooting"
	},
};

Task_t	tlMoveAway[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_MOVE_AWAY_FAIL },
	{ TASK_STORE_LASTPOSITION,		(float)0		},
	{ TASK_MOVE_AWAY_PATH,			(float)100		},
	{ TASK_WALK_PATH_FOR_UNITS,		(float)100		},
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_FACE_PLAYER,				(float)0.5 },
};

Schedule_t	slMoveAway[] =
{
	{
		tlMoveAway,
		ARRAYSIZE ( tlMoveAway ),
		0,
		0,
		"MoveAway"
	},
};


Task_t	tlMoveAwayFail[] =
{
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_FACE_PLAYER,				(float)0.5		},
};

Schedule_t	slMoveAwayFail[] =
{
	{
		tlMoveAwayFail,
		ARRAYSIZE ( tlMoveAwayFail ),
		0,
		0,
		"MoveAwayFail"
	},
};



Task_t	tlMoveAwayFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_TARGET_FACE },
	{ TASK_STORE_LASTPOSITION,		(float)0		},
	{ TASK_MOVE_AWAY_PATH,			(float)100				},
	{ TASK_WALK_PATH_FOR_UNITS,		(float)100		},
	{ TASK_STOP_MOVING,				(float)0		},
	{ TASK_SET_SCHEDULE,			(float)SCHED_TARGET_FACE },
};

Schedule_t	slMoveAwayFollow[] =
{
	{
		tlMoveAwayFollow,
		ARRAYSIZE ( tlMoveAwayFollow ),
		0,
		0,
		"MoveAwayFollow"
	},
};

Task_t	tlTlkIdleWatchClient[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
	{ TASK_TLK_LOOK_AT_CLIENT,	(float)6		},
};

Task_t	tlTlkIdleWatchClientStare[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
	{ TASK_TLK_CLIENT_STARE,	(float)6		},
	{ TASK_TLK_STARE,			(float)0		},
	{ TASK_TLK_IDEALYAW,		(float)0		},// look at who I'm talking to
	{ TASK_FACE_IDEAL,			(float)0		}, 
	{ TASK_SET_ACTIVITY,		(float)ACT_SIGNAL3	},
	{ TASK_TLK_EYECONTACT,		(float)0		},
};

Schedule_t	slTlkIdleWatchClient[] =
{
	{ 
		tlTlkIdleWatchClient,
		ARRAYSIZE ( tlTlkIdleWatchClient ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_CLIENT_PUSH	|
		bits_COND_CLIENT_UNSEEN	|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|
		
		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"TlkIdleWatchClient"
	},

	{ 
		tlTlkIdleWatchClientStare,
		ARRAYSIZE ( tlTlkIdleWatchClientStare ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_CLIENT_PUSH	|
		bits_COND_CLIENT_UNSEEN	|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|
		
		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"TlkIdleWatchClientStare"
	},
};


Task_t	tlTlkIdleEyecontact[] =
{
	{ TASK_TLK_IDEALYAW,	(float)0		},// look at who I'm talking to
	{ TASK_FACE_IDEAL,		(float)0		}, 

	//MODDD - CRITICAL. Removing this part of the IdleEyecontact schedule. What is the point of this?
	//It just makes the animation fidget a lot and inevitably reverting back to idle as soon as possible anyways, even mid animation.
	//{ TASK_SET_ACTIVITY,	(float)ACT_SIGNAL3	},

	{ TASK_TLK_EYECONTACT,	(float)0		},// Wait until speaker is done
};

Schedule_t	slTlkIdleEyecontact[] =
{
	{ 
		tlTlkIdleEyecontact,
		ARRAYSIZE ( tlTlkIdleEyecontact ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_CLIENT_PUSH	|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE,
		0,
		"TlkIdleEyecontact"
	},
};




//MODDD - scedule moved up to here from scientist.cpp (in the .h file so that other talk monster implementations may clearly see it
Task_t	tlStopFollowing[] =
{
	{ TASK_CANT_FOLLOW,		(float)0 },
};

Schedule_t	slStopFollowing[] =
{
	{
		tlStopFollowing,
		ARRAYSIZE ( tlStopFollowing ),
		0,
		0,
		"StopFollowing"
	},
};



DEFINE_CUSTOM_SCHEDULES( CTalkMonster )
{
	slIdleResponse,
	slIdleSpeak,
	slIdleHello,
	slIdleSpeakWait,
	slIdleStopShooting,
	slMoveAway,
	slMoveAwayFollow,
	slMoveAwayFail,
	slTlkIdleWatchClient,
	&slTlkIdleWatchClient[ 1 ],
	slTlkIdleEyecontact,
	//NEW!! moved up from scientist.
	slStopFollowing,
};

IMPLEMENT_CUSTOM_SCHEDULES( CTalkMonster, CBaseMonster );


void CTalkMonster :: SetActivity ( Activity newActivity )
{
	if (newActivity == ACT_IDLE && IsTalking() )
		newActivity = ACT_SIGNAL3;
	
	if ( newActivity == ACT_SIGNAL3 && (LookupActivity ( ACT_SIGNAL3 ) == ACTIVITY_NOT_AVAILABLE))
		newActivity = ACT_IDLE;

	CBaseMonster::SetActivity( newActivity );
}


void CTalkMonster :: StartTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		//MODDD - new, simple.
	case TASK_FOLLOW_SUCCESSFUL:
	{	
		consecutiveFollowFails = 0; //reset to mark success.
		TaskComplete();
		break;
	}
	case TASK_TLK_SPEAK:
		// ask question or make statement
		FIdleSpeak();
		TaskComplete();
		break;

	case TASK_TLK_RESPOND:
		// respond to question
		IdleRespond();
		TaskComplete();
		break;

	case TASK_TLK_HELLO:
		// greet player
		FIdleHello();
		TaskComplete();
		break;
	

	case TASK_TLK_STARE:
		// let the player know I know he's staring at me.
		FIdleStare();
		TaskComplete();
		break;

	case TASK_FACE_PLAYER:
	case TASK_TLK_LOOK_AT_CLIENT:
	case TASK_TLK_CLIENT_STARE:
		// track head to the client for a while.
		m_flWaitFinished = gpGlobals->time + pTask->flData;
		break;

	case TASK_TLK_EYECONTACT:
		break;

	case TASK_TLK_IDEALYAW:
		if (m_hTalkTarget != NULL)
		{

			pev->yaw_speed = 60;
			float yaw = VecToYaw(m_hTalkTarget->pev->origin - pev->origin) - pev->angles.y;

			if (yaw > 180) yaw -= 360;
			if (yaw < -180) yaw += 360;

			if (yaw < 0)
			{
				pev->ideal_yaw = min( yaw + 45, 0 ) + pev->angles.y;
			}
			else
			{
				pev->ideal_yaw = max( yaw - 45, 0 ) + pev->angles.y;
			}
		}
		TaskComplete();
		break;

	case TASK_TLK_HEADRESET:
		// reset head position after looking at something
		m_hTalkTarget = NULL;
		TaskComplete();
		break;

	case TASK_TLK_STOPSHOOTING:
		// tell player to stop shooting
		if(global_pissedNPCs < 1  || !globalPSEUDO_iCanHazMemez){
			PlaySentence( m_szGrp[TLK_NOSHOOT], RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_NORM );
		}else{
			PlaySentence( "BA_POKE_C", 7, VOL_NORM, ATTN_NORM );
		}

		TaskComplete();
		break;

	case TASK_CANT_FOLLOW:

		
		
		consecutiveFollowFails++;  //this counts as one failure.
		
		//MODDD - woa there, let's not be so hasty. It's possible the follow method failed because another NPC was in the way. Wait a little and try again.
		if(consecutiveFollowFails < 5){
			
			//m_IdealActivity = GetStoppedActivity();    //what the TASK_STOP_MOVING task does.
			this->m_IdealActivity = ACT_IDLE;  //safe?


			followAgainTime = gpGlobals->time + 5;
			easyForcePrintLine("OKAY? %d sched: %s", consecutiveFollowFails, getScheduleName());


			//TODO: perhaps unfollow without saying the unfollow phrase if taking damage or fleeing for cover?
			//Don't want to come back out in the open in the middle of gunfire or enemies wandering about.
			RouteClear();

		}else{

		
			//MODDD - extra arg for StopFollowing:  whether to play the generic "unFollow" phrase or not (sometimes overshadows the STOP one that is a bit different, so not this time)
			StopFollowing( FALSE, FALSE );

			//MODDD
			if(global_pissedNPCs < 1){
				PlaySentence( m_szGrp[TLK_STOP], RANDOM_FLOAT(2, 2.5), VOL_NORM, ATTN_NORM );
			}else{
				playInterPissed();
			}


			TaskComplete();
		}

		break;

	case TASK_WALK_PATH_FOR_UNITS:
		m_movementActivity = ACT_WALK;
		break;

	case TASK_MOVE_AWAY_PATH:
		{
			Vector dir = pev->angles;
			dir.y = pev->ideal_yaw + 180;
			Vector move;

			UTIL_MakeVectorsPrivate( dir, move, NULL, NULL );
			dir = pev->origin + move * pTask->flData;
			if ( MoveToLocation( ACT_WALK, 2, dir ) )
			{
				TaskComplete();
			}
			else if ( FindCover( pev->origin, pev->view_ofs, 0, CoverRadius() ) )
			{
				// then try for plain ole cover
				m_flMoveWaitFinished = gpGlobals->time + 2;
				TaskComplete();
			}
			else
			{
				// nowhere to go?
				TaskFail();
			}
		}
		break;

	case TASK_PLAY_SCRIPT:
		m_hTalkTarget = NULL;
		CBaseMonster::StartTask( pTask );
		break;

	default:
		CBaseMonster::StartTask( pTask );
	}
}


void CTalkMonster :: RunTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{

		

	case TASK_CANT_FOLLOW:
		{
		if(gpGlobals->time >= followAgainTime){
			//try again! This should get our own follow schedule (Scientist & barnies override what "SCHED_TARGET_CHASE" yields in GetScheduleOfType)
			ChangeSchedule(GetScheduleOfType(SCHED_TARGET_CHASE));
			//NOTICE!!! NEVER. Call TaskComplete() or TaskFail() following a ChangeSchedule call. It may seem like it makes sense to end
			//the current task by completion or failure, but calling ChangeSchedule already resets the task.
			//Calling TaskComplete() or TaskFail() skips the first task of the new schedule or cancels the entire schedule accordingly.
			//NOT WHAT YOU WANT

			TaskComplete();
		}
		break;
		}
	case TASK_TLK_CLIENT_STARE:
	case TASK_TLK_LOOK_AT_CLIENT:

		edict_t *pPlayer;

		// track head to the client for a while.
		if ( m_MonsterState == MONSTERSTATE_IDLE		&& 
			 !IsMoving()								&&
			 !IsTalking()								)
		{
			// Get edict for one player
			pPlayer = g_engfuncs.pfnPEntityOfEntIndex( 1 );

			if ( pPlayer && !entityHidden(pPlayer) )
			{
				IdleHeadTurn( pPlayer->v.origin );
			}
		}
		else
		{
			// started moving or talking
			TaskFail();
			return;
		}

		if ( pTask->iTask == TASK_TLK_CLIENT_STARE )
		{
			// fail out if the player looks away or moves away.
			if ( ( pPlayer->v.origin - pev->origin ).Length2D() > TLK_STARE_DIST )
			{
				// player moved away.
				TaskFail();
			}

			UTIL_MakeVectors( pPlayer->v.angles );
			if ( UTIL_DotPoints( pPlayer->v.origin, pev->origin, gpGlobals->v_forward ) < m_flFieldOfView )
			{
				// player looked away
				TaskFail();
			}
		}

		if ( gpGlobals->time > m_flWaitFinished )
		{
			TaskComplete();
		}
		break;

	case TASK_FACE_PLAYER:
		{
			// Get edict for one player
			edict_t *pPlayer = g_engfuncs.pfnPEntityOfEntIndex( 1 );


			//easyForcePrintLine("look test? %d", (pPlayer!=NULL)?(pPlayer->v.flags & FL_NOTARGET):0 );

			if ( pPlayer && !entityHidden(pPlayer) )
			{
				MakeIdealYaw ( pPlayer->v.origin );
				ChangeYaw ( pev->yaw_speed );
				IdleHeadTurn( pPlayer->v.origin );
				if ( gpGlobals->time > m_flWaitFinished && FlYawDiff() < 10 )
				{
					TaskComplete();
				}
			}
			else
			{
				TaskFail();
			}
		break;
		}

	case TASK_TLK_EYECONTACT:
		{
			easyForcePrintLine("OH no?! %d %d %d %d", !IsMoving(), IsTalking(), m_hTalkTarget != NULL, !entityHidden(m_hTalkTarget));

			if (!IsMoving() && IsTalking() && m_hTalkTarget != NULL && !entityHidden(m_hTalkTarget) )
			{
				// ALERT( at_console, "waiting %f\n", m_flStopTalkTime - gpGlobals->time );
			
				BOOL isPlayer = m_hTalkTarget->IsPlayer();


				IdleHeadTurn( m_hTalkTarget->pev->origin );
			}
			else
			{
				TaskComplete();
			}
			break;
		}

	case TASK_WALK_PATH_FOR_UNITS:
		{
			float distance;

			distance = (m_vecLastPosition - pev->origin).Length2D();

			// Walk path until far enough away
			if ( distance > pTask->flData || MovementIsComplete() )
			{
				TaskComplete();
				RouteClear();		// Stop moving
			}
			break;
		}
	case TASK_WAIT_FOR_MOVEMENT:
		if (IsTalking() && m_hTalkTarget != NULL && !entityHidden(m_hTalkTarget) )
		{
			// ALERT(at_console, "walking, talking\n");
			IdleHeadTurn( m_hTalkTarget->pev->origin );
		}
		else
		{
			IdleHeadTurn( pev->origin );
			// override so that during walk, a scientist may talk and greet player
			FIdleHello();
			if (RANDOM_LONG(0,m_nSpeak * 20) == 0)
			{
				FIdleSpeak();
			}
		}

		CBaseMonster::RunTask( pTask );
		if (TaskIsComplete())
			IdleHeadTurn( pev->origin );
		break;


	//MODDD - new
	case SCHED_TARGET_CHASE:
		//A talkmonster must provide its own follow method. This point should not be reached in the TalkMonster class.
		easyForcePrintLine("ERROR!!! This talkmonster, %s, did not provide its own SCHED_TARGET_CHASE method.", getClassname());

	break;

	default:
		if (IsTalking() && m_hTalkTarget != NULL && !entityHidden(m_hTalkTarget) )
		{
			IdleHeadTurn( m_hTalkTarget->pev->origin );
		}
		else
		{
			if(global_wildHeads != 1){
				SetBoneController( 0, 0 );
			}
		}
		CBaseMonster::RunTask( pTask );
	}
}



void CTalkMonster::Activate( void ){
	CBaseMonster::Activate();
}
void CTalkMonster::Spawn(void){
	
	if(this->m_flPlayerDamage > 0 && forgiveSomePlayerDamageTime == -1){
		forgiveSomePlayerDamageTime = gpGlobals->time + 5;
	}



	CBaseMonster::Spawn();
}
//MODDD - too inspecific.

BOOL CTalkMonster::getGermanModelRequirement(void){
	return FALSE;
}
const char* CTalkMonster::getGermanModel(void){
	return NULL;
}
const char* CTalkMonster::getNormalModel(void){
	return NULL;
}

//MODDD
void CTalkMonster::setModel(void){
	CTalkMonster::setModel(NULL);
}
void CTalkMonster::setModel(const char* m){
	//nothing broad applies to the specific talker NPCs.
	CBaseMonster::setModel(m);
}


GENERATE_KILLED_IMPLEMENTATION(CTalkMonster)
{
	iAmDead = TRUE;
	// If a client killed me (unless I was already Barnacle'd), make everyone else mad/afraid of him
	if ( (pevAttacker->flags & FL_CLIENT) && m_MonsterState != MONSTERSTATE_PRONE )
	{
		AlertFriends();
		LimitFollowers( CBaseEntity::Instance(pevAttacker), 0 );
	}

	m_hTargetEnt = NULL;
	
	if(scientistTryingToHealMe != NULL ){
		scientistTryingToHealMe->forgetHealNPC();
		scientistTryingToHealMe = NULL;
	}
	//If I am a scientist, I will forget anything I wanted to heal.
	forgetHealNPC();


	// Don't finish that sentence
	StopTalking();
	SetUse( NULL );
	GENERATE_KILLED_PARENT_CALL(CBaseMonster);
}



CBaseEntity	*CTalkMonster::EnumFriends( CBaseEntity *pPrevious, int listNumber, BOOL bTrace )
{
	CBaseEntity *pFriend = pPrevious;
	char *pszFriend;
	TraceResult tr;
	Vector vecCheck;

	pszFriend = m_szFriends[ FriendNumber(listNumber) ];
	while (pFriend = UTIL_FindEntityByClassname( pFriend, pszFriend ))
	{
		//MODDD - also don't talk to the player if using NOTARGET.
		if (pFriend == this || !pFriend->IsAlive() || entityHidden(pFriend) )
			// don't talk to self or dead people
			continue;
		if ( bTrace )
		{
			vecCheck = pFriend->pev->origin;
			vecCheck.z = pFriend->pev->absmax.z;

			UTIL_TraceLine( pev->origin, vecCheck, ignore_monsters, ENT(pev), &tr);
		}
		else
			tr.flFraction = 1.0;

		if (tr.flFraction == 1.0)
		{
			return pFriend;
		}
	}

	return NULL;
}


void CTalkMonster::AlertFriends( void )
{
	CBaseEntity *pFriend = NULL;
	int i;

	// for each friend in this bsp...
	for ( i = 0; i < TLK_CFRIENDS; i++ )
	{
		while (pFriend = EnumFriends( pFriend, i, TRUE ))
		{
			CBaseMonster *pMonster = pFriend->MyMonsterPointer();
			if ( pMonster->IsAlive() )
			{
				// don't provoke a friend that's playing a death animation. They're a goner
				pMonster->m_afMemory |= bits_MEMORY_PROVOKED;
			}
		}
	}
}



void CTalkMonster::ShutUpFriends( void )
{
	CBaseEntity *pFriend = NULL;
	int i;

	// for each friend in this bsp...
	for ( i = 0; i < TLK_CFRIENDS; i++ )
	{
		while (pFriend = EnumFriends( pFriend, i, TRUE ))
		{
			CBaseMonster *pMonster = pFriend->MyMonsterPointer();
			if ( pMonster )
			{
				pMonster->SentenceStop();
			}
		}
	}
}


// UNDONE: Keep a follow time in each follower, make a list of followers in this function and do LRU
// UNDONE: Check this in Restore to keep restored monsters from joining a full list of followers
void CTalkMonster::LimitFollowers( CBaseEntity *pPlayer, int maxFollowers )
{
	CBaseEntity *pFriend = NULL;
	int i, count;

	count = 0;
	// for each friend in this bsp...
	for ( i = 0; i < TLK_CFRIENDS; i++ )
	{
		while (pFriend = EnumFriends( pFriend, i, FALSE ))
		{
			CBaseMonster *pMonster = pFriend->MyMonsterPointer();
			if ( pMonster )
			{
				if ( pMonster->m_hTargetEnt == pPlayer )
				{
					count++;
					if ( count > maxFollowers )
						pMonster->StopFollowing( TRUE );
				}
			}
		}
	}
}


float CTalkMonster::TargetDistance( void )
{
	// If we lose the player, or he dies, return a really large distance
	if ( m_hTargetEnt == NULL || !m_hTargetEnt->IsAlive() )
		return 1e6;

	return (m_hTargetEnt->pev->origin - pev->origin).Length();
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CTalkMonster :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{		
	case SCRIPT_EVENT_SENTENCE_RND1:		// Play a named sentence group 25% of the time
		if (RANDOM_LONG(0,99) < 75)
			break;
		// fall through...
	case SCRIPT_EVENT_SENTENCE:				// Play a named sentence group
		ShutUpFriends();
		PlaySentence( pEvent->options, RANDOM_FLOAT(2.8, 3.4), VOL_NORM, ATTN_IDLE );
		//ALERT(at_console, "script event speak\n");
		break;

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

// monsters derived from ctalkmonster should call this in precache()

void CTalkMonster :: TalkInit( void )
{
	// every new talking monster must reset this global, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)

	CTalkMonster::g_talkWaitTime = 0;

	m_voicePitch = 100;
}	
//=========================================================
// FindNearestFriend
// Scan for nearest, visible friend. If fPlayer is true, look for
// nearest player
//=========================================================
CBaseEntity *CTalkMonster :: FindNearestFriend(BOOL fPlayer)
{
	CBaseEntity *pFriend = NULL;
	CBaseEntity *pNearest = NULL;
	float range = 10000000.0;
	TraceResult tr;
	Vector vecStart = pev->origin;
	Vector vecCheck;
	int i;
	char *pszFriend;
	int cfriends;

	vecStart.z = pev->absmax.z;
	
	if (fPlayer)
		cfriends = 1;
	else
		cfriends = TLK_CFRIENDS;

	// for each type of friend...

	for (i = cfriends-1; i > -1; i--)
	{
		if (fPlayer)
			pszFriend = "player";
		else
			pszFriend = m_szFriends[FriendNumber(i)];

		if (!pszFriend)
			continue;

		// for each friend in this bsp...
		while (pFriend = UTIL_FindEntityByClassname( pFriend, pszFriend ))
		{
			if (pFriend == this || !pFriend->IsAlive() || entityHidden(pFriend) )
				// don't talk to self or dead people
				continue;

			CBaseMonster *pMonster = pFriend->MyMonsterPointer();

			// If not a monster for some reason, or in a script, or prone
			if ( !pMonster || pMonster->m_MonsterState == MONSTERSTATE_SCRIPT || pMonster->m_MonsterState == MONSTERSTATE_PRONE )
				continue;

			vecCheck = pFriend->pev->origin;
			vecCheck.z = pFriend->pev->absmax.z;

			// if closer than previous friend, and in range, see if he's visible

			if (range > (vecStart - vecCheck).Length())
			{
				UTIL_TraceLine(vecStart, vecCheck, ignore_monsters, ENT(pev), &tr);

				if (tr.flFraction == 1.0)
				{
					// visible and in range, this is the new nearest scientist
					if ((vecStart - vecCheck).Length() < TALKRANGE_MIN)
					{
						pNearest = pFriend;
						range = (vecStart - vecCheck).Length();
					}
				}
			}
		}
	}
	return pNearest;
}

int CTalkMonster :: GetVoicePitch( void )
{
	//MODDD - check for "pissedNPCs".  If activated, do NOT allow pitch adjustments (only pitch of 100, play the original sound unmodded)
	if(global_pissedNPCs < 1 ){
		//normal.
		return m_voicePitch + RANDOM_LONG(0,3);
	}else{
		//always speek in pitch 100.  Some of the custom sounds are odd if not.
		return 100;
	}
	

}


void CTalkMonster :: Touch( CBaseEntity *pOther )
{
	// Did the player touch me?
	if ( pOther->IsPlayer() )
	{
		// Ignore if pissed at player
		if ( m_afMemory & bits_MEMORY_PROVOKED )
			return;

		// Stay put during speech
		if ( IsTalking() )
			return;

		// Heuristic for determining if the player is pushing me away
		float speed = fabs(pOther->pev->velocity.x) + fabs(pOther->pev->velocity.y);
		if ( speed > 50 )
		{
			SetConditions( bits_COND_CLIENT_PUSH );
			MakeIdealYaw( pOther->pev->origin );
		}
	}
}



//=========================================================
// IdleRespond
// Respond to a previous question
//=========================================================
void CTalkMonster :: IdleRespond( void )
{
	int pitch = GetVoicePitch();
	
	// play response
	if(global_pissedNPCs < 1){
		PlaySentence( m_szGrp[TLK_ANSWER], RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
	}else{
		playInterPissed();
	}
}

int CTalkMonster :: FOkToSpeak( void )
{
	// if in the grip of a barnacle, don't speak
	if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE )
	{
		return FALSE;
	}

	// if not alive, certainly don't speak
	if ( pev->deadflag != DEAD_NO )
	{
		return FALSE;
	}

	// if someone else is talking, don't speak
	if (gpGlobals->time <= CTalkMonster::g_talkWaitTime)
		return FALSE;

	if ( pev->spawnflags & SF_MONSTER_GAG )
		return FALSE;

	if ( m_MonsterState == MONSTERSTATE_PRONE )
		return FALSE;

	// if player is not in pvs, don't speak
	if (!IsAlive() || FNullEnt(FIND_CLIENT_IN_PVS(edict())))
		return FALSE;

	// don't talk if you're in combat
	if (m_hEnemy != NULL && FVisible( m_hEnemy ))
		return FALSE;

	return TRUE;
}


//MODDD - this is a version that allows speaking during combat.
int CTalkMonster :: FOkToSpeakAllowCombat( float waitTime )
{
	// if in the grip of a barnacle, don't speak
	if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE )
	{
		if(global_barneyPrintouts==1){
		easyPrintLine("BARNEY ALERT FAIL 1");
		}
		return FALSE;
	}

	// if not alive, certainly don't speak
	if ( pev->deadflag != DEAD_NO )
	{
		if(global_barneyPrintouts==1){
		easyPrintLine("BARNEY ALERT FAIL 2");
		}
		return FALSE;
	}

	// if someone else is talking, don't speak
	//MODDD - now uses a custom wait time.
	//if (gpGlobals->time <= CTalkMonster::g_talkWaitTime){
	if (gpGlobals->time <= waitTime){
		if(global_barneyPrintouts==1){
		easyPrintLine("BARNEY ALERT FAIL 3 %.2f, %.2f, %.2f", gpGlobals->time, waitTime, waitTime - gpGlobals->time);
		}
		return FALSE;
	}

	if ( pev->spawnflags & SF_MONSTER_GAG ){
		if(global_barneyPrintouts==1){
		easyPrintLine("BARNEY ALERT FAIL 4");
		}
		return FALSE;
	}

	if ( m_MonsterState == MONSTERSTATE_PRONE ){
		if(global_barneyPrintouts==1){
		easyPrintLine("BARNEY ALERT FAIL 5");
		}
		return FALSE;
	}

	// if player is not in pvs, don't speak
	if (!IsAlive() || FNullEnt(FIND_CLIENT_IN_PVS(edict()))){
		if(global_barneyPrintouts==1){
		easyPrintLine("BARNEY ALERT FAIL 6");
		}
		return FALSE;
	}

	// don't talk if you're in combat ... NOPE.
	//if (m_hEnemy != NULL && FVisible( m_hEnemy ))
	//	return FALSE;


	return TRUE;
}

int CTalkMonster::CanPlaySentence( BOOL fDisregardState ) 
{ 
	if ( fDisregardState )
		return CBaseMonster::CanPlaySentence( fDisregardState );
	return FOkToSpeak(); 
}

//=========================================================
// FIdleStare
//=========================================================
int CTalkMonster :: FIdleStare( void )
{
	if (!FOkToSpeak())
		return FALSE;

	//MODDD
	if(global_pissedNPCs < 1){
		PlaySentence( m_szGrp[TLK_STARE], RANDOM_FLOAT(5, 7.5), VOL_NORM, ATTN_IDLE );
	}else{
		playPissed();
	}


	
	m_hTalkTarget = FindNearestFriend( TRUE );
	return TRUE;
}



//=========================================================
// IdleHello
// Try to greet player first time he's seen
//=========================================================
int CTalkMonster :: FIdleHello( void )
{
	if (!FOkToSpeak())
		return FALSE;

	// if this is first time scientist has seen player, greet him
	if (!FBitSet(m_bitsSaid, bit_saidHelloPlayer))
	{
		// get a player
		CBaseEntity *pPlayer = FindNearestFriend(TRUE);

		if (pPlayer && !entityHidden(pPlayer) )
		{
			if (FInViewCone(pPlayer) && FVisible(pPlayer))
			{
				m_hTalkTarget = pPlayer;

				if(global_pissedNPCs < 1){
					if (FBitSet(pev->spawnflags, SF_MONSTER_PREDISASTER))
						PlaySentence( m_szGrp[TLK_PHELLO], RANDOM_FLOAT(3, 3.5), VOL_NORM,  ATTN_IDLE );
					else
						PlaySentence( m_szGrp[TLK_HELLO], RANDOM_FLOAT(3, 3.5), VOL_NORM,  ATTN_IDLE );
				}else{
					playPissed();
				}

				SetBits(m_bitsSaid, bit_saidHelloPlayer);
				
				return TRUE;
			}
		}
	}
	return FALSE;
}


// turn head towards supplied origin
void CTalkMonster :: IdleHeadTurn( Vector &vecFriend )
{
	 // turn head in desired direction only if ent has a turnable head
	if (m_afCapability & bits_CAP_TURN_HEAD)
	{
		float yaw = VecToYaw(vecFriend - pev->origin) - pev->angles.y;

		if (yaw > 180) yaw -= 360;
		if (yaw < -180) yaw += 360;

		// turn towards vector
		if(global_wildHeads != 1){
			SetBoneController( 0, yaw );
		}
	}
}

//=========================================================
// FIdleSpeak
// ask question of nearby friend, or make statement
//=========================================================
int CTalkMonster :: FIdleSpeak ( void )
{ 
	// try to start a conversation, or make statement
	int pitch;
	const char *szIdleGroup;
	const char *szQuestionGroup;
	float duration;

	if (!FOkToSpeak())
		return FALSE;

	// set idle groups based on pre/post disaster
	if (FBitSet(pev->spawnflags, SF_MONSTER_PREDISASTER))
	{
		szIdleGroup = m_szGrp[TLK_PIDLE];
		szQuestionGroup = m_szGrp[TLK_PQUESTION];
		// set global min delay for next conversation
		duration = RANDOM_FLOAT(4.8, 5.2);
	}
	else
	{
		szIdleGroup = m_szGrp[TLK_IDLE];
		szQuestionGroup = m_szGrp[TLK_QUESTION];
		// set global min delay for next conversation
		duration = RANDOM_FLOAT(2.8, 3.2);

	}

	pitch = GetVoicePitch();
		
	// player using this entity is alive and wounded?
	CBaseEntity *pTarget = m_hTargetEnt;

	if ( pTarget != NULL )
	{
		if ( pTarget->IsPlayer() )
		{
			if ( pTarget->IsAlive() )
			{
				m_hTalkTarget = m_hTargetEnt;
				if (!FBitSet(m_bitsSaid, bit_saidDamageHeavy) && 
					(m_hTargetEnt->pev->health <= m_hTargetEnt->pev->max_health / 8))
				{
					//EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, m_szGrp[TLK_PLHURT3], 1.0, ATTN_IDLE, 0, pitch);
					PlaySentence( m_szGrp[TLK_PLHURT3], duration, VOL_NORM, ATTN_IDLE );
					SetBits(m_bitsSaid, bit_saidDamageHeavy);
					return TRUE;
				}
				else if (!FBitSet(m_bitsSaid, bit_saidDamageMedium) && 
					(m_hTargetEnt->pev->health <= m_hTargetEnt->pev->max_health / 4))
				{
					//EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, m_szGrp[TLK_PLHURT2], 1.0, ATTN_IDLE, 0, pitch);
					PlaySentence( m_szGrp[TLK_PLHURT2], duration, VOL_NORM, ATTN_IDLE );
					SetBits(m_bitsSaid, bit_saidDamageMedium);
					return TRUE;
				}
				else if (!FBitSet(m_bitsSaid, bit_saidDamageLight) &&
					(m_hTargetEnt->pev->health <= m_hTargetEnt->pev->max_health / 2))
				{
					//EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, m_szGrp[TLK_PLHURT1], 1.0, ATTN_IDLE, 0, pitch);
					PlaySentence( m_szGrp[TLK_PLHURT1], duration, VOL_NORM, ATTN_IDLE );
					SetBits(m_bitsSaid, bit_saidDamageLight);
					return TRUE;
				}
			}
			else
			{
				//!!!KELLY - here's a cool spot to have the talkmonster talk about the dead player if we want.
				// "Oh dear, Gordon Freeman is dead!" -Scientist
				// "Damn, I can't do this without you." -Barney
			}
		}
	}

	// if there is a friend nearby to speak to, play sentence, set friend's response time, return
	CBaseEntity *pFriend = FindNearestFriend(FALSE);

	if (pFriend && !(pFriend->IsMoving()) && (RANDOM_LONG(0,99) < 75))
	{
		if(global_pissedNPCs < 1){
			PlaySentence( szQuestionGroup, duration, VOL_NORM, ATTN_IDLE );
			//SENTENCEG_PlayRndSz( ENT(pev), szQuestionGroup, 1.0, ATTN_IDLE, 0, pitch );
		}else{
			playInterPissed();
		}


		// force friend to answer
		CTalkMonster *pTalkMonster = (CTalkMonster *)pFriend;
		m_hTalkTarget = pFriend;
		pTalkMonster->SetAnswerQuestion( this ); // UNDONE: This is EVIL!!!
		pTalkMonster->m_flStopTalkTime = m_flStopTalkTime;

		m_nSpeak++;
		return TRUE;
	}

	// otherwise, play an idle statement, try to face client when making a statement.
	if ( RANDOM_LONG(0,1) )
	{
		//SENTENCEG_PlayRndSz( ENT(pev), szIdleGroup, 1.0, ATTN_IDLE, 0, pitch );
		CBaseEntity *pFriend = FindNearestFriend(TRUE);

		if ( pFriend )
		{
			m_hTalkTarget = pFriend;
			
			if(global_pissedNPCs < 1){
				PlaySentence( szIdleGroup, duration, VOL_NORM, ATTN_IDLE );
			}else{
				playInterPissed();
			}
			
			m_nSpeak++;
			return TRUE;
		}
	}

	// didn't speak
	Talk( 0 );
	CTalkMonster::g_talkWaitTime = 0;
	return FALSE;
}

void CTalkMonster::PlayScriptedSentence( const char *pszSentence, float duration, float volume, float attenuation, BOOL bConcurrent, CBaseEntity *pListener )
{
	if ( !bConcurrent )
		ShutUpFriends();

	ClearConditions( bits_COND_CLIENT_PUSH );	// Forget about moving!  I've got something to say!
	m_useTime = gpGlobals->time + duration;
	PlaySentence( pszSentence, duration, volume, attenuation );

	m_hTalkTarget = pListener;
}

void CTalkMonster::PlaySentence( const char *pszSentence, float duration, float volume, float attenuation )
{

	//easyPrintLine("PLAYING SENTENC %s", pszSentence);

	
	if ( !pszSentence )
		return;

	Talk ( duration );

	CTalkMonster::g_talkWaitTime = gpGlobals->time + duration + 2.0;
	if ( pszSentence[0] == '!' ){
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, pszSentence, volume, attenuation, 0, GetVoicePitch());
	}else{
		SENTENCEG_PlayRndSz( edict(), pszSentence, volume, attenuation, 0, GetVoicePitch() );
	}

	// If you say anything, don't greet the player - you may have already spoken to them
	SetBits(m_bitsSaid, bit_saidHelloPlayer);
}

//=========================================================
// Talk - set a timer that tells us when the monster is done
// talking.
//=========================================================
void CTalkMonster :: Talk( float flDuration )
{
	if ( flDuration <= 0 )
	{
		// no duration :( 
		m_flStopTalkTime = gpGlobals->time + 3;
	}
	else
	{
		m_flStopTalkTime = gpGlobals->time + flDuration;
	}
}

// Prepare this talking monster to answer question
void CTalkMonster :: SetAnswerQuestion( CTalkMonster *pSpeaker )
{
	if ( !m_pCine )
		ChangeSchedule( slIdleResponse );
	m_hTalkTarget = (CBaseMonster *)pSpeaker;
}








//Implementable methods. Let a monster say what it needs to when (no default behavior)...
//Provoked while declining a follow request. Scream or act really pissed.
void CTalkMonster::DeclineFollowingProvoked(CBaseEntity* pCaller){
	
}
//Provoked: this monster has turned on the player from too much or too direct friendly fire.
void CTalkMonster::SayProvoked(void){
	
}
//Suspicious: this monster is warning the player not to make that friendly fire mistake again.
void CTalkMonster::SaySuspicious(void){
	
}
void CTalkMonster::SayLeaderDied(void){

}


GENERATE_TRACEATTACK_IMPLEMENTATION(CTalkMonster)
{
	GENERATE_TRACEATTACK_PARENT_CALL(CBaseMonster);
}

GENERATE_TAKEDAMAGE_IMPLEMENTATION(CTalkMonster)
{

	int ret = GENERATE_TAKEDAMAGE_PARENT_CALL(CBaseMonster);
	if ( !IsAlive() ){
		//End early - no sense in response logic if we're unable to do anything.
		return ret;
	}


	
	if(m_MonsterState == MONSTERSTATE_SCRIPT && (m_pCine && !m_pCine->CanInterrupt() ) ){
		//in script, ignore already.
		return ret;
	}
	




	//MODDD - this is what used to be all there was to a generic CTalkMonster's TakeDamage method.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// make sure friends talk about it if player hurts talkmonsters...
	// if player damaged this entity, have other friends talk about it
	if (pevAttacker && m_MonsterState != MONSTERSTATE_PRONE && FBitSet(pevAttacker->flags, FL_CLIENT))
	{
		CBaseEntity *pFriend = FindNearestFriend(FALSE);

		if (pFriend && pFriend->IsAlive())
		{
			// only if not dead or dying!
			CTalkMonster *pTalkMonster = (CTalkMonster *)pFriend;
			pTalkMonster->ChangeSchedule( slIdleStopShooting );
		}
	}
	//MODDD noTODO - instantly turn friend hostile towards the player if killed by the player? I think that already happens in another place.
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



	//MODDD - Barney's logic generalized here as its a bit smarter. The Scientist would benefit from it.

	if ( m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) )
	{
		m_flPlayerDamage += flDamage;

		
		//MODDD - new factoring in m_flPlayerDamage.
		//        If the player has done too much damage, despite the SUSPICIOUS flag being off,
		//        Barney will turn hostile. m_flPlayerDamage will slowly go down over time too,
		//        like the SUSPICIOUS flag but is mutually exclusive.
		//        That is, firing two AR grenades that do a tiny bit of damage 30 seconds apart 
		//        can trigger a Barney, and so can firing two AR grenades that do a lot of damage 
		//        more than a minute apart (m_flPlayerDamge hasn't receded enough).
		
		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( m_hEnemy == NULL )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			//MODDD - extra provoking condition - taken too much cumulative damage from the player.
			if ( flDamage > 32 ||   //too much damage at one time? Pissed.
				 m_flPlayerDamage > 50 ||  //too much damage over a long time? Pissed.
				(m_afMemory & bits_MEMORY_SUSPICIOUS && m_flPlayerDamage > 8) || //two shots too soon + a little damage? Pissed.
				UTIL_IsFacing( pevAttacker, pev->origin ) && m_flPlayerDamage > 4 //Facing me when he did it with a lower damage tolerance? Pissed.
				) 
			{
				this->SayProvoked();
				// Alright, now I'm pissed!
				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
				//No more forgiveness.
				forgiveSuspiciousTime = -1;
				forgiveSomePlayerDamageTime = -1;
			}
			else
			{
				// Hey, be careful with that
				this->SaySuspicious();
				Remember( bits_MEMORY_SUSPICIOUS );
				//After almost a minute of being suspicious, forget about it.
				//Can't afford to hold a grudge in these times.
				forgiveSuspiciousTime = gpGlobals->time + 50;
				forgiveSomePlayerDamageTime = gpGlobals->time + 5;
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) )
		{
			//In this case, we already have an enemy. Assume the player misfired and meant to hit an enemy. But we aren't stupid.
			//Do the same check as above but be more tolerant because it's in the heat of combat.
			//SaySuspicious();
			if ( flDamage > 50 ||   //too much damage at one time? Pissed. Wait, how do you survive this?
				 m_flPlayerDamage > 50 ||  //too much damage over a long time? Pissed.
				(m_afMemory & bits_MEMORY_SUSPICIOUS && m_flPlayerDamage > 8) || //two shots too soon + a little damage? Pissed.
				UTIL_IsFacing( pevAttacker, pev->origin ) && m_flPlayerDamage > 4 //Facing me when he did it with a lower damage tolerance? Pissed.
				) 
			{
				this->SayProvoked();
				// Alright, now I'm pissed!
				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
				//No more forgiveness.
				forgiveSuspiciousTime = -1;
				forgiveSomePlayerDamageTime = -1;
			}
			else
			{
				// Hey, be careful with that
				this->SaySuspicious();
				Remember( bits_MEMORY_SUSPICIOUS );
				//After almost a minute of being suspicious, forget about it.
				//Can't afford to hold a grudge in these times.
				forgiveSuspiciousTime = gpGlobals->time + 50;
				forgiveSomePlayerDamageTime = gpGlobals->time + 5;
			}
		}
	}

	return ret;
}






Schedule_t* CTalkMonster :: GetScheduleOfType ( int Type )
{


	
	easyForcePrintLine("%s:%d WHATS GOOD IM CTalkMonster AND I PICKED SCHED TYPE %d", getClassname(), monsterID, Type);

	canGoRavingMad = FALSE; //by default.	
	BOOL passCondition = 0;

	//easyPrintLine("TALKMONSTER SCHED OF TYPE %d", Type);
	switch( Type )
	{
	//MODDD
	case SCHED_CANT_FOLLOW:
		return slStopFollowing;
	case SCHED_MOVE_AWAY:
		return slMoveAway;

	case SCHED_MOVE_AWAY_FOLLOW:
		return slMoveAwayFollow;

	case SCHED_MOVE_AWAY_FAIL:
		return slMoveAwayFail;

	case SCHED_TARGET_FACE:
		// speak during 'use'



		//MODDD
		if(global_NPCsTalkMore != 1 ){
			passCondition = (RANDOM_LONG(0,99) < 2);
		}else{
			passCondition = TRUE;
		}
		
		canGoRavingMad = TRUE;
		
		if (passCondition){
			//ALERT ( at_console, "target chase speak\n" );
			return slIdleSpeakWait;
		}else{
			return slIdleStand;
		}



		/*
		if (RANDOM_LONG(0,99) < 2)
			//ALERT ( at_console, "target chase speak\n" );
			return slIdleSpeakWait;
		else
			return slIdleStand;
		*/

	case SCHED_IDLE_STAND:
		{
			canGoRavingMad = TRUE;
		
			// if never seen player, try to greet him
			if (!FBitSet(m_bitsSaid, bit_saidHelloPlayer))
			{
				return slIdleHello;
			}

			// sustained light wounds?
			if (!FBitSet(m_bitsSaid, bit_saidWoundLight) && (pev->health <= (pev->max_health * 0.75)))
			{
				//SENTENCEG_PlayRndSz( ENT(pev), m_szGrp[TLK_WOUND], 1.0, ATTN_IDLE, 0, GetVoicePitch() );
				//CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(2.8, 3.2);
				PlaySentence( m_szGrp[TLK_WOUND], RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
				SetBits(m_bitsSaid, bit_saidWoundLight);
				return slIdleStand;
			}
			// sustained heavy wounds?
			else if (!FBitSet(m_bitsSaid, bit_saidWoundHeavy) && (pev->health <= (pev->max_health * 0.5)))
			{
				//SENTENCEG_PlayRndSz( ENT(pev), m_szGrp[TLK_MORTAL], 1.0, ATTN_IDLE, 0, GetVoicePitch() );
				//CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(2.8, 3.2);
				PlaySentence( m_szGrp[TLK_MORTAL], RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
				SetBits(m_bitsSaid, bit_saidWoundHeavy);
				return slIdleStand;
			}

			// talk about world
			if (FOkToSpeak() && RANDOM_LONG(0,m_nSpeak * 2) == 0)
			{
				//ALERT ( at_console, "standing idle speak\n" );
				return slIdleSpeak;
			}
			
			if ( !IsTalking() && HasConditions ( bits_COND_SEE_CLIENT ) && RANDOM_LONG( 0, 6 ) == 0 )
			{
				edict_t *pPlayer = g_engfuncs.pfnPEntityOfEntIndex( 1 );

				if ( pPlayer && !entityHidden(pPlayer) )
				{
					// watch the client.
					UTIL_MakeVectors ( pPlayer->v.angles );
					if ( ( pPlayer->v.origin - pev->origin ).Length2D() < TLK_STARE_DIST	&& 
						 UTIL_DotPoints( pPlayer->v.origin, pev->origin, gpGlobals->v_forward ) >= m_flFieldOfView )
					{
						// go into the special STARE schedule if the player is close, and looking at me too.
						return &slTlkIdleWatchClient[ 1 ];
					}

					return slTlkIdleWatchClient;
				}
			}
			else
			{
				if (IsTalking()){
					// look at who we're talking to
					//...can't go mad here though.
					canGoRavingMad = FALSE;
					return slTlkIdleEyecontact;
				}else{
					// regular standing idle
					return slIdleStand;
				}
			}


			// NOTE - caller must first CTalkMonster::GetScheduleOfType, 
			// then check result and decide what to return ie: if sci gets back
			// slIdleStand, return slIdleSciStand
			
		}
		break;
		//case SCHED_ALERT_FACE:
		case SCHED_ALERT_STAND:
		{
			canGoRavingMad = TRUE;
			return CBaseMonster::GetScheduleOfType( Type );
		}
		break;

	}

	return CBaseMonster::GetScheduleOfType( Type );
}

//=========================================================
// IsTalking - am I saying a sentence right now?
//=========================================================
BOOL CTalkMonster :: IsTalking( void )
{
	if ( m_flStopTalkTime > gpGlobals->time )
	{
		return TRUE;
	}

	return FALSE;
}


CTalkMonster::CTalkMonster(void){
	nextMadEffect = -1;
	madYaw = 0;
	
	//for NOW!
	//goneMad = TRUE;
	madDir = FALSE;
	canGoRavingMad = FALSE;
	scientistTryingToHealMe = NULL;

	//madInterSentencesMaxLocation = NULL;
	madInterSentencesLocation = NULL;

	m_flPlayerDamage = 0;
	forgiveSuspiciousTime = -1;
	forgiveSomePlayerDamageTime = -1;

	leaderRecentlyDied = FALSE;
	
	followAgainTime = -1;
	consecutiveFollowFails = -1;
	followResetFailTime = -1;

}






void CTalkMonster::MonsterThink(void){


	if(global_peopleStrobe == 1){
		if(nextMadEffect <= gpGlobals->time){
			//send effect!
			UTIL_generateFreakyLight(pev->origin);

			nextMadEffect = gpGlobals->time + global_raveEffectSpawnInterval;
		}
	}




	if(global_wildHeads == 1){
		
		//easyPrintLine("GGGG %.2f", madYaw);


		/*
		if(madDir == FALSE){
			madYaw -= (45*3)*0.1;
		}else{
			madYaw += (45*3)*0.7;
		}
		
		
		if (madYaw >= 45){
			//madYaw -= 360;
			madDir = FALSE;
			madYaw = 45;
		}
		if (madYaw <= -45){
			//madYaw += 360;
			madDir = TRUE;
			madYaw = -45;
		}
		*/


		//The RAW bone control value is between 0 and 255, inclusive.
		/*
		madYaw += 5;

		if(madYaw > 255){
			madYaw = 0;
		}
		*/

		madYaw = RANDOM_LONG(0, 255);


		/*
		if(madYaw >= 360){
			madYaw -= 360;
		}
		if(madYaw < 0){
			madYaw += 360;
		}
		*/

		/*
		if(madYaw >= 360){
			madYaw -= 360;
		}
		*/
		SetBoneControllerUnsafe( 0, madYaw );
		SetBoneControllerUnsafe( 1, 100 );
		
		//spawn effects every so often?


	}//END OF if(global_wildHeads)


	if(this->m_pSchedule == NULL){
		//easyPrintLine("DERYRRR.");
	}else{
		//easyPrintLine("talkmonstersched: %d %s %s", (this->m_pSchedule==slIdleStand), m_pSchedule->pName, slIdleStand->pName);
	}


	//sitting scientist should not attempt this.  either doesn't work or just... weirder than usual.
	if(canGoRavingMad && !FClassnameIs(pev, "monster_sitting_scientist") ){

		if(global_thatWasntPunch == 1){
			//pev->angles[0] 
			//UTIL_printLineVector("STUFFFFF", pev->angles);

			pev->angles[0] = RANDOM_LONG(-30, 30);
			pev->angles[1] = RANDOM_LONG(0, 359);
			//pev->angles[2] = RANDOM_LONG(0, 359);
		}

	}else{
		//nah, gonna bleep with ya.  Leave em' slanted funny.
		//pev->angles[0] = 0;
	}








	//OK. Let's have some halfway-normal think logic in here.

	if(forgiveSomePlayerDamageTime != -1 && gpGlobals->time >= forgiveSomePlayerDamageTime){
		
		m_flPlayerDamage = max(m_flPlayerDamage - 2 , 0);

		if(this->m_flPlayerDamage > 0){
			//more to go? Reset the timer.
			forgiveSomePlayerDamageTime = gpGlobals->time + 5;
		}else{
			//No more damage to recover.
			forgiveSomePlayerDamageTime = -1;
		}
	}

	if(!HasMemory(bits_MEMORY_PROVOKED)){
		//if unprovoked but suspicious... forget being suspicious after a while.
		if(this->forgiveSuspiciousTime != -1 && gpGlobals->time >= forgiveSuspiciousTime){
			//ok, forget about being suspicious.
			this->Forget(bits_MEMORY_SUSPICIOUS);
			forgiveSuspiciousTime = -1;
		}
	}







	CBaseMonster::MonsterThink();
}





//=========================================================
// If there's a player around, watch him.
//=========================================================
void CTalkMonster :: PrescheduleThink ( void )
{

	if ( !HasConditions ( bits_COND_SEE_CLIENT ) )
	{
		SetConditions ( bits_COND_CLIENT_UNSEEN );
	}
}

// try to smell something
void CTalkMonster :: TrySmellTalk( void )
{
	if ( !FOkToSpeak() )
		return;

	// clear smell bits periodically
	if ( gpGlobals->time > m_flLastSaidSmelled  )
	{
//		ALERT ( at_aiconsole, "Clear smell bits\n" );
		ClearBits(m_bitsSaid, bit_saidSmelled);
	}
	// smelled something?
	if (!FBitSet(m_bitsSaid, bit_saidSmelled) && HasConditions ( bits_COND_SMELL ))
	{
		PlaySentence( m_szGrp[TLK_SMELL], RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
		m_flLastSaidSmelled = gpGlobals->time + 60;// don't talk about the stinky for a while.
		SetBits(m_bitsSaid, bit_saidSmelled);
	}
}



int CTalkMonster::IRelationship( CBaseEntity *pTarget )
{
	if(!UTIL_IsAliveEntity(pTarget)){
		return R_NO;
	}

	if ( pTarget->IsPlayer() )
		if ( m_afMemory & bits_MEMORY_PROVOKED )
			return R_HT;
	return CBaseMonster::IRelationship( pTarget );
}




void CTalkMonster::StopFollowing( BOOL clearSchedule ){
	//by default, play the unuse sentence.
	CTalkMonster::StopFollowing(clearSchedule, TRUE);
}

void CTalkMonster::StopFollowing( BOOL clearSchedule, BOOL playUnuseSentence )
{
	if ( IsFollowing() )
	{

		consecutiveFollowFails = 0;  //resets the follower fail counter.


		if ( !(m_afMemory & bits_MEMORY_PROVOKED) )
		{
			//MODDD
			if(playUnuseSentence == TRUE){
				if(global_pissedNPCs < 1){
					PlaySentence( m_szGrp[TLK_UNUSE], RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
				}else{
					playPissed();
				}
			}
			
			m_hTalkTarget = m_hTargetEnt;
		}

		if ( m_movementGoal == MOVEGOAL_TARGETENT )
			RouteClear(); // Stop him from walking toward the player
		m_hTargetEnt = NULL;
		if ( clearSchedule )
			ClearSchedule();
		if ( m_hEnemy != NULL )
			m_IdealMonsterState = MONSTERSTATE_COMBAT;
	}
}


void CTalkMonster::StartFollowing( CBaseEntity *pLeader )
{

	//just in case.
	consecutiveFollowFails = 0;
	//if(m_pSchedule == slStopFollowing){TaskFail();}  //if we're waiting for this delay to pass, just stop.
	//...actually no need, any start / stop by the player pressing Use on talkmonsters already resets the schedule.

	if ( m_pCine )
		m_pCine->CancelScript();

	if ( m_hEnemy != NULL )
		m_IdealMonsterState = MONSTERSTATE_ALERT;

	m_hTargetEnt = pLeader;
	//MODDD
	if(global_pissedNPCs < 1){
		PlaySentence( m_szGrp[TLK_USE], RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
	}else{
		playPissed();
	}
	m_hTalkTarget = m_hTargetEnt;
	ClearConditions( bits_COND_CLIENT_PUSH );
	ClearSchedule();
}

//MODDD - new
void CTalkMonster::playPissed(){
	
	if(global_pissedNPCs == 2){
		PlaySentence("!testsentence", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
		return;
	}
	
	if(madSentencesMax > 0){

		long someRand;

		//chance for other groups too.
		if(RANDOM_LONG(0, 4) == 0){
			someRand = RANDOM_LONG(0, madSentencesMax-1);
		}else{
			someRand = 0;
		}

		char toPlay[128];
		toPlay[127] = '\0';

		if(someRand == 0){
			sprintf(toPlay, "!%s%d", madSentences[someRand], RANDOM_LONG(0, getMadSentencesMax() - 1 ) );
		}else{
			sprintf(toPlay, "%s", madSentences[someRand]);
		}

		PlaySentence( toPlay, RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
	}
}




//const char* CTalkMonster::madInterSentences[] = {"!testsentence"};
//int CTalkMonster::madInterSentencesMax = 1;




void CTalkMonster::playInterPissed(){

	
	if(global_pissedNPCs == 2){
		PlaySentence("!testsentence", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
		return;

	}

	int madInterSentencesMax = getMadInterSentencesMax();
	if(madInterSentencesMax > 0 && madInterSentencesLocation != NULL){
		long someRand = RANDOM_LONG(0, madInterSentencesMax-1);
		//easyPrintLine("IM %s, MAH CHOICES: %d :: DID %d", getClassname(), *madInterSentencesMaxLocation, someRand);
		PlaySentence( madInterSentencesLocation[someRand], RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
		
	}
}



BOOL CTalkMonster::CanFollow( void )
{

	if(EASY_CVAR_GET(playerFollowerMax) <= 0){
		return FALSE; //follower limit non-positive? Can't do it.
	}

	if ( m_MonsterState == MONSTERSTATE_SCRIPT )
	{
		if ( !m_pCine->CanInterrupt() )
			return FALSE;
	}
	
	if ( !IsAlive() )
		return FALSE;

	return !IsFollowing();
}


void CTalkMonster :: FollowerUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{

	// Don't allow use during a scripted_sentence
	if ( m_useTime > gpGlobals->time )
		return;


	
	//easyPrintLine("TALK DEBUG 1???");





	if ( pCaller != NULL && pCaller->IsPlayer() )
	{
		//easyPrintLine("TALK DEBUG 2???");
		// Pre-disaster followers can't be used
		if ( pev->spawnflags & SF_MONSTER_PREDISASTER )
		{
			//easyPrintLine("TALK DEBUG 3a???");
			DeclineFollowing();
		}
		else if ( CanFollow() )
		{


			//easyPrintLine("TALK DEBUG 3b???");
			//why minus 1? Because this is BEFORE letting the new NPC follow the player. So at a max value of "1", it's actually treated as a real max of 2.
			//Why? because before letting a new NPC follow the player, if one is following, the count is still 1. Then we add the new follower without doing the check again.
			//This guarantees that the most recent follow request gets to follow the player (without getting unfollowed right after) no matter what.
			LimitFollowers( pCaller , EASY_CVAR_GET(playerFollowerMax)-1 );

			if ( m_afMemory & bits_MEMORY_PROVOKED ){
				ALERT( at_console, "I'm not following you, you evil person!\n" );
				//MODDD - we can do even better than just a console printout.  
				DeclineFollowingProvoked(pCaller);
			}
			else
			{
				StartFollowing( pCaller );
				SetBits(m_bitsSaid, bit_saidHelloPlayer);	// Don't say hi after you've started following
			}

			
		}
		else
		{
			StopFollowing( TRUE );
		}
	}
}

void CTalkMonster::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "UseSentence"))
	{
		m_iszUse = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "UnUseSentence"))
	{
		m_iszUnUse = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}


void CTalkMonster::Precache( void )
{

	//easyForcePrintLine("HEY THIS IS %s AND WELCOME TO JACKASS %s", this->getClassname(), STRING( m_iszUse ));

	//NOTE: no need for sentence-sound-save stuff here really.
	if ( m_iszUse )
		m_szGrp[TLK_USE] = STRING( m_iszUse );
	if ( m_iszUnUse )
		m_szGrp[TLK_UNUSE] = STRING( m_iszUnUse );
}

BOOL CTalkMonster::isTalkMonster(void){
	return TRUE;
}


void CTalkMonster::forgetHealNPC(void){
	//By default, doesn't do anything.  Scientists use this.
}


void CTalkMonster::ReportAIState(){
	CBaseMonster::ReportAIState();

	easyForcePrintLine("TALKMONSTER: consecutiveFollowFails:%d", consecutiveFollowFails);
}