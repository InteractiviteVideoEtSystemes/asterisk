/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 2024, IVËS.
 *
 * Jean-Pierre BROCHET <jeanpierre.brochet@ives.fr>
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*! \file
 *
 * \author Jean-Pierre BROCHET <jeanpierre.brochet@ives.fr>
 *
 * \brief Websocket Text handling
 */

/*** MODULEINFO
	<support_level>core</support_level>
	<depend type="module">res_http_websocket</depend>
 ***/

#include "asterisk.h"

#include "asterisk/module.h"
#include "asterisk/http.h"
#include "asterisk/astobj2.h"
#include "asterisk/strings.h"
#include "asterisk/file.h"
#include "asterisk/unaligned.h"
#include "asterisk/uri.h"
#include "asterisk/uuid.h"
#include "asterisk/lock.h"
#include "asterisk/channel.h"
#include "asterisk/stream.h"
#include "asterisk/frame.h"
#include "asterisk/http_websocket.h"

struct websocket_frame
{
	struct ast_frame *frame;
	AST_LIST_ENTRY(websocket_frame) entry;
};

struct websocket_session
{
	struct ast_websocket *websocket;
	char channel_name[256];
	AST_LIST_HEAD(frame_stack, websocket_frame) frame_stack;
	AST_LIST_ENTRY(websocket_session) entry;
};

static AST_LIST_HEAD(websocket_session_list, websocket_session) websocket_session_list;

static int push_frame(struct websocket_session *ws_session, struct ast_frame *frame)
{
	struct websocket_frame *new_frame;

	new_frame = ast_calloc(1, sizeof(*new_frame));
	if (!new_frame) {
		ast_log(LOG_ERROR, "Failed to allocate memory for new frame\n");
		return -1;
	}

	new_frame->frame = ast_frdup(frame);

	AST_LIST_LOCK(&ws_session->frame_stack);
	AST_LIST_INSERT_HEAD(&ws_session->frame_stack, new_frame, entry);
	AST_LIST_UNLOCK(&ws_session->frame_stack);

	ast_log(LOG_DEBUG, "Frame pushed to stack for websocket session with channel name: %s\n", ws_session->channel_name);
	return 0;
}

static struct ast_frame *pop_frame(struct websocket_session *ws_session)
{
	struct websocket_frame *frame_wrapper;
	struct ast_frame *frame = NULL;

	AST_LIST_LOCK(&ws_session->frame_stack);
	frame_wrapper = AST_LIST_REMOVE_HEAD(&ws_session->frame_stack, entry);
	AST_LIST_UNLOCK(&ws_session->frame_stack);

	if (frame_wrapper) {
		frame = frame_wrapper->frame;
		ast_free(frame_wrapper);
		ast_log(LOG_DEBUG, "Frame popped from stack for websocket session with channel name: %s\n", ws_session->channel_name);
	} else {
		ast_log(LOG_DEBUG, "No frame in stack for websocket session with with channel name: %s\n", ws_session->channel_name);
	}

	return frame;
}

/* Function to add a variable to a list */
static void add_variable_to_list(struct ast_variable **head, const char *name, const char *value)
{
	struct ast_variable *new_var, *last;

	new_var = ast_variable_new(name, value, "");
	if (!new_var) {
		ast_log(LOG_ERROR, "Failed to create new variable\n");
		return;
	}

	if (*head == NULL) {
		*head = new_var;
	} else {
		last = *head;
		while (last->next) {
			last = last->next;
		}
		last->next = new_var;
	}

	ast_log(LOG_NOTICE, "Variable added: %s=%s\n", name, value);
}

static int mywebsocket_uri_cb(struct ast_tcptls_session_instance *ser
	, const struct ast_http_uri *urih
	, const char *uri
	, enum ast_http_method method
	, struct ast_variable *get_params
	, struct ast_variable *headers
	)
{
	ast_debug(1, "Entering WebSocket echo2 loop method %s uri %s\n", ast_get_http_method(method), uri);
/**/
	struct websocket_session *ws_session = NULL;

	// Recherche de la session websocket grace au nom du canal asterisk recupere sur la session sip.
	AST_LIST_LOCK(&websocket_session_list);
	AST_LIST_TRAVERSE(&websocket_session_list, ws_session, entry)
	{
		if (!strcmp(ws_session->channel_name, uri)) {
			break;
		}
	}
	AST_LIST_UNLOCK(&websocket_session_list);

	if (!ws_session) {
		ast_http_error(ser, 403, "Access Denied", "You do not have permission to access the requested URL.");
		return 0;
	}
/**/
	add_variable_to_list(&get_params, "uri", uri);

	return ast_websocket_uri_cb(ser, urih, uri, method, get_params, headers);
}

static struct ast_http_uri mywebsocketuri = {
	.callback = mywebsocket_uri_cb,
	.description = "Asterisk my HTTP WebSocket",
	.uri = "ws_text",
	.has_subtree = 1,
	.data = NULL,
	.key = __FILE__,
};

/*! \brief Simple echo implementation which echoes received text and binary frames */
static void mywebsocket_echo_callback(struct ast_websocket *websocket, struct ast_variable *parameters, struct ast_variable *headers)
{
	int res;
	struct websocket_session *ws_session = NULL;

	ast_debug(1, "Entering WebSocket echo2 loop %s, addr remote %s and local %s\n"
		, ast_websocket_session_id(websocket)
		, ast_sockaddr_stringify(ast_websocket_remote_address(websocket))
		, ast_sockaddr_stringify(ast_websocket_local_address(websocket))
		);

	struct ast_variable *i;
	for (i = parameters; i; i = i->next) {
		ast_debug(1, "Entering WebSocket echo2 loop %s parameters %s = %s\n", ast_websocket_session_id(websocket), i->name, i->value);
		if (!strcmp(i->name, "uri")) {
			// Recherche de la session websocket grace au nom du canal asterisk recupere sur la session sip.
			AST_LIST_LOCK(&websocket_session_list);
			AST_LIST_TRAVERSE(&websocket_session_list, ws_session, entry)
			{
				if (!strcmp(ws_session->channel_name, i->value)) {
					break;
				}
			}
			AST_LIST_UNLOCK(&websocket_session_list);
		}
	}
	for (i = headers; i; i = i->next) {
		ast_debug(1, "Entering WebSocket echo2 loop %s headers %s = %s\n", ast_websocket_session_id(websocket), i->name, i->value);
	}

	//ws_session = ast_calloc(1, sizeof(*ws_session));
	if (!ws_session) {
		ast_log(LOG_ERROR, "Failed to find uri or allocate memory for WebSocket client\n");
		//ast_http_error(ser, 403, "Access Denied", "You do not have permission to access the requested URL.");
		goto end;
	}

	ast_websocket_ref(websocket);
	ws_session->websocket = websocket;
/*	
	AST_LIST_LOCK(&websocket_session_list);
	AST_LIST_INSERT_HEAD(&websocket_session_list, ws_session, entry);
	AST_LIST_UNLOCK(&websocket_session_list);
*/
	if (ast_fd_set_flags(ast_websocket_fd(websocket), O_NONBLOCK)) {
		goto end;
	}

	while ((res = ast_websocket_wait_for_input(websocket, -1)) > 0) {
		char *payload;
		uint64_t payload_len;
		enum ast_websocket_opcode opcode;
		int fragmented;

		if (ast_websocket_read(websocket, &payload, &payload_len, &opcode, &fragmented)) {
			// We err on the side of caution and terminate the ws_session if any error occurs.
			ast_log(LOG_WARNING, "Read failure during WebSocket echo2 loop\n");
			break;
		}
/**/
		if (opcode == AST_WEBSOCKET_OPCODE_TEXT && payload_len > 0) {
			// On empile une ast_frame avec le texte recu dans une liste
			struct ast_frame f_in;
			memset(&f_in, 0, sizeof(f_in));
			f_in.frametype = AST_FRAME_TEXT;
			f_in.datalen = payload_len + 1;
			f_in.data.ptr = (void *)payload;
			f_in.subclass.integer = 0;
			f_in.offset = 0;

			ast_log(LOG_DEBUG, "Frame %d %*.s pre-push to stack for session with channel name: %s\n", (int)payload_len, (int)payload_len, (const char *)payload, ws_session->channel_name);

			push_frame(ws_session, &f_in);
		}
/**/
		if (opcode == AST_WEBSOCKET_OPCODE_TEXT) {

			//ast_websocket_write(websocket, AST_WEBSOCKET_OPCODE_TEXT, payload, payload_len);
/**/
			struct ast_frame *f_out = pop_frame(ws_session);
			if (f_out && f_out->frametype == AST_FRAME_TEXT) {
				payload = f_out->data.ptr;
				/*
				if (payload[f_out->datalen - 1]) {
					// Not zero terminated, we need to allocate.
					payload = ast_strndup(payload, f_out->datalen);
					if (!payload) {
						ast_log(LOG_ERROR, "Failed to allocate memory for WebSocket client frame\n");
						break;
					}
				}
				*/

				payload_len = f_out->datalen-1;
				opcode = AST_WEBSOCKET_OPCODE_TEXT;

				ast_websocket_write(websocket, AST_WEBSOCKET_OPCODE_TEXT, payload, payload_len);

				if (payload != f_out->data.ptr) {
					// Only free if we allocated.
					ast_free(payload);
				}
				ast_frfree(f_out);
			}
/**/
		} else if (opcode == AST_WEBSOCKET_OPCODE_CLOSE) {
			break;
		} else {
			ast_debug(1, "Ignored WebSocket opcode %u\n", opcode);
		}
	}

end:
	ast_debug(1, "Exiting WebSocket echo2 loop %s\n", ast_websocket_session_id(websocket));
	ast_websocket_unref(websocket);
}


static int load_module(void)
{
	mywebsocketuri.data = ast_websocket_server_create();
	if (!mywebsocketuri.data) {
		return AST_MODULE_LOAD_DECLINE;
	}
	ast_http_uri_link(&mywebsocketuri);
	ast_websocket_server_add_protocol(mywebsocketuri.data, "echo2", mywebsocket_echo_callback);

	struct websocket_session *ws_session = NULL;

	ws_session = ast_calloc(1, sizeof(*ws_session));
	if (!ws_session) {
		ast_log(LOG_ERROR, "Failed to allocate memory for WebSocket client\n");
		return -1;
	}

	strcpy(ws_session->channel_name, "channel_demo");

	AST_LIST_LOCK(&websocket_session_list);
	AST_LIST_INSERT_HEAD(&websocket_session_list, ws_session, entry);
	AST_LIST_UNLOCK(&websocket_session_list);

	return 0;
}

static int unload_module(void)
{
	struct websocket_session *ws_session = NULL;

	ast_websocket_server_remove_protocol(mywebsocketuri.data, "echo2", mywebsocket_echo_callback);
	ast_http_uri_unlink(&mywebsocketuri);
	ao2_ref(mywebsocketuri.data, -1);
	mywebsocketuri.data = NULL;

	AST_LIST_LOCK(&websocket_session_list);
	AST_LIST_TRAVERSE_SAFE_BEGIN(&websocket_session_list, ws_session, entry)
	{
		struct websocket_frame *frame_wrapper = NULL;

		AST_LIST_REMOVE_CURRENT(entry);
		ast_websocket_unref(ws_session->websocket);

		AST_LIST_TRAVERSE_SAFE_BEGIN(&ws_session->frame_stack, frame_wrapper, entry)
		{
			ast_frfree(frame_wrapper->frame);
			ast_free(frame_wrapper);
		}
		AST_LIST_TRAVERSE_SAFE_END;

		ast_free(ws_session);
	}
	AST_LIST_TRAVERSE_SAFE_END;
	AST_LIST_UNLOCK(&websocket_session_list);

	return 0;
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_DEFAULT, "my HTTP WebSocket Support",
	.support_level = AST_MODULE_SUPPORT_CORE,
	.load = load_module,
	.unload = unload_module,
	.requires = "res_http_websocket",
);
