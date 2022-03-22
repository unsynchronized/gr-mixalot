#!/usr/bin/env python3

from gnuradio import gr, gr_unittest, pager, gru
import mixalot_python as mixalot
import time, os, sys

class tester_top(gr.top_block):
    def __init__(self, options):
        gr.top_block.__init__(self, "tester")
        self.options = options

def get_options():
    parser = OptionParser(option_class=eng_option)
    parser.add_option("-v", "--verbose", action="store_true", default=False)
    (options, args) = parser.parse_args()
    if len(args) > 0:
        print("Run 'usrp_flex.py -h' for options.")
        sys.exit(1)
    return (options, args)


if __name__ == "__main__":
    (options, args) = get_options()
    tb = tester_top(options)
    try:
        tb.run()
    except KeyboardInterrupt:
        pass
