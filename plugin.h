#include <extdll.h>
#include <meta_api.h>
#include <cbase.h>
#include <player.h>
#include "messages.h"
#include "match_player.h"
#include "match_status.h"

#ifndef MATCH_META_PLUGIN_H
#define MATCH_META_PLUGIN_H

void InitPlugin();

void ClientCommand( edict_t *pEntity );
void ServerActivate_Post( edict_t *pEdictList, int edictCount, int clientMax );
void ClientDisconnect( edict_t *pEntity );
void ClientPutInServer( edict_t *pEntity );
BOOL ClientConnect_Post( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ] );

void CmdStartMatch( void );
void CmdLo3( void );
void CmdStopMatch( void );
void CmdNextPeriod( void );
void CmdRestartPeriod( void );

void SwitchTeams( void );

void StartFrame( void );

void PlayerConnect(edict_t *pEntity, const char *pszName, const char *pszAddress);
void PlayerPutInServer( MatchPlayer* pPlayer );
void PlayerDisconnect( MatchPlayer* pPlayer );

void TeamScore( void* );
void TextMsg( void* );

#endif