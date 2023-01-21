/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifndef INCLUDED_MIXALOT_GSCENCODE_H
#define INCLUDED_MIXALOT_GSCENCODE_H

#include <gnuradio/mixalot/api.h>
#ifdef GR_OLD
#include <gr_sync_block.h>
#else
#include <gnuradio/sync_block.h>
#endif

namespace gr {
  namespace mixalot {
    class MIXALOT_API gscencode : virtual public sync_block
    {
    public:
       typedef std::shared_ptr<gscencode> sptr;
       typedef enum { Numeric = 0, Alpha = 1 } msgtype_t;

       static sptr make(int type=0, unsigned int capcode = 0, std::string message="", unsigned long symrate = 38400);
    };

  } // namespace mixalot
} // namespace gr

#endif /* INCLUDED_MIXALOT_GSCENCODE_H */

