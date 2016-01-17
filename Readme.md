# LVA Geiger Counter v1.5/2.0 source code

![Image of Geiger Counter v2.0](http://lva.bg/products/geiger-counter/gallery/Geiger_main-smaller.jpg)

This is my fork of MightyOhm's Geiger Counter. It features:

* **v1.0** (this is the original counter kit from MightyOhm):
	* An Atmel-based geiger counter, supporting various GM tubes, like SI-3BG, SI-1G, SBM-20, STS-5, and others, with a tunable 300-600V tube bias voltage supply on-board;
	* LED and piezo notification on each detected particle;
	* Serial interface so you can connect it to Arduino, Raspberry Pi and the like, for data gathering and internet hookup. The serial data is easily parsable, in the form of a 1-line per second log, and includes both counts per minute (CPM), as well as equivalent dose (in µSv/h).
	* Pulse header: outputs a 100 µs active-high pulse on each detected particle;
	* ICSP header
	* Powered by standard 2×AAA batteries; lasts more than a week on a single pair of alkalines.

* **v1.5**
	* A 7-segment screen, for easy radiation readout. The device can now be used as a generic portable dosimeter;
	* "Pure GM Counter Mode": the device displays the total particles detected since startup;
	* 3-level display brightness setting (and can be turned off if you need to save power).

* **v2.0**
	* MCU changed to ATmega88, much more code space to play with (ATtiny2313 was *really*
	  tight on program memory);
	* Much brighter display, directly controlled by the MCU;
	* uSv/h <-> GM counts mode is freely toggled with the button;
	* 9-level display brightness setting;
	* Battery level monitoring, including an alarm tone if the batteries are nearly flat.

# Hardware

The device is sold through our friends at Robotev, [see here](http://www.robotev.com/product_info.php?cPath=1_50&products_id=578) if you want one (currently shipping to Bulgaria and Balkan-area countries only). More photos of the device can be found [here](http://lva.bg/products/geiger-counter/gallery).
