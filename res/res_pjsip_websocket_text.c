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
    <depend>pjproject</depend>
    <depend>res_pjsip</depend>
    <depend>res_pjsip_session</depend>
    <support_level>core</support_level>
 ***/

#include "asterisk.h"

#include <pjsip.h>
#include <pjsip_ua.h>
#include <pjmedia.h>
#include <pjlib.h>

#include "asterisk/utils.h"
#include "asterisk/module.h"
#include "asterisk/netsock2.h"
#include "asterisk/channel.h"
#include "asterisk/acl.h"
#include "asterisk/stream.h"
#include "asterisk/format_cache.h"

#include "asterisk/res_pjsip.h"
#include "asterisk/res_pjsip_session.h"

static const char STR_TEXT[] = "text";

/*! \brief Address for ws_text */
static struct ast_sockaddr address;

/*! \brief Websocket Text information */
struct ast_websocket_text
{
    int fd;
};

/*! \brief Supplement for adding framehook to session channel */
static struct ast_sip_session_supplement websocket_text_supplement = {
    .method = "INVITE",
    .priority = AST_SIP_SUPPLEMENT_PRIORITY_CHANNEL + 1,
//	.incoming_request = t38_incoming_invite_request,
//	.outgoing_request = t38_outgoing_invite_request,
};

static int ast_websocket_text_fd(const struct ast_websocket_text *ws_text)
{
    return ws_text->fd;
}

/*! \brief Destructor for T.38 state information */
static void ast_websocket_text_destroy(void *obj)
{
    ast_free(obj);
}

/*! \brief Function which negotiates an incoming media stream */
static int negotiate_incoming_sdp_stream(struct ast_sip_session *session,
    struct ast_sip_session_media *session_media, const struct pjmedia_sdp_session *sdp,
    int index, struct ast_stream *asterisk_stream)
{
    char host[NI_MAXHOST];
    pjmedia_sdp_media *stream = sdp->media[index];
    RAII_VAR(struct ast_sockaddr *, addrs, NULL, ast_free);

    if (!session->endpoint->media.websocket_text_configuration.enabled) {
        ast_debug(3, "Declining: websocket text configuration not enabled on session\n");
        return 0;
    }

    ast_copy_pj_str(host, stream->conn ? &stream->conn->addr : &sdp->conn->addr, sizeof(host));

    /* Ensure that the address provided is valid */
    if (ast_sockaddr_resolve(&addrs, host, PARSE_PORT_FORBID, AST_AF_UNSPEC) <= 0) {
        /* The provided host was actually invalid so we error out this negotiation */
        ast_debug(3, "Declining: provided host is invalid\n");
        return 0;
    }

    /* Check the address family to make sure it matches configured */
    if ((ast_sockaddr_is_ipv6(addrs) && !session->endpoint->media.websocket_text_configuration.ipv6) ||
        (ast_sockaddr_is_ipv4(addrs) && session->endpoint->media.websocket_text_configuration.ipv6)) {
        /* The address does not match configured */
        ast_debug(3, "Declining: provided host does not match configured address family\n");
        return 0;
    }

    return 1;
}

/*! \brief Function which creates an outgoing stream */
static int create_outgoing_sdp_stream(struct ast_sip_session *session, struct ast_sip_session_media *session_media,
    struct pjmedia_sdp_session *sdp, const struct pjmedia_sdp_session *remote, struct ast_stream *stream)
{
    pj_pool_t *pool = session->inv_session->pool_prov;
    static const pj_str_t STR_IN = {"IN", 2};
    static const pj_str_t STR_IP4 = {"IP4", 3};
    static const pj_str_t STR_IP6 = {"IP6", 3};
    static const pj_str_t STR_TEXT = {"text", 4};

    pjmedia_sdp_media *media;
    const char *hostip = NULL;
    struct ast_sockaddr addr;
/*
    char tmp[512];
    pj_str_t stmp;
*/

    if (!session->endpoint->media.websocket_text_configuration.enabled) {
        ast_debug(3, "Not creating outgoing SDP stream: websocket text not enabled\n");
        return 1;
    } else

    /*
    if ((session->t38state != T38_LOCAL_REINVITE) && (session->t38state != T38_PEER_REINVITE) &&
        (session->t38state != T38_ENABLED)) {
        ast_debug(3, "Not creating outgoing SDP stream: T.38 not enabled\n");
        return 1;
    } else if (!(state = t38_state_get_or_alloc(session))) {
        return -1;
    } else if (t38_initialize_session(session, session_media)) {
        ast_debug(3, "Not creating outgoing SDP stream: Failed to initialize T.38 session\n");
        return -1;
    }
    */

    if (!(media = pj_pool_zalloc(pool, sizeof(struct pjmedia_sdp_media))) ||
        !(media->conn = pj_pool_zalloc(pool, sizeof(struct pjmedia_sdp_conn)))) {
        return -1;
    }

    pj_strdup2(pool, &media->desc.media, ast_codec_media_type2str(session_media->type));
    media->desc.transport = STR_TEXT;

    if (ast_strlen_zero(session->endpoint->media.address)) {
        hostip = ast_sip_get_host_ip_string(session->endpoint->media.websocket_text_configuration.ipv6 ? pj_AF_INET6() : pj_AF_INET());
    } else {
        hostip = session->endpoint->media.address;
    }

    if (ast_strlen_zero(hostip)) {
        ast_debug(3, "Not creating outgoing SDP stream: no known host IP\n");
        return -1;
    }

    media->conn->net_type = STR_IN;
    media->conn->addr_type = session->endpoint->media.websocket_text_configuration.ipv6 ? STR_IP6 : STR_IP4;
    pj_strdup2(pool, &media->conn->addr, hostip);
    media->desc.port = (pj_uint16_t) ast_sockaddr_port(&addr);
    media->desc.port_count = 1;
    media->desc.fmt[media->desc.fmt_count++] = STR_TEXT;
    
    sdp->media[sdp->media_count++] = media;

    return 1;
}

static struct ast_frame *media_session_websocket_text_read_callback(struct ast_sip_session *session, struct ast_sip_session_media *session_media)
{
    struct ast_frame *frame = NULL;

    if (!session_media->websocket_text) {
        return &ast_null_frame;
    }

    //frame = ast_udptl_read(session_media->udptl);
    if (!frame) {
        return NULL;
    }

    frame->stream_num = session_media->stream_num;

    return frame;
}

static int media_session_websocket_text_write_callback(struct ast_sip_session *session, struct ast_sip_session_media *session_media, struct ast_frame *frame)
{
    if (!session_media->websocket_text) {
        return 0;
    }

    //return ast_udptl_write(session_media->udptl, frame);
    return 0;
}

/*! \brief Function which applies a negotiated stream */
static int apply_negotiated_sdp_stream(struct ast_sip_session *session,
    struct ast_sip_session_media *session_media, const struct pjmedia_sdp_session *local,
    const struct pjmedia_sdp_session *remote, int index, struct ast_stream *asterisk_stream)
{
    RAII_VAR(struct ast_sockaddr *, addrs, NULL, ast_free);
    pjmedia_sdp_media *remote_stream = remote->media[index];
    char host[NI_MAXHOST];

    if (!session_media->websocket_text) {
        ast_debug(3, "Not applying negotiated SDP stream: no Websocket Text session\n");
        return 0;
    }

    ast_copy_pj_str(host, remote_stream->conn ? &remote_stream->conn->addr : &remote->conn->addr, sizeof(host));

    /* Ensure that the address provided is valid */
    if (ast_sockaddr_resolve(&addrs, host, PARSE_PORT_FORBID, AST_AF_UNSPEC) <= 0) {
        /* The provided host was actually invalid so we error out this negotiation */
        ast_debug(3, "Not applying negotiated SDP stream: failed to resolve remote stream host\n");
        return -1;
    }

    ast_sip_session_media_set_write_callback(session, session_media, media_session_websocket_text_write_callback);
    ast_sip_session_media_add_read_callback(session
        , session_media
        , ast_websocket_text_fd(session_media->websocket_text)
        , media_session_websocket_text_read_callback
        );

    return 0;
}

/*! \brief Function which updates the media stream with external media address, if applicable */
static void change_outgoing_sdp_stream_media_address(pjsip_tx_data *tdata, struct pjmedia_sdp_media *stream, struct ast_sip_transport *transport)
{
    RAII_VAR(struct ast_sip_transport_state *, transport_state, ast_sip_get_transport_state(ast_sorcery_object_get_id(transport)), ao2_cleanup);
    char host[NI_MAXHOST];
    struct ast_sockaddr our_sdp_addr = {{0, }};

    /* If the stream has been rejected there will be no connection line */
    if (!stream->conn || !transport_state) {
        return;
    }

    ast_copy_pj_str(host, &stream->conn->addr, sizeof(host));
    ast_sockaddr_parse(&our_sdp_addr, host, PARSE_PORT_FORBID);

    /* Reversed check here. We don't check the remote endpoint being
     * in our local net, but whether our outgoing session IP is
     * local. If it is not, we won't do rewriting. No localnet
     * configured? Always rewrite. */
    if (ast_sip_transport_is_nonlocal(transport_state, &our_sdp_addr) && transport_state->localnet) {
        return;
    }
    ast_debug(5, "Setting media address to %s\n", ast_sockaddr_stringify_addr_remote(&transport_state->external_media_address));
    pj_strdup2(tdata->pool, &stream->conn->addr, ast_sockaddr_stringify_addr_remote(&transport_state->external_media_address));
}

/*! \brief Function which destroys the Websocket Text instance when session ends */
static void stream_destroy(struct ast_sip_session_media *session_media)
{
    if (session_media->websocket_text) {
        ast_websocket_text_destroy(session_media->websocket_text);
    }
    session_media->websocket_text = NULL;
}

/*! \brief SDP handler for 'application' media stream */
static struct ast_sip_session_sdp_handler text_sdp_handler = {
    .id = STR_TEXT,
    .negotiate_incoming_sdp_stream = negotiate_incoming_sdp_stream,
    .create_outgoing_sdp_stream = create_outgoing_sdp_stream,
    .apply_negotiated_sdp_stream = apply_negotiated_sdp_stream,
    .change_outgoing_sdp_stream_media_address = change_outgoing_sdp_stream_media_address,
    .stream_destroy = stream_destroy,
};

/*! \brief Unloads the SIP Websocket Text module from Asterisk */
static int unload_module(void)
{
    ast_sip_session_unregister_sdp_handler(&text_sdp_handler, STR_TEXT);
    ast_sip_session_unregister_supplement(&websocket_text_supplement);

    return 0;
}

/*!
 * \brief Load the module
 *
 * Module loading including tests for configuration or dependencies.
 * This function can return AST_MODULE_LOAD_FAILURE, AST_MODULE_LOAD_DECLINE,
 * or AST_MODULE_LOAD_SUCCESS. If a dependency or environment variable fails
 * tests return AST_MODULE_LOAD_FAILURE. If the module can not load the
 * configuration file or other non-critical problem return
 * AST_MODULE_LOAD_DECLINE. On success return AST_MODULE_LOAD_SUCCESS.
 */
static int load_module(void)
{
    if (ast_check_ipv6()) {
        ast_sockaddr_parse(&address, "::", 0);
    } else {
        ast_sockaddr_parse(&address, "0.0.0.0", 0);
    }

    ast_sip_session_register_supplement(&websocket_text_supplement);

    if (ast_sip_session_register_sdp_handler(&text_sdp_handler, STR_TEXT)) {
        ast_log(LOG_ERROR, "Unable to register SDP handler for %s stream type\n", STR_TEXT);
        goto end;
    }

    return AST_MODULE_LOAD_SUCCESS;
end:
    unload_module();

    return AST_MODULE_LOAD_DECLINE;
}

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_LOAD_ORDER, "PJSIP Websocket Text Support",
    .support_level = AST_MODULE_SUPPORT_CORE,
    .load = load_module,
    .unload = unload_module,
    .load_pri = AST_MODPRI_CHANNEL_DRIVER,
    .requires = "res_pjsip,res_pjsip_session",
    );
