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

#ifndef _JSONRPC_H_
#define _JSONRPC_H_

#define JSONRPC_DEFAULT_HTABLE_SIZE 500
#define JSONRPC_MAX_ID 1000000

#include <json.h>

static int rpc_htable_size;

enum jsonrpc_t{
	JSONRPC_REQUEST,
	JSONRPC_REPLY,
	JSONRPC_ERROR,
	JSONRPC_NOTIFICATION
};

json_object* build_jsonrpc_request(enum jsonrpc_t *req_type, char *method, json_object *params, char *cbdata, int (*cbfunc)(json_object*, char*));
int handle_jsonrpc_response(json_object *response);

#endif /* _JSONRPC_H_ */
