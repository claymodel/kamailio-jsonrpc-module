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

void socket_cb(int fd, short event, void *arg);
void cmd_pipe_cb(int fd, short event, void *arg);

struct tm_binds tmb;

int jsonrpc_io_child_process(int _cmd_pipe, char* host, int port) {
	cmd_pipe = _cmd_pipe;

	struct sockaddr_in  server;
	struct hostent *hp;

  struct event_base *evbase = event_init();

  server.sin_family = AF_INET;
  server.sin_port = htons(port);

	LM_ERR("gethostbyname(%s).\n", host);
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

	struct event pipe_ev, socket_ev;
	
	event_init();

	event_set(&socket_ev, sockfd, EV_READ | EV_PERSIST, socket_cb, &socket_ev);
	event_add(&socket_ev, NULL);
	
	event_set(&pipe_ev, cmd_pipe, EV_READ | EV_PERSIST, cmd_pipe_cb, &pipe_ev);
	event_add(&pipe_ev, NULL);
	
	event_dispatch();
	close(sockfd);
	close(cmd_pipe);

	return 0;
}

int result_cb(json_object *result, char *data) {
	struct jsonrpc_pipe_cmd *cmd = (struct jsonrpc_pipe_cmd*)data;

	pv_spec_t *dst = cmd->cb_pv;
	// pv_spec_t *dst = pkg_malloc(sizeof(pv_spec_t));
	// memcpy(dst, cmd->cb_pv, sizeof(dst));

//pv_spec_t *dst = cmd->cb_pv;
	pv_value_t val;
	
	const char* res = json_object_get_string(result);
	
	val.rs.s = res;
	val.rs.len = strlen(res);
	val.flags = PV_VAL_STR;

	if (!cmd->cb_pv->setf)
		LM_ERR("dst->setf is NULL");
	
	dst->setf(0, &dst->pvp, (int)EQ_T, &val);

	int n;
	n = route_get(&main_rt, cmd->cb_route);
	struct action *a = main_rt.rlist[n];

	tmb.t_continue(cmd->t_hash, cmd->t_label, a);	
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
	const char *json = json_object_get_string(req);

	char *ns; size_t bytes;
  bytes = netstring_encode_new(&ns, (char*)json, strlen(json));
	send(sockfd,ns,strlen(ns),0);
}

void socket_cb(int fd, short event, void *arg)
{	
	struct event *ev = (struct event*)arg;
	char *buffer=malloc(JSONRPC_BUFFER_SIZE);
  int bytes = recv(fd,buffer,JSONRPC_BUFFER_SIZE,0);
	char response[bytes];
	strcpy(response,buffer);
	free(buffer);
  char *netstring;
  size_t netstring_len;
  int retval = netstring_read(response, bytes, &netstring, &netstring_len);
	if (retval!= 0) {
		LM_ERR("bad netstring %s (%d)\n", netstring, retval);
	}	
	netstring[netstring_len] = '\0';
	
	struct json_object *res = json_tokener_parse(netstring);
	
	handle_jsonrpc_response(res);
}