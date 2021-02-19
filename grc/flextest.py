#!/usr/bin/env python2
# -*- coding: utf-8 -*-
##################################################
# GNU Radio Python Flow Graph
# Title: Flextest
# Author: cstone@pobox.com
# Description: Example flowgraph for FLEX transmitter
# Generated: Thu May 30 00:20:11 2019
##################################################


from gnuradio import analog
from gnuradio import blocks
from gnuradio import eng_notation
from gnuradio import gr
from gnuradio import uhd
from gnuradio.eng_option import eng_option
from gnuradio.filter import firdes
from gnuradio.filter import pfb
from optparse import OptionParser
import math
import mixalot
import time


class flextest(gr.top_block):

    def __init__(self, capcode=425321, message='GRAND CENTRAL\\x0a\\x0aHACKRF THE PLANET'):
        gr.top_block.__init__(self, "Flextest")

        ##################################################
        # Parameters
        ##################################################
        self.capcode = capcode
        self.message = message

        ##################################################
        # Variables
        ##################################################
        self.symrate = symrate = 38400
        self.samp_rate = samp_rate = 1000000
        self.pagerfreq = pagerfreq = 931337500
        self.max_deviation = max_deviation = 4800.0

        ##################################################
        # Blocks
        ##################################################
        self.uhd_usrp_sink_0 = uhd.usrp_sink(
        	",".join(('', "")),
        	uhd.stream_args(
        		cpu_format="fc32",
        		channels=range(1),
        	),
        )
        self.uhd_usrp_sink_0.set_samp_rate(samp_rate)
        self.uhd_usrp_sink_0.set_center_freq(pagerfreq, 0)
        self.uhd_usrp_sink_0.set_gain(37, 0)
        self.uhd_usrp_sink_0.set_antenna('TX/RX', 0)
        self.pfb_arb_resampler_xxx_0 = pfb.arb_resampler_ccf(
        	  float(samp_rate)/float(symrate),
                  taps=None,
        	  flt_size=32)
        self.pfb_arb_resampler_xxx_0.declare_sample_delay(0)

        self.mixalot_flexencode_0 = mixalot.flexencode()
        self.blocks_socket_pdu_0 = blocks.socket_pdu("TCP_SERVER", '127.0.0.1', '31337', 10000, False)
        self.blocks_multiply_const_vxx_0 = blocks.multiply_const_vcc((0.7, ))
        self.blocks_file_sink_1 = blocks.file_sink(gr.sizeof_char*1, '/tmp/out.syms', False)
        self.blocks_file_sink_1.set_unbuffered(False)
        self.blocks_char_to_float_0 = blocks.char_to_float(1, 1)
        self.analog_frequency_modulator_fc_0 = analog.frequency_modulator_fc(2.0 * math.pi * max_deviation / float(symrate))



        ##################################################
        # Connections
        ##################################################
        self.msg_connect((self.blocks_socket_pdu_0, 'pdus'), (self.mixalot_flexencode_0, 'beeps'))
        self.msg_connect((self.mixalot_flexencode_0, 'beeps_output'), (self.blocks_socket_pdu_0, 'pdus'))
        self.msg_connect((self.mixalot_flexencode_0, 'cmds_out'), (self.uhd_usrp_sink_0, 'command'))
        self.connect((self.analog_frequency_modulator_fc_0, 0), (self.blocks_multiply_const_vxx_0, 0))
        self.connect((self.blocks_char_to_float_0, 0), (self.analog_frequency_modulator_fc_0, 0))
        self.connect((self.blocks_multiply_const_vxx_0, 0), (self.pfb_arb_resampler_xxx_0, 0))
        self.connect((self.mixalot_flexencode_0, 0), (self.blocks_char_to_float_0, 0))
        self.connect((self.mixalot_flexencode_0, 0), (self.blocks_file_sink_1, 0))
        self.connect((self.pfb_arb_resampler_xxx_0, 0), (self.uhd_usrp_sink_0, 0))

    def get_capcode(self):
        return self.capcode

    def set_capcode(self, capcode):
        self.capcode = capcode

    def get_message(self):
        return self.message

    def set_message(self, message):
        self.message = message

    def get_symrate(self):
        return self.symrate

    def set_symrate(self, symrate):
        self.symrate = symrate
        self.pfb_arb_resampler_xxx_0.set_rate(float(self.samp_rate)/float(self.symrate))
        self.analog_frequency_modulator_fc_0.set_sensitivity(2.0 * math.pi * self.max_deviation / float(self.symrate))

    def get_samp_rate(self):
        return self.samp_rate

    def set_samp_rate(self, samp_rate):
        self.samp_rate = samp_rate
        self.uhd_usrp_sink_0.set_samp_rate(self.samp_rate)
        self.pfb_arb_resampler_xxx_0.set_rate(float(self.samp_rate)/float(self.symrate))

    def get_pagerfreq(self):
        return self.pagerfreq

    def set_pagerfreq(self, pagerfreq):
        self.pagerfreq = pagerfreq
        self.uhd_usrp_sink_0.set_center_freq(self.pagerfreq, 0)

    def get_max_deviation(self):
        return self.max_deviation

    def set_max_deviation(self, max_deviation):
        self.max_deviation = max_deviation
        self.analog_frequency_modulator_fc_0.set_sensitivity(2.0 * math.pi * self.max_deviation / float(self.symrate))


def argument_parser():
    description = 'Example flowgraph for FLEX transmitter'
    parser = OptionParser(usage="%prog: [options]", option_class=eng_option, description=description)
    parser.add_option(
        "", "--capcode", dest="capcode", type="intx", default=425321,
        help="Set capcode [default=%default]")
    parser.add_option(
        "", "--message", dest="message", type="string", default='GRAND CENTRAL\\x0a\\x0aHACKRF THE PLANET',
        help="Set message [default=%default]")
    return parser


def main(top_block_cls=flextest, options=None):
    if options is None:
        options, _ = argument_parser().parse_args()

    tb = top_block_cls(capcode=options.capcode, message=options.message)
    tb.start()
    tb.wait()


if __name__ == '__main__':
    main()
