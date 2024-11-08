#ifndef API_H
#define API_H

#include <crow.h>
#include <string>
#include "DatabaseHandler.h"

class API {
public:
    API(DatabaseHandler& dbHandler);
    void run(int port);

private:
    DatabaseHandler& db;

    crow::response getMediaData(const crow::request& req);
    crow::response getUserMetadata(const crow::request& req);

    crow::response addProfile(const crow::request& req);
    crow::response deleteProfile(const crow::request& req);
    crow::response listProfiles(const crow::request& req);

    std::unordered_map<std::string, std::string> tokens;

    bool checkToken(const std::string& tokenProvided, const std::string& userID);
    void loadTokens();
};

#endif // API_H
