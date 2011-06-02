#include <extdll.h>
#include <meta_api.h>

#ifndef MATCH_PLAYER_H
#define MATCH_PLAYER_H

class MatchPlayer 
{
	public:
		edict_t*	pEdict;
		char		name[32];
		int			index;
		bool		ingame;

		bool		active;

		char		team_name[32];
		int			team_index;

		bool		ready;

		MatchPlayer() { ready = false; };
};

#endif