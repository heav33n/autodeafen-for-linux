#!/usr/bin/env python3
"""Runs the real helper against a fake Discord and a fake mod."""
import os, pathlib, socket, stat, subprocess, sys, tempfile, threading

HELPER = sys.argv[1]
NONCE = "0123456789abcdef0123456789abcdef"
fails = 0

def check(name, ok):
    global fails
    print("PASS" if ok else "FAIL", name)
    fails += not ok

def exactly(sock, size):
    data = b""
    while len(data) < size:
        got = sock.recv(size - len(data))
        if not got:
            break
        data += got
    return data

def listener(family, address):
    s = socket.socket(family)
    s.settimeout(5)
    s.bind(address)
    s.listen(1)
    return s

def start(directory, port, nonce=NONCE, runtime=None, env=None):
    args = [HELPER, directory, str(port), nonce, directory if runtime is None else runtime]
    return subprocess.Popen(args, env=env or {}, stdin=subprocess.DEVNULL)

def proof(directory):
    return pathlib.Path(directory, "autodeafen-" + NONCE)

def session(directory, path="discord-ipc-0", **changes):
    discord = listener(socket.AF_UNIX, os.path.join(directory, path))
    mod = listener(socket.AF_INET, ("127.0.0.1", 0))
    process = start(directory, mod.getsockname()[1], **changes)
    game, peer = mod.accept()
    game.settimeout(5)
    status = exactly(game, 1)
    client, _ = discord.accept()
    client.settimeout(5)
    discord.close(); mod.close()
    return process, game, client, peer, status

with tempfile.TemporaryDirectory() as d:
    process, game, client, peer, status = session(d)
    check("proves which connection is its own", proof(d).read_text() == f"{peer[1]}\n")
    check("proof is private", stat.S_IMODE(proof(d).stat().st_mode) == 0o600)
    check("no temporary file left", [f for f in os.listdir(d) if f.endswith(".tmp")] == [])
    check("reports Discord found", status == b"K")
    game.sendall(b"hello"); check("game to Discord", exactly(client, 5) == b"hello")
    client.sendall(b"READY"); check("Discord to game", exactly(game, 5) == b"READY")
    blob = os.urandom(2_000_000)
    for name, a, b in (("large message to Discord", game, client), ("large message to game", client, game)):
        t = threading.Thread(target=a.sendall, args=(blob,)); t.start()
        check(name, exactly(b, len(blob)) == blob); t.join()
    game.close()
    check("game closing closes Discord", client.recv(1) == b"")
    check("exits after the game closes", process.wait(5) == 0)

with tempfile.TemporaryDirectory() as d:
    process, game, client, _, _ = session(d)
    client.sendall(b"bye"); client.close()
    check("last message is delivered", exactly(game, 3) == b"bye")
    check("Discord closing closes the game side", game.recv(1) == b"")
    check("exits after Discord closes", process.wait(5) == 0)

with tempfile.TemporaryDirectory() as d:
    process, game, client, _, _ = session(d, "discord-ipc-7")
    check("finds a later socket", True)
    game.close(); client.close(); process.wait(5)

with tempfile.TemporaryDirectory() as d:
    process, game, client, _, status = session(d, runtime="", env={"XDG_RUNTIME_DIR": d})
    check("falls back to XDG_RUNTIME_DIR", status == b"K")
    game.close(); client.close(); process.wait(5)

with tempfile.TemporaryDirectory() as d:
    mod = listener(socket.AF_INET, ("127.0.0.1", 0))
    process = start(d, mod.getsockname()[1])
    game, _ = mod.accept(); game.settimeout(5)
    check("reports Discord missing", exactly(game, 1) == b"N")
    check("exits when Discord is missing", process.wait(5) == 0)

with tempfile.TemporaryDirectory() as d:
    check("no arguments fails", subprocess.run([HELPER], timeout=5).returncode == 2)
    check("short nonce fails", start(d, 1, nonce="abc").wait(5) == 2)
    check("nonce cannot leave the folder", start(d, 1, nonce="../" + "a" * 29).wait(5) == 2)
    check("bad port fails", start(d, "x").wait(5) == 2)
    check("huge port fails", start(d, 70000).wait(5) == 2)
    mod = listener(socket.AF_INET, ("127.0.0.1", 0))
    check("missing folder fails", start(d + "/nope", mod.getsockname()[1]).wait(5) == 4)
    proof(d).symlink_to("/tmp/autodeafen-should-not-exist")
    process = start(d, mod.getsockname()[1]); process.wait(5)
    check("never writes through a link", not os.path.exists("/tmp/autodeafen-should-not-exist"))
    free = listener(socket.AF_INET, ("127.0.0.1", 0)); port = free.getsockname()[1]; free.close()
    check("nobody listening fails", start(d, port).wait(5) == 3)

print(f"{fails} failed")
sys.exit(fails != 0)
