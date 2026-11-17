/**
 * @file
 * @brief This includes every nn::nex header.
 */

#pragma once

#include <nn/nex/auth/AuthenticationInfo.h>
#include <nn/nex/auth/BackEndServices.h>
#include <nn/nex/auth/KerberosEncryption.h>
#include <nn/nex/auth/NexServiceToken.h>
#include <nn/nex/auth/NgsFacade.h>
#include <nn/nex/auth/NintendoLoginData.h>
#include <nn/nex/auth/NotificationEvent.h>
#include <nn/nex/auth/RVConnectionData.h>
#include <nn/nex/auth/SecureConnectionProtocolClient.h>
#include <nn/nex/auth/Ticket.h>
#include <nn/nex/auth/TicketGrantingProtocolClient.h>
#include <nn/nex/core/ByteStream.h>
#include <nn/nex/core/StationURL.h>
#include <nn/nex/matchmaking/AutoMatchmakeParam.h>
#include <nn/nex/matchmaking/CreateMatchmakeSessionParam.h>
#include <nn/nex/matchmaking/Gathering.h>
#include <nn/nex/matchmaking/JoinMatchmakeSessionParam.h>
#include <nn/nex/matchmaking/MatchMakingClient.h>
#include <nn/nex/matchmaking/MatchmakeDefs.h>
#include <nn/nex/matchmaking/MatchmakeExtensionClient.h>
#include <nn/nex/matchmaking/MatchmakeParam.h>
#include <nn/nex/matchmaking/MatchmakeSession.h>
#include <nn/nex/matchmaking/MatchmakeSessionSearchCriteria.h>
#include <nn/nex/matchmaking/PersistentGathering.h>
#include <nn/nex/matchmaking/PlayingSession.h>
#include <nn/nex/matchmaking/UpdateMatchmakeSessionParam.h>
#include <nn/nex/nat/NATTraversal.h>
#include <nn/nex/platform/RandomNumberGenerator.h>
#include <nn/nex/platform/SocketDriver.h>
#include <nn/nex/platform/Time.h>
#include <nn/nex/rmc/EndPoint.h>
#include <nn/nex/rmc/Message.h>
#include <nn/nex/runtime/CallContext.h>
#include <nn/nex/runtime/Scheduler.h>
#include <nn/nex/transport/PRUDPEndPoint.h>
#include <nn/nex/types.h>
