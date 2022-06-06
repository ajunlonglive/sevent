#ifndef SEVENT_EXAMPLES_HUB_HUBCODEC_H
#define SEVENT_EXAMPLES_HUB_HUBCODEC_H

#include "sevent/net/Buffer.h"

namespace pubsub {
using std::string;
enum ParseResult {
    kError,
    kSuccess,
    kContinue,
};
ParseResult parseMessage(sevent::net::Buffer *buf, string *cmd, string *topic,
                         string *content);
}

#endif