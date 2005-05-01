/* SeenServ - Nickname Seen Service - NeoStats Addon Module
** Copyright (c) 2003-2005 Justin Hammond, Mark Hetherington, DeadNotBuried
**
**  This program is free software; you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation; either version 2 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
**  USA
**
** SeenServ CVS Identification
** $Id$
*/

#include "neostats.h"    /* Required for bot support */
#include "seenserv.h"

/** Copyright info */
const char *sns_copyright[] = {
	"Copyright (c) 2004-2005 DeadNotBuried",
	"Portions Copyright (c) 1999-2005, NeoStats",
	NULL
};

const char *sns_about[] = {
	"\2Seen Service\2",
	"",
	"Provides access to SEEN command to see when Users were connected.",
	NULL
};

/*
 * Commands and Settings
*/
static bot_cmd sns_commands[]=
{
	{"SEEN",	sns_cmd_seenhost,	1,	0,			sns_help_seen,		sns_help_seen_oneline},
	{"SEENNICK",	sns_cmd_seennick,	1,	0,			sns_help_seennick,	sns_help_seennick_oneline},
	{"DEL",		sns_cmd_del,		1,	NS_ULEVEL_ADMIN,	sns_help_del,		sns_help_del_oneline},
	{"STATS",	sns_cmd_status,		0,	NS_ULEVEL_LOCOPER,	sns_help_status,		sns_help_status_oneline},
	{NULL,		NULL,			0, 	0,			NULL,			NULL}
};

static bot_setting sns_settings[]=
{
	{"VERBOSE",		&SeenServ.verbose,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_verbose,		NULL,			(void *)0 },
	{"EXCLUSIONS",		&SeenServ.exclusions,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_exclusions,	sns_set_exclusions,	(void *)1 },
	{"ENABLE",		&SeenServ.enable,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_enable,		NULL,			(void *)0 },
	{"ENABLESEENCHAN",	&SeenServ.enableseenchan,	SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_enableseenchan,	NULL,			(void *)0 },
	{"SEENCHANNAME",	&SeenServ.seenchan,		SET_TYPE_CHANNEL,	0,	MAXCHANLEN,	NS_ULEVEL_ADMIN,	NULL,	sns_help_set_seenchan,		sns_set_seenchan,	(void *)"#Seen" },
	{"MAXENTRIES",		&SeenServ.maxentries,		SET_TYPE_INT,		100,	100000,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_maxentries,	sns_set_maxentries,	(void *)2000 },
	{"EVENTSIGNON",		&SeenServ.eventsignon,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_eventsignon,	sns_set_eventsignon,	(void *)1 },
	{"EVENTQUIT",		&SeenServ.eventquit,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_eventquit,		sns_set_eventquit,	(void *)1 },
	{"EVENTKILL",		&SeenServ.eventkill,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_eventkill,		sns_set_eventkill,	(void *)1 },
	{"EVENTNICK",		&SeenServ.eventnick,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_eventnick,		sns_set_eventnick,	(void *)1 },
	{"EVENTJOIN",		&SeenServ.eventjoin,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_eventjoin,		sns_set_eventjoin,	(void *)1 },
	{"EVENTPART",		&SeenServ.eventpart,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_eventpart,		sns_set_eventpart,	(void *)1 },
	{"EVENTKICK",		&SeenServ.eventkick,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_eventkick,		sns_set_eventkick,	(void *)1 },
	{"EXPIRETIME",		&SeenServ.expiretime,		SET_TYPE_INT,		0,	1000,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_expiretime,	sns_set_expiretime,	(void *)0 },
	{NULL,			NULL,				0,			0,	0,		0,			NULL,	NULL,				NULL, 			NULL },
};

/*
 * Module Info definition 
*/
ModuleInfo module_info = {
	"SeenServ",
	"Seen Service Module For NeoStats",
	sns_copyright,
	sns_about,
	NEOSTATS_VERSION,
	"3.0",
	__DATE__,
	__TIME__,
	MODULE_FLAG_LOCAL_EXCLUDES,
	0,
};

/*
 * Module event list
*/
ModuleEvent module_events[] = {
	{EVENT_SIGNON,		SeenSignon,		EVENT_FLAG_EXCLUDE_ME},
	{EVENT_NICKIP,		SeenSignon,		EVENT_FLAG_EXCLUDE_ME},
	{EVENT_QUIT,		SeenQuit,		EVENT_FLAG_EXCLUDE_ME},
	{EVENT_KILL,		SeenKill,		EVENT_FLAG_EXCLUDE_ME},
	{EVENT_LOCALKILL,	SeenKill,		EVENT_FLAG_EXCLUDE_ME},
	{EVENT_GLOBALKILL,	SeenKill,		EVENT_FLAG_EXCLUDE_ME},
	{EVENT_SERVERKILL,	SeenKill,		EVENT_FLAG_EXCLUDE_ME},
	{EVENT_NICK,		SeenNickChange,		EVENT_FLAG_EXCLUDE_ME},
	{EVENT_JOIN,		SeenJoinChan,		EVENT_FLAG_EXCLUDE_ME},
	{EVENT_PART,		SeenPartChan,		EVENT_FLAG_EXCLUDE_ME},
	{EVENT_KICK,		SeenKicked,		EVENT_FLAG_EXCLUDE_ME},
	{EVENT_NULL,		NULL}
};

/** BotInfo */
static BotInfo sns_botinfo = 
{
	"SeenServ",
	"SeenServ1",
	"Seen",
	BOT_COMMON_HOST,
	"Seen Service",
	BOT_FLAG_SERVICEBOT|BOT_FLAG_PERSIST,
	sns_commands,
	sns_settings,
};

/*
 * Online event processing
*/
int ModSynch (void)
{
	/* Introduce a bot onto the network */
	sns_bot = AddBot (&sns_botinfo);	
	if (!sns_bot) {
		return NS_FAILURE;
	}
	if (SeenServ.enableseenchan) {
		irc_join (sns_bot, SeenServ.seenchan, "+o");
		irc_chanalert (sns_bot, "Seen Channel Now Available in %s", SeenServ.seenchan);
	} else {
		irc_chanalert (sns_bot, "Seen Channel Not Enabled");
	}
	return NS_SUCCESS;
};

/*
 * Init module
*/
int ModInit( void )
{
	ModuleConfig (sns_settings);
	loadseendata();
	return NS_SUCCESS;
}

/*
 * Exit module
*/
int ModFini( void )
{
	destroyseenlist();
	return NS_SUCCESS;
}

/*
 * Seen Channel Setting
*/
static int sns_set_seenchan (CmdParams *cmdparams, SET_REASON reason) {
	if (!SeenServ.enableseenchan) {
		return NS_SUCCESS;
	}
	if (reason == SET_VALIDATE) {
		irc_prefmsg (sns_bot, cmdparams->source, "Seen Channel changing from %s to %s", SeenServ.seenchan, cmdparams->av[1]);
		irc_chanalert (sns_bot, "Seen Channel Changing to %s , Parting %s (%s)", cmdparams->av[1], SeenServ.seenchan, cmdparams->source->name);
		irc_chanprivmsg (sns_bot, SeenServ.seenchan, "\0039%s has changed Channels, Seen functions will now be available in %s", cmdparams->source->name, cmdparams->av[1]);
		irc_part (sns_bot, SeenServ.seenchan, NULL);
		return NS_SUCCESS;
	}
	if (reason == SET_CHANGE) {
		irc_join (sns_bot, SeenServ.seenchan, "+o");
		irc_chanalert (sns_bot, "Seen functions now available in %s", SeenServ.seenchan);
		return NS_SUCCESS;
	}
	return NS_SUCCESS;
}

/*
 * Change Max Entries Saved
*/
static int sns_set_maxentries (CmdParams *cmdparams, SET_REASON reason) {
	if (reason == SET_CHANGE) {
		checkseenlistlimit();
		return NS_SUCCESS;
	}
	return NS_SUCCESS;
}

/*
 * Enable/Disable Global Exclusions
*/
static int sns_set_exclusions( CmdParams *cmdparams, SET_REASON reason )
{
	if( reason == SET_LOAD || reason == SET_CHANGE )
	{
		SetAllEventFlags( EVENT_FLAG_USE_EXCLUDE, SeenServ.exclusions );
	}
	return NS_SUCCESS;
}

/*
 * Enable/Disable Events
*/
static int sns_set_eventsignon( CmdParams *cmdparams, SET_REASON reason )
{
	if( reason == SET_LOAD || reason == SET_CHANGE )
	{
		if (SeenServ.eventsignon) {
			EnableEvent(EVENT_SIGNON);
			EnableEvent(EVENT_NICKIP);
		} else {
			DisableEvent(EVENT_SIGNON);
			DisableEvent(EVENT_NICKIP);
		}
	}
	return NS_SUCCESS;
}

static int sns_set_eventquit( CmdParams *cmdparams, SET_REASON reason )
{
	if( reason == SET_LOAD || reason == SET_CHANGE )
	{
		if (SeenServ.eventquit) {
			EnableEvent(EVENT_QUIT);
		} else {
			DisableEvent(EVENT_QUIT);
		}
	}
	return NS_SUCCESS;
}

static int sns_set_eventkill( CmdParams *cmdparams, SET_REASON reason )
{
	if( reason == SET_LOAD || reason == SET_CHANGE )
	{
		if (SeenServ.eventkill) {
			EnableEvent(EVENT_KILL);
			EnableEvent(EVENT_LOCALKILL);
			EnableEvent(EVENT_GLOBALKILL);
			EnableEvent(EVENT_SERVERKILL);
		} else {
			DisableEvent(EVENT_KILL);
			DisableEvent(EVENT_LOCALKILL);
			DisableEvent(EVENT_GLOBALKILL);
			DisableEvent(EVENT_SERVERKILL);
		}
	}
	return NS_SUCCESS;
}

static int sns_set_eventnick( CmdParams *cmdparams, SET_REASON reason )
{
	if( reason == SET_LOAD || reason == SET_CHANGE )
	{
		if (SeenServ.eventnick) {
			EnableEvent(EVENT_NICK);
		} else {
			DisableEvent(EVENT_NICK);
		}
	}
	return NS_SUCCESS;
}

static int sns_set_eventjoin( CmdParams *cmdparams, SET_REASON reason )
{
	if( reason == SET_LOAD || reason == SET_CHANGE )
	{
		if (SeenServ.eventjoin) {
			EnableEvent(EVENT_JOIN);
		} else {
			DisableEvent(EVENT_JOIN);
		}
	}
	return NS_SUCCESS;
}

static int sns_set_eventpart( CmdParams *cmdparams, SET_REASON reason )
{
	if( reason == SET_LOAD || reason == SET_CHANGE )
	{
		if (SeenServ.eventpart) {
			EnableEvent(EVENT_PART);
		} else {
			DisableEvent(EVENT_PART);
		}
	}
	return NS_SUCCESS;
}

static int sns_set_eventkick( CmdParams *cmdparams, SET_REASON reason )
{
	if( reason == SET_LOAD || reason == SET_CHANGE )
	{
		if (SeenServ.eventkick) {
			EnableEvent(EVENT_KICK);
		} else {
			DisableEvent(EVENT_KICK);
		}
	}
	return NS_SUCCESS;
}

/*
 * Check Entry Saved Time
*/
static int sns_set_expiretime (CmdParams *cmdparams, SET_REASON reason) {
	if (reason == SET_CHANGE && SeenServ.expiretime > 0) {
		checkseenlistlimit();
		return NS_SUCCESS;
	}
	return NS_SUCCESS;
}

