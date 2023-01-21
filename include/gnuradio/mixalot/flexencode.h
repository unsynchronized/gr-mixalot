/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifndef INCLUDED_MIXALOT_FLEXENCODE_H
#define INCLUDED_MIXALOT_FLEXENCODE_H

#include <gnuradio/mixalot/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace mixalot {


    class MIXALOT_API flexencode : virtual public sync_block
    {
    public:
       typedef std::shared_ptr<flexencode> sptr;
       typedef enum { Numeric = 0, Alpha = 1 } msgtype_t;

       static sptr make();
    };

  } // namespace mixalot
} // namespace gr

#endif /* INCLUDED_MIXALOT_FLEXENCODE_H */

