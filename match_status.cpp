#include "match_status.h"

// Should the server auto-switch the teams at the end of a period?
cvar_t  *match_autoswitch;
cvar_t	init_match_autoswitch = {"match_autoswitch", "1", FCVAR_EXTDLL, 1};

// Should the server force the teams to be a certain size before allow the match to start?
// If this is set to anything above 0, then each team must have the number of players specified before the match will start
cvar_t  *match_forceteamsize;
cvar_t	init_match_forceteamsize = {"match_forceteamsize", "5", FCVAR_EXTDLL, 5};

// Should the server auto-lo3 when everyone is ready?
// If this is set to 1, then the server will perform 3 sv_restart 1 commands and the match will be considered live 
// when all players are ready. This removes the need for a lo3 script, and is more reliable time-wise (because its 
// local)
cvar_t  *match_autolo3;
cvar_t	init_match_autolo3 = {"match_autolo3", "1", FCVAR_EXTDLL, 1};

// Should the server automatically assume overtime will start?
// If this is set to 1, then the server will automatically start overtime if a match ends in a draw
cvar_t  *match_autoovertime;
cvar_t	init_match_autoovertime = {"match_autoovertime", "1", FCVAR_EXTDLL, 1};

// What should all status messages be prefixed by?
// All status messages from this plugin are prefixed by whatever this CVAR is set to.
cvar_t  *match_agentname;
cvar_t	init_match_agentname =	{"match_agentname", "[Match Status]", FCVAR_EXTDLL};

// Should the teams be locked while a match is in a live state?
// If this is set then a player will not be able to switch teams while a match is in a live state.
cvar_t  *match_lockliveteams;
cvar_t	init_match_lockliveteams =	{"match_lockliveteams", "1", FCVAR_EXTDLL, 1};

const char *period_names[] = {"None", "First Half", "Second Half", "Over Time"};

const char *short_team_names[] = {"CT", "T"};
const char *long_team_names[] = {"Counter-Terrorists", "Terrorists"};

enum Period { PERIOD_NONE = 0, PERIOD_FIRST_HALF, PERIOD_SECOND_HALF, PERIOD_OVERTIME_1, PERIOD_OVERTIME_2 };

MatchStatus :: MatchStatus() {
	m_iActivePlayers = 0;

	Clear();
}

void MatchStatus :: Init(void) {
	CVAR_REGISTER (&init_match_autoswitch);
	match_autoswitch=CVAR_GET_POINTER("match_autoswitch");

	CVAR_REGISTER (&init_match_autolo3);
	match_autolo3=CVAR_GET_POINTER("match_autolo3");

	CVAR_REGISTER (&init_match_agentname);
	match_agentname=CVAR_GET_POINTER("match_agentname");

	CVAR_REGISTER (&init_match_forceteamsize);
	match_forceteamsize=CVAR_GET_POINTER("match_forceteamsize");

	CVAR_REGISTER (&init_match_lockliveteams);
	match_lockliveteams=CVAR_GET_POINTER("match_lockliveteams");

	CVAR_REGISTER (&init_match_autoovertime);
	match_autoovertime=CVAR_GET_POINTER("match_autoovertime");
}

void MatchStatus :: PostInit(void) {
	memset(players,0,sizeof(MatchPlayer)*33);
	Reset();
}

void MatchStatus :: Reset(void) {
	m_iActivePlayers = 0;

	Clear();
}

void MatchStatus :: ShowTeamMoney(int index) {
	char msg[50];
	for(int i = 1; i <= gpGlobals->maxClients; ++i) {
		MatchPlayer* pPlayer = GetPlayer(i);

		if((pPlayer->ingame) && (pPlayer->team_index == index)) {
			int money = *((int *)pPlayer->pEdict->pvPrivateData + OFFSET_CSMONEY);

			sprintf(msg, "%s: $%d", pPlayer->name, money);

			Alert(msg);
		}
	}
}

void MatchStatus :: Clear(void) {
	m_bIsLive = false;

	m_bIsInit = false;

	m_bIsOverTime = false;

	m_bScored = false;

	m_iPeriod = PERIOD_NONE;

	m_iOverTimeCounter = 0;

	m_iRoundCounter = 0;

	m_iRestartCount = -1;

	m_iScores[0] = 0;
	m_iScores[1] = 0;
	
	m_iPeriodScores[0] = 0;
	m_iPeriodScores[1] = 0;

	m_iTeamPlayersReady[1] = 0;
	m_iTeamPlayersReady[2] = 0;
	
	m_iReadyPlayers = 0;
}

void MatchStatus :: StartMatch(int num_rounds) {
	if(m_bIsInit) {
		LOG_CONSOLE(PLID, "ERROR: A game is already in progress, you must stop it first.\n");

		return;
	}

	m_bIsInit = true;

	char msg[70];
	sprintf(msg, "Match starting (MR%d)", num_rounds);

	Alert(msg);

	if(match_forceteamsize->value > 0) {
		sprintf(msg, "Each team must have %d players.", (int)match_forceteamsize->value);
		Alert(msg);
	}

	m_iHalfLength = num_rounds;
	m_iPeriod = PERIOD_FIRST_HALF;

	SERVER_COMMAND("exec addons/match/warmup.cfg\n");

	SERVER_COMMAND("sv_restart 1\n");
}

void MatchStatus :: OnTeamChange(edict_t *pEntity) {
	MatchPlayer* pPlayer = GetPlayerFromEntity(pEntity);

	if((pPlayer->active) && (IsLive()) && (match_lockliveteams->value > 0)) {
		Alert("You cannot change teams while a match is live.");

		RETURN_META(MRES_SUPERCEDE);
	}

	int team = atoi(CMD_ARGV(1));

	if(team != pPlayer->team_index) {
		// If this player is not marked as active then do so
		// This is done when a player first joins a team, and limits the pool of players the plugin considers
		// to those who actually join a team. This allows for spectating players who do not affect its operation.
		if(!pPlayer->active) {
			pPlayer->active = true;
			m_iActivePlayers++;
		}

		// If this player is marked as ready, then unready them cause they are changing team
		if(pPlayer->ready) {
			m_iTeamPlayersReady[pPlayer->team_index]--;
				
			m_iReadyPlayers--;

			pPlayer->ready = false;
		}

		pPlayer->team_index = team;
	}

	RETURN_META(MRES_IGNORED);
}

void MatchStatus :: Ready(edict_t *pEntity) {
	if(!IsLive() && m_bIsInit) {
		MatchPlayer* pPlayer = GetPlayerFromEntity(pEntity);

		if(!pPlayer->ready) {
			char message[50];

			pPlayer->ready = true;
			m_iReadyPlayers++;
			m_iTeamPlayersReady[pPlayer->team_index]++;

			sprintf(message, "%s is ready (%d/%d)", STRING(pEntity->v.netname), m_iReadyPlayers, m_iActivePlayers);

			Alert(message);

			if(m_iReadyPlayers == m_iActivePlayers) {
				if(match_forceteamsize->value > 0) {
					if((match_forceteamsize->value !=m_iTeamPlayersReady[1]) || 
						(match_forceteamsize->value != m_iTeamPlayersReady[2])) {
						sprintf(message, "%d players required on each team.", (int)match_forceteamsize->value);

						Alert(message);
					}
				}

				Alert("All players are ready.");

				NextPeriod();
			}

			RETURN_META(MRES_SUPERCEDE);
		}
	}
}


void MatchStatus :: Unready(edict_t *pEntity) {
	if(!IsLive() && m_bIsInit) {
		MatchPlayer* pPlayer = GetPlayerFromEntity(pEntity);

		if(pPlayer->ready) {
			pPlayer->ready = false;
			m_iReadyPlayers--;
			m_iTeamPlayersReady[pPlayer->team_index]--;

			char message[50];

			sprintf(message, "%s is not ready (%d/%d)", STRING(pEntity->v.netname), m_iReadyPlayers, m_iActivePlayers);

			Alert(message);

			RETURN_META(MRES_SUPERCEDE);
		}
	}
}

void MatchStatus :: RestartPeriod() {
	if((m_iPeriod == 0) || (!m_bIsLive)) {
		LOG_CONSOLE(PLID, "ERROR: There is not a game currently running.\n");

		return;
	}

	m_bIsLive = false;

	m_iScores[0] -= m_iPeriodScores[0];
	m_iScores[1] -= m_iPeriodScores[1];

	m_iPeriodScores[0] = 0;
	m_iPeriodScores[1] = 0;

	m_iRoundCounter = 0;

	char msg[100];

	sprintf(msg, "The %s is being restarted.", period_names[m_iPeriod]);
	Alert(msg);

	SayScore();

	if(match_autolo3->value == 1) {
		Lo3();
	}
}

void MatchStatus :: SayScore( void ) {
	char msg[100];

	sprintf(msg, "Current Score: (%s) %d - %d (%s)", Team1Name(), Team1Score(), Team2Score(), Team2Name());
	Alert(msg);
}

void MatchStatus :: Lo3( void ) {
	if(m_bIsLive) {
		LOG_CONSOLE(PLID, "ERROR: You cannot lo3 a match that is already live.\n");

		return;
	}

	if(m_iPeriod < PERIOD_OVERTIME_1) {
		SERVER_COMMAND("exec addons/match/match.cfg\n");
	}
	else {
		SERVER_COMMAND("exec addons/match/overtime.cfg\n");
	}

	m_iRestartCount = 3;

	m_bIsLive = true;
}

void MatchStatus :: CTScore(void) {
	int mod = ((m_iPeriod - 1) % 2);

	IncScore(mod);
}

void MatchStatus :: TScore(void) {
	int mod = 1 - ((m_iPeriod - 1) % 2);

	IncScore(mod);
}

void MatchStatus :: IncScore(int index) {
	m_iScores[index]++;
	m_iPeriodScores[index]++;

	m_bScored = true;

	m_iRoundCounter++;

	SayScore();

	if(m_iRoundCounter == m_iHalfLength) {
		char msg[50];

		sprintf(msg, "End of %s", period_names[m_iPeriod]);

		Alert(msg);

		if(m_iPeriod == PERIOD_FIRST_HALF) {
			if(match_autoswitch->value == 1) {
				SwitchTeams();
			}
			else {
				Alert("Please switch teams now!");
			}
		}
		else if(m_iPeriod == PERIOD_SECOND_HALF) {
			Alert("The match has finished in a draw.");

			if(match_autoovertime->value > 0) {
				Alert("Overtime is now being started...");
				SwitchTeams();
			}
			else {
				Alert("Overtime may be started if required.");
			}
		}

		EndPeriod();
	}
	else if((m_iPeriod == PERIOD_SECOND_HALF) && ((m_iScores[index] == (m_iHalfLength + 1)))) {
		char msg[100];

		sprintf(msg, "%s have won!", long_team_names[1 - ((m_iPeriod - 1) % 2)]);

		Alert(msg);
		Clear();
	}
}

bool MatchStatus :: ScoreUpdated(void) {
	if(m_bScored) {
		m_bScored = false;

		return true;
	}

	return false;
}

void MatchStatus :: NextPeriod(void) {
	if(m_bIsLive) {
		LOG_CONSOLE(PLID, "ERROR: The game is already live.\n");

		return;
	}

	char msg[50];

	sprintf(msg, "%s will be going live soon!", period_names[m_iPeriod]);

	Alert(msg);

	for(int i = 1; i <= gpGlobals->maxClients; ++i) {
		MatchPlayer* pPlayer = GetPlayer(i);
		
		pPlayer->ready = false;
	}

	m_iReadyPlayers = 0;

	if(match_autolo3->value == 1) {
		Lo3();
	}
}

void MatchStatus :: EndPeriod(void) {
	if(m_iPeriod == PERIOD_OVERTIME_2) {
		m_iPeriod--;
	}
	else {
		m_iPeriod++;
	}

	m_bIsLive = false;

	m_iPeriodScores[0] = 0;
	m_iPeriodScores[1] = 0;

	m_iRoundCounter = 0;

	SERVER_COMMAND("exec addons/match/warmup.cfg\n");
}

bool MatchStatus :: IsLive(void) {
	return m_bIsLive;
}

const char * MatchStatus :: Team1Name() {
	return short_team_names[(m_iPeriod - 1) % 2];
}

const char * MatchStatus :: Team2Name() {
	return short_team_names[1 - ((m_iPeriod - 1) % 2)];
}

int MatchStatus :: Team1Score() {
	return m_iScores[0];
}

int MatchStatus :: Team2Score() {
	return m_iScores[1];
}

void MatchStatus :: Alert(char *msg) {
	char alert[100];

	sprintf(alert, "say %s %s\n", match_agentname->string, msg);

	SERVER_COMMAND(alert);
}

MatchPlayer* MatchStatus :: GetPlayer(int index) {
	return &players[index];
}

MatchPlayer* MatchStatus :: GetPlayerFromEntity(edict_t* pEntity) {
	return &players[ENTINDEX(pEntity)];
}

void MatchStatus :: PrintStatus( void ) {
	if(!IsLive() && m_bIsInit) {
		char ready[768];
		char not_ready[768];

		char cur_name[35];

		bool readyf = 0, not_readyf = 0;

		int readypos = 0, not_readypos = 0;

		readypos = sprintf(ready, "Ready: ");
		not_readypos = sprintf(not_ready, "Not Ready: ");

		for(int i = 1; i <= gpGlobals->maxClients; ++i)  {
			MatchPlayer* pPlayer = GetPlayer(i);
					
			if(pPlayer->ingame) {
				if(pPlayer->ready) {
					if(readyf) {
						readypos += sprintf(ready + readypos, ", %s", STRING(pPlayer->pEdict->v.netname));
					}
					else {
						readypos += sprintf(ready + readypos, "%s", STRING(pPlayer->pEdict->v.netname));

						readyf = true;
					}
				}
				else {
					if(not_readyf) {
						not_readypos += sprintf(not_ready + not_readypos, ", %s", STRING(pPlayer->pEdict->v.netname));
					}
					else {
						not_readypos += sprintf(not_ready + not_readypos, "%s", STRING(pPlayer->pEdict->v.netname));

						not_readyf = true;
					}
				}
			}
		}

		Alert(ready);
		Alert(not_ready);
	}

	if(m_bIsInit) {
		SayScore();
	}
}

void MatchStatus :: SwitchTeams( void ) {
	for(int i = 1; i <= gpGlobals->maxClients; ++i) {
		MatchPlayer* pPlayer = GetPlayer(i);

		char msg[50];
		
		if(pPlayer->ingame) {
			if(pPlayer->team_index == 1) {
				CLIENT_COMMAND(pPlayer->pEdict, "jointeam 2\n");
			}
			else if(pPlayer->team_index == 2) {
				CLIENT_COMMAND(pPlayer->pEdict, "jointeam 1\n");
			}
		}
	}
}

void MatchStatus :: OnStartFrame ( void ) {
	if(m_iRestartCount > -1) {
		if(m_flNextRestart <= gpGlobals->time) {
			char msg[25];

			if(m_iRestartCount > 0) {
				SERVER_COMMAND("sv_restart 1\n");

				m_flNextRestart = gpGlobals->time + 1.0f;

				sprintf(msg, "%d...", m_iRestartCount);
				Alert(msg);
			}
			else {
				sprintf(msg, "%s is now LIVE!", period_names[m_iPeriod]);
				Alert(msg);

				SayScore();
			}

			m_iRestartCount--;
		}
	}

	if(m_bSwitchTeams) {
		SwitchTeams();

		m_bSwitchTeams = false;
	}
}

void MatchStatus :: StopMatch( void ) {
	if(m_bIsInit || IsLive()) {
		Alert("The current match has been terminated.");

		Clear();	
	}
}