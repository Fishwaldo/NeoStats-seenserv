/* YahtzeeServ - Yahtzee Game Service - NeoStats Addon Module
** Copyright (c) 2003-2005 DeadNotBuried
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

const char *sns_help_set_seenchan[] = {
	"\2SEENCHAN <#Channel>\2",
	"Set Channel SeenServ accepts commands from in channel",
	NULL
};

const char *sns_help_set_maxentries[] = {
	"\2MAXENTRIES <entries>\2",
	"Sets the maximum entries to save",
	NULL
};

const char *sns_help_set_verbose[] = {
	"\2VERBOSE <ON|OFF>\2",
	"Enable verbose mode.",
	NULL
};

const char sns_help_seen_oneline[] = "Displays Last Seen Nicks";
const char sns_help_seennick_oneline[] = "Displays Last Seen Nicks";
const char sns_help_del_oneline[] = "Deletes Seen Entries that match nick!user@host";
const char sns_help_stats_oneline[] = "Displays Seen Statistics";

const char *sns_help_seen[] = {
	"Syntax: \2SEEN <host>\2",
	"",
	"Displays the last seen entry of the matching entry.",
	NULL
};

const char *sns_help_seennick[] = {
	"Syntax: \2SEEN <nick>\2",
	"",
	"Displays the last seen entry of the matching nick.",
	NULL
};

const char *sns_help_del[] = {
	"Syntax: \2DEL <hostmask>\2",
	"",
	"Deletes any entries matching the nick!user@host provided.",
	NULL
};

const char *sns_help_stats[] = {
	"Syntax: \2STATS\2",
	"",
	"Displays SEEN entry statistics.",
	NULL
};
