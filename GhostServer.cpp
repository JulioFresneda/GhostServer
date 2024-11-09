#include <iostream>
#include <fstream>
#include <string>
#include "DatabaseHandler.h"
#include "json.hpp" // nlohmann/json header
#include "api.h"

using json = nlohmann::json;

std::string loadDatabasePath(const std::string& configFilePath) {
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Could not open configuration file: " + configFilePath);
    }

    json configJson;
    configFile >> configJson; // Read JSON file into configJson object

    if (configJson.contains("databasePath") && configJson["databasePath"].is_string()) {
        return configJson["databasePath"].get<std::string>();
    }
    else {
        throw std::runtime_error("Invalid configuration: 'databasePath' not found or incorrect type.");
    }
}

std::string loadCoversPath(const std::string& configFilePath) {
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Could not open configuration file: " + configFilePath);
    }

    json configJson;
    configFile >> configJson; // Read JSON file into configJson object

    if (configJson.contains("coversPath") && configJson["coversPath"].is_string()) {
        return configJson["coversPath"].get<std::string>();
    }
    else {
        throw std::runtime_error("Invalid configuration: 'coversPath' not found or incorrect type.");
    }
}

std::string loadChunksPath(const std::string& configFilePath) {
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Could not open configuration file: " + configFilePath);
    }

    json configJson;
    configFile >> configJson; // Read JSON file into configJson object

    if (configJson.contains("chunksPath") && configJson["chunksPath"].is_string()) {
        return configJson["chunksPath"].get<std::string>();
    }
    else {
        throw std::runtime_error("Invalid configuration: 'coversPath' not found or incorrect type.");
    }
}

int main() {
    try {
        // Specify the path to your configuration JSON file
        std::string configFilePath = "config.json"; // Adjust the path as needed

        // Load the database path from the config file
        std::string databasePath = loadDatabasePath(configFilePath);
        std::cout << "Database path loaded from config: " << databasePath << std::endl;

        // Create and initialize the DatabaseHandler with the database path
        DatabaseHandler dbHandler(databasePath);

        // Start your server logic here, for example:
        std::cout << "Server starting..." << std::endl;

        // Initialize the API with the database handler
        std::string coversPath = loadCoversPath(configFilePath);
        std::string chunksPath = loadChunksPath(configFilePath);

        API api(dbHandler, coversPath, chunksPath);

        // Run the API server on a specified port
        api.run(18080);

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
