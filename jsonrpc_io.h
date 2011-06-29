/**
 * $Id$
 *
 * Copyright (C) 2011 FlowRoute LLC (flowroute.com)
 *
 * This file is part of Kamailio, a free SIP server.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _JSONRPC_IO_H_
#define _JSONRPC_IO_H_

#define JSONRPC_BUFFER_SIZE 2048

#include "../../route_struct.h"
#include "../../pvar.h"

static char* remote_host;
static int   remote_port;

struct jsonrpc_pipe_cmd {
	char *method, *params, *cb_route;
	unsigned int t_hash, t_label;
	pv_spec_t *cb_pv;
	struct sip_msg *msg;
};

int jsonrpc_io_child_process(int data_pipe, char* host, int port);

#endif /* _JSONRPC_IO_H_ */
