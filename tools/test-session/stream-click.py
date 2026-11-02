#!/lsiopy/bin/python
# SPDX-FileCopyrightText: 2026 Ziad Arafat <ziadarafat1@gmail.com>
# SPDX-License-Identifier: CC0-1.0
#
# Drives the Selkies stream of the test desktop with headless Chromium, the way a browser on the Steam Deck would,
# and saves a screenshot of the page after each action as /tmp/clickN.png. Run inside the test container.
#
#   stream-click.py ACTION...
#     X,Y              click at page coordinates (the stream is 1280x656 at the top of the page)
#     X1,Y1-X2,Y2      drag from one point to the other
#     wait:SECONDS     pause without taking a screenshot
#     shot             take a screenshot without touching anything (a click on the desktop would unfocus the text field)
#
# Basic auth credentials come from the container environment (CUSTOM_USER, PASSWORD).
import asyncio, base64, json, os, subprocess, sys, urllib.request

import websockets


def parse(action):
    if action.startswith('wait:'):
        return ('wait', float(action[5:]))
    if action == 'shot':
        return ('shot',)
    if '-' in action:
        start, end = action.split('-')
        return ('drag', tuple(map(int, start.split(','))), tuple(map(int, end.split(','))))
    return ('click', tuple(map(int, action.split(','))))


async def main(actions):
    for _ in range(40):
        try:
            page = [t for t in json.load(urllib.request.urlopen('http://127.0.0.1:9222/json')) if t.get('type') == 'page'][0]
            break
        except Exception:
            await asyncio.sleep(0.5)
    async with websockets.connect(page['webSocketDebuggerUrl'], max_size=None) as ws:
        n = 0

        async def send(method, **params):
            nonlocal n
            n += 1
            mid = n
            await ws.send(json.dumps({'id': mid, 'method': method, 'params': params}))
            while True:
                message = json.loads(await ws.recv())
                if message.get('id') == mid:
                    return message.get('result', message.get('error'))

        async def mouse(kind, x, y, buttons):
            params = {'type': kind, 'x': x, 'y': y, 'buttons': buttons}
            if kind != 'mouseMoved':
                params.update(button='left', clickCount=1)
            await send('Input.dispatchMouseEvent', **params)

        await send('Page.enable')
        # Let the stream connect and the first frames arrive
        await asyncio.sleep(11)
        shot = 0
        for action in actions:
            if action[0] == 'wait':
                await asyncio.sleep(action[1])
                continue
            if action[0] == 'shot':
                pass
            elif action[0] == 'click':
                x, y = action[1]
                await mouse('mouseMoved', x, y, 0)
                await asyncio.sleep(0.2)
                await mouse('mousePressed', x, y, 1)
                await asyncio.sleep(0.08)
                await mouse('mouseReleased', x, y, 0)
            else:
                (x1, y1), (x2, y2) = action[1], action[2]
                await mouse('mouseMoved', x1, y1, 0)
                await asyncio.sleep(0.2)
                await mouse('mousePressed', x1, y1, 1)
                steps = 20
                for i in range(1, steps + 1):
                    await asyncio.sleep(0.03)
                    await mouse('mouseMoved', x1 + (x2 - x1) * i / steps, y1 + (y2 - y1) * i / steps, 1)
                await asyncio.sleep(0.1)
                await mouse('mouseReleased', x2, y2, 0)
            await asyncio.sleep(2.5)
            result = await send('Page.captureScreenshot', format='png')
            with open(f'/tmp/click{shot}.png', 'wb') as file:
                file.write(base64.b64decode(result['data']))
            shot += 1


actions = [parse(a) for a in sys.argv[1:]]
url = f"https://{os.environ['CUSTOM_USER']}:{os.environ['PASSWORD']}@127.0.0.1:3001/"
# Chromium hangs when it sees the desktop session's D-Bus or displays, so it gets none of them
chrome_env = {k: v for k, v in os.environ.items() if k not in ('DBUS_SESSION_BUS_ADDRESS', 'WAYLAND_DISPLAY', 'DISPLAY', 'XDG_RUNTIME_DIR')}
chrome = subprocess.Popen(['chromium-browser', '--headless=new', '--no-sandbox', '--ignore-certificate-errors',
                           '--remote-debugging-port=9222', '--remote-allow-origins=*', '--window-size=1280,800',
                           '--enable-unsafe-swiftshader', '--user-data-dir=/tmp/chr-cdp', url],
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, env=chrome_env)
try:
    asyncio.run(main(actions))
finally:
    chrome.terminate()
print('done')
