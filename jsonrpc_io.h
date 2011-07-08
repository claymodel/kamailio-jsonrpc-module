/**
 * $Id$
 *
 * Copyright (C) 2011 Flowroute LLC (flowroute.com)
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

#include "../../route_struct.h"
#include "../../pvar.h"

#define JSONRPC_SERVER_CONNECTED    1
#define JSONRPC_SERVER_DISCONNECTED 2
#define JSONRPC_SERVER_FAILURE      3

/* interval at which failed servers are retried */
#define JSONRPC_RECONNECT_INTERVAL  3

//#define JSONRPC_BUFFER_SIZE 2048

struct jsonrpc_pipe_cmd 
{
	char *method, *params, *cb_route;
	unsigned int t_hash, t_label;
	pv_spec_t *cb_pv;
	struct sip_msg *msg;
};

int jsonrpc_io_child_process(int data_pipe, char* servers);
void free_pipe_cmd(struct jsonrpc_pipe_cmd *cmd); 

#endif /* _JSONRPC_IO_H_ */
