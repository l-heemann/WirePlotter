import serial
ser = serial.Serial("/dev/ttyUSB0", 9600)

def awaitAck():
    resp = ser.readline();
    # print(resp)
    while resp != b'ack\r\n':
        print(resp);
        resp = ser.readline();

def moveTo(pos):
    s = str(pos) + "\r\n";
    print(s.encode('utf-8'))
    print(b'100\r\n')
    ser.write(s.encode('utf-8'))
    awaitAck()

print("move to 000")
moveTo(0)
print("move to 400")
moveTo(400)



