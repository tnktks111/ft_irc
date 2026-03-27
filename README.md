*This project has been created as part of the 42 curriculum by ttanaka, sguruge, sabe.*

## Description
The main goal ft_irc project the implementation of an irc server including an non-blocking interaction via TCP/IP (v4 or v6). The mandatory features are channel systems, authentication systems and private conversations for clients. In addition, channel operators must be implemented with the commands below.
∗ KICK - Eject a client from the channel
∗ INVITE - Invite a client to a channel
∗ TOPIC - Change or view the channel topic
∗ MODE - Change the channel’s mode:
    · i: Set/remove Invite-only channel
    · t: Set/remove the restrictions of the TOPIC command to channel operators
    · k: Set/remove the channel key (password)
    · o: Give/take channel operator privilege
    · l: Set/remove the user limit to channel

## Instuctions
The copmilation is conducted via Makefile and will run as follows:
```
make
```

The executable will be run as follows:
```
./ircserv <port> <password>
```

## Resources

