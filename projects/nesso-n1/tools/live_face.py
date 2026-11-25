#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Send a shared OpenToy pose, or relay Nesso touch to an OLED class board."""
import argparse
import asyncio
import re
from contextlib import AsyncExitStack

from bleak import BleakClient, BleakScanner

NUS = "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
RX = "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
TX = "6e400003-b5a3-f393-e0a9-e50e24dcca9e"
SERIAL = "00002a25-0000-1000-8000-00805f9b34fb"
MAC = re.compile(r"(?:[0-9a-f]{2}:){5}[0-9a-f]{2}")


class FaceLink:
    def __init__(self, client):
        self.client = client
        self.partial = bytearray()
        self.lines = asyncio.Queue(maxsize=32)

    def receive(self, _, data):
        self.partial.extend(data)
        while b"\n" in self.partial:
            line, _, remainder = self.partial.partition(b"\n")
            self.partial = remainder
            if self.lines.full():
                self.lines.get_nowait()
            self.lines.put_nowait(line.decode("ascii", errors="replace"))
        if len(self.partial) > 512:
            self.partial.clear()

    async def command(self, text, prefix):
        payload = text.encode("ascii")
        if not 1 <= len(payload) <= 63 or not payload.isascii() or any(
            byte < 32 or byte > 126 or byte == 59 for byte in payload
        ):
            raise ValueError("Send one printable OpenToy command, up to 63 characters.")
        while not self.lines.empty():
            self.lines.get_nowait()
        await self.client.write_gatt_char(RX, payload + b";", response=True)
        async with asyncio.timeout(5):
            while True:
                line = await self.lines.get()
                if line.startswith(prefix):
                    return line
                if line.startswith("ERR"):
                    raise RuntimeError(line)


async def connect(stack, name, factory_mac):
    device = await BleakScanner.find_device_by_filter(
        lambda _, adv: adv.local_name == name and NUS in adv.service_uuids,
        timeout=12,
    )
    if device is None:
        raise RuntimeError(f"{name} was not advertising the OpenToy service.")
    client = await stack.enter_async_context(BleakClient(device))
    actual = (await client.read_gatt_char(SERIAL)).decode("ascii").lower()
    if actual != factory_mac:
        raise RuntimeError(f"{name} reported a different factory MAC.")
    link = FaceLink(client)
    await client.start_notify(TX, link.receive)
    return link


async def run(args):
    async with AsyncExitStack() as stack:
        source = await connect(stack, *args.follow) if args.follow else None
        targets = [(name, await connect(stack, name, mac)) for name, mac in args.target]
        for name, link in targets:
            status = await link.command("status", "STATUS ")
            print(name, status, flush=True)
            if "doodle=1" not in status:
                raise RuntimeError(f"{name} does not advertise Face v2 support.")
        if source is not None:
            print("Touch the Nesso screen. Press Ctrl-C to stop the relay.", flush=True)
            while source.client.is_connected and all(link.client.is_connected for _, link in targets):
                try:
                    line = await asyncio.wait_for(source.lines.get(), timeout=2)
                except TimeoutError:
                    continue
                if line.startswith("EVENT doodle "):
                    for name, link in targets:
                        print(name, await link.command(line[6:], "OK "), flush=True)
            raise RuntimeError("A board disconnected. Start the relay again to reconnect.")
        if args.command:
            prefix = "STATUS " if args.command == "status" else "OK "
            for name, link in targets:
                print(name, await link.command(args.command, prefix), flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", nargs=2, action="append", required=True, metavar=("NAME", "FACTORY_MAC"))
    action = parser.add_mutually_exclusive_group()
    action.add_argument("--command")
    action.add_argument("--follow", nargs=2, metavar=("NESSO_NAME", "FACTORY_MAC"))
    args = parser.parse_args()
    specs = args.target + ([args.follow] if args.follow else [])
    for item in specs:
        item[1] = item[1].lower()
        if not MAC.fullmatch(item[1]):
            parser.error("Factory MACs use 02:00:00:00:00:0d form.")
    if len({item[1] for item in specs}) != len(specs):
        parser.error("Use a different factory MAC for each connected board.")
    try:
        asyncio.run(run(args))
    except KeyboardInterrupt:
        pass
    except (RuntimeError, TimeoutError, ValueError) as error:
        parser.exit(1, f"{str(error) or 'OpenToy response timed out.'}\n")


if __name__ == "__main__":
    main()
