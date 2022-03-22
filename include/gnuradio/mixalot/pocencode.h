/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifndef INCLUDED_MIXALOT_POCENCODE_H
#define INCLUDED_MIXALOT_POCENCODE_H

#include <gnuradio/mixalot/api.h>
#include <gnuradio/sync_block.h>

namespace gr {
  namespace mixalot {

    /*!
     * \brief <+description of block+>
     * \ingroup mixalot
     *
     */
    class MIXALOT_API pocencode : virtual public gr::sync_block
    {
    public:
       typedef std::shared_ptr<pocencode> sptr;
       typedef enum { Numeric = 0, Alpha = 1 } msgtype_t;

      /*!
       * \brief Return a shared_ptr to a new instance of mixalot::pocencode.
       *
       * To avoid accidental use of raw pointers, mixalot::pocencode's
       * constructor is in a private implementation
       * class. mixalot::pocencode::make is the public interface for
       * creating new instances.
       */
      static sptr make(int type=0, unsigned int baudrate = 1200, unsigned int capcode = 0, std::string message="", unsigned long symrate = 38400);
    };

  } // namespace mixalot
} // namespace gr

#endif /* INCLUDED_MIXALOT_POCENCODE_H */

