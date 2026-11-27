const { test } = require('node:test');
const assert = require('node:assert/strict');
const net = require('node:net');
const { spawn } = require('node:child_process');
const { mkdtemp, writeFile, rm } = require('node:fs/promises');
const { join } = require('node:path');
const { tmpdir } = require('node:os');

test('valid cached Discord sessions authenticate through IPC without refreshing over HTTPS', {
    skip: !process.env.DVM_AUTH_PROBE, timeout: 15000,
}, async t => {
    const cwd = await mkdtemp(join(tmpdir(), 'dvm-auth-'));
    await writeFile(join(cwd, 'discordOauth.json'), JSON.stringify({ access_token: 'synthetic-cached-token', refresh_token: 'synthetic-refresh-token' }));
    const clients = new Set();
    let authenticationCount = 0, stderr = '', child;
    const server = net.createServer(socket => {
        clients.add(socket); socket.on('close', () => clients.delete(socket));
        let buffer = Buffer.alloc(0);
        function send(opcode, packet) {
            const payload = Buffer.from(JSON.stringify(packet));
            const header = Buffer.alloc(8); header.writeUInt32LE(opcode); header.writeUInt32LE(payload.length, 4);
            socket.write(Buffer.concat([header, payload]));
        }
        socket.on('data', chunk => {
            buffer = Buffer.concat([buffer, chunk]);
            while (buffer.length >= 8) {
                const opcode = buffer.readUInt32LE(), length = buffer.readUInt32LE(4);
                if (buffer.length < 8 + length) return;
                const packet = JSON.parse(buffer.subarray(8, 8 + length)); buffer = buffer.subarray(8 + length);
                if (opcode === 0) {
                    send(1, { cmd: 'DISPATCH', evt: 'READY', data: { config: { cdn_host: 'unused.invalid' }, user: { id: 'recovery-test-user' } } });
                } else if (packet.cmd === 'AUTHENTICATE') {
                    authenticationCount++;
                    assert.equal(packet.args.access_token, 'synthetic-cached-token');
                    send(1, { cmd: 'AUTHENTICATE', nonce: packet.nonce, data: { user: { id: 'recovery-test-user' } } });
                } else {
                    throw Error('Unexpected authentication flow: ' + packet.cmd);
                }
            }
        });
    });
    t.after(async () => {
        child?.kill();
        for (const client of clients) client.destroy();
        await new Promise(resolve => server.close(resolve));
        await rm(cwd, { recursive: true, force: true });
    });
    await new Promise((resolve, reject) => {
        server.once('error', reject);
        server.listen('\\\\.\\pipe\\discord-ipc-0', resolve);
    });
    child = spawn(process.env.DVM_AUTH_PROBE, [], { cwd, windowsHide: true });
    child.stderr.on('data', data => { stderr += data; });
    const code = await new Promise((resolve, reject) => {
        child.once('error', reject); child.once('exit', resolve);
    });
    assert.equal(code, 0, stderr);
    assert.equal(authenticationCount, 1);
    assert.match(stderr, /Connected through pre-stored token/);
    assert.doesNotMatch(stderr, /Refreshing|REFRESH REQ|AUTH REQ/);
});
