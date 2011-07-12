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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "../../sr_module.h"
#include "../../mem/mem.h"

#include "jsonrpc.h"

typedef struct jsonrpc_event jsonrpc_event_t;

struct jsonrpc_event {
	int id;
	jsonrpc_event_t *next;
	int (*cbfunc)(json_object*, char*, int);
	char *cbdata;
};

struct jsonrpc_event * event_table[JSONRPC_DEFAULT_HTABLE_SIZE] = {0};
int next_id = 1;

struct jsonrpc_event* get_event(int id);
int store_event(struct jsonrpc_event* ev);


json_object* build_jsonrpc_request(char *method, json_object *params, char *cbdata, int (*cbfunc)(json_object*, char*, int))
{
	if (next_id>JSONRPC_MAX_ID) {
		next_id = 1;
	} else {
		next_id++;
	}

	struct jsonrpc_event *ev = pkg_malloc(sizeof(struct jsonrpc_event));
	ev->id = next_id;
	ev->cbfunc = cbfunc;
	ev->cbdata = cbdata;
	ev->next = NULL;
	if (!store_event(ev))
		return 0;

	json_object *req = json_object_new_object();

	json_object_object_add(req, "id", json_object_new_int(next_id));
	json_object_object_add(req, "jsonrpc", json_object_new_string("2.0"));
	json_object_object_add(req, "method", json_object_new_string(method));
	json_object_object_add(req, "params", params);

	return req;
}

json_object* build_jsonrpc_notification(char *method, json_object *params) 
{
	json_object *req = json_object_new_object();
	json_object_object_add(req, "jsonrpc", json_object_new_string("2.0"));
	json_object_object_add(req, "method", json_object_new_string(method));
	json_object_object_add(req, "params", params);

	return req; 
}


int handle_jsonrpc_response(json_object *response)
{
	struct jsonrpc_event *ev;	
	json_object *_id = json_object_object_get(response, "id");
	int id = json_object_get_int(_id);
	
	if (!(ev = get_event(id))) {
		json_object_put(response);
		return -1;
	}

	json_object *result = json_object_object_get(response, "result");
	
	if (result) {
		ev->cbfunc(result, ev->cbdata, 0);
	} else {
		json_object *error = json_object_object_get(response, "error");
		if (error) {
			ev->cbfunc(error, ev->cbdata, 1);
		} else {
			LM_ERR("Response received with neither a result nor an error.\n");
			return -1;
		}
	}

	json_object_put(response);
	pkg_free(ev);
	return 1;
}

int id_hash(int id) {
	return (id % JSONRPC_DEFAULT_HTABLE_SIZE);
}

struct jsonrpc_event* get_event(int id) {
	int key = id_hash(id);

	struct jsonrpc_event *ev, *prev_ev = NULL;
	ev = event_table[key];
	
	while (ev && ev->id != id) {
		prev_ev = ev;
		if (!(ev = ev->next)) {
			break;
		};
	}
	
	if (ev && ev->id == id) {
		if (prev_ev != NULL) {
			prev_ev-> next = ev->next;
		} else {
			event_table[key] = NULL;
		}
		return ev;
	}

	LM_ERR("event for response id %d (key %d) was not found.\n", id, key);
	return 0;
}

int store_event(struct jsonrpc_event* ev) {
	int key = id_hash(ev->id);
	struct jsonrpc_event* existing;

	if ((existing = event_table[key])) { /* collision */
		struct jsonrpc_event* i;
		for(i=existing; i; i=i->next) {
			if (i == NULL) {
				i = ev;
				LM_ERR("!!!!!!!");
				return 1;
			}
			if (i->next == NULL) {
				i->next = ev;
				return 1;
			}
		}
	} else {
		event_table[key] = ev;
	}
	return 1;
}
