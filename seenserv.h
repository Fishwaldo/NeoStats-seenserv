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

Bot *sns_bot;

struct SeenServ {
	int exclusions;
	int enable;
	int enableseenchan;
	char seenchan[MAXCHANLEN];
	int maxseenentries;
} SeenServ;

typedef struct SeenData {
	char nick[MAXNICK];
	char userhost[PARAMSIZE];
	char uservhost[PARAMSIZE];
	char message[BUFSIZE];
	int seentype;
	time_t seentime;
} SeenData;

/* Defines */
#define SS_CONNECTED		0x00000001	/* Seen Connection Type */
#define SS_QUIT			0x00000002	/* Seen Quit Type */
#define SS_KILLED		0x00000003	/* Seen Killed Type */
#define SS_NICKCHANGE		0x00000004	/* Seen Nick Change Type */
#define SS_JOIN			0x00000005	/* Seen Join Channel Type */
#define SS_PART			0x00000006	/* Seen Part Channel Type */
#define SS_KICKED		0x00000007	/* Seen Kicked Channel Type */

/* SeenServ Module Help */
extern const char *sns_help_set_exclusions[];
extern const char *sns_help_set_enable[];
extern const char *sns_help_set_enableseenchan[];
extern const char *sns_help_set_seenchan[];
extern const char *sns_help_set_maxentries[];
extern const char sns_help_seen_oneline[];
extern const char sns_help_seennick_oneline[];
extern const char sns_help_remove_oneline[];
extern const char sns_help_stats_oneline[];
extern const char *sns_help_seen[];
extern const char *sns_help_seennick[];
extern const char *sns_help_remove[];
extern const char *sns_help_stats[];

/* events.c */
int SeenSignon (CmdParams *cmdparams);
int SeenQuit (CmdParams *cmdparams);
int SeenKill (CmdParams *cmdparams);
int SeenNickChange (CmdParams *cmdparams);
int SeenJoinChan (CmdParams *cmdparams);
int SeenPartChan (CmdParams *cmdparams);
int SeenKicked (CmdParams *cmdparams);

/* seenserv.c */
static int sns_set_seenchan (CmdParams *cmdparams, SET_REASON reason);
static int sns_set_maxentries (CmdParams *cmdparams, SET_REASON reason);
static int sns_set_exclusions (CmdParams *cmdparams, SET_REASON reason);

/* seen.c */
void addseenentry(char *nick, char *user, char *host, char *vhost, char *message, int type);
void checkseenlistlimit(void);
void removepreviousnick(char *nn);
void loadseendata(void);
int loadseenrecords(void *data);
int sortlistbytime(const void *key1, const void *key2);
void destroyseenlist(void);
int sns_cmd_seenhost(CmdParams *cmdparams);
int sns_cmd_seennick(CmdParams *cmdparams);
int sns_cmd_remove(CmdParams *cmdparams);
int sns_cmd_stats(CmdParams *cmdparams);