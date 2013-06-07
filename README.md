gr-mixalot
==========

gr-mixalot is intended to be a set of GNU Radio blocks/utilities to encode 
pager messages.  Currently it only supports POCSAG numeric and alphanumeric 
pages.  It's also intended to be a fairly clear, short example of basic 
transmission with GNU Radio.   

The POCSAG encoder has been tested against many variants of the Motorola Bravo 
series, as well as the ATI Wireless CCP-6000.  Please get in touch if you
find a device that doesn't work properly.  


Building
========

Like many other out-of-tree modules, gr-mixalot uses cmake.  The only 
dependencies should be a recent version of GNU Radio (3.6.x) and the ITPP
library.

To build, create a new directory and run:

    % cmake PATH-TO-GR-MIXALOT-ROOT
    % make

To install (may require root), run from the same directory:
    % make install


WARNING
=======

POCSAG devices are still in use today.  Although it's possible to acquire 
working pagers from a variety of sources (often for little or no cost), they
may be tuned to an active channel.  Transmitting over an existing station
could cause legitimate users to silently lose messages.  It is your 
responsibility to ensure that this doesn't happen.  


Usage
=====

To send a page to a device, you'll need to know:
    * the frequency it's tuned to (often printed on the device)
    * its cap code (often printed on the device)
    * its baud rate (POCSAG is specified for 512, 1200, and 2400 baud)
    * whether it wants alphanumeric or numeric pages

After installing, start gnuradio-companion and open the pocsagtx.grc flowgraph.
Double-click the "Single-Page POCSAG Xmit" block to modify the baud rate, 
capcode, and message.  After saving, double-click the "pagerfreq" Variable block
to change the tunning frequency.  (This should be specified in Hz).  Change the
samp_rate and sink parameters as required by your setup.  Build and run the 
flow graph; it should send the page, then exit.  


Basic technical walkthrough
===========================

POCSAG is plain FSK with a deviation of 4500 Hz around the center frequency.  

The pocencode block ("Single-Page POCSAG Xmit" in grc) encodes the message as
a POCSAG bitstream.  Then, it converts the bitstream into a stream of samples,
running at a rate that is an integer multiple of the POCSAG baud rate.  (In 
my example flowgraph, I've used 38400, which is divided by 512, 1200, and 2400 
evenly.  This seems to work fine.)

Next, the GNU Radio FM block is used in the flowgraph to modulate samples at 
the intermediate sample rate.  Note that the output of pocencode is not a 
stream of (0, 1) bits, but (1, -1) samples.  This is for the benefit of the math
inside the FM block.  

Finally, the stream is interpolated to the sample rate desired by the sink.  
(In my example, there is also a const multiplication to avoid clipping; this
may not be necessary for your sink, so feel free to remove it.)

Although this works for me, I wrote this in order to teach myself the basics of
GNU Radio and working with signal processing through it; for that reason, 
the code is not as efficient as it could be.  Holler if you see anything 
seriously wrong or broken, or if you have any suggestions!


Protocol documentation and other references
===========================================

POCSAG is an extremely simple protocol; the spec is just a few pages long.
You can grab it from the ITU here:

http://www.itu.int/dms_pubrec/itu-r/rec/m/R-REC-M.584-2-199711-I!!PDF-E.pdf

Several folks have also written useful pages that clarify or explain certain
points of the spec, including:

* http://hem.passagen.se/communication/pocsag.html
* http://www.braddye.com/pocsag.html and http://www.braddye.com/paging.html
* http://www.hackcanada.com/blackcrawl/cell/pager/pager.html


Current todo list/Coming soon
=============================

* Working example flowgraph with a HackRF as the target.
* Eliminate dependency on the ITPP library (currently only used for BCH) 
* Golay/GSC encoding 
* Flex support


Credits
=======

gr-mixalot was written by Brandon Creighton <cstone@pobox.com>.  The name is 
an homage to Sir Mix-a-Lot, who wrote the inspirational track "Beepers".   

Suggestions and flames are equally welcome; as are additional pager-protocol
documents.

Thanks to the following for testing and general awesomeness, in alphabetical 
order: 
* arko
* banasidhe
* docwho
* dragorn
* jeffE
* the Ninjatel crew; barkode, cnelson, eliot, falconred, far_call, kubla
* Mike Ossmann
* pinguino
* vyrus

