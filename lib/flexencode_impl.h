/* Written by Brandon Creighton <cstone@pobox.com>.
 *
 * This code is in the public domain; however, note that most of its
 * dependent code, including GNU Radio, is not.
 */

#ifndef INCLUDED_MIXALOT_FLEXENCODE_IMPL_H
#define INCLUDED_MIXALOT_FLEXENCODE_IMPL_H

#include <gnuradio/mixalot/flexencode.h>
#include <queue>
#include <vector>
#include <itpp/comm/bch.h>

using namespace itpp;
using std::string;
using std::vector;
using std::shared_ptr;


namespace gr {
  namespace mixalot {

    class flexencode_impl : public flexencode
    {
    private:
        std::queue<bool> d_bitqueue;   // Queue of symbols to be sent out.
        std::vector<string> d_cmdlist;     // List of command IDs to ack
        unsigned int d_baudrate;            // baud rate to transmit at
        unsigned long d_symrate;            // output symbol rate (must be evenly divisible by the baud rate)

        inline void queuebit(bool bit);

    public:
      flexencode_impl();
      ~flexencode_impl();

        void clear_cmdid_queue();
        void add_command_id(std::string cmdid);
        bool queue_pocsag_batch(msgtype_t msgtype, unsigned int baudrate, unsigned int capcode, std::string message);
        bool queue_flex_batch(const msgtype_t msgtype, const vector<uint32_t> &codes, const char *msgbody);
        boost::mutex bitqueue_mutex;
        boost::mutex cmdlist_mutex;

        void tune_target(double freqhz);
        bool make_standard_numeric_msg(unsigned int nwords, unsigned int message_start, const string msg, vector<uint32_t> &vecwords, vector<uint32_t> &msgwords, uint32_t &checksum);
        bool make_alphanumeric_msg(unsigned int num_address_words, unsigned int message_start, const string msg, vector<uint32_t> &vecwords, vector<uint32_t> &msgwords);
        void beeps_message(pmt::pmt_t msg);
		void beeps_output(string const &msgtext);

        void queue_pocsag(shared_ptr<bvec> bvptr);
        void queue_pocsag(uint32_t val);
        void queue(shared_ptr<bvec> bvptr);
        void queue(uint8_t *arr, size_t sz);
        void queue(uint32_t val);
        int work(int noutput_items,
           gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
    };


  } // namespace mixalot
} // namespace gr

#endif /* INCLUDED_MIXALOT_FLEXENCODE_IMPL_H */

