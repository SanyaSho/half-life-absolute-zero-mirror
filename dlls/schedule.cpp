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
// schedule.cpp - functions and data pertaining to the 
// monsters' AI scheduling system.
//=========================================================
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "basemonster.h"
#include "animation.h"
#include "scripted.h"
#include "nodes.h"
#include "defaultai.h"
#include "soundent.h"

#include "custom_debug.h"

extern CGraph WorldGraph;

//MODDD
extern float global_sparksAIFailMulti;
extern float global_crazyMonsterPrintouts;
extern float global_movementIsCompletePrintout;
extern float global_noFlinchOnHard;

extern float global_drawCollisionBoundsAtDeath;
extern float global_drawHitBoundsAtDeath;



EASY_CVAR_EXTERN(scheduleInterruptPrintouts)
	
EASY_CVAR_EXTERN(pathfindStumpedMode)
EASY_CVAR_EXTERN(pathfindStumpedWaitTime)
EASY_CVAR_EXTERN(pathfindStumpedForgetEnemy)

//MODDD - new. Shortened form of 

BOOL CBaseMonster::attemptFindCoverFromEnemy(Task_t* pTask){
	entvars_t *pevCover;

	if ( m_hEnemy == NULL )
	{
		// Find cover from self if no enemy available
		pevCover = pev;
//				TaskFail();
//				return FALSE; //return;
	}
	else
		pevCover = m_hEnemy->pev;

	if ( FindLateralCover( pevCover->origin, pevCover->view_ofs ) )
	{
		// try lateral first
		m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
		return TRUE;//TaskComplete();
	}
	else if ( FindCover( pevCover->origin, pevCover->view_ofs, 0, CoverRadius() ) )
	{
		// then try for plain ole cover
		m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
		return TRUE;//TaskComplete();
	}
	else
	{
		// no coverwhatsoever.
		return FALSE;//TaskFail();
	}
}//END OF attemptFindCoverFromEnemy()





//=========================================================
// FHaveSchedule - Returns TRUE if monster's m_pSchedule
// is anything other than NULL.
//=========================================================
BOOL CBaseMonster :: FHaveSchedule( void )
{
	if ( m_pSchedule == NULL )
	{
		return FALSE;
	}

	return TRUE;
}

//=========================================================
// ClearSchedule - blanks out the caller's schedule pointer
// and index.
//=========================================================
void CBaseMonster :: ClearSchedule( void )
{
	m_iTaskStatus = TASKSTATUS_NEW;
	m_pSchedule = NULL;
	m_iScheduleIndex = 0;
}

//=========================================================
// FScheduleDone - Returns TRUE if the caller is on the
// last task in the schedule
//=========================================================
BOOL CBaseMonster :: FScheduleDone ( void )
{
	ASSERT( m_pSchedule != NULL );
	
	if ( m_iScheduleIndex == m_pSchedule->cTasks )
	{
		return TRUE;
	}

	return FALSE;
}

//=========================================================
// ChangeSchedule - replaces the monster's schedule pointer
// with the passed pointer, and sets the ScheduleIndex back
// to 0
//=========================================================
void CBaseMonster :: ChangeSchedule ( Schedule_t *pNewSchedule )
{
	
	if(global_crazyMonsterPrintouts)easyForcePrintLine("YOU despicable person %s %d", pNewSchedule->pName, pNewSchedule->iInterruptMask);

	ASSERT( pNewSchedule != NULL );



	//MODDD - for now, let's count failing to change to a new schedule as a TaskFail() but be sure to talk
	//        about it in printouts, really don't want to miss this happening.
	if(pNewSchedule == NULL){
		easyPrintLine("WARNING: %s:%d: ChangeSchedule called with NULL schedule!", this->getClassname(), monsterID);
		TaskFail();
		return;
	}



	m_pSchedule			= pNewSchedule;
	m_iScheduleIndex	= 0;
	m_iTaskStatus		= TASKSTATUS_NEW;

	//MODDD - important.	ChangeSchedule now no longer resets conditions alone.
	//Runtask() now resets some, if not most conditions before calling RunAI(). Or early on in RunAI() once.


	//MODDD NOTE - don't get slick.  Just clear all conditions.
	m_afConditions		= 0;// clear all of the conditions

	//m_afConditions		&= ~(bits_COND_TASK_FAILED | bits_COND_SCHEDULE_DONE);

	//m_afConditions		&= (bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2 | bits_COND_CAN_MELEE_ATTACK2);


	m_failSchedule		= SCHED_NONE;


	m_fNewScheduleThisFrame = TRUE; //for recording. Notify if this same schedule ends up having a task complete or the schedule marked for failure before
	// this same frame (MonsterThink) ends once. If so ChangeSchedule was immediately followed by TaskComplete or TaskFail, must know not to do that.

	if ( m_pSchedule->iInterruptMask & bits_COND_HEAR_SOUND && !(m_pSchedule->iSoundMask) )
	{
		ALERT ( at_aiconsole, "COND_HEAR_SOUND with no sound mask!\n" );
	}
	else if ( m_pSchedule->iSoundMask && !(m_pSchedule->iInterruptMask & bits_COND_HEAR_SOUND) )
	{
		ALERT ( at_aiconsole, "Sound mask without COND_HEAR_SOUND!\n" );
	}

#if _DEBUG
	if ( !ScheduleFromName( pNewSchedule->pName ) )
	{
		ALERT( at_console, "Schedule %s not in table!!!\n", pNewSchedule->pName );
	}
#endif
	
// this is very useful code if you can isolate a test case in a level with a single monster. It will notify
// you of every schedule selection the monster makes.
#if 0
	if ( FClassnameIs( pev, "monster_human_grunt" ) )
	{
		Task_t *pTask = GetTask();
		
		if ( pTask )
		{
			const char *pName = NULL;

			if ( m_pSchedule )
			{
				pName = m_pSchedule->pName;
			}
			else
			{
				pName = "No Schedule";
			}
			
			if ( !pName )
			{
				pName = "Unknown";
			}

			ALERT( at_aiconsole, "%s: picked schedule %s\n", STRING( pev->classname ), pName );
		}
	}
#endif// 0

}

//=========================================================
// NextScheduledTask - increments the ScheduleIndex
//=========================================================
void CBaseMonster :: NextScheduledTask ( void )
{
	ASSERT( m_pSchedule != NULL );

	m_iTaskStatus = TASKSTATUS_NEW;
	m_iScheduleIndex++;

	if ( FScheduleDone() )
	{
		// just completed last task in schedule, so make it invalid by clearing it.
		SetConditions( bits_COND_SCHEDULE_DONE );
		//ClearSchedule();	
	}
}

//=========================================================
// IScheduleFlags - returns an integer with all Conditions
// bits that are currently set and also set in the current
// schedule's Interrupt mask.
//=========================================================
int CBaseMonster :: IScheduleFlags ( void )
{
	if( !m_pSchedule )
	{
		return 0;
	}
	
	// strip off all bits excepts the ones capable of breaking this schedule.
	return m_afConditions & m_pSchedule->iInterruptMask;
}






//#bitToCheck
#define CHEAPO(nickname, bitToCheck) ,\
	#nickname, (m_pSchedule->iInterruptMask & bits_COND_##bitToCheck)!=0, (m_afConditions & bits_COND_##bitToCheck)!= 0


#define CHEAPO2(nickname, bitToCheck) ,\
	#nickname, 9, (m_afConditions & bits_COND_##bitToCheck)!= 0

//=========================================================
// FScheduleValid - returns TRUE as long as the current
// schedule is still the proper schedule to be executing,
// taking into account all conditions
//=========================================================
BOOL CBaseMonster :: FScheduleValid ( void )
{
	if ( m_pSchedule == NULL )
	{
		if(global_crazyMonsterPrintouts)easyForcePrintLine("FScheduleValid: fail A");
		// schedule is empty, and therefore not valid.
		return FALSE;
	}

	if ( HasConditions( m_pSchedule->iInterruptMask | bits_COND_SCHEDULE_DONE | bits_COND_TASK_FAILED ) )
	{




		if(global_crazyMonsterPrintouts)easyForcePrintLine("FScheduleValid: fail B: %s %d ::: %d %d %d", m_pSchedule->pName, m_pSchedule->iInterruptMask, (m_afConditions & m_pSchedule->iInterruptMask), (m_afConditions & bits_COND_SCHEDULE_DONE), (m_afConditions & bits_COND_TASK_FAILED) );
		


		//This is a table of all conditions, whether the interrupt mask has them (counts for interrupt), and whether we happen to
		//have the condition set at this time (only interrupts if we have the condition set AND it is in the interupt list).
		
		
		//NOTE - if the schedule is done, it is natural for its task number to be -1. It ran out of tasks, which ended the
		//schedule and is why it is now interrupting (naturally) to look for a new one.

		
		//if(FClassnameIs(this->pev, "monster_scientist"))
		if(EASY_CVAR_GET(scheduleInterruptPrintouts))easyForcePrintLine("%s:%d SCHEDULE INTERRUPTED. Name:%s task:%d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n"
			"%s: %d %d\n",
			getClassname(), monsterID, getScheduleName(), this->getTaskNumber()
			CHEAPO(noammo, NO_AMMO_LOADED)
			CHEAPO(seehat, SEE_HATE)
			CHEAPO(se_fea, SEE_FEAR)
			CHEAPO(se_dis, SEE_DISLIKE)
			CHEAPO(se_ene, SEE_ENEMY)
			CHEAPO(en_occ, ENEMY_OCCLUDED)
			CHEAPO(sm_foo, SMELL_FOOD)
			CHEAPO(en_far, ENEMY_TOOFAR)
			CHEAPO(li_dmg, LIGHT_DAMAGE)
			CHEAPO(hv_dmg, HEAVY_DAMAGE)
			CHEAPO(c_rat1, CAN_RANGE_ATTACK1)
			CHEAPO(c_mat1, CAN_MELEE_ATTACK1)
			CHEAPO(c_rat2, CAN_RANGE_ATTACK2)
			CHEAPO(c_mat2, CAN_MELEE_ATTACK2)

			CHEAPO(provok, PROVOKED)
			CHEAPO(new_en, NEW_ENEMY)
			CHEAPO(hear_s, HEAR_SOUND)
			CHEAPO(smellg, SMELL)
			CHEAPO(en_fac, ENEMY_FACING_ME)
			CHEAPO(en_dea, ENEMY_DEAD)
			CHEAPO(se_cli, SEE_CLIENT)
			CHEAPO(se_nem, SEE_NEMESIS)
			CHEAPO(spec_1, SPECIAL1)
			CHEAPO(spec_2, SPECIAL2)
			CHEAPO2(tasfai, TASK_FAILED)
			CHEAPO2(schdon, SCHEDULE_DONE)
			);

			

#ifdef DEBUG
		if ( HasConditions ( bits_COND_TASK_FAILED ) && m_failSchedule == SCHED_NONE )
		{
			// fail! Send a visual indicator.
			ALERT ( at_aiconsole, "Schedule: %s Failed\n", m_pSchedule->pName );

			Vector tmp = pev->origin;
			tmp.z = pev->absmax.z + 16;

			//MODDD
			//UTIL_Sparks( tmp );
			UTIL_Sparks2( tmp, DEFAULT_SPARK_BALLS, global_sparksAIFailMulti );
		}
#endif // DEBUG

		// some condition has interrupted the schedule, or the schedule is done
		return FALSE;
	}
	
	return TRUE;
}



//=========================================================
// MaintainSchedule - does all the per-think schedule maintenance.
// ensures that the monster leaves this function with a valid
// schedule!
//=========================================================
void CBaseMonster :: MaintainSchedule ( void )
{
	Schedule_t	*pNewSchedule;
	int			i;
	
	/*
		if(m_iTaskStatus == TASKSTATUS_RUNNING){
			return;
		}
		*/

	if(global_crazyMonsterPrintouts == 1){
		easyPrintLine("DOCKS1 %d", HasConditions(bits_COND_CAN_MELEE_ATTACK1));
	}
	// UNDONE: Tune/fix this 10... This is just here so infinite loops are impossible
	for ( i = 0; i < 10; i++ )
	{
		if ( m_pSchedule != NULL && TaskIsComplete() )
		{
			NextScheduledTask();                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   
		}


		if(global_crazyMonsterPrintouts == 1){
			easyPrintLine("OOPS A PLENTY 1 %d ::: %d %d ::: %d %d", HasConditions(bits_COND_CAN_MELEE_ATTACK1), !FScheduleValid(), m_MonsterState != m_IdealMonsterState,  m_MonsterState, m_IdealMonsterState );
		}
	// validate existing schedule 


		if ( !FScheduleValid() || m_MonsterState != m_IdealMonsterState )
		{
			if(global_crazyMonsterPrintouts)easyForcePrintLine("MaintainSchedule: INVALID A %d %d %d", FScheduleValid(), m_MonsterState, m_IdealMonsterState);



			// if we come into this block of code, the schedule is going to have to be changed.
			// if the previous schedule was interrupted by a condition, GetIdealState will be 
			// called. Else, a schedule finished normally.

			// Notify the monster that his schedule is changing
			//!!!!! THIS IS BARELY USED! I missed it and tried implementing the same idea.
			ScheduleChange();
			

			if(global_crazyMonsterPrintouts == 1){
				easyPrintLine("OOPS A PLENTY 2 %d", HasConditions(bits_COND_CAN_MELEE_ATTACK1));
			}
			// Call GetIdealState if we're not dead and one or more of the following...
			// - in COMBAT state with no enemy (it died?)
			// - conditions bits (excluding SCHEDULE_DONE) indicate interruption,
			// - schedule is done but schedule indicates it wants GetIdealState called
			//   after successful completion (by setting bits_COND_SCHEDULE_DONE in iInterruptMask)
			// DEAD & SCRIPT are not suggestions, they are commands!


			//easyForcePrintLine("%s:%d WHAT IS UP?? State: cur:%d idea:%d", getClassname(), monsterID, m_MonsterState, m_IdealMonsterState);
			if ( m_IdealMonsterState != MONSTERSTATE_DEAD && 
				 (m_IdealMonsterState != MONSTERSTATE_SCRIPT || m_IdealMonsterState == m_MonsterState) )
			{

				//easyForcePrintLine("POOPIN PENIS (%d && %d) %d (%d && %d)", m_afConditions, !HasConditions(bits_COND_SCHEDULE_DONE), (m_pSchedule && (m_pSchedule->iInterruptMask & bits_COND_SCHEDULE_DONE)), (m_MonsterState == MONSTERSTATE_COMBAT), (m_hEnemy==NULL) );
				if (	(m_afConditions && !HasConditions(bits_COND_SCHEDULE_DONE)) ||
						
					//Sorry... What? Doesn't any schedule being done count as being done? They don't have to explicitly say they are interrupted by being done, 
					//it is implied they are... what.  Keeping for safety.
					(m_pSchedule && (m_pSchedule->iInterruptMask & bits_COND_SCHEDULE_DONE)) ||

						((m_MonsterState == MONSTERSTATE_COMBAT) && (m_hEnemy == NULL))	)
				{
					if(global_crazyMonsterPrintouts == 1){
						easyPrintLine("OOPS A PLENTY 3 %d", HasConditions(bits_COND_CAN_MELEE_ATTACK1));
					}
					GetIdealState();
					if(global_crazyMonsterPrintouts == 1){
						easyPrintLine("OOPS A PLENTY 4 %d", HasConditions(bits_COND_CAN_MELEE_ATTACK1));
					}
				}
			}
			if ( HasConditions( bits_COND_TASK_FAILED ) && m_MonsterState == m_IdealMonsterState )
			{
				if(global_crazyMonsterPrintouts == 1){
					easyPrintLine("OOPS A PLENTY 5 %d", HasConditions(bits_COND_CAN_MELEE_ATTACK1));
				}
				if ( m_failSchedule != SCHED_NONE ){
					pNewSchedule = GetScheduleOfType( m_failSchedule );
				}else{
					pNewSchedule = GetScheduleOfType( SCHED_FAIL );
				}
				// schedule was invalid because the current task failed to start or complete
				ALERT ( at_aiconsole, "Schedule Failed at %d!\n", m_iScheduleIndex );
				if(global_crazyMonsterPrintouts == 1){
					easyPrintLine("OOPS A PLENTY 6 %d", HasConditions(bits_COND_CAN_MELEE_ATTACK1));
				}
				ChangeSchedule( pNewSchedule );
				if(global_crazyMonsterPrintouts == 1){
					easyPrintLine("OOPS A PLENTY 7 %d", HasConditions(bits_COND_CAN_MELEE_ATTACK1));
				}
			}
			else
			{
				if(global_crazyMonsterPrintouts == 1){
					easyPrintLine("OOPS A PLENTY 8 %d ::: %d, %d ::: %d %d", HasConditions(bits_COND_CAN_MELEE_ATTACK1),  m_MonsterState == MONSTERSTATE_SCRIPT,  m_MonsterState == MONSTERSTATE_DEAD,    m_MonsterState == MONSTERSTATE_SCRIPT, m_MonsterState == MONSTERSTATE_DEAD  );
				}
				SetState( m_IdealMonsterState );
				if ( m_MonsterState == MONSTERSTATE_SCRIPT || m_MonsterState == MONSTERSTATE_DEAD ){
					pNewSchedule = CBaseMonster::GetSchedule();
					if(global_crazyMonsterPrintouts)easyForcePrintLine("MaintainSchedule: INVALID B1. Schedule was: %s", getScheduleName());
				}
				else{
					//easyForcePrintLine("%s:%d MY SCHEDULE WAS INTERRUPTED. IT WAS %s", getClassname(), monsterID, getScheduleName());
					pNewSchedule = GetSchedule();
					if(global_crazyMonsterPrintouts)easyForcePrintLine("MaintainSchedule: INVALID B2. Schedule was: %s", getScheduleName());
				}
				if(global_crazyMonsterPrintouts == 1){
					easyPrintLine("OOPS A PLENTY 9 %d::: new sched: %s", HasConditions(bits_COND_CAN_MELEE_ATTACK1), pNewSchedule->pName);
				}
				//YOU STUPID LITTLE man, I WILL OBLITERATE YOUR VERY WILL TO LIVE AND PLAY IN THE ASHES.
				//~what is this, Dilbert?    Damn, the rage is real yo.
				
				
				if(global_crazyMonsterPrintouts)easyForcePrintLine("MaintainSchedule: NEW SCHEDULE WILL BE %s", pNewSchedule->pName);

				ChangeSchedule( pNewSchedule );
				if(global_crazyMonsterPrintouts == 1){
					easyPrintLine("OOPS A PLENTY 10 %d", HasConditions(bits_COND_CAN_MELEE_ATTACK1));
				}
			}

		if(global_crazyMonsterPrintouts == 1){
			easyPrintLine("OOPS A PLENTY 11 %d", HasConditions(bits_COND_CAN_MELEE_ATTACK1));
		}
		}//END OF valid check




		if ( m_iTaskStatus == TASKSTATUS_NEW )
		{	
			Task_t *pTask = GetTask();
			ASSERT( pTask != NULL );



			TaskBegin();
			
			//MODDD - change-schedule task complete / schedule failure check added.
			m_fNewScheduleThisFrame = FALSE; //reset.

			StartTask( pTask );

			//Do the check.
			if( m_fNewScheduleThisFrame ){
				//If we recently called ChangeSchedule (this same frame) and the task ended up completed by a TaskComplete call or the schedule ended up failed by a TaskFail() in this
				//same frame, something did not go right.
		
				 if(HasConditions(bits_COND_TASK_FAILED)){
					 easyForcePrintLine("!!!CRITICAL: %s:%d, sched:%s task:%d. Fresh schedule failed before running a single task! REPORT THIS", getClassname(), monsterID, getScheduleName(), getTaskNumber());
					
					 ClearConditions(bits_COND_TASK_FAILED);

				 }else if( m_iTaskStatus == TASKSTATUS_COMPLETE){
					 easyForcePrintLine("!!!CRITICAL: %s:%d, sched:%s task:%d. Fresh schedule completed first task before running a single task! REPORT THIS", getClassname(), monsterID, getScheduleName(), getTaskNumber());
					 m_iTaskStatus = TASKSTATUS_NEW;
				 }
				 //Some correction for now, but still need to track any calls like these down and fix them. These automatic adjustments may not be all that is needed.
			}//END OF m_fNewScheduleThisFrame check



		}//END OF if ( m_iTaskStatus == TASKSTATUS_NEW )




		
		if(global_crazyMonsterPrintouts == 1){
			easyPrintLine("OOPS A PLENTY 12 %d", HasConditions(bits_COND_CAN_MELEE_ATTACK1));
		}
		// UNDONE: Twice?!!!

		//MODDD TODO - add a check for " || signalActivityUpdate" too? may cause things to break, careful.
		if ( m_Activity != m_IdealActivity )
		{
			SetActivity ( m_IdealActivity );
		}
		if(global_crazyMonsterPrintouts == 1){
			easyPrintLine("OOPS A PLENTY 13 %d", HasConditions(bits_COND_CAN_MELEE_ATTACK1));
		}
		
		if ( !TaskIsComplete() && m_iTaskStatus != TASKSTATUS_NEW )
			break;
			
		if(global_crazyMonsterPrintouts == 1){
			easyPrintLine("OOPS A PLENTY 14 %d", HasConditions(bits_COND_CAN_MELEE_ATTACK1));
		}

	}//END OF THE LOOP.  Come on now, let's not try and hide this.

	
	



	if ( TaskIsRunning() )
	{
		Task_t *pTask = GetTask();
		ASSERT( pTask != NULL );
		//easyForcePrintLine("IM GONNA EXPLODE sched:%s task:%d act:%d", this->getScheduleName(), pTask->iTask, m_Activity);


		//MODDD - change-schedule task complete / schedule failure check added.
		m_fNewScheduleThisFrame = FALSE; //reset.

		RunTask( pTask );

		//Do the check.
		if( m_fNewScheduleThisFrame ){
			//If we recently called ChangeSchedule (this same frame) and the task ended up completed by a TaskComplete call or the schedule ended up failed by a TaskFail() in this
			//same frame, something did not go right.
		
			 if(HasConditions(bits_COND_TASK_FAILED)){
				 easyForcePrintLine("!!!CRITICAL: %s:%d, sched:%s task:%d. Fresh schedule failed before running a single task! REPORT THIS", getClassname(), monsterID, getScheduleName(), getTaskNumber());
				 
				 ClearConditions(bits_COND_TASK_FAILED);
			 }else if( m_iTaskStatus == TASKSTATUS_COMPLETE){
				 easyForcePrintLine("!!!CRITICAL: %s:%d, sched:%s task:%d. Fresh schedule completed first task before running a single task! REPORT THIS", getClassname(), monsterID, getScheduleName(), getTaskNumber());
				 m_iTaskStatus = TASKSTATUS_NEW;
			 }
			 //Some correction for now, but still need to track any calls like these down and fix them. These automatic adjustments may not be all that is needed.
		}//END OF m_fNewScheduleThisFrame check


	}//END OF TaskIsRunning()
	












	// UNDONE: We have to do this so that we have an animation set to blend to if RunTask changes the animation
	// RunTask() will always change animations at the end of a script!
	// Don't do this twice

	//MODDD - allowing for "signalActivityUpdate" to also trigger SetActivity... once.
	if ( m_Activity != m_IdealActivity || signalActivityUpdate )
	{
		//MODDD - well yea.
		signalActivityUpdate = FALSE;


		SetActivity ( m_IdealActivity );
	}

	if(global_crazyMonsterPrintouts == 1){
		easyPrintLine("CAN I MELEE COND1? %d", HasConditions(bits_COND_CAN_MELEE_ATTACK1));
	}
}

//=========================================================
// RunTask 
//=========================================================
void CBaseMonster :: RunTask ( Task_t *pTask )
{

	
	if(m_pSchedule == slPathfindStumped){
		easyForcePrintLine("RunTask: %s:%d task: %d", getClassname(), monsterID, pTask->iTask);
	}


	switch ( pTask->iTask )
	{
	case TASK_TURN_RIGHT:
	case TASK_TURN_LEFT:
		{
			ChangeYaw( pev->yaw_speed );

			if ( FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}

	case TASK_PLAY_SEQUENCE_FACE_ENEMY:
	case TASK_PLAY_SEQUENCE_FACE_TARGET:
		{
			CBaseEntity *pTarget;

			if ( pTask->iTask == TASK_PLAY_SEQUENCE_FACE_TARGET )
				pTarget = m_hTargetEnt;
			else
				pTarget = m_hEnemy;
			if ( pTarget )
			{
				pev->ideal_yaw = UTIL_VecToYaw( pTarget->pev->origin - pev->origin );
				ChangeYaw( pev->yaw_speed );
			}
			if ( m_fSequenceFinished )
				TaskComplete();
		}
		break;

	case TASK_PLAY_SEQUENCE:
	case TASK_PLAY_ACTIVE_IDLE:
		{
			if ( m_fSequenceFinished )
			{
				TaskComplete();
			}
			break;
		}


	case TASK_FACE_ENEMY:
		{


			MakeIdealYaw( m_vecEnemyLKP );

			ChangeYaw( pev->yaw_speed );

			//easyForcePrintLine("TASK_FACE_ENEMY: %s%d WHAT? yawdif:%.2f yaw_spd:%.2f", this->getClassname(), this->monsterID,    FlYawDiff(), pev->yaw_speed);

			if ( FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}

	//MODDD - new. No need
	case TASK_FACE_POINT:
	case TASK_FACE_HINTNODE:
	case TASK_FACE_LASTPOSITION:
	case TASK_FACE_TARGET:
	case TASK_FACE_IDEAL:
	case TASK_FACE_ROUTE:
	case TASK_FACE_PREV_LKP:
	case TASK_FACE_BEST_SOUND:
		{

			if(pTask->iTask == TASK_FACE_TARGET && this->m_hTargetEnt == NULL){TaskFail();break;}  //if we are told to face a target that does not / no longer exists, stop.


			ChangeYaw( pev->yaw_speed );

			if ( FacingIdeal() )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_CHECK_STUMPED:
		{
			//Nothing yet, but if this has its own delay to wait for before letting the "unstumping" commence (re-route to the updated LKP that matches the real enemy position),
			//that would go here and call "TaskComplete()" when the delay is up.
		}
	break;
	case TASK_WAIT_PVS:
		{
			if ( !FNullEnt(FIND_CLIENT_IN_PVS(edict())) )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_WAIT_INDEFINITE:
		{
			// don't do anything.
			break;
		}


	case TASK_WAIT_ENEMY_LOOSE_SIGHT:{

		//keep looking at the enemy.
		MakeIdealYaw ( m_vecEnemyLKP );
		ChangeYaw( pev->yaw_speed ); 

		//only continue if we loose sight of the enemy.
		if(!HasConditions(bits_COND_SEE_ENEMY)){
			TaskComplete();
		}	
	break;}

	case TASK_WAIT:
	case TASK_WAIT_RANDOM:
	case TASK_WAIT_STUMPED:
		{
			if ( gpGlobals->time >= m_flWaitFinished )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_WAIT_FACE_ENEMY:
		{
			MakeIdealYaw ( m_vecEnemyLKP );
			ChangeYaw( pev->yaw_speed ); 

			if ( gpGlobals->time >= m_flWaitFinished )
			{
				TaskComplete();
			}
			break;
		}
	case TASK_MOVE_TO_TARGET_RANGE:
		{
			float distance;
			float targetDistToGoal_ChangeReq = pTask->flData*0.5;
			

			//easyForcePrintLine("WELL WHAT IS IT? NOTNULL? %d", (m_hTargetEnt != NULL));

			if ( m_hTargetEnt == NULL ){
				//easyPrintLine("TASK_MOVE_TO_TARGET_RANGE FAILED. null enemy.  My ID: %d", this->monsterID);
				TaskFail();
			}else
			{
				float targetDistToGoal = (m_vecMoveGoal - m_hTargetEnt->pev->origin).Length();
				float myDistToTargetEnt = (m_hTargetEnt->pev->origin - pev->origin).Length();

				//easyForcePrintLine("TARGETNAME: %s", m_hTargetEnt->getClassname());

				distance = ( m_vecMoveGoal - pev->origin ).Length();  //current distance from me to the movegoal, which may or may not exactly match my distance to the targetEnt's origin.
				// Re-evaluate when you think your finished, or the target has moved too far


				
				//"(distance < pTask->flData)" ? wouldn't that already end the method soon? Even with the distance changing a little bit?
				if(global_movementIsCompletePrintout)easyForcePrintLine("RunTask TASK_MOVE_TO_TARGET_RANGE. I am %s:%d sched:%s myDistToGoal: %.2f myDistToGoalReq: %.2f myDistToTargetEnt: %.2f targetDistToGoal:%.2f reqToChange:%.2f",
					getClassname(),
					monsterID,
					getScheduleName(),
					distance,
					pTask->flData,
					myDistToTargetEnt,
					targetDistToGoal,
					targetDistToGoal_ChangeReq);

				if ( (FALSE) || (targetDistToGoal > targetDistToGoal_ChangeReq) )
				{
					if(global_movementIsCompletePrintout)easyForcePrintLine("REEEEEROUTED");
					m_vecMoveGoal = m_hTargetEnt->pev->origin;
					
					//distance = ( m_vecMoveGoal - pev->origin ).Length2D();
					//distance = myDistToTargetEnt;

					BOOL successfulRefresh = FRefreshRoute();
					if(!successfulRefresh){TaskFail();/*easyForcePrintLine("YEAAAHGHHHHHHHHHHH")*/;break;} //Not a good refresh? Stop.
				}



				
				// Set the appropriate activity based on an overlapping range
				// overlap the range to prevent oscillation
				//MODDD - new possible satisfying condition. I am closer to my target's absolute position than the goaldist. I know, stunning.
				if ( distance < pTask->flData || myDistToTargetEnt < pTask->flData )
				{
					if(global_movementIsCompletePrintout)easyForcePrintLine("FINISH!!!");
					TaskComplete();
					RouteClear();		// Stop moving
					return;
				}
				else if ( distance < 190 && m_movementActivity != ACT_WALK )
					m_movementActivity = ACT_WALK;
				else if ( distance >= 270 && m_movementActivity != ACT_RUN )
					m_movementActivity = ACT_RUN;





			}//END OF m_hTargetEnt check

			break;
		}
	case TASK_MOVE_TO_ENEMY_RANGE:
		{

			
	//see TASK_WAIT_FOR_MOVEMENT_RANGE... interesting stuff.




			if (MovementIsComplete()){
				//easyForcePrintLine("I GOT YOU!");
				//TaskComplete();
				//RouteClear();
				//MODDD - no, restsart! If there is a case this never causes this task to end to say, re-route at an apparent dead-end, need to adjust this.



				//No, just TaskComplete() as usual...
				//BOOL test = FRefreshRoute();
				//if(!test){if(global_movementIsCompletePrintout)easyForcePrintLine("TASK_MOVE_TO_ENEMY_RANGE: FAILURE 2"); TaskFail();}else{if(global_movementIsCompletePrintout)easyForcePrintLine("TASK_MOVE_TO_ENEMY_RANGE: SUCCESS 2");};

				//break; //is stopping this early this frame here necessary?

				if(global_movementIsCompletePrintout)easyPrintLine("TASK_MOVE_TO_ENEMY_RANGE success??? My ID: %d", this->monsterID);
				
				TaskComplete();
				return;
			}



			//float distance;

			if ( m_hEnemy == NULL ){
				if(global_movementIsCompletePrintout)easyPrintLine("TASK_MOVE_TO_ENEMY_RANGE FAILED THE EMPRAH. null enemy. My ID: %d", this->monsterID);
				TaskFail();
			}else
			{


				//or m_hEnemy->pev->origin ???
				Vector& vecDestination = m_vecEnemyLKP;

				//distance = ( m_vecMoveGoal - pev->origin ).Length2D();


				// Re-evaluate when you think your finished, or the ENEMY has moved too far
				

				//only re-route early (2nd condition here) IF we can physically see the enemy (in view cone)
				//

				//2000 200
				//1000 100
				//400 40

				BOOL redoPass = FALSE;




				//which one?
				//ALSO STILL... headcrab jumpiness and hassassin backwards anim events + restore out.
				//const float enemyRealDist = (pev->origin - vecDestination).Length();
				const float enemyRealDist = (pev->origin - m_vecMoveGoal).Length();

				//interestingly enough, "FInViewCone" does not count as a Visible check.
				//As in, if there is a solid wall between this monster and the enemy, FInViewCone will still report "true" if this monster is facing the enemy regardless.
				//ALSO, if any damage has been sustained, this monster will forcibly re-route. Kind of like an "OH there you are!" moment.
				if( (FInViewCone( m_hEnemy ) && FVisible(m_hEnemy)) || HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE) ){


					//m_vecEnemyLKP = m_hEnemy->pev->origin;

					//MODDD NOTE - should we check to see there is a clear line of sight with the enemy, regardless of facing direction, before forcing a reroute? ){

					if(global_movementIsCompletePrintout){
						//include it. Clearly we care.
						::DebugLine_Setup(0, m_vecMoveGoal - Vector(0,0,8), m_vecMoveGoal+Vector(0, 0, 8), 0, 255, 0);
						::DebugLine_Setup(1, vecDestination - Vector(0,0,8), vecDestination+Vector(0, 0, 8), 255, 0, 0);

					}
					float enemyPathGoalDistance = (m_vecMoveGoal - vecDestination).Length();
					
					/*
					if(pTask->flData <= 1){
						//code! Use the flexible test.
						const float enemyRealDist = (pev->origin - vecDestination).Length();
						redoPass = ((m_vecMoveGoal - vecDestination).Length() > enemyRealDist*pTask->flData);
					}else{
						redoPass = ((m_vecMoveGoal - vecDestination).Length() > pTask->flData);
					}
					*/
					//easyForcePrintLine("TEST: e:%.2f m:%.2f a:%.2f", enemyRealDist, (m_vecMoveGoal - vecDestination).Length2D(), enemyRealDist*0.2);
					//distance between the enemy and where I was told to go (position of the enemy since doing a path-find before)
					redoPass = (enemyPathGoalDistance > enemyRealDist*0.2); //((m_vecMoveGoal - vecDestination).Length2D() > enemyRealDist*0.3);
					
				}



				if (
					//(enemyRealDist < pTask->flData) ||
					(redoPass)
					)
				{
					//if "m_vecMoveGoal" even matters. moveGoal was set to ENEMY in the startTask case for this task.
					//m_vecMoveGoal = vecDestination;
					//distance = ( m_vecMoveGoal - pev->origin ).Length2D();
					//distance = 0; //do not allow this to pass.



					/*
					
	case TASK_GET_PATH_TO_ENEMY:
		{
			CBaseEntity *pEnemy = m_hEnemy;

			if ( pEnemy == NULL )
			{
				TaskFail();
				return;
			}
			
			if ( BuildRoute ( pEnemy->pev->origin, bits_MF_TO_ENEMY, pEnemy ) )
			{
				TaskComplete();
			}
			else if (BuildNearestRoute( pEnemy->pev->origin, pEnemy->pev->view_ofs, 0, (pEnemy->pev->origin - pev->origin).Length() ))
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
					*/

			//fRefreshRoute:
		/*

		case MOVEGOAL_ENEMY:
			returnCode = BuildRoute( m_vecEnemyLKP, bits_MF_TO_ENEMY, m_hEnemy );
		*/

					//COMPARe. Does refreshRoute handle the above, TASK_GET_PATH_TO_ENEMY, adequately?
					//FRefreshRoute();


					m_vecMoveGoal = m_vecEnemyLKP;
					BOOL test = FRefreshRoute();
					
					if(!test){if(global_movementIsCompletePrintout)easyForcePrintLine("TASK_MOVE_TO_ENEMY_RANGE: FAILURE 3"); TaskFail();}else{if(global_movementIsCompletePrintout)easyForcePrintLine("TASK_MOVE_TO_ENEMY_RANGE: SUCCESS 3");};
				}

				
					//easyForcePrintLine("WHAT???!!! %.2f %.2f", distance, pTask->flData);

				// Set the appropriate activity based on an overlapping range
				// overlap the range to prevent oscillation
				if ( enemyRealDist < pTask->flData )
				{

					//TaskComplete();
					//RouteClear();		// Stop moving
					//MODDD - NO. This alls for a re-route.
					
					m_vecMoveGoal = m_vecEnemyLKP;
					BOOL test = FRefreshRoute();
					if(!test)TaskFail();

				}
				else{
					if ( LookupActivity( ACT_RUN ) != ACTIVITY_NOT_AVAILABLE )
					{
						m_movementActivity = ACT_RUN;
					}
					else
					{
						m_movementActivity = ACT_WALK;
					}
				}
				/*
				else if ( distance < 190 && m_movementActivity != ACT_WALK )
					m_movementActivity = ACT_WALK;
				else if ( distance >= 270 && m_movementActivity != ACT_RUN )
					m_movementActivity = ACT_RUN;
					*/
			}
			break;
		}
	//MODDD - clone of "TASK_MOVE_TO_TARGET_RANGE".
	case TASK_MOVE_TO_POINT_RANGE:
		{
		float distance;

		//easyForcePrintLine("RUNNIN AND RUNNIN");

		/*
		if ( m_hTargetEnt == NULL ){
			easyPrintLine("I HAVE FAILED THE EMPRAH.  ID: %d", this->monsterID);
			TaskFail();
		}else
		*/
			
			distance = ( m_vecMoveGoal - pev->origin ).Length2D();
			//no updates. It is a static location that anything can update if it chooses to. 
			//  m_vecMoveGoal    that is.

			// Set the appropriate activity based on an overlapping range
			// overlap the range to prevent oscillation
			if ( distance < pTask->flData )
			{
				//easyForcePrintLine("%s:%d O REALLY NOW! %.2f %.2f", this->getClassnameShort(), this->monsterID, distance, pTask->flData);
				TaskComplete();
				RouteClear();		// Stop moving
			}
			else if ( distance < 350 && m_movementActivity != ACT_WALK )
				m_movementActivity = ACT_WALK;
			else if ( distance >= 500 && m_movementActivity != ACT_RUN )
				m_movementActivity = ACT_RUN;

			break;
		}
	case TASK_WAIT_FOR_MOVEMENT:
		{
			if(global_movementIsCompletePrintout == 1){
				easyPrintLine("%s:%d: IS MOVEMENT COMPLETE?: %d", getClassname(), monsterID, MovementIsComplete());
				easyPrintLine("MOVEGOAL: %d", this->m_movementGoal);

				if(this->m_movementGoal == MOVEGOAL_LOCATION){
					UTIL_printLineVector("GOAL LOC:", this->m_vecMoveGoal);
				}

			}
			if (MovementIsComplete())
			{
				TaskComplete();
				RouteClear();		// Stop moving
			}
			break;
		}
	case TASK_WAIT_FOR_MOVEMENT_RANGE:
		{

			if (MovementIsComplete())
			{
				TaskComplete();
				RouteClear();		// Stop moving
			}else{
				//another chance...
				//easyForcePrintLine("WHAT %d", m_iRouteIndex);

				//if m_iRouteIndex is 0, we are on our way to the GOAL node, as opposed to going past any corners.  Check the distance to see if we are close enough.
				
				/*
				if(m_iRouteIndex > -1){
					easyForcePrintLine("TASK_WAIT_FOR_MOVEMENT_RANGE mg:%d ri:%d ::: F:%d g:%d e:%d d:%d", m_movementGoal, m_iRouteIndex, m_Route[m_iRouteIndex].iType, m_Route[m_iRouteIndex].iType&bits_MF_IS_GOAL, m_Route[m_iRouteIndex].iType&bits_MF_TO_ENEMY, m_Route[m_iRouteIndex].iType&bits_MF_TO_DETOUR);
				}else{
					easyForcePrintLine("TASK_WAIT_FOR_MOVEMENT_RANGE mg:%d ri:-1", m_movementGoal);
				}
				*/

				//NOTICE - the "m_iRouteIndex == 0" is bad. 0 does not always have to be the final goal of a path!
				//this->m_movementGoal
				if(m_Route[m_iRouteIndex].iType & bits_MF_IS_GOAL){
					float distToGoal = ( m_Route[ m_iRouteIndex ].vecLocation - pev->origin ).Length();
					//easyForcePrintLine("TASK_WAIT_FOR_MOVEMENT_RANGE: distToGoal:%.2f req:%.2f", distToGoal, pTask->flData);
					if(distToGoal < pTask->flData){
						//done!
						TaskComplete();
						RouteClear();		// Stop moving
					}
				}

			}
			break;
		}
	case TASK_DIE:
		{

			if(pev->frame >= (255*0.4)){
				//40% of the way there? Anything should be able to tell we're dead to stop marking this as an enemy / focus on something else.
				//MODDD TODO - make this adjustable per death anim per monster? Sounds tricky and unworthwhile most of the time, but the option could exist.
				//This is just used so the AI can know to drop this enemy a little early (they sure look dead) and move on in the heat of combat.
				recognizablyDead = TRUE;
			}

			//Is the end 255 or 256?! DAMN.
			if ( m_fSequenceFinished && pev->frame >= 255 )
			{

				//MODDD - FOR DEBUGGING
				if(global_drawCollisionBoundsAtDeath == 1){
					UTIL_drawBox(pev->origin + pev->mins, pev->origin + pev->maxs);
				}
				if(global_drawHitBoundsAtDeath == 1){
					UTIL_drawBox(pev->absmin, pev->absmax);
				}
				



				//MODDD - does not seem effective, scrapped.
				
				/*
				CBaseMonster::smartResize();
				//if(gpGlobals->time > 20 && gpGlobals->time < 21){
					//UTIL_printoutVector(pev->origin);
					easyPrintLine("GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG");
					easyPrintLine("GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG");
					easyPrintLine("GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG");
					easyPrintLine("GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG");
					easyPrintLine("GGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG");
					easyPrintLine("ahhh %.2f %.2f %.2f, %.2f %.2f %.2f", pev->mins.x, pev->mins.y, pev->mins.z, pev->maxs.x, pev->maxs.y, pev->maxs.z );
					UTIL_drawBox(pev->origin + pev->mins, pev->origin + pev->maxs);
				//}
				*/
				
				DeathAnimationEnd();
				

				//MODDD
				//pev->solid = SOLID_TRIGGER;
				//pev->movetype = MOVETYPE_NONE;


				/*
				//pev->movetype = MOVETYPE_NOCLIP;
				
				//MODDD - set size.
				//UTIL_SetSize ( pev, Vector ( -64, -64, -64 ), Vector ( 64, 64, 64 ) );

				UTIL_SetSize( pev, Vector(-16, -16, -32), Vector(16, 16, 0) );

				//pev->classname = MAKE_STRING("monster_barnacle");

				pev->solid			= SOLID_SLIDEBOX;
				pev->movetype		= MOVETYPE_NONE;
				*/

			}
			break;
		}
	case TASK_DIE_LOOP:
	{
		//No default behavior. Runs indefinitely unless a child class implements this and tells it when to stop with "TaskComplete" and let TASK_DIE proceed as usual
		//(pick a typical death sequence, run until the end and freeze at the last frame).

	break;
	}
	case TASK_RANGE_ATTACK1_NOTURN:
	case TASK_MELEE_ATTACK1_NOTURN:
	case TASK_MELEE_ATTACK2_NOTURN:
	case TASK_RANGE_ATTACK2_NOTURN:
	case TASK_RELOAD_NOTURN:
		{
			if ( m_fSequenceFinished )
			{
				m_Activity = ACT_RESET;
				TaskComplete();
			}
			break;
		}
	case TASK_RANGE_ATTACK1:
	case TASK_MELEE_ATTACK1:
	case TASK_MELEE_ATTACK2:
	case TASK_RANGE_ATTACK2:
	case TASK_SPECIAL_ATTACK1:
	case TASK_SPECIAL_ATTACK2:
	case TASK_RELOAD:
		{

			//easyPrintLine("????????? %d", m_fSequenceFinished);



			MakeIdealYaw ( m_vecEnemyLKP );
			ChangeYaw ( pev->yaw_speed );

			if ( m_fSequenceFinished )
			{
				//MODDD NOTE - BEWARE. This is likely to pick the same range attack activity again if the ideal activity remains that way.
				m_Activity = ACT_RESET;
				TaskComplete();
			}
			break;
		}
	case TASK_SMALL_FLINCH:
		{
			if ( m_fSequenceFinished )
			{
				TaskComplete();
			}
		}
		break;
	case TASK_WAIT_FOR_SCRIPT:
		{
			if ( m_pCine->m_iDelay <= 0 && gpGlobals->time >= m_pCine->m_startTime )
			{
				TaskComplete();
				m_pCine->StartSequence( (CBaseMonster *)this, m_pCine->m_iszPlay, TRUE );
				if ( m_fSequenceFinished )
					ClearSchedule();
				pev->framerate = 1.0;
				//ALERT( at_aiconsole, "Script %s has begun for %s\n", STRING( m_pCine->m_iszPlay ), STRING(pev->classname) );
			}
			break;
		}
	case TASK_PLAY_SCRIPT:
		{
			if (m_fSequenceFinished)
			{
				m_pCine->SequenceDone( this );
			}
			break;
		}

	
	//MODDD - new
	case TASK_WAIT_FOR_SEQUENCEFINISH:
		{

			//easyForcePrintLine("!!!!!!!!!!!!!!!!!!!!!!! %d", m_fSequenceFinished);
			//BEWARE: looping anims may just keep going!  
			//If necessary, anims could have a separate "loopedOnce" flag to be set when the anim would have usually ended but decided to loop instead, that is read HERE instead.
			if(m_fSequenceFinished){
				TaskComplete();
			}


		break;
		}



	}//END OF switch(...)

}//END OF RunTask(...)

//=========================================================
// SetTurnActivity - measures the difference between the way
// the monster is facing and determines whether or not to
// select one of the 180 turn animations.
//=========================================================
void CBaseMonster :: SetTurnActivity ( void )
{


	if(this->iAmDead == TRUE || !UTIL_IsAliveEntity(this) || deadSetActivityBlock){
		easyForcePrintLine("!!! SETTURNACTIVITY CALLED WHILE DEAD? BLASPHEMY !!! Printing out my stats...");
		this->ReportAIState();
		return;
	}

	float flYD;
	flYD = FlYawDiff();

	
	//MODDD - new. Remember the old activity before turning. Not that this is used in many places yet.
	//Might not need it at all.
	//m_IdealActivityBeforeTurn = m_IdealActivity;


	if ( flYD <= -45 && LookupActivity ( ACT_TURN_RIGHT ) != ACTIVITY_NOT_AVAILABLE )
	{// big right turn
		m_IdealActivity = ACT_TURN_RIGHT;
	}
	else if ( flYD > 45 && LookupActivity ( ACT_TURN_LEFT ) != ACTIVITY_NOT_AVAILABLE )
	{// big left turn
		m_IdealActivity = ACT_TURN_LEFT;
	}


	
	if(FClassnameIs(pev, "monster_houndeye") && LookupActivity ( ACT_TURN_RIGHT ) == ACTIVITY_NOT_AVAILABLE && LookupActivity ( ACT_TURN_LEFT ) == ACTIVITY_NOT_AVAILABLE ){
		easyForcePrintLine("HOUNDEYE ISSUE::: SETTURNACTIVITY BETTER BE DOING STUFF...... yawdelta: %.2f resulting act: %d", flYD, m_IdealActivity );
	}


}

//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule. 
//=========================================================
void CBaseMonster :: StartTask ( Task_t *pTask )
{
	//easyForcePrintLine("EACH DAY I DIE SOME MORE StartTask sched:%s: task:%d index:%d", getScheduleName(), pTask->iTask, m_iScheduleIndex);


	if(m_pSchedule == slPathfindStumped){
		easyForcePrintLine("StartTask: %s:%d task: %d", getClassname(), monsterID, pTask->iTask);
	}


	switch(pTask->iTask){
	case TASK_PLAY_SEQUENCE_FACE_ENEMY:
	case TASK_PLAY_SEQUENCE_FACE_TARGET:
	case TASK_PLAY_SEQUENCE:
	if(pTask->flData == ACT_EXCITED || pTask->flData == ACT_CROUCH || pTask->flData == ACT_CROUCHIDLE){
		if(FClassnameIs(this->pev, "monster_scientist")){easyForcePrintLine("I WILL BREAK YOUR FIRST BORN CHILDS FACE!!!");}
	}
	break;
	}


	switch ( pTask->iTask )
	{
	case TASK_TURN_RIGHT:
		{
			float flCurrentYaw;
			
			flCurrentYaw = UTIL_AngleMod( pev->angles.y );
			pev->ideal_yaw = UTIL_AngleMod( flCurrentYaw - pTask->flData );
			SetTurnActivity();
			break;
		}
	case TASK_TURN_LEFT:
		{
			float flCurrentYaw;
			
			flCurrentYaw = UTIL_AngleMod( pev->angles.y );
			pev->ideal_yaw = UTIL_AngleMod( flCurrentYaw + pTask->flData );
			SetTurnActivity();
			break;
		}
	case TASK_REMEMBER:
		{
			Remember ( (int)pTask->flData );
			TaskComplete();
			break;
		}
	case TASK_FORGET:
		{
			Forget ( (int)pTask->flData );
			TaskComplete();
			break;
		}
	case TASK_FIND_HINTNODE:
		{
			m_iHintNode = FindHintNode();

			if ( m_iHintNode != NO_NODE )
			{
				TaskComplete();
			}
			else
			{
				TaskFail();
			}
			break;
		}
	case TASK_STORE_LASTPOSITION:
		{
			m_vecLastPosition = pev->origin;
			TaskComplete();
			break;
		}
	case TASK_CLEAR_LASTPOSITION:
		{
			m_vecLastPosition = g_vecZero;
			TaskComplete();
			break;
		}
	case TASK_CLEAR_HINTNODE:
		{
			m_iHintNode = NO_NODE;
			TaskComplete();
			break;
		}
	case TASK_STOP_MOVING:
		{
			//easyForcePrintLine("WEEEEEEEEEEEEEEEEEEELLLLLLLLLLLLLLLLLLLLLLLLA %d %d %d", m_IdealActivity, m_movementActivity, this->usingCustomSequence);
			if ( m_IdealActivity == m_movementActivity )
			{
				m_IdealActivity = GetStoppedActivity();
			}
			//easyForcePrintLine("WEEEEEEEEEEEEEEEEEEELLLLLLLLLLLLLLLLLLLLLLLLB %d %d %d", m_IdealActivity, m_movementActivity, this->usingCustomSequence);


			RouteClear();
			TaskComplete();
			break;
		}
	case TASK_PLAY_SEQUENCE_FACE_ENEMY:
	case TASK_PLAY_SEQUENCE_FACE_TARGET:
	case TASK_PLAY_SEQUENCE:
		{
			m_IdealActivity = ( Activity )( int )pTask->flData;
			break;
		}
	case TASK_PLAY_ACTIVE_IDLE:
		{
			// monsters verify that they have a sequence for the node's activity BEFORE
			// moving towards the node, so it's ok to just set the activity without checking here.
			m_IdealActivity = ( Activity )WorldGraph.m_pNodes[ m_iHintNode ].m_sHintActivity;
			break;
		}
	case TASK_SET_SCHEDULE:
		{
			Schedule_t *pNewSchedule;

			pNewSchedule = GetScheduleOfType( (int)pTask->flData );
			
			if ( pNewSchedule )
			{
				ChangeSchedule( pNewSchedule );
			}
			else
			{
				TaskFail();
			}

			break;
		}
	case TASK_FIND_NEAR_NODE_COVER_FROM_ENEMY:
		{
			if ( m_hEnemy == NULL )
			{
				TaskFail();
				return;
			}

			if ( FindCover( m_hEnemy->pev->origin, m_hEnemy->pev->view_ofs, 0, pTask->flData ) )
			{
				// try for cover farther than the FLData from the schedule.
				TaskComplete();
			}
			else
			{
				// no coverwhatsoever.
				TaskFail();
			}
			break;
		}
	case TASK_FIND_FAR_NODE_COVER_FROM_ENEMY:
		{
			if ( m_hEnemy == NULL )
			{
				TaskFail();
				return;
			}

			if ( FindCover( m_hEnemy->pev->origin, m_hEnemy->pev->view_ofs, pTask->flData, CoverRadius() ) )
			{
				// try for cover farther than the FLData from the schedule.
				TaskComplete();
			}
			else
			{
				// no coverwhatsoever.
				TaskFail();
			}
			break;
		}
	case TASK_FIND_NODE_COVER_FROM_ENEMY:
		{
			if ( m_hEnemy == NULL )
			{
				TaskFail();
				return;
			}

			if ( FindCover( m_hEnemy->pev->origin, m_hEnemy->pev->view_ofs, 0, CoverRadius() ) )
			{
				// try for cover farther than the FLData from the schedule.
				TaskComplete();
			}
			else
			{
				// no coverwhatsoever.
				TaskFail();
			}
			break;
		}
	case TASK_FIND_COVER_FROM_ENEMY:
		{
			BOOL coverAttempt = attemptFindCoverFromEnemy(pTask);

			if(coverAttempt){
				TaskComplete(); //assume the cover is setup and ready to use.
			}else{
				TaskFail();  //try something else?

				//MODDD - I am important to stop a fidget!
				m_movementGoal = MOVEGOAL_NONE;

			}
			break;
		}
	case TASK_FIND_COVER_FROM_ENEMY_OR_CHASE:
	{
		BOOL coverAttempt = attemptFindCoverFromEnemy(pTask);

		if(coverAttempt){
			TaskComplete(); //assume the cover is setup and ready to use.
		}else{
			ChangeSchedule( GetScheduleOfType(SCHED_CHASE_ENEMY) );
		}
		break;
	}
	case TASK_FIND_COVER_FROM_ENEMY_OR_FIGHT:
	{
		BOOL coverAttempt = attemptFindCoverFromEnemy(pTask);

		//if(coverAttempt){
		//	TaskComplete(); //assume the cover is setup and ready to use.
		//}else{

		//DEBUG - always fight for now.

		{
			
			//the "fight" part, since "flight" doesn't look so good.
			//Really just a repeat of the "SEE" condition script (attack-condition checks) in GetSchedule.
			if ( HasConditions(bits_COND_CAN_RANGE_ATTACK1) )
			{
				ChangeSchedule(GetScheduleOfType( SCHED_RANGE_ATTACK1 ));
				break;
			}
			if ( HasConditions(bits_COND_CAN_RANGE_ATTACK2) )
			{
				ChangeSchedule(GetScheduleOfType( SCHED_RANGE_ATTACK2 ));
				break;
			}
			if ( HasConditions(bits_COND_CAN_MELEE_ATTACK1) )
			{
				ChangeSchedule(GetScheduleOfType( SCHED_MELEE_ATTACK1 ));
				break;
			}
			if ( HasConditions(bits_COND_CAN_MELEE_ATTACK2) )
			{
				ChangeSchedule(GetScheduleOfType( SCHED_MELEE_ATTACK2 ));
				break;
			}

			//if ( !HasConditions(bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK1) )
			{
				// if we can see enemy but can't use either attack type, we must need to get closer to enemy
				//easyPrintLine("ducks2");
				ChangeSchedule(GetScheduleOfType( SCHED_CHASE_ENEMY ));
				break;
			}

			//made it here, how??
			TaskFail();

		}//END OF else OF coverAttempt check

		break;
	}
	case TASK_FIND_COVER_FROM_ORIGIN:
		{
			if ( FindCover( pev->origin, pev->view_ofs, 0, CoverRadius() ) )
			{
				easyForcePrintLine("TASK_FIND_COVER_FROM_ORIGIN: I FOUND COVER OKAYYYYYYYYYY");
				// then try for plain ole cover
				m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
				TaskComplete();
			}
			else
			{
				easyForcePrintLine("TASK_FIND_COVER_FROM_ORIGIN: I FAIL HARD.");
				// no cover!
				TaskFail();
			}
		}
		break;
	case TASK_FIND_COVER_FROM_BEST_SOUND:
		{
			CSound *pBestSound;

			pBestSound = PBestSound();

			ASSERT( pBestSound != NULL );
			/*
			if ( pBestSound && FindLateralCover( pBestSound->m_vecOrigin, g_vecZero ) )
			{
				// try lateral first
				m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
				TaskComplete();
			}
			*/

			//MODDD - assuming this is something like a grenade, we can up the maximum distance. Going to go somewhere regardless.
			//if ( pBestSound && FindCover( pBestSound->m_vecOrigin, g_vecZero, pBestSound->m_iVolume, CoverRadius() ) )
			if ( pBestSound && FindCover( pBestSound->m_vecOrigin, g_vecZero, pBestSound->m_iVolume*0.5, CoverRadius()*3 ) )
			{
				// then try for plain ole cover
				m_flMoveWaitFinished = gpGlobals->time + pTask->flData;
				TaskComplete();
			}
			else
			{
				// no coverwhatsoever. or no sound in list
				TaskFail();
			}
			break;
		}
	//MODDD - NEW
	case TASK_FACE_POINT:
		{
		MakeIdealYaw ( this->m_vecMoveGoal );
		SetTurnActivity(); 
		break;
		}
	case TASK_FACE_HINTNODE:
		{
			pev->ideal_yaw = WorldGraph.m_pNodes[ m_iHintNode ].m_flHintYaw;
			SetTurnActivity();
			break;
		}
	
	case TASK_FACE_LASTPOSITION:
		MakeIdealYaw ( m_vecLastPosition );
		SetTurnActivity(); 
		break;

	case TASK_FACE_TARGET:
		if ( m_hTargetEnt != NULL )
		{
			MakeIdealYaw ( m_hTargetEnt->pev->origin );
			SetTurnActivity(); 
		}
		else
			TaskFail();
		break;
	case TASK_FACE_ENEMY:
		{
			MakeIdealYaw ( m_vecEnemyLKP );
			SetTurnActivity(); 
			break;
		}
	case TASK_FACE_PREV_LKP:
		{
			//HACK - since we know this is only called by the stumped method, do a check to see
			//  if we're going to just route to the enemy's modern position anyways and skip the rest of this schedule.
			if(EASY_CVAR_GET(pathfindStumpedMode) == 3){
				//is this okay?
				m_vecEnemyLKP = m_hEnemy->pev->origin;

				TaskFail();
				ChangeSchedule(GetSchedule());
				return;
			}


			MakeIdealYaw(m_vecEnemyLKP_prev);
			SetTurnActivity(); 
			break;
		}
	case TASK_FACE_IDEAL:
		{
			SetTurnActivity();
			break;
		}
	case TASK_FACE_ROUTE:
		{
			if (FRouteClear())
			{
				ALERT(at_aiconsole, "No route to face!\n");
				TaskFail();
			}
			else
			{
				MakeIdealYaw(m_Route[m_iRouteIndex].vecLocation);
				SetTurnActivity();
			}
			break;
		}
	case TASK_FACE_BEST_SOUND:
		{
			//what????
			//fuck

			CSound *pSound;
			pSound = PBestSound();

			//if ( pSound && MoveToLocation( m_movementActivity, 2, pSound->m_vecOrigin ) )
			if(pSound)
			{
				MakeIdealYaw(pSound->m_vecOrigin);
				SetTurnActivity();
				//TaskComplete();
			}else{
				//is that fine?
				TaskFail();
			}
			
			break;
		}
	case TASK_CHECK_STUMPED:
		{
			easyForcePrintLine("I MUST SAY, I AM STUMPED.");

			//TaskComplete();
			//return;
			//no dont do it!!!!




			//It is possible we're facing our last known position, but just repeatedly getting satisifed with that.
			//Do a check. Are we looking at the enemy right now? If not, we need to force the LKP to the player to seek them.

			if(!HasConditions(bits_COND_SEE_ENEMY)){
				//Not looking at the enemy, but looking at LKP (presumably)?


				if(EASY_CVAR_GET(pathfindStumpedMode) == 0){
					//Forget the enemy, we lost sight. Will pick up on an old remembered enemy pushed into memory in a stack
					//if there is one there.
					m_hEnemy = NULL;
					//GetEnemy();
					TaskComplete();
					return;
				}


				m_vecEnemyLKP_prev = m_vecEnemyLKP; //the old to look at for a little.


				easyForcePrintLine("AHHH. I don\'t see the enemy. Has enemy to seek? %d", (m_hEnemy != NULL));
				if(m_hEnemy != NULL){
					//Go actually look at them next time to break this cycle.

					//safety feature, do this all the time.
					m_vecEnemyLKP = m_hEnemy->pev->origin;
					//Before we resume, pause for a little.
					//...Actually this will suffice, just fail regardless.
					m_failSchedule = SCHED_PATHFIND_STUMPED;  //A variant of FAIL that will be interrupted like idle, by sounds, seeing the enemy, etc.
					TaskFail();
				}else{
					m_failSchedule = SCHED_PATHFIND_STUMPED;
					TaskFail(); //no enemy... what?
				}

			}else{
				easyForcePrintLine("But it is ok, I see the enemy at least.");
				TaskComplete();  //If we actually can see the enemy now, no need for corrective action.
			}

		}
	break;
	case TASK_UPDATE_LKP:
		{
		//Force me to know the enemy's location.
		if(m_hEnemy != NULL){
			m_vecEnemyLKP = m_hEnemy->pev->origin;
		}
		TaskComplete();
		break;
		}
	case TASK_WAIT_PVS:
	case TASK_WAIT_INDEFINITE:
		{
			// don't do anything.
			break;
		}
	case TASK_WAIT:
	case TASK_WAIT_FACE_ENEMY:
		{// set a future time that tells us when the wait is over.
			m_flWaitFinished = gpGlobals->time + pTask->flData;	
			break;
		}
	case TASK_WAIT_RANDOM:
		{// set a future time that tells us when the wait is over.
			m_flWaitFinished = gpGlobals->time + RANDOM_FLOAT( 0.1, pTask->flData );
			break;
		}
	case TASK_WAIT_STUMPED:
		{
			m_flWaitFinished = gpGlobals->time + EASY_CVAR_GET(pathfindStumpedWaitTime);
			break;
		}
	case TASK_MOVE_TO_TARGET_RANGE:
		{

			if(m_hTargetEnt == NULL){
				//HOW DARE YOU.  HOw. DARE. YOU.
				TaskFail();
				return;
			}

			if ( (m_hTargetEnt->pev->origin - pev->origin).Length() < 1 )
				TaskComplete();
			else
			{
				m_vecMoveGoal = m_hTargetEnt->pev->origin;
				if ( !MoveToTarget( ACT_WALK, 2 ) )
					TaskFail();
			}
			break;
		}

		//MODDD - this is forced to involve the enemy LKP now instead, even at the start.
	case TASK_MOVE_TO_ENEMY_RANGE:
		{

			if(m_hEnemy == NULL){
				//what??
				TaskFail();
				return;
			}


			//if ( (m_hEnemy->pev->origin - pev->origin).Length() < 1 )
			if ( (m_vecEnemyLKP - pev->origin).Length() < 1 )
				TaskComplete();
			else
			{
				//if ( !MoveToEnemy( ACT_WALK, 2 ) )
				//	TaskFail();

				//This gets the real enemy location, which may or may not be a good idea. It can seem  like cheating if done way too often to constantly just know where you are.
				//But that's todo.
				//BOOL test = FRefreshRouteChaseEnemySmart();
				BOOL test = FRefreshRouteChaseEnemySmart();
				
				if(!test){
					easyForcePrintLine("!!! %s:%d YOU HAVE ALREADY FAILED.", this->getClassname(), this->monsterID);
					TaskFail();
				};
			}
			break;
		}
	//MODDD - new, clone of TASK_MOVE_TO_TARGET_RANGE but for a point (m_vecMoveGoal) instead.
	case TASK_MOVE_TO_POINT_RANGE:
	{

		//NOTICE: task assumes "m_vecMoveGoal" has been set!
		if ( (m_vecMoveGoal - pev->origin).Length() < 1 ){
			//easyForcePrintLine("HORRIBLE OKAY");
			TaskComplete();
		}else
		{
			//m_vecMoveGoal = m_vecMoveGoal;
			//if ( !MoveToTarget( ACT_WALK, 2 ) ){
			if(!MoveToLocation(m_movementActivity, 2, m_vecMoveGoal)){
				//easyForcePrintLine("HORRIBLE FFFF");
				TaskFail();
			}
		}
		break;
	}

	case TASK_RUN_TO_TARGET:
	case TASK_WALK_TO_TARGET:
		{
			Activity newActivity;

			if(m_hTargetEnt == NULL){
				//easyForcePrintLine("I WILL PLAY JUMPROPE WITH YOUR INTESTINES YOU WORTHLESS SCUMyayER %d", monsterID);
				easyForcePrintLine("I\'m not feeling so fantastic.. (null target)  %s:%d", getClassname(), monsterID);
				TaskFail();
				break;
			}

			if ( (m_hTargetEnt->pev->origin - pev->origin).Length() < 1 )
				TaskComplete();
			else
			{

				//MODDD - if this monster lacks a run act, this won't work out too well.

				if ( pTask->iTask == TASK_WALK_TO_TARGET )
					newActivity = ACT_WALK;
				else
					newActivity = ACT_RUN;



				// This monster can't do this!
				if ( LookupActivity( newActivity ) == ACTIVITY_NOT_AVAILABLE )
					TaskComplete();
				else 
				{
					if ( m_hTargetEnt == NULL || !MoveToTarget( newActivity, 2 ) )
					{
						TaskFail();
						ALERT( at_aiconsole, "%s Failed to reach target!!!\n", STRING(pev->classname) );
						RouteClear();
					}
				}
			}
			TaskComplete();
			break;
		}
	case TASK_CLEAR_MOVE_WAIT:
		{
			m_flMoveWaitFinished = gpGlobals->time;
			TaskComplete();
			break;
		}
	case TASK_MELEE_ATTACK1_NOTURN:
	case TASK_MELEE_ATTACK1:
		{
			m_IdealActivity = ACT_MELEE_ATTACK1;
			//this->signalActivityUpdate = TRUE;
			break;
		}
	case TASK_MELEE_ATTACK2_NOTURN:
	case TASK_MELEE_ATTACK2:
		{
			m_IdealActivity = ACT_MELEE_ATTACK2;
			//this->signalActivityUpdate = TRUE;
			break;
		}
	case TASK_RANGE_ATTACK1_NOTURN:
	case TASK_RANGE_ATTACK1:
		{
			m_IdealActivity = ACT_RANGE_ATTACK1;

			//MODDD - CRITICAL NEW.
			//this->signalActivityUpdate = TRUE;
			//Force the activity to pick a new anim even if already on that activity.
			//This stops the monster from freezing on the last set animation if already on the activity for some reason.
			//...ACTUALLY don't do this. Look at the end of TASK_RANGE_ATTACK1 and others for setting the current activity to ACT_RESET.
			//This effectively forces the sequence to be regathered too.  Doing it here too is actually redundant.
			//See if a monster isn't doing the same at the end of their own TASK_RANGE_ATTACK1 or similar.
			break;
		}
	case TASK_RANGE_ATTACK2_NOTURN:
	case TASK_RANGE_ATTACK2:
		{
			m_IdealActivity = ACT_RANGE_ATTACK2;
			//this->signalActivityUpdate = TRUE;
			break;
		}

	//MODDD - new. This task acts as a gate: only pass if we are able to make an attack. Otherwise fail this schedule.
	case TASK_CHECK_RANGED_ATTACK_1:
		{
			if(HasConditions(bits_COND_CAN_RANGE_ATTACK1)){
				TaskComplete();
			}else{
				TaskFail();
			}

			break;
		}
	case TASK_RELOAD_NOTURN:
	case TASK_RELOAD:
		{
			m_IdealActivity = ACT_RELOAD;
			break;
		}
	case TASK_SPECIAL_ATTACK1:
		{
			m_IdealActivity = ACT_SPECIAL_ATTACK1;
			break;
		}
	case TASK_SPECIAL_ATTACK2:
		{
			m_IdealActivity = ACT_SPECIAL_ATTACK2;
			break;
		}
	case TASK_SET_ACTIVITY:
		{
			m_IdealActivity = (Activity)(int)pTask->flData;
			TaskComplete();
			break;
		}


		//MODDD - We're forcing even GET_PATH_TO_ENEMY to use the LastKnownPath instead.
	case TASK_GET_PATH_TO_ENEMY:
	case TASK_GET_PATH_TO_ENEMY_LKP:
		{

			//const char* schedName = m_pSchedule->pName;

			
			//if ( BuildRoute ( m_vecEnemyLKP, bits_MF_TO_LOCATION, NULL ) )

			//is it safe to use bits_MF_TO_ENEMY to get "MOVEGOAL_ENEMY" anyways?
			//Looks like it. This just says to send the current "m_hEnemy" to path methods like CheckLocalMove to mark them as exceptions for
			//colliding with. After all it would be silly to say "Path to player failed", because the "player" was in the way of the destination point!
			if ( BuildRoute ( m_vecEnemyLKP, bits_MF_TO_ENEMY, NULL ) )
			{
				TaskComplete();
			}
			else if (BuildNearestRoute( m_vecEnemyLKP, pev->view_ofs, 0, (m_vecEnemyLKP - pev->origin).Length() )  )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToEnemyLKP failed!!\n" );
				TaskFail();

				//MODDD - be sure to set the movegoal to NONE when a path-building task fails.
				//        This stops the monster from fidgeting for a frame futily moving.
				m_movementGoal = MOVEGOAL_NONE;
				//just in case...?
			}
			break;
		}

		/*
	case TASK_GET_PATH_TO_ENEMY:
		{
			CBaseEntity *pEnemy = m_hEnemy;

			if ( pEnemy == NULL )
			{
				TaskFail();
				return;
			}

			//Clearly we want to just route to the enemy. Update the LKP to not confuse the pathfinding.
			m_vecEnemyLKP = m_hEnemy->pev->origin;

			if ( BuildRoute ( pEnemy->pev->origin, bits_MF_TO_ENEMY, pEnemy ) )
			{
				TaskComplete();
			}
			else if (BuildNearestRoute( pEnemy->pev->origin, pEnemy->pev->view_ofs, 0, (pEnemy->pev->origin - pev->origin).Length() ))
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
		*/
	case TASK_GET_PATH_TO_ENEMY_CORPSE:
		{

			//MODDD NOTE - is trusting that the "m_vecEnemyLKP" is the same as the enemy corpse position okay?
			UTIL_MakeVectors( pev->angles );
			if ( BuildRoute ( m_vecEnemyLKP - gpGlobals->v_forward * 64, bits_MF_TO_LOCATION, NULL ) )
			{
				TaskComplete();
			}
			else
			{
				ALERT ( at_aiconsole, "GetPathToEnemyCorpse failed!!\n" );
				TaskFail();
				m_movementGoal = MOVEGOAL_NONE;
			}
		}
		break;
	case TASK_GET_PATH_TO_SPOT:
		{
			CBaseEntity *pPlayer = CBaseEntity::Instance( FIND_ENTITY_BY_CLASSNAME( NULL, "player" ) );
			if ( BuildRoute ( m_vecMoveGoal, bits_MF_TO_LOCATION, pPlayer ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToSpot failed!!\n" );
				TaskFail();
				m_movementGoal = MOVEGOAL_NONE;
			}
			break;
		}

	case TASK_GET_PATH_TO_TARGET:
		{
			RouteClear();
			if ( m_hTargetEnt != NULL && MoveToTarget( m_movementActivity, 1 ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToSpot failed!!\n" );
				TaskFail();
				m_movementGoal = MOVEGOAL_NONE;
			}
			break;
		}
	case TASK_GET_PATH_TO_HINTNODE:// for active idles!
		{
			if ( MoveToLocation( m_movementActivity, 2, WorldGraph.m_pNodes[ m_iHintNode ].m_vecOrigin ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToHintNode failed!!\n" );
				TaskFail();
				m_movementGoal = MOVEGOAL_NONE;
			}
			break;
		}
	case TASK_GET_PATH_TO_LASTPOSITION:
		{
			m_vecMoveGoal = m_vecLastPosition;

			if ( MoveToLocation( m_movementActivity, 2, m_vecMoveGoal ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToLastPosition failed!!\n" );
				TaskFail();
				m_movementGoal = MOVEGOAL_NONE;
			}
			break;
		}
	case TASK_GET_PATH_TO_BESTSOUND:
		{
			CSound *pSound;

			pSound = PBestSound();

			if ( pSound && MoveToLocation( m_movementActivity, 2, pSound->m_vecOrigin ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToBestSound failed!!\n" );
				TaskFail();
				m_movementGoal = MOVEGOAL_NONE;
			}
			break;
		}
case TASK_GET_PATH_TO_BESTSCENT:
		{
			CSound *pScent;

			pScent = PBestScent();

			if ( pScent && MoveToLocation( m_movementActivity, 2, pScent->m_vecOrigin ) )
			{
				TaskComplete();
			}
			else
			{
				// no way to get there =(
				ALERT ( at_aiconsole, "GetPathToBestScent failed!!\n" );
				
				TaskFail();
				m_movementGoal = MOVEGOAL_NONE;
			}
			break;
		}
	case TASK_RUN_PATH:
		{
			// UNDONE: This is in some default AI and some monsters can't run? -- walk instead?
			if ( LookupActivity( ACT_RUN ) != ACTIVITY_NOT_AVAILABLE )
			{
				m_movementActivity = ACT_RUN;
			}
			else
			{
				m_movementActivity = ACT_WALK;
			}
			TaskComplete();
			break;
		}
	case TASK_WALK_PATH:
		{
			if ( pev->movetype == MOVETYPE_FLY )
			{
				m_movementActivity = ACT_FLY;
			}

			//MODDD NOTE - was the lack of "else" here intentional? That means a check for ACT_WALK will be done regardless and used instead. Most likely so.
			if ( LookupActivity( ACT_WALK ) != ACTIVITY_NOT_AVAILABLE )
			{
				m_movementActivity = ACT_WALK;
			}
			else
			{
				m_movementActivity = ACT_RUN;
			}
			TaskComplete();
			break;
		}
	case TASK_STRAFE_PATH:
		{
			Vector2D	vec2DirToPoint; 
			Vector2D	vec2RightSide;

			// to start strafing, we have to first figure out if the target is on the left side or right side
			UTIL_MakeVectors ( pev->angles );

			vec2DirToPoint = ( m_Route[ 0 ].vecLocation - pev->origin ).Make2D().Normalize();
			vec2RightSide = gpGlobals->v_right.Make2D().Normalize();

			if ( DotProduct ( vec2DirToPoint, vec2RightSide ) > 0 )
			{
				// strafe right
				m_movementActivity = ACT_STRAFE_RIGHT;
			}
			else
			{
				// strafe left
				m_movementActivity = ACT_STRAFE_LEFT;
			}
			TaskComplete();
			break;
		}


	case TASK_WAIT_FOR_MOVEMENT:
		{
			//easyPrintLine("IMA vividly do something rather foul %d", FRouteClear());
			if (FRouteClear())
			{
				TaskComplete();
			}
			break;
		}
		//MODDD - going to let the "run" method handle this one better...
	case TASK_WAIT_FOR_MOVEMENT_RANGE:
		{
			//easyPrintLine("IMA vividly do something rather foul %d", FRouteClear());
			if (FRouteClear())
			{
				TaskComplete();
			}
			break;
		}
	case TASK_EAT:
		{
			Eat( pTask->flData );
			TaskComplete();
			break;
		}
	case TASK_SMALL_FLINCH:
		{
			m_IdealActivity = GetSmallFlinchActivity();
			break;
		}
	case TASK_DIE:
		{
			
			
			DeathAnimationStart();
			

			break;
		}
	case TASK_DIE_LOOP:
	{
		//Starter for the task.
		//In a monster that calls for SCHED_DIE_LOOP instead of SCHED_DIE, ensure "getLoopingDeathSequence" is overridden to refer
		//to a fitting (falling?) sequence to loop until it has a reason to be interrupted (hit the ground)
		//It is still up to the monster itself to tell how TASK_DIE_LOOP calls TaskComplete (on hitting the ground).
		this->SetSequenceByIndex(getLoopingDeathSequence(), 1.0f);

		//These calls / settings are based off of "DeathAnimationStart" from basemonster.cpp.
		//It is implied this sort of thing happens at the start of death.
		RouteClear();
		deadSetActivityBlock = TRUE;

		
		//don't force re-getting an animation just yet.
		//A new animation comes from a discrepency between m_Activity and m_IdealActivity, so forcing both stops regetting an animation.
		//also BOB SAGGETS FUCKING ASS. GetDeathActivity doesn't work if the pev->deadflag isn't DEAD_NO.
		m_IdealActivity = GetDeathActivity();
		m_Activity = m_IdealActivity;
		
		pev->deadflag = DEAD_DYING;
		


		break;
	}
	case TASK_SOUND_WAKE:
		{
			AlertSound();
			TaskComplete();
			break;
		}
	case TASK_SOUND_DIE:
		{
			DeathSound();
			TaskComplete();
			break;
		}
	case TASK_SOUND_IDLE:
		{
			IdleSound();
			TaskComplete();
			break;
		}
	case TASK_SOUND_PAIN:
		{
			PainSound();
			TaskComplete();
			break;
		}
	case TASK_SOUND_DEATH:
		{
			DeathSound();
			TaskComplete();
			break;
		}
	case TASK_SOUND_ANGRY:
		{
			// sounds are complete as soon as we get here, cause we've already played them.
			ALERT ( at_aiconsole, "SOUND\n" );			
			TaskComplete();
			break;
		}
	case TASK_WAIT_FOR_SCRIPT:
		{
			if (m_pCine->m_iszIdle)
			{
				m_pCine->StartSequence( (CBaseMonster *)this, m_pCine->m_iszIdle, FALSE );
				if (FStrEq( STRING(m_pCine->m_iszIdle), STRING(m_pCine->m_iszPlay)))
				{
					pev->framerate = 0;
				}
			}
			else
				m_IdealActivity = ACT_IDLE;

			break;
		}
	case TASK_PLAY_SCRIPT:
		{
			pev->movetype = MOVETYPE_FLY;
			ClearBits(pev->flags, FL_ONGROUND);
			m_scriptState = SCRIPT_PLAYING;
			break;
		}
	case TASK_ENABLE_SCRIPT:
		{
			m_pCine->DelayStart( 0 );
			TaskComplete();
			break;
		}
	case TASK_PLANT_ON_SCRIPT:
		{
			if ( m_hTargetEnt != NULL )
			{
				pev->origin = m_hTargetEnt->pev->origin;	// Plant on target
			}

			TaskComplete();
			break;
		}
	case TASK_FACE_SCRIPT:
		{
			if ( m_hTargetEnt != NULL )
			{
				pev->ideal_yaw = UTIL_AngleMod( m_hTargetEnt->pev->angles.y );
			}

			TaskComplete();
			m_IdealActivity = ACT_IDLE;
			RouteClear();
			break;
		}

	case TASK_SUGGEST_STATE:
		{
			m_IdealMonsterState = (MONSTERSTATE)(int)pTask->flData;
			TaskComplete();
			break;
		}

	case TASK_SET_FAIL_SCHEDULE:
		m_failSchedule = (int)pTask->flData;
		TaskComplete();
		break;

	case TASK_CLEAR_FAIL_SCHEDULE:
		m_failSchedule = SCHED_NONE;
		TaskComplete();
		break;

	case TASK_SET_SEQUENCE_BY_NUMBER:
		{
			setAnimationSmart((int)pTask->flData, 1.0f);
			break;
		}
	//MODDD - new
	case TASK_RANDOMWANDER_CHECKSEEKSHORT:
	{
		Vector vecStart;
		Vector vecEnd;
		Vector vecFinalEnd;
		Vector vecTempOff;
		int randomTries;
		Vector randomTargetVectorAttempt;
		float randomTargetYawAttempt;
		BOOL success;
		float totalDist;
		float distReg;

		BOOL vecLastTrySuccess;
		int vecLastTryLength;
		Vector vecLastTry[4];
		Vector vecLastTryOrigin;

		//SEEK SHORT wants to pick a random direction and see if we can straight-shot it for a very short distance (not path finding).
			
		//not needed?
		//UTIL_MakeVectorsPrivate(pev->angles, vec_forward, vec_right, vec_up);

		//how many tries until we give up.
		randomTries = RANDOMWANDER_TRIES;

			
			vecStart = pev->origin + Vector(0, 0, 6);

		vecLastTryOrigin = pev->origin + Vector(0, 0, 6);

		while(randomTries > 0){

			randomTargetYawAttempt = RANDOM_FLOAT(0, 359.99);

			randomTargetVectorAttempt = UTIL_YawToVec(randomTargetYawAttempt);

			//try to go in this direction.


			//at least this test.
			vecEnd = vecStart + randomTargetVectorAttempt * 120;

			totalDist = (vecEnd - vecStart).Length();


			//record for drawing for debug purposes.
			vecLastTry[ RANDOMWANDER_TRIES - randomTries ] = vecEnd; 


			//test!
			success = this->CheckLocalMove(vecStart, vecEnd, NULL, &distReg);
				
			if(success){
				//because on success, distReg is likely not written to. Bizarre.
				distReg = totalDist;
			}

			//easyForcePrintLine("IS IT OK OR NOT?! %d %.2f", success, distReg);
			if(distReg > 55){
				//if okay?
					
				pev->ideal_yaw = randomTargetYawAttempt;

				//vecHopDest = vecEnd;
				vecTempOff = randomTargetVectorAttempt * RANDOM_FLOAT(40, distReg - 15) ;

				//NOTE: is this safe?
				this->m_vecMoveGoal = vecStart + vecTempOff;

				
					
				//UTIL_printVector("ye", vecStart);
				//UTIL_printVector("ye", randomTargetVectorAttempt);
				//UTIL_printVector("ye", vecTempOff);
				//UTIL_printVector("ye", vecHopDest);


				vecLastTrySuccess = TRUE;
				vecLastTryLength = (RANDOMWANDER_TRIES - randomTries) + 1;
				//done! Have a destination.
				TaskComplete();
				return;
			}



			randomTries--;

		}//END OF while(randomTries > 0)

		vecLastTrySuccess = FALSE;
		//all four were done.
		vecLastTryLength = RANDOMWANDER_TRIES;

		//Reached here? If the loop didn't call "TaskComplete" and return, this was reached.
		//Fail.
		TaskFail();
		break;
	}
  
  
	case TASK_RANDOMWANDER_TEST:

		if ( MoveToLocation( m_movementActivity, 2, this->m_vecMoveGoal ) )
		{
			//EASY_CVAR_PRINTIF_PRE(chumtoadPrintout, easyPrintLine("TASK_TOAD_HOPSHORT: path okay") );
			TaskComplete();
		}else{
			//EASY_CVAR_PRINTIF_PRE(chumtoadPrintout, easyPrintLine("TASK_TOAD_HOPSHORT: path not okay. How at this stage?") );
			//How???
			TaskFail();
		}
	break;
  
  







	default:
		{
			ALERT ( at_aiconsole, "No StartTask entry for %d\n", (SHARED_TASKS)pTask->iTask );
			break;
		}
	}
}

//=========================================================
// GetTask - returns a pointer to the current 
// scheduled task. NULL if there's a problem.
//=========================================================
Task_t	*CBaseMonster :: GetTask ( void ) 
{
	//MODDD - any random crashes caused by this?  Doubt it, but a null check never hurts.
	if(m_pSchedule == NULL){
		return NULL;
	}


	if ( m_iScheduleIndex < 0 || m_iScheduleIndex >= m_pSchedule->cTasks )
	{
		// m_iScheduleIndex is not within valid range for the monster's current schedule.
		return NULL;
	}
	else
	{
		return &m_pSchedule->pTasklist[ m_iScheduleIndex ];
	}
}


//MODDD - get schedule name, typically for printouts.
const char* CBaseMonster::getScheduleName(void){
	if(m_pSchedule != NULL){
		return m_pSchedule->pName;
	}else{
		return "NULL!";
	}
}
//MODDD - new, intended for printout ease (not necessarily limited to)
int CBaseMonster::getTaskNumber(void){
	Task_t* attempt = GetTask();
	if(attempt != NULL){
		return attempt->iTask;
	}else{
		return -1;
	}
}




//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *CBaseMonster :: GetSchedule ( void )
{
	//MODDD - safety.
	if(iAmDead){
		return GetScheduleOfType( SCHED_DIE );
	}
	SCHEDULE_TYPE baitSched = getHeardBaitSoundSchedule();

	if(baitSched != SCHED_NONE){
		return GetScheduleOfType ( baitSched );
	}

	switch	( m_MonsterState )
	{
	case MONSTERSTATE_PRONE:
		{
			return GetScheduleOfType( SCHED_BARNACLE_VICTIM_GRAB );
			break;
		}
	case MONSTERSTATE_NONE:
		{
			ALERT ( at_aiconsole, "MONSTERSTATE IS NONE!\n" );
			break;
		}
	case MONSTERSTATE_IDLE:
		{
			if ( HasConditions ( bits_COND_HEAR_SOUND ) )
			{
				return GetScheduleOfType( SCHED_ALERT_FACE );
			}
			else if ( FRouteClear() )
			{
				// no valid route!
				return GetScheduleOfType( SCHED_IDLE_STAND );
			}
			else
			{
				// valid route. Get moving
				return GetScheduleOfType( SCHED_IDLE_WALK );
			}
			break;
		}
	case MONSTERSTATE_ALERT:
		{
			if ( HasConditions( bits_COND_ENEMY_DEAD ) && LookupActivity( ACT_VICTORY_DANCE ) != ACTIVITY_NOT_AVAILABLE )
			{
				return GetScheduleOfType ( SCHED_VICTORY_DANCE );
			}

			if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE) )
			{
				if ( fabs( FlYawDiff() ) < (1.0 - m_flFieldOfView) * 60 ) // roughly in the correct direction
				{
					return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ORIGIN );
				}
				else
				{
					return GetScheduleOfType( SCHED_ALERT_SMALL_FLINCH );
				}
			}

			else if ( HasConditions ( bits_COND_HEAR_SOUND ) )
			{
				return GetScheduleOfType( SCHED_ALERT_FACE );
			}
			else
			{
				return GetScheduleOfType( SCHED_ALERT_STAND );
			}
			break;
		}
	case MONSTERSTATE_COMBAT:
		{
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// clear the current (dead) enemy and try to find another.
				m_hEnemy = NULL;

				if ( GetEnemy() )
				{
					ClearConditions( bits_COND_ENEMY_DEAD );
					return GetSchedule();
				}
				else
				{
					SetState( MONSTERSTATE_ALERT );
					return GetSchedule();
				}
			}

			if ( HasConditions(bits_COND_NEW_ENEMY) )
			{
				return GetScheduleOfType ( SCHED_WAKE_ANGRY );
			}
			//MODDD - other condition.  If "noFlinchOnHard" is on and the skill is hard, don't flinch from getting hit.
			else if (HasConditions(bits_COND_LIGHT_DAMAGE) && !HasMemory( bits_MEMORY_FLINCHED) && !(global_noFlinchOnHard==1 && g_iSkillLevel==SKILL_HARD)  )
			{
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
			}
			else if ( !HasConditions(bits_COND_SEE_ENEMY) )
			{
				// we can't see the enemy
				if ( !HasConditions(bits_COND_ENEMY_OCCLUDED) )
				{
					// enemy is unseen, but not occluded!
					// turn to face enemy
					return GetScheduleOfType( SCHED_COMBAT_FACE );
				}
				else
				{
					// chase!
					//easyPrintLine("ducks??");

					if(m_hEnemy != NULL && IRelationship(m_hEnemy) == R_FR){
						//if I fear this enemy, and they are not seen and are occluded, just state.
						return GetScheduleOfType(SCHED_COMBAT_FACE);
					}else{
						return GetScheduleOfType( SCHED_CHASE_ENEMY );
					}
				}
			}
			else  
			{

				//easyPrintLine("I say, really now? %d %d", HasConditions(bits_COND_CAN_RANGE_ATTACK1), HasConditions(bits_COND_CAN_RANGE_ATTACK2) );



				if(m_hEnemy != NULL && IRelationship(m_hEnemy) == R_FR){
					if( HasConditions(bits_COND_CAN_MELEE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK2) ){
						//if either melee attack is possible, lower chance of running away.
						if(RANDOM_LONG(0, 4) <= 0){
							return GetScheduleOfType( SCHED_FIGHT_OR_FLIGHT );
						}
					}else{
						//can't melee? higher chance of running away.
						if(RANDOM_LONG(0, 4) <= 2){
							return GetScheduleOfType( SCHED_FIGHT_OR_FLIGHT );
						}
					}
				}


				// we can see the enemy
				if ( HasConditions(bits_COND_CAN_RANGE_ATTACK1) )
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
				}
				if ( HasConditions(bits_COND_CAN_RANGE_ATTACK2) )
				{
					return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
				}
				if ( HasConditions(bits_COND_CAN_MELEE_ATTACK1) )
				{
					return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
				}
				if ( HasConditions(bits_COND_CAN_MELEE_ATTACK2) )
				{
					return GetScheduleOfType( SCHED_MELEE_ATTACK2 );
				}
				//MODDD - NOTE - is that intentional?  range1 & melee1,  and not say,  melee1 & melee2???
				//MODDD - ok, this condition is actually redundant. If all 4 condition checks above failed, each RANGE and MELEE attack, 1 and 2, failed.
				//        That means RANGE1 and MELEE1 also had to have failed. This is guaranteed true.
				if ( !HasConditions(bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK1) )
				{

					if(m_hEnemy != NULL && IRelationship(m_hEnemy) == R_FR){
						//if I fear the enemy and have no possible attacks, have another 3/4 chance of running away.
						if(RANDOM_LONG(0, 3) < 3){
							//of course if we can't find cover, chase anyways.
							return GetScheduleOfType( ::SCHED_TAKE_COVER_FROM_ENEMY_OR_CHASE );
						}
					}



					// if we can see enemy but can't use either attack type, we must need to get closer to enemy
					//easyPrintLine("ducks2");
					return GetScheduleOfType( SCHED_CHASE_ENEMY );

				}
				else if ( !FacingIdeal() )
				{
					//turn
					return GetScheduleOfType( SCHED_COMBAT_FACE );
				}
				else
				{
					ALERT ( at_aiconsole, "No suitable combat schedule!\n" );
				}
			}
			break;
		}
	case MONSTERSTATE_DEAD:
		{
			return GetScheduleOfType( SCHED_DIE );
			break;
		}
	case MONSTERSTATE_SCRIPT:
		{
			//
			//ASSERT( m_pCine != NULL );

			if(m_pCine == NULL){
				easyPrintLine("WARNING: m_pCine IS NULL!");
			}

			if ( !m_pCine )
			{
				ALERT( at_aiconsole, "Script failed for %s\n", STRING(pev->classname) );
				CineCleanup();
				return GetScheduleOfType( SCHED_IDLE_STAND );
			}

			return GetScheduleOfType( SCHED_AISCRIPT );
		}
	default:
		{
			ALERT ( at_aiconsole, "Invalid State for GetSchedule!\n" );
			break;
		}
	}

	return &slError[ 0 ];
}
