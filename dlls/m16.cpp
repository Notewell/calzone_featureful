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

LINK_ENTITY_TO_CLASS( weapon_m16, CM16 )
LINK_ENTITY_TO_CLASS( weapon_556AR, CM16 )

//=========================================================
//=========================================================

void CM16::Spawn()
{
	pev->classname = MAKE_STRING( "weapon_m16" ); // hack to allow for old names
	Precache();
	SET_MODEL( ENT( pev ), MyWModel() );

	InitDefaultAmmo(M16_DEFAULT_GIVE);

	FallInit();// get ready to fall down.
}

void CM16::Precache( void )
{
	PRECACHE_MODEL( "models/v_m16.mdl" );
	PRECACHE_MODEL( MyWModel() );
	PrecachePModel( "models/p_9mmAR.mdl" );

	m_iShell = PRECACHE_MODEL( "models/saw_shell.mdl" );// brass shellTE_MODEL

	PRECACHE_MODEL( "models/grenade.mdl" );	// grenade

	PRECACHE_MODEL( "models/w_m16clip.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );

	PRECACHE_SOUND( "items/clipinsert1.wav" );
	PRECACHE_SOUND( "items/cliprelease1.wav" );

	PRECACHE_SOUND( "weapons/colts1.wav" );
	PRECACHE_SOUND( "weapons/colts2.wav" );
	PRECACHE_SOUND( "weapons/colts3.wav" );

	//PRECACHE_SOUND( "weapons/glauncher.wav" );
	//PRECACHE_SOUND( "weapons/glauncher2.wav" );

	m_usM16 = PRECACHE_EVENT( 1, "events/m16.sc" );
	//m_usM162 = PRECACHE_EVENT( 1, "events/m162.sc" ); //No Altfire on the M16A1
}

bool CM16::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "556";
	p->iMaxClip = M16_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 6;
	p->iFlags = 0;
	p->iId = WeaponId();
	p->iWeight = M16_WEIGHT;
	p->pszAmmoEntity = "ammo_556";
	p->iDropAmmo = AMMO_556MAG_GIVE;

	return true;
}

bool CM16::AddToPlayer( CBasePlayer *pPlayer )
{
	return AddToPlayerDefault(pPlayer);
}

bool CM16::Deploy()
{
	return DefaultDeploy( "models/v_m16.mdl", "models/p_9mmAR.mdl", MP5_DEPLOY, "mp5" );
}

void CM16::PrimaryAttack()
{
	// don't fire underwater
	if( m_pPlayer->pev->waterlevel == WL_Eyes )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	if( m_iClip <= 0 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.15f;
		return;
	}

	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_pPlayer->pev->punchangle.x = RANDOM_FLOAT(-1.0f, 1.0f);
	m_pPlayer->pev->punchangle.y = RANDOM_FLOAT(-1.0f, 1.0f);

	m_iClip--;

	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );


	const Vector vecSpread = VECTOR_CONE_3DEGREES;
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, vecSpread, 8192, BULLET_PLAYER_556, 1, 0, m_pPlayer->pev, m_pPlayer->random_seed );

	int flags;
#if CLIENT_WEAPONS
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif
	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usM16, 0.0f, g_vecZero, g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );

	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", false, 0 );

	m_flNextPrimaryAttack = GetNextAttackDelay( 0.1f );

	if( m_flNextPrimaryAttack < UTIL_WeaponTimeBase() )
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.1f;
		
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CM16::SecondaryAttack( void )
{
	return;
}

void CM16::Reload( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 || m_iClip == M16_MAX_CLIP )
		return;

	DefaultReload( M16_MAX_CLIP, MP5_RELOAD, 1.5f );
}

void CM16::WeaponIdle( void )
{
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	switch( RANDOM_LONG( 0, 1 ) )
	{
	case 0:	
		iAnim = MP5_LONGIDLE;	
		break;
	default:
	case 1:
		iAnim = MP5_IDLE1;
		break;
	}

	SendWeaponAnim( iAnim );

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 ); // how long till we do this again.
}
