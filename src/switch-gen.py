#!/usr/bin/python

"""A hash-function generator, which generates code to handle a specific set
   of ASCII commands in an optimal (for the AVR 8-bit MCUs) way."""

import random
	
INPUT = """
	BLVW (bit) - Battery low-voltage warning
	UASU (bit) - UART active on startup
	GETTM (void) - Get tube multiplier
	STMN (int) - Set tube multiplier numerator
	STMD (int) - Set tube multiplier denominator
	HELO (void) - Print hello message
	STATUS (void) - Print device status
	RESET (void) - Resets the device
	SILENT (void) - Disable UART reports
	NOISY (void) - Enable UART reports
	RSLOG (void) - Read SRAM log
	REELOG (void) - Read EEPROM log
	GETID (void) - Get device id
	SID (int) - Set device id
	GETRA (void) - Get radiation alarm limit
	STRA (int) - Set radiation alarm limit
	GETDA (void) - Get dose alarm limit
	STDA (int) - Set dose alarm limit
	CLOG (void) - Clear all logs
	STPP (int) - Set programming pointer
	RDPP (void) - Read program data from the programming pointer and increment it
	WRPP (int) - Write byte data at the programming pointer and increment it
"""

print """
/**
 * @retval -1 - command has no arguments
 * @retval  0 - command has argument '0'
 * @retval +1 - command has argument '1'
 * @retval +2 - command has incorrectly formatted argument
 */
static int8_t has_bool_arg(const char* cmd)
{
	if (*cmd != ' ') return -1;
	cmd++;
	if (*cmd == '0') return 0;
	if (*cmd == '1') return 1;
	return 2;
}

/**
 * @retval 1 - everything is correct
 * @retval 2 - invalid argument (bad format, out of range, etc.)
 * @retval 3 - command has no arguments, but it should
 */
static int8_t has_arg(const char* cmd, uint16_t* arg)
{
	if (*cmd != ' ') return 3;
	*arg = 0;
	// parse the argument as a number, with overflow detection:
	for (cmd++; *cmd; cmd++) {
		uint8_t t = *cmd - '0';
		if (t > 9 || *arg > 6553) return 2;
		if (*arg == 6553 && t > 5) return 2;
		*arg = *arg * 10 + t;
	}
	return 1;
}"""

handlers = {
	"int": (lambda cmd: """
			if ((ok = has_arg(cmd + %d, &arg)) != 1) return ok;
			//
			//
			return 1;""" % len(cmd)),
	"bit": (lambda cmd: """
			return handle_bool_cmd(BIT_%s, has_bool_arg(cmd + %d));""" % (cmd, len(cmd))),
	"void": (lambda cmd: """
			//
			return 1;"""),
}

class HashObject(object):
	def __init__(self, bits, mul = -1):
		self.bits = bits
		self.mod = 2**bits
		self.mul = mul
		if mul == -1:
			self.mul = random.randint(0, self.mod - 1)

	def allDifferent(self, cmds):
		seen = set()
		for cmd in cmds:
			h = self.hash(cmd)
			if h in seen: return False
			seen.add(h)
		return True

	def hash(self, s):
		acc = 0
		for c in s:
			val = ord(c)
			acc = (acc * self.mul + val) % self.mod
		return acc

	def emitBody(self, suffix=""):
		T = "uint%d_t" % self.bits
		print """
static %s hash%s(const char* s)
{
	%s x = 0;
	for (; *s && *s != ' '; ++s) {
		x = x * %d + *s;
	}
	return x;
}""" % (T, suffix, T, self.mul)


items = INPUT.split('\n')[1:-1]
cmds = []
descriptions = {}

for item in items:
	cmd_type, description = item.strip().split(' - ')
	cmd = cmd_type[:cmd_type.find('(') - 1]
	T = cmd_type[cmd_type.find('(') + 1 : cmd_type.find(')')]
	descriptions[cmd] = (T, description)
	cmds.append(cmd)

cmds.sort()

while True:
	h = HashObject(16, 37491)
	if h.allDifferent(cmds):
		break
h.emitBody()
print """

...
	int8_t ok;
	uint16_t arg;
	switch (hash(cmd)) {"""
for c in cmds:
	T, description = descriptions[c]
	print """
		/* %s - %s */
		case 0x%04X:
		{%s
		}""" % (c, description, h.hash(c), handlers[T](c))
print """
		default:
			return 0;
	}"""
