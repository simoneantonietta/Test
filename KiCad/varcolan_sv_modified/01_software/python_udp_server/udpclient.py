import socket
import sys

#UDP_IP = 'localhost'
UDP_IP = "127.0.0.1"
UDP_PORT = 5000

# Create a UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

#server_address = ('localhost', 5000)
server_address = (UDP_IP, UDP_PORT)
message = 'This is the message.  It will be repeated.'

try:

    # Send data
    print >>sys.stderr, 'sending "%s"' % message
    sent = sock.sendto(message, server_address)

    # Receive response
    print >>sys.stderr, 'waiting to receive'
    data, server = sock.recvfrom(4096)
    print >>sys.stderr, 'received "%s"' % data

finally:
    print >>sys.stderr, 'closing socket'
    sock.close()
