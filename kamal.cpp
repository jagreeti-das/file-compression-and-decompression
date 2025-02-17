#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <algorithm>
#include <limits>
#include <conio.h>
#include <regex>
#include <sstream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

using namespace std;

// User structure
struct User
{
    int id;
    string name;
    string role;
    string password;
};

class FileManagementSystem
{
private:
    string userFile = "users.txt";
    map<int, User> users;
    char encryptionKey = 0xAA;

    // Static admin credentials
    const int ADMIN_ID = 1;
    const string ADMIN_PASSWORD = "admin123";

    // Helper functions
    string toLower(const string &str)
    {
        string result = str;
        transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    string trim(const string &str)
    {
        size_t first = str.find_first_not_of(" \t\n\r");
        size_t last = str.find_last_not_of(" \t\n\r");
        return (first == string::npos || last == string::npos) ? "" : str.substr(first, last - first + 1);
    }

    string getPassword(const string &prompt)
    {
        string password;
        char ch;

        cout << prompt;
        while ((ch = _getch()) != '\r')
        {
            if (ch == '\b')
            {
                if (!password.empty())
                {
                    password.pop_back();
                    cout << "\b \b";
                }
            }
            else
            {
                password.push_back(ch);
                cout << '*';
            }
        }
        cout << endl;
        return password;
    }

    string xorEncryptDecrypt(const string &data, char key)
    {
        string result = data;
        for (size_t i = 0; i < data.size(); i++)
        {
            result[i] = data[i] ^ key;
        }
        return result;
    }

    bool isValidEmail(const string &email)
    {
        const regex emailRegex(R"((\w+)(\.{0,1})(\w*)@(\w+)(\.\w{2,})+)");
        return regex_match(email, emailRegex);
    }

    bool isValidPassword(const string &password)
    {
        const regex passwordRegex(R"((?=.*\d)(?=.*[@$!%*?&])[A-Za-z\d@$!%*?&]{6,})");
        return regex_match(password, passwordRegex);
    }

    bool isValidName(const string &name)
    {
        if (name.empty() || name.length() < 2)
            return false;
        return all_of(name.begin(), name.end(), [](char c)
                      { return isalpha(c) || isspace(c); });
    }

    void loadUsers()
    {
        ifstream file(userFile);
        if (!file.is_open())
        {
            cerr << "Error: Could not open user file!" << endl;
            return;
        }
        User user;
        while (file >> user.id)
        {
            file.ignore();
            getline(file, user.name);
            getline(file, user.role);
            getline(file, user.password);
            users[user.id] = user;
        }
        file.close();
    }

    void saveUsers()
    {
        ofstream file(userFile);
        if (!file.is_open())
        {
            cerr << "Error: Could not open user file for writing!" << endl;
            return;
        }
        for (const auto &pair : users)
        {
            const User &user = pair.second;
            file << user.id << "\n"
                 << user.name << "\n"
                 << user.role << "\n"
                 << user.password << "\n";
        }
        file.close();
    }

public:
    FileManagementSystem()
    {
        loadUsers();
    }

    void signup()
    {
        cout << "\n=== SIGNUP ===\n";
        User newUser;
        newUser.id = 0; // Initialize id to 0

        while (true)
        {
            if (newUser.id == 0)
            {
                // Generate a new unique ID
                newUser.id = users.empty() ? 2 : (users.rbegin()->first + 1);
            }

            if (newUser.name.empty())
            {
                do
                {
                    cout << "Enter Name (alphabets and spaces only, minimum 2 characters): ";
                    getline(cin >> ws, newUser.name);
                    newUser.name = trim(newUser.name);
                    if (!isValidName(newUser.name))
                    {
                        cout << "Invalid name. Please use only alphabets and spaces, with a minimum of 2 characters.\n";
                        newUser.name = "";
                    }
                } while (newUser.name.empty());
            }

            if (newUser.role.empty())
            {
                string email;
                cout << "Enter Email: ";
                cin >> email;
                if (!isValidEmail(email))
                {
                    cout << "Invalid email format. Please try again.\n";
                    continue;
                }
                newUser.role = "user";
            }

            if (newUser.password.empty())
            {
                string password = getPassword("Enter Password (min 6 characters, at least 1 digit, 1 special character): ");
                if (!isValidPassword(password))
                {
                    cout << "Invalid password. Please meet the requirements.\n";
                    continue;
                }
                newUser.password = xorEncryptDecrypt(password, encryptionKey);
            }

            break; // All fields are valid, exit the loop
        }

        users[newUser.id] = newUser;
        saveUsers();
        cout << "Signup successful! Your assigned ID is: " << newUser.id << "\n";
    }

    void login()
    {
        cout << "\n=== LOGIN ===\n";
        int id;
        string password;
        bool validInput = false;

        while (!validInput)
        {
            cout << "Enter ID: ";
            if (!(cin >> id))
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Please enter a number for ID.\n";
                continue;
            }

            password = getPassword("Enter Password: ");

            if (id == ADMIN_ID && password == ADMIN_PASSWORD)
            {
                cout << "Admin login successful!\n";
                adminMenu();
                return;
            }

            auto it = users.find(id);
            if (it == users.end())
            {
                cout << "User not found. Please try again.\n";
                continue;
            }

            if (xorEncryptDecrypt(it->second.password, encryptionKey) == password)
            {
                cout << "Login successful! Welcome, " << it->second.name << ".\n";
                userMenu(it->second.name);
                validInput = true;
            }
            else
            {
                cout << "Incorrect password. Please try again.\n";
            }
        }
    }

    void adminMenu()
    {
        int choice;
        do
        {
            cout << "\n=== ADMIN MENU ===\n"
                 << "1. Remove User\n"
                 << "2. View All Users\n"
                 << "3. Logout\n"
                 << "Enter choice: ";

            if (!(cin >> choice))
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Please enter a number.\n";
                continue;
            }

            switch (choice)
            {
            case 1:
                removeUser();
                break;
            case 2:
                viewAllUsers();
                break;
            case 3:
                cout << "Logging out from admin...\n";
                break;
            default:
                cout << "Invalid choice!\n";
            }
        } while (choice != 3);
    }

    void removeUser()
    {
        int id;
        cout << "Enter ID of the user to remove: ";
        cin >> id;

        if (id == ADMIN_ID)
        {
            cout << "Cannot remove admin account.\n";
            return;
        }

        if (users.erase(id) > 0)
        {
            cout << "User removed successfully.\n";
            saveUsers();
        }
        else
        {
            cout << "User not found.\n";
        }
    }

    void viewAllUsers()
    {
        cout << "\n=== ALL USERS ===\n";
        cout << "ID: " << ADMIN_ID << ", Name: Admin, Role: admin\n";
        for (const auto &pair : users)
        {
            const User &user = pair.second;
            cout << "ID: " << user.id << ", Name: " << user.name << ", Role: " << user.role << endl;
        }
    }

    void userMenu(const string &name)
    {
        int choice;
        do
        {
            cout << "\n=== USER MENU ===\n"
                 << "1. Compress Text\n"
                 << "2. Decompress Text\n"
                 //  << "3. Compress Image\n"
                 << "3. Change File Extension\n"
                 << "4. Logout\n"
                 << "Enter choice: ";

            if (!(cin >> choice))
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Please enter a number.\n";
                continue;
            }

            switch (choice)
            {
            case 1:
                compressText();
                break;
            case 2:
                decompressText();
                break;
            // case 3:
            //     compressImage();
            //     break;
            case 3:
                changeFileExtension();
                break;
            case 4:
                cout << "Logout(back to main menu) \n";
                break;
            default:
                cout << "Invalid choice!\n";
            }
        } while (choice != 4);
    }

    void compressText()
    {
        string inputFileName, outputFileName;
        cout << "Enter the input file name: ";
        cin >> inputFileName;

        string content;
        ifstream file(inputFileName);
        if (!file.is_open())
        {
            cerr << "Error opening file: " << inputFileName << endl;
            return;
        }
        ostringstream ss;
        ss << file.rdbuf(); // read the buffer file
        content = ss.str();
        file.close();

        ostringstream compressed;
        int count = 1;
        for (size_t i = 0; i < content.length(); i++)
        {
            if (i + 1 < content.length() && content[i] == content[i + 1])
            {
                count++;
            }
            else
            {
                compressed << content[i] << count;
                count = 1;
            }
        }
        outputFileName = inputFileName + ".compressed";
        ofstream outputFile(outputFileName);
        outputFile << compressed.str();
        outputFile.close();
        cout << "File compressed successfully to " << outputFileName << endl;
    }

    void decompressText()
    {
        string inputFileName, outputFileName;
        cout << "Enter the compressed file name: ";
        cin >> inputFileName;

        ifstream inputFile(inputFileName);
        outputFileName = inputFileName + ".decompressed";
        ofstream outputFile(outputFileName);

        if (!inputFile.is_open() || !outputFile.is_open())
        {
            cerr << "Error: Could not open files!" << endl;
            return;
        }

        char current;
        int count;
        while (inputFile >> current >> count)
        {
            outputFile << string(count, current);
        }
        inputFile.close();
        outputFile.close();
        cout << "File decompressed successfully to " << outputFileName << endl;
    }

    void compressImage()
    {
        string inputImagePath, outputImagePath;
        int quality = 80;     // Default quality
        int resizeFactor = 1; // Default resize factor (no resizing)

        cin.ignore(numeric_limits<streamsize>::max(), '\n'); // Clear input buffer

        cout << "Enter the path to the input image: ";
        getline(cin, inputImagePath);
        cout << "Enter the path to save the output image: ";
        getline(cin, outputImagePath);

        // chnage in the quality jpeg image based (1-100)
        cout << "Enter JPEG quality (1-100, default 80): ";
        string qualityInput;
        getline(cin, qualityInput);
        if (!qualityInput.empty())
        {
            quality = stoi(qualityInput); // convert a string that represents an integer into an actual integer type
        }

        // demension will be change baesd on resize factor 1 willl not chanage after that like 2 or 3 it divide the demension
        cout << "Enter resize factor (1 for no resizing, default 1): ";
        string resizeFactorInput;
        getline(cin, resizeFactorInput);
        if (!resizeFactorInput.empty())
        {
            resizeFactor = stoi(resizeFactorInput);
        }

        // Call the compressImage function
        if (!compressImage(inputImagePath, outputImagePath, quality, resizeFactor))
        {
            cerr << "Image compression failed." << endl;
        }
        else
        {
            cout << "Image compression succeeded!" << endl;
        }
    }

    bool compressImage(const string &inputImagePath, const string &outputImagePath, int quality, int resizeFactor = 1)
    {
        int width, height, channels;

        // Load the input image
        unsigned char *data = stbi_load(inputImagePath.c_str(), &width, &height, &channels, 0);
        if (!data)
        {
            cerr << "Error loading image: " << inputImagePath << endl;
            return false;
        }
        cout << "Image loaded: " << width << "x" << height << " with " << channels << " channels." << endl;

        // Resize the image if needed
        unsigned char *resizedData = data;
        int newWidth = width, newHeight = height;
        if (resizeFactor > 1)
        {
            newWidth = width / resizeFactor;
            newHeight = height / resizeFactor;

            resizedData = new unsigned char[newWidth * newHeight * channels];
            if (!stbir_resize_uint8(data, width, height, 0, resizedData, newWidth, newHeight, 0, channels))
            {
                cerr << "Error resizing image." << endl;
                stbi_image_free(data);
                delete[] resizedData;
                return false;
            }
            cout << "Image resized to: " << newWidth << "x" << newHeight << endl;
        }

        // Compress and save the image as JPEG
        if (!stbi_write_jpg(outputImagePath.c_str(), newWidth, newHeight, channels, resizedData, quality))
        {
            cerr << "Error writing compressed image: " << outputImagePath << endl;
            stbi_image_free(data);
            if (resizedData != data)
                delete[] resizedData;
            return false;
        }

        cout << "Compressed image saved to: " << outputImagePath << " with quality " << quality << endl;

        // Free memory
        stbi_image_free(data);
        if (resizedData != data)
            delete[] resizedData;

        return true;
    }

    void changeFileExtension()
    {
        string filePath, newExtension;
        cout << "Enter the file path (with current extension): ";
        cin >> filePath;

        cout << "Enter the new extension (e.g., .txt, .cpp): ";
        cin >> newExtension;

        size_t pos = filePath.rfind(".");
        if (pos == string::npos)
        {
            cout << "The file has no extension." << endl;
            return;
        }

        string newFileName = filePath.substr(0, pos) + newExtension;
        if (rename(filePath.c_str(), newFileName.c_str()) == 0)
        {
            cout << "File renamed to: " << newFileName << endl;
        }
        else
        {
            cout << "Error renaming file!" << endl;
        }
    }
};

int main()
{
    FileManagementSystem system;
    int choice;

    do
    {
        cout << "\n=== FILE MANAGEMENT SYSTEM ===\n"
             << "1. Signup \n"
             << "2. Login for existing user and admin\n"
             << "3. Exit\n"
             << "Enter choice: ";

        if (!(cin >> choice))
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Please enter a number.\n";
            continue;
        }

        switch (choice)
        {
        case 1:
            system.signup();
            break;
        case 2:
            system.login();
            break;
        case 3:
            cout << "Exiting...\n";
            return 0;
        default:
            cout << "Invalid choice! Please try again.\n";
        }
    } while (true);

    return 0;
}
