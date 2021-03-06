/*
 * Copyright (C) 2019 by Sukchan Lee <acetcom@gmail.com>
 *
 * This file is part of Open5GS.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ogs-pfcp.h"

ogs_sock_t *ogs_pfcp_server(ogs_socknode_t *node)
{
    char buf[OGS_ADDRSTRLEN];
    ogs_sock_t *pfcp;
    ogs_assert(node);

    pfcp = ogs_udp_server(node);
    ogs_assert(pfcp);

    ogs_info("pfcp_server() [%s]:%d",
            OGS_ADDR(node->addr, buf), OGS_PORT(node->addr));

    return pfcp;
}

int ogs_pfcp_connect(
        ogs_sock_t *ipv4, ogs_sock_t *ipv6, ogs_pfcp_node_t *node)
{
    ogs_sockaddr_t *addr;
    char buf[OGS_ADDRSTRLEN];

    ogs_assert(ipv4 || ipv6);
    ogs_assert(node);
    ogs_assert(node->sa_list);

    addr = node->sa_list;
    while (addr) {
        ogs_sock_t *sock = NULL;

        if (addr->ogs_sa_family == AF_INET)
            sock = ipv4;
        else if (addr->ogs_sa_family == AF_INET6)
            sock = ipv6;
        else
            ogs_assert_if_reached();

        if (sock) {
            ogs_info("ogs_pfcp_connect() [%s]:%d",
                    OGS_ADDR(addr, buf), OGS_PORT(addr));

            node->sock = sock;
            memcpy(&node->addr, addr, sizeof node->addr);
            break;
        }

        addr = addr->next;
    }

    if (addr == NULL) {
        ogs_error("ogs_pfcp_connect() [%s]:%d failed",
                OGS_ADDR(node->sa_list, buf), OGS_PORT(node->sa_list));
        ogs_error("Please check the IP version between SMF and UPF nodes.");
        return OGS_ERROR;
    }

    return OGS_OK;
}

int ogs_pfcp_send(ogs_pfcp_node_t *node, ogs_pkbuf_t *pkbuf)
{
    ssize_t sent;
    ogs_sock_t *sock = NULL;

    ogs_assert(node);
    ogs_assert(pkbuf);
    sock = node->sock;
    ogs_assert(sock);

    sent = ogs_send(sock->fd, pkbuf->data, pkbuf->len, 0);
    if (sent < 0 || sent != pkbuf->len) {
        ogs_log_message(OGS_LOG_ERROR, ogs_socket_errno,
                "ogs_pfcp_send() failed");
        return OGS_ERROR;
    }

    return OGS_OK;
}

int ogs_pfcp_sendto(ogs_pfcp_node_t *node, ogs_pkbuf_t *pkbuf)
{
    ssize_t sent;
    ogs_sock_t *sock = NULL;
    ogs_sockaddr_t *addr = NULL;

    ogs_assert(node);
    ogs_assert(pkbuf);
    sock = node->sock;
    ogs_assert(sock);
    addr = &node->addr;
    ogs_assert(addr);

    sent = ogs_sendto(sock->fd, pkbuf->data, pkbuf->len, 0, addr);
    if (sent < 0 || sent != pkbuf->len) {
        ogs_log_message(OGS_LOG_ERROR, ogs_socket_errno,
                "ogs_pfcp_sendto() failed");
        return OGS_ERROR;
    }

    return OGS_OK;
}

void ogs_pfcp_send_heartbeat_request(ogs_pfcp_node_t *node,
        void (*cb)(ogs_pfcp_xact_t *xact, void *data))
{
    int rv;
    ogs_pkbuf_t *pkbuf = NULL;
    ogs_pfcp_header_t h;
    ogs_pfcp_xact_t *xact = NULL;

    ogs_assert(node);

    memset(&h, 0, sizeof(ogs_pfcp_header_t));
    h.type = OGS_PFCP_HEARTBEAT_REQUEST_TYPE;
    h.seid = 0;

    pkbuf = ogs_pfcp_build_heartbeat_request(h.type);
    ogs_expect_or_return(pkbuf);

    xact = ogs_pfcp_xact_local_create(node, &h, pkbuf, cb, node);
    ogs_expect_or_return(xact);

    rv = ogs_pfcp_xact_commit(xact);
    ogs_expect(rv == OGS_OK);
}

void ogs_pfcp_send_heartbeat_response(ogs_pfcp_xact_t *xact)
{
    int rv;
    ogs_pkbuf_t *pkbuf = NULL;
    ogs_pfcp_header_t h;

    ogs_assert(xact);

    memset(&h, 0, sizeof(ogs_pfcp_header_t));
    h.type = OGS_PFCP_HEARTBEAT_RESPONSE_TYPE;
    h.seid = 0;

    pkbuf = ogs_pfcp_build_heartbeat_response(h.type);
    ogs_expect_or_return(pkbuf);

    rv = ogs_pfcp_xact_update_tx(xact, &h, pkbuf);
    ogs_expect_or_return(rv == OGS_OK);

    rv = ogs_pfcp_xact_commit(xact);
    ogs_expect(rv == OGS_OK);
}

void ogs_pfcp_cp_send_association_setup_request(ogs_pfcp_node_t *node,
        void (*cb)(ogs_pfcp_xact_t *xact, void *data))
{
    int rv;
    ogs_pkbuf_t *pkbuf = NULL;
    ogs_pfcp_header_t h;
    ogs_pfcp_xact_t *xact = NULL;

    ogs_assert(node);

    memset(&h, 0, sizeof(ogs_pfcp_header_t));
    h.type = OGS_PFCP_ASSOCIATION_SETUP_REQUEST_TYPE;
    h.seid = 0;

    pkbuf = ogs_pfcp_cp_build_association_setup_request(h.type);
    ogs_expect_or_return(pkbuf);

    xact = ogs_pfcp_xact_local_create(node, &h, pkbuf, cb, node);
    ogs_expect_or_return(xact);

    rv = ogs_pfcp_xact_commit(xact);
    ogs_expect(rv == OGS_OK);
}

void ogs_pfcp_cp_send_association_setup_response(ogs_pfcp_xact_t *xact,
        uint8_t cause)
{
    int rv;
    ogs_pkbuf_t *pkbuf = NULL;
    ogs_pfcp_header_t h;

    ogs_assert(xact);

    memset(&h, 0, sizeof(ogs_pfcp_header_t));
    h.type = OGS_PFCP_ASSOCIATION_SETUP_RESPONSE_TYPE;
    h.seid = 0;

    pkbuf = ogs_pfcp_cp_build_association_setup_response(h.type, cause);
    ogs_expect_or_return(pkbuf);

    rv = ogs_pfcp_xact_update_tx(xact, &h, pkbuf);
    ogs_expect_or_return(rv == OGS_OK);

    rv = ogs_pfcp_xact_commit(xact);
    ogs_expect(rv == OGS_OK);
}

void ogs_pfcp_up_send_association_setup_request(ogs_pfcp_node_t *node,
        void (*cb)(ogs_pfcp_xact_t *xact, void *data))
{
    int rv;
    ogs_pkbuf_t *pkbuf = NULL;
    ogs_pfcp_header_t h;
    ogs_pfcp_xact_t *xact = NULL;

    ogs_assert(node);

    memset(&h, 0, sizeof(ogs_pfcp_header_t));
    h.type = OGS_PFCP_ASSOCIATION_SETUP_REQUEST_TYPE;
    h.seid = 0;

    pkbuf = ogs_pfcp_up_build_association_setup_request(h.type);
    ogs_expect_or_return(pkbuf);

    xact = ogs_pfcp_xact_local_create(node, &h, pkbuf, cb, node);
    ogs_expect_or_return(xact);

    rv = ogs_pfcp_xact_commit(xact);
    ogs_expect(rv == OGS_OK);
}

void ogs_pfcp_up_send_association_setup_response(ogs_pfcp_xact_t *xact,
        uint8_t cause)
{
    int rv;
    ogs_pkbuf_t *pkbuf = NULL;
    ogs_pfcp_header_t h;

    ogs_assert(xact);

    memset(&h, 0, sizeof(ogs_pfcp_header_t));
    h.type = OGS_PFCP_ASSOCIATION_SETUP_RESPONSE_TYPE;
    h.seid = 0;

    pkbuf = ogs_pfcp_up_build_association_setup_response(h.type, cause);
    ogs_expect_or_return(pkbuf);

    rv = ogs_pfcp_xact_update_tx(xact, &h, pkbuf);
    ogs_expect_or_return(rv == OGS_OK);

    rv = ogs_pfcp_xact_commit(xact);
    ogs_expect(rv == OGS_OK);
}

void ogs_pfcp_send_g_pdu(ogs_pfcp_pdr_t *pdr, ogs_pkbuf_t *sendbuf)
{
    char buf[OGS_ADDRSTRLEN];
    int rv;
    ogs_gtp_header_t *gtp_h = NULL;
    ogs_gtp_extension_header_t *ext_h = NULL;
    ogs_gtp_node_t *gnode = NULL;

    ogs_pfcp_far_t *far = NULL;
    ogs_pfcp_qer_t *qer = NULL;

    ogs_assert(pdr);

    far = pdr->far;
    if (!far) {
        ogs_error("No FAR");
        return;
    }

    gnode = far->gnode;
    ogs_assert(gnode);
    ogs_assert(gnode->sock);
    ogs_assert(sendbuf);

    qer = pdr->qer;

    /* Add GTP-U header */
    if (qer && qer->qfi) {
        ogs_assert(ogs_pkbuf_push(sendbuf, OGS_GTPV1U_5GC_HEADER_LEN));
        gtp_h = (ogs_gtp_header_t *)sendbuf->data;
        /* Bits    8  7  6  5  4  3  2  1
         *        +--+--+--+--+--+--+--+--+
         *        |version |PT| 1| E| S|PN|
         *        +--+--+--+--+--+--+--+--+
         *         0  0  1   1  0  1  0  0
         */
        gtp_h->flags = 0x34;
        gtp_h->type = OGS_GTPU_MSGTYPE_GPDU;
        gtp_h->length = htobe16(sendbuf->len - OGS_GTPV1U_HEADER_LEN);
        gtp_h->teid = htobe32(far->outer_header_creation.teid);

        ext_h = (ogs_gtp_extension_header_t *)(
                sendbuf->data + OGS_GTPV1U_HEADER_LEN);
        ext_h->type = OGS_GTP_EXTENSION_HEADER_TYPE_PDU_SESSION_CONTAINER;
        ext_h->len = 1;
        ext_h->pdu_type =
            OGS_GTP_EXTENSION_HEADER_PDU_TYPE_DL_PDU_SESSION_INFORMATION;
        ext_h->qos_flow_identifier = qer->qfi;
        ext_h->next_type =
            OGS_GTP_EXTENSION_HEADER_TYPE_NO_MORE_EXTENSION_HEADERS;
    } else {
        ogs_assert(ogs_pkbuf_push(sendbuf, OGS_GTPV1U_HEADER_LEN));
        gtp_h = (ogs_gtp_header_t *)sendbuf->data;
        /* Bits    8  7  6  5  4  3  2  1
         *        +--+--+--+--+--+--+--+--+
         *        |version |PT| 1| E| S|PN|
         *        +--+--+--+--+--+--+--+--+
         *         0  0  1   1  0  0  0  0
         */
        gtp_h->flags = 0x30;
        gtp_h->type = OGS_GTPU_MSGTYPE_GPDU;
        gtp_h->length = htobe16(sendbuf->len - OGS_GTPV1U_HEADER_LEN);
        gtp_h->teid = htobe32(far->outer_header_creation.teid);
    }

    /* Send G-PDU */
    ogs_debug("SEND G-PDU to Peer[%s] : TEID[0x%x]",
        OGS_ADDR(&gnode->addr, buf), far->outer_header_creation.teid);
    rv = ogs_gtp_sendto(gnode, sendbuf);
    if (rv != OGS_OK) {
        if (ogs_socket_errno != OGS_EAGAIN) {
            ogs_error("SEND G-PDU to Peer[%s] : TEID[0x%x]",
                OGS_ADDR(&gnode->addr, buf), far->outer_header_creation.teid);
        }
    }

    ogs_pkbuf_free(sendbuf);
}

void ogs_pfcp_send_end_marker(ogs_pfcp_pdr_t *pdr)
{
    char buf[OGS_ADDRSTRLEN];
    int rv;
    ogs_pkbuf_t *sendbuf = NULL;

    ogs_gtp_header_t *gtp_h = NULL;
    ogs_gtp_extension_header_t *ext_h = NULL;
    ogs_gtp_node_t *gnode = NULL;

    ogs_pfcp_far_t *far = NULL;
    ogs_pfcp_qer_t *qer = NULL;

    ogs_assert(pdr);
    far = pdr->far;
    ogs_assert(far);

    gnode = far->gnode;
    if (!gnode) {
        ogs_error("No GTP Node Setup");
        return;
    }
    if (!gnode->sock) {
        ogs_error("No GTP Socket Setup");
        return;
    }

    sendbuf = ogs_pkbuf_alloc(NULL,
            100 /* enough for END_MARKER; use smaller buffer */);
    ogs_assert(sendbuf);
    ogs_pkbuf_put(sendbuf, 100);
    memset(sendbuf->data, 0, sendbuf->len);

    gtp_h = (ogs_gtp_header_t *)sendbuf->data;

    qer = pdr->qer;

    /* Add GTP-U header */
    if (qer && qer->qfi) {
        /* Bits    8  7  6  5  4  3  2  1
         *        +--+--+--+--+--+--+--+--+
         *        |version |PT| 1| E| S|PN|
         *        +--+--+--+--+--+--+--+--+
         *         0  0  1   1  0  1  0  0
         */
        gtp_h->flags = 0x34;
        gtp_h->type = OGS_GTPU_MSGTYPE_END_MARKER;
        gtp_h->teid = htobe32(far->outer_header_creation.teid);

        ext_h = (ogs_gtp_extension_header_t *)(
                sendbuf->data + OGS_GTPV1U_HEADER_LEN);
        ext_h->type = OGS_GTP_EXTENSION_HEADER_TYPE_PDU_SESSION_CONTAINER;
        ext_h->len = 1;
        ext_h->pdu_type =
            OGS_GTP_EXTENSION_HEADER_PDU_TYPE_DL_PDU_SESSION_INFORMATION;
        ext_h->qos_flow_identifier = qer->qfi;
        ext_h->next_type =
            OGS_GTP_EXTENSION_HEADER_TYPE_NO_MORE_EXTENSION_HEADERS;
    } else {
        /* Bits    8  7  6  5  4  3  2  1
         *        +--+--+--+--+--+--+--+--+
         *        |version |PT| 1| E| S|PN|
         *        +--+--+--+--+--+--+--+--+
         *         0  0  1   1  0  0  0  0
         */
        gtp_h->flags = 0x30;
        gtp_h->type = OGS_GTPU_MSGTYPE_END_MARKER;
        gtp_h->teid = htobe32(far->outer_header_creation.teid);
    }

    /* Send End Marker */
    ogs_debug("SEND End Marker to Peer[%s] : TEID[0x%x]",
        OGS_ADDR(&gnode->addr, buf), far->outer_header_creation.teid);
    rv = ogs_gtp_sendto(gnode, sendbuf);
    if (rv != OGS_OK) {
        if (ogs_socket_errno != OGS_EAGAIN) {
            ogs_error("SEND End Marker to Peer[%s] : TEID[0x%x]",
                OGS_ADDR(&gnode->addr, buf), far->outer_header_creation.teid);
        }
    }

    ogs_pkbuf_free(sendbuf);
}

void ogs_pfcp_send_buffered_packet(ogs_pfcp_pdr_t *pdr)
{
    ogs_pfcp_far_t *far = NULL;
    int i;

    ogs_assert(pdr);
    far = pdr->far;

    if (far && far->gnode) {
        if (far->apply_action & OGS_PFCP_APPLY_ACTION_FORW) {
            for (i = 0; i < far->num_of_buffered_packet; i++) {
                ogs_pfcp_send_g_pdu(pdr, far->buffered_packet[i]);
            }
            far->num_of_buffered_packet = 0;
        }
    }
}

void ogs_pfcp_send_error_message(
    ogs_pfcp_xact_t *xact, uint64_t seid, uint8_t type,
    uint8_t cause_value, uint16_t offending_ie_value)
{
    int rv;
    ogs_pfcp_message_t errmsg;
    ogs_pfcp_tlv_cause_t *cause = NULL;
    ogs_pfcp_tlv_offending_ie_t *offending_ie = NULL;
    ogs_pkbuf_t *pkbuf = NULL;

    ogs_assert(xact);

    memset(&errmsg, 0, sizeof(ogs_pfcp_message_t));
    errmsg.h.seid = seid;
    errmsg.h.type = type;

    switch (type) {
    case OGS_PFCP_PFD_MANAGEMENT_RESPONSE_TYPE:
        cause = &errmsg.pfcp_pfd_management_response.cause;
        offending_ie = &errmsg.pfcp_pfd_management_response.offending_ie;
        break;
    case OGS_PFCP_ASSOCIATION_SETUP_RESPONSE_TYPE:
        cause = &errmsg.pfcp_association_setup_response.cause;
        break;
    case OGS_PFCP_ASSOCIATION_UPDATE_RESPONSE_TYPE:
        cause = &errmsg.pfcp_association_update_response.cause;
        break;
    case OGS_PFCP_ASSOCIATION_RELEASE_RESPONSE_TYPE:
        cause = &errmsg.pfcp_association_release_response.cause;
        break;
    case OGS_PFCP_NODE_REPORT_RESPONSE_TYPE:
        cause = &errmsg.pfcp_node_report_response.cause;
        offending_ie = &errmsg.pfcp_node_report_response.offending_ie;
        break;
    case OGS_PFCP_SESSION_SET_DELETION_RESPONSE_TYPE:
        cause = &errmsg.pfcp_session_set_deletion_response.cause;
        offending_ie = &errmsg.pfcp_session_set_deletion_response.offending_ie;
        break;
    case OGS_PFCP_SESSION_ESTABLISHMENT_RESPONSE_TYPE:
        cause = &errmsg.pfcp_session_establishment_response.cause;
        offending_ie = &errmsg.pfcp_session_establishment_response.offending_ie;
        break;
    case OGS_PFCP_SESSION_MODIFICATION_RESPONSE_TYPE:
        cause = &errmsg.pfcp_session_modification_response.cause;
        offending_ie = &errmsg.pfcp_session_modification_response.offending_ie;
        break;
    case OGS_PFCP_SESSION_DELETION_RESPONSE_TYPE:
        cause = &errmsg.pfcp_session_deletion_response.cause;
        offending_ie = &errmsg.pfcp_session_deletion_response.offending_ie;
        break;
    case OGS_PFCP_SESSION_REPORT_RESPONSE_TYPE:
        cause = &errmsg.pfcp_session_report_response.cause;
        offending_ie = &errmsg.pfcp_session_report_response.offending_ie;
        break;
    default:
        ogs_assert_if_reached();
        return;
    }

    ogs_assert(cause);

    cause->presence = 1;
    cause->u8 = cause_value;

    if (offending_ie && offending_ie_value) {
        offending_ie->presence = 1;
        offending_ie->u16 = offending_ie_value;
    }

    pkbuf = ogs_pfcp_build_msg(&errmsg);
    ogs_expect_or_return(pkbuf);

    rv = ogs_pfcp_xact_update_tx(xact, &errmsg.h, pkbuf);
    ogs_expect_or_return(rv == OGS_OK);

    rv = ogs_pfcp_xact_commit(xact);
    ogs_expect(rv == OGS_OK);
}
