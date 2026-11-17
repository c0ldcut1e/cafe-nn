/**
 * @file
 * @brief Games derive from this and register it with BackEndServices to receive notification events, which arrive on the Dispatch thread.
 */

#pragma once

#include <nn/nex/auth/NotificationEvent.h>

namespace nn::nex
{

    /**
     * @brief Games derive from this and register it with BackEndServices to receive notification events, which arrive on the Dispatch thread.
     */
    class NotificationEventHandler
    {
    public:
        virtual ~NotificationEventHandler()                                   = default;
        virtual void ProcessNotificationEvent(const NotificationEvent &event) = 0;
    };

} // namespace nn::nex
