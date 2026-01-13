class Nfc {

    constructor(connectionString = null, eventCallback = null, log = null) {
        if (!connectionString) {
            connectionString = `ws://${location.host}/ws`;
        }
        this.connectionString = connectionString;
        this.eventCallback = eventCallback;
        this.log = log;

        // State
        this.reconnectTries = 0;
        this.readers = {};
    }

    connect() {
        if (this.log) this.log('WS: Opening ' + this.connectionString);
        this.ws = new WebSocket(this.connectionString);

        this.ws.addEventListener('open', () => {
            if (this.log) this.log('WS: Opened');
            this.reconnectTries = 0;
        });

        this.ws.addEventListener('message', (wsEvent) => {
            const eventData = JSON.parse(wsEvent.data);

            const newReader = !(eventData.reader in this.readers);
            const previousId = newReader ? '' : this.readers[eventData.reader];
            this.readers[eventData.reader] = eventData.card;

            const cardEvent = {
                source: this,
                reader: eventData.reader,
                id: eventData.card,
                previousId: previousId,
                readers: this.readers,
            };

            const message = JSON.stringify(cardEvent);
            if (this.log) this.log(message);

            if (this.eventCallback) this.eventCallback(cardEvent);
        });

        this.ws.addEventListener('close', () => {
            if (this.log) this.log('WS: Closed');

            // Schedule reconnection with backoff
            this.reconnectTries++;
            let reconnectInterval = (1.2 ** Math.min(this.reconnectTries, 10)) * 10;
            setTimeout(() => {
                this.connect(this.connectionString);
            }, reconnectInterval * 1000);
        });

        this.ws.addEventListener('error', (error) => {
            if (this.log) this.log(`WS: Error: ${error.message}`);
        });
    }
}

// Hack to export only if imported as a module (top-level await a regexp divided, otherwise an undefined variable divided followed by a comment)
if(0)typeof await/0//0; export default Nfc
