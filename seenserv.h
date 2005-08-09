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

/* Defines */
typedef enum SEEN_CHECK
{
	SS_CHECK_WILDCARD,	/* WildCard Entry Check */
	SS_CHECK_NICK,		/* Nick Entry Check */
	SEEN_CHECK_TYPE_MAX,
} SEEN_CHECK;

typedef enum SEEN_TYPE
{
	SS_CONNECTED,		/* Seen Connection Type */
	SS_QUIT,		/* Seen Quit Type */
	SS_KILLED,		/* Seen Killed Type */
	SS_NICKCHANGE,		/* Seen Nick Change Type */
	SS_JOIN,		/* Seen Join Channel Type */
	SS_PART,		/* Seen Part Channel Type */
	SS_KICKED,		/* Seen Kicked Channel Type */
	SEEN_TYPE_MAX,
} SEEN_TYPE;

typedef enum SEEN_LISTLIMIT
{
	SS_LISTLIMIT_COUNT,	/* Check List Limit by Record Count */
	SS_LISTLIMIT_AGE,	/* Check List Limit by Record Age */
	SEEN_LISTLIMIT_MAX,
} SEEN_LISTLIMIT;

#define SS_MESSAGESIZE	300	/* Message Field Size */
#define SS_GENCHARLEN	128	/* General Character Field Length */

/* Variables And Structs */
Bot *sns_bot;

struct SeenServ {
	int exclusions;
	int enable;
	int enableseenchan;
	char seenchan[MAXCHANLEN];
	int maxentries;
	int eventsignon;
	int eventquit;
	int eventkill;
	int eventnick;
	int eventjoin;
	int eventpart;
	int eventkick;
	int expiretime;
	int dbupdatetime;
	int memorylist;
} SeenServ;

typedef struct SeenData {
	char nick[MAXNICK];
	char userhost[USERHOSTLEN];
	char uservhost[USERHOSTLEN];
	char message[SS_MESSAGESIZE];
	SEEN_TYPE seentype;
	time_t seentime;
	int recordsaved;
} SeenData;

/* SeenServ Module Help - seenserv_help.c */
extern const char *sns_help_set_exclusions[];
extern const char *sns_help_set_enable[];
extern const char *sns_help_set_enableseenchan[];
extern const char *sns_help_set_seenchan[];
extern const char *sns_help_set_maxentries[];
extern const char *sns_help_set_eventsignon[];
extern const char *sns_help_set_eventquit[];
extern const char *sns_help_set_eventkill[];
extern const char *sns_help_set_eventnick[];
extern const char *sns_help_set_eventjoin[];
extern const char *sns_help_set_eventpart[];
extern const char *sns_help_set_eventkick[];
extern const char *sns_help_set_expiretime[];
extern const char *sns_help_set_dbupdatetime[];
extern const char *sns_help_set_memorylist[];
extern const char *sns_help_seen[];
extern const char *sns_help_seennick[];
extern const char *sns_help_del[];
extern const char *sns_help_status[];

/* events.c */
int SeenSignon (CmdParams *cmdparams);
int SeenQuit (CmdParams *cmdparams);
int SeenKill (CmdParams *cmdparams);
int SeenNickChange (CmdParams *cmdparams);
int SeenJoinChan (CmdParams *cmdparams);
int SeenPartChan (CmdParams *cmdparams);
int SeenKicked (CmdParams *cmdparams);

/* seenserv.c */
int removeagedseenrecords(void *);

/* seen.c */
int removepreviousnick(char *nick);
void addseenentry(char *nick, char *host, char *vhost, char *message, int type);
int dbsavetimer(void *);
void checkseenlistlimit(int checktype);
void createseenlist(void);
void loadseendata(void);
int sortlistbytime(const void *key1, const void *key2);
void destroyseenlist(void);
int sns_cmd_seenhost(CmdParams *cmdparams);
int sns_cmd_seennick(CmdParams *cmdparams);
int CheckSeenData(CmdParams *cmdparams, SEEN_CHECK checktype);
int sns_cmd_del(CmdParams *cmdparams);
int sns_cmd_status(CmdParams *cmdparams);
