#ifndef API_H
#define API_H

#include <crow.h>
#include <string>
#include "DatabaseHandler.h"

class API {
public:
    API(DatabaseHandler& dbHandler, const std::string& coversPath, const std::string& chunksPath, const std::string& domain);
    void run(int port);
    std::string getPublicIP(const std::string& domain);

private:
    DatabaseHandler& db;
    const std::string& coversPath;
    const std::string chunksPath;
	const std::string domain;

    crow::response downloadMediaData(const crow::request& req);
    crow::response downloadMediaMetadata(const crow::request& req);

    crow::response handleManifestRequest(const crow::request& req, const std::string& media_id);
    crow::response handleChunkRequest(const crow::request& req, const std::string& media_id, const std::string& chunk_name);
    crow::response subtitlesRequest(const crow::request& req, const std::string& media_id, const std::string& language);

    crow::response serveFile(const std::string& path);

    crow::response getMediaData(const crow::request& req);
    crow::response getMediaMetadata(const crow::request& req);
    crow::response updateMediaMetadata(const crow::request& req);

    crow::response addProfile(const crow::request& req);
    crow::response deleteProfile(const crow::request& req);
    crow::response listProfiles(const crow::request& req);

    crow::response getCoverImage(const crow::request& req, const std::string& id);

    std::unordered_map<std::string, std::string> passwords;

    crow::response login(const crow::request& req);
    bool checkPassword(const std::string& tokenProvided, const std::string& userID);
    void loasPasswords();

    bool validateRequest(const crow::request& req, std::string& userID);

    



};

#endif // API_H
