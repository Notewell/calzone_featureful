/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "soundent.h"

#if !CLIENT_DLL
#include "gamerules.h"
#endif

LINK_ENTITY_TO_CLASS( weapon_m4, CM4 )
LINK_ENTITY_TO_CLASS( weapon_556Carbine, CM4 )

//=========================================================
//=========================================================

void CM4::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_m4" ); // hack to allow for old names
	Precache();
	SET_MODEL( ENT( pev ), MyWModel() );

	InitDefaultAmmo(M4_DEFAULT_GIVE);

	if( bIsMultiplayer() )
		m_iDefaultAmmo = M4_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
	m_iBurstState = BURST_IDLE;
}

void CM4::Precache( void )
{
	PRECACHE_MODEL( "models/v_m4.mdl" );
	PRECACHE_MODEL( MyWModel() );
	PrecachePModel( "models/p_9mmAR.mdl" );

	m_iShell = PRECACHE_MODEL( "models/shell.mdl" );// brass shellTE_MODEL

	PRECACHE_MODEL( "models/grenade.mdl" );	// grenade

	PRECACHE_MODEL( "models/w_m16clip.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "items/clipinsert1.wav" );
	PRECACHE_SOUND( "items/cliprelease1.wav" );

	PRECACHE_SOUND("weapons/colts1.wav");
	PRECACHE_SOUND("weapons/colts2.wav");
	PRECACHE_SOUND("weapons/colts3.wav");

	PRECACHE_SOUND( "weapons/glauncher.wav" );
	PRECACHE_SOUND( "weapons/glauncher2.wav" );

	m_usM4 = PRECACHE_EVENT( 1, "events/m4.sc" );
	m_usM42 = PRECACHE_EVENT( 1, "events/m42.sc" );
}

bool CM4::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "556";
	p->pszAmmo2 = "ARgrenades";
	p->iMaxClip = M4_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = WeaponId();
	p->iWeight = M4_WEIGHT;
	p->pszAmmoEntity = "ammo_9mmAR";
	p->iDropAmmo = AMMO_556MAG_GIVE;

	return true;
}

bool CM4::AddToPlayer( CBasePlayer *pPlayer )
{
	
	return AddToPlayerDefault(pPlayer);
}

bool CM4::Deploy()
{
	return DefaultDeploy( "models/v_m4.mdl", "models/p_9mmAR.mdl", MP5_DEPLOY, "mp5" );
}

void CM4::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == WL_Eyes)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	// optimized multiplayer. Widened to make it easier to hit a moving player
	const Vector vecSpread = bIsMultiplayer() ? VECTOR_CONE_6DEGREES : VECTOR_CONE_3DEGREES;
	Vector vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_556, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usM4, 0.0f, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", false, 0);

	if (m_iBurstState == BURST_IDLE)
	{
		m_iBurstState = BURST_TRIGGER;

	}
	else if (m_iBurstState >= BURST_NUM_SHOTS) //Burst is over, stop firing.
	{
		m_flNextPrimaryAttack = GetNextAttackDelay(M4_BETWEENBURST_DELAY);

		if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + M4_BETWEENBURST_DELAY; //Longer delay

		m_iBurstState = BURST_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); //Set idle time here only
		return; //Do not increment burst state
	}

	m_flNextPrimaryAttack = GetNextAttackDelay(M4_INBURST_DELAY);
	m_flTimeWeaponIdle = GetNextAttackDelay(M4_INBURST_DELAY);

	if (m_flNextPrimaryAttack < UTIL_WeaponTimeBase())
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + M4_INBURST_DELAY;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + M4_INBURST_DELAY;
	}


	m_iBurstState++;

	

}

void CM4::SecondaryAttack( void )
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == WL_Eyes )
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15f;
		return;
	}

	if( m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] == 0 )
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = UTIL_WeaponTimeBase() + 0.2f;

	m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType]--;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

 	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	// we don't add in player velocity anymore.
#if !CLIENT_DLL
	CGrenade::ShootContact( m_pPlayer->pev,
					m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16.0f,
					gpGlobals->v_forward * 800.0f );
#endif

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT( flags, m_pPlayer->edict(), m_usM42 );

	m_flNextPrimaryAttack = GetNextAttackDelay( 1.0f );
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0f;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0f;// idle pretty soon after shooting.

	if( !m_pPlayer->m_rgAmmo[m_iSecondaryAmmoType] )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", false, 0 );
}

void CM4::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == M4_MAX_CLIP)
		return;

	DefaultReload( M4_MAX_CLIP, MP5_RELOAD, 1.5f );
}

void CM4::WeaponIdle( void )
{

	if (m_iBurstState > BURST_IDLE) //Are we doing a burst?
	{
		if (UTIL_WeaponTimeBase() > m_flNextPrimaryAttack) //Has enough of a delay elapsed for the next shot?
		{
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + M4_INBURST_DELAY; //Make sure primaryattack doesn't get called again simultaneously via input.
			//m_iBurstState++;
			PrimaryAttack();
			
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + M4_INBURST_DELAY; //I can only say the same thing so many different ways, but I need the game to cooperate here
		}
		else
		{
			m_flTimeWeaponIdle = 0.1f; //Delay a tick and recheck.
		}

		return; //No idle when bursting.
	}
	


	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		iAnim = MP5_LONGIDLE;
		break;
	default:
	case 1:
		iAnim = MP5_IDLE1;
		break;
	}

	SendWeaponAnim(iAnim);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}
