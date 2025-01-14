#include <iostream>
#include <fstream>
#include <string>
#include "DatabaseHandler.h"
#include "json.hpp" // nlohmann/json header
#include "api.h"

using json = nlohmann::json;

void loadPaths(const std::string& configFilePath, std::string& databasePath, std::string& coversPath, std::string& chunksPath, std::string& duckdnsDomain) {
    std::ifstream configFile(configFilePath);
    if (!configFile.is_open()) {
        throw std::runtime_error("Could not open configuration file: " + configFilePath);
    }

    json configJson;
    configFile >> configJson; // Read JSON file into configJson object

    if (configJson.contains("databasePath") && configJson["databasePath"].is_string()) {
        databasePath = configJson["databasePath"].get<std::string>();
    }
    else {
        throw std::runtime_error("Invalid configuration: 'databasePath' not found or incorrect type.");
    }
    if (configJson.contains("coversPath") && configJson["coversPath"].is_string()) {
        coversPath = configJson["coversPath"].get<std::string>();
    }
    else {
        throw std::runtime_error("Invalid configuration: 'coversPath' not found or incorrect type.");
    }
    if (configJson.contains("chunksPath") && configJson["chunksPath"].is_string()) {
        chunksPath = configJson["chunksPath"].get<std::string>();
    }
    else {
        throw std::runtime_error("Invalid configuration: 'chunksPath' not found or incorrect type.");
    }
    if (configJson.contains("duckdnsDomain") && configJson["duckdnsDomain"].is_string()) {
        duckdnsDomain = configJson["duckdnsDomain"].get<std::string>();
    }
    else {
        throw std::runtime_error("Invalid configuration: 'duckdnsDomain' not found or incorrect type.");
    }
}


int main() {
    try {

        std::cout << R"(
            
            ,---.  .           .  .---.                      
            |  -'  |-. ,-. ,-. |- \___  ,-. ,-. .  , ,-. ,-. 
            |  ,-' | | | | `-. |      \ |-' |   | /  |-' |   
            `---|  ' ' `-' `-' `' `---' `-' '   `'   `-' '   
             ,-.|                                            
             `-+'                                            
                                           
        )" << std::endl;

        // Specify the path to your configuration JSON file
        std::string configFilePath = "config.json"; // Adjust the path as needed

        // Load the database path from the config file
        std::string databasePath; 
        std::string coversPath;
        std::string chunksPath;
        std::string duckdnsDomain; 
        
        loadPaths(configFilePath, databasePath, coversPath, chunksPath, duckdnsDomain);
        std::cout << "Database path loaded from config: " << databasePath << std::endl;

        // Create and initialize the DatabaseHandler with the database path
        DatabaseHandler dbHandler(databasePath);

        // Start your server logic here, for example:
        std::cout << "Server starting..." << std::endl;

        // Initialize the API with the database handler
        

        API api(dbHandler, coversPath, chunksPath, duckdnsDomain);
        std::cout << "Public IP: " << api.getPublicIP(duckdnsDomain) << std::endl;
        // Run the API server on a specified port
        api.run(38080);

    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
