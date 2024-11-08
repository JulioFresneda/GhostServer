#include <iostream>
#include "DatabaseHandler.h"

void testDatabaseHandler(DatabaseHandler& dbHandler) {
    int choice;
    std::string input;

    while (true) {
        std::cout << "\nChoose an option:\n";
        std::cout << "1. Get all collections\n";
        std::cout << "2. Get collection by ID\n";
        std::cout << "3. Get all media IDs from a collection\n";
        std::cout << "4. Get media data by media ID\n";
        std::cout << "5. Get all media metadata by media ID\n";
        std::cout << "0. Exit\n";
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        try {
            switch (choice) {
            case 1: {
                auto collections = dbHandler.getAllCollections();
                std::cout << "\nCollections:\n";
                for (const auto& collection : collections) {
                    std::cout << collection << "\n";
                }
                break;
            }
            case 2: {
                std::cout << "Enter collection ID: ";
                std::cin >> input;
                std::string collectionTitle = dbHandler.getCollectionById(input);
                std::cout << "Collection Title: " << collectionTitle << "\n";
                break;
            }
            case 3: {
                std::cout << "Enter collection ID: ";
                std::cin >> input;
                auto mediaIds = dbHandler.getAllMediaIdsFromCollection(input);
                std::cout << "\nMedia IDs in collection:\n";
                for (const auto& mediaId : mediaIds) {
                    std::cout << mediaId << "\n";
                }
                break;
            }
            case 4: {
                std::cout << "Enter media ID: ";
                std::cin >> input;
                std::string mediaData = dbHandler.getMediaDataById(input);
                std::cout << "\nMedia Data:\n" << mediaData << "\n";
                break;
            }
            case 5: {
                std::cout << "Enter media ID: ";
                std::cin >> input;
                auto metadataEntries = dbHandler.getAllMediaMetadataByMediaId(input);
                std::cout << "\nMedia Metadata:\n";
                for (const auto& entry : metadataEntries) {
                    std::cout << entry << "\n";
                }
                break;
            }
            case 0:
                return;
            default:
                std::cout << "Invalid choice. Try again.\n";
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }
}

int main() {
    DatabaseHandler dbHandler("../ghost.db");
    testDatabaseHandler(dbHandler);
    return 0;
}
