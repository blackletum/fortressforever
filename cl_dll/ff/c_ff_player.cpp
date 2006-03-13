//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "input.h"
#include "c_ff_player.h"
#include "ff_weapon_base.h"
#include "c_basetempentity.h"
#include "c_ff_buildableobjects.h"
#include "ff_buildableobjects_shared.h"
#include "ff_utils.h"
#include "engine/IEngineSound.h"
#include "in_buttons.h"

#include "ff_hud_grenade1timer.h"
#include "ff_hud_grenade2timer.h"

#if defined( CFFPlayer )
	#undef CFFPlayer
#endif

extern CHudGrenade1Timer *g_pGrenade1Timer;
extern CHudGrenade2Timer *g_pGrenade2Timer;

#include "c_gib.h"

#include "c_ff_timers.h"
#include "vguicenterprint.h"

// --> Mirv: Decaptiation code
// These need to be mirrored in ff_player.cpp
#define DECAP_HEAD			( 1 << 0 )
#define DECAP_LEFT_ARM		( 1 << 1 )
#define DECAP_RIGHT_ARM		( 1 << 2 )
#define DECAP_LEFT_LEG		( 1 << 3 )
#define DECAP_RIGHT_LEG		( 1 << 4 )
// <-- Mirv: Decaptiation code

// --> Mirv: Conc stuff
static ConVar horiz_speed( "ffdev_concuss_hspeed", "2.0", 0, "Horizontal speed" );
static ConVar horiz_mag( "ffdev_concuss_hmag", "2.0", 0, "Horizontal magnitude" );
static ConVar vert_speed( "ffdev_concuss_vspeed", "1.0", 0, "Vertical speed" );
static ConVar vert_mag( "ffdev_concuss_vmag", "2.0", 0, "Vertical magnitude" );
static ConVar conc_test( "ffdev_concuss_test", "0", 0, "Show conced decals" );
// <-- Mirv: Conc stuff

// #0000331: impulse 81 not working (weapon_cubemap)
#include "../c_weapon__stubs.h"
#include "ff_weapon_base.h"

STUB_WEAPON_CLASS( weapon_cubemap, WeaponCubemap, C_BaseCombatWeapon );

void OnTimerExpired(C_FFTimer *pTimer)
{
	string name = pTimer->GetTimerName();
	DevMsg("OnTimerExpired(%s)\n",name.c_str());
	char buf[256];
	sprintf(buf,"OnTimerExpired(%s)\n",name.c_str());
	internalCenterPrint->SetTextColor( 255, 255, 255, 255 );
	internalCenterPrint->Print( buf );
}

// --> Mirv: Toggle grenades (requested by defrag)
void CC_ToggleOne( void )
{
	if(!engine->IsConnected() || !engine->IsInGame())
		return;

	C_FFPlayer *pLocalPlayer = C_FFPlayer::GetLocalFFPlayer();

	if( pLocalPlayer->m_iGrenadeState == FF_GREN_PRIMEONE || pLocalPlayer->m_iGrenadeState == FF_GREN_PRIMETWO )
		CC_ThrowGren();
	else
		CC_PrimeOne();
}

void CC_ToggleTwo( void )
{
	if(!engine->IsConnected() || !engine->IsInGame())
		return;

	C_FFPlayer *pLocalPlayer = C_FFPlayer::GetLocalFFPlayer();

	if( pLocalPlayer->m_iGrenadeState == FF_GREN_PRIMEONE || pLocalPlayer->m_iGrenadeState == FF_GREN_PRIMETWO )
		CC_ThrowGren();
	else
		CC_PrimeTwo();
}
// <-- Mirv: Toggle grenades (requested by defrag)

void CC_PrimeOne( void )
{
	if(!engine->IsConnected() || !engine->IsInGame())
		return;

	C_FFPlayer *pLocalPlayer = C_FFPlayer::GetLocalFFPlayer();

	if( pLocalPlayer->m_bClientBuilding )
	{
		DevMsg( "[Client] Building - not priming a gren!\n" );
		return;
	}

	// Bug #0000176: Sniper gren2 shouldn't trigger timer.wav
	// Bug #0000064: Civilian has primary & secondary grenade.
	if(pLocalPlayer->m_iPrimary <= 0)
	{
		DevMsg("[Grenades] You are out of primary grenades!\n");
		return;
	}

	// Bug #0000169: Grenade timer is played when player is dead and primes a grenade
	if (!pLocalPlayer->IsAlive() || pLocalPlayer->GetTeamNumber() < TEAM_BLUE)
		return;

	// Bug #0000170: Grenade timer plays on gren2 command if the player is already priming a gren1
	if (pLocalPlayer->m_iGrenadeState != FF_GREN_NONE)
		return;

	// Bug #0000366: Spy's cloaking & grenade quirks
	// Spy shouldn't be able to prime grenades when feigned
	if (pLocalPlayer->GetEffects() & EF_NODRAW)
		return;

	pLocalPlayer->m_flPrimeTime = engine->Time();

	C_FFTimer *pTimer = g_FFTimers.Create("PrimeGren", 4.0f);
	if (pTimer)
	{
		pTimer->m_bRemoveWhenExpired = true;
		pTimer->StartTimer();				
	}

	pLocalPlayer->EmitSound( "Grenade.Timer" );

	Assert (g_pGrenade1Timer);
	g_pGrenade1Timer->SetTimer(4.0f);
}

void CC_PrimeTwo( void )
{
	if(!engine->IsConnected() || !engine->IsInGame())
		return;

	C_FFPlayer *pLocalPlayer = C_FFPlayer::GetLocalFFPlayer();

	if( pLocalPlayer->m_bClientBuilding )
	{
		DevMsg( "[Client] Building - not priming a gren!\n" );
		return;
	}

	// Bug #0000176: Sniper gren2 shouldn't trigger timer.wav
	// Bug #0000064: Civilian has primary & secondary grenade.
	if(pLocalPlayer->m_iSecondary <= 0)
	{
		DevMsg("[Grenades] You are out of secondary grenades!\n");
		return;
	}

	// Bug #0000169: Grenade timer is played when player is dead and primes a grenade
	if (!pLocalPlayer->IsAlive() || pLocalPlayer->GetTeamNumber() < TEAM_BLUE)
		return;

	// Bug #0000170: Grenade timer plays on gren2 command if the player is already priming a gren1
	if (pLocalPlayer->m_iGrenadeState != FF_GREN_NONE)
		return;

	// Bug #0000366: Spy's cloaking & grenade quirks
	// Spy shouldn't be able to prime grenades when feigned
	if (pLocalPlayer->GetEffects() & EF_NODRAW)
		return;

	pLocalPlayer->m_flPrimeTime = engine->Time();
	
	C_FFTimer *pTimer = g_FFTimers.Create("PrimeGren", 4.0f);
	if (pTimer)
	{
		pTimer->m_bRemoveWhenExpired = true;
		pTimer->StartTimer();				
	}
	pLocalPlayer->EmitSound( "Grenade.Timer" );

	Assert (g_pGrenade2Timer);
	g_pGrenade2Timer->SetTimer(4.0f);
}
void CC_ThrowGren( void )
{
	if(!engine->IsConnected() || !engine->IsInGame())
		return;

	C_FFPlayer *pLocalPlayer = C_FFPlayer::GetLocalFFPlayer();
	if(
		((pLocalPlayer->m_iGrenadeState == FF_GREN_PRIMEONE) && (pLocalPlayer->m_iPrimary == 0))
		||
		((pLocalPlayer->m_iGrenadeState == FF_GREN_PRIMETWO) && (pLocalPlayer->m_iSecondary == 0))
		)
	{
		return;
	}
}
void CC_TestTimers( void )
{
	DevMsg("[L0ki] CC_TestTimers\n");
	if(!engine->IsConnected() || !engine->IsInGame())
	{
		DevMsg("[L0ki] \tNOT connected or NOT active!\n");
		return;
	}

	C_FFTimer *pTimer = g_FFTimers.Create("ClientTimer0", 3.0f);
	if(pTimer)
	{
		pTimer->SetExpiredCallback(OnTimerExpired, true);
		pTimer->StartTimer();
	}
}

ConCommand testtimers("cc_test_timers",CC_TestTimers,"Tests the basic timer classes.");

#define FF_PLAYER_MODEL "models/player/terror.mdl"

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEPlayerAnimEvent, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate( DataUpdateType_t updateType )
	{
		// Create the effect.
		C_FFPlayer *pPlayer = dynamic_cast< C_FFPlayer* >( m_hPlayer.Get() );
		if ( pPlayer && !pPlayer->IsDormant() )
		{
			pPlayer->DoAnimationEvent( (PlayerAnimEvent_t)m_iEvent.Get() );
		}	
	}

public:
	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
};

IMPLEMENT_CLIENTCLASS_EVENT( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent );

BEGIN_RECV_TABLE_NOBASE( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropInt( RECVINFO( m_iEvent ) )
END_RECV_TABLE()

// Prototype
void RecvProxy_PrimeTime( const CRecvProxyData *pData, void *pStruct, void *pOut );

BEGIN_RECV_TABLE_NOBASE( C_FFPlayer, DT_FFLocalPlayerExclusive )
	RecvPropInt( RECVINFO( m_iShotsFired ) ),

	RecvPropInt( RECVINFO( m_iClassStatus ) ),	// |-- Mirv: Class status

	// Beg: Added by Mulchman for building objects and such
	RecvPropEHandle( RECVINFO( m_hDispenser ) ),
	RecvPropEHandle( RECVINFO( m_hSentryGun ) ),
	RecvPropEHandle( RECVINFO( m_hDetpack ) ),
	RecvPropInt( RECVINFO( m_bBuilding ) ),
	RecvPropInt( RECVINFO( m_iCurBuild ) ),
	RecvPropInt( RECVINFO( m_bCancelledBuild ) ),
	// End: Added by Mulchman for building objects and such

	// ---> added by billdoor
	RecvPropInt(RECVINFO(m_iMaxHealth)),
	RecvPropInt(RECVINFO(m_iArmor)),
	RecvPropInt(RECVINFO(m_iMaxArmor)),
	RecvPropFloat(RECVINFO(m_fArmorType)),

	RecvPropInt(RECVINFO(m_iSkiState)),
	// ---> end

	// Beg: Added by L0ki - Grenade related
	RecvPropInt( RECVINFO( m_iPrimary ) ),
	RecvPropInt( RECVINFO( m_iSecondary ) ),
	RecvPropInt( RECVINFO( m_iGrenadeState ) ),
	RecvPropFloat( RECVINFO( m_flServerPrimeTime ), 0, RecvProxy_PrimeTime ),
	// End: Added by L0ki

	// Beg: Added by FryGuy - Status Effects related
	RecvPropFloat( RECVINFO( m_flNextBurnTick ) ),
	RecvPropInt( RECVINFO( m_iBurnTicks ) ),
	RecvPropFloat( RECVINFO( m_flBurningDamage ) ),
	// End: Added by FryGuy

	// --> Mirv: Map guide
	RecvPropEHandle( RECVINFO( m_hNextMapGuide ) ),
	RecvPropEHandle( RECVINFO( m_hLastMapGuide ) ),
	RecvPropFloat( RECVINFO( m_flNextMapGuideTime ) ),
	// <-- Mirv: Map guide

	RecvPropFloat( RECVINFO( m_flConcTime ) ),		// |-- Mirv: Concussed

	RecvPropFloat(RECVINFO(m_flMassCoefficient)),
END_RECV_TABLE( )

void RecvProxy_PrimeTime( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	DevMsg("[Grenades] RecvProxy_PrimeTime\n");
	// Unpack the data.
	if(!engine->IsConnected() || !engine->IsInGame())
	{
		DevMsg("[Grenades] \t NOT connected or NOT active!\n");
		return;
	}
	C_FFPlayer *pLocalPlayer = C_FFPlayer::GetLocalFFPlayer();
	if(pLocalPlayer)
	{
		pLocalPlayer->m_flServerPrimeTime = pData->m_Value.m_Float;
		if(pLocalPlayer->m_flServerPrimeTime != 0.0f)
			pLocalPlayer->m_flLatency = engine->Time() - pLocalPlayer->m_flPrimeTime;
		DevMsg("[Grenades] \tm_flServerPrimeTime: %f\n", pLocalPlayer->m_flServerPrimeTime);
		DevMsg("[Grenades] \tm_flLatency: %f\n", pLocalPlayer->m_flLatency);
	}
}

IMPLEMENT_CLIENTCLASS_DT( C_FFPlayer, DT_FFPlayer, CFFPlayer )
	RecvPropDataTable( "fflocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_FFLocalPlayerExclusive) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),
	RecvPropEHandle( RECVINFO( m_hRagdoll ) ),
END_RECV_TABLE( )

class C_FFRagdoll : public C_BaseAnimatingOverlay
{
public:
	DECLARE_CLASS( C_FFRagdoll, C_BaseAnimatingOverlay );
	DECLARE_CLIENTCLASS();

	C_FFRagdoll();
	~C_FFRagdoll();

	virtual void OnDataChanged( DataUpdateType_t type );

	// HACKHACK: Fix to allow laser beam to shine off ragdolls
	virtual CollideType_t ShouldCollide()
	{
		return ENTITY_SHOULD_COLLIDE_RESPOND;
	}

	int GetPlayerEntIndex() const;
	IRagdoll* GetIRagdoll() const;

	void ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName );

private:

	C_FFRagdoll( const C_FFRagdoll & ) {}

	void Interp_Copy( VarMapping_t *pDest, CBaseEntity *pSourceEntity, VarMapping_t *pSrc );

	void CreateRagdoll();


private:

	EHANDLE	m_hPlayer;
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );

	// --> Mirv: State of player's limbs
	CNetworkVar( int, m_fBodygroupState );
	// <-- Mirv: State of player's limbs
};


IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_FFRagdoll, DT_FFRagdoll, CFFRagdoll )
	RecvPropVector( RECVINFO(m_vecRagdollOrigin) ),
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropInt( RECVINFO( m_nModelIndex ) ),
	RecvPropInt( RECVINFO(m_nForceBone) ),
	RecvPropVector( RECVINFO(m_vecForce) ),
	RecvPropVector( RECVINFO( m_vecRagdollVelocity ) ),
	// --> Mirv: State of player's limbs
	RecvPropInt( RECVINFO(m_fBodygroupState) )
	// <-- Mirv: State of player's limbs
END_RECV_TABLE()


C_FFRagdoll::C_FFRagdoll()
{
}

C_FFRagdoll::~C_FFRagdoll()
{
	PhysCleanupFrictionSounds( this );
}

void C_FFRagdoll::Interp_Copy( VarMapping_t *pDest, CBaseEntity *pSourceEntity, VarMapping_t *pSrc )
{
	if ( !pDest || !pSrc )
		return;

	if ( pDest->m_Entries.Count() != pSrc->m_Entries.Count() )
	{
		Assert( false );
		return;
	}

	int c = pDest->m_Entries.Count();
	for ( int i = 0; i < c; i++ )
	{
		pDest->m_Entries[ i ].watcher->Copy( pSrc->m_Entries[i].watcher );
	}

	Interp_Copy( pDest->m_pBaseClassVarMapping, pSourceEntity, pSrc->m_pBaseClassVarMapping );
}

void C_FFRagdoll::ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if( !pPhysicsObject )
		return;

	// --> Mirv: [TODO] Return on impact to invisible bodygroup
	// This will need to wait until the #s of the hitgroups are finalised
	// <-- Mirv: [TODO] Return on impact to invisible bodygroup

	Vector dir = pTrace->endpos - pTrace->startpos;

	if ( iDamageType == DMG_BLAST )
	{
		dir *= 4000;  // adjust impact strenght

		// apply force at object mass center
		pPhysicsObject->ApplyForceCenter( dir );
	}
	else
	{
		Vector hitpos;  

		VectorMA( pTrace->startpos, pTrace->fraction, dir, hitpos );
		VectorNormalize( dir );

		dir *= 4000;  // adjust impact strenght

		// apply force where we hit it
		pPhysicsObject->ApplyForceOffset( dir, hitpos );	
	}
}


void C_FFRagdoll::CreateRagdoll()
{
	// First, initialize all our data. If we have the player's entity on our client,
	// then we can make ourselves start out exactly where the player is.
	C_FFPlayer *pPlayer = dynamic_cast< C_FFPlayer* >( m_hPlayer.Get() );

	if ( pPlayer && !pPlayer->IsDormant() )
	{
		// move my current model instance to the ragdoll's so decals are preserved.
		pPlayer->SnatchModelInstance( this );

		VarMapping_t *varMap = GetVarMapping();

		// Copy all the interpolated vars from the player entity.
		// The entity uses the interpolated history to get bone velocity.
		bool bRemotePlayer = (pPlayer != C_BasePlayer::GetLocalPlayer());			
		if ( bRemotePlayer )
		{
			Interp_Copy( varMap, pPlayer, pPlayer->C_BaseAnimatingOverlay::GetVarMapping() );

			SetAbsAngles( pPlayer->GetRenderAngles() );
			GetRotationInterpolator().Reset();

			m_flAnimTime = pPlayer->m_flAnimTime;
			SetSequence( pPlayer->GetSequence() );
			m_flPlaybackRate = pPlayer->GetPlaybackRate();
		}
		else
		{
			// This is the local player, so set them in a default
			// pose and slam their velocity, angles and origin
			SetAbsOrigin( m_vecRagdollOrigin );

			SetAbsAngles( pPlayer->GetRenderAngles() );

			SetAbsVelocity( m_vecRagdollVelocity );

			int iSeq = LookupSequence( "walk_lower" );
			if ( iSeq == -1 )
			{
				// Mulch: to start knowing what asserts are popping up for when testing stuff
				AssertMsg( false, "missing sequence walk_lower" ); 
				//Assert( false );	// missing walk_lower?
				iSeq = 0;
			}

			SetSequence( iSeq );	// walk_lower, basic pose
			SetCycle( 0.0 );

			Interp_Reset( varMap );
		}		
	}
	else
	{
		// overwrite network origin so later interpolation will
		// use this position
		SetNetworkOrigin( m_vecRagdollOrigin );

		SetAbsOrigin( m_vecRagdollOrigin );
		SetAbsVelocity( m_vecRagdollVelocity );

		Interp_Reset( GetVarMapping() );

	}

	SetModelIndex( m_nModelIndex );



	// --> Mirv: Spawn ragdoll with correct limbs missing
	DevMsg( "[CLIENT] Spawned ragdoll, received m_fBodygroupState as %d\n", m_fBodygroupState );

	if( m_fBodygroupState & DECAP_HEAD )
		SetBodygroup( 1, 1 );
	if( m_fBodygroupState & DECAP_LEFT_ARM )
		SetBodygroup( 2, 1 );
	if( m_fBodygroupState & DECAP_LEFT_LEG )
		SetBodygroup( 3, 1 );
	if( m_fBodygroupState & DECAP_RIGHT_ARM )
		SetBodygroup( 4, 1 );
	if( m_fBodygroupState & DECAP_RIGHT_LEG )
		SetBodygroup( 5, 1 );
	// <-- Mirv: Spawn ragdoll with correct limbs missing

	// Turn it into a ragdoll.
	// Make us a ragdoll..
	m_nRenderFX = kRenderFxRagdoll;

	BecomeRagdollOnClient( false );

	// HACKHACK: Fix to allow laser beam to shine off ragdolls
	SetSolid(SOLID_BBOX);
	AddSolidFlags(FSOLID_NOT_STANDABLE);
	SetCollisionGroup(COLLISION_GROUP_WEAPON);
}


void C_FFRagdoll::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		CreateRagdoll();
	
		IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

		if( pPhysicsObject )
		{
			AngularImpulse aVelocity(0,0,0);

			Vector vecExaggeratedVelocity = 3 * m_vecRagdollVelocity;

			pPhysicsObject->AddVelocity( &vecExaggeratedVelocity, &aVelocity );
		}
	}
}

IRagdoll* C_FFRagdoll::GetIRagdoll() const
{
	return m_pRagdoll;
}

C_BaseAnimating * C_FFPlayer::BecomeRagdollOnClient( bool bCopyEntity )
{
	// Let the C_CSRagdoll entity do this.
	// m_builtRagdoll = true;
	return NULL;
}


IRagdoll* C_FFPlayer::GetRepresentativeRagdoll() const
{
	if ( m_hRagdoll.Get() )
	{
		C_FFRagdoll *pRagdoll = (C_FFRagdoll*)m_hRagdoll.Get();

		return pRagdoll->GetIRagdoll();
	}
	else
	{
		return NULL;
	}
}



C_FFPlayer::C_FFPlayer() : 
	m_iv_angEyeAngles( "C_FFPlayer::m_iv_angEyeAngles" )
{
	m_PlayerAnimState = CreatePlayerAnimState( this, this, LEGANIM_9WAY, true );

	m_angEyeAngles.Init();
	AddVar( &m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR );

	m_iLocalSkiState = 0;

	// BEG: Added by Mulchman
	m_bClientBuilding = false;
	// END: Added by Mulchman
	
	m_flConcTime = m_flConcTimeStart = 0;	// |-- Mirv: Don't start conced
}

C_FFPlayer::~C_FFPlayer()
{
	m_PlayerAnimState->Release();
}

C_FFPlayer* C_FFPlayer::GetLocalFFPlayer()
{
	return ToFFPlayer( C_BasePlayer::GetLocalPlayer() );
}

// --> Mirv: Conc angles
void C_FFPlayer::PreThink( void )
{
	if( m_flConcTime > gpGlobals->curtime )
	{
		// When did this start
		if( m_flConcTimeStart == 0 )
			m_flConcTimeStart = gpGlobals->curtime;

		// Work our conc amount with this rather slow formula for now
		float flConcAmount = 10.0f * ( m_flConcTime - gpGlobals->curtime ) / ( m_flConcTime - m_flConcTimeStart );
			
		// Our conc angles, this is also quite slow for now
		m_angConced = QAngle( flConcAmount * vert_mag.GetFloat() * sin(vert_speed.GetFloat() * gpGlobals->curtime), flConcAmount * horiz_mag.GetFloat() * sin(horiz_speed.GetFloat() * gpGlobals->curtime), 0 );
	}
	else
		m_flConcTimeStart = 0;

	// Do we need to do a class specific skill?
	if (m_afButtonPressed & IN_ATTACK2)
		ClassSpecificSkill();

	else if (m_afButtonReleased & IN_ATTACK2)
		ClassSpecificSkill_Post();

	BaseClass::PreThink();
}

// Stomp any movement if we're in mapguide mode
void C_FFPlayer::CreateMove(float flInputSampleTime, CUserCmd *pCmd)
{
	// Mapguides
	if (GetTeamNumber() == TEAM_SPECTATOR && m_hNextMapGuide)
	{
		pCmd->buttons = 0;
		pCmd->forwardmove = 0;
		pCmd->sidemove = 0;
		pCmd->upmove = 0;
	}
}

// Handy function to get the midpoint angle between two angles
float MidAngle(float target, float value, float amount) 
{
	target = anglemod(target);
	value = anglemod(value);

	float delta = target - value;

	if (delta < -180) 
		delta += 360;
	else if (delta > 180) 
		delta -= 360;

	delta *= amount;

	value += delta;

	if (value < -180)
		value += 360;
	else if (value > 360)
		value -= 360;

	return value;
}

const QAngle &C_FFPlayer::EyeAngles()
{
	// Mapguides
	if (GetTeamNumber() < TEAM_BLUE && m_hNextMapGuide)
	{
		float t = clamp((m_flNextMapGuideTime - gpGlobals->curtime) / m_hLastMapGuide->m_flTime, 0, 1.0f);

		static QAngle angDirection;

		// Dealing with 1-t here really, so swap Next/Last
		angDirection.x = MidAngle(m_hLastMapGuide->GetAbsAngles().x, m_hNextMapGuide->GetAbsAngles().x, t);
		angDirection.y = MidAngle(m_hLastMapGuide->GetAbsAngles().y, m_hNextMapGuide->GetAbsAngles().y, t);
		angDirection.z = MidAngle(m_hLastMapGuide->GetAbsAngles().z, m_hNextMapGuide->GetAbsAngles().z, t);

		return angDirection;
	}

	// Concussion
	if( m_flConcTime > gpGlobals->curtime && conc_test.GetInt() != 0 )
	{
		m_angConcedTest = BaseClass::EyeAngles() + m_angConced;
		return m_angConcedTest;
	}
	else
		return BaseClass::EyeAngles();
}

void C_FFPlayer::CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov )
{
	BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );

	if( m_flConcTime > gpGlobals->curtime && conc_test.GetInt() == 0 )
		eyeAngles += m_angConced;
}

void C_FFPlayer::CalcViewModelView( const Vector& eyeOrigin, const QAngle& eyeAngles)
{
	if( m_flConcTime > gpGlobals->curtime )
		BaseClass::CalcViewModelView( eyeOrigin, eyeAngles - m_angConced );
	else
		BaseClass::CalcViewModelView( eyeOrigin, eyeAngles );
}
// <-- Mirv: Conc angles

const QAngle& C_FFPlayer::GetRenderAngles()
{
	if ( IsRagdoll() )
	{
		return vec3_angle;
	}
	else
	{
		return m_PlayerAnimState->GetRenderAngles();
	}
}


void C_FFPlayer::UpdateClientSideAnimation()
{
	// Update the animation data. It does the local check here so this works when using
	// a third-person camera (and we don't have valid player angles).
	if ( this == C_FFPlayer::GetLocalFFPlayer() )
		m_PlayerAnimState->Update( EyeAngles()[YAW], EyeAngles()[PITCH] );	// |-- Mirv: 3rd person viewmodel fix
	else
		m_PlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );

	BaseClass::UpdateClientSideAnimation();
}


void C_FFPlayer::PostDataUpdate( DataUpdateType_t updateType )
{
	// C_BaseEntity assumes we're networking the entity's angles, so pretend that it
	// networked the same value we already have.
	SetNetworkAngles( GetLocalAngles() );
	
	BaseClass::PostDataUpdate( updateType );
}

void C_FFPlayer::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	// BEG: Added by Mulchman
	if( m_bBuilding && !m_bClientBuilding )
	{
		// We started building
		DevMsg( "Started building a... " );

		bool bDrawTimer = true;
		string szTimerName = FF_BUILDABLE_TIMER_BUILD_STRING;
		float flTimerDuration = 2.0f;

		if( m_iCurBuild == FF_BUILD_DISPENSER )
		{
			DevMsg( "Dispenser!\n" );
			flTimerDuration = 2.0f;
		}
		else if( m_iCurBuild == FF_BUILD_SENTRYGUN )
		{
			DevMsg( "SentryGun!\n" );
			flTimerDuration = 5.0f;
		}
		else if( m_iCurBuild == FF_BUILD_DETPACK )
		{
			DevMsg( "Detpack!\n" );	
			flTimerDuration = 4.0f;
		}
		else
		{
			DevMsg( "ERROR!? WTFBBQ!?\n" );
			bDrawTimer = false;
		}

		if( bDrawTimer )
		{
			C_FFTimer *pTimer = g_FFTimers.Create( szTimerName, flTimerDuration );
			if( pTimer )
			{
				pTimer->m_bRemoveWhenExpired = true;
				pTimer->StartTimer();				
			}
		}
	}
	else if( !m_bBuilding && m_bClientBuilding )
	{
		// We stopped building
		DevMsg( "Stopped building (or cancelled building)\n" );

		// If the timer still exists and we've stopped building (or cancelled)
		// then kill it off manually
		C_FFTimer *pTimer = g_FFTimers.FindTimer( FF_BUILDABLE_TIMER_BUILD_STRING );

		if( pTimer )
			g_FFTimers.DeleteTimer( pTimer );
/*	

		// Code to get mins/maxs of a model - leave in please
		/*		
		//C_FFDispenser *pObject = ( C_FFDispenser * )m_hDispenser.Get( );
		//C_FFSentryGun *pObject = ( C_FFSentryGun * )m_hSentryGun.Get( );
		C_FFDetpack *pObject = ( C_FFDetpack * )m_hDetpack.Get( );
		if( pObject )
		{
			Vector mins, maxs;

			pObject->GetRenderBounds( mins, maxs );
			DevMsg( "Mins: %f, %f, %f\nMaxs: %f, %f, %f\n", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z );
		}
		//*/
	}

	// Update client buildling state to that of
	// the server
	m_bClientBuilding = m_bBuilding;
	// END: Added by Mulchman

	UpdateVisibility();
}

void C_FFPlayer::Simulate()
{
	BaseClass::Simulate();

	g_FFTimers.SimulateTimers();
}

void C_FFPlayer::DoAnimationEvent( PlayerAnimEvent_t event )
{
	m_PlayerAnimState->DoAnimationEvent( event );
}

bool C_FFPlayer::ShouldDraw( void )
{
	// If we're dead, our ragdoll will be drawn for us instead.
	if ( !IsAlive() )
		return false;

	if( GetTeamNumber() == TEAM_SPECTATOR )
		return false;

	if( IsLocalPlayer() && IsRagdoll() )
		return true;

	return BaseClass::ShouldDraw();
}

// --> Mirv: Get the class
int C_FFPlayer::GetClassSlot( void )
{
	return ( m_iClassStatus & 0x0000000F );
}
// <-- Mirv: Get the class

CFFWeaponBase* C_FFPlayer::GetActiveFFWeapon() const
{
	return dynamic_cast< CFFWeaponBase* >( GetActiveWeapon() );
}

void C_FFPlayer::SwapToWeapon(FFWeaponID weaponid)
{
	if (GetActiveFFWeapon() && GetActiveFFWeapon()->GetWeaponID() == weaponid)
		return;

	CFFWeaponBase *weap;

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		weap = dynamic_cast<CFFWeaponBase *>(GetWeapon(i));
		if (weap && weap->GetWeaponID() == weaponid)
			::input->MakeWeaponSelection(weap);
	}	
}