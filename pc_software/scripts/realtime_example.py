#!/usr/bin/python
# -*- coding: utf8
"""A tool for reading real-time radiation data from a LVA Geiger Counter connected to the serial port."""

import os, sys, time, serial.threaded

USAGE = """Usage: realtime_example [-port SERIAL_PORT]"""

def main(args):
	if len(args) == 2 and args[1] in ["-h", "--help"]:
		print USAGE
		return
	
	# automatic port selection (depending on OS):
	if sys.platform == "win32":
		port = "COM1"
	else:
		port = "/dev/ttyUSB0"
	
	# parse cmdline args:
	for i in xrange(1, len(args), 2):
		par = args[i][1:]
		value = args[i + 1]
		
		if par == "port":
			port = value
		else:
			print "Unknown switch: `%s'. Use -h for usage." % args[i]
			return
	
	ser = serial.Serial(port)
	if not ser.isOpen():
		print "Serial port cannot be opened (specify device with -port)"
		return
	
	""" REAL TIME DATA USAGE EXAMPLE:
	    ===============================
	    Accumulate GM discharge counts in a 5-minute window (much better averaging for typical background levels
	    than 1-minute window the Geiger Counter itself uses). Print reports each 15 seconds.
	    
	    Both numbers can be tuned below:
	"""
	
	MAX_BUFF_SIZE = 300 # seconds (i.e. 5 minutes)
	PRINT_RATE    = 15  # each 15 seconds
	buff = []
	i = 0
	
	while True:
		s = ser.readline()[:-1]
		if s.startswith("CPS,"): # the line looks like "CPS, 1, CPM, 22, uSv/hr, 0.12, SLOW"
			counts = int(s.split(",")[1]) # fetch the int after "CPS"
			if len(buff) < MAX_BUFF_SIZE:
				buff.append(counts)
			else:
				buff[i] = counts
			i += 1
			i %= MAX_BUFF_SIZE
			if i % PRINT_RATE == 0:
				cpm = float(sum(buff)) / (len(buff) / 60.0)
				timestamp = time.strftime("%Y-%m-%d %H:%M:%S")
				print "[%s] (len=%.2fm) %.1f CPM" % (timestamp, len(buff) / 60.0, cpm)

if __name__ == "__main__":
	main(sys.argv)
