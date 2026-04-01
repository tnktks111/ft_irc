import time

from fuzz_common import CRASH, EXCEPTION, FAIL, IRCFuzzHarness, PASS


def _nick(prefix="j"):
    return f"{prefix[:4]}{int(time.time() * 1000) % 100000:05d}"


def test_join_success(h):
    h.start_server("JOIN success")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("join"), user="u")
        _ = h.drain(sock)
        h.send_line(sock, "JOIN #chan")
        out = h.drain(sock)
        if h.has_numeric(out, 353) and h.has_numeric(out, 366):
            result = PASS
        else:
            result = FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_join_missing_param(h):
    h.start_server("JOIN missing parameter")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("joinm"), user="u")
        _ = h.drain(sock)
        h.send_line(sock, "JOIN")
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


def test_join_no_such_channel(h):
    h.start_server("JOIN no such channel")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("jns"), user="u")
        _ = h.drain(sock)
        h.send_line(sock, "JOIN #definitely_missing_channel")
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


def test_join_invite_only_reject(h):
    h.start_server("JOIN +i reject without invite")
    op = user = None
    result = None
    try:
        op = h.new_client()
        h.auth(op, nick=_nick("op"), user="op")
        _ = h.drain(op)
        h.send_line(op, "JOIN #invite")
        _ = h.drain(op)
        h.send_line(op, "MODE #invite +i")
        _ = h.drain(op)

        user = h.new_client()
        h.auth(user, nick=_nick("u"), user="u")
        _ = h.drain(user)
        h.send_line(user, "JOIN #invite")
        out = h.drain(user)
        result = PASS if h.has_numeric(out, 473) else FAIL
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


def test_join_bad_channel_mask(h):
    h.start_server("JOIN bad channel mask")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("jbm"), user="u")
        _ = h.drain(sock)
        h.send_line(sock, "JOIN not_a_channel")
        out = h.drain(sock)
        result = PASS if h.has_numeric(out, 476) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_join_bad_key(h):
    h.start_server("JOIN bad key")
    op = user = None
    result = None
    try:
        op = h.new_client()
        user = h.new_client()
        h.auth(op, nick=_nick("opk"), user="op")
        h.auth(user, nick=_nick("uk"), user="u")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, "JOIN #keychan")
        _ = h.drain(op)
        h.send_line(op, "MODE #keychan +k secret")
        _ = h.drain(op)

        h.send_line(user, "JOIN #keychan wrong")
        out = h.drain(user)
        result = PASS if h.has_numeric(out, 475) else FAIL
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


def test_join_channel_full(h):
    h.start_server("JOIN channel full")
    op = user = None
    result = None
    try:
        op = h.new_client()
        user = h.new_client()
        h.auth(op, nick=_nick("opl"), user="op")
        h.auth(user, nick=_nick("ul"), user="u")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, "JOIN #limit")
        _ = h.drain(op)
        h.send_line(op, "MODE #limit +l 1")
        _ = h.drain(op)

        h.send_line(user, "JOIN #limit")
        out = h.drain(user)
        result = PASS if h.has_numeric(out, 471) else FAIL
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


def test_join_banned_from_channel(h):
    h.start_server("JOIN banned from channel")
    op = user = None
    result = None
    try:
        op = h.new_client()
        user = h.new_client()
        h.auth(op, nick=_nick("opb"), user="op")
        h.auth(user, nick=_nick("ub"), user="u")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, "JOIN #banme")
        _ = h.drain(op)
        h.send_line(op, "MODE #banme +b *!*@*")
        _ = h.drain(op)

        h.send_line(user, "JOIN #banme")
        out = h.drain(user)
        result = PASS if h.has_numeric(out, 474) else FAIL
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


def test_join_too_many_channels(h):
    h.start_server("JOIN too many channels")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("jmax"), user="u")
        _ = h.drain(sock)
        hit = False
        for i in range(1, 31):
            h.send_line(sock, f"JOIN #c{i}")
            out = h.drain(sock, duration=0.2)
            if h.has_numeric(out, 405):
                hit = True
                break
        result = PASS if hit else FAIL
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
    return {
        "JOIN success": test_join_success(h),
        "JOIN missing parameter": test_join_missing_param(h),
        "JOIN no such channel": test_join_no_such_channel(h),
        "JOIN +i reject without invite": test_join_invite_only_reject(h),
        "JOIN bad channel mask": test_join_bad_channel_mask(h),
        "JOIN bad key": test_join_bad_key(h),
        "JOIN channel full": test_join_channel_full(h),
        "JOIN banned from channel": test_join_banned_from_channel(h),
        "JOIN too many channels": test_join_too_many_channels(h),
    }


if __name__ == "__main__":
    results = run()
    for k, v in results.items():
        print(f"{k}: {v}")
