//#define CPPHTTPLIB_OPENSSL_SUPPORT
//#include "httplib.h"
#include <string>

using std::string;

#define API_KEY "7t2ODcsCQG8oKZw5MVICQ2NtTl0bHL3M9fr6VDCn"

std::string getPin( const string & pin ) {
/*
    httplib::Client cli("https://5n71v2lg48.execute-api.us-west-2.amazonaws.com");

    string pinnumjson = "{ \"key\":\"" + pin + "\"}";

    httplib::Headers headers = {
        { "Content-Type", "application/json" },
        { "x-api-key", API_KEY },
    };

    auto res = cli.Post("/default/getTokenAndDelete", headers, pinnumjson, "application/json" );
    if( res->status != 200 ){
        return "error";
    }
    return res->body;
*/
  return std::string("");
}