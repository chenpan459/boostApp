#pragma once

#include "domain/message.hpp"
#include "namespace.hpp"

NV_NS_DOMAIN_BEGIN

class IEventStore {
public:
    virtual ~IEventStore() = default;
    virtual void append(const Message& message) = 0;
};

NV_NS_DOMAIN_END
