/* SeenServ - Nickname Seen Service - NeoStats Addon Module
** Copyright (c) 2004-2005 DeadNotBuried
** Portions Copyright (c) 1999-2005, NeoStats
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
	{"REMOVE",	sns_cmd_remove,		1,	NS_ULEVEL_ADMIN,	sns_help_remove,	sns_help_remove_oneline},
	{"STATS",	sns_cmd_stats,		0,	NS_ULEVEL_LOCOPER,	sns_help_stats,		sns_help_stats_oneline},
	{NULL,		NULL,			0, 	0,			NULL,			NULL}
};

static bot_setting sns_settings[]=
{
	{"VERBOSE",		&SeenServ.verbose,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_verbose,		NULL,			(void *)0 },
	{"EXCLUSIONS",		&SeenServ.exclusions,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_exclusions,	sns_set_exclusions,	(void *)1 },
	{"ENABLE",		&SeenServ.enable,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_enable,		NULL,			(void *)0 },
	{"ENABLESEENCHAN",	&SeenServ.enableseenchan,	SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_enableseenchan,	NULL,			(void *)0 },
	{"SEENCHANNAME",	&SeenServ.seenchan,		SET_TYPE_CHANNEL,	0,	MAXCHANLEN,	NS_ULEVEL_ADMIN,	NULL,	sns_help_set_seenchan,		sns_set_seenchan,	(void *)"#Seen" },
	{"MAXSEENENTRIES",	&SeenServ.maxseenentries,	SET_TYPE_INT,		100,	100000,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_maxentries,	sns_set_maxentries,	(void *)2000 },
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
	0,
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
	if (reason == SET_VALIDATE) {
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
