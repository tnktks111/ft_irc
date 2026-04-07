Using your reference client with your server must be similar to using it with any
official IRC server. However, you only have to implement the following features:
◦ You must be able to authenticate, set a nickname, a username, join a channel,
send and receive private messages using your reference client.
◦ All the messages sent from one client to a channel have to be forwarded to
every other client that joined the channel.
◦ You must have operators and regular users.
◦ Then, you have to implement the commands that are specific to channel
operators:
∗ KICK - Eject a client from the channel
∗ INVITE - Invite a client to a channel
∗ TOPIC - Change or view the channel topic
∗ MODE - Change the channel’s mode:
· i: Set/remove Invite-only channel
· t: Set/remove the restrictions of the TOPIC command to channel
operators
· k: Set/remove the channel key (password)
8
ft_irc Internet Relay Chat
· o: Give/take channel operator privilege
· l: Set/remove the user limit to channel