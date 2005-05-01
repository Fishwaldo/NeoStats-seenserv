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

static char ttxt[4][12];
static char dt[SS_GENCHARLEN];
static char matchstr[USERHOSTLEN];
static char cc[SS_GENCHARLEN];
static char senf[5][MAXNICK+3];
static char nickstr[SS_MESSAGESIZE];

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
	const SeenData *sd = key1;
	return( ircstrcasecmp( sd->nick,( char * )key2 ) );
}

/*
 *  Removes SeenData for nickname if exists
*/
void removepreviousnick(char *nn)
{
	lnode_t *ln;
	SeenData *sd;

	ln = lnode_find( seenlist, nn, findnick );
	if( ln )
	{
		sd = lnode_get(ln);
		DBADelete( "seendata", sd->nick);
		ns_free(sd);
		list_delete(seenlist, ln);
		lnode_destroy(ln);
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
	int current;
	int maxage;
	lnode_t *ln, *ln2;
	SeenData *sd;

	current = list_count(seenlist);
	maxage = me.now - ( SeenServ.expiretime * 86400 );
	ln = list_first( seenlist );
	sd = lnode_get( ln );
	while( ( current > SeenServ.maxentries ) || ( SeenServ.expiretime > 0 && ( maxage > sd->seentime ) ) )
	{
		ln2 = list_next( seenlist, ln );
		DBADelete( "seendata", sd->nick );
		ns_free( sd );
		list_delete( seenlist, ln );
		lnode_destroy( ln );
		sd = lnode_get( ln2 );
		current --;
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
	 * lnode_destroy_auto doesn't seem to free the memory
	 * (could be wrong here, but memory usage stayed up on
	 * unloading module, and increased even more when loading)
	 * so loop through and make sure memory is free.
	 * since it uses lots of memory for big lists.
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
int sns_cmd_seenhost(CmdParams *cmdparams) {
	return CheckSeenData(cmdparams, SS_CHECK_WILDCARD);
}

/*
 * Seen for valid nickname
*/
int sns_cmd_seennick(CmdParams *cmdparams) {
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
int CheckSeenData(CmdParams *cmdparams, int checktype) {
	lnode_t *ln;
	SeenData *sd;
	SeenData *sdo;
	Client *u;
	Channel *c;
	int d, h, m, s, mf, sef;
	
	if ( SeenServ.verbose == 1 ) {
		if (checktype == SS_CHECK_NICK) {
			irc_chanalert (sns_bot, "SeenNick Command used by %s (SEENNICK %s)", cmdparams->source->name, cmdparams->av[0]);
		} else if (checktype == SS_CHECK_WILDCARD) {
			irc_chanalert (sns_bot, "Seen Command used by %s (SEEN %s)", cmdparams->source->name, cmdparams->av[0]);
		}
	}
	if (!SeenServ.enable && cmdparams->channel == NULL && cmdparams->source->user->ulevel < NS_ULEVEL_LOCOPER) {
		return NS_SUCCESS;
	}
	if (!SeenServ.enableseenchan && cmdparams->channel != NULL && cmdparams->source->user->ulevel < NS_ULEVEL_LOCOPER) {
		return NS_SUCCESS;
	}
	if (ValidateNick(cmdparams->av[0]) == NS_SUCCESS) {
		u = FindUser(cmdparams->av[0]);
		if (u) {
			if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL) {
				irc_prefmsg (sns_bot, cmdparams->source, "%s (%s@%s) is connected right now", u->name, u->user->username, u->user->hostname);
			} else {
				seen_report( cmdparams, "%s (%s@%s) is connected right now", u->name, u->user->username, u->user->vhost);
			}
			return NS_SUCCESS;
		}
	}
	if (checktype == SS_CHECK_NICK) {
		if (ValidateNick(cmdparams->av[0]) == NS_FAILURE) {
			seen_report( cmdparams, "%s is not a valid nickname", cmdparams->av[0] );
			return NS_SUCCESS;
		}
	}
	for ( d = 0 ; d < 5 ; d++ ) {
		senf[d][0] = '\0';
		if (d < 4) {
			ttxt[d][0] = '\0';
		}
	}
	cc[0] = '\0';
	h = m = s = sef = 0;
	if (checktype == SS_CHECK_WILDCARD) {
		if (! ( strchr( cmdparams->av[0], '*' ) == NULL ) ) {
			ircsnprintf(matchstr, USERHOSTLEN, "%s", cmdparams->av[0]);
		} else {
			ircsnprintf(matchstr, USERHOSTLEN, "*%s*", cmdparams->av[0]);
		}
	}
	ln = list_last(seenlist);
	while (ln != NULL && sef < 5) {
		mf = 0;
		sd = lnode_get(ln);
		if (checktype == SS_CHECK_NICK) {
			if (!ircstrcasecmp(cmdparams->av[0], sd->nick)) {
				mf = 1;
			}
		} else if (checktype == SS_CHECK_WILDCARD) {
			if ( ( match(matchstr, sd->userhost) && cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER ) || match(matchstr, sd->uservhost) ) {
				mf = 1;
			}
		}
		if (mf) {
			if (!sef) {
				sdo = ns_calloc( sizeof( SeenData ) );
				os_memcpy( sdo, sd, sizeof( SeenData ) );
				if (checktype == SS_CHECK_NICK) {
					sef = 4;
				} else {
					strlcpy(senf[sef], sd->nick, MAXNICK+3);
				}
			} else {
				ircsnprintf(senf[sef], MAXNICK+3, ", %s", sd->nick);
			}
			sef++;
		}
		ln = list_prev(seenlist, ln);
	}
	if (sef) {
		d = (me.now - sdo->seentime);
		if (d > 0) {
			s = (d % 60);
			if (s) {
				ircsnprintf(ttxt[3], 12, "%d Seconds", s);
				d -= s;
			}
			d = (d / 60);
			m = (d % 60);
			if (m) {
				ircsnprintf(ttxt[2], 12, "%d Minutes ", m);
				d -= m;
			}
			d = (d / 60);
			h = (d % 24);
			if (h) {
				ircsnprintf(ttxt[1], 12, "%d Hours ", h);
				d -= h;
			}
			d = (d / 24);
			if (d) {
				ircsnprintf(ttxt[0], 12, "%d Days ", d);
			}
		} else {
			ircsnprintf(ttxt[3], 12, "0 Seconds");
		}
		ircsnprintf(dt, SS_GENCHARLEN, "%s%s%s%s", ttxt[0], ttxt[1], ttxt[2], ttxt[3]);
		if (checktype == SS_CHECK_NICK || sef == 1) {
			nickstr[0] = '\0';
		} else if (checktype == SS_CHECK_WILDCARD) {
			ircsnprintf(nickstr, SS_MESSAGESIZE, "The %d most recent matches are - %s%s%s%s%s : ", sef, senf[0], senf[1], senf[2], senf[3], senf[4]);
		}
		if ( sdo->seentype == SS_CONNECTED ) {
			u = FindUser(sdo->nick);
			if (u) {
				if (!ircstrcasecmp(sdo->userhost, u->user->userhostmask)) {
					ircsnprintf(cc, SS_GENCHARLEN, ", %s is currently connected", u->name);
				}
			}
			if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL) {
				irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen connecting %s ago%s", nickstr, sdo->userhost, dt, cc);
			} else {
				seen_report( cmdparams, "%s%s was last seen connecting %s ago%s", nickstr, sdo->nick, dt, cc );
			}
		} else if ( sdo->seentype == SS_QUIT ) {
			if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL) {
				irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen quiting %s ago, stating %s", nickstr, sdo->userhost, dt, sdo->message);
			} else {
				seen_report( cmdparams, "%s%s was last seen quiting %s ago, stating %s", nickstr, sdo->uservhost, dt, sdo->message);
			}
		} else if ( sdo->seentype == SS_KILLED ) {
			if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL) {
				irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen being killed %s ago %s", nickstr, sdo->userhost, dt, sdo->message);
			} else {
				seen_report( cmdparams, "%s%s was last seen being killed %s ago %s", nickstr, sdo->uservhost, dt, sdo->message );
			}
		} else if ( sdo->seentype == SS_NICKCHANGE ) {
			u = FindUser(sdo->message);
			if (u) {
				if (!ircstrcasecmp(sdo->userhost, u->user->userhostmask)) {
					ircsnprintf(cc, SS_GENCHARLEN, ", %s is currently connected", u->name);
				}
			}
			if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL) {
				irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen changing Nickname %s ago to %s%s", nickstr, sdo->userhost, dt, sdo->message, cc);
			} else {
				seen_report( cmdparams, "%s%s was last seen changing Nickname %s ago to %s%s", nickstr, sdo->uservhost, dt, sdo->message, cc );
			}
		} else if ( sdo->seentype == SS_JOIN ) {
			u = FindUser(sdo->nick);
			if (u) {
				if (!ircstrcasecmp(sdo->userhost, u->user->userhostmask)) {
					c = FindChannel(sdo->message);
					if (c) {
						if (IsChannelMember(c, u) && !is_hidden_chan(c)) {
							ircsnprintf(cc, SS_GENCHARLEN, ", %s is currently in %s", u->name, c->name);
						}
					}
				}
			}
			if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL) {
				irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen Joining %s %s ago%s", nickstr, sdo->userhost, sdo->message, dt, cc);
			} else {
				seen_report( cmdparams, "%s%s was last seen Joining %s %s ago%s", nickstr, sdo->uservhost, sdo->message, dt, cc );
			}
		} else if ( sdo->seentype == SS_PART ) {
			if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL) {
				irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen Parting %s %s ago", nickstr, sdo->userhost, sdo->message, dt);
			} else {
				seen_report( cmdparams, "%s%s was last seen Parting %s %s ago", nickstr, sdo->uservhost, sdo->message, dt );
			}
		} else if ( sdo->seentype == SS_KICKED ) {
			if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL) {
				irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen being Kicked From %s %s ago", nickstr, sdo->userhost, sdo->message, dt);
			} else {
				seen_report( cmdparams, "%s%s was last seen Kicked From %s %s", nickstr, sdo->uservhost, sdo->message, dt);
			}
		}
		ns_free(sdo);
		return NS_SUCCESS;
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
int sns_cmd_del(CmdParams *cmdparams) {
	lnode_t *ln, *ln2;
	SeenData *sd;
	int i;
	
	if ( SeenServ.verbose == 1 ) {
		irc_chanalert (sns_bot, "Delete Command used by %s (DEL %s)", cmdparams->source->name, cmdparams->av[0]);
	}
	i = 0;
	ln = list_first(seenlist);
	while (ln != NULL) {
		sd = lnode_get(ln);
		if (match(cmdparams->av[0], sd->userhost) || match(cmdparams->av[0], sd->uservhost)) {
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
	if ( SeenServ.verbose == 1 ) {
		irc_chanalert (sns_bot, "%d matching entries deleted", i);
	}
	return NS_SUCCESS;
}

/*
 * Display Seen Statistics
*/
int sns_cmd_status(CmdParams *cmdparams) {
	lnode_t *ln;
	SeenData *sd;
	int sc[10], i;

	if ( SeenServ.verbose == 1 ) {
		irc_chanalert (sns_bot, "Stats Command used by %s (STATS)", cmdparams->source->name);
	}
	for ( i = 0 ; i < 10 ; i++ ) {
		sc[i] = 0;
	}
	ln = list_first(seenlist);
	while (ln != NULL) {
		sd = lnode_get(ln);
		sc[sd->seentype]++;
		ln = list_next(seenlist, ln);
	}
	seen_report( cmdparams, "Seen Statistics (Current Records Per Type)");
	seen_report( cmdparams, "%d Connections", sc[SS_CONNECTED]);
	seen_report( cmdparams, "%d Quits", sc[SS_QUIT]);
	seen_report( cmdparams, "%d Kills", sc[SS_KILLED]);
	seen_report( cmdparams, "%d Nick Changes", sc[SS_NICKCHANGE]);
	seen_report( cmdparams, "%d Channel Joins", sc[SS_JOIN]);
	seen_report( cmdparams, "%d Channel Parts", sc[SS_PART]);
	seen_report( cmdparams, "%d Channel Kicks", sc[SS_KICKED]);
	seen_report( cmdparams, "End Of Statistics");
	return NS_SUCCESS;
}
