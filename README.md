*This project has been created as part of the 42 curriculum by ttanaka, guruguruge.*

# ft_irc — Internet Relay Chat Server

## Description

`ircserv` is an IRC server written in C++98. It accepts connections from real
IRC clients over TCP/IP, authenticates them with a connection password, and
lets them chat in channels or by private message, following the core of
RFC 1459 / RFC 2812.

Key characteristics:

- Single-threaded, single `poll()` event loop — every accept, receive, and
  send is driven by one `poll()` call. No forking, no blocking I/O.
- All client sockets are set to non-blocking mode with
  `fcntl(fd, F_SETFL, O_NONBLOCK)`.
- Partial and pipelined input is aggregated per client until a complete
  CRLF-terminated command is available (the `nc` Ctrl+D fragment test works).
- Messages are capped at 512 bytes (RFC 2812 §2.3) in both directions.

Implemented commands:

| Category | Commands |
| --- | --- |
| Registration | `CAP`, `PASS`, `NICK`, `USER`, `PING`, `QUIT` |
| Channels | `JOIN`, `PART`, `PRIVMSG`, `TOPIC` |
| Operator | `KICK`, `INVITE`, `TOPIC`, `MODE` (`i`, `t`, `k`, `o`, `l`) |
| User | `MODE <nick>` (minimal user modes) |

Channel messages are forwarded to every other member of the channel.
Channels distinguish operators from regular users; operator-only actions are
rejected for regular users with the proper numeric replies.

## Instructions

Build (requires `c++` with C++98 support):

```sh
make          # builds ./ircserv
make clean    # remove objects
make fclean   # remove objects and binary
make re       # rebuild from scratch
```

Run:

```sh
./ircserv <port> <password>
# example
./ircserv 6667 secret
```

- `port`: listening port (1024–65535)
- `password`: connection password each client must send with `PASS`

Connect with an IRC client (irssi is the reference client):

```sh
irssi -c 127.0.0.1 -p 6667 -w secret
```

Or test by hand with netcat:

```sh
nc -C 127.0.0.1 6667
PASS secret
NICK alice
USER alice 0 * :Alice
JOIN #test
PRIVMSG #test :hello
```

## Resources

- [RFC 1459 — Internet Relay Chat Protocol](https://datatracker.ietf.org/doc/html/rfc1459)
- [RFC 2812 — Internet Relay Chat: Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812)
- [Modern IRC Client Protocol documentation](https://modern.ircdocs.horse/)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- `man 2 poll`, `man 2 socket`, `man 2 fcntl`

### How AI was used

AI assistants (Claude) were used as a support tool, always with human review:

- Learning how to write the code: idiomatic C++98 patterns, non-blocking
  socket handling, and how to structure a poll()-driven event loop.
- Understanding the RFC specifications: reading RFC 1459/2812 together and
  clarifying message formats, numeric replies, and channel/mode semantics.

All merged code was built and tested locally by the team; the architecture
and command implementations were designed and written by the members listed
above.
