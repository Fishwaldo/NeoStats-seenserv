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

static list_t *seenlist;

static char ttxt[4][12];
static char th[SS_HOSTSIZE];
static char dt[SS_GENCHARLEN];
static char matchstr[SS_HOSTSIZE];
static char cc[SS_GENCHARLEN];
static char senf[5][MAXNICK+3];
static char nickstr[SS_MESSAGESIZE];

/*
 *  add new seen entry to list
*/
void addseenentry(char *nick, char *user, char *host, char *vhost, char *message, int type) {
	SeenData *sd;
	
	removepreviousnick(nick);
	sd = ns_calloc(sizeof(SeenData));
	strlcpy(sd->nick, nick, MAXNICK);
	ircsnprintf(sd->userhost, SS_HOSTSIZE, "%s!%s@%s", nick, user, host);
	ircsnprintf(sd->uservhost, SS_HOSTSIZE, "%s!%s@%s", nick, user, vhost);
	strlcpy(sd->message, message, SS_MESSAGESIZE);
	sd->seentype = type;
	sd->seentime = me.now;
	lnode_create_append( seenlist, sd );
	DBAStore( "seendata", sd->nick,( void * )sd, sizeof( SeenData ) );
	checkseenlistlimit();
}

/*
 *  Removes SeenData if records past max entries setting
*/
void checkseenlistlimit(void) {
	lnode_t *ln;
	SeenData *sd;

	while (list_count(seenlist) > SeenServ.maxseenentries) {
		ln = list_first(seenlist);
		sd = lnode_get(ln);
		DBADelete( "seendata", sd->nick);
		ns_free(sd);
		list_delete(seenlist, ln);
		lnode_destroy(ln);
	}
}

/*
 *  Removes SeenData for nickname if exists
*/
void removepreviousnick(char *nn) {
	lnode_t *ln;
	SeenData *sd;

	ln = list_first(seenlist);
	while (ln != NULL) {
		sd = lnode_get(ln);
		if (!ircstrcasecmp(sd->nick, nn)) {
			DBADelete( "seendata", sd->nick);
			ns_free(sd);
			list_delete(seenlist, ln);
			lnode_destroy(ln);
			break;
		} else {
			ln = list_next(seenlist, ln);
		}
	}
}

/*
 * Load Saved Seen Records
*/
void loadseendata(void) {
	seenlist = list_create( -1 );
	DBAFetchRows( "seendata", loadseenrecords );
	list_sort( seenlist, sortlistbytime );
}
int loadseenrecords(void *data)
{
	SeenData *sd;

	sd = ns_calloc( sizeof( SeenData ) );
	os_memcpy( sd, data, sizeof( SeenData ) );
	lnode_create_append( seenlist, sd );
	return NS_FALSE;
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
void destroyseenlist(void) {
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
				if (cmdparams->channel == NULL) {
					irc_prefmsg (sns_bot, cmdparams->source, "%s (%s@%s) is connected right now", u->name, u->user->username, u->user->vhost);
				} else {
					irc_chanprivmsg (sns_bot, cmdparams->channel->name, "%s (%s@%s) is connected right now", u->name, u->user->username, u->user->vhost);
				}
			}
		}
		return NS_SUCCESS;
	}
	if (checktype == SS_CHECK_NICK) {
		if (ValidateNick(cmdparams->av[0]) == NS_FAILURE) {
			if (cmdparams->channel == NULL) {
				irc_prefmsg (sns_bot, cmdparams->source, "%s is not a valid nickname", cmdparams->av[0]);
			} else {
				irc_chanprivmsg (sns_bot, cmdparams->channel->name, "%s is not a valid nickname", cmdparams->av[0]);
			}
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
		if (!strchr(cmdparams->av[0], '*') == NULL) {
			ircsnprintf(matchstr, SS_HOSTSIZE, "%s", cmdparams->av[0]);
		} else {
			ircsnprintf(matchstr, SS_HOSTSIZE, "*%s*", cmdparams->av[0]);
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
		if (checktype == SS_CHECK_NICK) {
			nickstr[0] = '\0';
		} else if (checktype == SS_CHECK_WILDCARD) {
			ircsnprintf(nickstr, SS_MESSAGESIZE, "The %d most recent matches are: %s%s%s%s%s , ", sef, senf[0], senf[1], senf[2], senf[3], senf[4]);
		}
		if ( sdo->seentype == SS_CONNECTED ) {
			u = FindUser(sdo->nick);
			if (u) {
				ircsnprintf(th, SS_HOSTSIZE, "%s!%s@%s", u->name, u->user->username, u->user->hostname);
				if (!ircstrcasecmp(sdo->userhost, th)) {
					ircsnprintf(cc, SS_GENCHARLEN, ", %s is currently connected", u->name);
				}
			}
			if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL) {
				irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen connecting %s ago%s", nickstr, sdo->userhost, dt, cc);
			} else {
				if (cmdparams->channel == NULL) {
					irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen connecting %s ago%s", nickstr, sdo->uservhost, dt, cc);
				} else {
					irc_chanprivmsg (sns_bot, cmdparams->channel->name, "%s%s was last seen connecting %s ago%s", nickstr, sdo->uservhost, dt, cc);
				}
			}
		} else if ( sdo->seentype == SS_QUIT ) {
			if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL) {
				irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen quiting %s ago, stating %s", nickstr, sdo->userhost, dt, sdo->message);
			} else {
				if (cmdparams->channel == NULL) {
					irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen quiting %s ago, stating %s", nickstr, sdo->uservhost, dt, sdo->message);
				} else {
					irc_chanprivmsg (sns_bot, cmdparams->channel->name, "%s%s was last seen quiting %s ago, stating %s", nickstr, sdo->uservhost, dt, sdo->message);
				}
			}
		} else if ( sdo->seentype == SS_KILLED ) {
			if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL) {
				irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen being killed %s ago %s", nickstr, sdo->userhost, dt, sdo->message);
			} else {
				if (cmdparams->channel == NULL) {
					irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen being killed %s ago %s", nickstr, sdo->uservhost, dt, sdo->message);
				} else {
					irc_chanprivmsg (sns_bot, cmdparams->channel->name, "%s%s was last seen being killed %s ago %s", nickstr, sdo->uservhost, dt, sdo->message);
				}
			}
		} else if ( sdo->seentype == SS_NICKCHANGE ) {
			u = FindUser(sdo->message);
			if (u) {
				ircsnprintf(th, SS_HOSTSIZE, "%s!%s@%s", u->name, u->user->username, u->user->hostname);
				if (!ircstrcasecmp(sdo->userhost, th)) {
					ircsnprintf(cc, SS_GENCHARLEN, ", %s is currently connected", u->name);
				}
			}
			if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL) {
				irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen changing Nickname %s ago to %s%s", nickstr, sdo->userhost, dt, sdo->message, cc);
			} else {
				if (cmdparams->channel == NULL) {
					irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen changing Nickname %s ago to %s%s", nickstr, sdo->uservhost, dt, sdo->message, cc);
				} else {
					irc_chanprivmsg (sns_bot, cmdparams->channel->name, "%s%s was last seen changing Nickname %s ago to %s%s", nickstr, sdo->uservhost, dt, sdo->message, cc);
				}
			}
		} else if ( sdo->seentype == SS_JOIN ) {
			u = FindUser(sdo->nick);
			if (u) {
				ircsnprintf(th, SS_HOSTSIZE, "%s!%s@%s", u->name, u->user->username, u->user->hostname);
				if (!ircstrcasecmp(sdo->userhost, th)) {
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
				if (cmdparams->channel == NULL) {
					irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen Joining %s %s ago%s", nickstr, sdo->uservhost, sdo->message, dt, cc);
				} else {
					irc_chanprivmsg (sns_bot, cmdparams->channel->name, "%s%s was last seen Joining %s %s ago%s", nickstr, sdo->uservhost, sdo->message, dt, cc);
				}
			}
		} else if ( sdo->seentype == SS_PART ) {
			if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL) {
				irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen Parting %s %s ago", nickstr, sdo->userhost, sdo->message, dt);
			} else {
				if (cmdparams->channel == NULL) {
					irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen Parting %s %s ago", nickstr, sdo->uservhost, sdo->message, dt);
				} else {
					irc_chanprivmsg (sns_bot, cmdparams->channel->name, "%s%s was last seen Parting %s %s ago", nickstr, sdo->uservhost, sdo->message, dt);
				}
			}
		} else if ( sdo->seentype == SS_KICKED ) {
			if (cmdparams->source->user->ulevel >= NS_ULEVEL_LOCOPER && cmdparams->channel == NULL) {
				irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen being Kicked From %s %s ago", nickstr, sdo->userhost, sdo->message, dt);
			} else {
				if (cmdparams->channel == NULL) {
					irc_prefmsg (sns_bot, cmdparams->source, "%s%s was last seen Kicked From %s %s", nickstr, sdo->uservhost, sdo->message, dt);
				} else {
					irc_chanprivmsg (sns_bot, cmdparams->channel->name, "%s%s was last seen Kicked From %s %s", nickstr, sdo->uservhost, sdo->message, dt);
				}
			}
		}
		ns_free(sdo);
		return NS_SUCCESS;
	} else if (checktype == SS_CHECK_NICK) {
		if (cmdparams->channel == NULL) {
			irc_prefmsg (sns_bot, cmdparams->source, "Sorry %s, I can't remember seeing anyone called %s", cmdparams->source->name, cmdparams->av[0]);
		} else {
			irc_chanprivmsg (sns_bot, cmdparams->channel->name, "Sorry %s, I can't remember seeing anyone called %s", cmdparams->source->name, cmdparams->av[0]);
		}
	} else if (checktype == SS_CHECK_WILDCARD) {
		if (cmdparams->channel == NULL) {
			irc_prefmsg (sns_bot, cmdparams->source, "Sorry %s, I can't remember seeing anyone matching that mask (%s)", cmdparams->source->name, matchstr);
		} else {
			irc_chanprivmsg (sns_bot, cmdparams->channel->name, "Sorry %s, I can't remember seeing anyone matching that mask (%s)", cmdparams->source->name, matchstr);
		}
	}
	return NS_SUCCESS;
}

/*
 * Remove all matching entries
*/
int sns_cmd_remove(CmdParams *cmdparams) {
	lnode_t *ln, *ln2;
	SeenData *sd;
	int i;
	
	if ( SeenServ.verbose == 1 ) {
		irc_chanalert (sns_bot, "Remove Command used by %s (REMOVE %s)", cmdparams->source->name, cmdparams->av[0]);
	}
	i = 0;
	ln = list_first(seenlist);
	while (ln != NULL) {
		sd = lnode_get(ln);
		if (match(cmdparams->av[0], sd->userhost) || match(cmdparams->av[0], sd->uservhost)) {
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
	if (cmdparams->channel == NULL) {
		irc_prefmsg (sns_bot, cmdparams->source, "%d matching entries removed", i);
	} else {
		irc_chanprivmsg (sns_bot, cmdparams->channel->name, "%d matching entries removed", i);
	}
	if ( SeenServ.verbose == 1 ) {
		irc_chanalert (sns_bot, "%d matching entries removed", i);
	}
	return NS_SUCCESS;
}

/*
 * Display Seen Statistics
*/
int sns_cmd_stats(CmdParams *cmdparams) {
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
	if (cmdparams->channel == NULL) {
		irc_prefmsg (sns_bot, cmdparams->source, "Seen Statistics (Current Records Per Type)");
		irc_prefmsg (sns_bot, cmdparams->source, "%d Connections", sc[SS_CONNECTED]);
		irc_prefmsg (sns_bot, cmdparams->source, "%d Quits", sc[SS_QUIT]);
		irc_prefmsg (sns_bot, cmdparams->source, "%d Kills", sc[SS_KILLED]);
		irc_prefmsg (sns_bot, cmdparams->source, "%d Nick Changes", sc[SS_NICKCHANGE]);
		irc_prefmsg (sns_bot, cmdparams->source, "%d Channel Joins", sc[SS_JOIN]);
		irc_prefmsg (sns_bot, cmdparams->source, "%d Channel Parts", sc[SS_PART]);
		irc_prefmsg (sns_bot, cmdparams->source, "%d Channel Kicks", sc[SS_KICKED]);
		irc_prefmsg (sns_bot, cmdparams->source, "End Of Statistics");
	} else {
		irc_chanprivmsg (sns_bot, cmdparams->channel->name, "Stats command used");
		irc_chanprivmsg (sns_bot, cmdparams->channel->name, "Seen Statistics (Current Records Per Type)");
		irc_chanprivmsg (sns_bot, cmdparams->channel->name, "%d Connections", sc[SS_CONNECTED]);
		irc_chanprivmsg (sns_bot, cmdparams->channel->name, "%d Quits", sc[SS_QUIT]);
		irc_chanprivmsg (sns_bot, cmdparams->channel->name, "%d Kills", sc[SS_KILLED]);
		irc_chanprivmsg (sns_bot, cmdparams->channel->name, "%d Nick Changes", sc[SS_NICKCHANGE]);
		irc_chanprivmsg (sns_bot, cmdparams->channel->name, "%d Channel Joins", sc[SS_JOIN]);
		irc_chanprivmsg (sns_bot, cmdparams->channel->name, "%d Channel Parts", sc[SS_PART]);
		irc_chanprivmsg (sns_bot, cmdparams->channel->name, "%d Channel Kicks", sc[SS_KICKED]);
		irc_chanprivmsg (sns_bot, cmdparams->channel->name, "End Of Statistics");
	}
	return NS_SUCCESS;
}
