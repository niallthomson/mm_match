#include <extdll.h>
#include "match_player.h"
#include "cstrike.h"

#ifndef MATCH_STATUS_H
#define MATCH_STATUS_H

class MatchStatus {
	public:
		MatchStatus();
		void StartMatch(int num_rounds);
		void Clear();
		void Reset();
		void RestartPeriod();
		void NextPeriod();
		void EndPeriod();

		void Init();

		void PostInit();

		void StopMatch();

		//void SwitchTeams();
		bool ScoreUpdated();

		// Increment the CT score
		void CTScore();

		// Increment the T Score
		void TScore();

		// Increment the specified teams scores
		void IncScore(int index);

		void OnTeamChange(edict_t *pEntity);

		// Perform a live-on-three to live a match
		void Lo3();

		void Ready(edict_t *pEntity);
		void Unready(edict_t *pEntity);

		// Set the specified players status to 'ready'
		void Ready(MatchPlayer *pPlayer);

		// Set the specified players status to 'not ready'
		void Unready(MatchPlayer *pPlayer);

		// Get Team 1's score
		int Team1Score();

		// Get Team 2's score
		int Team2Score();

		const char *Team1Name();
		const char *Team2Name();

		// Say the current scores so players can see them
		void SayScore();

		void ShowTeamMoney(int index);

		// Is the match currently live?
		bool IsLive();

		void Alert(char *msg);

		// Get a teams index based on its name
		int GetTeamIndex(char *name);

		// The number of active players in the game. Active players are those who have joined a team
		int m_iActivePlayers;
		int m_iReadyPlayers;

		void PrintStatus();

		void SwitchTeams();

		void OnStartFrame();

		// The time at which the next sv_restart 1 should be performed when lo3'ing
		float m_flNextRestart;

		// The number of restarts left to be performed
		int m_iRestartCount;

		// Flag to indicate that teams should be switched on the next StartFrame
		bool m_bSwitchTeams;

		// The current player period (see enum)
		int m_iPeriod;

		int m_iTeamPlayersReady[3];

		MatchPlayer* GetPlayer(int index);
		MatchPlayer* GetPlayerFromEntity(edict_t* pEntity);

	private:
		
		bool m_bIsLive;

		bool m_bIsInit;

		int m_iHalfLength;

		int m_iRoundCounter;

		bool m_bIsOverTime;
		bool m_bScored;

		int m_iOverTimeCounter;

		int m_iScores[2];
		int m_iPeriodScores[2];

		MatchPlayer players[33];
};

#endif