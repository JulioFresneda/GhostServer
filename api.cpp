#include "api.h"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>
using json = nlohmann::json;



API::API(DatabaseHandler& dbHandler) : db(dbHandler) {
    // Load tokens into memory on startup
    loadTokens();

    //db.generateMediaDataJson();
    //db.generateUserMetadataJson("Julio", "test");
}

void API::loadTokens() {
    std::vector<std::pair<std::string, std::string>> users = db.getAllUserTokens();
    for (const auto& user : users) {
        tokens[user.first] = user.second;  // user.first is userID, user.second is token
    }
}

void API::run(int port) {
    crow::SimpleApp app;

    CROW_ROUTE(app, "/media/data").methods(crow::HTTPMethod::GET)([this](const crow::request& req) {
        return getMediaData(req);
        });

    // Route to get user-specific metadata JSON
    CROW_ROUTE(app, "/user/metadata").methods(crow::HTTPMethod::POST)([this](const crow::request& req) {
        return getUserMetadata(req);
        });


    // Route to add a new profile
    CROW_ROUTE(app, "/profile/add").methods(crow::HTTPMethod::POST)([this](const crow::request& req) {
        return addProfile(req);
        });

    // Route to delete a profile
    CROW_ROUTE(app, "/profile/delete").methods(crow::HTTPMethod::POST)([this](const crow::request& req) {
        return deleteProfile(req);
        });

    // Route to list all profiles for a specific user
    CROW_ROUTE(app, "/profile/list").methods(crow::HTTPMethod::POST)([this](const crow::request& req) {
        return listProfiles(req);
        });

    CROW_ROUTE(app, "/download/media_data")
        ([this]() {
        std::ifstream file("media_data.json");
        if (!file.is_open()) {
            return crow::response(500, "Failed to open media_data.json");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        crow::response res(buffer.str());
        res.add_header("Content-Type", "application/json");
        return res;
            });

    // Route to serve the pre-generated user metadata JSON file
    CROW_ROUTE(app, "/download/user_metadata").methods(crow::HTTPMethod::POST)
        ([this](const crow::request& req) {
        auto x = crow::json::load(req.body);
        if (!x) {
            return crow::response(400, "Invalid JSON");
        }

        std::string userID = x["userID"].s();
        std::string profileID = x["profileID"].s();

        // Generate user metadata JSON if needed
        db.generateUserMetadataJson(userID, profileID);

        std::ifstream file("user_metadata.json");
        if (!file.is_open()) {
            return crow::response(500, "Failed to open user_metadata.json");
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        crow::response res(buffer.str());
        res.add_header("Content-Type", "application/json");
        return res;
            });

    app.port(port).multithreaded().run();
}




crow::response API::getMediaData(const crow::request& req) {
    // Generate the media data JSON file
    db.generateMediaDataJson();

    // Read the generated file
    std::ifstream file("media_data.json");
    if (!file.is_open()) {
        return crow::response(500, "Failed to open media_data.json");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return crow::response{ buffer.str() };
}

crow::response API::getUserMetadata(const crow::request& req) {
    auto x = crow::json::load(req.body);
    if (!x) {
        return crow::response(400, "Invalid JSON");
    }

    std::string userID = x["userID"].s();
    std::string profileID = x["profileID"].s();

    // Generate the user metadata JSON file based on userID and profileID
    db.generateUserMetadataJson(userID, profileID);

    // Read the generated file
    std::ifstream file("user_metadata.json");
    if (!file.is_open()) {
        return crow::response(500, "Failed to open user_metadata.json");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    return crow::response{ buffer.str() };
}




crow::response API::addProfile(const crow::request& req) {
    auto x = crow::json::load(req.body);
    if (!x) {
        return crow::response(400, "Invalid JSON");
    }

    std::string userID = x["userID"].s();
    std::string profileID = x["profileID"].s();
    std::string pictureID = x["pictureID"].s();
    std::string token = x["token"].s();

    crow::json::wvalue response;
    

    if (!checkToken(token, userID)) {
        response["status"] = "error";
        response["message"] = "Incorrect token.";
        return crow::response(401, response);
    }

    bool success = db.addProfile(userID, profileID, pictureID);
    if (success) {
        response["status"] = "success";
        response["message"] = "Profile created successfully.";
    }
    else {
        response["status"] = "error";
        response["message"] = "Failed to create profile. User may not exist or profile already exists.";
    }

    return crow::response(std::move(response));  // Use std::move here
}

crow::response API::deleteProfile(const crow::request& req) {
    auto x = crow::json::load(req.body);
    if (!x) {
        return crow::response(400, "Invalid JSON");
    }

    std::string profileID = x["profileID"].s();
    std::string userID = x["userID"].s();
    std::string token = x["token"].s();

    crow::json::wvalue response;

    if (!checkToken(token, userID)) {
        response["status"] = "error";
        response["message"] = "Incorrect token.";
        return crow::response(401, response);
    }


    bool success = db.deleteProfile(userID, profileID);
    

    if (success) {
        response["status"] = "success";
        response["message"] = "Profile deleted successfully.";
    }
    else {
        response["status"] = "error";
        response["message"] = "Failed to delete profile. Profile may not exist.";
    }

    return crow::response(std::move(response));  // Use std::move here
}

crow::response API::listProfiles(const crow::request& req) {
    auto x = crow::json::load(req.body);
    if (!x) {
        return crow::response(400, "Invalid JSON");
    }

    std::string userID = x["userID"].s();
    std::string token = x["token"].s();

    crow::json::wvalue response;

    if (!checkToken(token, userID)) {
        response["status"] = "error";
        response["message"] = "Incorrect token.";
        return crow::response(401, response);
    }

    std::vector<std::pair<std::string, std::string>> profiles;
    db.getProfiles(userID, profiles);





    crow::json::wvalue::list profileList;

    for (const auto& profile : profiles) {
        crow::json::wvalue profileJson;
        profileJson["profileID"] = profile.first;
        profileJson["pictureID"] = profile.second;
        profileList.push_back(std::move(profileJson));  // Use std::move for inner elements
    }

    response["profiles"] = std::move(profileList);  // Move the list instead of copying
    return crow::response(std::move(response));      // Move response to avoid copy
}


bool API::checkToken(const std::string& token, const std::string& userID) {
    auto it = tokens.find(userID);
    return (it != tokens.end() && it->second == token);
}

