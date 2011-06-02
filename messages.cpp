#include <extdll.h>

#include <dllapi.h>
#include <meta_api.h>

#include <cbase.h>
#include <player.h>

#include "messages.h"

struct dfunc_t msgHooks[MAX_REG_MSGS];

int curMessageIndex;

bool msgActive;

message_t curMessage[10];

int cur_msg_type;

edict_t *pCurEntity;

void REG_MESSAGE_HOOK(char *msg, void (*function)(void*))
{
	int msg_id;

	if(msg_id = GET_USER_MSG_ID(PLID, msg, NULL ))
	{
		msgHooks[msg_id].function = function;
	}
}

void BEGIN_MESSAGE_HOOK(int msg_dest, int msg_type, edict_t *pEntity)
{
	curMessageIndex = 0;

	cur_msg_type = msg_type;

	if(msgHooks[cur_msg_type].function)
	{
		msgActive = true;
		pCurEntity = pEntity;
	}
	else
	{
		msgActive = false;
	}	
}

void CALL_MESSAGE_HOOK()
{
	char msg[50];

	sprintf(msg, "Called: %d\n", cur_msg_type);

	SERVER_COMMAND(msg);
	if(msgHooks[cur_msg_type].function)
	{
		(*msgHooks[cur_msg_type].function)(NULL);
	}
}

void PARSE_MESSAGE_SHORT(int iValue)
{
	if(msgActive)
	{
		message_var_t message_var;
		message_t message;

		message_var.iValue = iValue;

		//SERVER_PRINT(temp->sValue);

		message.iType = MESSAGE_TYPE_SHORT;
		message.mValue = message_var;

		CLEAR_VAR(curMessageIndex);

		curMessage[curMessageIndex] = message;

		curMessageIndex++;
	}
}

void PARSE_MESSAGE_BYTE(int iValue)
{
	if(msgActive)
	{
		message_var_t message_var;
		message_t message;

		message_var.iValue = iValue;

		//SERVER_PRINT(temp->sValue);

		message.iType = MESSAGE_TYPE_BYTE;
		message.mValue = message_var;

		CLEAR_VAR(curMessageIndex);

		curMessage[curMessageIndex] = message;

		curMessageIndex++;
	}
}

void PARSE_MESSAGE_COORD(float flValue)
{
	if(msgActive)
	{
		message_var_t message_var;
		message_t message;

		message_var.fValue = flValue;

		message.iType = MESSAGE_TYPE_COORD;
		message.mValue = message_var;

		CLEAR_VAR(curMessageIndex);

		curMessage[curMessageIndex] = message;

		curMessageIndex++;
	}
}

void PARSE_MESSAGE_STRING(const char *sz)
{
	if(msgActive)
	{
		message_var_t message_var;
		message_t message;

		strcpy(message_var.sValue, sz);

		/*char tmp[50];
		sprintf(tmp, "Setting index %d to %s\n", curMessageIndex, sz);
		SERVER_PRINT(tmp);*/

		message.iType = MESSAGE_TYPE_STRING;
		message.mValue = message_var;

		CLEAR_VAR(curMessageIndex);

		curMessage[curMessageIndex] = message;

		curMessageIndex++;
	}
}

void PARSE_MESSAGE_UNSUPPORTED()
{
	if(msgActive)
	{
		curMessageIndex++;
	}
}

int GET_MESSAGE_SHORT(int index)
{
	if(index >= curMessageIndex)
		return -1;

	if(curMessage[index].iType != MESSAGE_TYPE_SHORT)
		return -2;

	return curMessage[index].mValue.iValue;
}

int GET_MESSAGE_BYTE(int index)
{
	if(index >= curMessageIndex)
		return -1;

	if(curMessage[index].iType != MESSAGE_TYPE_BYTE)
		return -2;

	return curMessage[index].mValue.iValue;
}

float GET_MESSAGE_COORD(int index)
{
	if(index >= curMessageIndex)
		return -1;

	if(curMessage[index].iType != MESSAGE_TYPE_COORD)
		return -2;

	return curMessage[index].mValue.fValue;
}

char *GET_MESSAGE_STRING(int index)
{
	if(index >= curMessageIndex)
		return NULL;

	if(curMessage[index].iType != MESSAGE_TYPE_STRING)
		return NULL;

	return curMessage[index].mValue.sValue;
}

void CLEAR_VAR(int index)
{

}