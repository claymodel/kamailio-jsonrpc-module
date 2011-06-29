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

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <event.h>

#include "../../sr_module.h"
#include "../../route.h"
#include "../../route_struct.h"
#include "../../lvalue.h"
#include "../tm/tm_load.h"

#include "jsonrpc_io.h"
#include "jsonrpc.h"
#include "netstring.h"


int sockfd, cmd_pipe;
struct sockaddr_in  server;

void socket_cb(int fd, short event, void *arg);
void cmd_pipe_cb(int fd, short event, void *arg);

struct tm_binds tmb;

int setnonblock(int fd)
{
	int flags;

	flags = fcntl(fd, F_GETFL);
	if (flags < 0)
		return flags;
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) < 0)
		return -1;

	return 0;
}

int jsonrpc_io_child_process(int _cmd_pipe, char* host, int port) {
	cmd_pipe = _cmd_pipe;
  struct event_base *evbase = event_init();

	struct hostent *hp;
  server.sin_family = AF_INET;
  server.sin_port = htons(port);

	hp = gethostbyname(host);
	if (hp == NULL) {
		LM_ERR("gethostbyname(%s) failed with h_errno=%d.\n", host, h_errno);
		return -1;
	}
	memcpy(&(server.sin_addr.s_addr), hp->h_addr, hp->h_length);

	sockfd = socket(AF_INET,SOCK_STREAM,0);
	
  if (connect(sockfd, (struct sockaddr *)&server, sizeof(server))) {
    LM_ERR("error connecting to %s on port %d... %s\n", remote_host, remote_port, strerror(errno));
		return -1;
  }

	setnonblock(sockfd);
	setnonblock(cmd_pipe);

	struct event pipe_ev, socket_ev;
	
	event_init();

	event_set(&socket_ev, sockfd, EV_READ | EV_PERSIST, socket_cb, &socket_ev);
	event_add(&socket_ev, NULL);
	
	event_set(&pipe_ev, cmd_pipe, EV_READ | EV_PERSIST, cmd_pipe_cb, &pipe_ev);
	event_add(&pipe_ev, NULL);
	
	event_dispatch();
	close(sockfd);
	close(cmd_pipe);
	//free(evbase);
	return 0;
}

int result_cb(json_object *result, char *data) {
	struct jsonrpc_pipe_cmd *cmd = (struct jsonrpc_pipe_cmd*)data;

	pv_spec_t *dst = cmd->cb_pv;
	pv_value_t val;
	
	const char* res = json_object_get_string(result);
	
	
	val.rs.s = (char*)res;
	val.rs.len = strlen(res);
	val.flags = PV_VAL_STR;
	
	dst->setf(0, &dst->pvp, (int)EQ_T, &val);

	int n;
	n = route_get(&main_rt, cmd->cb_route);
	struct action *a = main_rt.rlist[n];

	tmb.t_continue(cmd->t_hash, cmd->t_label, a);	
	
	json_object_put(result);
	shm_free(cmd);
	return 0;
}

int (*cb)(json_object*, char*) = &result_cb;


void cmd_pipe_cb(int fd, short event, void *arg)
{
	struct jsonrpc_pipe_cmd *cmd;
	struct event *ev = (struct event*)arg;

	if (read(fd, &cmd, sizeof(cmd)) != sizeof(cmd)) {
		LM_ERR("failed to read from command pipe: %s\n",
			strerror(errno));
	}

	enum jsonrpc_t *t = JSONRPC_REQUEST;
	json_object *params = json_tokener_parse(cmd->params);;
	json_object *req = build_jsonrpc_request(t, cmd->method, params, (char*)cmd, cb);
	if (!req) {
		LM_ERR("failed to build jsonrpc_request (method: %s, params: %s)\n", cmd->method, cmd->params);	
	}
	char *json = (char*)json_object_get_string(req);

	char *ns; size_t bytes;
  bytes = netstring_encode_new(&ns, json, (size_t)strlen(json));

	if (send(sockfd,ns,bytes,0) != bytes) {
		LM_ERR("send failed!!!!!!!\n");
	}
	free(ns);
	json_object_put(req);
}

void socket_cb(int fd, short event, void *arg)
{	
	if (event != EV_READ) {
		LM_ERR("unexpected socket event (%d)\n", event);
		return;
	}
		
	struct event *ev = (struct event*)arg;
	
	int bytes = 0;
	while (1) {
		char buffer[JSONRPC_BUFFER_SIZE] = {0};
	  int new_bytes = recv(fd,buffer,JSONRPC_BUFFER_SIZE,0);
		if (new_bytes < 0) {
			LM_ERR("recv failed: %s (%d).\n", strerror(errno), errno);
			return;
		}
		
		bytes += new_bytes;
	
	  if (new_bytes) {
		  char *netstring;
		  size_t netstring_len;
		  int retval = netstring_read(buffer, bytes, &netstring, &netstring_len);
		
		
		} else if (bytes==0){
			LM_ERR("socket closed...what now?\n");
			event_del(ev);
			return;
		} else {
			return;
		}
		
	}
	
	// char response[bytes];
	// strcpy(response,buffer);

	if (retval != 0) {
		LM_ERR("bad netstring %s (%d)\n", response, retval);
		return;
	}	
	
	netstring[netstring_len] = '\0';
	
	struct json_object *res = json_tokener_parse(netstring);
	handle_jsonrpc_response(res);
	//pkg_free(buffer);
}
