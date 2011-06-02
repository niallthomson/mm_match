#ifndef DEV_META_MESSAGES_H
#define DEV_META_MESSAGES_H

enum { MESSAGE_TYPE_BYTE = 0, MESSAGE_TYPE_STRING, MESSAGE_TYPE_SHORT, MESSAGE_TYPE_COORD};

#define SERVER_COMMAND	(*g_engfuncs.pfnServerCommand)

void REG_MESSAGE_HOOK(char *msg, void (*function)(void*));
void BEGIN_MESSAGE_HOOK(int msg_dest, int msg_type, edict_t *pEntity);
void CALL_MESSAGE_HOOK();

void CLEAR_VAR(int index);

void PARSE_MESSAGE_SHORT(int iValue);
void PARSE_MESSAGE_BYTE(int iValue);
void PARSE_MESSAGE_STRING(const char *sz);
void PARSE_MESSAGE_COORD(float flValue);

void PARSE_MESSAGE_UNSUPPORTED();

int GET_MESSAGE_SHORT(int index);
int GET_MESSAGE_BYTE(int index);
char *GET_MESSAGE_STRING(int index);
float GET_MESSAGE_COORD(int index);

struct dfunc_t
{
	void (*function)(void*);
};

extern struct dfunc_t msgHooks[MAX_REG_MSGS];
extern edict_t *pCurEntity;

union message_var_t
{
	float fValue;
	int	iValue;
	char sValue[100];
};

struct message_t
{
	union message_var_t mValue;
	int iType;
};

#endif