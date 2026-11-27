var websocket = null,
    uuid = null,
    actionInfo = {};

let requestTimer = null;
let reconnectTimer = null;
let requestAttempts = 0;
let reconnectAttempts = 0;
let inspectorLoaded = false;
let disposed = false;

function setConnectionStatus(message) {
    const status = document.getElementById("connectionStatus");
    if (status) {
        status.textContent = message;
        status.hidden = !message;
    }
}

function sendToStreamDeck(message) {
    if (!websocket || websocket.readyState !== WebSocket.OPEN) return false;
    try {
        websocket.send(JSON.stringify(message));
        return true;
    } catch (_) {
        return false;
    }
}

function requestPropertyInspector() {
    clearTimeout(requestTimer);
    if (disposed || inspectorLoaded) return;
    sendToStreamDeck({
        event: "sendToPlugin",
        context: uuid,
        action: actionInfo.action,
        payload: { cmd: "requestPropertyInspector" },
    });
    requestAttempts++;
    if (requestAttempts >= 4) {
        setConnectionStatus("Waiting for Discord Volume Mixer. Retrying automatically...");
    }
    // Stop when the body arrives so retries cannot replace a form being edited.
    requestTimer = setTimeout(requestPropertyInspector,
        Math.min(500 * Math.pow(2, requestAttempts - 1), 5000));
}

const commandFunctions = {
    updateDiscordConnection: function (msg) {
        const label = document.getElementById("discordConnectionLabel");
        const reason = document.getElementById("discordConnectionReason");
        const button = document.getElementById("discordReconnect");
        if (!label || !reason || !button) return;
        label.textContent = msg.state === "connected" ? "متصل"
            : msg.state === "connecting" ? "جاري الاتصال…" : "تعذّر الاتصال";
        reason.textContent = typeof msg.reason === "string" ? msg.reason : "";
        reason.hidden = !reason.textContent;
        button.disabled = msg.state === "connecting";
    },
    updateBody: function (msg) {
        if (typeof msg.content !== "string") return;
        document.getElementById("body").innerHTML = msg.content;
        inspectorLoaded = true;
        clearTimeout(requestTimer);
        requestTimer = null;
        setConnectionStatus("");
    },
};

function connectElgatoStreamDeckSocket(inPort, inUUID, inRegisterEvent, inInfo, inActionInfo) {
    disposed = false;
    clearTimeout(requestTimer);
    clearTimeout(reconnectTimer);
    if (websocket) {
        websocket.onclose = null;
        websocket.close();
    }
    uuid = inUUID;
    actionInfo = JSON.parse(inActionInfo);
    inspectorLoaded = false;
    reconnectAttempts = 0;

    function connect() {
        if (disposed) return;
        const socket = new WebSocket('ws://localhost:' + inPort);
        websocket = socket;
        socket.onopen = function () {
            if (disposed || websocket !== socket) return;
            reconnectAttempts = 0;
            requestAttempts = 0;
            inspectorLoaded = false;
            // Register before asking Stream Deck to route a reply to this PI.
            sendToStreamDeck({ event: inRegisterEvent, uuid: inUUID });
            requestPropertyInspector();
        };
        socket.onmessage = function (ev) {
            if (disposed || websocket !== socket) return;
            let message;
            try { message = JSON.parse(ev.data); } catch (_) { return; }
            if (!message || message.event !== "sendToPropertyInspector") return;
            const payload = message.payload;
            if (!payload || !Object.prototype.hasOwnProperty.call(commandFunctions, payload.cmd)) return;
            commandFunctions[payload.cmd](payload);
        };
        socket.onclose = function () {
            if (disposed || websocket !== socket) return;
            clearTimeout(requestTimer);
            setConnectionStatus("Connection to Stream Deck lost. Reconnecting...");
            reconnectTimer = setTimeout(connect,
                Math.min(1000 * Math.pow(2, reconnectAttempts++), 10000));
        };
        // Error is followed by close; reconnect in one place to avoid duplicates.
        socket.onerror = function () {};
    }
    connect();
}

function notifyValueChanged(elementID, value) {
    sendToStreamDeck({
        event: "sendToPlugin",
        context: uuid,
        action: actionInfo.action,
        payload: { cmd: "valueChanged", element: elementID, value: value },
    });
}

function reconnectDiscord() {
    if (sendToStreamDeck({
        event: "sendToPlugin", context: uuid, action: actionInfo.action,
        payload: { cmd: "reconnectDiscord" },
    })) {
        commandFunctions.updateDiscordConnection({ state: "connecting", reason: "" });
    } else {
        setConnectionStatus("تعذّر التواصل مع Stream Deck. انتظر استعادة الاتصال ثم حاول مجددًا.");
    }
}

function openUrl(url) {
    sendToStreamDeck({ event: "openUrl", payload: { url: url } });
}

window.addEventListener("unload", function () {
    disposed = true;
    clearTimeout(requestTimer);
    clearTimeout(reconnectTimer);
    if (websocket) websocket.close();
});
