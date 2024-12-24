#ifndef DATABASEHANDLER_H
#define DATABASEHANDLER_H

#include "CppSQLite3.h"
#include <string>
#include <vector>

class DatabaseHandler {
public:
    DatabaseHandler(const std::string& dbPath);
    ~DatabaseHandler();

    bool addProfile(const std::string& userID, const std::string& profileID, int pictureID);
    bool deleteProfile(const std::string& userID, const std::string& profileID);
    void getProfiles(const std::string& userID, std::vector<std::pair<std::string, std::string>>& profiles);

    std::string getToken(const std::string& userID);
    std::vector<std::pair<std::string, std::string>> getAllUserPasswords();




    std::vector<std::string> getAllCollections();
    std::string getCollectionById(const std::string& collectionId);
    std::vector<std::string> getAllMediaIdsFromCollection(const std::string& collectionId);
    std::string getMediaDataById(const std::string& mediaId);
    std::vector<std::string> getAllMediaMetadataByMediaId(const std::string& mediaId);

    std::vector<std::string> getColumnNames(const std::string& tableName);
    void generateMediaDataJson();
    void generateMediaMetadataJson(const std::string& userID, const std::string& profileID);
    int insertMediaMetadata(
        const std::string& userID,
        const std::string& profileID,
        const std::string& mediaID,
        double percentageWatched,
        const std::string& languageChosen,
        const std::string& subtitlesChosen
    );
    std::string getImagePathById(const std::string& id, const std::string& coversPath);

private:
    CppSQLite3DB db;
};

#endif // DATABASEHANDLER_H
