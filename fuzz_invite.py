import time

from fuzz_common import CRASH, EXCEPTION, FAIL, IRCFuzzHarness, PASS


def _nick(prefix="i"):
    return f"{prefix}{int(time.time() * 1000) % 100000}"


def test_invite_by_operator(h):
    h.start_server("INVITE by operator")
    op = target = None
    try:
        op = h.new_client()
        target = h.new_client()

        op_nick = _nick("op")
        target_nick = _nick("tg")
        h.auth(op, nick=op_nick, user="op")
        h.auth(target, nick=target_nick, user="t")
        _ = h.drain(op)
        _ = h.drain(target)

        h.send_line(op, "JOIN #invite")
        _ = h.drain(op)
        h.send_line(op, "MODE #invite +i")
        _ = h.drain(op)

        h.send_line(op, f"INVITE {target_nick} #invite")
        out_op = h.drain(op)
        out_target = h.drain(target)
        ok = h.has_numeric(out_op, 341) and "INVITE" in out_target
        return PASS if ok else FAIL
    except Exception:
        return EXCEPTION
    finally:
        if op:
            op.close()
        if target:
            target.close()
        if not h.is_server_alive():
            return CRASH
        h.stop_server()


def test_invite_non_operator_denied(h):
    h.start_server("INVITE non operator denied")
    op = user = target = None
    try:
        op = h.new_client()
        user = h.new_client()
        target = h.new_client()

        op_nick = _nick("op2")
        user_nick = _nick("u")
        target_nick = _nick("tg2")

        h.auth(op, nick=op_nick, user="op")
        h.auth(user, nick=user_nick, user="u")
        h.auth(target, nick=target_nick, user="t")
        _ = h.drain(op)
        _ = h.drain(user)
        _ = h.drain(target)

        h.send_line(op, "JOIN #invite2")
        h.send_line(user, "JOIN #invite2")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(user, f"INVITE {target_nick} #invite2")
        out_user = h.drain(user)
        return PASS if h.has_numeric(out_user, 482) else FAIL
    except Exception:
        return EXCEPTION
    finally:
        if op:
            op.close()
        if user:
            user.close()
        if target:
            target.close()
        if not h.is_server_alive():
            return CRASH
        h.stop_server()


def test_invite_missing_param(h):
    h.start_server("INVITE missing parameter")
    sock = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("im"), user="u")
        _ = h.drain(sock)
        h.send_line(sock, "INVITE")
        out = h.drain(sock)
        return PASS if h.has_numeric(out, 461) else FAIL
    except Exception:
        return EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            return CRASH
        h.stop_server()


def test_invite_no_such_nick(h):
    h.start_server("INVITE no such nick")
    op = None
    try:
        op = h.new_client()
        h.auth(op, nick=_nick("ins"), user="op")
        _ = h.drain(op)

        h.send_line(op, "JOIN #invns")
        _ = h.drain(op)

        h.send_line(op, "INVITE ghost_user_404 #invns")
        out = h.drain(op)
        return PASS if h.has_numeric(out, 401) else FAIL
    except Exception:
        return EXCEPTION
    finally:
        if op:
            op.close()
        if not h.is_server_alive():
            return CRASH
        h.stop_server()


def test_invite_not_on_channel(h):
    h.start_server("INVITE not on channel")
    owner = outsider = target = None
    try:
        owner = h.new_client()
        outsider = h.new_client()
        target = h.new_client()

        target_nick = _nick("it")
        h.auth(owner, nick=_nick("io"), user="o")
        h.auth(outsider, nick=_nick("iu"), user="u")
        h.auth(target, nick=target_nick, user="t")
        _ = h.drain(owner)
        _ = h.drain(outsider)
        _ = h.drain(target)

        h.send_line(owner, "JOIN #inv3")
        _ = h.drain(owner)

        h.send_line(outsider, f"INVITE {target_nick} #inv3")
        out = h.drain(outsider)
        return PASS if h.has_numeric(out, 442) else FAIL
    except Exception:
        return EXCEPTION
    finally:
        if owner:
            owner.close()
        if outsider:
            outsider.close()
        if target:
            target.close()
        if not h.is_server_alive():
            return CRASH
        h.stop_server()


def test_invite_user_on_channel(h):
    h.start_server("INVITE user already on channel")
    op = target = None
    try:
        op = h.new_client()
        target = h.new_client()
        target_nick = _nick("ion")
        h.auth(op, nick=_nick("iop"), user="op")
        h.auth(target, nick=target_nick, user="u")
        _ = h.drain(op)
        _ = h.drain(target)

        h.send_line(op, "JOIN #inv4")
        h.send_line(target, "JOIN #inv4")
        _ = h.drain(op)
        _ = h.drain(target)

        h.send_line(op, f"INVITE {target_nick} #inv4")
        out = h.drain(op)
        return PASS if h.has_numeric(out, 443) else FAIL
    except Exception:
        return EXCEPTION
    finally:
        if op:
            op.close()
        if target:
            target.close()
        if not h.is_server_alive():
            return CRASH
        h.stop_server()


def run(host="127.0.0.1", port=6667, password="password"):
    h = IRCFuzzHarness(host=host, port=port, password=password)
    return {
        "INVITE by operator": test_invite_by_operator(h),
        "INVITE non operator denied": test_invite_non_operator_denied(h),
        "INVITE missing parameter": test_invite_missing_param(h),
        "INVITE no such nick": test_invite_no_such_nick(h),
        "INVITE not on channel": test_invite_not_on_channel(h),
        "INVITE user already on channel": test_invite_user_on_channel(h),
    }


if __name__ == "__main__":
    results = run()
    for k, v in results.items():
        print(f"{k}: {v}")
