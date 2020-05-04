#!/usr/bin/python
# -*- coding: utf8
"""A tool to download long logging data + various metadata from a LVA Geiger Counter connected to the serial port."""

import os, sys, math, random, re, datetime, time
import serial.threaded

USAGE = """Usage: download_log [-start|-end YYYY-MM-DD-HH:MM] [-title TITLE] [-sn DEVICE_SN] [-port SERIAL_PORT] [-o OUTFILE]

Tries to establish connection to the geiger counter using the PC link feature present in r331 and later.
If the log data is downloaded successfully, it will be written in a text file named like "data_YYYY-MM-DD-[SN#]-HH_MM.geigerlog",
where the date/time is the start of the recording.
Only the -start/-end parameter is strictly required, all others are optional.

Parameters:
  -start    - specify when the data collection started (when the geiger counter was switched on).
  -end      - when the data collection ended (start is inferred from this, using the length of recorded data).
              You can also say "now" here, in which case the current datetime will be used.
  -title    - optional textual label for this log. If not specified, will default to "Log #XXX from device #YYY"
  -sn       - specify device's serial number. If not specified, it will be read from device. If the device's serial
              number is not programmed in (via the SID command), it will be asked for interactively.
  -port     - which serial port to use; defaults to /dev/ttyUSB0 on Linux and COM1 on Windows
  -o        - output file name. Defaults to "data_YYYY-MM-DD-[SN#]-HH_MM.geigerlog" as described above.

---------------------------------------------------
LOG FILE FORMAT:

A typical log file looks like (without the "====" sentinels):
=================================
log_file_format: 1
title: Log #15 from device #161
sn: 161
start: 2020-04-06 03:43
end: 2020-04-10 11:51
timing_by: start
tube_factor_dev: 0.00570
tube_factor_used: 0.00570
*
LINE1
LINE2
LINE3
=================================

Where:
 - log_file_format: describes the exact format of this log file, currently 1.
 - timing_by: whether the log timing was given by its start (more accurate) or end (less accurate)
 - tube_factor_dev: tube sensitivity as given by the device (e.g. if the GETTM cmd returns 57/100, this will be 0.00570)
 - tube_factor_used: tube sensitivity, as you've calibrated it. Will be used as the authoritative number by decode_log
                     and other utilities. download_log will write it the same as tube_factor_dev, but you can change
                     it later.
 - LINE1, LINE2, LINE3: the actual log data, verbatim from what comes from the geiger counter device (result of RSLOG or
                        REELOG commands).
"""

class LogFile(object):
	def __init__(self):
		self.title = ""
		self.start = None
		self.end   = None
		self.timingBy = "none"
		self.sn    = 0
		self.logid = 0
		self.tubeNum = 0
		self.tubeDen = 1
		self.tubeFactorDev = 0.0
		self.tubeFactorUsed = 0.0
		self.dataLength = 0 # in seconds
		self.lines = []
		self.outFile = ""
	
	def defaultOutFile(self):
		self.outFile = self.start.strftime("data_%Y-%m-%d-") + "[%03d]" % self.sn + self.start.strftime("-%H_%M.geigerlog")
	
	def defaultTitle(self):
		self.title = "Log #%d from device #%03d" % (self.logid, self.sn)
		
	def serialize(self):
		if not self.title:
			self.defaultTitle()
		if not self.outFile:
			self.defaultOutFile()
		
		f = open(self.outFile, "wt")
		if not f:
			print "Cannot serialize, write error!"
			return False
		f.write("log_file_format: 1\n")
		f.write("title: %s\n" % self.title)
		f.write("sn: %03d\n" % self.sn)
		f.write(self.start.strftime("start: %Y-%m-%d %H:%M\n"))
		f.write(self.end.strftime("end: %Y-%m-%d %H:%M\n"))
		f.write("timing_by: %s\n" % self.timingBy)
		f.write("tube_factor_dev: %.5f\n" % self.tubeFactorDev)
		f.write("tube_factor_used: %.5f\n" % self.tubeFactorDev)
		f.write("*\n")
		for line in self.lines:
			f.write(line + "\n")
		f.close()
	
	def getDataLengthDescr(self):
		if self.dataLength < 60:
			return "%d seconds" % int(self.dataLength)
		minutes = math.floor(self.dataLength / 60 + 0.5)
		if minutes < 60:
			return "%d minutes" % int(minutes)
		hours = self.dataLength / 3600.0
		if hours < 24:
			return "%.1f hours" % hours
		return "%.1f days" % (self.dataLength / (3600.0 * 24))
	
def parseDateTime(s):
	"""Parse date/time in YYYY-MM-DD-HH:MM format. Also recognize the special string "now". """
	if s == "now":
		return datetime.datetime.now()
	items = map(int, s.replace("-", " ").replace(":", " ").split())
	return datetime.datetime(*items)

def cmd(serialHandle, command, responseLines=1):
	serialHandle.write(command + "\n")
	lines = []
	for i in xrange(responseLines):
		lines.append(serialHandle.readline()[:-1])
	if responseLines == 1:
		return lines[0]
	else:
		return lines

def main(args):
	f = LogFile()
	if len(args) == 2 and args[1] in ["-h", "--help"]:
		print USAGE
		return
	if not (len(args) % 2):
		print "Wrong number of arguments"
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
		
		if par == "start":
			f.start = parseDateTime(value)
			f.timingBy = "start"
		elif par == "end":
			f.end = parseDateTime(value)
			f.timingBy = "end"
		elif par == "title":
			f.title = value
		elif par == "sn":
			f.sn = int(value)
		elif par == "port":
			port = value
		elif par == "o":
			f.outFile = value
		else:
			print "Unknown switch: `%s'. Use -h for usage." % args[i]
			return
	
	if not ((f.start is None) ^ (f.end is None)):
		print "You haven't specified -start or -end (or you specified both). One of them is required."
		print "Use -h for usage"
		return
	
	ser = serial.Serial(port)
	if not ser.isOpen():
		print "Serial port cannot be opened (specify device with -port)"
	
	ser.write("SILENT\n")
	# wait a bit:
	time.sleep(0.5)
	ser.read_all()
	
	# try "HELO"
	if cmd(ser, "HELO")[:5] != "O HAI":
		print "Cannot connect: device does not respond to greeting"
		return
	
	devSn = int(cmd(ser, "GETID"))
	
	if devSn != 0 and f.sn != 0 and devSn != f.sn:
		print "Conflicting SN values: %03d from device, %03d given by you" % (devSn, f.sn)
		return
	
	if devSn == 0 and f.sn == 0:
		print "Device SN not known. Please enter it now"
		f.sn = int(raw_input("SN: ").strip())
	
	f.tubeNum, f.tubeDen = map(int, cmd(ser, "GETTM").split('/'))
	f.tubeFactorDev = f.tubeNum / float(f.tubeDen) / 100.0
	f.tubeFactorUsed = f.tubeFactorDev
	f.lines = cmd(ser, "REELOG", 3)
	items = f.lines[0].split(',')
	f.logid = int(items[0])
	f.dataLength = 15 * 2**int(items[1]) * int(items[3])
	if f.start:
		f.end = f.start + datetime.timedelta(seconds=f.dataLength)
	else:
		f.start = f.end - datetime.timedelta(seconds=f.dataLength)
	
	ser.close()
	f.serialize()
	print "%s' worth of logged data saved as `%s'" % (f.getDataLengthDescr(), f.outFile)

if __name__ == "__main__":
	main(sys.argv)
