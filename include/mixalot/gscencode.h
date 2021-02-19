/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifndef INCLUDED_MIXALOT_GSCENCODE_H
#define INCLUDED_MIXALOT_GSCENCODE_H

#include <mixalot/api.h>
#ifdef GR_OLD
#include <gr_sync_block.h>
#else
#include <gnuradio/sync_block.h>
#endif

namespace gr {
  namespace mixalot {


#ifdef GR_OLD
    class MIXALOT_API gscencode : virtual public gr_sync_block
#else 
    class MIXALOT_API gscencode : virtual public sync_block
#endif
    {
    public:
       typedef boost::shared_ptr<gscencode> sptr;
       typedef enum { Numeric = 0, Alpha = 1 } msgtype_t;

       static sptr make(msgtype_t type=Numeric, unsigned int capcode = 0, std::string message="", unsigned long symrate = 38400);
    };

  } // namespace mixalot
} // namespace gr

#endif /* INCLUDED_MIXALOT_GSCENCODE_H */

