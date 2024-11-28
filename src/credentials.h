#include <iostream>
#include <string.h> 
#include "Timer.h"

Timer timer = Timer();

//-------------------------------- ACCESS POINT CREDENTIALS

/*const char* ssid = "UNIFI_IDO2";
const char* password = "99Bidules!";*/

const char* ssid = "238lavigne";
const char* password = "238lavigne";

//-------------------------------- API URL

const char* server_api_url = "http://192.168.20.111:8001/api/";
const char* school_api_url = "http://192.168.20.139:8001/api/";
const char* home_api_url = "http://192.168.2.23:8001/api/";

const char* api_url = home_api_url;

/*const char* authUrl = "http://192.168.20.111:8001/api/token/";
const char* endPoint = "http://192.168.20.111:8001/api/data/";*/

/*const char* authUrl = "http://192.168.20.139:8001/api/token/";
const char* endPoint = "http://192.168.20.139:8001/api/data/";*/

const char* authUrl = "http://192.168.2.23:8001/api/token/";
const char* endPoint = "http://192.168.2.23:8001/api/data/";

//-------------------------------- USER AUTHENTICATE

const char* username = "jospin";
const char* user_password = "admin";

String accessToken;
String refreshToken;