/* SeenServ - Nickname Seen Service - NeoStats Addon Module
** Copyright (c) 2003-2005 Justin Hammond, Mark Hetherington, Jeff Lang
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

#define MAX_NICK_HISTORY 5
#define SEEN_ENTRY_NICK_SIZE ( ( MAXNICK + 3 ) * MAX_NICK_HISTORY )

static list_t *seenlist;

static char combinedtimetext[SS_GENCHARLEN];
static char matchstr[USERHOSTLEN];
static char currentlyconnectedtext[SS_GENCHARLEN];
static char seenentrynick[SEEN_ENTRY_NICK_SIZE];
static char matchednickstr[SS_MESSAGESIZE];
static char nicklower[MAXNICK];

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
 *
 * returns NS_SUCCESS if nick removed or NS_FAILURE if nick not in list
*/
int removepreviousnick(char *nick)
{
	lnode_t *ln;
	SeenData *sd;

	ln = list_last( seenlist );
	while ( ln )
	{
		sd = lnode_get( ln );
		if (!ircstrcasecmp(nick, sd->nick)) 
		{
			ns_free( sd );
			list_delete( seenlist, ln );
			lnode_destroy( ln );
			return NS_SUCCESS;
		}
		ln = list_prev( seenlist, ln );
	}
	return NS_FAILURE;
}

/*
 *  add new seen entry to list
*/
void addseenentry(char *nick, char *host, char *vhost, char *message, int type)
{
	SeenData *sd;
	int nickremoved;
	
	nickremoved = removepreviousnick(nick);
	sd = ns_calloc(sizeof(SeenData));
	strlcpy(sd->nick, nick, MAXNICK);
	strlcpy(sd->userhost, host, USERHOSTLEN);
	strlcpy(sd->uservhost, vhost, USERHOSTLEN);
	strlcpy(sd->message, message ? message : "", SS_MESSAGESIZE);
	sd->seentype = type;
	sd->seentime = me.now;
	sd->recordsaved = 0;
	lnode_create_append( seenlist, sd );
	/* only check list limit if the nick wasn't already in the list */
	if( nickremoved == NS_FAILURE )
		checkseenlistlimit(SS_LISTLIMIT_COUNT);
	return;
}

/*
 * Save Data to DB on timer
 * 
 * remove records from list if set to work from DB only
*/
int dbsavetimer(void) 
{
	SET_SEGV_LOCATION();
	lnode_t *ln, *ln2;
	SeenData *sd;
	
	ln = list_last( seenlist );
	while ( ln )
	{
		sd = lnode_get( ln );
		ln2 = list_prev( seenlist, ln );
		if (sd->recordsaved == 0)
		{
			sd->recordsaved = 1;
			strlcpy( nicklower, sd->nick, MAXNICK );
			DBAStore( "seendata", strlwr(nicklower),( void * )sd, sizeof( SeenData ) );
			if( !SeenServ.memorylist )
			{
				ns_free( sd );
				list_delete( seenlist, ln );
				lnode_destroy( ln );
			}
		} else {
			if( !SeenServ.memorylist )
			{
				ns_free( sd );
				list_delete( seenlist, ln );
				lnode_destroy( ln );
			} else {
				return NS_SUCCESS;
			}
		}		
		ln = ln2;
	}
	return NS_SUCCESS;
}

/*
 *  Removes SeenData if records past max entries setting
*/
void checkseenlistlimit(int checktype)
{
	int currentlistcount;
	int maxageallowed;
	lnode_t *ln, *ln2;
	SeenData *sd;

	currentlistcount = list_count(seenlist);
	maxageallowed = me.now - ( SeenServ.expiretime * TS_ONE_DAY );
	ln = list_first( seenlist );
	sd = lnode_get( ln );
	while( ( checktype == SS_LISTLIMIT_COUNT && currentlistcount > SeenServ.maxentries ) || ( checktype == SS_LISTLIMIT_AGE && SeenServ.expiretime > 0 && maxageallowed > sd->seentime ) )
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
	return;
}

/*
 * Load Saved Seen Records
*/
int loadseenrecords(void *data, int size)
{
	SeenData *sd;

	sd = ns_calloc( sizeof( SeenData ) );
	os_memcpy( sd, data, sizeof( SeenData ) );
	sd->recordsaved = 1;
	lnode_create_append( seenlist, sd );
	return NS_FALSE;
}

void createseenlist(void)
{
	seenlist = list_create( -1 );
}

void loadseendata(void)
{
	DBAFetchRows( "seendata", loadseenrecords );
	list_sort( seenlist, sortlistbytime );
	return;
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
 * Check whether we can run the seen command
*/
static int SeenAvailable( CmdParams *cmdparams )
{
	if( cmdparams->source->user->ulevel < NS_ULEVEL_LOCOPER )
	{
		if( !SeenServ.enable && cmdparams->channel == NULL )
			return NS_FALSE;
		if( !SeenServ.enableseenchan && cmdparams->channel != NULL )
			return NS_FALSE;
	}
	return NS_TRUE;
}

/*
 * Seen for wildcarded Host
*/
int sns_cmd_seenhost(CmdParams *cmdparams) 
{
	Client *u;

	SET_SEGV_LOCATION();
	/* do lookup on nick only if working from DB and not memory list */
	if( !SeenServ.memorylist )
		return sns_cmd_seennick(cmdparams);
	if( SeenAvailable( cmdparams ) == NS_FALSE )
		return NS_SUCCESS;
	/* ensure DB is saved before doing lookup */
	dbsavetimer();
	if( ValidateNick( cmdparams->av[0] ) == NS_SUCCESS ) 
	{
		u = FindUser( cmdparams->av[0] );
		if (u) 
		{
			seen_report( cmdparams, "%s (%s@%s) is connected right now", u->name, u->user->username, u->user->vhost);
			return NS_SUCCESS;
		}
	}
	if( CheckSeenData( cmdparams, SS_CHECK_WILDCARD ) == NS_FAILURE )
		seen_report( cmdparams, "Sorry %s, I can't remember seeing anyone matching that mask (%s)", cmdparams->source->name, matchstr );
	return NS_SUCCESS;
}

/*
 * Seen for valid nickname
*/
int sns_cmd_seennick(CmdParams *cmdparams)
{
	Client *u;

	SET_SEGV_LOCATION();
	if( SeenAvailable( cmdparams ) == NS_FALSE )
		return NS_SUCCESS;
	/* ensure DB is saved before doing lookup */
	dbsavetimer();
	if( ValidateNick( cmdparams->av[0] ) == NS_FAILURE ) 
	{
		seen_report( cmdparams, "%s is not a valid nickname", cmdparams->av[0] );
		return NS_SUCCESS;
	}
	u = FindUser( cmdparams->av[0] );
	if (u) 
	{
		seen_report( cmdparams, "%s (%s@%s) is connected right now", u->name, u->user->username, u->user->vhost);
		return NS_SUCCESS;
	}
	if( CheckSeenData( cmdparams, SS_CHECK_NICK ) == NS_FAILURE )
		seen_report( cmdparams, "Sorry %s, I can't remember seeing anyone called %s", cmdparams->source->name, cmdparams->av[0] );
	return NS_SUCCESS;
}

/*
 * Build time string
*/
void BuildTimeString( int ts )
{
	static char temptimetext[12];
	int d, h, m, s;

	combinedtimetext[0] = '\0';
	if (ts > 0) 
	{
		s = ( ts % 60 );
		ts -= s;
		ts = ( ts / 60 );
		m = ( ts % 60 );
		ts -= m;
		ts = ( ts / 60 );
		h = ( ts % 24 );
		ts -= h;
		d = ( ts / 24 );
		if( d )
		{
			ircsnprintf( temptimetext, 12, "%d Days ", d );
			strlcat( combinedtimetext, temptimetext, SS_GENCHARLEN );
		}
		if( h )
		{
			ircsnprintf( temptimetext, 12, "%d Hours ", h );
			strlcat( combinedtimetext, temptimetext, SS_GENCHARLEN );
		}
		if( m )
		{
			ircsnprintf( temptimetext, 12, "%d Minutes ", m );
			strlcat( combinedtimetext, temptimetext, SS_GENCHARLEN );
		}
		if( s )
		{
			ircsnprintf( temptimetext, 12, "%d Seconds", s );
			strlcat( combinedtimetext, temptimetext, SS_GENCHARLEN );
		}
	} else {
		ircsnprintf( combinedtimetext, SS_GENCHARLEN, "0 Seconds" );
	}
}

/*
 * Check For Seen Records
*/
int CheckSeenData(CmdParams *cmdparams, SEEN_CHECK checktype)
{
	lnode_t *ln, *oln[MAX_NICK_HISTORY];
	SeenData *sd, *sdo = NULL;
	Client *u;
	Channel *c;
	int matchfound = 0, seenentriesfound = 0, maxageallowed = 0;
	int isopersource = 0, i;
	
	if( cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER )
		isopersource = 1;
	/* used for expiring records on age if shown in request */
	for( i = 0 ; i < MAX_NICK_HISTORY ; i++ )
		oln[i] = NULL;
	seenentrynick[0] = '\0';
	currentlyconnectedtext[0] = '\0';
	if (checktype == SS_CHECK_WILDCARD) 
	{
		if ( strchr( cmdparams->av[0], '*' ) )
			ircsnprintf(matchstr, USERHOSTLEN, "%s", cmdparams->av[0]);
		else
			ircsnprintf(matchstr, USERHOSTLEN, "*%s*", cmdparams->av[0]);
	}
	if( !SeenServ.memorylist )
	{
		sdo = ns_calloc( sizeof( SeenData ) );
		strlcpy( nicklower, cmdparams->av[0], MAXNICK );
		if( DBAFetch( "seendata", strlwr(nicklower), ( void * )sdo, sizeof( SeenData ) ) != NS_FAILURE )
			seenentriesfound = 1;
	} else {
		ln = list_last(seenlist);
		while( ln != NULL && seenentriesfound < MAX_NICK_HISTORY ) 
		{
			sd = lnode_get(ln);
			if (checktype == SS_CHECK_NICK) 
			{
				if( !ircstrcasecmp( cmdparams->av[0], sd->nick ) )
					matchfound = 1;
			} 
			else if (checktype == SS_CHECK_WILDCARD) 
			{
				if( isopersource )
				{
				if ( match( matchstr, sd->userhost ) )
					matchfound = 1;
			}
			if( match( matchstr, sd->uservhost ) )
				matchfound = 1;
			}
			if (matchfound) 
			{
				oln[seenentriesfound] = ln;
				seenentriesfound++;
				if( seenentriesfound == 1 ) 
				{
					sdo = sd;
					if (checktype == SS_CHECK_NICK)
						break;
					else
						strlcpy( seenentrynick, sd->nick, SEEN_ENTRY_NICK_SIZE );
				} else {
					strlcat( seenentrynick, ", ", SEEN_ENTRY_NICK_SIZE );
					strlcat( seenentrynick, sd->nick, SEEN_ENTRY_NICK_SIZE );
				}
				matchfound = 0;
			}
			ln = list_prev(seenlist, ln);
		}
	}
	if( seenentriesfound == 0 ) 
		return NS_FAILURE;
	BuildTimeString( ( int )( me.now - sdo->seentime ) );
	matchednickstr[0] = '\0';
	if( seenentriesfound > 1 )
		ircsnprintf(matchednickstr, SS_MESSAGESIZE, "The %d most recent matches are - %s : ", seenentriesfound, seenentrynick);
	switch( sdo->seentype )
	{
		case SS_CONNECTED:
			u = FindUser(sdo->nick);
			if (u) 
			{
				if (!ircstrcasecmp(sdo->userhost, u->user->userhostmask))
					ircsnprintf(currentlyconnectedtext, SS_GENCHARLEN, ", %s is currently connected", u->name);
			}
			if( isopersource && cmdparams->channel == NULL )
				irc_prefmsg( sns_bot, cmdparams->source, "%s%s was last seen connecting %s ago%s", matchednickstr, sdo->userhost, combinedtimetext, currentlyconnectedtext );
			else
				seen_report( cmdparams, "%s%s was last seen connecting %s ago%s", matchednickstr, sdo->nick, combinedtimetext, currentlyconnectedtext );
			break;
		case SS_QUIT:
			if( isopersource && cmdparams->channel == NULL )
				irc_prefmsg( sns_bot, cmdparams->source, "%s%s was last seen quiting %s ago, stating %s", matchednickstr, sdo->userhost, combinedtimetext, sdo->message );
			else
				seen_report( cmdparams, "%s%s was last seen quiting %s ago, stating %s", matchednickstr, sdo->uservhost, combinedtimetext, sdo->message);
			break;
		case SS_KILLED:
			if( isopersource && cmdparams->channel == NULL )
				irc_prefmsg( sns_bot, cmdparams->source, "%s%s was last seen being killed %s ago %s", matchednickstr, sdo->userhost, combinedtimetext, sdo->message );
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
			if( isopersource && cmdparams->channel == NULL )
				irc_prefmsg( sns_bot, cmdparams->source, "%s%s was last seen changing Nickname %s ago to %s%s", matchednickstr, sdo->userhost, combinedtimetext, sdo->message, currentlyconnectedtext );
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
			if( isopersource && cmdparams->channel == NULL )
				irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen Joining %s %s ago%s", matchednickstr, sdo->userhost, sdo->message, combinedtimetext, currentlyconnectedtext);
			else
				seen_report( cmdparams, "%s%s was last seen Joining %s %s ago%s", matchednickstr, sdo->uservhost, sdo->message, combinedtimetext, currentlyconnectedtext );
			break;
		case SS_PART:
			if( isopersource && cmdparams->channel == NULL )
				irc_prefmsg( sns_bot, cmdparams->source, "%s%s was last seen Parting %s %s ago", matchednickstr, sdo->userhost, sdo->message, combinedtimetext );
			else
				seen_report( cmdparams, "%s%s was last seen Parting %s %s ago", matchednickstr, sdo->uservhost, sdo->message, combinedtimetext );
			break;
		case SS_KICKED:
			if( isopersource && cmdparams->channel == NULL )
				irc_prefmsg( sns_bot, cmdparams->source, "%s%s was last seen being Kicked From %s %s ago", matchednickstr, sdo->userhost, sdo->message, combinedtimetext );
			else
				seen_report( cmdparams, "%s%s was last seen Kicked From %s %s", matchednickstr, sdo->uservhost, sdo->message, combinedtimetext);
			break;
		default:
			break;
	}
	if( !SeenServ.memorylist )
	{
		/* delete record if DB Only and past expiration date */
		if( SeenServ.expiretime > 0 && ( ( me.now - ( SeenServ.expiretime * TS_ONE_DAY ) ) > sdo->seentime ) )
		{
			DBADelete( "seendata", strlwr(nicklower) );
		}
		ns_free( sdo );
	} else {
		/* expire displayed records on age if required */
		if( SeenServ.expiretime > 0 )
		{
			maxageallowed = me.now - ( SeenServ.expiretime * TS_ONE_DAY );
			i = 0;
			while( i < MAX_NICK_HISTORY && oln[i] != NULL )
			{
				ln = oln[i];
				sd = lnode_get( ln );
				if( maxageallowed > sd->seentime )
				{
					DBADelete( "seendata", sd->nick );
					ns_free( sd );
					list_delete( seenlist, ln );
					lnode_destroy( ln );
				}
				i++;
			}
		}
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
	if( !SeenServ.memorylist )
	{
		seen_report( cmdparams, "Seen Statistics Unavailable when using DB only" );
		return NS_SUCCESS;
	}
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
