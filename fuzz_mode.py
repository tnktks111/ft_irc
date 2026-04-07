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


def test_mode_missing_param(h):
    h.start_server("MODE missing parameter")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("mmp"), user="u")
        _ = h.drain(sock)
        h.send_line(sock, "MODE")
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


def test_mode_o_missing_param(h):
    h.start_server("MODE +o missing parameter")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("mop"), user="op")
        _ = h.drain(sock)
        h.send_line(sock, "JOIN #mode_o_missing")
        _ = h.drain(sock)
        h.send_line(sock, "MODE #mode_o_missing +o")
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


def test_mode_minus_o_missing_param(h):
    h.start_server("MODE -o missing parameter")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("mmo"), user="op")
        _ = h.drain(sock)
        h.send_line(sock, "JOIN #mode_minus_o")
        _ = h.drain(sock)
        h.send_line(sock, "MODE #mode_minus_o -o")
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


def test_mode_mixed_plus_minus_parser(h):
    h.start_server("MODE mixed + and - parser")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("mmx"), user="op")
        _ = h.drain(sock)

        h.send_line(sock, "JOIN #mode_mix")
        _ = h.drain(sock)
        h.send_line(sock, "MODE #mode_mix +k secret")
        _ = h.drain(sock)
        h.send_line(sock, "MODE #mode_mix +il-k 2")
        _ = h.drain(sock)

        h.send_line(sock, "MODE #mode_mix")
        out = h.drain(sock)
        ok = (
            h.has_numeric(out, 324)
            and "+il" in out
            and " k" not in out
            and "secret" not in out
        )
        result = PASS if ok else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_mode_remove_key_allows_join_without_key(h):
    h.start_server("MODE -k allows JOIN without key")
    op = user = None
    result = None
    try:
        op = h.new_client()
        user = h.new_client()
        h.auth(op, nick=_nick("mrko"), user="op")
        h.auth(user, nick=_nick("mrku"), user="u")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, "JOIN #mode_rm_key")
        _ = h.drain(op)
        h.send_line(op, "MODE #mode_rm_key +k secret")
        _ = h.drain(op)
        h.send_line(op, "MODE #mode_rm_key -k")
        _ = h.drain(op)

        h.send_line(user, "JOIN #mode_rm_key")
        out = h.drain(user)
        ok = (
            h.has_numeric(out, 353)
            and h.has_numeric(out, 366)
            and not h.has_numeric(out, 475)
        )
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


def test_mode_remove_limit_allows_join(h):
    h.start_server("MODE -l allows extra JOIN")
    op = user = None
    result = None
    try:
        op = h.new_client()
        user = h.new_client()
        h.auth(op, nick=_nick("mrlo"), user="op")
        h.auth(user, nick=_nick("mrlu"), user="u")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, "JOIN #mode_rm_limit")
        _ = h.drain(op)
        h.send_line(op, "MODE #mode_rm_limit +l 1")
        _ = h.drain(op)

        h.send_line(user, "JOIN #mode_rm_limit")
        out_blocked = h.drain(user)

        h.send_line(op, "MODE #mode_rm_limit -l")
        _ = h.drain(op)

        h.send_line(user, "JOIN #mode_rm_limit")
        out_ok = h.drain(user)

        ok = (
            h.has_numeric(out_blocked, 471)
            and h.has_numeric(out_ok, 353)
            and h.has_numeric(out_ok, 366)
        )
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


def test_mode_operator_grant_allows_mode_change(h):
    h.start_server("MODE +o grant allows mode change")
    op = user = None
    result = None
    try:
        op = h.new_client()
        user = h.new_client()

        user_nick = _nick("mgru")
        h.auth(op, nick=_nick("mgro"), user="op")
        h.auth(user, nick=user_nick, user="u")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, "JOIN #mode_grant")
        h.send_line(user, "JOIN #mode_grant")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, f"MODE #mode_grant +o {user_nick}")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(user, "MODE #mode_grant +i")
        out_user = h.drain(user)
        out_op = h.drain(op)

        ok = (not h.has_numeric(out_user, 482)) and (
            " MODE #mode_grant +i" in out_user or " MODE #mode_grant +i" in out_op
        )
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
            "MODE missing parameter": test_mode_missing_param(h),
            "MODE +o missing parameter": test_mode_o_missing_param(h),
            "MODE -o missing parameter": test_mode_minus_o_missing_param(h),
            "MODE mixed + and - parser": test_mode_mixed_plus_minus_parser(h),
            "MODE -k allows JOIN without key": test_mode_remove_key_allows_join_without_key(
                h
            ),
            "MODE -l allows extra JOIN": test_mode_remove_limit_allows_join(h),
            "MODE +o grant allows mode change": test_mode_operator_grant_allows_mode_change(
                h
            ),
        }
    )


if __name__ == "__main__":
    results = run()
    for k, v in results.items():
        print(f"{k}: {v}")
