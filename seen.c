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

static list_t *seenlist;

static char timetext[4][12];
static char combinedtimetext[SS_GENCHARLEN];
static char matchstr[USERHOSTLEN];
static char currentlyconnectedtext[SS_GENCHARLEN];
static char seenentrynick[5][MAXNICK+3];
static char matchednickstr[SS_MESSAGESIZE];

/** @brief findnick
 *
 *  list sorting helper
 *
 *  @param key1
 *  @param key2
 *
 *  @return results of strcmp
 */

int findnick( const void *key1, const void *key2 )
{
	SeenData *sd = ( SeenData * ) key1;
	return ircstrcasecmp( sd->nick, key2 );
}

/*
 *  Removes SeenData for nickname if exists
*/
void removepreviousnick(char *nick)
{
	lnode_t *ln;
	SeenData *sd;

/*		for some reason this bit segfaults, so commented out for now
		and manually searching for previous entries on the list
		(it's not just the DBADelete)

	ln = lnode_find( seenlist, nick, findnick );
	if (!ln)
		return;
	sd = lnode_get(ln);
	 * DBADelete( "seendata", sd->nick);
	ns_free(sd);
	list_delete(seenlist, ln);
	lnode_destroy(ln);
*/
	ln = list_first( seenlist );
	while ( ln )
	{
		sd = lnode_get( ln );
		if (!ircstrcasecmp(nick, sd->nick)) 
		{
			/* commented out the delete due to segfaults
			 *
			 *  DBADelete( "seendata", sd->nick);
			 */
			ns_free( sd );
			list_delete( seenlist, ln );
			lnode_destroy( ln );
			ln = list_last( seenlist );
		}
		ln = list_next( seenlist, ln );
	}
}

/*
 *  add new seen entry to list
*/
void addseenentry(char *nick, char *host, char *vhost, char *message, int type)
{
	SeenData *sd;
	
	removepreviousnick(nick);
	sd = ns_calloc(sizeof(SeenData));
	strlcpy(sd->nick, nick, MAXNICK);
	strlcpy(sd->userhost, host, USERHOSTLEN);
	strlcpy(sd->uservhost, vhost, USERHOSTLEN);
	strlcpy(sd->message, message ? message : "", SS_MESSAGESIZE);
	sd->seentype = type;
	sd->seentime = me.now;
	lnode_create_append( seenlist, sd );
	DBAStore( "seendata", sd->nick,( void * )sd, sizeof( SeenData ) );
	checkseenlistlimit();
}

/*
 *  Removes SeenData if records past max entries setting
*/
void checkseenlistlimit(void)
{
	int currentlistcount;
	int maxageallowed;
	lnode_t *ln, *ln2;
	SeenData *sd;

	currentlistcount = list_count(seenlist);
	maxageallowed = me.now - ( SeenServ.expiretime * 86400 );
	ln = list_first( seenlist );
	sd = lnode_get( ln );
	while( ( currentlistcount > SeenServ.maxentries ) || ( SeenServ.expiretime > 0 && ( maxageallowed > sd->seentime ) ) )
	{
		ln2 = list_next( seenlist, ln );
		DBADelete( "seendata", sd->nick );
		ns_free( sd );
		list_delete( seenlist, ln );
		lnode_destroy( ln );
		ln = ln2;
		sd = lnode_get( ln );
		currentlistcount --;
	}
}

/*
 * Load Saved Seen Records
*/
int loadseenrecords(void *data, int size)
{
	SeenData *sd;

	sd = ns_calloc( sizeof( SeenData ) );
	os_memcpy( sd, data, sizeof( SeenData ) );
	lnode_create_append( seenlist, sd );
	return NS_FALSE;
}

void loadseendata(void)
{
	seenlist = list_create( -1 );
	DBAFetchRows( "seendata", loadseenrecords );
	list_sort( seenlist, sortlistbytime );
}

int sortlistbytime( const void *key1, const void *key2 )
{
	const SeenData *sd1 = key1;
	const SeenData *sd2 = key2;
	return (sd1->seentime - sd2->seentime);
}

/*
 * Destroy Seen List
*/
void destroyseenlist(void)
{
	lnode_t *ln, *ln2;
	SeenData *sd;

	/*
	 * Destroy the Seen List just to ensure
	 * all memory is free'd
	*/
	ln = list_first(seenlist);
	while( ln )
	{
		sd = lnode_get(ln);
		ln2 = list_next( seenlist, ln );
		ns_free(sd);
		list_delete(seenlist, ln);
		lnode_destroy(ln);
		ln = ln2;
	}
	list_destroy_auto(seenlist);
}

/*
 * Seen for wildcarded Host
*/
int sns_cmd_seenhost(CmdParams *cmdparams) 
{
	SET_SEGV_LOCATION();
	return CheckSeenData(cmdparams, SS_CHECK_WILDCARD);
}

/*
 * Seen for valid nickname
*/
int sns_cmd_seennick(CmdParams *cmdparams)
{
	SET_SEGV_LOCATION();
	return CheckSeenData(cmdparams, SS_CHECK_NICK);
}

/** seen_report
 *
 *  handles channel/user message selection
 */
static char seen_report_buf[BUFSIZE];

void seen_report( CmdParams *cmdparams, const char *fmt, ... )
{
	va_list ap;

	va_start( ap, fmt );
	ircvsnprintf( seen_report_buf, BUFSIZE, fmt, ap );
	va_end( ap );
	if( cmdparams->channel == NULL )
		irc_prefmsg (sns_bot, cmdparams->source, seen_report_buf );
	else
		irc_chanprivmsg (sns_bot, cmdparams->channel->name, seen_report_buf );
}


/*
 * Check For Seen Records
*/
int CheckSeenData(CmdParams *cmdparams, SEEN_CHECK checktype)
{
	lnode_t *ln;
	SeenData *sd;
	SeenData *sdo;
	Client *u;
	Channel *c;
	char *seenhostmask;
	int d, h, m, s, matchfound, seenentriesfound;
	
	if (!SeenServ.enable && cmdparams->channel == NULL && cmdparams->source->user->ulevel < NS_ULEVEL_LOCOPER)
		return NS_SUCCESS;
	if (!SeenServ.enableseenchan && cmdparams->channel != NULL && cmdparams->source->user->ulevel < NS_ULEVEL_LOCOPER)
		return NS_SUCCESS;
	if (ValidateNick(cmdparams->av[0]) == NS_SUCCESS) 
	{
		u = FindUser(cmdparams->av[0]);
		if (u) 
		{
			seen_report( cmdparams, "%s (%s@%s) is connected right now", u->name, u->user->username, u->user->vhost);
			return NS_SUCCESS;
		}
	}
	if (checktype == SS_CHECK_NICK) 
	{
		if (ValidateNick(cmdparams->av[0]) == NS_FAILURE) 
		{
			seen_report( cmdparams, "%s is not a valid nickname", cmdparams->av[0] );
			return NS_SUCCESS;
		}
	}
	for ( d = 0 ; d < 5 ; d++ ) {
		seenentrynick[d][0] = '\0';
		if (d < 4)
			timetext[d][0] = '\0';
	}
	currentlyconnectedtext[0] = '\0';
	h = m = s = seenentriesfound = 0;
	if (checktype == SS_CHECK_WILDCARD) 
	{
		if ( strchr( cmdparams->av[0], '*' ) )
			ircsnprintf(matchstr, USERHOSTLEN, "%s", cmdparams->av[0]);
		else
			ircsnprintf(matchstr, USERHOSTLEN, "*%s*", cmdparams->av[0]);
	}
	ln = list_last(seenlist);
	while (ln != NULL && seenentriesfound < 5) 
	{
		matchfound = 0;
		sd = lnode_get(ln);
		if (checktype == SS_CHECK_NICK) 
		{
			if (!ircstrcasecmp(cmdparams->av[0], sd->nick))
				matchfound = 1;
		} else if (checktype == SS_CHECK_WILDCARD) {
			if ( ( match(matchstr, sd->userhost) && cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER ) || match(matchstr, sd->uservhost) )
				matchfound = 1;
		}
		if (matchfound) 
		{
			if (!seenentriesfound) 
			{
				sdo = sd;
				if (checktype == SS_CHECK_NICK)
					seenentriesfound = 4;
				else
					strlcpy(seenentrynick[seenentriesfound], sd->nick, MAXNICK+3);
			} else {
				ircsnprintf(seenentrynick[seenentriesfound], MAXNICK+3, ", %s", sd->nick);
			}
			seenentriesfound++;
		}
		ln = list_prev(seenlist, ln);
	}
	if (seenentriesfound) 
	{
		d = (me.now - sdo->seentime);
		if (d > 0) 
		{
			s = (d % 60);
			d -= s;
			d = (d / 60);
			m = (d % 60);
			d -= m;
			d = (d / 60);
			h = (d % 24);
			d -= h;
			d = (d / 24);
			if (d)
				ircsnprintf(timetext[0], 12, "%d Days ", d);
			if (h)
				ircsnprintf(timetext[1], 12, "%d Hours ", h);
			if (m)
				ircsnprintf(timetext[2], 12, "%d Minutes ", m);
			if (s)
				ircsnprintf(timetext[3], 12, "%d Seconds", s);
			ircsnprintf(combinedtimetext, SS_GENCHARLEN, "%s%s%s%s", timetext[0], timetext[1], timetext[2], timetext[3]);
		} else {
			ircsnprintf(combinedtimetext, SS_GENCHARLEN, "0 Seconds");
		}
		matchednickstr[0] = '\0';
		if (checktype == SS_CHECK_WILDCARD && seenentriesfound > 1)
			ircsnprintf(matchednickstr, SS_MESSAGESIZE, "The %d most recent matches are - %s%s%s%s%s : ", seenentriesfound, seenentrynick[0], seenentrynick[1], seenentrynick[2], seenentrynick[3], seenentrynick[4]);
		switch( sdo->seentype )
		{
			case SS_CONNECTED:
				u = FindUser(sdo->nick);
				if (u) 
				{
					if (!ircstrcasecmp(sdo->userhost, u->user->userhostmask))
						ircsnprintf(currentlyconnectedtext, SS_GENCHARLEN, ", %s is currently connected", u->name);
				}
				if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL)
					irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen connecting %s ago%s", matchednickstr, sdo->userhost, combinedtimetext, currentlyconnectedtext);
				else
					seen_report( cmdparams, "%s%s was last seen connecting %s ago%s", matchednickstr, sdo->nick, combinedtimetext, currentlyconnectedtext );
				break;
			case SS_QUIT:
				if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL)
					irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen quiting %s ago, stating %s", matchednickstr, sdo->userhost, combinedtimetext, sdo->message);
				else
					seen_report( cmdparams, "%s%s was last seen quiting %s ago, stating %s", matchednickstr, sdo->uservhost, combinedtimetext, sdo->message);
				break;
			case SS_KILLED:
				if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL)
					irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen being killed %s ago %s", matchednickstr, sdo->userhost, combinedtimetext, sdo->message);
				else
					seen_report( cmdparams, "%s%s was last seen being killed %s ago %s", matchednickstr, sdo->uservhost, combinedtimetext, sdo->message );
				break;
			case SS_NICKCHANGE:
				u = FindUser(sdo->message);
				if (u) 
				{
					if (!ircstrcasecmp(sdo->userhost, u->user->userhostmask))
						ircsnprintf(currentlyconnectedtext, SS_GENCHARLEN, ", %s is currently connected", u->name);
				}
				if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL)
					irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen changing Nickname %s ago to %s%s", matchednickstr, sdo->userhost, combinedtimetext, sdo->message, currentlyconnectedtext);
				else
					seen_report( cmdparams, "%s%s was last seen changing Nickname %s ago to %s%s", matchednickstr, sdo->uservhost, combinedtimetext, sdo->message, currentlyconnectedtext );
				break;
			case SS_JOIN:
				u = FindUser(sdo->nick);
				if (u) 
				{
					if (!ircstrcasecmp(sdo->userhost, u->user->userhostmask)) 
					{
						c = FindChannel(sdo->message);
						if (c) 
						{
							if (IsChannelMember(c, u) && !is_hidden_chan(c))
								ircsnprintf(currentlyconnectedtext, SS_GENCHARLEN, ", %s is currently in %s", u->name, c->name);
						}
					}
				}
				if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL)
					irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen Joining %s %s ago%s", matchednickstr, sdo->userhost, sdo->message, combinedtimetext, currentlyconnectedtext);
				else
					seen_report( cmdparams, "%s%s was last seen Joining %s %s ago%s", matchednickstr, sdo->uservhost, sdo->message, combinedtimetext, currentlyconnectedtext );
				break;
			case SS_PART:
				if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL)
					irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen Parting %s %s ago", matchednickstr, sdo->userhost, sdo->message, combinedtimetext);
				else
					seen_report( cmdparams, "%s%s was last seen Parting %s %s ago", matchednickstr, sdo->uservhost, sdo->message, combinedtimetext );
				break;
			case SS_KICKED:
				if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL)
					irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen being Kicked From %s %s ago", matchednickstr, sdo->userhost, sdo->message, combinedtimetext);
				else
					seen_report( cmdparams, "%s%s was last seen Kicked From %s %s", matchednickstr, sdo->uservhost, sdo->message, combinedtimetext);
				break;
		}
	} else if (checktype == SS_CHECK_NICK) {
		seen_report( cmdparams, "Sorry %s, I can't remember seeing anyone called %s", cmdparams->source->name, cmdparams->av[0] );
	} else if (checktype == SS_CHECK_WILDCARD) {
		seen_report( cmdparams, "Sorry %s, I can't remember seeing anyone matching that mask (%s)", cmdparams->source->name, matchstr );
	}
	return NS_SUCCESS;
}

/*
 * Delete all matching entries
*/
int sns_cmd_del(CmdParams *cmdparams) 
{
	lnode_t *ln, *ln2;
	SeenData *sd;
	int i;
	
	SET_SEGV_LOCATION();
	i = 0;
	ln = list_first(seenlist);
	while (ln != NULL) 
	{
		sd = lnode_get(ln);
		if (match(cmdparams->av[0], sd->userhost) || match(cmdparams->av[0], sd->uservhost)) 
		{
			DBADelete( "seendata", sd->nick);
			i++;
			ns_free(sd);
			ln2 = list_next(seenlist, ln);
			list_delete(seenlist, ln);
			lnode_destroy(ln);
			ln = ln2;
		} else {
			ln = list_next(seenlist, ln);
		}
	}
	seen_report( cmdparams, "%d matching entries deleted", i );
	return NS_SUCCESS;
}

/*
 * Display Seen Statistics
*/
int sns_cmd_status(CmdParams *cmdparams)
{
	int seenstats[SEEN_TYPE_MAX];
	lnode_t *ln;
	SeenData *sd;

	SET_SEGV_LOCATION();
	os_memset( seenstats, 0, sizeof( seenstats ) );
	ln = list_first(seenlist);
	while (ln) 
	{
		sd = lnode_get(ln);
		seenstats[ sd->seentype ]++;
		ln = list_next(seenlist, ln);
	}
	seen_report( cmdparams, "Seen Statistics (Current Records Per Type)" );
	seen_report( cmdparams, "%d Connections", seenstats[SS_CONNECTED] );
	seen_report( cmdparams, "%d Quits", seenstats[SS_QUIT] );
	seen_report( cmdparams, "%d Kills", seenstats[SS_KILLED] );
	seen_report( cmdparams, "%d Nick Changes", seenstats[SS_NICKCHANGE] );
	seen_report( cmdparams, "%d Channel Joins", seenstats[SS_JOIN] );
	seen_report( cmdparams, "%d Channel Parts", seenstats[SS_PART] );
	seen_report( cmdparams, "%d Channel Kicks", seenstats[SS_KICKED] );
	seen_report( cmdparams, "End Of Statistics");
	return NS_SUCCESS;
}
