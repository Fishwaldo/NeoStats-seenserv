/* SeenServ - Nickname Seen Service - NeoStats Addon Module
** Copyright (c) 2003-2006 Justin Hammond, Mark Hetherington, Jeff Lang
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

static list_t *seenchanlist;

static int sns_set_enablechan (const CmdParams *cmdparams, SET_REASON reason);
static int sns_set_seenchan (const CmdParams *cmdparams, SET_REASON reason);
static int sns_set_maxentries (const CmdParams *cmdparams, SET_REASON reason);
static int sns_set_exclusions (const CmdParams *cmdparams, SET_REASON reason);
static int sns_set_eventsignon( const CmdParams *cmdparams, SET_REASON reason );
static int sns_set_eventquit( const CmdParams *cmdparams, SET_REASON reason );
static int sns_set_eventkill( const CmdParams *cmdparams, SET_REASON reason );
static int sns_set_eventnick( const CmdParams *cmdparams, SET_REASON reason );
static int sns_set_eventjoin( const CmdParams *cmdparams, SET_REASON reason );
static int sns_set_eventpart( const CmdParams *cmdparams, SET_REASON reason );
static int sns_set_eventkick( const CmdParams *cmdparams, SET_REASON reason );
static int sns_set_expiretime (const CmdParams *cmdparams, SET_REASON reason);
static int sns_set_dbupdatetime (const CmdParams *cmdparams, SET_REASON reason);
static int sns_set_memorylist (const CmdParams *cmdparams, SET_REASON reason);

/** Copyright info */
const char *sns_copyright[] = {
	"Copyright (c) 2006, NeoStats",
	"http://www.neostats.net/",
	NULL};

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
	{"SEEN",	sns_cmd_seenhost,	1,	0,			sns_help_seen},
	{"SEENNICK",	sns_cmd_seennick,	1,	0,			sns_help_seennick},
	{"DEL",		sns_cmd_del,		1,	NS_ULEVEL_ADMIN,	sns_help_del},
	{"STATUS",	sns_cmd_status,		0,	NS_ULEVEL_LOCOPER,	sns_help_status},
	{"CHAN",	sns_cmd_chan,		0,	NS_ULEVEL_ADMIN,	sns_help_chan},
	NS_CMD_END()
};

static bot_setting sns_settings[]=
{
	{"EXCLUSIONS",		&SeenServ.exclusions,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_exclusions,	sns_set_exclusions,	(void *)1 },
	{"ENABLE",		&SeenServ.enable,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_enable,		NULL,			(void *)0 },
	{"ENABLESEENCHAN",	&SeenServ.enableseenchan,	SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_enableseenchan,	sns_set_enablechan,	(void *)0 },
	{"SEENCHANNAME",	&SeenServ.seenchan,		SET_TYPE_CHANNEL,	0,	MAXCHANLEN,	NS_ULEVEL_ADMIN,	NULL,	sns_help_set_seenchanname,	sns_set_seenchan,	(void *)"#Seen" },
	{"MAXENTRIES",		&SeenServ.maxentries,		SET_TYPE_INT,		100,	100000,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_maxentries,	sns_set_maxentries,	(void *)2000 },
	{"EVENTSIGNON",		&SeenServ.eventsignon,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_eventsignon,	sns_set_eventsignon,	(void *)1 },
	{"EVENTQUIT",		&SeenServ.eventquit,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_eventquit,		sns_set_eventquit,	(void *)1 },
	{"EVENTKILL",		&SeenServ.eventkill,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_eventkill,		sns_set_eventkill,	(void *)1 },
	{"EVENTNICK",		&SeenServ.eventnick,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_eventnick,		sns_set_eventnick,	(void *)1 },
	{"EVENTJOIN",		&SeenServ.eventjoin,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_eventjoin,		sns_set_eventjoin,	(void *)1 },
	{"EVENTPART",		&SeenServ.eventpart,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_eventpart,		sns_set_eventpart,	(void *)1 },
	{"EVENTKICK",		&SeenServ.eventkick,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_eventkick,		sns_set_eventkick,	(void *)1 },
	{"EXPIRETIME",		&SeenServ.expiretime,		SET_TYPE_INT,		0,	1000,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_expiretime,	sns_set_expiretime,	(void *)0 },
	{"DBUPDATETIME",	&SeenServ.dbupdatetime,		SET_TYPE_INT,		1,	900,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_dbupdatetime,	sns_set_dbupdatetime,	(void *)300 },
	{"MEMORYLIST",		&SeenServ.memorylist,		SET_TYPE_BOOLEAN,	0,	0,		NS_ULEVEL_ADMIN,	NULL,	sns_help_set_memorylist,	sns_set_memorylist,	(void *)1 },
	NS_SETTING_END()
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
	MODULE_VERSION,
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
	{EVENT_SERVERKILL,	SeenKill,		EVENT_FLAG_EXCLUDE_ME},
	{EVENT_GLOBALKILL,	SeenKill,		EVENT_FLAG_EXCLUDE_ME},
	{EVENT_NICK,		SeenNickChange,		EVENT_FLAG_EXCLUDE_ME},
	{EVENT_JOIN,		SeenJoinChan,		EVENT_FLAG_EXCLUDE_ME},
	{EVENT_PART,		SeenPartChan,		EVENT_FLAG_EXCLUDE_ME},
	{EVENT_KICK,		SeenKicked,		EVENT_FLAG_EXCLUDE_ME},
	{EVENT_NEWCHAN,		SeenNewChan,		0},
	{EVENT_DELCHAN,		SeenDelChan,		0},
	NS_EVENT_END()
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
	ExtraSeenChans *esc;
	lnode_t *ln;

	/* Introduce a bot onto the network */
	sns_bot = AddBot (&sns_botinfo);	
	if (!sns_bot)
		return NS_FAILURE;
	if (SeenServ.enableseenchan) {
		irc_join (sns_bot, SeenServ.seenchan, "+o");
		irc_chanalert (sns_bot, "Seen Channel Now Available in %s", SeenServ.seenchan);
	} else {
		irc_chanalert (sns_bot, "Seen Channel Not Enabled");
	}
	ln = list_first(seenchanlist);
	while (ln != NULL)
	{
		esc = lnode_get(ln);
		esc->c = FindChannel(esc->name);
		if (esc->c)
			irc_join (sns_bot, esc->name, "+o");
		ln = list_next(seenchanlist, ln);
	}
	AddTimer (TIMER_TYPE_DAILY, removeagedseenrecords, "removeagedseenrecords", 0, NULL);
	AddTimer (TIMER_TYPE_INTERVAL, dbsavetimer, "seenservdbsavetimer", SeenServ.dbupdatetime, NULL);
	return NS_SUCCESS;
};

/*
 * Init module
*/
int ModInit( void )
{
	ModuleConfig (sns_settings);
	DBAOpenTable("seendata");
	createseenlist();
	if( SeenServ.memorylist )
		loadseendata();
	seenchanlist = list_create( LISTCOUNT_T_MAX );
	loadseenchandata();
	return NS_SUCCESS;
}

/*
 * Exit module
*/
int ModFini( void )
{
	DelTimer ("seenservdbsavetimer");
	DelTimer ("removeagedseenrecords");
	dbsavetimer(NULL);
	destroyseenchanlist();
	destroyseenlist();
	DBACloseTable("seendata");
	return NS_SUCCESS;
}

/*
 * Seen Channel Enable/Disable
*/
static int sns_set_enablechan (const CmdParams *cmdparams, SET_REASON reason) 
{
	if (!SeenServ.seenchan)
		return NS_SUCCESS;
	if (reason == SET_CHANGE) 
	{
		if (SeenServ.enableseenchan) 
		{
			irc_join (sns_bot, SeenServ.seenchan, "+o");
			irc_chanalert (sns_bot, "Seen functions now available in %s", SeenServ.seenchan);
			return NS_SUCCESS;
		} else {
			irc_part (sns_bot, SeenServ.seenchan, NULL);
			irc_chanalert (sns_bot, "Seen functions are no longer available in %s", SeenServ.seenchan);
			return NS_SUCCESS;
		}
	}
	return NS_SUCCESS;
}

/*
 * Seen Channel Setting
*/
static int sns_set_seenchan (const CmdParams *cmdparams, SET_REASON reason) 
{
	if (!SeenServ.enableseenchan)
		return NS_SUCCESS;
	if (reason == SET_VALIDATE) 
	{
		irc_prefmsg (sns_bot, cmdparams->source, "Seen Channel changing from %s to %s", SeenServ.seenchan, cmdparams->av[1]);
		irc_chanalert (sns_bot, "Seen Channel Changing to %s , Parting %s (%s)", cmdparams->av[1], SeenServ.seenchan, cmdparams->source->name);
		irc_chanprivmsg (sns_bot, SeenServ.seenchan, "\0039%s has changed Channels, Seen functions will now be available in %s", cmdparams->source->name, cmdparams->av[1]);
		irc_part (sns_bot, SeenServ.seenchan, NULL);
		return NS_SUCCESS;
	}
	if (reason == SET_CHANGE) 
	{
		irc_join (sns_bot, SeenServ.seenchan, "+o");
		irc_chanalert (sns_bot, "Seen functions now available in %s", SeenServ.seenchan);
		return NS_SUCCESS;
	}
	return NS_SUCCESS;
}

/*
 * Change Max Entries Saved
*/
static int sns_set_maxentries (const CmdParams *cmdparams, SET_REASON reason) 
{
	if (reason == SET_CHANGE) 
	{
		checkseenlistlimit(SS_LISTLIMIT_COUNT);
		return NS_SUCCESS;
	}
	return NS_SUCCESS;
}

/*
 * Enable/Disable Global Exclusions
*/
static int sns_set_exclusions( const CmdParams *cmdparams, SET_REASON reason )
{
	if( reason == SET_LOAD || reason == SET_CHANGE )
		SetAllEventFlags( EVENT_FLAG_USE_EXCLUDE, SeenServ.exclusions );
	return NS_SUCCESS;
}

/*
 * Enable/Disable Events
*/
static int sns_set_eventsignon( const CmdParams *cmdparams, SET_REASON reason )
{
	if( reason == SET_LOAD || reason == SET_CHANGE )
	{
		if (SeenServ.eventsignon)
			EnableEvent(EVENT_SIGNON);
		else
			DisableEvent(EVENT_SIGNON);
	}
	return NS_SUCCESS;
}

static int sns_set_eventquit( const CmdParams *cmdparams, SET_REASON reason )
{
	if( reason == SET_LOAD || reason == SET_CHANGE )
	{
		if (SeenServ.eventquit)
			EnableEvent(EVENT_QUIT);
		else
			DisableEvent(EVENT_QUIT);
	}
	return NS_SUCCESS;
}

static int sns_set_eventkill( const CmdParams *cmdparams, SET_REASON reason )
{
	if( reason == SET_LOAD || reason == SET_CHANGE )
	{
		if (SeenServ.eventkill) 
		{
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

static int sns_set_eventnick( const CmdParams *cmdparams, SET_REASON reason )
{
	if( reason == SET_LOAD || reason == SET_CHANGE )
	{
		if (SeenServ.eventnick)
			EnableEvent(EVENT_NICK);
		else
			DisableEvent(EVENT_NICK);
	}
	return NS_SUCCESS;
}

static int sns_set_eventjoin( const CmdParams *cmdparams, SET_REASON reason )
{
	if( reason == SET_LOAD || reason == SET_CHANGE )
	{
		if (SeenServ.eventjoin)
			EnableEvent(EVENT_JOIN);
		else
			DisableEvent(EVENT_JOIN);
	}
	return NS_SUCCESS;
}

static int sns_set_eventpart( const CmdParams *cmdparams, SET_REASON reason )
{
	if( reason == SET_LOAD || reason == SET_CHANGE )
	{
		if (SeenServ.eventpart)
			EnableEvent(EVENT_PART);
		else
			DisableEvent(EVENT_PART);
	}
	return NS_SUCCESS;
}

static int sns_set_eventkick( const CmdParams *cmdparams, SET_REASON reason )
{
	if( reason == SET_LOAD || reason == SET_CHANGE )
	{
		if (SeenServ.eventkick)
			EnableEvent(EVENT_KICK);
		else
			DisableEvent(EVENT_KICK);
	}
	return NS_SUCCESS;
}

/*
 * Check Entry Saved Time
*/
static int sns_set_expiretime (const CmdParams *cmdparams, SET_REASON reason) 
{
	if (reason == SET_CHANGE && SeenServ.expiretime > 0) 
	{
		checkseenlistlimit(SS_LISTLIMIT_AGE);
		return NS_SUCCESS;
	}
	return NS_SUCCESS;
}

/*
 * Check DB Update Time and update timer interval
*/
static int sns_set_dbupdatetime (const CmdParams *cmdparams, SET_REASON reason) 
{
	if (reason == SET_CHANGE) 
		SetTimerInterval( "seenservdbsavetimer", SeenServ.dbupdatetime );
	return NS_SUCCESS;
}

/*
 * Check Meory List is actually changing, and perform actions as needed
*/
static int sns_set_memorylist (const CmdParams *cmdparams, SET_REASON reason) 
{
	if (reason == SET_VALIDATE)
	{
		if( SeenServ.memorylist && !ircstrcasecmp( cmdparams->av[1], "ON" ) )
		{
			irc_prefmsg (sns_bot, cmdparams->source, "ERROR: MEMORYLIST is already set ON");
			return NS_FAILURE;
		}
		if( !SeenServ.memorylist && !ircstrcasecmp( cmdparams->av[1], "OFF" ) )
		{
			irc_prefmsg (sns_bot, cmdparams->source, "ERROR: MEMORYLIST is already set OFF");
			return NS_FAILURE;
		}
	}
	if (reason == SET_CHANGE) 
	{
		if( SeenServ.memorylist )
		{
			loadseendata();
			checkseenlistlimit(SS_LISTLIMIT_COUNT);
			removeagedseenrecords( NULL);
		} else {
			dbsavetimer( NULL );
		}
	}
	return NS_SUCCESS;
}

/*
 * Remove Aged Records if required
*/
int removeagedseenrecords(void *userptr) 
{
	SET_SEGV_LOCATION();
	if( SeenServ.expiretime > 0 )
		checkseenlistlimit(SS_LISTLIMIT_AGE);
	return NS_SUCCESS;
}

/*
 * Add/Remove/List Extra Seen Chans
*/
int sns_cmd_chan (const CmdParams *cmdparams)
{
	ExtraSeenChans *esc;
	lnode_t *ln;

	if (!ircstrcasecmp(cmdparams->av[0], "LIST"))
	{
		ln = list_first(seenchanlist);
		while (ln != NULL)
		{
			esc = lnode_get(ln);
			seen_report( cmdparams, "%s %s", esc->name, esc->c ? "(Open)" : "" );
			ln = list_next(seenchanlist, ln);
		}
		seen_report( cmdparams, "End Of List" );
		return NS_SUCCESS;
	} else {
		if (cmdparams->ac > 1)
		{
			if (!ircstrcasecmp(cmdparams->av[0], "ADD"))
			{
				ln = list_first(seenchanlist);
				while (ln != NULL)
				{
					esc = lnode_get(ln);
					if (!ircstrcasecmp(esc->name, cmdparams->av[1]))
						break;
					ln = list_next(seenchanlist, ln);
				}
				if (ln != NULL)
				{
					seen_report( cmdparams, "%s has already been added to the Extra Seen Chans List", cmdparams->av[1] );
					return NS_SUCCESS;
				}
				esc = ns_calloc( sizeof( ExtraSeenChans ) );
				strlcpy(esc->name, cmdparams->av[1], MAXCHANLEN);
				esc->c = NULL;
				ns_strlwr(esc->name);
				lnode_create_append( seenchanlist, esc );
				DBAStore( "ExtraSeenChans", esc->name, ( void * )esc, sizeof( ExtraSeenChans ) );
				esc->c = FindChannel(esc->name);
				if (esc->c)
					irc_join (sns_bot, esc->name, "+o");
				seen_report( cmdparams, "%s added to the Extra Seen Chans List", cmdparams->av[1] );
				list_sort( seenchanlist, sns_sort_chanlist );
				return NS_SUCCESS;
			} else if (!ircstrcasecmp(cmdparams->av[0], "DEL")) {
				ln = list_first(seenchanlist);
				while (ln != NULL)
				{
					esc = lnode_get(ln);
					if (!ircstrcasecmp(esc->name, cmdparams->av[1]))
						break;
					ln = list_next(seenchanlist, ln);
				}
				if (ln == NULL)
				{
					seen_report( cmdparams, "%s is not an Extra Seen Chan", cmdparams->av[1] );
					return NS_SUCCESS;
				}
				irc_part (sns_bot, esc->name, NULL);
				DBADelete( "ExtraSeenChans", esc->name);
				list_delete(seenchanlist, ln);
				lnode_destroy(ln);
				seen_report( cmdparams, "%s removed from the Extra Seen Chans List", cmdparams->av[1] );
				ns_free(esc);
			}
		}
	}
	return NS_SUCCESS;
}

int sns_sort_chanlist( const void *key1, const void *key2 ) {
	const ExtraSeenChans *esc1 = key1;
	const ExtraSeenChans *esc2 = key2;
	return( ircstrcasecmp( esc1->name, esc2->name ) );
}

/*
 * Load Saved Seen Records
*/
int loadseenchanrecords(void *data, int size)
{
	ExtraSeenChans *esc;

	esc = ns_calloc( sizeof( ExtraSeenChans ) );
	os_memcpy( esc, data, sizeof( ExtraSeenChans ) );
	lnode_create_append( seenchanlist, esc );
	return NS_FALSE;
}

void loadseenchandata(void)
{
	DBAFetchRows( "ExtraSeenChans", loadseenchanrecords );
	list_sort( seenchanlist, sns_sort_chanlist );
	return;
}

/*
 * Destroy Seen Chan List
*/
void destroyseenchanlist(void)
{
	lnode_t *ln, *ln2;
	ExtraSeenChans *esc;

	ln = list_first(seenchanlist);
	while( ln )
	{
		esc = lnode_get(ln);
		ln2 = list_next( seenchanlist, ln );
		ns_free(esc);
		list_delete(seenchanlist, ln);
		lnode_destroy(ln);
		ln = ln2;
	}
	list_destroy_auto(seenchanlist);
}

/*
 *  Join on channel creation / Part on Empty Channel
 */
int SeenNewChan (const CmdParams *cmdparams) 
{
	lnode_t *ln;
	ExtraSeenChans *esc;

	SET_SEGV_LOCATION();
	ln = list_first(seenchanlist);
	while( ln )
	{
		esc = lnode_get(ln);
		if( !ircstrcasecmp( esc->name , cmdparams->channel->name ) )
		{
			esc->c = cmdparams->channel;
			irc_join (sns_bot, esc->name, "+o");
			break;
		}
		ln = list_next( seenchanlist, ln );
	}
	return NS_SUCCESS;
}

int SeenDelChan (const CmdParams *cmdparams) 
{
	lnode_t *ln;
	ExtraSeenChans *esc;

	SET_SEGV_LOCATION();
	if( cmdparams->channel->users != 1 )
		return NS_SUCCESS;
	ln = list_first(seenchanlist);
	while( ln )
	{
		esc = lnode_get(ln);
		if( !ircstrcasecmp( esc->name , cmdparams->channel->name ) )
		{
			irc_part (sns_bot, esc->name, NULL);
			esc->c = NULL;
			break;
		}
		ln = list_next( seenchanlist, ln );
	}
	return NS_SUCCESS;
}
