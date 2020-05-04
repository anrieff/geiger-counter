This collection of scripts serves to manage geiger counters connected to a PC serial port (typically via a FTDI UART converter).

# Prerequisites

* Python 2.7
* [pyserial](https://pypi.org/project/pyserial/)
* [gnuplot](http://www.gnuplot.info/)

# Example workflow:

1. Start your geiger counter so that it collects data. You may turn off the display to save power.
2. Record when you started it.
3. You can turn the device off when you want to stop gathering data (optional - you can also download data as-you-go).
4. Connect the geiger counter to the PC using the UART converter (on Windows you may need to know which port number it got assigned to).
5. Run the `download_log` script, e.g.: `./download_log.py -start 2020-04-10-16:35`
6. You'll get a text file with the data downloaded.
7. You may visualize this file with the decode_log script, e.g.: `./decode_log.py data_2020-04-10-[169]-16_35.geigerlog`

## download_log.py

Downloads a log of radiation data as stored in the device (using the REELOG command).

Typical usage:

  ./download_log.py -start 2020-04-10-16:35

This invocation assumes that the log gathering was started at 10 Apr 2020, at 16:35. When it ended is automatically
deduced from the length of the data in the device.

You can also download the data from a running device (if you skipped step 3 in the list above):

  ./download_log.py -end now

Run `./download_log.py --help` for the full list of command-line switches supported.

## decode_log.py

Displays a log file, as collected via `download_log`.

Typical usage:

  ./decode_log.py data_2020-04-10-[169]-16_35.geigerlog


This uses gnuplot to display an image like this one:

  <img src="http://anrieff.net/abs/i/2020-04-10..2020-04-22.png" alt="Geiger log" />

Run `./decode_log.py --help` for the full list of command-line switches supported.

# Issues and caveats

* This is only well-tested on Linux, but it reportedly works on Windows. You need to specify the COM port using the -p switch for `download_log`.
* The scripts assume you are knowledgeable about when you started logging data. It cannot verify your inputs - the devices don't have a RTC.
