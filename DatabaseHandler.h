#ifndef DATABASEHANDLER_H
#define DATABASEHANDLER_H

#include "CppSQLite3.h"
#include <string>
#include <vector>

class DatabaseHandler {
public:
    DatabaseHandler(const std::string& dbPath);
    ~DatabaseHandler();

    bool addProfile(const std::string& userID, const std::string& profileID, const std::string& pictureID);
    bool deleteProfile(const std::string& userID, const std::string& profileID);
    void getProfiles(const std::string& userID, std::vector<std::pair<std::string, std::string>>& profiles);

    std::string getToken(const std::string& userID);
    std::vector<std::pair<std::string, std::string>> getAllUserTokens();




    std::vector<std::string> getAllCollections();
    std::string getCollectionById(const std::string& collectionId);
    std::vector<std::string> getAllMediaIdsFromCollection(const std::string& collectionId);
    std::string getMediaDataById(const std::string& mediaId);
    std::vector<std::string> getAllMediaMetadataByMediaId(const std::string& mediaId);

    std::vector<std::string> getColumnNames(const std::string& tableName);
    void generateMediaDataJson();
    void generateUserMetadataJson(const std::string& userID, const std::string& profileID);

private:
    CppSQLite3DB db;
};

#endif // DATABASEHANDLER_H
