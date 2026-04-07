import time

from fuzz_common import CRASH, EXCEPTION, FAIL, IRCFuzzHarness, PASS


def _nick(prefix="m"):
    return f"{prefix[:4]}{int(time.time() * 1000) % 100000:05d}"


def test_mode_itkol_by_operator(h):
    h.start_server("MODE i/t/k/o/l by operator")
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

        h.send_line(op, "JOIN #mode")
        h.send_line(user, "JOIN #mode")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, "MODE #mode +i")
        h.send_line(op, "MODE #mode +t")
        h.send_line(op, "MODE #mode +k secret")
        h.send_line(op, "MODE #mode +l 5")
        h.send_line(op, f"MODE #mode +o {user_nick}")
        h.send_line(op, "MODE #mode")
        out = h.drain(op, duration=0.8)

        ok = h.has_numeric(out, 324) and "+" in out
        result = PASS if ok else FAIL
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


def test_mode_non_operator_denied(h):
    h.start_server("MODE non-operator denied")
    op = user = None
    result = None
    try:
        op = h.new_client()
        user = h.new_client()
        h.auth(op, nick=_nick("op2"), user="op")
        h.auth(user, nick=_nick("u2"), user="u")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, "JOIN #mode2")
        h.send_line(user, "JOIN #mode2")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(user, "MODE #mode2 +i")
        out = h.drain(user)
        result = PASS if h.has_numeric(out, 482) else FAIL
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


def test_mode_no_such_channel(h):
    h.start_server("MODE no such channel")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("mns"), user="u")
        _ = h.drain(sock)

        h.send_line(sock, "MODE #definitely_missing +i")
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


def test_mode_not_on_channel(h):
    h.start_server("MODE not on channel")
    owner = outsider = None
    result = None
    try:
        owner = h.new_client()
        outsider = h.new_client()

        h.auth(owner, nick=_nick("mo"), user="o")
        h.auth(outsider, nick=_nick("mu"), user="u")
        _ = h.drain(owner)
        _ = h.drain(outsider)

        h.send_line(owner, "JOIN #mode_noton")
        _ = h.drain(owner)

        h.send_line(outsider, "MODE #mode_noton +i")
        out = h.drain(outsider)
        result = PASS if h.has_numeric(out, 442) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if owner:
            owner.close()
        if outsider:
            outsider.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_mode_o_no_such_nick(h):
    h.start_server("MODE +o no such nick")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("mon"), user="op")
        _ = h.drain(sock)

        h.send_line(sock, "JOIN #mode_nonick")
        _ = h.drain(sock)

        h.send_line(sock, "MODE #mode_nonick +o ghost_user_404")
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


def test_mode_o_user_not_in_channel(h):
    h.start_server("MODE +o user not in channel")
    op = outsider = None
    result = None
    try:
        op = h.new_client()
        outsider = h.new_client()

        outsider_nick = _nick("out")
        h.auth(op, nick=_nick("mon2"), user="op")
        h.auth(outsider, nick=outsider_nick, user="u")
        _ = h.drain(op)
        _ = h.drain(outsider)

        h.send_line(op, "JOIN #mode_not_member")
        _ = h.drain(op)

        h.send_line(op, f"MODE #mode_not_member +o {outsider_nick}")
        out = h.drain(op)
        result = PASS if h.has_numeric(out, 441) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if op:
            op.close()
        if outsider:
            outsider.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_mode_k_missing_param(h):
    h.start_server("MODE +k missing parameter")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("mk"), user="u")
        _ = h.drain(sock)
        h.send_line(sock, "JOIN #mode3")
        _ = h.drain(sock)
        h.send_line(sock, "MODE #mode3 +k")
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


def test_mode_k_already_set(h):
    h.start_server("MODE +k already set")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("mka"), user="u")
        _ = h.drain(sock)
        h.send_line(sock, "JOIN #mode4")
        _ = h.drain(sock)
        h.send_line(sock, "MODE #mode4 +k secret")
        _ = h.drain(sock)
        h.send_line(sock, "MODE #mode4 +k other")
        out = h.drain(sock)
        result = PASS if h.has_numeric(out, 467) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_mode_unknown_flag(h):
    h.start_server("MODE unknown flag")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("mu"), user="u")
        _ = h.drain(sock)
        h.send_line(sock, "JOIN #mode5")
        _ = h.drain(sock)
        h.send_line(sock, "MODE #mode5 +z")
        out = h.drain(sock)
        result = PASS if h.has_numeric(out, 472) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_mode_l_missing_param(h):
    h.start_server("MODE +l missing parameter")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("ml"), user="u")
        _ = h.drain(sock)
        h.send_line(sock, "JOIN #mode6")
        _ = h.drain(sock)
        h.send_line(sock, "MODE #mode6 +l")
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


def run(host="127.0.0.1", port=6667, password="password"):
    h = IRCFuzzHarness(host=host, port=port, password=password)
    return h.apply_leak_overrides(
        {
            "MODE i/t/k/o/l by operator": test_mode_itkol_by_operator(h),
            "MODE non-operator denied": test_mode_non_operator_denied(h),
            "MODE no such channel": test_mode_no_such_channel(h),
            "MODE not on channel": test_mode_not_on_channel(h),
            "MODE +k missing parameter": test_mode_k_missing_param(h),
            "MODE +k already set": test_mode_k_already_set(h),
            "MODE +o no such nick": test_mode_o_no_such_nick(h),
            "MODE +o user not in channel": test_mode_o_user_not_in_channel(h),
            "MODE unknown flag": test_mode_unknown_flag(h),
            "MODE +l missing parameter": test_mode_l_missing_param(h),
        }
    )


if __name__ == "__main__":
    results = run()
    for k, v in results.items():
        print(f"{k}: {v}")
