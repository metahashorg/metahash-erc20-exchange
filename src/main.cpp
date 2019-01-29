#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <string>
#include <memory>
#include <malloc.h>
#include <boost/algorithm/string.hpp>
#include <string>
#include <iostream>
#include <sniper/log/log.h>
#include <sniper/app/App.hpp>
#include "meta_erc_convert.h"
using namespace std;

int main (int argc, char** argv)
{

    std::string app_name = sniper::app::App::get_name();
    srand(time(NULL) + getpid());
    std::cout<<app_name<<std::endl;

    openlog(app_name.c_str(), LOG_PID | LOG_NDELAY, LOG_LOCAL1);
    srand(time(NULL) + getpid());

    meta_erc_convert::Meta_ERC_Convert appdb;
    appdb.start("./");
    exit(1);

    return 0;
}


