#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "protocol.h"

#include "csapp.h"
#include "debug.h"

ssize_t header_size(void) {
    return sizeof(BRS_PACKET_HEADER);
}

// returns -1 if error, otherwise 0
int validate_header_packet(BRS_PACKET_HEADER *hdr) {
    // check if header packet is null
    if (hdr == NULL) {
        error("null packet header given");

        return -1;
    }

    // check if header packet is correct size
    if (header_size() != sizeof(*hdr)) {
        error("header size is not the correct size; expected: %lu, actual: %lu", header_size(), sizeof(*hdr));

        debug("type is: %d size %lu", hdr->type, sizeof(hdr->type));
        debug("size is: %u size %lu", hdr->size, sizeof(hdr->size));
        debug("timestamp_sec is: %u size %lu", hdr->timestamp_sec, sizeof(hdr->timestamp_sec));
        debug("timestamp_nsec is: %u size %lu", hdr->timestamp_nsec, sizeof(hdr->timestamp_nsec));

        return -1;
    }

    // check that header packet is an expected type
    if ( hdr->type != BRS_NO_PKT &&
        hdr->type != BRS_LOGIN_PKT &&
        hdr->type != BRS_STATUS_PKT &&
        hdr->type != BRS_DEPOSIT_PKT &&
        hdr->type != BRS_WITHDRAW_PKT &&
        hdr->type != BRS_ESCROW_PKT &&
        hdr->type != BRS_RELEASE_PKT &&
        hdr->type != BRS_BUY_PKT &&
        hdr->type != BRS_SELL_PKT &&
        hdr->type != BRS_CANCEL_PKT &&
        hdr->type != BRS_ACK_PKT &&
        hdr->type != BRS_NACK_PKT &&
        hdr->type != BRS_BOUGHT_PKT &&
        hdr->type != BRS_SOLD_PKT &&
        hdr->type != BRS_POSTED_PKT &&
        hdr->type != BRS_CANCELED_PKT &&
        hdr->type != BRS_TRADED_PKT
    ) {
        error("invalid packet type");

        return -1;
    }

    return 0;
}

// TODO: check if file-descriptor exists?
int proto_send_packet(int fd, BRS_PACKET_HEADER *hdr, void *payload) {
    // validate the header packet
    if (validate_header_packet(hdr) == -1) {
        error("proto_send_packet header error");

        return -1;
    }

    // check that if a header-payload size is 0, that the payload is null
    if (hdr->size == 0 && payload != NULL) {
        error("payload found when header not expected");

        return -1;
    }

    // check that if a payload is null, that the header payload size is 0
    if (payload == NULL && hdr->size != 0) {
        error("header expected payload but none was given");

        return -1;
    }

    // set errno here to clear
    errno = 0;

    // write out the header
    ssize_t wrote_bytes = write(fd, hdr, header_size());

    // we aren't writing this anymore, so use host order
    // debug("hdr->size is: %u", hdr->size);
    // hdr->size = ntohs(hdr->size);
    // debug("hdr->size is now: %u", hdr->size);

    // check for short write
    if (wrote_bytes != header_size()) {
        error("header short write (errno: %u), expected: %lu, actual: %lu", errno, header_size(), wrote_bytes);

        return -1;
    }

    // check if payload is correct size
    // int *ptr_tmp = (int *)payload;
    if (payload != NULL) {
        // if (sizeof(*ptr_tmp) != hdr->size) {
        //     error("payload given, but isn't the expected size; expected: %u, actual: %lu", hdr->size, sizeof(*ptr_tmp));

        //     return -1;
        // }

        // payload isn't null, so write out the payload
        errno = 0;
        wrote_bytes = write(fd, payload, ntohs(hdr->size));

        // check for a short write
        if (wrote_bytes != ntohs(hdr->size)) {
            error("payload short write (errno: %u), expected: %u, actual: %lu", errno, ntohs(hdr->size), wrote_bytes);

            return -1;
        }
    } else {
        // payload is NULL so we're done.
        debug("successful write - no payload");

        return 0;
    }

    // success
    debug("successful write - has payload");

    return 0;
}

// char *bytes_from_network_to_host(char *bytes) {

// }

int proto_recv_packet(int fd, BRS_PACKET_HEADER *hdr, void **payloadp) {
    // set errno here to clear
    errno = 0;

    // read out the header into supplied storage hdr
    ssize_t read_bytes = read(fd, hdr, header_size());

    // check for EOF
    if (read_bytes == 0) {
        warn("reached EOF");

        return -1;
    }

    // after reading, convert from host to net
    // uint16_t tmp_type = htons(hdr->type);
    // debug("the real type is: %u", tmp_type);
    hdr->size = htons(hdr->size);
    hdr->timestamp_sec = htonl(hdr->timestamp_sec);
    hdr->timestamp_nsec = htonl(hdr->timestamp_nsec);

    // check for short write
    if (read_bytes != header_size()) {
        error("header short read (errno: %u), expected: %lu, actual: %lu", errno, header_size(), read_bytes);

        return -1;
    }

    // validate the header packet
    if (validate_header_packet(hdr) == -1) {
        error("proto_recv_packet header error");

        return -1;
    }

    if (hdr->size != 0) {
        errno = 0;

        // need to read in payload
        char *read_payload;
        read_payload = Malloc(hdr->size);
        read_bytes = read(fd, read_payload, hdr->size);

        // after reading, convert from host to net
        // *read_payload = htonl(*read_payload);

        // check for a short read
        if (read_bytes != hdr->size) {
            error("payload short read (errno: %u), expected: %u, actual: %lu", errno, hdr->size, read_bytes);

            return -1;
        }

        // read correct amt of bytes
        read_payload[read_bytes] = '\0';
        *payloadp = (void *)read_payload;

        debug("type is: %d", hdr->type);
        debug("size is: %u", hdr->size);
        debug("payload is: %s", read_payload);
    } else {
        // no payload to read
        debug("successful read - no payload");

        return 0;
    }

    // success
    debug("successful read - has payload");

    return 0;
}
