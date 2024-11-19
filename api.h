#ifndef API_H
#define API_H

#include <crow.h>
#include <string>
#include "DatabaseHandler.h"

class API {
public:
    API(DatabaseHandler& dbHandler, const std::string& coversPath, const std::string& chunksPath);
    void run(int port);

private:
    DatabaseHandler& db;
    const std::string& coversPath;
    const std::string chunksPath;

    crow::response handleManifestRequest(const crow::request& req, const std::string& media_id);
    crow::response handleChunkRequest(const crow::request& req, const std::string& media_id, const std::string& chunk_name);
    crow::response subtitlesRequest(const crow::request& req, const std::string& media_id, const std::string& language);

    crow::response serveFile(const std::string& path);

    crow::response getMediaData(const crow::request& req);
    crow::response getUserMetadata(const crow::request& req);

    crow::response addProfile(const crow::request& req);
    crow::response deleteProfile(const crow::request& req);
    crow::response listProfiles(const crow::request& req);

    crow::response getCoverImage(const crow::request& req, const std::string& id);

    std::unordered_map<std::string, std::string> tokens;

    bool checkToken(const std::string& tokenProvided, const std::string& userID);
    void loadTokens();

    bool validateRequest(const crow::request& req, std::string& userID, std::string& token);



};

#endif // API_H
