/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 2024, IVèS.
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
#include "asterisk/http_websocket.h"

#include "asterisk/res_pjsip.h"
#include "asterisk/res_pjsip_session.h"
#include "asterisk/res_pjsip_session_caps.h"

static const char STR_TEXT[] = "text";

/*! \brief Address for ws_text */
static struct ast_sockaddr address_ws_text;

/*! \brief Websocket Text information */
struct ast_websocket_text
{
    int fd;
};

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


static void replace_newline(char *buffer, char replacement)
{
    // Parcours du buffer jusqu'à la fin de la chaîne
    for (int i = 0; i < strlen(buffer); i++) {
        // Si le caractère courant est un retour à la ligne
        if (buffer[i] == '\r' || buffer[i] == '\n') {
            // Remplacement par le caractère spécifié
            buffer[i] = replacement;
        }
    }
}


static int push_frame(struct websocket_session *session, struct ast_frame *frame)
{
    struct websocket_frame *new_frame;

    new_frame = ast_calloc(1, sizeof(*new_frame));
    if (!new_frame) {
        ast_log(LOG_ERROR, "Failed to allocate memory for new frame\n");
        return -1;
    }

    new_frame->frame = ast_frdup(frame);

    AST_LIST_LOCK(&session->frame_stack);
    AST_LIST_INSERT_HEAD(&session->frame_stack, new_frame, entry);
    AST_LIST_UNLOCK(&session->frame_stack);

    ast_log(LOG_DEBUG, "Frame pushed to stack for session with channel name: %s\n", session->channel_name);
    return 0;
}

static struct ast_frame *pop_frame(struct websocket_session *session)
{
    struct websocket_frame *frame_wrapper;
    struct ast_frame *frame = NULL;

    AST_LIST_LOCK(&session->frame_stack);
    frame_wrapper = AST_LIST_REMOVE_HEAD(&session->frame_stack, entry);
    AST_LIST_UNLOCK(&session->frame_stack);

    if (frame_wrapper) {
        frame = frame_wrapper->frame;
        ast_free(frame_wrapper);
        ast_log(LOG_DEBUG, "Frame popped from stack for session with channel name: %s\n", session->channel_name);
    } else {
        ast_log(LOG_DEBUG, "No frame in stack for session with with channel name: %s\n", session->channel_name);
    }

    return frame;
}

/*! \brief Supplement for adding framehook to sip_session channel */
static struct ast_sip_session_supplement websocket_text_supplement = {
    .method = "INVITE",
    .priority = AST_SIP_SUPPLEMENT_PRIORITY_CHANNEL + 1,
//	.incoming_request = xx_incoming_invite_request,
//	.outgoing_request = xx_outgoing_invite_request,
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


static struct ast_format_cap *set_incoming_call_offer_cap(
    struct ast_sip_session *session, struct ast_sip_session_media *session_media,
    const struct pjmedia_sdp_media *stream)
{
    struct ast_format_cap *incoming_call_offer_cap;
    struct ast_format_cap *remote;
    SCOPE_ENTER(1, "%s\n", ast_sip_session_get_name(session));

    remote = ast_format_cap_alloc(AST_FORMAT_CAP_FLAG_DEFAULT);
    if (!remote) {
        ast_log(LOG_ERROR, "Failed to allocate %s incoming remote capabilities\n",
            ast_codec_media_type2str(session_media->type));
        SCOPE_EXIT_RTN_VALUE(NULL, "Couldn't allocate caps\n");
    }

    /* Get the peer's capabilities*/

    incoming_call_offer_cap = ast_sip_session_create_joint_call_cap(
        session, session_media->type, remote);

    ao2_ref(remote, -1);

    if (!incoming_call_offer_cap || ast_format_cap_empty(incoming_call_offer_cap)) {
        ao2_cleanup(incoming_call_offer_cap);
        SCOPE_EXIT_RTN_VALUE(NULL, "No incoming call offer caps\n");
    }

    SCOPE_EXIT_RTN_VALUE(incoming_call_offer_cap);
}

/*! \brief Function which negotiates an incoming media stream */
static int negotiate_incoming_sdp_stream(struct ast_sip_session *sip_session,
    struct ast_sip_session_media *sip_session_media, const struct pjmedia_sdp_session *sdp,
    int index, struct ast_stream *asterisk_stream)
{
    char host[NI_MAXHOST];
    pjmedia_sdp_media *stream = sdp->media[index];
    struct ast_format_cap *joint;
    struct ast_sip_session_media *session_media_transport;
    int res;
    RAII_VAR(struct ast_sockaddr *, addrs, NULL, ast_free);
    SCOPE_ENTER(1, "%s\n", ast_sip_session_get_name(sip_session));

    ast_debug(3, "websocket negotiate_incoming_sdp_stream for media type '%s' direction output %d\n", ast_codec_media_type2str(sip_session_media->type), sip_session->call_direction);

    if (!sip_session->endpoint->media.websocket_text_configuration.enabled) {
        SCOPE_EXIT_RTN_VALUE(0, "Declining: websocket text configuration not enabled on sip_session\n");
    }

    ast_copy_pj_str(host, stream->conn ? &stream->conn->addr : &sdp->conn->addr, sizeof(host));

    /* Ensure that the address provided is valid */
    if (ast_sockaddr_resolve(&addrs, host, PARSE_PORT_FORBID, AST_AF_UNSPEC) <= 0) {
        /* The provided host was actually invalid so we error out this negotiation */
        SCOPE_EXIT_RTN_VALUE(0, "Declining: provided host is invalid\n");
    }

    /* Check the address family to make sure it matches configured */
    if ((ast_sockaddr_is_ipv6(addrs) && !sip_session->endpoint->media.websocket_text_configuration.ipv6) ||
        (ast_sockaddr_is_ipv4(addrs) && sip_session->endpoint->media.websocket_text_configuration.ipv6)) {
        /* The address does not match configured */
        SCOPE_EXIT_RTN_VALUE(0, "Declining: provided host does not match configured address family\n");
    }

    RAII_VAR(char *, transport_str, ast_strndup(stream->desc.transport.ptr, stream->desc.transport.slen), ast_free);

    if (!transport_str || !strstr(transport_str, "TCP/WSS")) {
        SCOPE_EXIT_RTN_VALUE(-1, "Incompatible transport\n");
    }

    struct websocket_session *ws_session = NULL;

    if (!sip_session_media->websocket_text) {
        sip_session_media->websocket_text = ast_calloc(1, sizeof(*sip_session_media->websocket_text));
        if (!sip_session_media->websocket_text) {
            SCOPE_EXIT_RTN_VALUE(-1, "Couldn't create rtp\n");
        }
        ast_debug(3, "websocket negotiate_incoming_sdp_stream created '%s'\n", ast_codec_media_type2str(sip_session_media->type));
    } else {
        ast_debug(3, "websocket negotiate_incoming_sdp_stream already created '%s'\n", ast_codec_media_type2str(sip_session_media->type));
    }

    if (sip_session->channel && sip_session_media->websocket_text->fd != 0xDEAD) {
        sip_session_media->websocket_text->fd = 0xDEAD;

        ws_session = ast_calloc(1, sizeof(*ws_session));
        if (!ws_session) {
            SCOPE_EXIT_RTN_VALUE(-1, "Failed to allocate memory for websocket session\n");
        }

        strcpy(ws_session->channel_name, "channel_demo");

        AST_LIST_LOCK(&websocket_session_list);
        AST_LIST_INSERT_HEAD(&websocket_session_list, ws_session, entry);
        AST_LIST_UNLOCK(&websocket_session_list);

        // ast_free(ws_session);

        ast_debug(3, "websocket negotiate_incoming_sdp_stream linked with sip session channel name %s\n", ast_channel_name(sip_session->channel));

    } else {
        ast_debug(3, "websocket negotiate_incoming_sdp_stream without sip session channel\n");
    }

    session_media_transport = ast_sip_session_media_get_transport(sip_session, sip_session_media);

    if (session_media_transport == sip_session_media || !sip_session_media->bundled) {
    }

    joint = set_incoming_call_offer_cap(sip_session, sip_session_media, stream);
    ao2_cleanup(joint);

    SCOPE_EXIT_RTN_VALUE(1);
}

/*! \brief Function which creates an outgoing stream */
static int create_outgoing_sdp_stream(struct ast_sip_session *sip_session, struct ast_sip_session_media *sip_session_media,
    struct pjmedia_sdp_session *sdp, const struct pjmedia_sdp_session *remote, struct ast_stream *stream)
{
    pj_pool_t *pool = sip_session->inv_session->pool_prov;
    static const pj_str_t STR_IN = {"IN", 2};
    static const pj_str_t STR_IP4 = {"IP4", 3};
    static const pj_str_t STR_IP6 = {"IP6", 3};
    static const pj_str_t STR_TCP_WSS = {"TCP/WSS", 7};
    static const pj_str_t STR_T140 = {"t140", 4};
    static const pj_str_t STR_RTP_AVP = {"RTP/AVPF", 8};

    pjmedia_sdp_media *media;
    const char *hostip = NULL;
    struct ast_sockaddr addr;

    char tmp[512];
    pj_str_t stmp;
    SCOPE_ENTER(1, "%s Type: %s %s\n", ast_sip_session_get_name(sip_session),
        ast_codec_media_type2str(sip_session_media->type), ast_str_tmp(128, ast_stream_to_str(stream, &STR_TMP)));

    ast_debug(3, "websocket create_outgoing_sdp_stream for media type '%s' direction output %d\n", ast_codec_media_type2str(sip_session_media->type), sip_session->call_direction);

    if (!sip_session->endpoint->media.websocket_text_configuration.enabled) {
        SCOPE_EXIT_RTN_VALUE(1, "Not creating outgoing SDP stream: websocket text not enabled\n");
    } else {
        struct ast_sockaddr temp_media_address;
        struct ast_sockaddr *media_address = &address_ws_text;

        if (sip_session->endpoint->media.bind_rtp_to_media_address && !ast_strlen_zero(sip_session->endpoint->media.address)) {
            if (ast_sockaddr_parse(&temp_media_address, sip_session->endpoint->media.address, 0)) {
                ast_debug_rtp(1, "Endpoint %s: Binding Websocket text media to %s\n",
                    ast_sorcery_object_get_id(sip_session->endpoint),
                    sip_session->endpoint->media.address);
                media_address = &temp_media_address;
            } else {
                ast_debug_rtp(1, "Endpoint %s: Websocket text media address invalid: %s\n",
                    ast_sorcery_object_get_id(sip_session->endpoint),
                    sip_session->endpoint->media.address);
            }
        } else {
            struct ast_sip_transport *transport;

            transport = ast_sorcery_retrieve_by_id(ast_sip_get_sorcery(), "transport",
                sip_session->endpoint->transport);
            if (transport) {
                struct ast_sip_transport_state *trans_state;

                trans_state = ast_sip_get_transport_state(ast_sorcery_object_get_id(transport));
                if (trans_state) {
                    char hoststr[PJ_INET6_ADDRSTRLEN];

                    pj_sockaddr_print(&trans_state->host, hoststr, sizeof(hoststr), 0);
                    if (ast_sockaddr_parse(&temp_media_address, hoststr, 0)) {
                        ast_debug_rtp(1, "Transport %s bound to %s: Using it for Websocket text media.\n",
                            sip_session->endpoint->transport, hoststr);
                        media_address = &temp_media_address;
                    } else {
                        ast_debug_rtp(1, "Transport %s bound to %s: Invalid for Websocket text media.\n",
                            sip_session->endpoint->transport, hoststr);
                    }
                    ao2_ref(trans_state, -1);
                }
                ao2_ref(transport, -1);
            }
        }
    }

    struct websocket_session *ws_session = NULL;

    if (!sip_session_media->websocket_text) {
        sip_session_media->websocket_text = ast_calloc(1, sizeof(*sip_session_media->websocket_text));
        if (!sip_session_media->websocket_text) {
            SCOPE_EXIT_RTN_VALUE(-1, "Couldn't create rtp\n");
        }
        ast_debug(3, "websocket create_outgoing_sdp_stream created '%s'\n", ast_codec_media_type2str(sip_session_media->type));
    } else {
        ast_debug(3, "websocket create_outgoing_sdp_stream already created '%s'\n", ast_codec_media_type2str(sip_session_media->type));
    }

    if (sip_session->channel && sip_session_media->websocket_text->fd != 0xDEAD )
    {
        sip_session_media->websocket_text->fd = 0xDEAD;

        ws_session = ast_calloc(1, sizeof(*ws_session));
        if (!ws_session) {
            SCOPE_EXIT_RTN_VALUE(-1, "Failed to allocate memory for websocket session\n");
        }

        strcpy(ws_session->channel_name, "channel_demo");

        AST_LIST_LOCK(&websocket_session_list);
        AST_LIST_INSERT_HEAD(&websocket_session_list, ws_session, entry);
        AST_LIST_UNLOCK(&websocket_session_list);

        // ast_free(ws_session);

        ast_debug(3, "websocket create_outgoing_sdp_stream linked with sip session channel name %s\n", ast_channel_name(sip_session->channel));
    } else {
        ast_debug(3, "websocket create_outgoing_sdp_stream without sip session channel\n");
    }

    pjmedia_sdp_attr *attr;

#ifndef HAVE_PJSIP_ENDPOINT_COMPACT_FORM
    extern pj_bool_t pjsip_use_compact_form;
#else
    pj_bool_t pjsip_use_compact_form = pjsip_cfg()->endpt.use_compact_form;
#endif

    if (sip_session->call_direction == AST_SIP_SESSION_OUTGOING_CALL) {
        int rtp_code;
        pjmedia_sdp_rtpmap rtpmap;

        if (!(media = pj_pool_zalloc(pool, sizeof(struct pjmedia_sdp_media))) ||
            !(media->conn = pj_pool_zalloc(pool, sizeof(struct pjmedia_sdp_conn)))) {
            SCOPE_EXIT_RTN_VALUE(-1, "Pool alloc failure\n");
        }

        pj_strdup2(pool, &media->desc.media, ast_codec_media_type2str(sip_session_media->type));

        media->desc.transport = STR_RTP_AVP;

        if (ast_strlen_zero(sip_session->endpoint->media.address)) {
            hostip = ast_sip_get_host_ip_string(sip_session->endpoint->media.websocket_text_configuration.ipv6 ? pj_AF_INET6() : pj_AF_INET());
        } else {
            hostip = sip_session->endpoint->media.address;
        }

        if (ast_strlen_zero(hostip)) {
            SCOPE_EXIT_RTN_VALUE(-1, "No local host ip\n");
        }

        media->conn->net_type = STR_IN;
        media->conn->addr_type = sip_session->endpoint->media.websocket_text_configuration.ipv6 ? STR_IP6 : STR_IP4;
        pj_strdup2(pool, &media->conn->addr, hostip);
        media->desc.port = 16922; // (pj_uint16_t)ast_sockaddr_port(&addr);
        media->desc.port_count = 1;

        rtp_code = 99;
        snprintf(tmp, sizeof(tmp), "%d", rtp_code);
        pj_strdup2(pool, &media->desc.fmt[media->desc.fmt_count++], tmp);

        rtpmap.pt = media->desc.fmt[media->desc.fmt_count - 1];

        rtpmap.clock_rate = 1000;
        pj_strdup2(pool, &rtpmap.enc_name, "red");
        pj_cstr(&rtpmap.param, NULL);

        pjmedia_sdp_rtpmap_to_attr(pool, &rtpmap, &attr);
        media->attr[media->attr_count++] = attr;

        rtp_code = rtp_code - 1;

        snprintf(tmp, sizeof(tmp), "%d %d/%d/%d", rtp_code + 1, rtp_code, rtp_code, rtp_code);
        attr = pjmedia_sdp_attr_create(pool, "fmtp", pj_cstr(&stmp, tmp));
        media->attr[media->attr_count++] = attr;

        snprintf(tmp, sizeof(tmp), "%d", rtp_code);
        pj_strdup2(pool, &media->desc.fmt[media->desc.fmt_count++], tmp);

        rtpmap.pt = media->desc.fmt[media->desc.fmt_count - 1];
        rtpmap.clock_rate = 1000;
        pj_strdup2(pool, &rtpmap.enc_name, "t140");
        pj_cstr(&rtpmap.param, NULL);

        pjmedia_sdp_rtpmap_to_attr(pool, &rtpmap, &attr);
        media->attr[media->attr_count++] = attr;

    } else {
        if (!(media = pj_pool_zalloc(pool, sizeof(struct pjmedia_sdp_media)))) {
            SCOPE_EXIT_RTN_VALUE(-1, "Pool alloc failure\n");
        }

        pj_strdup2(pool, &media->desc.media, ast_codec_media_type2str(sip_session_media->type));

        media->desc.transport = STR_TCP_WSS;
        media->desc.port = 8089;
        media->desc.port_count = 1;

        media->desc.fmt[media->desc.fmt_count++] = STR_T140;

        snprintf(tmp, sizeof(tmp), "%s", "wss://dev56.dev.ives.fr:8089/ws_text/channel_demo" );
        attr = pjmedia_sdp_attr_create(pool, tmp, NULL);
        media->attr[media->attr_count++] = attr;
    }

    sdp->media[sdp->media_count++] = media;

    char szSdpBuffer[2048];
    int buf_size;
    buf_size = pjmedia_sdp_print(sdp, szSdpBuffer, 2048);
    if (buf_size >= 0) {
        szSdpBuffer[buf_size] = '\0';
        replace_newline(szSdpBuffer, '#');
        ast_debug(3, "websocket create_outgoing_sdp_stream with sdp %s\n", szSdpBuffer);
    } else {
        ast_debug(3, "websocket create_outgoing_sdp_stream with sdp error %d\n", buf_size);
    }

    SCOPE_EXIT_RTN_VALUE(1, "RC: 1\n");
}

static struct ast_frame *media_sip_session_websocket_text_read_callback(struct ast_sip_session *sip_session, struct ast_sip_session_media *sip_session_media)
{
    struct ast_frame *frame = NULL;

    if (!sip_session_media->websocket_text) {
        return &ast_null_frame;
    }

    // On depile une frame dans une liste alimentée auparavant par le websocket.
    struct websocket_session *ws_session = NULL;

    // Recherche de la session websocket grace au nom du canal asterisk recupere sur la session sip.
    AST_LIST_LOCK(&websocket_session_list);
    AST_LIST_TRAVERSE(&websocket_session_list, ws_session, entry)
    {
        //ast_channel_name(sip_session->channel)

        if (strcmp(ws_session->channel_name, "channel_demo") == 0) {
            frame = pop_frame(ws_session);
            break;
        }
    }
    AST_LIST_UNLOCK(&websocket_session_list);

    if (!frame) {
        return &ast_null_frame;
    }

    //frame->frametype = AST_FRAME_TEXT;
    frame->stream_num = sip_session_media->stream_num;

    return frame;
}

static int media_sip_session_websocket_text_write_callback(struct ast_sip_session *sip_session, struct ast_sip_session_media *sip_session_media, struct ast_frame *frame)
{
    if (!sip_session_media->websocket_text) {
        return 0;
    }

    if (frame && frame->frametype == AST_FRAME_TEXT) {
        struct ast_websocket *websocket = NULL;
        struct websocket_session *ws_session = NULL;

        // Recherche de la session websocket grace au nom du canal asterisk recupere sur la session sip.
        AST_LIST_LOCK(&websocket_session_list);
        AST_LIST_TRAVERSE(&websocket_session_list, ws_session, entry)
        {
            //ast_channel_name(sip_session->channel)

            if (strcmp(ws_session->channel_name, "channel_demo") == 0) {
                websocket = ws_session->websocket;
                break;
            }
        }
        AST_LIST_UNLOCK(&websocket_session_list);

        if (websocket) {
            char *payload = frame->data.ptr;
            uint64_t payload_len = frame->datalen - 1;
            enum ast_websocket_opcode opcode = AST_WEBSOCKET_OPCODE_TEXT;

            ast_websocket_write(websocket, opcode, payload, payload_len);
        }
    }

    return 0;
}

/*! \brief Function which applies a negotiated stream */
static int apply_negotiated_sdp_stream(struct ast_sip_session *sip_session,
    struct ast_sip_session_media *sip_session_media, const struct pjmedia_sdp_session *local,
    const struct pjmedia_sdp_session *remote, int index, struct ast_stream *asterisk_stream)
{
    RAII_VAR(struct ast_sockaddr *, addrs, NULL, ast_free);
    pjmedia_sdp_media *remote_stream = remote->media[index];
    char host[NI_MAXHOST];
    SCOPE_ENTER(1, "%s Stream: %s\n", ast_sip_session_get_name(sip_session),
        ast_str_tmp(128, ast_stream_to_str(asterisk_stream, &STR_TMP)));

    if (!sip_session->channel) {
        SCOPE_EXIT_RTN_VALUE(1, "No channel\n");
    }

    ast_debug(3, "websocket apply_negotiated_sdp_stream for media type '%s' with channel name %s direction output %d\n"
        , ast_codec_media_type2str(sip_session_media->type) 
        , ast_channel_name(sip_session->channel)
        , sip_session->call_direction
        );

    char szSdpBuffer[2048];
    int buf_size;
    buf_size = pjmedia_sdp_print(local, szSdpBuffer, 2048);
    if (buf_size >= 0) {
        szSdpBuffer[buf_size] = '\0';
        replace_newline(szSdpBuffer, '#');
        ast_debug(3, "websocket apply_negotiated_sdp_stream with local sdp %s\n", szSdpBuffer);
    } else {
        ast_debug(3, "websocket apply_negotiated_sdp_stream with local sdp error %d\n", buf_size);
    }
    buf_size = pjmedia_sdp_print(remote, szSdpBuffer, 2048);
    if (buf_size >= 0) {
        szSdpBuffer[buf_size] = '\0';
        replace_newline(szSdpBuffer, '#');
        ast_debug(3, "websocket apply_negotiated_sdp_stream with remote sdp %s\n", szSdpBuffer);
    } else {
        ast_debug(3, "websocket apply_negotiated_sdp_stream with remote sdp error %d (remote %d)\n", buf_size, remote ? 1 : 0);
    }

    ast_copy_pj_str(host, remote_stream->conn ? &remote_stream->conn->addr : &remote->conn->addr, sizeof(host));

    /* Ensure that the address provided is valid */
    if (ast_sockaddr_resolve(&addrs, host, PARSE_PORT_FORBID, AST_AF_UNSPEC) <= 0) {
        /* The provided host was actually invalid so we error out this negotiation */
        SCOPE_EXIT_RTN_VALUE(-1, "Not applying negotiated SDP stream: failed to resolve remote stream host\n");
    }

    struct websocket_session *ws_session = NULL;

    if (!sip_session_media->websocket_text) {
        sip_session_media->websocket_text = ast_calloc(1, sizeof(*sip_session_media->websocket_text));
        if (!sip_session_media->websocket_text) {
            SCOPE_EXIT_RTN_VALUE(-1, "Couldn't create rtp\n");
        }
        ast_debug(3, "websocket apply_negotiated_sdp_stream created '%s'\n", ast_codec_media_type2str(sip_session_media->type));
    } else {
        ast_debug(3, "websocket apply_negotiated_sdp_stream already created '%s'\n", ast_codec_media_type2str(sip_session_media->type));
    }

    if (sip_session->channel && sip_session_media->websocket_text->fd != 0xDEAD) {
        sip_session_media->websocket_text->fd = 0xDEAD;

        ws_session = ast_calloc(1, sizeof(*ws_session));
        if (!ws_session) {
            SCOPE_EXIT_RTN_VALUE(-1, "Failed to allocate memory for websocket session\n");
        }

        strcpy(ws_session->channel_name, "channel_demo");

        AST_LIST_LOCK(&websocket_session_list);
        AST_LIST_INSERT_HEAD(&websocket_session_list, ws_session, entry);
        AST_LIST_UNLOCK(&websocket_session_list);

        // ast_free(ws_session);

        ast_debug(3, "websocket apply_negotiated_sdp_stream linked with sip session channel name %s\n", ast_channel_name(sip_session->channel));
    } else {
        ast_debug(3, "websocket apply_negotiated_sdp_stream already linked with sip session channel name %s\n", ast_channel_name(sip_session->channel));
    }

    ast_sip_session_media_set_write_callback(sip_session, sip_session_media, media_sip_session_websocket_text_write_callback);
    ast_sip_session_media_add_read_callback(sip_session
        , sip_session_media
        , ast_websocket_text_fd(sip_session_media->websocket_text)
        , media_sip_session_websocket_text_read_callback
        );

    SCOPE_EXIT_RTN_VALUE(1, "Handled\n");
}

/*! \brief Function which destroys the Websocket Text instance when sip_session ends */
static void stream_destroy(struct ast_sip_session_media *sip_session_media)
{
    if (sip_session_media->websocket_text) {
        ast_websocket_text_destroy(sip_session_media->websocket_text);
    }
    sip_session_media->websocket_text = NULL;
}

/*! \brief SDP handler for 'application' media stream */
static struct ast_sip_session_sdp_handler text_sdp_handler = {
    .id = "test_ws", //STR_TEXT,
    .defer_incoming_sdp_stream = NULL,
    .negotiate_incoming_sdp_stream = negotiate_incoming_sdp_stream,
    .create_outgoing_sdp_stream = create_outgoing_sdp_stream,
    .apply_negotiated_sdp_stream = apply_negotiated_sdp_stream,
    .change_outgoing_sdp_stream_media_address = NULL,
    .stream_stop = NULL,
    .stream_destroy = stream_destroy,
};

/*! \brief Unloads the SIP Websocket Text module from Asterisk */
static int unload_module(void)
{
    struct websocket_session *ws_session = NULL;

    ast_sip_session_unregister_sdp_handler(&text_sdp_handler, STR_TEXT);
    ast_sip_session_unregister_supplement(&websocket_text_supplement);

    AST_LIST_LOCK(&websocket_session_list);
    AST_LIST_TRAVERSE_SAFE_BEGIN(&websocket_session_list, ws_session, entry)
    {
        struct websocket_frame *frame_wrapper = NULL;
        //struct ast_frame *frame = NULL;

        AST_LIST_REMOVE_CURRENT(entry);
        if (ws_session->websocket) {
            ast_websocket_unref(ws_session->websocket);
        }
        /**/
        AST_LIST_TRAVERSE_SAFE_BEGIN(&ws_session->frame_stack, frame_wrapper, entry)
        {
            ast_frfree(frame_wrapper->frame);
            ast_free(frame_wrapper);
        }
        AST_LIST_TRAVERSE_SAFE_END;
        /**/
        /*
        frame = pop_frame(ws_session);
        while (frame != NULL) {
            ast_frfree(frame);
            frame = pop_frame(ws_session);
        }
        */

        ast_free(ws_session);
    }
    AST_LIST_TRAVERSE_SAFE_END;
    AST_LIST_UNLOCK(&websocket_session_list);

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
        ast_sockaddr_parse(&address_ws_text, "::", 0);
    } else {
        ast_sockaddr_parse(&address_ws_text, "0.0.0.0", 0);
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
