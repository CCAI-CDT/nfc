from flask import Flask, send_from_directory
from flask_sock import Sock

from nfc import multi_reader

app = Flask(__name__)
sock = Sock(app)

@app.route('/')
def index():
    return send_from_directory('.', 'server.html')

@sock.route('/ws')
def echo(sock):
    while True:
        data = sock.receive()
        sock.send(data)

def card_callback(reader, card):
    # TODO: Send on all connected WebSocket clients?
    print('' + reader.device + '\t' + card)

multi_reader(card_callback)

# Run Flask server
app.run(host='0.0.0.0', port=5001)
