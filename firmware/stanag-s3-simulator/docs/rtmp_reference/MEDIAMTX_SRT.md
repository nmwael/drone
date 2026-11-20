# MediaMTX SRT Source Reference

## Repository
**URL:** https://github.com/bluenviron/mediamtx (formerly github.com/aler9/mediamtx)

## Relevant Files

### SRT Protocol Implementation
| File | Path in Repository | Purpose |
|------|-------------------|---------|
| srt/conn.go | srt/conn.go | SRT connection handling |
| srt/server.go | srt/server.go | SRT server listener |
| srt/session.go | srt/session.go | SRT session management |
| srt/conn.go (DATA packets) | srt/conn.go | SRT DATA packet handling |

### Key SRT Functions in MediaMTX
- srtConn.readPacket() - Read SRT packets
- srtConn.writePacket() - Write SRT packets
- srtConn.readMessage() - Read SRT messages
- srtConn.writeMessage() - Write SRT messages
- srtConn.onData() - Handle SRT DATA packets

### Key SRT Implementation Details

#### SRT Handshake Sequence
1. Client sends UDT4 induction handshake
2. Server responds with SRT1 conclusion handshake
3. Client sends final handshake confirmation

### SRT DATA Packet Format (libsrt wire format)
SRT DATA packet header (16 bytes):
- Word 0: SEQNO (bit31=0=data, bits30-16=0(DATA msg type), bits15-0=seqno)
- Word 1: MSGNO (bits31-30=boundary, bit29=inorder, bits25-0=msg seq)
- Word 2: TIMESTAMP
- Word 3: DESTINATION SOCKET ID (peer's socket ID)

### SRT Socket ID Convention
- **sockid_local**: Our own socket ID (randomly generated at connection)
- **sockid_peer**: Peer's socket ID (received during handshake response)

### Critical Bug Fix (SRT DATA Packets)
**CRITICAL:** SRT DATA packets must contain the PEER's socket ID (sockid_peer) in word 3, NOT our own socket ID (sockid_local). MediaMTX validates DATA packets by checking word 3 against the peer's socket ID assigned during handshake.

Incorrect:  put32(p + 12, c->sockid_local)  // Sends OUR socket ID
Correct:   put32(p + 12, c->sockid_peer)  // Sends PEER's socket ID

### Key SRT Constants
```
SRT_CMD_HSREQ = 0x07 (Handshake Request)
SRT_CMD_HSRES = 0x08 (Handshake Response)  
SRT_CMD_HSACK = 0x09 (Handshake Acknowledgement)
SRT_CMD_KEEPALIVE = 0x0A
SRT_CMD_ACK = 0x0C
SRT_CMD_ACKACK = 0x0D
SRT_CMD_DATA = 0x01
SRT_CMD_SHUTDOWN = 0x0E
```

## Key Implementation Notes
- SRT uses UDT4 induction + SRT1 conclusion handshake
- Socket IDs are exchanged during handshake
- MediaMTX validates DATA packets by checking word 3 against peer's sockid
- Using sockid_local instead of sockid_peer in DATA word 3 causes ECONNRESET
- SRT socket IDs are randomly generated at connection time