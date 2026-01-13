# NFC - ACR122U
# Dan Jackson, 2026

import sys
import subprocess
import threading
from time import sleep

BINARY='./nfc-poll'
DEVICE_FILTER='acr122_usb:'


class NfcReader:
    """
    ACR122U NFC Card Reader
    Dan Jackson, 2026
    """

    def __init__(self, device = None, callback=None):
        self.device = device
        self.callback = callback
        self.proc = None
        self.thread = None

    def __del__(self):
        self.close()
    
    def __enter__(self):
        self.open()
        return self
    
    def __exit__(self, type, value, tb):
        self.close()
    

    def list_devices():
        #sys.stderr.write("NfcReader: Listing devices...\n")
        result = subprocess.run([BINARY, '-l'], capture_output=True, text=True)
        if result.returncode != 0:
            print("RETURN: " + str(result.returncode))
            print("STDOUT: " + result.stdout)
            print("STDERR: " + result.stderr)
            raise Exception("NfcReader: Failed to list devices: " + result.stderr)
        # Parse result.stdout to find devices (one per line)
        devices = []
        for line in result.stdout.splitlines():
            line = line.strip()
            if line:
                devices.append(line)
        return devices

    def open(self):
        if self.thread:
            raise Exception("NfcReader: Already open")

        #sys.stderr.write("NfcReader: Opening..." + str(self.device) + '\n')
        self.proc = None
        self.thread = threading.Thread(target=self.start)
        self.thread.start()

    def start(self):
        try:
            command = [BINARY]
            if self.device:
                command.append(self.device)
            self.proc = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True, cwd='.', shell=False)
            #os.set_blocking(proc.stdout.fileno(), False)  # Not supported on Windows
            while self.proc:
                if self.proc.poll() is None:
                    card = self.proc.stdout.readline().strip()  # Empty string if none
                    if self.callback:
                        self.callback(self, card)
                else:
                    sys.stderr.write("NfcReader: Process exited for device " + str(self.device) + '\n')
                    self.proc = None
                    break
        finally:
            sys.stderr.write("NfcReader: Exiting reader thread for device " + str(self.device) + '\n')
            if self.proc:
                self.proc.terminate()
                sys.stderr.write("NfcReader: Waiting for process termination for device " + str(self.device) + '\n')
                self.proc.wait()
                self.proc = None
            sys.stderr.write("NfcReader: Leaving thread " + str(self.device) + '\n')
            self.thread = None


    def wait(self):
        if self.thread:
            sys.stderr.write("NfcReader: Waiting/reading for device " + str(self.device) + '\n')
            self.thread.join()
    
    def close(self):
        sys.stderr.write("NfcReader: Closing..." + str(self.device) + '\n')
        if self.proc:
            self.proc.terminate()
        if self.thread:
            self.thread.join()


def multi_reader(callback):
    devices = NfcReader.list_devices()

    # Filter devices
    if DEVICE_FILTER:
        devices = [d for d in devices if DEVICE_FILTER in d]

    if not devices:
        print("! No NFC devices found.")
        # If true, fake events with delays
        if False:
            sleep(5)
            callback("FAKE", "1234")
            sleep(5)
            callback("FAKE", "")
    else:
        for i, device in enumerate(devices):
            print(f"#{i} {device}")

    readers = []
    for device in devices:
        new_reader = NfcReader(device, callback)
        readers.append(new_reader)
    
    for reader in readers:
        reader.open()

    for reader in readers:
        reader.wait()


# Example code if run from command-line
if __name__ == "__main__":
    def card_callback(reader, card):
        print('' + reader.device + '\t' + card)
    multi_reader(card_callback)
