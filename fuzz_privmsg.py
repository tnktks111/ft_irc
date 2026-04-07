import time

from fuzz_common import CRASH, EXCEPTION, FAIL, IRCFuzzHarness, PASS


def _nick(prefix="p"):
    return f"{prefix[:4]}{int(time.time() * 1000) % 100000:05d}"


def test_privmsg_channel_forward(h):
    h.start_server("PRIVMSG channel forward")
    a = b = None
    result = None
    try:
        a = h.new_client()
        b = h.new_client()

        nick_a = _nick("a")
        nick_b = _nick("b")
        h.auth(a, nick=nick_a, user="a")
        h.auth(b, nick=nick_b, user="b")
        _ = h.drain(a)
        _ = h.drain(b)

        h.send_line(a, "JOIN #talk")
        h.send_line(b, "JOIN #talk")
        _ = h.drain(a)
        _ = h.drain(b)

        h.send_line(a, "PRIVMSG #talk :hello")
        out_b = h.drain(b)
        result = PASS if "PRIVMSG #talk :hello" in out_b else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if a:
            a.close()
        if b:
            b.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_privmsg_missing_text(h):
    h.start_server("PRIVMSG missing text")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("pm"), user="u")
        _ = h.drain(sock)
        h.send_line(sock, "PRIVMSG #talk")
        out = h.drain(sock)
        # 現実装はERR_NOTEXTTOSEND(412)を返す。
        result = PASS if h.has_numeric(out, 412) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_privmsg_no_such_nick(h):
    h.start_server("PRIVMSG no such nick")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("pm2"), user="u")
        _ = h.drain(sock)
        h.send_line(sock, "PRIVMSG ghost_user :hello")
        out = h.drain(sock)
        result = PASS if h.has_numeric(out, 401) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_privmsg_no_recipient(h):
    h.start_server("PRIVMSG no recipient")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("pm3"), user="u")
        _ = h.drain(sock)
        h.send_line(sock, "PRIVMSG :hello")
        out = h.drain(sock)
        result = PASS if h.has_numeric(out, 411) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_privmsg_cannot_send_to_chan(h):
    h.start_server("PRIVMSG cannot send to channel")
    member = outsider = None
    result = None
    try:
        member = h.new_client()
        outsider = h.new_client()
        h.auth(member, nick=_nick("pmm"), user="m")
        h.auth(outsider, nick=_nick("pmo"), user="o")
        _ = h.drain(member)
        _ = h.drain(outsider)

        h.send_line(member, "JOIN #talk2")
        _ = h.drain(member)

        h.send_line(outsider, "PRIVMSG #talk2 :hello")
        out = h.drain(outsider)
        result = PASS if h.has_numeric(out, 404) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if member:
            member.close()
        if outsider:
            outsider.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_privmsg_too_many_targets(h):
    h.start_server("PRIVMSG too many targets")
    sender = target = None
    result = None
    try:
        sender = h.new_client()
        target = h.new_client()

        target_nick = _nick("pt")
        h.auth(sender, nick=_nick("ps"), user="s")
        h.auth(target, nick=target_nick, user="t")
        _ = h.drain(sender)
        _ = h.drain(target)

        h.send_line(sender, f"PRIVMSG {target_nick},{target_nick} :dup")
        out = h.drain(sender)
        result = PASS if h.has_numeric(out, 407) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sender:
            sender.close()
        if target:
            target.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def run(host="127.0.0.1", port=6667, password="password"):
    h = IRCFuzzHarness(host=host, port=port, password=password)
    return h.apply_leak_overrides(
        {
            "PRIVMSG channel forward": test_privmsg_channel_forward(h),
            "PRIVMSG missing text": test_privmsg_missing_text(h),
            "PRIVMSG no such nick": test_privmsg_no_such_nick(h),
            "PRIVMSG no recipient": test_privmsg_no_recipient(h),
            "PRIVMSG cannot send to channel": test_privmsg_cannot_send_to_chan(h),
            "PRIVMSG too many targets": test_privmsg_too_many_targets(h),
        }
    )


if __name__ == "__main__":
    results = run()
    for k, v in results.items():
        print(f"{k}: {v}")
