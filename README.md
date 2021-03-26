gr-mixalot
==========

gr-mixalot is intended to be a set of GNU Radio blocks/utilities to encode 
pager messages.  It supports the POCSAG, FLEX, and Golay/GSC protocols.

Here is a breakdown of the blocks:

* pocencode / "Single-Page POCSAG Xmit": Given parameters (capcode, baud rate, 
  message, message type), it generates a stream of symbols that can be modulated
  as seen in examples/pocsagtx.grc.
* gscencode / "Single-Page GSC Xmit": Like the above, but for GSC pagers.  There
  is an example in examples/gsctx.grc.
* flexencode / "PDU-driven POCSAG/Flex Encoder": Unlike the above, this does not 
  send just one page -- it runs continuously, watching for PDUs on input to specify
  pages, and then modulates them.  The example flowgraph (examples/pagerserver.grc)
  uses a "Socket PDU" source to run as a TCP-based server.  See below for the commands.


PDU Commands and Responses
==========================

Commands sent to the PDU-driven encoder use the following form:

```
<protocol> <messagetag> <frequency_hz> <alpha|numeric> <capcode> <hexl-encoded message>
```

'protocol' is one of:
* flex
* pocsag512
* pocsag1200
* pocsag2400

'messagetag' is an arbitrary string (no spaces allowed) that will be sent back in responses.

'frequency_hz' is the frequency in Hz.  Note: this requires an SDR that can be tuned live;
this works for the USRP sinks as shown in examples/pagerserver.grc, but will probably not
work elsewhere without some extra work.

'alpha' generates an alphanumeric message; 'numeric' generates a numeric message.

'capcode': the numeric (decimal) capcode of the pager.

The message is hexl-encoded (even for numeric messages); so the message HELLO would be 
`48454C4C4F`.

Responses from the server will be of the form `'<messagetag> OK'`.

Examples (with responses):


Send a numeric message '0123' to a 1200-baud POCSAG pager with capcode 1615132 on 931.8625 MHz:
```
pocsag1200 0 931862500 numeric 1615132 30313233
0 OK
```

Send the alphanumeric message 'HACK THE PLANET' to a FLEX pager with capcode 1337000 on
931.3375MHz:

```
flex doit 931337500 alpha 1337000 4841434b2054484520504c414e4554
doit OK
```


Compatibility
=============

The POCSAG encoder has been tested against many variants of the Motorola Bravo 
and Advisor series, as well as the ATI Wireless CCP-6000.  

The FLEX encoder has been tested against a wide variety of Motorola pagers and
a small numer of NEC pagers.

The Golay encoder is the least tested.

Please get in touch if you find a device that doesn't work properly.  


Building
========

Like many other out-of-tree modules, gr-mixalot uses cmake.  This branch is targeted 
toward GNU Radio 3.8.x+ -- previous branches should run on 3.6-3.7.  The IT++/ITPP
library is also a dependency.  To use the HackRF sink, you'll also need gr-osmosdr 
installed.

To build, create a new directory and run:

    % cmake PATH-TO-GR-MIXALOT-ROOT
    % make

To install (may require root), run from the same directory:
    % make install

On UNIX systems, ensure that the install destination lib folder is added to
your LD\_LIBRARY\_PATH, or add it to your /etc/ld.so.conf. A default install
will place library files in /usr/local/lib, which is not searched by default
on many Linux distributions.

Examples:

    % export LD_LIBRARY_PATH /usr/local/lib
    or
    % echo /usr/local/lib | sudo tee -a /etc/ld.so.conf

WARNING
=======

Pagers are still in production use today.  Although it's possible to acquire 
working pagers from a variety of sources (often for little or no cost), they
may be tuned to an active channel.  Transmitting over an existing station
could cause legitimate users to silently lose messages.  It is your 
responsibility to ensure that this doesn't happen.  


Usage
=====

To send a page to a device, you'll need to know:

* the encoding scheme in use (FLEX, POCSAG, or GSC)
* in the case of POCSAG, its baud rate
* the frequency it's tuned to (often printed on the device)
* its cap code (often printed on the device)
* whether it wants alphanumeric or numeric pages

In the case of Motorola devices, some of these parameters can be discovered from 
decoding the model number.  See [pagercodes.md](pagercodes.md) to decode yours.

To use the single-shot flowgraphs:

After installing, start gnuradio-companion and open the example flowgraph.  These are 
in examples/ -- for POCSAG, use pocsagtx.grc; for GSC, use gsctx.grc.

Double-click the "Single-Page <type> Xmit" block to modify the baud rate, 
capcode, and message.  After saving, double-click the "pagerfreq" Variable block
to change the tunning frequency.  (This should be specified in Hz).  Change the
samp_rate and sink parameters as required by your setup.  (If you're not using a USRP,
you'll want to swap out that sink for one appropriate for your SDR.) Build and run the 
flow graph; it should send the page, then exit.  


Basic technical walkthrough (POCSAG)
====================================

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


### POCSAG

POCSAG is an extremely simple protocol; the spec is just a few pages long.
You can grab it from the ITU here:

http://www.itu.int/dms_pubrec/itu-r/rec/m/R-REC-M.584-2-199711-I!!PDF-E.pdf

Several folks have also written useful pages that clarify or explain certain
points of the spec, including:

* http://hem.passagen.se/communication/pocsag.html
* http://www.braddye.com/pocsag.html and http://www.braddye.com/paging.html
* http://www.hackcanada.com/blackcrawl/cell/pager/pager.html

### FLEX

The full FLEX specification is included in the ARIB standard (RCR STD-43A), as Reference Document A; this is the best place to look.

This can be found at http://www.arib.or.jp/english/html/overview/doc/1-STD-43_A-E1.pdf.

Other folks have implemented FLEX to some extent as well:

* https://github.com/chris007de/flex

### GSC

The GSC (Golay Sequential Code) protocol is defined in the ITU Report 900-2, starting on Page 9: https://www.itu.int/dms_pub/itu-r/opb/rep/R-REP-M.900-2-1990-PDF-E.pdf.

There is a lot of information about GSC at Brad Dye's excellent paging information site: [http://www.braddye.com/](http://www.braddye.com/), including this Motorola document: [http://braddye.com/golay.pdf](http://braddye.com/golay.pdf)


Current todo list/Coming soon
=============================

* Add GSC to the PDU-driven encoder
* Eliminate dependency on the ITPP library (currently only used for BCH) 
* Eliminate dependency on third-party code to do Golay coding


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

