#pragma once

#include "core/sloth_defines.h"
#include "core/sloth_arena.h"

#include <memory>

namespace sloth {

    // Opaque handle to a network connection - either a client's connection
    // to a server, or (server-side) one of the connections accepted through
    // a listen socket. Not valid across different NetworkSystem instances.
    struct NetConnection {
        static constexpr u32 InvalidId = 0; // matches k_HSteamNetConnection_Invalid

        u32 Id = InvalidId;

        bool IsValid() const { return Id != InvalidId; }
    };

    enum class NetConnectionState : u8 {
        Connecting,
        Connected,
        ClosedByPeer,
        ProblemDetectedLocally, // also covers internal/transient states we don't otherwise expose
    };

    enum class NetSendType : u8 {
        Unreliable,
        Reliable,
    };

    enum class NetEventType : u8 {
        IncomingConnection, // server role only: a peer wants in, call AcceptConnection() or CloseConnection()
        Connected,          // handshake finished, both sides may now send/receive
        Disconnected,       // see NetEvent::closeState for why
        MessageReceived,
    };

    struct NetEvent {
        NetEventType        type;
        NetConnection       connection;

        // Valid only when type == Disconnected.
        NetConnectionState  closeState = NetConnectionState::ClosedByPeer;

        // Valid only when type == MessageReceived. Points into the arena
        // passed to NetworkSystem::Update() for this frame - do not store
        // across frames.
        const u8 *          data = nullptr;
        usize                dataSize = 0;
    };

    // Thin wrapper around GameNetworkingSockets, hiding its headers/types
    // behind Impl (same PIMPL pattern PhysicsWorld uses to hide Jolt). Only
    // one NetworkSystem may exist per process - the underlying library is
    // itself a process-wide singleton.
    //
    // Roles: the same instance exposes both the client half of the API
    // (Connect) and the server half (Listen/AcceptConnection); callers just
    // use whichever half matches their role and ignore the other.
    //
    // Call Update() once per frame, passing an arena to allocate this
    // frame's events (and any received message payloads) from - typically
    // the engine's frame arena. Events are only valid until the next
    // Update() call, same lifetime rule as the arena they came from.
    class NetworkSystem {
      public:
        NetworkSystem();
        ~NetworkSystem();

        SL_NON_COPYABLE( NetworkSystem );
        SL_NON_MOVABLE( NetworkSystem );

        // --- Client role ---
        NetConnection Connect( const char * address ); // "ip:port", e.g. "127.0.0.1:27020"

        // --- Server role ---
        bool          Listen( u16 port );
        void          AcceptConnection( NetConnection connection );

        // --- Both roles ---
        void          CloseConnection( NetConnection connection );
        void          SendMessage( NetConnection connection, const void * data, usize size, NetSendType sendType );

        void          Update( Arena & arena );

        const NetEvent * GetEvents() const;
        usize             GetEventCount() const;

        NetConnectionState GetConnectionState( NetConnection connection ) const;

        // Public only so sloth_network.cpp's file-scope GameNetworkingSockets
        // callback (a plain C function pointer, so it can't be a member) can
        // name this type - not part of the public API, don't touch from
        // outside this class or sloth_network.cpp.
        struct Impl;

      private:
        std::unique_ptr<Impl> impl;
    };

} // namespace sloth
