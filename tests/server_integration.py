"""Headless TCP integration test for Retro Battle."""

import json
import socket
import struct
import subprocess
import sys
import time
from contextlib import ExitStack


def send(sock, message):
    data = json.dumps(
        message, separators=(",", ":")
    ).encode("utf-8")

    sock.sendall(struct.pack(">I", len(data)) + data)


def receive(sock):
    def exact(size):
        result = b""

        while len(result) < size:
            chunk = sock.recv(size - len(result))

            if not chunk:
                raise AssertionError(
                    "Server closed unexpectedly"
                )

            result += chunk

        return result

    length = struct.unpack(">I", exact(4))[0]
    assert 0 < length <= 4096

    return json.loads(exact(length))


def run(binary):
    # Pick an available local port.
    with socket.socket() as probe:
        probe.bind(("127.0.0.1", 0))
        port = probe.getsockname()[1]

    server = subprocess.Popen(
        [binary, "--port", str(port)],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
    )

    try:
        def connect():
            for _ in range(80):
                if server.poll() is not None:
                    raise AssertionError(
                        "Server exited before accepting connections"
                    )

                try:
                    sock = socket.create_connection(
                        ("127.0.0.1", port),
                        timeout=0.2,
                    )
                    sock.settimeout(2)
                    return sock
                except OSError:
                    time.sleep(0.05)

            raise AssertionError("Server did not begin listening")

        with ExitStack() as stack:
            # First client joins.
            alice = stack.enter_context(connect())
            send(alice, {"type": "hello", "version": 1})

            welcome = receive(alice)
            assert welcome["type"] == "welcome"
            assert len(welcome["players"]) == 1

            alice_id = welcome["player_id"]

            # Second client joins.
            bob = connect()
            send(bob, {"type": "hello", "version": 1})

            bob_welcome = receive(bob)
            bob_id = bob_welcome["player_id"]

            assert bob_id != alice_id
            assert {
                p["id"] for p in bob_welcome["players"]
            } == {alice_id, bob_id}

            joined = receive(alice)
            assert joined["type"] == "player_joined"
            assert joined["player"]["id"] == bob_id

            # Disconnect the second client.
            bob.close()

            assert receive(alice) == {
                "type": "player_left",
                "player_id": bob_id,
            }

            # Movement before hello is invalid.
            invalid = stack.enter_context(connect())

            send(invalid, {
                "type": "move",
                "direction": "north",
            })

            assert receive(invalid) == {
                "type": "error",
                "reason": "Expected hello",
            }

            invalid.close()

            # Test eight-player capacity.
            extra = []

            for _ in range(7):
                peer = stack.enter_context(connect())
                send(peer, {"type": "hello", "version": 1})

                response = receive(peer)
                assert response["type"] == "welcome"
                assert len(response["players"]) == len(extra) + 2

                for old in [alice, *extra]:
                    assert receive(old)["type"] == "player_joined"

                extra.append(peer)

            # Ninth connection must be refused.
            full = stack.enter_context(connect())

            assert receive(full) == {
                "type": "error",
                "reason": "Server full",
            }

            # Movement is not enabled until phase 3D.
            send(alice, {
                "type": "move",
                "direction": "north",
            })

            assert receive(alice) == {
                "type": "error",
                "reason": "Unexpected message",
            }

            print(
                "Server integration passed: "
                "handshake, joins, leaves, "
                "validation and capacity"
            )

    finally:
        server.terminate()

        try:
            server.communicate(timeout=3)
        except subprocess.TimeoutExpired:
            server.kill()
            server.communicate()


if __name__ == "__main__":
    run(sys.argv[1])
