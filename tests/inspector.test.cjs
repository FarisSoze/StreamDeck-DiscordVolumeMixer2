const { test } = require('node:test');
const assert = require('node:assert/strict');
const { readFileSync } = require('node:fs');
const { join } = require('node:path');
const vm = require('node:vm');

const script = readFileSync(join(__dirname, '../deps/qtstreamdeck2-local/dist/qtstreamdeck2/propertyinspector.js'), 'utf8');

function fixture() {
    const timers = new Map(), sockets = [], events = {};
    const body = { innerHTML: 'Loading property inspector...' };
    const status = { textContent: '', hidden: true };
    const label = { textContent: '' }, reason = { textContent: '', hidden: true }, button = { disabled: false };
    const elements = { body, connectionStatus: status, discordConnectionLabel: label,
        discordConnectionReason: reason, discordReconnect: button };
    let timerID = 0;
    class Socket {
        static OPEN = 1;
        constructor(url) { this.url = url; this.readyState = 0; this.sent = []; sockets.push(this); }
        send(text) { if (this.readyState !== 1) throw Error('closed'); this.sent.push(JSON.parse(text)); }
        open() { this.readyState = 1; this.onopen?.(); }
        close() { this.readyState = 3; this.onclose?.(); }
        receive(value) { this.onmessage?.({ data: typeof value === 'string' ? value : JSON.stringify(value) }); }
    }
    const context = vm.createContext({
        WebSocket: Socket,
        document: { getElementById: id => elements[id] || null },
        window: { addEventListener: (name, fn) => { events[name] = fn; } },
        setTimeout: (fn, delay) => { const id = ++timerID; timers.set(id, { fn, delay }); return id; },
        clearTimeout: id => timers.delete(id),
    });
    vm.runInContext(script, context);
    const connect = () => context.connectElgatoStreamDeckSocket('1234', 'pi-context', 'registerPropertyInspector', '{}', '{"action":"mixer"}');
    const next = () => {
        const [id, timer] = timers.entries().next().value;
        timers.delete(id); timer.fn(); return timer.delay;
    };
    const reply = socket => socket.receive({ event: 'sendToPropertyInspector', payload: { cmd: 'updateBody', content: '<input id="setting">' } });
    connect();
    return { context, timers, sockets, body, status, label, reason, button, events, connect, next, reply };
}

test('requests the body after registration when the initial appearance notification was missed', () => {
    const f = fixture(), s = f.sockets[0]; s.open();
    assert.equal(s.sent[0].event, 'registerPropertyInspector');
    assert.equal(s.sent[1].payload.cmd, 'requestPropertyInspector');
    assert.equal(s.sent[1].context, 'pi-context');
    f.reply(s);
    assert.equal(f.body.innerHTML, '<input id="setting">');
    assert.equal(f.timers.size, 0);
});

test('recovers after a dropped request and stops retries when the response arrives', () => {
    const f = fixture(), s = f.sockets[0]; s.open();
    assert.equal(f.next(), 500);
    assert.equal(s.sent.filter(m => m.payload?.cmd === 'requestPropertyInspector').length, 2);
    f.reply(s);
    f.body.innerHTML = 'unsaved user edit';
    assert.equal(f.timers.size, 0);
    assert.equal(f.body.innerHTML, 'unsaved user edit');
});

test('a slow native plugin gets status text and bounded retry intervals', () => {
    const f = fixture(); f.sockets[0].open();
    const delays = Array.from({ length: 8 }, () => f.next());
    assert.ok(delays.every(d => d <= 5000));
    assert.match(f.status.textContent, /Retrying/);
    f.reply(f.sockets[0]);
    assert.equal(f.status.hidden, true);
});

test('reconnects and requests a fresh body after a socket disconnect', () => {
    const f = fixture(), old = f.sockets[0]; old.open(); f.reply(old); old.close();
    assert.match(f.status.textContent, /Reconnecting/);
    assert.equal(f.next(), 1000);
    const current = f.sockets[1]; current.open();
    assert.equal(current.sent[1].payload.cmd, 'requestPropertyInspector');
    old.receive({ event: 'sendToPropertyInspector', payload: { cmd: 'updateBody', content: 'stale' } });
    assert.notEqual(f.body.innerHTML, 'stale');
    f.reply(current);
    assert.equal(f.timers.size, 0);
});

test('malformed or unknown messages do not terminate recovery', () => {
    const f = fixture(), s = f.sockets[0]; s.open();
    for (const message of ['bad json', 'null', '{}', { event: 'sendToPropertyInspector' },
        { event: 'sendToPropertyInspector', payload: { cmd: 'constructor' } },
        { event: 'sendToPropertyInspector', payload: { cmd: 'updateBody', content: null } }]) {
        assert.doesNotThrow(() => s.receive(message));
    }
    assert.equal(f.timers.size, 1);
    f.reply(s); assert.equal(f.timers.size, 0);
});

test('settings changes still use the original protocol and disconnected sends are safe', () => {
    const f = fixture(), s = f.sockets[0]; s.open(); f.reply(s);
    f.context.notifyValueChanged('volume', '50');
    assert.deepEqual(s.sent.at(-1).payload, { cmd: 'valueChanged', element: 'volume', value: '50' });
    s.close();
    assert.doesNotThrow(() => f.context.notifyValueChanged('volume', '60'));
    assert.doesNotThrow(() => f.context.openUrl('https://example.com'));
});

test('unloading cancels recovery without reconnecting a closed inspector', () => {
    const f = fixture(); f.sockets[0].open(); f.events.unload();
    assert.equal(f.timers.size, 0);
});

test('reinitialization replaces the socket without creating duplicate reconnect loops', () => {
    const f = fixture(); f.sockets[0].open(); f.connect();
    assert.equal(f.timers.size, 0);
    f.sockets[1].open(); f.reply(f.sockets[1]);
    assert.equal(f.timers.size, 0);
});

test('live Discord status updates preserve the settings form and expose the error reason safely', () => {
    const f = fixture(), s = f.sockets[0]; s.open(); f.reply(s);
    f.body.innerHTML = 'settings being edited';
    s.receive({ event: 'sendToPropertyInspector', payload: { cmd: 'updateDiscordConnection', state: 'error', reason: '<open Discord>' } });
    assert.equal(f.label.textContent, 'تعذّر الاتصال');
    assert.equal(f.reason.textContent, '<open Discord>');
    assert.equal(f.reason.hidden, false);
    assert.equal(f.button.disabled, false);
    assert.equal(f.body.innerHTML, 'settings being edited');
    s.receive({ event: 'sendToPropertyInspector', payload: { cmd: 'updateDiscordConnection', state: 'connected', reason: '' } });
    assert.equal(f.label.textContent, 'متصل');
    assert.equal(f.reason.hidden, true);
});

test('reconnect requests update only connection feedback and handle a lost Stream Deck socket', () => {
    const f = fixture(), s = f.sockets[0]; s.open(); f.reply(s);
    f.context.reconnectDiscord();
    assert.equal(s.sent.at(-1).payload.cmd, 'reconnectDiscord');
    assert.equal(f.label.textContent, 'جاري الاتصال…');
    assert.equal(f.button.disabled, true);
    s.close();
    assert.doesNotThrow(() => f.context.reconnectDiscord());
    assert.match(f.status.textContent, /Stream Deck/);
});
