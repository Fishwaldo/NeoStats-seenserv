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

static char tmpmsg[SS_MESSAGESIZE];

/*
 * Signon Events
*/
int SeenSignon (CmdParams *cmdparams) {
	if (ModIsUserExcluded(cmdparams->source)) {
		return NS_SUCCESS;
	}
	ircsnprintf(tmpmsg, SS_MESSAGESIZE, "%s", cmdparams->param);
	addseenentry(cmdparams->source->name, cmdparams->source->user->userhostmask, cmdparams->source->user->uservhostmask, tmpmsg, SS_CONNECTED);
	return NS_SUCCESS;
}

/*
 * Quit Events
*/
int SeenQuit (CmdParams *cmdparams) {
	if (ModIsUserExcluded(cmdparams->source)) {
		return NS_SUCCESS;
	}
	ircsnprintf(tmpmsg, SS_MESSAGESIZE, "(%s)", cmdparams->param);
	addseenentry(cmdparams->source->name, cmdparams->source->user->userhostmask, cmdparams->source->user->uservhostmask, tmpmsg, SS_QUIT);
	return NS_SUCCESS;
}

/*
 * Kill Events
*/
int SeenKill (CmdParams *cmdparams) {
	if (ModIsUserExcluded(cmdparams->target)) {
		return NS_SUCCESS;
	}
	ircsnprintf(tmpmsg, SS_MESSAGESIZE, "by %s (%s)", cmdparams->target->name, cmdparams->source->name, cmdparams->param);
	addseenentry(cmdparams->target->name, cmdparams->target->user->userhostmask, cmdparams->target->user->uservhostmask, tmpmsg, SS_KILLED);
	return NS_SUCCESS;
}

/*
 * Nick Events
*/
int SeenNickChange (CmdParams *cmdparams) {
	if (ModIsUserExcluded(cmdparams->source)) {
		return NS_SUCCESS;
	}
	if (ModIsChannelExcluded(cmdparams->channel)) {
		return NS_SUCCESS;
	}
	strlcpy(tmpmsg, cmdparams->source->name, SS_MESSAGESIZE);
	addseenentry(cmdparams->param, cmdparams->source->user->userhostmask, cmdparams->source->user->uservhostmask, tmpmsg, SS_NICKCHANGE);
	return NS_SUCCESS;
}

/*
 * Join Events
*/
int SeenJoinChan (CmdParams *cmdparams) {
	if (is_hidden_chan(cmdparams->channel)) {
		return NS_SUCCESS;
	}
	if (ModIsUserExcluded(cmdparams->source)) {
		return NS_SUCCESS;
	}
	if (ModIsChannelExcluded(cmdparams->channel)) {
		return NS_SUCCESS;
	}
	strlcpy(tmpmsg, cmdparams->channel->name, SS_MESSAGESIZE);
	addseenentry(cmdparams->source->name, cmdparams->source->user->userhostmask, cmdparams->source->user->uservhostmask, tmpmsg, SS_JOIN);
	return NS_SUCCESS;
}

/*
 * Part Events
*/
int SeenPartChan (CmdParams *cmdparams) {
	if (is_hidden_chan(cmdparams->channel)) {
		return NS_SUCCESS;
	}
	if (ModIsUserExcluded(cmdparams->source)) {
		return NS_SUCCESS;
	}
	if (ModIsChannelExcluded(cmdparams->channel)) {
		return NS_SUCCESS;
	}
	if (cmdparams->param) {
		ircsnprintf(tmpmsg, SS_MESSAGESIZE, "%s (%s)", cmdparams->channel->name, cmdparams->param);
	} else {
		strlcpy(tmpmsg, cmdparams->channel->name, SS_MESSAGESIZE);
	}
	addseenentry(cmdparams->source->name, cmdparams->source->user->userhostmask, cmdparams->source->user->uservhostmask, tmpmsg, SS_PART);
	return NS_SUCCESS;
}

/*
 * Kick Events
*/
int SeenKicked (CmdParams *cmdparams) {
	if (is_hidden_chan(cmdparams->channel)) {
		return NS_SUCCESS;
	}
	if (ModIsUserExcluded(cmdparams->target)) {
		return NS_SUCCESS;
	}
	if (ModIsChannelExcluded(cmdparams->channel)) {
		return NS_SUCCESS;
	}
	ircsnprintf(tmpmsg, SS_MESSAGESIZE, "%s by %s (%s)", cmdparams->channel->name, cmdparams->source->name, cmdparams->param);
	addseenentry(cmdparams->target->name, cmdparams->target->user->userhostmask, cmdparams->target->user->uservhostmask, tmpmsg, SS_KICKED);
	return NS_SUCCESS;
}
