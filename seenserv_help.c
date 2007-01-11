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
** YahtzeeServ CVS Identification
** $Id$
*/

#include "neostats.h"    /* Required for bot support */
#include "seenserv.h"

const char *sns_help_set_chan[] = {
	"\2CHAN <#Channel>\2 - Set Channel Yahtzee Games Play in",
	NULL
};

const char *sns_help_set_exclusions[] = {
	"\2EXCLUSIONS <ON|OFF>\2",
	"Use global exclusion list in addition to local exclusion list",
	NULL
};

const char *sns_help_set_enable[] = {
	"\2ENABLE <ON|OFF>\2",
	"Enable or Disable SEEN and SEENNICK command sent as a message",
	NULL
};

const char *sns_help_set_enableseenchan[] = {
	"\2ENABLESEENCHAN <ON|OFF>\2",
	"Enable SeenServ to join the Seen Channel and accept commands in channel",
	NULL
};

const char *sns_help_set_seenchanname[] = {
	"\2SEENCHANNAME <#Channel>\2",
	"Set Channel SeenServ accepts commands from in channel",
	NULL
};

const char *sns_help_set_maxentries[] = {
	"\2MAXENTRIES <entries>\2",
	"Sets the maximum entries to save",
	NULL
};

const char *sns_help_set_eventsignon[] = {
	"\2EVENTSIGNON <ON|OFF>\2",
	"Enable Connection Event Recording.",
	NULL
};

const char *sns_help_set_eventquit[] = {
	"\2EVENTQUIT <ON|OFF>\2",
	"Enable Quit Event Recording.",
	NULL
};

const char *sns_help_set_eventkill[] = {
	"\2EVENTKILL <ON|OFF>\2",
	"Enable Kill Event Recording.",
	NULL
};

const char *sns_help_set_eventnick[] = {
	"\2EVENTNICK <ON|OFF>\2",
	"Enable Nick Change Event Recording.",
	NULL
};

const char *sns_help_set_eventjoin[] = {
	"\2EVENTJOIN <ON|OFF>\2",
	"Enable Channel Join Event Recording.",
	NULL
};

const char *sns_help_set_eventpart[] = {
	"\2EVENTPART <ON|OFF>\2",
	"Enable Channel Part Event Recording.",
	NULL
};

const char *sns_help_set_eventkick[] = {
	"\2EVENTKICK <ON|OFF>\2",
	"Enable Channel Kick Event Recording.",
	NULL
};

const char *sns_help_set_expiretime[] = {
	"\2EXPIRETIME <days>\2",
	"expire records older than set days.",
	"NOTE: 0 Days expires on MAXENTRIES only.",
	NULL
};

const char *sns_help_set_dbupdatetime[] = {
	"\2DBUPDATETIME <seconds>\2",
	"Sets how often data is written to the database.",
	"NOTE: Data will also be saved when a SEEN or",
	"SEENNICK command is performed.",
	NULL
};

const char *sns_help_set_memorylist[] = {
	"\2MEMORYLIST <ON|OFF>\2",
	"Sets if the seen data should be queried from an",
	"In Memory List, if set to OFF queries will be from",
	"the DB only, and lookups will only be able to be",
	"made via the nickname and not the userhost",
	NULL
};

const char *sns_help_seen[] = {
	"Displays Last Seen Nicks",
	"Syntax: \2SEEN <nick|userhost>\2",
	"",
	"Displays the last seen entry of the matching entry.",
	"NOTE: If queries are from the DB only, then only <nick> is a valid entry.",
	NULL
};

const char *sns_help_seennick[] = {
	"Displays Last Seen Nicks",
	"Syntax: \2SEEN <nick>\2",
	"",
	"Displays the last seen entry of the matching nick.",
	NULL
};

const char *sns_help_del[] = {
	"Deletes Seen Entries that match nick!user@host",
	"Syntax: \2DEL <hostmask>\2",
	"",
	"Deletes any entries matching the nick!user@host provided.",
	NULL
};

const char *sns_help_status[] = {
	"Displays Seen Statistics",
	"Syntax: \2STATUS\2",
	"",
	"Displays seenserv status information and statistics.",
	"NOTE: Statistics Unavailable when using DB only.",
	NULL
};
