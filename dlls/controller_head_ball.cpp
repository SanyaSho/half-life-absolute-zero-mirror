
#include "controller_head_ball.h"
#include "weapons.h"



LINK_ENTITY_TO_CLASS( controller_head_ball, CControllerHeadBall );

CControllerHeadBall::CControllerHeadBall(void){

	nextNormalThinkTime = 0;
}

void CControllerHeadBall :: Spawn( void )
{
	Precache( );
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "sprites/xspark4.spr");
	pev->rendermode = kRenderTransAdd;
	pev->rendercolor.x = 255;
	pev->rendercolor.y = 255;
	pev->rendercolor.z = 255;
	pev->renderamt = 255;
	pev->scale = 2.0;

	UTIL_SetSize(pev, Vector( 0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin( pev, pev->origin );

	SetThink( &CControllerHeadBall::HuntThink );
	SetTouch( &CControllerHeadBall::BounceTouch );

	m_vecIdeal = Vector( 0, 0, 0 );

	pev->nextthink = gpGlobals->time + 0.1;

	m_hOwner = Instance( pev->owner );
	pev->dmgtime = gpGlobals->time;
}


extern int global_useSentenceSave;
void CControllerHeadBall :: Precache( void )
{

	PRECACHE_MODEL("sprites/xspark1.spr");
	
	global_useSentenceSave = TRUE;
	PRECACHE_SOUND("weapons/electro4.wav", TRUE);//don't skip. This is precached by the player gauss, just keep it.
	PRECACHE_SOUND("debris/zap4.wav");
	global_useSentenceSave = FALSE;

}


void CControllerHeadBall :: HuntThink( void  )
{

	//MODDD - I think twice as fast only to spawn the used-to-be-commented-out lighning effect more often.
	//        Still do the rest of the logic at the same rate (0.1 seconds) like how Mr. friendly vomit handles it.
	pev->nextthink = gpGlobals->time + 0.05;



	if(gpGlobals->time >= nextNormalThinkTime){
		
		pev->renderamt -= 5;

		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_ELIGHT );
			WRITE_SHORT( entindex( ) );		// entity, attachment
			WRITE_COORD( pev->origin.x );		// origin
			WRITE_COORD( pev->origin.y );
			WRITE_COORD( pev->origin.z );
			WRITE_COORD( pev->renderamt / 16 );	// radius
			WRITE_BYTE( 255 );	// R
			WRITE_BYTE( 255 );	// G
			WRITE_BYTE( 255 );	// B
			WRITE_BYTE( 2 );	// life * 10
			WRITE_COORD( 0 ); // decay
		MESSAGE_END();

		// check world boundaries
		if (gpGlobals->time - pev->dmgtime > 5 || pev->renderamt < 64 || m_hEnemy == NULL || m_hOwner == NULL || pev->origin.x < -4096 || pev->origin.x > 4096 || pev->origin.y < -4096 || pev->origin.y > 4096 || pev->origin.z < -4096 || pev->origin.z > 4096)
		{
			SetTouch( NULL );
			UTIL_Remove( this );
			return;
		}

		MovetoTarget( m_hEnemy->Center( ) );

		if ((m_hEnemy->Center() - pev->origin).Length() < 64)
		{
			TraceResult tr;

			UTIL_TraceLine( pev->origin, m_hEnemy->Center(), dont_ignore_monsters, ENT(pev), &tr );

			CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);
			if (pEntity != NULL && pEntity->pev->takedamage)
			{
				ClearMultiDamage( );
				pEntity->TraceAttack( m_hOwner->pev, gSkillData.controllerDmgZap, pev->velocity, &tr, DMG_SHOCK );
				ApplyMultiDamage( pev, m_hOwner->pev );
			}

			//MODDD - noise boost. and color cange.
			//ALSO, this looks similar to the Crawl method's TE_BEAMENTPOINT, but this only happens if very close to the player.
			//To look like a connection with the player right before the shock damage likely?
			MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
				WRITE_BYTE( TE_BEAMENTPOINT );
				WRITE_SHORT( entindex() );
				WRITE_COORD( tr.vecEndPos.x );
				WRITE_COORD( tr.vecEndPos.y );
				WRITE_COORD( tr.vecEndPos.z );
				WRITE_SHORT( g_sModelIndexLaser );
				WRITE_BYTE( 0 ); // frame start
				WRITE_BYTE( 10 ); // framerate
				WRITE_BYTE( 2 ); // life. WAS 3.
				WRITE_BYTE( 20 );  // width
				WRITE_BYTE( 80 );   // noise. WAS 0
				WRITE_BYTE( 255 );   // r, g, b. Were all 255 before.
				WRITE_BYTE( 255 );   // r, g, b
				WRITE_BYTE( 80 );   // r, g, b
				WRITE_BYTE( 255 );	// brightness
				WRITE_BYTE( 10 );		// speed
			MESSAGE_END();
			

			UTIL_EmitAmbientSound_Filtered( ENT(pev), tr.vecEndPos, "weapons/electro4.wav", 0.5, ATTN_NORM, 0, RANDOM_LONG( 140, 160 ), FALSE );

			m_flNextAttack = gpGlobals->time + 3.0;

			SetThink( &CControllerHeadBall::DieThink );
			pev->nextthink = gpGlobals->time + 0.3;
		}

		nextNormalThinkTime = gpGlobals->time + 0.1;

	}//END OF nextNormalThinkTime check

	
	//MODDD - this call used to be commented out, enabled because we're crazy! Draws some white lines around the ball I guess.
	Crawl( );
}//END OF HuntThink


void CControllerHeadBall :: DieThink( void  )
{
	UTIL_Remove( this );
}


void CControllerHeadBall :: MovetoTarget( Vector vecTarget )
{
	// accelerate
	float flSpeed = m_vecIdeal.Length();
	if (flSpeed == 0)
	{
		m_vecIdeal = pev->velocity;
		flSpeed = m_vecIdeal.Length();
	}

	if (flSpeed > 400)
	{
		m_vecIdeal = m_vecIdeal.Normalize( ) * 400;
	}
	m_vecIdeal = m_vecIdeal + (vecTarget - pev->origin).Normalize() * 100;
	pev->velocity = m_vecIdeal;
}



void CControllerHeadBall :: Crawl( void  )
{

	Vector vecAim = Vector( RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ), RANDOM_FLOAT( -1, 1 ) ).Normalize( );
	Vector anticipatedOrigin = pev->origin + pev->velocity * 0.3;
	Vector vecPnt = anticipatedOrigin + vecAim * 64;



	//MODDD NOTE - being straight white lines isn't very electricity-like. How about wavy (noise) and a tint of something else? purple?
	//             Based off the islave, 20 looks fairly direct and 80 is pretty wild.

	/*
	easyForcePrintLine("CControllerHeadBall: Crawl: %d: or:(%.2f %.2f %.2f) v:(%.2f %.2f %.2f) gl:(%.2f %.2f %.2f)",
		entindex(),
		pev->origin.x, pev->origin.y, pev->origin.z,
		pev->velocity.x, pev->velocity.y, pev->velocity.z,
		anticipatedOrigin.x, anticipatedOrigin.y, anticipatedOrigin.z
	);
	*/


	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMENTPOINT );
		WRITE_SHORT( entindex() );
		WRITE_COORD( vecPnt.x);
		WRITE_COORD( vecPnt.y);
		WRITE_COORD( vecPnt.z);
		WRITE_SHORT( g_sModelIndexLaser );
		WRITE_BYTE( 0 ); // frame start
		WRITE_BYTE( 10 ); // framerate
		WRITE_BYTE( 2 ); // life. WAS 3.
		WRITE_BYTE( 20 );  // width
		WRITE_BYTE( 80 );   // noise.  WAS 0
		WRITE_BYTE( 255 );   // r, g, b. Were all 255 before.
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 80 );   // r, g, b
		WRITE_BYTE( 255 );	// brightness
		WRITE_BYTE( 10 );		// speed
	MESSAGE_END();
	

	
	/*
	UTIL_drawLineFrame(pev->origin - Vector(0,0,46), pev->origin + Vector(0,0,46), 16, 0, 255, 0);

	//MODDD - DEBUG
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMENTPOINT );
		WRITE_SHORT( entindex() );
		WRITE_COORD( pev->origin.x);
		WRITE_COORD( pev->origin.y);
		WRITE_COORD( pev->origin.z + 80 );
		WRITE_SHORT( g_sModelIndexLaser );
		WRITE_BYTE( 0 ); // frame start
		WRITE_BYTE( 10 ); // framerate
		WRITE_BYTE( 2 ); // life. WAS 3.
		WRITE_BYTE( 20 );  // width
		WRITE_BYTE( 0 );   // noise.  WAS 0
		WRITE_BYTE( 80 );   // r, g, b. Were all 255 before.
		WRITE_BYTE( 80 );   // r, g, b
		WRITE_BYTE( 255 );   // r, g, b
		WRITE_BYTE( 255 );	// brightness
		WRITE_BYTE( 10 );		// speed
	MESSAGE_END();
	*/


	
}


void CControllerHeadBall::BounceTouch( CBaseEntity *pOther )
{
	Vector vecDir = m_vecIdeal.Normalize( );

	TraceResult tr = UTIL_GetGlobalTrace( );

	float n = -DotProduct(tr.vecPlaneNormal, vecDir);

	vecDir = 2.0 * tr.vecPlaneNormal * n + vecDir;

	m_vecIdeal = vecDir * m_vecIdeal.Length();
}


float CControllerHeadBall::massInfluence(void){
	return 0.05f;
}//END OF massInfluence








