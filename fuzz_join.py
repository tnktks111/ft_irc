import time
import re

from fuzz_common import CRASH, EXCEPTION, FAIL, IRCFuzzHarness, PASS


def _nick(prefix="j"):
    return f"{prefix[:4]}{int(time.time() * 1000) % 100000:05d}"


def _split_lines(payload):
    return [ln for ln in payload.replace("\r", "").split("\n") if ln]


def _count_numeric(payload, code):
    pat = re.compile(rf"(^|\s){int(code):03d}(\s|$)")
    return sum(1 for ln in _split_lines(payload) if pat.search(ln))


def _find_index(lines, pred):
    for i, ln in enumerate(lines):
        if pred(ln):
            return i
    return -1


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


def test_join_multi_channel_success(h):
    h.start_server("JOIN multi channel success")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("jmcs"), user="u")
        _ = h.drain(sock)

        h.send_line(sock, "JOIN #mca,#mcb")
        out = h.drain(sock, duration=0.8)
        ok = (
            _count_numeric(out, 353) >= 2
            and _count_numeric(out, 366) >= 2
            and "#mca" in out
            and "#mcb" in out
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


def test_join_multi_channel_key_mapping(h):
    h.start_server("JOIN multi channel key mapping")
    op = user = None
    result = None
    try:
        op = h.new_client()
        user = h.new_client()
        h.auth(op, nick=_nick("opmk"), user="op")
        h.auth(user, nick=_nick("umk"), user="u")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, "JOIN #mkeya,#mkeyb")
        _ = h.drain(op)
        h.send_line(op, "MODE #mkeya +k ka")
        _ = h.drain(op)
        h.send_line(op, "MODE #mkeyb +k kb")
        _ = h.drain(op)

        h.send_line(user, "JOIN #mkeya,#mkeyb ka,kb")
        out = h.drain(user, duration=0.8)
        ok = (
            not h.has_numeric(out, 475)
            and _count_numeric(out, 366) >= 2
            and "#mkeya" in out
            and "#mkeyb" in out
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


def test_join_partial_failure_continue(h):
    h.start_server("JOIN partial failure continue")
    op = user = None
    result = None
    try:
        op = h.new_client()
        user = h.new_client()
        h.auth(op, nick=_nick("oppf"), user="op")
        h.auth(user, nick=_nick("upf"), user="u")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, "JOIN #pfaila,#pfailb")
        _ = h.drain(op)
        h.send_line(op, "MODE #pfaila +k goodkey")
        _ = h.drain(op)

        h.send_line(user, "JOIN #pfaila,#pfailb badkey,")
        out = h.drain(user, duration=0.8)
        ok = h.has_numeric(out, 475) and h.has_numeric(out, 366) and "#pfailb" in out
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


def test_join_zero_parts_all_channels(h):
    h.start_server("JOIN 0 parts all channels")
    keep = user = None
    result = None
    try:
        keep = h.new_client()
        user = h.new_client()
        h.auth(keep, nick=_nick("keep"), user="k")
        h.auth(user, nick=_nick("j0"), user="u")
        _ = h.drain(keep)
        _ = h.drain(user)

        h.send_line(keep, "JOIN #zpa,#zpb")
        _ = h.drain(keep)
        h.send_line(user, "JOIN #zpa,#zpb")
        _ = h.drain(user)

        h.send_line(user, "JOIN 0")
        _ = h.drain(user, duration=0.6)

        h.send_line(user, "TOPIC #zpa :after_zero")
        out = h.drain(user)
        result = PASS if h.has_numeric(out, 442) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if keep:
            keep.close()
        if user:
            user.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_join_success_strict_order(h):
    h.start_server("JOIN success strict order")
    op = user = None
    result = None
    try:
        op = h.new_client()
        user = h.new_client()
        op_nick = _nick("ordo")
        user_nick = _nick("ordu")
        h.auth(op, nick=op_nick, user="op")
        h.auth(user, nick=user_nick, user="u")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, "JOIN #ordjoin")
        _ = h.drain(op)
        h.send_line(op, "TOPIC #ordjoin :topic_for_order")
        _ = h.drain(op)

        h.send_line(user, "JOIN #ordjoin")
        out = h.drain(user, duration=0.9)
        lines = _split_lines(out)

        join_idx = _find_index(
            lines, lambda ln: f":{user_nick}!" in ln and " JOIN #ordjoin" in ln
        )
        topic_idx = _find_index(lines, lambda ln: " 332 " in ln and " #ordjoin " in ln)
        names_idx = _find_index(lines, lambda ln: " 353 " in ln and " #ordjoin" in ln)
        end_idx = _find_index(lines, lambda ln: " 366 " in ln and " #ordjoin " in ln)

        ok = (
            join_idx != -1
            and topic_idx != -1
            and names_idx != -1
            and end_idx != -1
            and join_idx < topic_idx < names_idx < end_idx
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


def test_join_broadcast_recipients_strict(h):
    h.start_server("JOIN broadcast recipients strict")
    op = user = outsider = None
    result = None
    try:
        op = h.new_client()
        user = h.new_client()
        outsider = h.new_client()
        user_nick = _nick("brdu")
        h.auth(op, nick=_nick("brdo"), user="op")
        h.auth(user, nick=user_nick, user="u")
        h.auth(outsider, nick=_nick("brdx"), user="x")
        _ = h.drain(op)
        _ = h.drain(user)
        _ = h.drain(outsider)

        h.send_line(op, "JOIN #bcast")
        _ = h.drain(op)

        h.send_line(user, "JOIN #bcast")
        out_op = h.drain(op, duration=0.8)
        out_user = h.drain(user, duration=0.8)
        out_outsider = h.drain(outsider, duration=0.8)

        op_sees = f":{user_nick}!" in out_op and " JOIN #bcast" in out_op
        user_sees = f":{user_nick}!" in out_user and " JOIN #bcast" in out_user
        outsider_sees = (
            f":{user_nick}!" in out_outsider and " JOIN #bcast" in out_outsider
        )
        result = PASS if (op_sees and user_sees and not outsider_sees) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if op:
            op.close()
        if user:
            user.close()
        if outsider:
            outsider.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_join_invite_only_accept_after_invite(h):
    h.start_server("JOIN +i accept after invite")
    op = user = None
    result = None
    try:
        op = h.new_client()
        user = h.new_client()
        user_nick = _nick("invu")
        h.auth(op, nick=_nick("invo"), user="op")
        h.auth(user, nick=user_nick, user="u")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, "JOIN #invok")
        _ = h.drain(op)
        h.send_line(op, "MODE #invok +i")
        _ = h.drain(op)
        h.send_line(op, f"INVITE {user_nick} #invok")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(user, "JOIN #invok")
        out = h.drain(user)
        result = PASS if (h.has_numeric(out, 353) and h.has_numeric(out, 366)) else FAIL
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


def test_join_key_missing_param(h):
    h.start_server("JOIN key missing parameter")
    op = user = None
    result = None
    try:
        op = h.new_client()
        user = h.new_client()
        h.auth(op, nick=_nick("opkm"), user="op")
        h.auth(user, nick=_nick("ukm"), user="u")
        _ = h.drain(op)
        _ = h.drain(user)

        h.send_line(op, "JOIN #kmiss")
        _ = h.drain(op)
        h.send_line(op, "MODE #kmiss +k secret")
        _ = h.drain(op)

        h.send_line(user, "JOIN #kmiss")
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


def test_join_too_many_targets(h):
    h.start_server("JOIN too many targets")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("j407"), user="u")
        _ = h.drain(sock)
        targets = ",".join(f"#t{i}" for i in range(1, 90))
        h.send_line(sock, f"JOIN {targets}")
        out = h.drain(sock, duration=0.8)
        result = PASS if h.has_numeric(out, 407) else FAIL
    except Exception:
        result = EXCEPTION
    finally:
        if sock:
            sock.close()
        if not h.is_server_alive():
            result = CRASH
        h.stop_server()
    return result


def test_join_unavail_resource(h):
    h.start_server("JOIN unavailable resource")
    sock = None
    result = None
    try:
        sock = h.new_client()
        h.auth(sock, nick=_nick("j437"), user="u")
        _ = h.drain(sock)
        h.send_line(sock, "JOIN #temporarily_unavailable_resource")
        out = h.drain(sock)
        result = PASS if h.has_numeric(out, 437) else FAIL
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
            "JOIN success": test_join_success(h),
            "JOIN missing parameter": test_join_missing_param(h),
            "JOIN no such channel": test_join_no_such_channel(h),
            "JOIN +i reject without invite": test_join_invite_only_reject(h),
            "JOIN bad channel mask": test_join_bad_channel_mask(h),
            "JOIN bad key": test_join_bad_key(h),
            "JOIN channel full": test_join_channel_full(h),
            "JOIN banned from channel": test_join_banned_from_channel(h),
            "JOIN too many channels": test_join_too_many_channels(h),
            "JOIN multi channel success": test_join_multi_channel_success(h),
            "JOIN multi channel key mapping": test_join_multi_channel_key_mapping(h),
            "JOIN partial failure continue": test_join_partial_failure_continue(h),
            "JOIN 0 parts all channels": test_join_zero_parts_all_channels(h),
            "JOIN success strict order": test_join_success_strict_order(h),
            "JOIN broadcast recipients strict": test_join_broadcast_recipients_strict(
                h
            ),
            "JOIN +i accept after invite": test_join_invite_only_accept_after_invite(h),
            "JOIN key missing parameter": test_join_key_missing_param(h),
            "JOIN too many targets": test_join_too_many_targets(h),
            "JOIN unavailable resource": test_join_unavail_resource(h),
        }
    )


if __name__ == "__main__":
    results = run()
    for k, v in results.items():
        print(f"{k}: {v}")
