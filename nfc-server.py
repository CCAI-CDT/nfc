import threading

from flask import Flask, send_from_directory
from flask_sock import Sock

from nfc import multi_reader

app = Flask(__name__)
sock = Sock(app)
connections = []

@app.route('/')
def index():
    return send_from_directory('.', 'test.html')

@app.route('/nfc.js')
def nfc_js():
    return send_from_directory('.', 'nfc.js')

@sock.route('/ws')
def echo(sock):
    try:
        connections.append(sock)
        while True:
            data = sock.receive()
            print("Received: " + data)
            #sock.send(data)
    except:
        print("WebSocket connection error.")
    finally:
        if sock in connections:
            connections.remove(sock)

def card_callback(reader, card):
    print('' + reader.device + '\t' + card)
    # Create JSON message
    message = f'{{"reader": "{reader.device}", "card": "{card}"}}'
    # Send to all connected WebSocket clients
    for conn in connections:
        try:
            conn.send(message)
        except:
            print("Failed to send message to a WebSocket client.")

# Run reader in a separate thread
def reader():
    multi_reader(card_callback)
reader_thread = threading.Thread(target=reader)
reader_thread.start()

# Run Flask server
host = '127.0.0.1'
port = 5001
print(f'Starting server at http://{host}:{port}')
app.run(host, port)

reader_thread.join()
