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
#ifndef DEFAULTAI_H
#define DEFAULTAI_H

extern Schedule_t slWaitForSequence[];

//=========================================================
// Failed
//=========================================================
extern Schedule_t	slFail[];

extern Schedule_t slPathfindStumped[];


//=========================================================
//	Idle Schedules
//=========================================================
extern Schedule_t	slIdleStand[];
extern Schedule_t	slIdleTrigger[];
extern Schedule_t	slIdleWalk[];

//=========================================================
//	Wake Schedules
//=========================================================
extern Schedule_t slWakeAngry[];

//=========================================================
// AlertTurn Schedules
//=========================================================
extern Schedule_t	slAlertFace[];

//=========================================================
// AlertIdle Schedules
//=========================================================
extern Schedule_t	slAlertStand[];

//=========================================================
// CombatIdle Schedule
//=========================================================
extern Schedule_t	slCombatStand[];

//=========================================================
// CombatFace Schedule
//=========================================================
extern Schedule_t	slCombatFace[];
extern Schedule_t slCombatFaceNoStump[];
extern Schedule_t slCombatFaceSound[];

//=========================================================
// reload schedule
//=========================================================
extern Schedule_t slReload[];

//=========================================================
//	Attack Schedules
//=========================================================

extern Schedule_t	slRangeAttack1[];
extern Schedule_t	slRangeAttack2[];

extern Schedule_t	slTakeCoverFromBestSound[];



// primary melee attack
//MODDD - WARNING - even in the as-is script, this seems to be out of date. There are now two melee attack schedules, and neither is called just this.
//extern Schedule_t	slMeleeAttack[];
extern Schedule_t slPrimaryMeleeAttack[];
extern Schedule_t slSecondaryMeleeAttack[];

	 
// Chase enemy schedule
extern Schedule_t slChaseEnemy[];
extern Schedule_t slChaseEnemySmart[];

//=========================================================
// small flinch, used when a relatively minor bit of damage
// is inflicted.
//=========================================================
extern Schedule_t slSmallFlinch[];

//=========================================================
// Die!
//=========================================================
extern Schedule_t slDie[];

//=========================================================
//	Universal Error Schedule
//=========================================================
extern Schedule_t slError[];

//=========================================================
//	Scripted sequences
//=========================================================
extern Schedule_t slWalkToScript[];
extern Schedule_t slRunToScript[];
extern Schedule_t slWaitScript[];

#endif		// DEFAULTAI_H