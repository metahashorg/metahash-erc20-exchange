
#include <stdio.h>
#include <vector>
#include <string>
#include <iostream>
#include <thread>
#include <mutex>
#include <boost/shared_ptr.hpp>
#include <unordered_set>
#include <boost/filesystem.hpp>

#include <ev.h>
#include <sniper/threads/spinlock.hpp>
#include <sniper/mhd/MHD.h>
#include <rapidjson/document.h>
#include <sniper/event/Event.h>
#include <gmp.h>
#include <gmpxx.h>

#include <boost/thread/thread.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>

using namespace rocksdb;
using namespace std;

namespace meta_erc_convert {

using std::vector;
using std::pair;
using std::set;
using std::map;
using std::string;

class Meta_ERC_Convert : public sniper::mhd::MHD {
public:
	Meta_ERC_Convert();
	virtual ~Meta_ERC_Convert();

    virtual bool run(int thread_number, Request& mhd_req, Response& mhd_resp);

protected:
    virtual bool init();

private:
    string pass_Gen(int passLenght);
    int make_tx(int type, const string & addr, mpz_class & val);
    struct LocalStore {
    	utils::LibEvent levent;
    	string debug_response;
        string req_id_str, method, cur, address, key;
        string error_mes;
        string code;
    };
    void update_thread();

    string db_path;
    //hosts
    string host_cr_eth;
    string host_cr_mhc;
  
    //ports
    int port_cr_eth;
    int port_cr_mhc;

    //paths
    int port;
    string path_cr_eth;
    string path_cr_mhc;

    string common_eth;
    string common_mhc;
    string address_erc_tkn;

    string group_eth;
    string group_mhc;
    string common_pass_eth;

    bool read_config();
    int make_tx_full(const string & key_it, mpz_class & value_it, int type);
    uint64_t get_id_req(string & out_val);

    vector<LocalStore*> local_store;
    utils::LibEvent levent_update;
    string last_block;

    std::thread* update_th = nullptr;

    unsigned char hexval(unsigned char c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    else if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    else if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    else abort();
}

void hex2ascii(const string& in, string& out)
{
    out.clear();
    out.reserve(in.length() / 2);
    for (string::const_iterator p = in.begin(); p != in.end(); p++)
    {
       unsigned char c = hexval(*p);
       p++;
       if (p == in.end()) break; // incomplete last digit - should report error
       c = (c << 4) + hexval(*p); // + takes precedence over <<
       out.push_back(c);
    }
}

    template<typename C>
void test_strpbrk(string const& s, char const* delims, C& ret) {
  C output;

  char const* p = s.c_str();
  char const* q = strpbrk(p, delims);
  for( ; q != NULL; q = strpbrk(p, delims) )
  {
    if(p!=q)
      output.push_back(typename C::value_type(p, q));
    p = q + 1;
  }
  if(p)
    output.push_back(p);
  output.swap(ret);
}
};


} 
