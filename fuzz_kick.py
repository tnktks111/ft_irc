import time

from fuzz_common import CRASH, EXCEPTION, FAIL, IRCFuzzHarness, PASS


def _nick(prefix="k"):
    return f"{prefix[:4]}{int(time.time() * 1000) % 100000:05d}"


def test_kick_by_operator(h):
    h.start_server("KICK by operator")
    op = user = None
    result = None
    try:
        op = h.new_client()
        user = h.new_client()

        op_nick = _nick("op")
        user_nick = _nick("u")
        h.auth(op, nick=op_nick, user="op")
        h.auth(user, nick=user_nick, user="u")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, "JOIN #kick")
        h.send_line(user, "JOIN #kick")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, f"KICK #kick {user_nick} :bye")
        out_user = h.drain(user)
        result = PASS if "KICK #kick" in out_user else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if op:
            op.close()
        if user:
            user.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_kick_non_operator_denied(h):
    h.start_server("KICK non operator denied")
    op = user = None
    result = None
    try:
        op = h.new_client()
        user = h.new_client()

        op_nick = _nick("op2")
        user_nick = _nick("u2")
        h.auth(op, nick=op_nick, user="op")
        h.auth(user, nick=user_nick, user="u")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, "JOIN #kick2")
        h.send_line(user, "JOIN #kick2")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(user, f"KICK #kick2 {op_nick} :nope")
        out_user = h.drain(user)
        result = PASS if h.has_numeric(out_user, 482) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if op:
            op.close()
        if user:
            user.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_kick_missing_param(h):
    h.start_server("KICK missing parameter")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("km"), user="u")
        _ = h.drain(sock)
        h.send_line(sock, "KICK #kick")
        out = h.drain(sock)
        result = PASS if h.has_numeric(out, 461) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_kick_no_such_channel(h):
    h.start_server("KICK no such channel")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("kns"), user="u")
        _ = h.drain(sock)
        h.send_line(sock, "KICK #no_such someone")
        out = h.drain(sock)
        result = PASS if h.has_numeric(out, 403) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_kick_not_on_channel(h):
    h.start_server("KICK not on channel")
    owner = outsider = target = None
    result = None
    try:
        owner = h.new_client()
        outsider = h.new_client()
        target = h.new_client()
        target_nick = _nick("kt")

        h.auth(owner, nick=_nick("ko"), user="o")
        h.auth(outsider, nick=_nick("kout"), user="u")
        h.auth(target, nick=target_nick, user="t")
        _ = h.drain(owner)
        _ = h.drain(outsider)
        _ = h.drain(target)

        h.send_line(owner, "JOIN #kick3")
        _ = h.drain(owner)

        h.send_line(outsider, f"KICK #kick3 {target_nick}")
        out = h.drain(outsider)
        result = PASS if h.has_numeric(out, 442) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if owner:
            owner.close()
        if outsider:
            outsider.close()
        if target:
            target.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_kick_user_not_in_channel(h):
    h.start_server("KICK user not in channel")
    op = target = None
    result = None
    try:
        op = h.new_client()
        target = h.new_client()
        target_nick = _nick("knin")
        h.auth(op, nick=_nick("kop"), user="op")
        h.auth(target, nick=target_nick, user="u")
        _ = h.drain(op)
        _ = h.drain(target)

        h.send_line(op, "JOIN #kick4")
        _ = h.drain(op)

        h.send_line(op, f"KICK #kick4 {target_nick}")
        out = h.drain(op)
        result = PASS if h.has_numeric(out, 441) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if op:
            op.close()
        if target:
            target.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def run(host="127.0.0.1", port=6667, password="password"):
    h = IRCFuzzHarness(host=host, port=port, password=password)
    return {
        "KICK by operator": test_kick_by_operator(h),
        "KICK non operator denied": test_kick_non_operator_denied(h),
        "KICK missing parameter": test_kick_missing_param(h),
        "KICK no such channel": test_kick_no_such_channel(h),
        "KICK not on channel": test_kick_not_on_channel(h),
        "KICK user not in channel": test_kick_user_not_in_channel(h),
    }


if __name__ == "__main__":
    results = run()
    for k, v in results.items():
        print(f"{k}: {v}")
