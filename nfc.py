# NFC - ACR122U
# Dan Jackson, 2026

import sys

class NfcReader:
    """
    ACR122U NFC Card Reader
    Dan Jackson, 2026
    """

    def __init__(self, port):
        self.port = port
        self.open = False
    
    def __del__(self):
        self.close()
    
    def __enter__(self):
        self.open()
        return self
    
    def __exit__(self, type, value, tb):
        self.close()
    

    def open(self):
        sys.stderr.write("NfcReader: Opening..." + self.port + '\n')
        self.open = True
        pass
    
    
    def close(self):
        if not self.open:
            return
        sys.stderr.write("NfcReader: Closing..." + self.port + '\n')
        self.open = False

    def read(self):
        if not self.open:
            raise Exception("NfcReader: Not open")
        card = None
        return card


# Example code if run from command-line
if __name__ == "__main__":
    port = '???'
    if len(sys.argv) > 1:
        port = sys.argv[1]
    with NfcReader(port) as nfc:
        card = nfc.read()
        if card:
            print("CARD: " + card)
