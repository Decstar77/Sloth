#include "sloth_network.h"

// vcpkg built GameNetworkingSockets as a static lib (x64-windows-static-md),
// but its headers default to dllimport linkage unless told otherwise - this
// must be defined before the headers below are included.
#define STEAMNETWORKINGSOCKETS_STATIC_LINK

#include <GameNetworkingSockets/steam/steamnetworkingsockets.h>
#include <GameNetworkingSockets/steam/isteamnetworkingutils.h>

#include <cstring>
#include <vector>

namespace sloth {

    // ------------------------------------------------------------------------
    // GameNetworkingSockets is itself a process-wide singleton (there is one
    // global ISteamNetworkingSockets instance, reached via the free function
    // SteamNetworkingSockets()), and its connection-status callback is a
    // plain C function pointer with no user-data parameter. s_activeInstance
    // lets that free function reach back into whichever NetworkSystem::Impl
    // is currently alive - safe because only one NetworkSystem may exist per
    // process (enforced in the constructor).
    // ------------------------------------------------------------------------

    struct NetworkSystem::Impl {
        ISteamNetworkingSockets * sockets = nullptr;
        HSteamListenSocket        listenSocket = k_HSteamListenSocket_Invalid;
        HSteamNetPollGroup        pollGroup = k_HSteamNetPollGroup_Invalid;

        std::vector<NetEvent>     pendingEvents;
        const NetEvent *          events = nullptr;
        usize                     eventCount = 0;

        void HandleStatusChanged( SteamNetConnectionStatusChangedCallback_t * info );
    };

    static NetworkSystem::Impl * s_activeInstance = nullptr;

    static NetConnectionState ToNetConnectionState( ESteamNetworkingConnectionState state ) {
        switch ( state ) {
            case k_ESteamNetworkingConnectionState_Connecting:
            case k_ESteamNetworkingConnectionState_FindingRoute:
                return NetConnectionState::Connecting;
            case k_ESteamNetworkingConnectionState_Connected:
                return NetConnectionState::Connected;
            case k_ESteamNetworkingConnectionState_ClosedByPeer:
                return NetConnectionState::ClosedByPeer;
            default:
                return NetConnectionState::ProblemDetectedLocally;
        }
    }

    static void OnConnectionStatusChanged( SteamNetConnectionStatusChangedCallback_t * info ) {
        if ( s_activeInstance != nullptr ) {
            s_activeInstance->HandleStatusChanged( info );
        }
    }

    void NetworkSystem::Impl::HandleStatusChanged( SteamNetConnectionStatusChangedCallback_t * info ) {
        switch ( info->m_info.m_eState ) {
            case k_ESteamNetworkingConnectionState_Connecting:
                // Only report this as an event on the server side - a peer
                // connecting into one of our listen sockets is a decision we
                // need the caller to make (AcceptConnection/CloseConnection).
                // On the client side this same state just means "still
                // dialing", which isn't actionable.
                if ( info->m_info.m_hListenSocket != k_HSteamListenSocket_Invalid ) {
                    pendingEvents.push_back( NetEvent { NetEventType::IncomingConnection, NetConnection { (u32)info->m_hConn } } );
                }
                break;

            case k_ESteamNetworkingConnectionState_Connected:
                sockets->SetConnectionPollGroup( info->m_hConn, pollGroup );
                pendingEvents.push_back( NetEvent { NetEventType::Connected, NetConnection { (u32)info->m_hConn } } );
                break;

            case k_ESteamNetworkingConnectionState_ClosedByPeer:
            case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
                NetEvent event {};
                event.type = NetEventType::Disconnected;
                event.connection = NetConnection { (u32)info->m_hConn };
                event.closeState = ToNetConnectionState( info->m_info.m_eState );
                pendingEvents.push_back( event );

                // GNS still owns resources for the handle until we close it
                // locally, even though the peer (or a local network problem)
                // already ended the connection.
                sockets->CloseConnection( info->m_hConn, 0, nullptr, false );
                break;
            }

            default:
                break;
        }
    }

    NetworkSystem::NetworkSystem() : impl( std::make_unique<Impl>() ) {
        SL_ASSERT_MSG( s_activeInstance == nullptr, "NetworkSystem: only one instance may exist per process" );

        SteamNetworkingErrMsg errMsg;
        bool ok = GameNetworkingSockets_Init( nullptr, errMsg );
        SL_ASSERT_MSG( ok, "GameNetworkingSockets_Init failed: %s", errMsg );
        SL_UNUSED( ok );

        impl->sockets = SteamNetworkingSockets();
        impl->pollGroup = impl->sockets->CreatePollGroup();

        s_activeInstance = impl.get();
        SteamNetworkingUtils()->SetGlobalCallback_SteamNetConnectionStatusChanged( &OnConnectionStatusChanged );
    }

    NetworkSystem::~NetworkSystem() {
        if ( impl->listenSocket != k_HSteamListenSocket_Invalid ) {
            impl->sockets->CloseListenSocket( impl->listenSocket );
        }
        impl->sockets->DestroyPollGroup( impl->pollGroup );

        s_activeInstance = nullptr;
        GameNetworkingSockets_Kill();
    }

    NetConnection NetworkSystem::Connect( const char * address ) {
        SteamNetworkingIPAddr addr;
        if ( !addr.ParseString( address ) ) {
            SL_LOG_ERROR( "NetworkSystem::Connect: failed to parse address '%s'", address );
            return NetConnection {};
        }

        HSteamNetConnection conn = impl->sockets->ConnectByIPAddress( addr, 0, nullptr );
        if ( conn == k_HSteamNetConnection_Invalid ) {
            SL_LOG_ERROR( "NetworkSystem::Connect: ConnectByIPAddress failed for '%s'", address );
            return NetConnection {};
        }

        impl->sockets->SetConnectionPollGroup( conn, impl->pollGroup );
        return NetConnection { (u32)conn };
    }

    bool NetworkSystem::Listen( u16 port ) {
        SteamNetworkingIPAddr addr;
        addr.Clear(); // [::]:0 - all interfaces
        addr.m_port = port;

        impl->listenSocket = impl->sockets->CreateListenSocketIP( addr, 0, nullptr );
        if ( impl->listenSocket == k_HSteamListenSocket_Invalid ) {
            SL_LOG_ERROR( "NetworkSystem::Listen: CreateListenSocketIP failed for port %u", (u32)port );
            return false;
        }

        return true;
    }

    void NetworkSystem::AcceptConnection( NetConnection connection ) {
        EResult result = impl->sockets->AcceptConnection( (HSteamNetConnection)connection.Id );
        if ( result != k_EResultOK ) {
            SL_LOG_WARN( "NetworkSystem::AcceptConnection: AcceptConnection failed with EResult %d", (int)result );
        }
    }

    void NetworkSystem::CloseConnection( NetConnection connection ) {
        impl->sockets->CloseConnection( (HSteamNetConnection)connection.Id, 0, nullptr, false );
    }

    void NetworkSystem::SendMessage( NetConnection connection, const void * data, usize size, NetSendType sendType ) {
        int flags = sendType == NetSendType::Reliable ? k_nSteamNetworkingSend_Reliable : k_nSteamNetworkingSend_Unreliable;
        impl->sockets->SendMessageToConnection( (HSteamNetConnection)connection.Id, data, (uint32)size, flags, nullptr );
    }

    void NetworkSystem::Update( Arena & arena ) {
        impl->pendingEvents.clear();

        impl->sockets->RunCallbacks();

        static constexpr int MaxMessagesPerPoll = 64;
        SteamNetworkingMessage_t * messages[MaxMessagesPerPoll];

        for ( ;; ) {
            int count = impl->sockets->ReceiveMessagesOnPollGroup( impl->pollGroup, messages, MaxMessagesPerPoll );
            if ( count <= 0 ) {
                break;
            }

            for ( int i = 0; i < count; i++ ) {
                SteamNetworkingMessage_t * msg = messages[i];

                u8 * copy = arena.PushArray<u8>( (usize)msg->m_cbSize );
                std::memcpy( copy, msg->m_pData, (usize)msg->m_cbSize );

                NetEvent event {};
                event.type = NetEventType::MessageReceived;
                event.connection = NetConnection { (u32)msg->m_conn };
                event.data = copy;
                event.dataSize = (usize)msg->m_cbSize;
                impl->pendingEvents.push_back( event );

                msg->Release();
            }

            if ( count < MaxMessagesPerPoll ) {
                break;
            }
        }

        NetEvent * events = arena.PushArray<NetEvent>( impl->pendingEvents.size() );
        std::memcpy( events, impl->pendingEvents.data(), impl->pendingEvents.size() * sizeof( NetEvent ) );

        impl->events = events;
        impl->eventCount = impl->pendingEvents.size();
    }

    const NetEvent * NetworkSystem::GetEvents() const { return impl->events; }
    usize             NetworkSystem::GetEventCount() const { return impl->eventCount; }

    NetConnectionState NetworkSystem::GetConnectionState( NetConnection connection ) const {
        SteamNetConnectionInfo_t info;
        if ( !impl->sockets->GetConnectionInfo( (HSteamNetConnection)connection.Id, &info ) ) {
            return NetConnectionState::ProblemDetectedLocally;
        }
        return ToNetConnectionState( info.m_eState );
    }

} // namespace sloth
