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

/*
 * Signon Events
*/
int SeenSignon (CmdParams *cmdparams) {
	char tmpmsg[512];

	ircsnprintf(tmpmsg, 512, "%s", cmdparams->param);
	addseenentry(cmdparams->source->name, cmdparams->source->user->username, cmdparams->source->user->hostname, cmdparams->source->user->vhost, tmpmsg, SS_CONNECTED);
	return NS_SUCCESS;
}

/*
 * Quit Events
*/
int SeenQuit (CmdParams *cmdparams) {
	char tmpmsg[512];

	ircsnprintf(tmpmsg, 512, "(%s)", cmdparams->param);
	addseenentry(cmdparams->source->name, cmdparams->source->user->username, cmdparams->source->user->hostname, cmdparams->source->user->vhost, tmpmsg, SS_QUIT);
	return NS_SUCCESS;
}

/*
 * Kill Events
*/
int SeenKill (CmdParams *cmdparams) {
	char tmpmsg[512];

	ircsnprintf(tmpmsg, 512, "by %s (%s)", cmdparams->target->name, cmdparams->source->name, cmdparams->param);
	addseenentry(cmdparams->target->name, cmdparams->target->user->username, cmdparams->target->user->hostname, cmdparams->target->user->vhost, tmpmsg, SS_KILLED);
	return NS_SUCCESS;
}

/*
 * Nick Events
*/
int SeenNickChange (CmdParams *cmdparams) {
	char tmpmsg[512];

	ircsnprintf(tmpmsg, 512, "%s", cmdparams->source->name);
	addseenentry(cmdparams->param, cmdparams->source->user->username, cmdparams->source->user->hostname, cmdparams->source->user->vhost, tmpmsg, SS_NICKCHANGE);
	return NS_SUCCESS;
}

/*
 * Join Events
*/
int SeenJoinChan (CmdParams *cmdparams) {
	char tmpmsg[512];

	if (is_hidden_chan(cmdparams->channel)) {
		return NS_SUCCESS;
	}
	ircsnprintf(tmpmsg, 512, "%s", cmdparams->channel->name);
	addseenentry(cmdparams->source->name, cmdparams->source->user->username, cmdparams->source->user->hostname, cmdparams->source->user->vhost, tmpmsg, SS_JOIN);
	return NS_SUCCESS;
}

/*
 * Part Events
*/
int SeenPartChan (CmdParams *cmdparams) {
	char tmpmsg[512];

	if (is_hidden_chan(cmdparams->channel)) {
		return NS_SUCCESS;
	}
	if (cmdparams->param) {
		ircsnprintf(tmpmsg, 512, "%s (%s)", cmdparams->channel->name, cmdparams->param);
	} else {
		ircsnprintf(tmpmsg, 512, "%s", cmdparams->channel->name);
	}
	addseenentry(cmdparams->source->name, cmdparams->source->user->username, cmdparams->source->user->hostname, cmdparams->source->user->vhost, tmpmsg, SS_PART);
	return NS_SUCCESS;
}

/*
 * Kick Events
*/
int SeenKicked (CmdParams *cmdparams) {
	char tmpmsg[512];

	if (is_hidden_chan(cmdparams->channel)) {
		return NS_SUCCESS;
	}
	ircsnprintf(tmpmsg, 512, "%s by %s (%s)", cmdparams->channel->name, cmdparams->source->name, cmdparams->param);
	addseenentry(cmdparams->target->name, cmdparams->target->user->username, cmdparams->target->user->hostname, cmdparams->target->user->vhost, tmpmsg, SS_KICKED);
	return NS_SUCCESS;
}
