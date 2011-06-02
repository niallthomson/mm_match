#include "plugin.h"

/**

TODO (Prioritised):

- Add a say command for players to see whose ready and who isnt [DONE]
- Add in forceteamsize [DONE]
- Enhance score reporting to show which team has which score ie. (CT) 5 : 2 (T)
- Add a say command and a server command to print the status of the match: First Half - (CT) 5 : 2 (T)
- Add an option which allows players to pause the game if a player from both teams say 'pause'
- Add game events so that Remote Monitor plugin can pick up and send to Control Panel
- Add the ability to restore to a specific round, which means team scores and player money

**/

MatchStatus matchStatus;

void InitPlugin() {
	REG_SVR_COMMAND ("match_start", CmdStartMatch);
	REG_SVR_COMMAND ("match_stop", CmdStopMatch);
	REG_SVR_COMMAND ("match_live", CmdLo3);
	REG_SVR_COMMAND ("match_restartperiod", CmdRestartPeriod);
	REG_SVR_COMMAND ("match_forceready", CmdRestartPeriod);
	REG_SVR_COMMAND ("lo3", CmdLo3);

	REG_MESSAGE_HOOK("TextMsg", TextMsg);

	matchStatus.Init();

	SERVER_COMMAND("exec addons/match/options.cfg\n");
}

void ServerActivate_Post( edict_t *pEdictList, int edictCount, int clientMax ) {
	matchStatus.PostInit();
}

BOOL ClientConnect_Post( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[ 128 ]  ) {
	if ( META_RESULT_ORIG_RET(BOOL) ) {
		PlayerConnect(pEntity, pszName, pszAddress);
	}

	RETURN_META_VALUE(MRES_IGNORED, TRUE);
}

void ClientPutInServer( edict_t *pEntity ) {
	PlayerPutInServer( matchStatus.GetPlayerFromEntity(pEntity) );

	RETURN_META(MRES_IGNORED);
}

void ClientDisconnect( edict_t *pEntity ) {
	MatchPlayer* pPlayer = matchStatus.GetPlayerFromEntity(pEntity);

	if ( pPlayer->ingame ) {
		PlayerDisconnect( pPlayer );
	}
	
	RETURN_META(MRES_IGNORED);
}

void PlayerConnect(edict_t *pEntity, const char *pszName, const char *pszAddress) {
	MatchPlayer* pPlayer = matchStatus.GetPlayerFromEntity(pEntity);
	pPlayer->pEdict = pEntity;
	pPlayer->index = pPlayer->index;

	strncpy(pPlayer->name,pszName,32);
	pPlayer->name[31]=0;

	strcpy(pPlayer->team_name, "NOTEAM");
	pPlayer->team_index = 0;

	pPlayer->active = false;
	pPlayer->ready = false;
}

void PlayerPutInServer( MatchPlayer* pPlayer ) {
	pPlayer->ingame	= true;
}

void PlayerDisconnect( MatchPlayer *pPlayer) {
	pPlayer->ingame	= false;

	if(pPlayer->active) {
		matchStatus.m_iActivePlayers--;
		pPlayer->active = false;

		if(pPlayer->ready) {
			pPlayer->ready = false;
			matchStatus.m_iReadyPlayers--;
		}
	}
}

void StartFrame( void ) {
	matchStatus.OnStartFrame();

	RETURN_META(MRES_IGNORED);
}

void ClientCommand( edict_t *pEntity ) {
	const char* cmds = CMD_ARGV(0);
	const char* said = CMD_ARGV(1);
	
	if ( !strcmp(cmds, "say") ) {
		if(!strcmp(said, "unready") ) {
			matchStatus.Unready(pEntity);
		}
		else if(!strcmp(said, "ready") ) {
			matchStatus.Ready(pEntity);
		}
		else if(!strcmp(said, "status") ) {
			if(!matchStatus.IsLive()) {
				matchStatus.PrintStatus();
			}

			RETURN_META(MRES_SUPERCEDE);
		}
		/*else if(!strcmp(said, "money") ) {
			MatchPlayer* pPlayer = matchStatus.GetPlayerFromEntity(pEntity);
			matchStatus.ShowTeamMoney(pPlayer->team_index);

			RETURN_META(MRES_SUPERCEDE);
		}*/
	}
	else if ( !strcmp(cmds, "jointeam") ) {
		matchStatus.OnTeamChange(pEntity);
	}
	else {
		RETURN_META(MRES_IGNORED);
	}
}

void CmdStartMatch( void ) {
	int num_rounds;

	num_rounds = atoi(CMD_ARGV(1));

	if(num_rounds == 0) {
		num_rounds = 15;
	}

	matchStatus.StartMatch(num_rounds);	
}

void CmdStopMatch( void ) {
	matchStatus.StopMatch();
}

void CmdLo3( void ) {
	matchStatus.Lo3();
}

void CmdNextPeriod( void ) {
	matchStatus.NextPeriod();	
}

void CmdRestartPeriod( void ) {
	matchStatus.RestartPeriod();
}

void TextMsg( void* ) {
	int dest = GET_MESSAGE_BYTE(0);
	char *message = GET_MESSAGE_STRING(1);

	UTIL_LogPrintf("TextMsg\n");

	if(matchStatus.IsLive()) {
		UTIL_LogPrintf(".....Is Live\n");
		if(dest == 4) {
			UTIL_LogPrintf("Correct dest\n");

			if (!strncmp("#Target_B", message, 9)) {
				matchStatus.TScore();
			}
			else if (!strncmp("#Target_S", message, 9)) {
				matchStatus.CTScore();
			}
			else if(!strncmp("#Bomb_Defused", message, 13)) {
				matchStatus.CTScore();
			}
			else if(!strncmp("#CTs_W", message, 6)) {
				matchStatus.CTScore();
			}
			else if(!strncmp("#Terrorists_W", message, 13)) {
				matchStatus.TScore();
			}
		}
	}
}