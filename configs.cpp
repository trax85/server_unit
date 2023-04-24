#include "configs.h"

std::string HOME_DIR = "/home/tejas/temp";
std::string DATABASE_NAME = "web";

std::string DATABASE_URL = "tcp://127.0.0.1:3306";
std::string DATABASE_USERNAME = "mcu";
std::string DATABASE_PASSWORD = "password";

std::string INSTANCE_FILE_DATA_DIR = "/instance/";
std::string USER_FILE_DATA_DIR = "/user_data/";

std::string USERDAT_TABLE_NAME = "user_data";
std::string USERDAT_DAT_COL_ID = "file_name";
std::string USERDAT_RES_COL_ID = "final_result";
std::string USERDAT_INTER_COL_ID = "intermidiate_result";
std::string USERDAT_ALGO_COL_ID = "instance_name";
std::string USERDAT_DAT_PRIORITY_COL_ID = "priority";

std::string INSTANCE_TABLE_NAME = "instance";
std::string INSTANCE_NAME_COL_ID = "instance_name";
std::string INSTANCE_ALGO_COL_ID = "algorithm";
std::string INSTANCE_FILE_COL_ID = "csvfile";