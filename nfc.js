class Nfc {

    constructor(connectionString = null, eventCallback = null, exclusives = null, log = null) {
        if (!connectionString) {
            connectionString = `ws://${location.host}/ws`;
        }
        this.connectionString = connectionString;
        this.eventCallback = eventCallback;
        this.exclusives = exclusives;
        this.log = log;

        // State
        this.reconnectTries = 0;
        this.readers = {};
        this.exclusiveMapping = {};
        this.exclusiveReader = {};

        // Prepare mutually-exclusive group mapping
        if (this.exclusives) {
            for (const [exclusiveId, ids] of Object.entries(this.exclusives)) {
                for (const id of ids) {
                    if (id in this.exclusiveMapping) {
                        throw new Error(`Duplicate exclusive ID mapping for ID ${id} -- already mapped to ${this.exclusiveMapping[id]} -- cannot also map to ${exclusiveId}.`);
                    }
                    this.exclusiveMapping[id] = exclusiveId;
                }
            }
        }
        // Prepare exclusive state
        if (this.exclusives) {
            for (const exclusiveId of Object.keys(this.exclusives)) {
                this.exclusiveReader[exclusiveId] = null;
            }
        }
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

            // Handle exclusives
            let notExclusive = eventData.card ? true : false;
            let previousExclusiveId = null;
            let newExclusiveId = null;
            if (this.exclusives) {
                if (previousId in this.exclusiveMapping) {
                    previousExclusiveId = this.exclusiveMapping[previousId];
                    this.exclusiveReader[previousExclusiveId] = null;
                }
                if (eventData.card && eventData.card in this.exclusiveMapping) {
                    notExclusive = false;
                    newExclusiveId = this.exclusiveMapping[eventData.card];
                    this.exclusiveReader[newExclusiveId] = eventData.reader;
                }
            }
            let exclusiveState = {};
            for (const [exclusiveId, reader] of Object.entries(this.exclusiveReader)) {
                exclusiveState[exclusiveId] = {
                    name: exclusiveId,
                    reader: reader,
                    id: reader ? this.readers[reader] : null,
                    changed: (exclusiveId === newExclusiveId) ? 'new' : ((exclusiveId === previousExclusiveId) ? 'removed' : false),
                    index: reader && this.readers[reader] ? Object.values(this.exclusives[exclusiveId]).indexOf(this.readers[reader]) : null,
                };
            }

            const cardEvent = {
                source: this,
                reader: eventData.reader,
                id: eventData.card,
                previousId: previousId,
                readers: this.readers,
                notExclusive: notExclusive,
                exclusiveState: exclusiveState,
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
