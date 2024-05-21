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

#define AST_API_MODULE
#include "asterisk/http_websocket.h"

struct websocket_client
{
	struct ast_websocket *websocket;
	AST_LIST_ENTRY(websocket_client) entry;
};

static AST_LIST_HEAD(clients, websocket_client) clients;

static struct ast_http_uri websocketuri = {
	.callback = AST_OPTIONAL_API_NAME(ast_websocket_uri_cb),
	.description = "Asterisk my HTTP WebSocket",
	.uri = "ws_text",
	.has_subtree = 0,
	.data = NULL,
	.key = __FILE__,
};

/*! \brief Simple echo implementation which echoes received text and binary frames */
static void websocket_echo_callback(struct ast_websocket *session, struct ast_variable *parameters, struct ast_variable *headers)
{
	int res;
	struct websocket_client *client;

	ast_debug(1, "Entering WebSocket echo2 loop %s\n", ast_websocket_session_id(session));

	client = ast_calloc(1, sizeof(*client));
	if (!client) {
		ast_log(LOG_ERROR, "Failed to allocate memory for WebSocket client\n");
		ast_websocket_unref(session);
		return;
	}

	ast_websocket_ref(session);
	client->websocket = session;
	
	AST_LIST_LOCK(&clients);
	AST_LIST_INSERT_HEAD(&clients, client, entry);
	AST_LIST_UNLOCK(&clients);

	if (ast_fd_set_flags(ast_websocket_fd(session), O_NONBLOCK)) {
		goto end;
	}

	while ((res = ast_websocket_wait_for_input(session, -1)) > 0) {
		char *payload;
		uint64_t payload_len;
		enum ast_websocket_opcode opcode;
		int fragmented;

		if (ast_websocket_read(session, &payload, &payload_len, &opcode, &fragmented)) {
			/* We err on the side of caution and terminate the session if any error occurs */
			ast_log(LOG_WARNING, "Read failure during WebSocket echo2 loop\n");
			break;
		}

		if (opcode == AST_WEBSOCKET_OPCODE_TEXT || opcode == AST_WEBSOCKET_OPCODE_BINARY) {
			ast_websocket_write(session, opcode, payload, payload_len);
		} else if (opcode == AST_WEBSOCKET_OPCODE_CLOSE) {
			break;
		} else {
			ast_debug(1, "Ignored WebSocket opcode %u\n", opcode);
		}
	}

end:
	ast_debug(1, "Exiting WebSocket echo2 loop %s\n", ast_websocket_session_id(session));
	ast_websocket_unref(session);
}


static int load_module(void)
{
	websocketuri.data = ast_websocket_server_create();
	if (!websocketuri.data) {
		return AST_MODULE_LOAD_DECLINE;
	}
	ast_http_uri_link(&websocketuri);
	ast_websocket_server_add_protocol(websocketuri.data, "echo2", websocket_echo_callback);

	return 0;
}

static int unload_module(void)
{
	struct websocket_client *client;

	ast_websocket_server_remove_protocol(websocketuri.data, "echo2", websocket_echo_callback);
	ast_http_uri_unlink(&websocketuri);
	ao2_ref(websocketuri.data, -1);
	websocketuri.data = NULL;

	AST_LIST_LOCK(&clients);
	AST_LIST_TRAVERSE_SAFE_BEGIN(&clients, client, entry)
	{
		AST_LIST_REMOVE_CURRENT(entry);
		ast_websocket_unref(client->websocket);
		ast_free(client);
	}
	AST_LIST_TRAVERSE_SAFE_END;
	AST_LIST_UNLOCK(&clients);

	return 0;
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_DEFAULT, "my HTTP WebSocket Support",
	.support_level = AST_MODULE_SUPPORT_CORE,
	.load = load_module,
	.unload = unload_module,
	.requires = "res_http_websocket",
);
