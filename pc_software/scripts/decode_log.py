#!/usr/bin/python
# -*- coding: utf8
"""A graphing tool to visualize LVA Geiger Counter logged data (collected by the long logging functionality).
   It uses GNUplot to draw the charts - you must have it installed and added to your PATH."""

import os, sys, math, random, re, datetime, tempfile, time

USAGE = """Usage: decode_log FILE1 [FILE2 ...] [-o Outfile.png] [options...]

Each file to be plotted is a separate data dump of the Geiger counter.

-o Sends the graph to a .png file, instead of displaying it on-screen.

Available options:
	-grid        show grid
	-range X-Y   specify the radiation range on the Y axis (in units uSv/h); disable the automatic selection
	-keepFiles   keep the temporary gnuplot files instead of deleting them
"""


SCRIPTDIR = ""

class DumpDataError(Exception):
	def __init__(self, fn, error):
		self.fn = fn
		self.error = error
	def __str__(self):
		return "Dump file format error in file %s: %s" % (self.fn, self.error)

class RadiationLog(object):
	def __init__(self, filename):
		self.resolution = 0
		self.scaling = 0
		self.logId = 0
		self.samples = []
		self.sampleLen = 15
		self.start = None
		self.timingBy = "none"
		self.filename = filename
		self.title = ""
		self.cpm_to_svh = 0.0
		self.counts_to_sv = 0.0
		self.sn = 0
		self.timemult = 1
		self.timeunit = "minutes"
	
	def _parseDataFromGeigerCounter(self, line1, line2, line3):
		n = 0
		self.logId, self.resolution, self.scaling, n = map(int, line1.strip().split(','))
		samples = map(int, line2.strip().split(','))
		if n != len(samples):
			raise DumpDataError(fn, "Indicated number of samples doesn't match the actual list!")
		self.samples = map(lambda x: x * 2**self.scaling, samples)
		self.sampleLen = 15 * 2**self.resolution
		# determine X axis value:
		self.timemult, self.timeunit = self.determineXAxis()
	
	@staticmethod
	def _parseDateTimeFmt(s):
		items = map(int, s.replace("-", " ").replace(":", " ").split())
		return datetime.datetime(*items)
	
	def _parseLogHeaderField(self, field, data):
		if field == "log_file_format":
			if data != "1":
				return "Unsupported log file format version"
		elif field == "title":
			self.title = data
		elif field == "sn":
			self.sn = int(data)
		elif field == "start":
			self.start = self._parseDateTimeFmt(data)
		elif field == "end":
			self.end = self._parseDateTimeFmt(data)
		elif field == "timing_by":
			self.timingBy = data
		elif field == "tube_factor_used":
			self.cpm_to_svh = float(data) / 10**6
		return ""
	
	def parseGeigerLogData(self):
		deviceLines = []
		gather = False
		f = open(self.filename, "rt")
		if not f:
			return "Cannot open file: %s" % fn
		for line in f:
			if gather:
				deviceLines.append(line)
			elif line.strip() == "*":
				gather = True
			else:
				i = line.find(":")
				if i > 0:
					status = self._parseLogHeaderField(line[:i], line[i + 2:].rstrip())
					if status != "":
						f.close()
						return status
		f.close()
		assert len(deviceLines) >= 3, "File too short (expecting 3 lines after REELOG)"
		self._parseDataFromGeigerCounter(deviceLines[0], deviceLines[1], deviceLines[2])
		return ""

	def determineXAxis(self):
		totalMinutes = len(self.samples) * self.sampleLen
		scaling = 1.0 / 60
		if totalMinutes * scaling < 500: # less than 500 minutes, or ~8 hours:
			return scaling, "minutes"
		
		scaling /= 60.0
		if totalMinutes * scaling < 60: # less than 60 hours, or ~2.5 days:
			return scaling, "hours"
		
		scaling /= 24.0
		if totalMinutes * scaling < 90: # less than 90 days:
			return scaling, "days"

		scaling /= 30.5
		return scaling, "months"

	def prepare(self):
		self.counts_to_sv = self.cpm_to_svh / 60.0

	def getIntensityForSample(self, index):
		return (self.samples[index] / (self.sampleLen / 60.0)) * (self.cpm_to_svh * 10**6)

	def getTotalTime(self):
		return len(self.samples) * self.sampleLen

	def getSampleLength(self):
		return self.sampleLen
	
	def getTotalDose(self):
		return sum(self.samples) * self.counts_to_sv
	
	def getAverageRate(self):
		return self.getTotalDose() * 3600 / self.getTotalTime()

	def getStartStr(self):
		if self.start:
			return self.start.isoformat(" ")
		else:
			return "null"


"""Returns a RadiationLog instance, out of reading a .geigerlog file"""
def readFileData(fn):
	result = RadiationLog(fn)
	status = result.parseGeigerLogData()
	if status == "":
		return result
	else:
		print "Cannot read .geigerlog file: %s" % status
		sys.exit(1)

def exportDataToGNUPlot(data, plot, useMinutes):
	plot.write("#\t%s\tRadiation\n" % (useMinutes and data.timeunit or "%"))
	
	for i in xrange(len(data.samples)):
		R = data.getIntensityForSample(i)
		if useMinutes:
			s = i * data.getSampleLength() * data.timemult
		else:
			s = 100.0 * i / float(len(data.samples) - 1)
		plot.write("\t%.4f %.3f\n" % (s, R))
	plot.close()


def intensity(c):
	return c[0] * 0.299 + c[1] * 0.587 + c[2] * 0.114

def scale(c, m):
	return (c[0] * m, c[1] * m, c[2] * m)

def difference(a, b):
	return intensity((abs(a[0] - b[0]), abs(a[1] - b[1]), abs(a[2] - b[2])))

def interpolate(a, p, b):
	return ((1-p) * a[0] + p * b[0], (1-p) * a[1] + p * b[1], (1-p) * a[2] + p * b[2]) 

def tohex(c):
	return "".join(map(lambda x: "%02x" % min(255, max(0, int(math.floor(x * 255.0 + 0.5)))), c))

"""Generates N visually different colors"""
def generateColors(N):
	colors = [(1.0, 0.0, 0.0), (0.0, 0.0, 1.0), (0.0, 0.5, 0.0)]
	if N <= 3:
		colors = colors[:N]
		return colors
	random.seed(0)
	TARGET_INTENSITY = 0.2
	Iw = intensity((1.0, 1.0, 1.0))
	while len(colors) < N:
		bestColor = None
		maxDiff = 0.0
		for tries in xrange(1000):
			c = (random.random(), random.random(), random.random())
			I = intensity(c)
			if I > TARGET_INTENSITY:
				c = scale(c, TARGET_INTENSITY/I)
			else:
				c = scale(c, min(1.0/max(c), TARGET_INTENSITY/I))
				I = intensity(c)
				if I < TARGET_INTENSITY + 1e-6:
					c = interpolate(c, (TARGET_INTENSITY-I) / (Iw-I), (1.0, 1.0, 1.0))
			minDiff = 999
			for i in xrange(len(colors)):
				minDiff = min(minDiff, difference(colors[i], c))
			if minDiff > maxDiff:
				maxDiff = minDiff
				bestColor = c
		colors.append(bestColor)
	return colors

def formatRadiation(rate, unit):
	if rate < 1e-3:
		return (rate * 1e6, "µ" + unit)
	elif rate < 1:
		return (rate * 1e3, "m" + unit)
	else:
		return (rate, unit)

def formatInterval(seconds):
	minutes = int(seconds / 60)
	if seconds < 3600:
		return "%d minutes" % minutes
	hours = minutes / 60
	if hours < 24:
		return "%d:%02d hours" % (hours, minutes % 60)
	days = hours / 24
	if days < 30:
		return "%d days %d hours" % (days, hours % 24)
	if days < 365:
		return "%d months %d hours" % (days / 30, days % 30)
	years = days / 365
	return "%d years %d months" % (years, days % 365 / 30)

def parseRadRange(arg):
	return map(float, arg.split('-'))

def makeTempFile(tempDir, fileName):
	fn = os.path.join(tempDir, fileName)
	return open(fn, "wt")

def escapeFileNameForGNUplot(fileName):
	return fileName.replace("\\", "\\\\")

def main(args):
	for kw in ['-h', '--help']:
		if kw in args:
			print USAGE
			return -1
	files = [] # array of fileinfos
	
	def last():
		return files[len(files) - 1]
	i = 1
	outFile = None
	printDischargeValues = False
	multiScaledX = False
	drawPoints = True
	radRangeMin = None
	radRangeMax = None
	calcRangeMin = None
	calcRangeMax = None
	keepFiles = None
	showGrid = False
	while i < len(args):
		arg = args[i]
		i += 1
		if arg[0] == "-":
			arg = arg[1:]
			if arg == "o":
				outFile = args[i]
				i += 1
			elif arg == "range":
				try:
					(radRangeMin, radRangeMax) = parseRadRange(args[i])
					i += 1
				except ValueError, err:
					print err
					return -1
			elif arg == "grid":
				showGrid = True
			elif arg == "keepFiles":
				keepFiles = True
			else:
				print USAGE
				return -1
		else:
			try:
				files.append(readFileData(arg))
			except DumpDataError, err:
				print err
				return -1
	if not files:
		print USAGE
		return -1
	for f in files:
		f.prepare()
	singleFile = (len(files) == 1)
	outDataFN = []
	tempDir = tempfile.mkdtemp(prefix="decode.log.", suffix="")
	for fidx in xrange(len(files)):
		f = files[fidx]
		if not singleFile:
			prefix = "%s:" % f.filename
		else:
			prefix = ""
		if not singleFile and not f is files[0]:
			print ""
		print "%sResolution  : 2**%d (%.1f minutes per sample)" % (prefix, f.resolution, f.sampleLen / 60.0)
		print "%sScaling     : 2**%d" % (prefix, f.scaling)
		if f.timingBy != "none":
			print "%sLog start   : %s (timed by its %s)" % (prefix, f.start.isoformat(" "), f.timingBy)
		print "%sLog length  : %s" % (prefix, formatInterval(f.getTotalTime()))
		print "%sTotal dose  : %.2f %s" % ((prefix,) + formatRadiation(f.getTotalDose(), "Sv"))
		print "%sAverage rate: %.3f %s" % ((prefix,) + formatRadiation(f.getAverageRate(), "Sv/h"))
		print "%s# of samples: %d" % (prefix, len(f.samples))
		print "%sTotal counts: %d" % (prefix, sum(f.samples))
		outDataFN.append(makeTempFile(tempDir, "data.%d.txt" % fidx))
	for i in xrange(len(files)):
		exportDataToGNUPlot(files[i], outDataFN[i], (len(files) == 1) or not multiScaledX)
	cmds = makeTempFile(tempDir, "main.txt")
	if outFile:
		cmds.write('set terminal png truecolor font "Ubuntu" linewidth 1.5')
		cmds.write("\n")
		cmds.write('set output "%s"\n' % outFile)
		assert outFile[-4:] == ".png", "Only .PNG files are supported"
	if showGrid:
		cmds.write("set grid back\n")

	# make gnuplot file:
	outfn = escapeFileNameForGNUplot(outDataFN[0].name)
	
	if singleFile:
		cmds.write("""
			set xlabel "Log time (%s)"
			set ylabel "Radiation level (µSv/h)"
		""" % files[0].timeunit)
		f = files[0]
		if f.title:
			cmds.write('set title "%s"\n' % f.title)
		if radRangeMax:
			cmds.write("set yrange [%.4f:%.4f]\n" % (radRangeMin, radRangeMax))
		cursor_y = 0.93
		if f.timingBy != "none":
			cmds.write('set label "Log start: %s" at graph 0.95, graph %.2f right\n' % (f.getStartStr(), cursor_y))
			cursor_y -= 0.05
		cmds.write('set label "Log length: %s" at graph 0.95, graph %.2f right\n' % (formatInterval(f.getTotalTime()), cursor_y))
		cursor_y -= 0.05
		cmds.write('set label "Average rate: %.2f %s" at graph 0.95, graph %.2f right\n' % (formatRadiation(f.getAverageRate(), "Sv/h") + (cursor_y,)))
		cursor_y -= 0.05
		cmds.write('set label "Total dose: %.1f %s" at graph 0.95, graph %.2f right\n' % (formatRadiation(f.getTotalDose(), "Sv") + (cursor_y,)))
		cmds.write("\n\n")
		plots = 'plot "%s" using 1:2 notitle with points lt rgb "#ccccdd", "%s" using 1:2 notitle smooth bezier with lines lt rgb "red"' % (outfn, outfn)
		cmds.write(plots + "\n")
	else:
		###
		print "WARNING: multi-file support is not quite finished at the moment."
		print "         Especially if log files started at different times!"
		###
		cmds.write("set xlabel \"Log time (%s)\"\nset ylabel \"Radiation level (µSv/h)\"\n" % (files[0].timeunit))
		if outFile:
			cmds.write("set multiplot\nunset key\n")
		if radRangeMax:
			cmds.write("set yrange [%.4f:%.4f]\n" % (radRangeMin, radRangeMax))
		cmds.write("plot ")
		COLORS = generateColors(len(files))
		for i in xrange(len(files)):
			fn = escapeFileNameForGNUplot(outDataFN[i].name)
			if i: cmds.write(", \\\n")
			if drawPoints:
				cmds.write("\"%s\" using 1:2 notitle with points lt rgb \"#%s\"," %
					(fn, tohex(interpolate(COLORS[i], 0.8, (1.0, 1.0, 1.0)))))
			cmds.write(" \"%s\" using 1:2" % fn)
			if outFile:
				cmds.write(" notitle")
			else:
				cmds.write(" title \"%s\"" % files[i].title)
			cmds.write(" smooth bezier with lines lt rgb \"#%s\" " % tohex(COLORS[i]))
		if outFile:
			cmds.write("\nset key\nunset border\nunset tics\nunset xlabel\nunset ylabel\nplot ")
			for i in xrange(len(files)):
				if i: cmds.write(", \\\n")
				else: cmds.write("[][0:1]")
				cmds.write("2 title '%s' with lines lt rgb \"#%s\" lw 3" % (files[i].title, tohex(COLORS[i])))
		cmds.write("\n")
	cmds.close()
	os.system("gnuplot %s %s" % (outFile and "" or "-persist", cmds.name))
	time.sleep(0.5)
	if not keepFiles:
		os.unlink(cmds.name)
		for fn in outDataFN:
			os.unlink(outDataFN[i].name)
		os.rmdir(tempDir)
	


if __name__ == "__main__":
	SCRIPTDIR = os.path.realpath(__file__)
	SCRIPTDIR = SCRIPTDIR[:SCRIPTDIR.rfind('/')]
	main(sys.argv)
