const { test } = require('node:test');
const assert = require('node:assert/strict');
const { spawn } = require('node:child_process');
const { mkdtemp, rm } = require('node:fs/promises');
const { join } = require('node:path');
const { tmpdir } = require('node:os');
const { WebSocketServer } = require('ws');

test('native plugin handles startup devices and repeated lifecycle events without an inspector appearance event', {
    skip: !process.env.DVM_TEST_EXECUTABLE, timeout: 20000,
}, async t => {
    const server = new WebSocketServer({ port: 0 });
    await new Promise(resolve => server.once('listening', resolve));
    const cwd = await mkdtemp(join(tmpdir(), 'dvm-protocol-'));
    let child;
    let inspectorBody;
    const states = [];
    t.after(async () => {
        child?.kill();
        for (const client of server.clients) client.terminate();
        await new Promise(resolve => server.close(resolve));
        await rm(cwd, { recursive: true, force: true });
    });
    const result = new Promise((resolve, reject) => {
        server.once('connection', socket => {
            const send = object => socket.send(JSON.stringify(object));
            socket.on('message', data => {
                const message = JSON.parse(data);
                if (message.event === 'registerPlugin') {
                    // No deviceDidConnect: this already-connected device was
                    // supplied in the startup info, as on the real Stream Deck.
                    send({ event: 'willAppear', device: 'test-device', context: 'test-action', action: 'com.farissoze.voicemixerfix.openmixer', payload: { settings: {}, controller: 'Keypad', coordinates: { column: 0, row: 0 } } });
                    send({ event: 'deviceDidConnect', device: 'test-device', deviceInfo: { type: 7, size: { columns: 4, rows: 2 } } });
                    // Stream Deck can announce an already visible action again
                    // during profile restoration. The old action's destructor
                    // must not remove the replacement from the context lookup.
                    send({ event: 'willAppear', device: 'test-device', context: 'test-action', action: 'com.farissoze.voicemixerfix.openmixer', payload: { settings: {}, controller: 'Keypad', coordinates: { column: 0, row: 0 } } });
                    // Deliberately omit propertyInspectorDidAppear. This request
                    // must recover the body even when that notification is lost.
                    send({ event: 'sendToPlugin', context: 'test-action', action: 'com.farissoze.voicemixerfix.openmixer', payload: { cmd: 'requestPropertyInspector' } });
                }
                if (message.event === 'sendToPropertyInspector' && message.payload.cmd === 'updateBody' && !inspectorBody) {
                    inspectorBody = message;
                    send({ event: 'sendToPlugin', context: 'test-action', action: 'com.farissoze.voicemixerfix.openmixer', payload: { cmd: 'reconnectDiscord' } });
                    // A double click must not start overlapping handshakes.
                    send({ event: 'sendToPlugin', context: 'test-action', action: 'com.farissoze.voicemixerfix.openmixer', payload: { cmd: 'reconnectDiscord' } });
                }
                if (inspectorBody && message.event === 'sendToPropertyInspector' && message.payload.cmd === 'updateDiscordConnection') {
                    states.push(message.payload);
                    if (message.payload.state === 'error' && states.some(s => s.state === 'connecting')) {
                        // Capture this completed attempt: a second queued click
                        // can publish a new connecting state before await resumes.
                        resolve({ body: inspectorBody, failure: message.payload, states: states.slice() });
                    }
                }
            });
        });
        const startupInfo = { devices: [{ id: 'test-device', type: 7, size: { columns: 4, rows: 2 } }] };
        child = spawn(process.env.DVM_TEST_EXECUTABLE, ['-port', String(server.address().port), '-pluginUUID', 'test-plugin', '-registerEvent', 'registerPlugin', '-info', JSON.stringify(startupInfo)], { cwd, windowsHide: true });
        child.once('error', reject);
        child.once('exit', code => reject(Error(`Plugin exited before replying: ${code}`)));
    });
    const completed = await result;
    const response = completed.body;
    assert.equal(response.context, 'test-action');
    assert.equal(response.payload.cmd, 'updateBody');
    assert.match(response.payload.content, /Discord settings/);
    assert.match(response.payload.content, /client_id/);
    assert.match(response.payload.content, /type="password"/);
    assert.match(response.payload.content, /id="discordReconnect"/);
    assert.match(response.payload.content, /إعادة الاتصال \/ <bdi lang="en">Reconnect<\/bdi>/);
    assert.ok(completed.states.some(s => s.state === 'connecting'));
    assert.match(completed.failure.reason, /Client ID/);
    assert.equal(child.exitCode, null, 'Reconnect must keep the same native process alive');
});
