#Conversion to Python 3
import serial
import time
import sys
import glob


# global variables for module
startMarker = 60
endMarker = 62

#========================

def valToArduino(ledA, ledB, servo):
	sendStr = "{},{},{}".format(ledA, ledB, servo)
	print ("SENDSTR {}".format( sendStr))
	sendToArduino(sendStr)

#========================

def listSerialPorts():
	# http://stackoverflow.com/questions/12090503/listing-available-com-ports-with-python
	
    """Lists serial ports

    :raises EnvironmentError:
        On unsupported or unknown platforms
    :returns:
        A list of available serial ports
    """
    if sys.platform.startswith('win'):
        ports = ['COM{}'.format(i + 1) for i in range(256)]

    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this is to exclude your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')

    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')

    else:
        raise EnvironmentError('Unsupported platform')

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result

#========================

def setupSerial(serPort):
	
	global  ser
	
	baudRate = 115200
	ser = serial.Serial(serPort, baudRate)
	print ("Serial port " + serPort + " opened  Baudrate " + str(baudRate))

	#waitForArduino()

#========================

def closeSerial():
	
	global ser
	if 'ser' in globals():
		ser.close()
		print ("Serial Port Closed")
	else:
		print ("Serial Port Not Opened")

#========================

def sendToArduino(sendStr):
	
	global startMarker, endMarker, ser
	
	ser.write(chr(startMarker).encode('utf-8'))
	ser.write(sendStr.encode())
	ser.write(chr(endMarker).encode('utf-8'))


#===========================

def recvFromArduino(timeOut): # timeout in seconds eg 1.5
	
	global startMarker, endMarker, ser
	
	#~ print "Called with T-O %s" %(timeOut)
  
	dataBuf = ""
	x = "z" # any value that is not an end- or startMarker
	startTime = time.time()

	# wait for the start marker
	while  ord(x) != startMarker: 
		if time.time() - startTime >= timeOut:
			return('<<')
		if ser.inWaiting() > 0: # because ser.read() blocks
			x = ser.read().decode()
  

	# save data until the end marker is found
	while ord(x) != endMarker:
		if time.time() - startTime >= timeOut:
			return('>>')
		if ord(x) != startMarker:
			dataBuf = dataBuf + x
		if ser.inWaiting() > 0:
			x = ser.read().decode()
		else:
			x = chr(startMarker) # crude way to prevent repeat characters
								 #   when no data is received
			
	return(dataBuf)

#============================
      
      
def waitForArduino():

   # wait until the Arduino sends 'Arduino Ready' - allows time for Arduino reset
   # it also ensures that any bytes left over from a previous message are discarded
   
	print ("Waiting for Arduino to reset")
    
	msg = ""
	while msg.find("Arduino is ready") == -1:

		msg = recvFromArduino(10)

		# print (msg)
		print

