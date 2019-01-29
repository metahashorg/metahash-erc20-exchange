#include "meta_erc_convert.h"

#include <time.h>
#include <libconfig.h++>
#include <signal.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <algorithm>

#include <sniper/log/log.h>

using namespace rocksdb;
using namespace std;

namespace meta_erc_convert {

using boost::lexical_cast;
using boost::bad_lexical_cast;
using namespace rocksdb;

//paths to DB
std::string kDBPath_info ;
std::string kDBPath_mhc ;
std::string kDBPath_erc ;
std::string kDBPath_tx_mhc ;
std::string kDBPath_tx_eth ;
std::string kDBPath_tx_mhc_un ;
std::string kDBPath_tx_eth_un ;

//RocksDB
//db for binding eth->mhc
DB* db_erc;
//db for transactions
DB* db_mhc_tx;
DB* db_erc_tx;
//db for unloaded transactions
DB* db_mhc_tx_un;
DB* db_erc_tx_un;

//db info (id, limits, last_block)
DB* db_info;

Meta_ERC_Convert::Meta_ERC_Convert()
{
	// TODO Auto-generated constructor stub

}

Meta_ERC_Convert::~Meta_ERC_Convert() {
	
}

static void sig_handler(int sig)
{
	
    exit(1);
}

bool Meta_ERC_Convert::run(int thread_number, Request& mhd_req, Response& mhd_resp)
{
	LocalStore* store = local_store[thread_number];
	store->req_id_str.clear();
	store->method.clear();
	store->cur.clear();
	store->key.clear();
	store->address.clear();
	store->error_mes.clear();
	store->code.clear();

	if(!mhd_req.post.empty()){

		string val;
		rapidjson::Document req_post;
		if (!req_post.Parse(mhd_req.post.c_str()).HasParseError()) {
			
			if (req_post.HasMember("id") && req_post["id"].IsString() &&
				req_post.HasMember("method") && req_post["method"].IsString()){
				
				store->req_id_str=req_post["id"].GetString();
				store->method=req_post["method"].GetString();

			
			 if (store->method == "BindAddress" && req_post.HasMember("params") && req_post["params"].IsArray()  && req_post["params"].Size() > 0){
				mhd_resp.data = 
				"{"
					"\"id\": \""+store->req_id_str+"\","
					"\"method\": \""+store->method+"\","
			   		"\"result\": [";
			   	bool isf=true;
				for(uint64_t it_keys=0; it_keys<req_post["params"].Size(); it_keys++){
					if( req_post["params"][it_keys].IsObject()){
                    	//запись в бд связки
                    	if(req_post["params"][it_keys].HasMember("cur") && req_post["params"][it_keys].HasMember("address") && req_post["params"][it_keys].HasMember("external")){
                    		if(req_post["params"][it_keys]["cur"].IsString() && req_post["params"][it_keys]["address"].IsString() && req_post["params"][it_keys]["external"].IsString() ){
                    			string addr=req_post["params"][it_keys]["address"].GetString();
                    			string ext=req_post["params"][it_keys]["external"].GetString();
                    			if(isf)
                    				isf=false;
                    			else
                    				mhd_resp.data +=",";

                    			string cur(req_post["params"][it_keys]["cur"].GetString());
                    			mhd_resp.data += "{ \"cur\":\""+cur+"\","
                    				"\"address\":\""+addr+"\","
                    				"\"external\":\""+ext+"\","
                    				"\"result\":";

                    			if(cur=="ETH"){
                    				    boost::to_lower(addr);
                    					Status s=db_erc->Get(ReadOptions(), Slice(addr), &val);
                                    	if(!s.ok()){
                                            boost::to_lower(ext);
                                        	s=db_erc->Put(WriteOptions(), Slice(addr), Slice(ext));
                                        	if(!s.ok()){
                                                log_info("Bindind: error write in db {}", mhd_req.post);
                                            	mhd_resp.data += "\"error\"}";
                                        	}
                                       		else{
                                           		mhd_resp.data += "\"ok\"}";
                                            }
                                    	}
                                    	else{
                                            log_info("Bindind: error write ib db {}", mhd_req.post);
                                    		mhd_resp.data += "\"exist\"}";
                                        }
                    			}
                    			else
                    			{
                    				//unknown currency
                                    log_info("Binding: unknown currency {}", mhd_req.post);
                    				mhd_resp.data += "\"error\"}";
                    			}
                    		}
                    	}
                    	else
                    	{
                    		log_info("Can't find some fields {}", mhd_req.post);
                    	}
					}
				}
				mhd_resp.data+="]}";
				log_info("Out Response {}", mhd_resp.data.c_str());
				mhd_resp.code = 200;
				return true;
            }else if (store->method == "GetBinding" && req_post.HasMember("params") && req_post["params"].IsArray()  && req_post["params"].Size() > 0){
                mhd_resp.data = 
                "{"
                    "\"id\": \""+store->req_id_str+"\","
                    "\"method\": \""+store->method+"\","
                    "\"result\": [";
                bool isf=true;
                for(uint64_t it_keys=0; it_keys<req_post["params"].Size(); it_keys++){
                    if( req_post["params"][it_keys].IsObject()){

                        if(req_post["params"][it_keys].HasMember("cur") && req_post["params"][it_keys].HasMember("address") ){
                            if(req_post["params"][it_keys]["cur"].IsString() && req_post["params"][it_keys]["address"].IsString()){
                                string addr=req_post["params"][it_keys]["address"].GetString();
                                string cur=req_post["params"][it_keys]["cur"].GetString();

                                if(isf)
                                    isf=false;
                                else
                                    mhd_resp.data +=",";

                                mhd_resp.data += "{ \"cur\":\""+cur+"\","
                                    "\"address\":\""+addr+"\","
                                    "\"result\":";

                                string val;
                                if(cur=="ETH"){
                                     boost::to_lower(addr);
                                    Status s=db_erc->Get(ReadOptions(), Slice(addr), &val);
                                    if(s.ok())
                                        mhd_resp.data +="\""+val+"\"}";
                                    else{
                                        mhd_resp.data +="\"null\"}";
                                    }
                                }
                                else
                                {
                                    //unknown currency
                                    log_info("GetBinding: unknown currency {}", mhd_req.post);
                                    mhd_resp.data += "\"error\"}";
                                }
                            }
                        }
                        else
                        {
                            log_info("Can't find some fields {}", mhd_req.post);
                          
                        }
                    }
                }

                mhd_resp.data += "]}";
                log_info("Out Response {}", mhd_resp.data.c_str());
                mhd_resp.code = 200;
                return true; 
			}else if (store->method == "SetLimit" && req_post.HasMember("params") && req_post["params"].IsObject() ){
				mhd_resp.data = 
				"{"
					"\"id\": \""+store->req_id_str+"\","
					"\"method\": \""+store->method+"\","
			   		"\"result\": \"";
				if(req_post["params"].HasMember("limit") && req_post["params"]["limit"].IsString()){
					string limit(req_post["params"]["limit"].GetString());
                    try{
                        mpz_class tmpint;
                        tmpint=limit;
                        Status s=db_info->Put(WriteOptions(), Slice("limit_tkn"), Slice(limit));
                            if(!s.ok()){
                                log_info("SetLimit: error  put limit_tkn{}", mhd_req.post);
                                mhd_resp.data +="error";
                            }
                            else{
                                mhd_resp.data +="ok";
                            }
                    }
                    catch(...){
                        log_info("SetLimit: error type put limit_tkn{}", mhd_req.post);
                        mhd_resp.data +="error";
                    }
        		}
                else{
                    log_info("Can't find some fields {}", mhd_req.post);
                    mhd_resp.data +="error";
                }

        		mhd_resp.data +="\"}";
				log_info("Out Response {}", mhd_resp.data.c_str());
				mhd_resp.code = 200;
				return true; 
                
			}else if (store->method == "GetCurrentLimit"){
                mhd_resp.data = 
                "{"
                    "\"id\": \""+store->req_id_str+"\","
                    "\"method\": \""+store->method+"\","
                    "\"result\": [\"";

                string val;
                Status s=db_info->Get(ReadOptions(), Slice("limit_tkn"), &val);
                if(s.ok())
                    mhd_resp.data +=val+"\",\"";
                else{
                    log_info("GetLimit: error  get limit_tkn{}", mhd_req.post);
                    mhd_resp.data +="error\",\"";
                }
                    
                s=db_info->Get(ReadOptions(), Slice("limit_tkn_cur"), &val);
                if(s.ok())
                    mhd_resp.data +=val;
                else{
                    log_info("GetLimit: error  get limit_tkn_cur{}", mhd_req.post);
                    mhd_resp.data +="error";
                }

                mhd_resp.data += "\"]}";
                log_info("Out Response {}", mhd_resp.data.c_str());
                mhd_resp.code = 200;
                return true; 
            }else{
				mhd_resp.data = 
				"{"
					"\"id\":" +store->req_id_str+","
			   		"\"result\": []"
				"}";
				log_info("Unknown method {}", mhd_req.post.c_str());
				mhd_resp.code = 200;
				return true;
			}
			}else{
				mhd_resp.data = 
				"{"
					"\"id\": null,"
			   		"\"result\": []"
				"}";
				log_info("Can't find required fields {}", mhd_req.post.c_str());
				mhd_resp.code = 200;
				return true;
			}

		}
		else
		{
			mhd_resp.data = 
			"{"
				"\"id\": null,"
			   	"\"result\": []"
			"}";
			log_info("Bad json {}", mhd_req.post.c_str());
			mhd_resp.code = 200;
			return true;
		}
	}
	else{
		mhd_resp.data = 
			"{"
				"\"id\": null,"
			   	"\"result\": []"
			"}";
			log_info("Empty post {}", mhd_req.post.c_str());
			mhd_resp.code = 200;
			return true;
	}
}


bool Meta_ERC_Convert::init() {
    signal(SIGTERM, sig_handler);
    log_notice("meta_erc_convert START");

    if (!read_config())
        return false;

    try {
        local_store.resize(get_threads()+1);
        for(size_t i = 0; i < local_store.size(); i++) {
            local_store[i] = new LocalStore;
        }
     } catch (std::exception& e) {
         log_err("[{}:%d] Catch exception while init LocalStore: {}", __PRETTY_FUNCTION__, __LINE__, e.what());
        return false;
    }
   
   //init DB
    kDBPath_erc = db_path+"rocksdb_erc_to_mhc";
    kDBPath_info = db_path+"rocksdb_address_info";
    kDBPath_tx_mhc = db_path+"rocksdb_tx_mhc";
    kDBPath_tx_eth = db_path+"rocksdb_tx_eth";
    kDBPath_tx_mhc_un = db_path+"rocksdb_tx_mhc_unload";
    kDBPath_tx_eth_un = db_path+"rocksdb_tx_eth_unload";
   
    Options options_mhc, options_erc, options_info, options_mhc_tx, options_erc_tx, options_mhc_tx_un, options_erc_tx_un;

    options_mhc.IncreaseParallelism();
    options_mhc.OptimizeLevelStyleCompaction();
    // create the DB if it's not already present
    options_mhc.create_if_missing = true;

    options_erc.IncreaseParallelism();
    options_erc.OptimizeLevelStyleCompaction();
    // create the DB if it's not already present
    options_erc.create_if_missing = true;

    options_info.IncreaseParallelism();
    options_info.OptimizeLevelStyleCompaction();
    // create the DB if it's not already present
    options_info.create_if_missing = true;

    options_mhc_tx.IncreaseParallelism();
    options_mhc_tx.OptimizeLevelStyleCompaction();
    // create the DB if it's not already present
    options_mhc_tx.create_if_missing = true;

    options_erc_tx.IncreaseParallelism();
    options_erc_tx.OptimizeLevelStyleCompaction();
    // create the DB if it's not already present
    options_erc_tx.create_if_missing = true;

    options_mhc_tx_un.IncreaseParallelism();
    options_mhc_tx_un.OptimizeLevelStyleCompaction();
    // create the DB if it's not already present
    options_mhc_tx_un.create_if_missing = true;

    options_erc_tx_un.IncreaseParallelism();
    options_erc_tx_un.OptimizeLevelStyleCompaction();
    // create the DB if it's not already present
    options_erc_tx_un.create_if_missing = true;

    Status s = DB::Open(options_mhc, kDBPath_mhc, &db_mhc);
    s = DB::Open(options_erc, kDBPath_erc, &db_erc);
    s = DB::Open(options_info, kDBPath_info, &db_info);
    s = DB::Open(options_mhc_tx, kDBPath_tx_mhc, &db_mhc_tx);
    s = DB::Open(options_erc_tx, kDBPath_tx_eth, &db_erc_tx);
    s = DB::Open(options_mhc_tx_un, kDBPath_tx_mhc_un, &db_mhc_tx_un);
    s = DB::Open(options_erc_tx_un, kDBPath_tx_eth_un, &db_erc_tx_un);
  

    string val;
    s=db_info->Get(ReadOptions(), Slice("last_block"), &val);
    if(!s.ok()){
        s=db_info->Put(WriteOptions(), Slice("last_block"), Slice("0"));
        if(!s.ok()){
           log_err("can't put last_block in db");
        }
    }
    s=db_info->Get(ReadOptions(), Slice("limit_tkn"), &val);
    if(!s.ok()){
        s=db_info->Put(WriteOptions(), Slice("limit_tkn"), Slice("1000"));
        if(!s.ok()){
           log_err("can't put limit in db");
        }
    }
     else
        log_info(" start limit_tkn {}", val);
    s=db_info->Get(ReadOptions(), Slice("limit_tkn_cur"), &val);
    if(!s.ok()){
        s=db_info->Put(WriteOptions(), Slice("limit_tkn_cur"), Slice("0"));
        if(!s.ok()){
           log_err("can't put limit current in db");
        }
    }
    else
        log_info(" start limit_tkn_cur {}", val);
    
    s=db_info->Get(ReadOptions(), Slice("id_request"), &val);
    if(!s.ok()){
        s=db_info->Put(WriteOptions(), Slice("id_request"), Slice("1"));
        if(!s.ok()){
           log_err("can't put id_request in db");
        }
    }
   
    //start thread updating
    update_th=  new std::thread(&meta_erc_convert::Meta_ERC_Convert::update_thread, this);
	return true;
}
//type=0 - on common mhc
//type=1 - on common eth
int Meta_ERC_Convert::make_tx_full(const string & key_it, mpz_class & value_it, int type){
    string addr, tx,key;
    vector<string> vec;
    bool result=0;     
    
        addr=key_it;
        Status s;
        //checking limits
            string val_lim;
            if(type==0)
                s=db_info->Get(ReadOptions(), Slice("limit_tkn"), &val_lim);
            if(s.ok() || type==1){
                string val_lim_cur;
                s=db_info->Get(ReadOptions(), Slice("limit_tkn_cur"), &val_lim_cur);
                if(s.ok()){
                    mpz_class lim_cur;
                    lim_cur=val_lim_cur; 
                    mpz_class lim_int; 
                    mpz_class lim_dif;
                   
                    if(type==0){
                        lim_int=val_lim; 
                        lim_dif=value_it+lim_cur;

                        if(lim_dif<=lim_int){
                            s=db_info->Put(WriteOptions(), Slice("limit_tkn_cur"),Slice(lim_dif.get_str()));
                            if(s.ok()){
                                //make transaction from common erc to binding eth
                               int  res=make_tx(2, addr, value_it);
                               if(res==0){
                                    result=1;
                                }
                                else{
                                    //return reserved tokens
                                    s=db_info->Put(WriteOptions(), Slice("limit_tkn_cur"),Slice(val_lim_cur));
                                }
                            }
                            else
                                log_info("error make_tx_full limit_tkn_cur in db");
                        }
                        else
                            log_info("error make_tx_full limit_tkn_cur+value_tx> limit");        
                    }
                    else if(type==1){
                        
                        if(lim_cur>=value_it){
                            lim_dif=lim_cur-value_it;
                            s=db_info->Put(WriteOptions(), Slice("limit_tkn_cur"),Slice(lim_dif.get_str()));
                            if(s.ok()){
                               //make transaction from common mhc to binding mhc
                               int  res=make_tx(3, addr, value_it);
                               if(res==0){
                                    result=1;
                                }
                                else{
                                     //return reserved tokens
                                    s=db_info->Put(WriteOptions(), Slice("limit_tkn_cur"),val_lim_cur);
                                }
                           }
                           else
                                log_info("error make_tx_full limit_tkn_cur in db");
                        }
                        else
                            log_info("error make_tx_full limit_tkn_cur-value_tx<0 ");      
                    }
                }
            }
    return result;
}

void Meta_ERC_Convert::update_thread(){
    while(true){
  		
  		log_info("start update");
    	unordered_set<string> for_del;
        mpz_class val_mzp, lim_mzp;
        vector<string> vec;
        string val_db, addr;
        Status s;
       
        log_info("start unloaded tx");

        string to,trx, finding, addr_out;
        //trying make unloaded transaction 
        rocksdb::Iterator* it = db_erc_tx_un->NewIterator(rocksdb::ReadOptions());
        for(it->SeekToFirst(); it->Valid(); it->Next()){
            vec.clear();
            val_mzp=0;
            string key(it->key().ToString());
            string value(it->value().ToString());

            test_strpbrk(key,":",vec);
            if(vec.size()>1){
                addr=vec.at(0);
                trx=vec.at(1);
                s =db_erc->Get(ReadOptions(), Slice(addr), &addr_out);
                if(s.ok()){
                    val_mzp=value;
                    int isok=make_tx_full(addr_out, val_mzp,1);
                    if(isok==1){
                        for_del.insert(key);
                        s=db_erc_tx->Put(WriteOptions(), Slice(key), Slice(val_mzp.get_str()));
                    }
                }
                else
                    log_info("Update binding not found {} {}", addr,value);
            }
        }
  		for(auto itt=for_del.begin(); itt!=for_del.end(); itt++){           
            s=db_erc_tx_un->Delete(WriteOptions(), Slice(*itt));
        }
        for_del.clear();

  		//make new transactions
    	string request;
    	string data, data_field;
    	//MHC
        log_info("start new mhc tx");
        uint64_t id_req;
        string id_req_out, from;
  		string str=common_mhc;
  			
            id_req=get_id_req(id_req_out);
  			request="{\"id\":"+id_req_out+",\"params\":{\"address\": \""+str+"\"},\"method\":\"fetch-history\"}";
  			int code = levent_update.post_core(host_cr_mhc, port_cr_mhc, host_cr_mhc, path_cr_mhc, request, data, 5000);
  			if (code == 200 && !data.empty()) {		
				rapidjson::Document resp;
				if (!resp.Parse(data.c_str()).HasParseError()) {
				    if(resp.HasMember("id") && resp["id"].IsUint64() && resp.HasMember("result") && resp["result"].IsArray() && resp["result"].Size()>0){
                        uint64_t idd=resp["id"].GetUint64();
                       
                        if(idd==id_req){
                            for(uint64_t it_keys=0; it_keys<resp["result"].Size(); it_keys++){
                                if( resp["result"][it_keys].IsObject()){
                                    if(resp["result"][it_keys].HasMember("to") && resp["result"][it_keys]["to"].IsString() &&
                                       resp["result"][it_keys].HasMember("value") && resp["result"][it_keys]["value"].IsUint64() &&
                                       resp["result"][it_keys].HasMember("transaction") && resp["result"][it_keys]["transaction"].IsString() &&
                                       resp["result"][it_keys].HasMember("data") && resp["result"][it_keys]["data"].IsString() &&
                                       resp["result"][it_keys].HasMember("from") && resp["result"][it_keys]["from"].IsString() ){
                                        val_mzp=0;
                                        to=string(resp["result"][it_keys]["to"].GetString());
                                        from=string(resp["result"][it_keys]["from"].GetString());
                                        trx=string(resp["result"][it_keys]["transaction"].GetString());
                                        data_field=string(resp["result"][it_keys]["data"].GetString());
                                        val_mzp=resp["result"][it_keys]["value"].GetUint64();
                                        finding=from+":"+trx;
                                        addr_out.clear();
                                        if(!data_field.empty())
                                            hex2ascii(data_field, addr_out);     
                                        
                                        if(to==str && val_mzp>0 && !addr_out.empty()){
                                            s=db_mhc_tx->Get(ReadOptions(), Slice(finding), &val_db);
                                            if(!s.ok()){
                                                int isok=make_tx_full(addr_out, val_mzp,0);
                                                if(isok==1){
                                                    log_info("Update request make_tx_full db_mhc_tx {}, {}", finding, val_mzp.get_str()) ;
                                                    s=db_mhc_tx->Put(WriteOptions(), Slice(finding), Slice(val_mzp.get_str()));      
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
				}
			}
  		//ETH
        log_info("eth ");
        id_req_out.clear();
        id_req=get_id_req(id_req_out);
        val_db.clear();
        s=db_info->Get(ReadOptions(), Slice("last_block"), &val_db);
        log_info("last_block {}",val_db);
        request="{\"id\":"+id_req_out+",\"params\":{\"group\": \""+group_eth+"\", \"block\": "+val_db+"},\"method\":\"batch-history-tkn\"}";

        code = levent_update.post_core(host_cr_eth, port_cr_eth, host_cr_eth, path_cr_eth, request, data, 5000);
        if (code == 200 && !data.empty()) {          
                rapidjson::Document resp;
                if (!resp.Parse(data.c_str()).HasParseError()) {
                    if(resp.HasMember("id") && resp["id"].IsUint64() && resp.HasMember("data") && resp["data"].IsObject() 
                        ){
                        uint64_t idd=resp["id"].GetUint64();
                        if(idd==id_req){
                            if(resp["data"].HasMember("last_block") && resp["data"]["last_block"].IsUint64() && 
                                resp["data"].HasMember("history") && resp["data"]["history"].IsArray()){
                            uint64_t last=resp["data"]["last_block"].GetUint64();
                            s=db_info->Put(WriteOptions(), Slice("last_block"), Slice(boost::lexical_cast<string>(last)));
                            if(!s.ok()){
                                log_info("can't put last_block in db");
                            }
                            for(uint64_t it_keys=0; it_keys<resp["data"]["history"].Size(); it_keys++){
                                if(resp["data"]["history"][it_keys].HasMember("address") && resp["data"]["history"][it_keys]["address"].IsString()){
                                    string addr(resp["data"]["history"][it_keys]["address"].GetString());
                                    if(addr==common_eth){
                                   
                                        if( resp["data"]["history"][it_keys].HasMember("value") && resp["data"]["history"][it_keys]["value"].IsArray()
                                          && resp["data"]["history"][it_keys]["value"].Size()>0){

                                            for(uint64_t it_val=0; it_val<resp["data"]["history"][it_keys]["value"].Size(); it_val++){
                                                if(resp["data"]["history"][it_keys]["value"][it_val].HasMember("to") && 
                                                    resp["data"]["history"][it_keys]["value"][it_val]["to"].IsString() &&
                                                    resp["data"]["history"][it_keys]["value"][it_val].HasMember("to_value") && 
                                                    resp["data"]["history"][it_keys]["value"][it_val]["to_value"].IsString () &&
                                                    resp["data"]["history"][it_keys]["value"][it_val].HasMember("transaction") && 
                                                    resp["data"]["history"][it_keys]["value"][it_val]["transaction"].IsString() &&
                                                    resp["data"]["history"][it_keys]["value"][it_val].HasMember("token") && 
                                                    resp["data"]["history"][it_keys]["value"][it_val]["token"].IsString() &&
                                                    resp["data"]["history"][it_keys]["value"][it_val].HasMember("from") && 
                                                    resp["data"]["history"][it_keys]["value"][it_val]["from"].IsString() ){

                                                    val_mzp=0;
                                                    string token (resp["data"]["history"][it_keys]["value"][it_val]["token"].GetString());
                                                    if(token==address_erc_tkn){

                                                    to=string(resp["data"]["history"][it_keys]["value"][it_val]["to"].GetString());
                                                    from=string(resp["data"]["history"][it_keys]["value"][it_val]["from"].GetString());
                                                    trx=string(resp["data"]["history"][it_keys]["value"][it_val]["transaction"].GetString());
                                                    val_mzp=resp["data"]["history"][it_keys]["value"][it_val]["to_value"].GetString();

                                                    if(to==common_eth && val_mzp>0 ){
                                                        finding=from+":"+trx;
                                                        addr_out.clear();
                                                        s =db_erc->Get(ReadOptions(), Slice(from), &addr_out);
                                                        if(s.ok()){
                                                            s=db_erc_tx_un->Get(ReadOptions(), Slice(finding), &val_db);
                                                            if(!s.ok()){
                                                            
                                                                s=db_erc_tx->Get(ReadOptions(), Slice(finding), &val_db);
                                                                if(!s.ok()){
                                                                    int isok=make_tx_full(addr_out, val_mzp,1);
                                                                    if(isok==1){
                                                                        log_info("Update request make_tx_full db_erc_tx {} {}", from,val_mzp.get_str());
                                                                        s=db_erc_tx->Put(WriteOptions(), Slice(finding), Slice(val_mzp.get_str()));
                                                                    }
                                                                    else if(isok==0){
                                                                        log_info("Update request make_tx_full db_erc_tx_un {} {}", from,val_mzp.get_str());
                                                                        s=db_erc_tx_un->Put(WriteOptions(), Slice(finding), Slice(val_mzp.get_str()));      
                                                                    }
                                                                }
                                                            }
                                                        }
                                                        else
                                                        {
                                                            log_info("Update binding not found {} {}", from,val_mzp.get_str());
                                                            s=db_erc_tx_un->Put(WriteOptions(), Slice(finding), Slice(val_mzp.get_str()));     
                                                        }      
                                                    }
                                                    }
                                                }
                                        
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        sleep(60);
    }
}
uint64_t Meta_ERC_Convert::get_id_req(string & out_val){
	string val;
    uint64_t val_int=0;
	Status s=db_info->Get(ReadOptions(), Slice("id_request"), &val);

    if(s.ok()){
        val_int=boost::lexical_cast<uint64_t>(val);
    	uint64_t intValue=val_int+1;
    	//Slice value((char*)&intValue, sizeof(uint64_t));
        s=db_info->Put(WriteOptions(), Slice("id_request"), Slice(boost::lexical_cast<string>(intValue)));
        if(!s.ok()){
           log_err("can't put id_request in db");
        }
    }
    out_val=val;
    return val_int;
}
//type: 
//2 - from erc common to erc
//3 - from mhc common to mhc 
//return 0 - ok
// 1 - error
int Meta_ERC_Convert::make_tx(int type, const string & addr, mpz_class & val){
	string request="";
	string host_make, path_make, data, pass;
	int port_make=0;
    string id_req_out;
	uint64_t id_req_out_int=get_id_req(id_req_out);
    bool is_create=false;
	
	if(type == 2){
        is_create=true;
		host_make=host_cr_eth;
		path_make=path_cr_eth;
		port_make=port_cr_eth;
        pass=common_pass_eth;
		request+="{\"id\":" +id_req_out+ ", \"method\": \"create-tx-token\", \"params\": {"
        "\"password\": \""+pass+"\", \"address\": \""+ common_eth +"\", \"to\":\""+addr+"\","
        "\"token\": \"" + address_erc_tkn +"\","
        "\"value\": \""+val.get_str()+"\", \"fee\": \"auto\",\"isPending\": \"true\"}}";

        log_info("from erc common to erc : {}", request);
	}
	else if(type==3){
		host_make=host_cr_mhc;
		path_make=path_cr_mhc;
		port_make=port_cr_mhc;
        //сначала проверка баланса кошелька
        request="{\"id\":"+id_req_out+", \"method\":\"fetch-balance\", "
            "\"params\":{\"address\":\""+common_mhc+"\"}}";
        int code = levent_update.post_core(host_make, port_make, host_make, path_make, request, data, 5000);
        log_info("fetch-balance {}", data); 
        if(code == 200 && !data.empty()){
            rapidjson::Document resp;
            if (!resp.Parse(data.c_str()).HasParseError()) {
                if(resp.HasMember("id") && resp["id"].IsUint64() && id_req_out_int==resp["id"].GetUint64()){
                    if(resp.HasMember("error")){
                        log_info("balance_ERROR {}", data); 
                    }
                    else if(resp.HasMember("result") && resp["result"].IsObject()){
                        if(resp["result"].HasMember("received") && resp["result"]["received"].IsUint64() &&
                           resp["result"].HasMember("spent") && resp["result"]["spent"].IsUint64() ){
                            mpz_class rec,sp;
                            rec= resp["result"]["received"].GetUint64();
                            sp= resp["result"]["spent"].GetUint64();
                            if(sp<rec){                               
                                if((rec-sp)>=val){
                                    log_info("balance_OK {}", data); 
                                    is_create=true;
                                }
                            }
                        }
                    }
                }
            }
        }
		request="{\"id\":"+id_req_out+", \"method\":\"create-tx\", "
            "\"params\":{\"address\":\""+common_mhc+"\",\"to\":\"" + addr+"\", \"value\":\""+val.get_str()+ "\", \"fee\": \"0\", \"nonce\": \"1\" }}";
        log_info("from mhc common to mhc  : {}", request);
	}
    if(is_create){
        data.clear();
    log_info("Tx_create request {}", request);
    int code = levent_update.post_core(host_make, port_make, host_make, path_make, request, data, 5000);
    if(code == 200 && !data.empty()){
        log_info("Tx_create post_core {}", data);          
        rapidjson::Document resp;
        if (!resp.Parse(data.c_str()).HasParseError()) {
            if(resp.HasMember("id") && resp["id"].IsUint64() && id_req_out_int==resp["id"].GetUint64()){
                if( type==3){
                    if(resp.HasMember("error") )
                        log_info("Tx_create_ERROR {}", data);
                    else if(resp.HasMember("params") && resp["params"].IsObject()){
                        log_info("Tx_create_OK {}", data);

                        request="{\"id\":"+id_req_out+", \"method\":\"send-tx\", "
                            "\"params\":{\"address\":\""+common_mhc+"\",\"to\":\"" + addr+"\", \"value\":\""+val.get_str()+ "\", \"fee\": \"0\" }}";

                        data.clear();
                        log_info("Tx_try_send {}", request);

                        levent_update.post_core(host_make, port_make, host_make, path_make, request, data, 5000);
                        if(code == 200 && !data.empty()){
                                 rapidjson::Document resp1;
                                if (!resp1.Parse(data.c_str()).HasParseError()){
                                    if(resp1.HasMember("error") )
                                        log_info("Tx_send_ERROR {}", data);
                                    else if(resp1.HasMember("result") && resp1["result"].IsString()){
                                        string res(resp1["result"].GetString());
                                        if(res!="ERROR" && res!="error"){
                                            log_info("Tx_send_OK {}", data);
                                            return 0;
                                        }
                                        log_info("Tx_send_ERROR {}", data); 
                                    }
                                    else
                                         log_info("Tx_try_send_parse {} ,data: {}", code, data);
                                } 
                        }
                        else
                        {
                             log_info("Tx_try_send {} ,data: {}", code, data);
                        }
                        
                    }
                    else
                        log_info("Tx_create_ERROR {}", data);
                }
                else if(type==2){
                    if(resp.HasMember("error") )
                        log_info("Tx_create_ERROR {}", data);
                    else if(resp.HasMember("result") && resp["result"].IsString() ){
                        string res(resp["result"].GetString());
                        if(res=="ok"){
                            
                            log_info("Tx_create_OK {}", data);
                           
                            request="{\"id\":" +id_req_out+ ", \"method\": \"send-tx-token\", \"params\": {"
                                "\"password\": \""+pass+"\", \"address\": \""+ common_eth +"\", \"to\":\""+addr+"\","
                                "\"token\": \"" + address_erc_tkn +"\","
                                "\"value\": \""+val.get_str()+"\", \"fee\": \"auto\",\"isPending\": \"true\"}}";

                            
                            data.clear();
                            log_info("Tx_try_send {}", request);
                            levent_update.post_core(host_make, port_make, host_make, path_make, request, data, 5000);
                            if(code == 200 && !data.empty()){
                                 rapidjson::Document resp1;
                                if (!resp1.Parse(data.c_str()).HasParseError()){
                                    if(resp1.HasMember("result") && resp1["result"].IsString()){  
                                        string res(resp1["result"].GetString());
                                        if(res!="ERROR" && res!="error"){
                                            log_info("Tx_send_OK {}", data);
                                            return 0;
                                        }
                                        log_info("Tx_send_ERROR {}", data); 
                                    }

                                    else
                                         log_info("Tx_try_send_parse {} , data: {}", code, data);
                                } 
                            }
                            else
                                log_info("Tx_try_send {}, data: {}", code, data);
                        
                        }
                        else
                            log_info("Tx_create_ERROR {}", data);
                    }
                    else
                        log_info("Tx_create_UNKNOWN_FIELDS {}", data);
                }
            }
        }
    }
    }
    return 1;
}


bool Meta_ERC_Convert::read_config() {
	libconfig::Config cfg;

    try {
        cfg.readFile("meta_erc_convert.conf");
    } catch(const libconfig::FileIOException &fioex) {
        log_err("[{}:%d] Was not found config: meta_erc_convert.conf", __PRETTY_FUNCTION__, __LINE__);
        return false;
    } catch(libconfig::ParseException &pex) {
        log_err("[{}:%d] Config file meta_db.conf parse error at %d - {}", __PRETTY_FUNCTION__, __LINE__,  pex.getLine(), pex.getError());
        return false;
    }


    try {
    	/* Default section */
    	if (cfg.getRoot().exists("meta_erc_convert")) {
            unsigned int threads_count = 8;
            cfg.getRoot()["meta_erc_convert"].lookupValue("thread_count", threads_count);
			set_threads(threads_count);
			//std::cout<<threads_count<<std::endl;

            unsigned int port = 8082;
            cfg.getRoot()["meta_erc_convert"].lookupValue("port", port);
			set_port(port);
			//std::cout<<port<<std::endl;

			string curr_host="127.0.0.1";
            cfg.getRoot()["meta_erc_convert"].lookupValue("host", curr_host);
			set_host(curr_host);

			cfg.getRoot()["meta_erc_convert"].lookupValue("directory_db", db_path);
			//std::cout<<out_dir<<std::endl;
			
			cfg.getRoot()["meta_erc_convert"].lookupValue("common_eth", common_eth);
			cfg.getRoot()["meta_erc_convert"].lookupValue("common_mhc", common_mhc);

			cfg.getRoot()["meta_erc_convert"].lookupValue("host_cr_eth", host_cr_eth);
			cfg.getRoot()["meta_erc_convert"].lookupValue("host_cr_mhc", host_cr_mhc);

			cfg.getRoot()["meta_erc_convert"].lookupValue("path_cr_eth", path_cr_eth);
			cfg.getRoot()["meta_erc_convert"].lookupValue("path_cr_mhc", path_cr_mhc);
			
			cfg.getRoot()["meta_erc_convert"].lookupValue("port_cr_eth", port_cr_eth);
			cfg.getRoot()["meta_erc_convert"].lookupValue("port_cr_mhc", port_cr_mhc);

			cfg.getRoot()["meta_erc_convert"].lookupValue("address_erc_tkn", address_erc_tkn);

			cfg.getRoot()["meta_erc_convert"].lookupValue("group_eth", group_eth);
			cfg.getRoot()["meta_erc_convert"].lookupValue("group_mhc", group_mhc);


    	}

        return true;
    }
    catch (const libconfig::SettingNotFoundException& e) {
        log_err("[{}:%d] Setting {} does not found in ss_mzalg.conf", __PRETTY_FUNCTION__, __LINE__, e.getPath());
    }
    catch (std::exception& e) {
        log_err("[{}:%d] Unknown exception: {} in ss_mzalg.conf", __PRETTY_FUNCTION__, __LINE__, e.what());
    }

    return false;
}

}