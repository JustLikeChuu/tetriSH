# tetriSH

Server-authoritative networked multiplayer Tetris Battle Royale, written in C.

Originally proposed and built as the 50.005 half of a cross-course university project
(CoreStack Challenge, 50.003 × 50.005) with teammates Li Lian and Ryan Ngo. tetriSH
replaces the course's standard PA1 (shell/daemon/IPC) and PA2 (authenticated, encrypted
client-server protocol) with one integrated system. This is my solo continuation —
commit history is squashed on import, but the design below is what shipped, largely my
own work within it. Some of it diverged from our original proposal during the build;
noted below where that matters.

Course spec: https://natalieagus.github.io/50005/pa/tetrish

## Layering

+---------------------------------------------+
| Application: HTTTP messages |
| (HyperText Tetris Transfer Protocol) |
+---------------------------------------------+
| Secure session (cert auth, RSA-wrapped AES) |
+---------------------------------------------+
| Transport: TCP via POSIX sockets |
+---------------------------------------------+

## Binaries

| Binary       | Role                                                                                                                                                                                                                                                 |
| ------------ | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `tetrisd`    | Game server. Single-threaded `poll()`-based event loop — not epoll, not thread-per-client. Server-authoritative, owns per-player `GameState`, runs the dynamic lobby (`hub.c`/`room.c`). Handles `SIGINT` for shutdown; ignores `SIGCHLD`/`SIGPIPE`. |
| `tetrisu`    | Terminal client, TCP only (proposal originally called for RayLib + UDP for position smoothing — not what shipped). Input-forwarding / state-rendering only, never computes its own game state.                                                       |
| `tetrislogd` | Standalone logging daemon, talks to `tetrisd` over a Unix domain socket so a logging failure can't take down the game. Handles `SIGTERM` (flush + exit) and `SIGHUP` (reopen the log file for rotation).                                             |

## Libraries

| Library             | Role                                                                                                                                                                    |
| ------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `libtetrisbrain`    | Core Tetris logic — pure, no I/O. Board control, movement, hold, 7-bag randomization. Battle Royale garbage math (`calculate_garbage`, `queue_garbage`) lives here too. |
| `libtetrisprotocol` | tetriSH-specific message layer built on top of corestack's `libhtttp`, linked into both `tetrisd` and `tetrisu` so client and server can't drift on message format.     |
| `libtetrislog`      | Client-side logging library `tetrisd` uses to ship records to `tetrislogd`.                                                                                             |

## HTTTP — the wire protocol

Application-layer protocol modeled on HTTP, fixed by the course spec for every group:

| Method   | Path                      | Purpose                                             |
| -------- | ------------------------- | --------------------------------------------------- |
| `JOIN`   | `/room/<id>`              | Join or create a room                               |
| `LEAVE`  | `/room/<id>`              | Leave a room                                        |
| `START`  | `/room/<id>`              | Begin the game (room owner only)                    |
| `MOVE`   | `/room/<id>/player/<pid>` | Body: `LEFT` or `RIGHT`                             |
| `ROTATE` | `/room/<id>/player/<pid>` | Body: `CW` or `CCW`                                 |
| `DROP`   | `/room/<id>/player/<pid>` | Body: `SOFT` or `HARD`                              |
| `STATE`  | `/room/<id>`              | Server-originated. Pushed broadcast of board state. |

## Battle Royale garbage routing

When a room clears 2+ lines, the outgoing garbage is queued as a `HubAttack` and drained
into the target room on its next tick (`apply_remote_garbage` in `room.c`). Because
`tetrisd` is single-threaded, this is an in-process mailbox, not a synchronization
problem — no shared memory or semaphores involved, despite that being the original plan.

## Reliability

- Sequence-number gap detection between `tetrisd` and `tetrislogd` — dropped log
  packets are visible, not silent.

## `corestack/`

Vendored shared library (secure session handshake, HTTP-alike parsing/serialisation,
transport) — built by Ryan Ngo and Li Lian during the original CoreStack Challenge,
not by me. Taken forward as-is with their permission; I'll diverge from it over time,
but the current state in this repo is entirely their work.

Original joint repo: [[CoreStack Project for Team HTTTPBets (C1C7)](https://github.com/50005-computer-system-engineering/2026-corestack-50005-htttpbets)]