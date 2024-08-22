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
    <depend>res_pjsip_sdp_rtp</depend>
    <depend>res_http_websocket</depend>
    <support_level>core</support_level>
 ***/

#include "asterisk.h"

#include <pjsip.h>
#include <pjsip_ua.h>
#include <pjmedia.h>
#include <pjlib.h>

#include "asterisk/module.h"
#include "asterisk/astobj2.h"
#include "asterisk/strings.h"
#include "asterisk/file.h"

#include "asterisk/channel.h"
#include "asterisk/stream.h"
#include "asterisk/format_cache.h"
#include "asterisk/http.h"
#include "asterisk/http_websocket.h"

#include "asterisk/res_pjsip.h"
#include "asterisk/res_pjsip_session.h"
#include "asterisk/res_pjsip_session_caps.h"

static const char STR_TEXT[] = "text";

/*! \brief Websocket Text information */
struct ast_websocket_session_text
{
    char id[256];
    int pipe_fds[2];
    struct ast_websocket *websocket;

    AST_LIST_ENTRY(ast_websocket_session_text) entry;
};

static AST_LIST_HEAD(websocket_session_text_list, ast_websocket_session_text) websocket_session_text_list;

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

static int get_websocket_tls_port(void)
{
    struct ast_config *cfg;
    struct ast_flags config_flags = {0};
    int websocket_tls_port = 8089;

    // Charger le fichier de configuration http.conf
    cfg = ast_config_load("http.conf", config_flags);
    if (cfg) {
        const char *port_str = ast_variable_retrieve(cfg, "general", "tlsbindaddr");
        if (port_str) {
            const char *port_start = strrchr(port_str, ':');
            if (port_start) {
                websocket_tls_port = atoi(port_start + 1);
            }
        }

        ast_config_destroy(cfg);
    } else {
        ast_log(LOG_ERROR, "Failed to load http.conf\n");
    }

    return websocket_tls_port;
}

/*! \brief Supplement for adding framehook to sip_session channel */
static struct ast_sip_session_supplement websocket_session_text_supplement = {
    .method = "INVITE",
    .priority = AST_SIP_SUPPLEMENT_PRIORITY_CHANNEL + 1,
};

static int ast_websocket_session_text_fd(const struct ast_websocket_session_text *ws_text)
{
    return ws_text->pipe_fds[0];
}

/*! \brief Destructor for T.38 state information */
static void ast_websocket_session_text_destroy(void *obj)
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

    // Ajouter le format T.140/RED
    if (ast_format_cap_append(remote, ast_format_t140_red, 0) != 0) {
        ast_log(LOG_ERROR, "Failed to add T.140/RED format\n");
        SCOPE_EXIT_RTN_VALUE(NULL, "Impossible to add T.140/RED in call offer caps\n");
    }

    // Ajouter le format T.140
    if (ast_format_cap_append(remote, ast_format_t140, 0) != 0) {
        ast_log(LOG_ERROR, "Failed to add T.140 format\n");
        SCOPE_EXIT_RTN_VALUE(NULL, "Impossible to add t140 in call offer caps\n");
    }

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
    int res;
    RAII_VAR(struct ast_sockaddr *, addrs, NULL, ast_free);
    SCOPE_ENTER(1, "%s\n", ast_sip_session_get_name(sip_session));

    if (!sip_session->endpoint->media.websocket_text_configuration.enabled) {
        SCOPE_EXIT_RTN_VALUE(0, "Declining: websocket text configuration not enabled on sip_session\n");
    }

    ast_copy_pj_str(host, stream->conn ? &stream->conn->addr : &sdp->conn->addr, sizeof(host));

    /* Ensure that the address provided is valid */
    if (ast_sockaddr_resolve(&addrs, host, PARSE_PORT_FORBID, AST_AF_UNSPEC) <= 0) {
        /* The provided host was actually invalid so we error out this negotiation */
        SCOPE_EXIT_RTN_VALUE(0, "Declining: provided host is invalid\n");
    }

    /* If no type formats have been configured reject this stream */
    if (!ast_format_cap_has_type(sip_session->endpoint->media.codecs, sip_session_media->type)) {
        ast_debug(3, "Endpoint has no codecs for media type '%s', declining stream\n",
            ast_codec_media_type2str(sip_session_media->type));
        SCOPE_EXIT_RTN_VALUE(0, "Endpoint has no codecs\n");
    }

    RAII_VAR(char *, transport_str, ast_strndup(stream->desc.transport.ptr, stream->desc.transport.slen), ast_free);

    if (!transport_str || !strstr(transport_str, "TCP/WSS")) {
        SCOPE_EXIT_RTN_VALUE(0, "Incompatible transport\n");
    }

    if (sip_session->inv_session) {
        if (!sip_session_media->websocket_session_text) {
            sip_session_media->websocket_session_text = ast_calloc(1, sizeof(*sip_session_media->websocket_session_text));
            if (!sip_session_media->websocket_session_text) {
                SCOPE_EXIT_RTN_VALUE(-1, "Failed to allocate memory for websocket session\n");
            }

            strcpy(sip_session_media->websocket_session_text->id, sip_session->inv_session->obj_name + strlen("inv0x"));

            AST_LIST_LOCK(&websocket_session_text_list);
            AST_LIST_INSERT_HEAD(&websocket_session_text_list, sip_session_media->websocket_session_text, entry);
            AST_LIST_UNLOCK(&websocket_session_text_list);

            ast_debug(3, "websocket negotiate_incoming_sdp_stream created '%s' with id %s\n", ast_codec_media_type2str(sip_session_media->type), sip_session_media->websocket_session_text->id);
        } else {
            ast_debug(3, "websocket negotiate_incoming_sdp_stream already created '%s' with id %s\n", ast_codec_media_type2str(sip_session_media->type), sip_session_media->websocket_session_text->id);
        }
    } else {
        SCOPE_EXIT_RTN_VALUE(-1, "Failed to generate id for websocket session\n");
    }

    joint = set_incoming_call_offer_cap(sip_session, sip_session_media, stream);
    ast_stream_set_formats(asterisk_stream, joint);
    ao2_cleanup(joint);

    ast_stream_set_state(asterisk_stream, AST_STREAM_STATE_SENDRECV);

    SCOPE_EXIT_RTN_VALUE(1);
}

/*! \brief Function which creates an outgoing stream */
static int create_outgoing_sdp_stream(struct ast_sip_session *sip_session, struct ast_sip_session_media *sip_session_media,
    struct pjmedia_sdp_session *sdp, const struct pjmedia_sdp_session *remote, struct ast_stream *asterisk_stream)
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
        ast_codec_media_type2str(sip_session_media->type), ast_str_tmp(128, ast_stream_to_str(asterisk_stream, &STR_TMP)));

    if (!sip_session->endpoint->media.websocket_text_configuration.enabled) {
        SCOPE_EXIT_RTN_VALUE(1, "Not creating outgoing SDP stream: websocket text not enabled\n");
    }

    if (sip_session->inv_session) {
        if (!sip_session_media->websocket_session_text) {
            sip_session_media->websocket_session_text = ast_calloc(1, sizeof(*sip_session_media->websocket_session_text));
            if (!sip_session_media->websocket_session_text) {
                SCOPE_EXIT_RTN_VALUE(-1, "Failed to allocate memory for websocket session\n");
            }

            strcpy(sip_session_media->websocket_session_text->id, sip_session->inv_session->obj_name + strlen("inv0x"));

            AST_LIST_LOCK(&websocket_session_text_list);
            AST_LIST_INSERT_HEAD(&websocket_session_text_list, sip_session_media->websocket_session_text, entry);
            AST_LIST_UNLOCK(&websocket_session_text_list);

            ast_debug(3, "websocket create_outgoing_sdp_stream created '%s' with id %s\n", ast_codec_media_type2str(sip_session_media->type), sip_session_media->websocket_session_text->id);
        } else {
            ast_debug(3, "websocket create_outgoing_sdp_stream already created '%s' with id %s\n", ast_codec_media_type2str(sip_session_media->type), sip_session_media->websocket_session_text->id);
        }
    } else {
        SCOPE_EXIT_RTN_VALUE(-1, "Failed to generate id for websocket session\n");
    }

    pjmedia_sdp_attr *attr;
    const pj_str_t *hostname = pj_gethostname();

#ifndef HAVE_PJSIP_ENDPOINT_COMPACT_FORM
    extern pj_bool_t pjsip_use_compact_form;
#else
    pj_bool_t pjsip_use_compact_form = pjsip_cfg()->endpt.use_compact_form;
#endif

    if (!(media = pj_pool_zalloc(pool, sizeof(struct pjmedia_sdp_media)))) {
        SCOPE_EXIT_RTN_VALUE(-1, "Pool alloc failure\n");
    }

    pj_strdup2(pool, &media->desc.media, ast_codec_media_type2str(sip_session_media->type));

    media->desc.transport = STR_TCP_WSS;
    media->desc.port = get_websocket_tls_port();
    media->desc.port_count = 1;

    media->desc.fmt[media->desc.fmt_count++] = STR_T140;

    if (sip_session->inv_session) {
        snprintf(tmp, sizeof(tmp), "wss://%s:%d/ws_text/%s", hostname ? hostname->ptr : "localhost", media->desc.port, sip_session->inv_session->obj_name + strlen( "inv0x"));
    } else {
        snprintf(tmp, sizeof(tmp), "wss://%s:%d/ws_text/%s", hostname ? hostname->ptr : "localhost", media->desc.port, "channel_demo");
    }

    attr = pjmedia_sdp_attr_create(pool, tmp, NULL);
    media->attr[media->attr_count++] = attr;

    sdp->media[sdp->media_count++] = media;

    SCOPE_EXIT_RTN_VALUE(1, "RC: 1\n");
}

static struct ast_frame *media_sip_session_websocket_session_text_read_callback(struct ast_sip_session *sip_session, struct ast_sip_session_media *sip_session_media)
{
    struct ast_frame *frame = NULL;

    if (!sip_session_media->websocket_session_text) {
        return &ast_null_frame;
    }

    ssize_t bytes_read;
    size_t buffer_capacity = 396;
    char *final_buffer = NULL;
    size_t total_length = 0;
    int done = 0;

    final_buffer = (char *)ast_malloc(buffer_capacity);
    if (final_buffer == NULL) {
        ast_log(LOG_ERROR, "websocket text malloc failed\n");
    }

    while (!done) {
        if (total_length >= buffer_capacity) {
            buffer_capacity *= 2;
            final_buffer = (char *)ast_realloc(final_buffer, buffer_capacity);
            if (final_buffer == NULL) {
                ast_log(LOG_ERROR, "websocket text realloc failed\n");
                total_length = 0;
                done = 1;
            }
        }

        bytes_read = read(ast_websocket_session_text_fd(sip_session_media->websocket_session_text), final_buffer + total_length, buffer_capacity - total_length);
        if (bytes_read > 0) {
            ast_debug(3, "Reading websocket text string, length %" PRIu64 "\n", bytes_read);
            total_length += bytes_read;
            if (bytes_read < (ssize_t)(buffer_capacity - total_length)) {
                done = 1;
            }
            done = 1;

        } else if (bytes_read == 0) {
            ast_log(LOG_WARNING, "Reading websocket text EOF\n");
            done = 1;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                ast_log(LOG_WARNING, "websocket text read no additional data available\n");
                done = 1;
            } else {
                ast_log(LOG_ERROR, "websocket text read failed: %s\n", strerror(errno));
                done = 1;
            }
        }
    }

    if (total_length > 0) {
        struct ast_websocket_session_text *ws_session = NULL;

        AST_LIST_LOCK(&websocket_session_text_list);
        AST_LIST_TRAVERSE(&websocket_session_text_list, ws_session, entry)
        {
            if (!strcmp(ws_session->id, sip_session->inv_session->obj_name + strlen("inv0x"))) {
                struct ast_frame f_in;

                memset(&f_in, 0, sizeof(f_in));
                f_in.frametype = AST_FRAME_TEXT;
                //f_in.subclass.format = ast_format_none;
                //f_in.subclass.format = ast_format_t140;
                f_in.subclass.format = ast_format_t140_red;
                f_in.datalen = total_length;
                f_in.data.ptr = (void *)final_buffer;

                frame = ast_frdup(&f_in);

                ast_debug(3, "Reading websocket text string, total length %" PRIu64 "\n", total_length);

                break;
            }
        }
        AST_LIST_UNLOCK(&websocket_session_text_list);

        ast_free(final_buffer);
    }

    if (!frame) {
        ast_log(LOG_ERROR, "websocket text session or frame not found, length %" PRIu64 " loss\n", total_length);
        return NULL; // Error.
    }

    frame->stream_num = sip_session_media->stream_num;

    return frame;
}

static int media_sip_session_websocket_session_text_write_callback(struct ast_sip_session *sip_session, struct ast_sip_session_media *sip_session_media, struct ast_frame *frame)
{
    if (!sip_session_media->websocket_session_text) {
        return 0;
    }

    if (frame && frame->frametype == AST_FRAME_TEXT) {
        struct ast_websocket *websocket = NULL;
        struct ast_websocket_session_text *ws_session = NULL;

        // Recherche de la session websocket grace au nom du canal asterisk recupere sur la session sip.
        AST_LIST_LOCK(&websocket_session_text_list);
        AST_LIST_TRAVERSE(&websocket_session_text_list, ws_session, entry)
        {
            if (!strcmp(ws_session->id, sip_session->inv_session->obj_name + strlen("inv0x"))) {
                websocket = ws_session->websocket;
                break;
            }
        }
        AST_LIST_UNLOCK(&websocket_session_text_list);

        if (frame->datalen > 0) {
            char *text = frame->data.ptr;

            if (text[frame->datalen - 1] != '\0') {
                /* Not zero terminated, we need to allocate */
                text = ast_strndup(text, frame->datalen);
            }

            if (text) {
                uint64_t text_len = strlen(text);
                if (websocket) {
                    enum ast_websocket_opcode opcode = AST_WEBSOCKET_OPCODE_TEXT;

                    ast_websocket_write(websocket, opcode, text, text_len);
                } else {
                    ast_log(LOG_ERROR, "websocket text session not found, length %" PRIu64 " loss\n", text_len);
                }

                if (text != frame->data.ptr) {
                    /* Only free if we allocated */
                    ast_free(text);
                }
            }
        }
    }

    return 0;
}

static int set_caps(struct ast_sip_session *session,
    struct ast_sip_session_media *session_media,
    const struct pjmedia_sdp_media *stream,
    int is_offer, struct ast_stream *asterisk_stream)
{
    RAII_VAR(struct ast_format_cap *, caps, NULL, ao2_cleanup);
    RAII_VAR(struct ast_format_cap *, peer, NULL, ao2_cleanup);
    RAII_VAR(struct ast_format_cap *, joint, NULL, ao2_cleanup);
    enum ast_media_type media_type = session_media->type;
    int direct_media_enabled = !ast_sockaddr_isnull(&session_media->direct_media_addr) &&
        ast_format_cap_count(session->direct_media_cap);
    int dsp_features = 0;
    SCOPE_ENTER(1, "%s %s\n", ast_sip_session_get_name(session), is_offer ? "OFFER" : "ANSWER");

    if (!(caps = ast_format_cap_alloc(AST_FORMAT_CAP_FLAG_DEFAULT)) ||
        !(peer = ast_format_cap_alloc(AST_FORMAT_CAP_FLAG_DEFAULT)) ||
        !(joint = ast_format_cap_alloc(AST_FORMAT_CAP_FLAG_DEFAULT))) {
        ast_log(LOG_ERROR, "Failed to allocate %s capabilities\n",
            ast_codec_media_type2str(session_media->type));
        SCOPE_EXIT_RTN_VALUE(-1, "Couldn't create %s capabilities\n",
            ast_codec_media_type2str(session_media->type));
    }

    /* get the endpoint capabilities */
    if (direct_media_enabled) {
        ast_format_cap_get_compatible(session->endpoint->media.codecs, session->direct_media_cap, caps);
    } else {
        ast_format_cap_append_from_cap(caps, session->endpoint->media.codecs, media_type);
    }

    // Ajouter le format T.140/RED
    if (ast_format_cap_append(peer, ast_format_t140_red, 0) != 0) {
        ast_log(LOG_ERROR, "Failed to add T.140/RED format\n");
        SCOPE_EXIT_RTN_VALUE(-1, "Impossible to add T.140/RED in call offer caps\n");
    }

    // Ajouter le format T.140
    if (ast_format_cap_append(peer, ast_format_t140, 0) != 0) {
        ast_log(LOG_ERROR, "Failed to add T.140 format\n");
        SCOPE_EXIT_RTN_VALUE(-1, "Impossible to add t140 in call offer caps\n");
    }

    /* get the joint capabilities between peer and endpoint */
    ast_format_cap_get_compatible(caps, peer, joint);

    ast_stream_set_formats(asterisk_stream, joint);

    if (session->channel && ast_sip_session_is_pending_stream_default(session, asterisk_stream)) {
        ast_channel_lock(session->channel);

        ast_format_cap_remove_by_type(caps, AST_MEDIA_TYPE_UNKNOWN);
        ast_format_cap_append_from_cap(caps, ast_channel_nativeformats(session->channel),
            AST_MEDIA_TYPE_UNKNOWN);
        ast_format_cap_remove_by_type(caps, media_type);

        ast_format_cap_append_from_cap(caps, joint, media_type);

        /*
         * Apply the new formats to the channel, potentially changing
         * raw read/write formats and translation path while doing so.
         */
        ast_channel_nativeformats_set(session->channel, caps);

        if (ast_channel_is_bridged(session->channel)) {
            ast_channel_set_unbridged_nolock(session->channel, 1);
        }

        ast_channel_unlock(session->channel);
    }

    SCOPE_EXIT_RTN_VALUE(0);
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

    RAII_VAR(char *, transport_str, ast_strndup(remote_stream->desc.transport.ptr, remote_stream->desc.transport.slen), ast_free);

    if (!transport_str || !strstr(transport_str, "TCP/WSS")) {
        SCOPE_EXIT_RTN_VALUE(0, "Incompatible transport\n");
    }

    ast_copy_pj_str(host, remote_stream->conn ? &remote_stream->conn->addr : &remote->conn->addr, sizeof(host));

    /* Ensure that the address provided is valid */
    if (ast_sockaddr_resolve(&addrs, host, PARSE_PORT_FORBID, AST_AF_UNSPEC) <= 0) {
        /* The provided host was actually invalid so we error out this negotiation */
        SCOPE_EXIT_RTN_VALUE(-1, "Not applying negotiated SDP stream: failed to resolve remote stream host\n");
    }

    if (sip_session->inv_session) {
        if (!sip_session_media->websocket_session_text) {
            sip_session_media->websocket_session_text = ast_calloc(1, sizeof(*sip_session_media->websocket_session_text));
            if (!sip_session_media->websocket_session_text) {
                SCOPE_EXIT_RTN_VALUE(-1, "Failed to allocate memory for websocket session\n");
            }

            strcpy(sip_session_media->websocket_session_text->id, sip_session->inv_session->obj_name + strlen("inv0x"));

            AST_LIST_LOCK(&websocket_session_text_list);
            AST_LIST_INSERT_HEAD(&websocket_session_text_list, sip_session_media->websocket_session_text, entry);
            AST_LIST_UNLOCK(&websocket_session_text_list);

            ast_debug(3, "websocket apply_negotiated_sdp_stream created '%s' with id %s\n", ast_codec_media_type2str(sip_session_media->type), sip_session_media->websocket_session_text->id);
        } else {
            ast_debug(3, "websocket apply_negotiated_sdp_stream already created '%s' with id %s\n", ast_codec_media_type2str(sip_session_media->type), sip_session_media->websocket_session_text->id);
        }
    } else {
        SCOPE_EXIT_RTN_VALUE(-1, "Failed to generate id for websocket session\n");
    }

    ast_sip_session_media_set_write_callback(sip_session, sip_session_media, media_sip_session_websocket_session_text_write_callback);

    if (pipe(sip_session_media->websocket_session_text->pipe_fds) == -1 || sip_session_media->websocket_session_text == NULL) {
        SCOPE_EXIT_RTN_VALUE(-1, "pipe create to exchange frames failed\n");
    } else {
        //ast_fd_set_flags(ast_websocket_session_text_fd(sip_session_media->websocket_session_text), O_NONBLOCK);
        
        ast_sip_session_media_add_read_callback(sip_session
            , sip_session_media
            , ast_websocket_session_text_fd(sip_session_media->websocket_session_text)
            , media_sip_session_websocket_session_text_read_callback
            );
    }

    if (set_caps(sip_session, sip_session_media, remote_stream, 0, asterisk_stream)) {
        SCOPE_EXIT_RTN_VALUE(-1, "set_caps failed\n");
    }

    SCOPE_EXIT_RTN_VALUE(1, "Handled\n");
}

/*! \brief Function which destroys the Websocket Text instance when sip_session ends */
static void stream_destroy(struct ast_sip_session_media *sip_session_media)
{
    if (sip_session_media->websocket_session_text) {
        struct ast_websocket_session_text *ws_session = NULL;

        AST_LIST_LOCK(&websocket_session_text_list);
        AST_LIST_TRAVERSE_SAFE_BEGIN(&websocket_session_text_list, ws_session, entry)
        {
            if (sip_session_media->websocket_session_text == ws_session) {
                AST_LIST_REMOVE_CURRENT(entry);

                if (ws_session->websocket) {
                    ast_debug(3, "websocket text unref websocket with id %s\n", sip_session_media->websocket_session_text->id);

                    ast_websocket_unref(ws_session->websocket);
                    ws_session->websocket = NULL;
                }
                ast_debug(3, "websocket text deleted with id %s\n", sip_session_media->websocket_session_text->id);
                break;
            }
        }
        AST_LIST_TRAVERSE_SAFE_END;
        AST_LIST_UNLOCK(&websocket_session_text_list);

        if (sip_session_media->websocket_session_text->pipe_fds[0] > 0) {
            close(sip_session_media->websocket_session_text->pipe_fds[0]);
            sip_session_media->websocket_session_text->pipe_fds[0] = -1;
        }
        if (sip_session_media->websocket_session_text->pipe_fds[1] > 0) {
            close(sip_session_media->websocket_session_text->pipe_fds[1]);
            sip_session_media->websocket_session_text->pipe_fds[1] = -1;
        }

        ast_websocket_session_text_destroy(sip_session_media->websocket_session_text);
    }

    sip_session_media->websocket_session_text = NULL;
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

//========== WEBSOCKET SERVER ==========
//======================================

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

static int websocket_text_t140_uri_cb(struct ast_tcptls_session_instance *ser
    , const struct ast_http_uri *urih
    , const char *uri
    , enum ast_http_method method
    , struct ast_variable *get_params
    , struct ast_variable *headers
    )
{
    struct ast_websocket_session_text *ws_session = NULL;

    ast_debug(1, "Entering websocket text t140 loop method %s uri %s\n", ast_get_http_method(method), uri);

    // Recherche de la session websocket grace au nom du canal asterisk recupere sur la session sip.
    AST_LIST_LOCK(&websocket_session_text_list);
    AST_LIST_TRAVERSE(&websocket_session_text_list, ws_session, entry)
    {
        if (!strcmp(ws_session->id, uri)) {
            break;
        }
    }
    AST_LIST_UNLOCK(&websocket_session_text_list);

    if (!ws_session) {
        ast_http_error(ser, 403, "Access Denied", "You do not have permission to access the requested URL.");
        return 0;
    }

    add_variable_to_list(&get_params, "uri", uri);

    return ast_websocket_uri_cb(ser, urih, uri, method, get_params, headers);
}

static struct ast_http_uri websocket_text_t140_uri = {
    .callback = websocket_text_t140_uri_cb,
    .description = "Asterisk HTTP WebSocket text t140",
    .uri = "ws_text",
    .has_subtree = 1,
    .data = NULL,
    .key = __FILE__,
};

/*! \brief Simple echo implementation which echoes received text and binary frames */
static void websocket_session_text_t140_callback(struct ast_websocket *websocket, struct ast_variable *parameters, struct ast_variable *headers)
{
    int res;
    struct ast_websocket_session_text *ws_session = NULL;

    ast_debug(1, "Entering websocket text t140 loop %s, addr remote %s and local %s\n"
        , ast_websocket_session_id(websocket)
        , ast_sockaddr_stringify(ast_websocket_remote_address(websocket))
        , ast_sockaddr_stringify(ast_websocket_local_address(websocket))
        );

    struct ast_variable *i;
    for (i = parameters; i; i = i->next) {
        if (!strcmp(i->name, "uri")) {
            AST_LIST_LOCK(&websocket_session_text_list);
            AST_LIST_TRAVERSE(&websocket_session_text_list, ws_session, entry)
            {
                if (!strcmp(ws_session->id, i->value)) {
                    break;
                }
            }
            AST_LIST_UNLOCK(&websocket_session_text_list);
        }
    }
    if (!ws_session) {
        ast_log(LOG_ERROR, "Failed to find uri or allocate memory for websocket client\n");
        goto end;
    }

    if (ast_fd_set_flags(ast_websocket_fd(websocket), O_NONBLOCK)) {
        goto end;
    }

    if (!ws_session->websocket) {
        ast_websocket_ref(websocket);
        ws_session->websocket = websocket;
    }

    while ((res = ast_websocket_wait_for_input(websocket, -1)) > 0) {
        char *payload;
        uint64_t payload_len;
        enum ast_websocket_opcode opcode;
        int fragmented;

        if (ast_websocket_read(websocket, &payload, &payload_len, &opcode, &fragmented)) {
            // We err on the side of caution and terminate the ws_session if any error occurs.
            ast_log(LOG_WARNING, "Read failure during websocket t140 loop\n");
            break;
        }

        if (opcode == AST_WEBSOCKET_OPCODE_TEXT && payload_len > 0) {
            //write(ws_session->pipe_fds[1], payload, payload_len);
            size_t offset = 0;
            while (offset < payload_len) {
                size_t bytes_to_write = (payload_len - offset >= 396) ? 396 : (payload_len - offset);

                write(ws_session->pipe_fds[1], payload + offset, bytes_to_write);

                offset += bytes_to_write;
            }
        } else if (opcode == AST_WEBSOCKET_OPCODE_CLOSE) {
            break;
        } else {
            ast_debug(1, "Ignored websocket text t140 opcode %u\n", opcode );
        }
    }

end:
    ast_debug(1, "Exiting websocket text t140 loop %s\n", ast_websocket_session_id(websocket));

    AST_LIST_LOCK(&websocket_session_text_list);
    AST_LIST_TRAVERSE_SAFE_BEGIN(&websocket_session_text_list, ws_session, entry)
    {
        if (ws_session->websocket == websocket) {
            ast_debug(3, "websocket text unref websocket (callback) with id %s\n", ws_session->id);

            ast_websocket_unref(ws_session->websocket);
            ws_session->websocket = NULL;
            break;
        }
    }
    AST_LIST_TRAVERSE_SAFE_END;
    AST_LIST_UNLOCK(&websocket_session_text_list);
}

/*! \brief Unloads the SIP Websocket Text module from Asterisk */
static int unload_module(void)
{
    struct ast_websocket_session_text *ws_session = NULL;

    ast_sip_session_unregister_sdp_handler(&text_sdp_handler, STR_TEXT);
    ast_sip_session_unregister_supplement(&websocket_session_text_supplement);

    ast_websocket_server_remove_protocol(websocket_text_t140_uri.data, "t140", websocket_session_text_t140_callback);
    ast_http_uri_unlink(&websocket_text_t140_uri);
    ao2_ref(websocket_text_t140_uri.data, -1);
    websocket_text_t140_uri.data = NULL;

    AST_LIST_LOCK(&websocket_session_text_list);
    AST_LIST_TRAVERSE_SAFE_BEGIN(&websocket_session_text_list, ws_session, entry)
    {
        AST_LIST_REMOVE_CURRENT(entry);
        if (ws_session->websocket) {
            ast_debug(3, "websocket text unref websocket (unload) with id %s\n", ws_session->id);

            ast_websocket_unref(ws_session->websocket);
            ws_session->websocket = NULL;
        }

        ast_debug(3, "websocket text deleted (unload) with id %s\n", ws_session->id);

        ast_free(ws_session);
    }
    AST_LIST_TRAVERSE_SAFE_END;
    AST_LIST_UNLOCK(&websocket_session_text_list);

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
    websocket_text_t140_uri.data = ast_websocket_server_create();
    if (!websocket_text_t140_uri.data) {
        return AST_MODULE_LOAD_DECLINE;
    }
    ast_http_uri_link(&websocket_text_t140_uri);
    ast_websocket_server_add_protocol(websocket_text_t140_uri.data, "t140", websocket_session_text_t140_callback);

    ast_sip_session_register_supplement(&websocket_session_text_supplement);

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
    .requires = "res_pjsip,res_pjsip_session,res_pjsip_sdp_rtp,res_http_websocket",
    );

