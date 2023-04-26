#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <tuple>
#include <sys/stat.h>
#include <cerrno>
#include <regex>
#include <functional>
#include <algorithm>

class FileReader{
public:
    // The constructor sets the file name
    FileReader(std::string filename){
        _filename = filename; 
    }

    // Return the size of the file
    long Size(){
        struct stat st;
        if (stat(_filename.c_str(), &st) == 0) {
            return st.st_size;
        } else std::perror("FileReader construtor");
        return -1;
    }
    
    // Return the name of the file
    std::string Name(){
        return _filename;
    }

    /* Open the file and store the raw data in a vector */
    std::vector<char> Raw(){
        std::ifstream inFile(_filename, std::ios::binary);       // Open file

        if(inFile.is_open()){
            std::vector<char> data((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
            inFile.close();
            return data;
        }

        std::perror("FileReader getRawData");
        std::vector<char> empty;
        return empty;
    }

    std::vector< std::tuple<std::string, std::string, char>> Parse(std::vector< std::tuple<std::string, std::string, char>>(*callback)(std::string)){
        std::vector<char> rawData = Raw();

        if (rawData.empty()) {
            std::cerr << "No raw data to parse" << std::endl;
            std::vector< std::tuple<std::string, std::string, char>> empty;
            return empty;
        }

        std::string dataStr(rawData.begin(), rawData.end());
        return callback(dataStr);
    }


private:
    std::string _filename;
    
};

std::vector< std::tuple<std::string, std::string, char>> parseTxt( std::string dataStr) {
    std::cout << "*  Text file data doesn't need to be parsed" << std::endl;
    std::cout << dataStr << std::endl << std::endl;

    std::vector< std::tuple<std::string, std::string, char>> empty;
    return empty;
}

std::vector< std::tuple<std::string, std::string, char>> parseBin( std::string dataStr) {
    std::cout << "*  Binary file data can't be parsed using regex" << std::endl;
    std::cout << dataStr << std::endl << std::endl;
    std::vector< std::tuple<std::string, std::string, char>> empty;
    return empty;
}

std::vector< std::tuple<std::string, std::string, char>> parseCSV( std::string dataStr) {
    /*
    std::cout << "*  Raw CSV Data" << std::endl;
    std::cout << dataStr << std::endl;
    */

    std::cout << "*  CSV Text Data" << std::endl;
    std::istringstream dataStream(dataStr);
    std::regex pattern("[^,]*");
    std::string line;       // Stores a line of CSV data

    std::string first, second, third;       // Temporary strings to store CSV elements in
    char delim = ',';
    std::tuple<std::string, std::string, char> csvTuple;        // Declaring Tuple
    std::vector< std::tuple<std::string, std::string, char>> storeCSV;      // Vector of tuples to store CSV data

    while (std::getline(dataStream, line)) {
        // std::cout << line << std::endl;             // Print the line

        std::istringstream lineStream(line);

        std::getline(lineStream, first, delim);
        std::getline(lineStream, second, delim);
        std::getline(lineStream, third, delim);
        storeCSV.push_back(make_tuple(first, second, third[0]));
    }

    return storeCSV;
}

void printVectorOfTuples(std::vector< std::tuple<std::string, std::string, char>> storeCSV) {
    for (auto i = storeCSV.begin(); i != storeCSV.end(); ++i) {
        std::cout << std::get<0>(*i) << ' ' << std::get<1>(*i) << ' ' << std::get<2>(*i) << std::endl;
    }
}

// Define the custom predicate function
bool matchTuple(const std::tuple<std::string, std::string, char>& tuple1, const std::tuple<std::string, std::string, char>& tuple2, int tupleIndex) {
    switch (tupleIndex){            
    case 0:         // Compare the first field of the tuples
        return std::get<0>(tuple1) == std::get<0>(tuple2);
        break;
    case 1:         // Compare the second field of the tuples
        return std::get<1>(tuple1) == std::get<1>(tuple2);
        break;
    case 2:         // Compare the third field of the tuples
        return std::get<2>(tuple1) == std::get<2>(tuple2);
        break;
    default:
        return false;
        break;
    }
}

/** Searches a vector of tuples for a match. 
  *  Takes in a tuple of type <std::string, std::string, std::char> and returns a char for the matching tuple
*/
char searchVector(
std::vector< std::tuple<std::string, std::string, char>> storeCSV, 
std::tuple<std::string, std::string, char> searchTuple,
int pos) {  
    auto it = std::search(storeCSV.begin(), storeCSV.end(), &searchTuple, &searchTuple + 1, [&](const auto& tuple1, const auto& tuple2) {
        return matchTuple(tuple1, tuple2, pos);
        });
    if (it != storeCSV.end()) {
        //std::cout << "  Match Found in tuple: " << "< " << std::get<0>(*it) << ' ' << std::get<1>(*it) << ' ' << std::get<2>(*it) << " >" << std::endl;
        return std::get<2>(*it);
    }
    else {
        //std::cout << "  No Match Found :( " << std::endl;
        return ' ';
    }   
}

/** Punch Card decription class
    
*/
class Decryptor {
public:
    Decryptor(std::vector<char> punchCardData, std::vector< std::tuple<std::string, std::string, char>> storeCSV) {       // Constructor
        std::string tempStr(punchCardData.begin(), punchCardData.end());
        _punchCardDataStr = tempStr;
        _ebdicCodes = storeCSV;

        std::string line;
        std::istringstream punchCardDataStream(_punchCardDataStr);
        std::vector<std::string> translate;
        int punchcounter = 1;

        std::getline(punchCardDataStream, line);        // Discarding the first line of the file

        while (std::getline(punchCardDataStream, line)) {
            if (line.find("--") != std::string::npos) {
                // reached the end of a card
                std::cout << "Punchcard " << punchcounter << " :\t";
                processPunchCard(translate);
                std::cout << std::endl;
                translate.clear();
                punchcounter++;
            }
            else {
                translate.push_back(line);              // add the line to the translation vector                
            }
        }
    }

    // Read the punch card and convert it into human readable characters
    void processPunchCard(std::vector<std::string> vec) {
        // Iterate through the punch card column by column
        for (int col = 0; col < 80; col++) {
            std::string code;
            for (auto it = vec.begin(); it != vec.end(); it++) {
                std::string line = *it;
                
                if (line[col] == '1') {                             // If there is a '1', there is a punch
                    int index = std::distance(vec.begin(), it);
                    //std::cout << index << " ";
                    code.append(convert(index));
                    
                } /*else {                                            // Otherwise do nothing
                    std::cout << " "  << " ";
                }*/
            }
            //std::cout << "\t" << code;

            std::tuple<std::string, std::string, char> codeTuple = std::make_tuple(code, "", '\0');
            char temp = searchVector(_ebdicCodes, codeTuple, 0);
            //std::cout << "\t" << temp;
            std::cout << temp;            
        }
    }
    
    std::string convert(int index) {
        switch (index) {
        case 0:
            return "Y";
        case 1:
            return "X";
        case 2:
            return "0";
        case 3:
            return "1";
        case 4:
            return "2";
        case 5:
            return "3";
        case 6:
            return "4";
        case 7:
            return "5";
        case 8:
            return "6";
        case 9:
            return "7";
        case 10:
            return "8";
        case 11:
            return "9";
        
        default:
            return "Z";
        }
    }

    void printVector(std::vector<std::string> vec) {
        for (auto it = vec.begin(); it != vec.end(); it++) {
            std::cout << *it << std::endl;
        }
        std::cout << std::endl;
    }

private:
    std::string _punchCardDataStr;
    std::vector< std::tuple<std::string, std::string, char>> _ebdicCodes;
};



int main(int argc, const char * argv[]) {
    // Declaring stuff here
    std::string textFile = "Encrypt.txt";
    std::string binaryFile = "Morse.bin";
    std::string csvFile = "Tuple.csv";
    
    std::cout << std::endl;
    std::cout <<  "*****************************************" << std::endl;
    std::cout <<  "*\tFile Reader\t\t\t*" << std::endl;
    std::cout <<  "*****************************************" << std::endl << std::endl;

    std::cout <<  "Handling Text file" << std::endl;
    std::cout <<  "*****************************************" << std::endl;
    FileReader textFileTest(textFile);
    std::cout << "*  " << "Name of  the file is"<<" "<< textFileTest.Name() << std::endl;
    std::cout << "*  "  <<"Size of the file is"<<" "<< textFileTest.Size() <<" "<<"bytes" << std::endl;
    std::vector< std::tuple<std::string, std::string, char>> txt = textFileTest.Parse(parseTxt);       // Parse data
    
    std::cout <<  "Handling Binary file" << std::endl;
    std::cout <<  "*****************************************" << std::endl;
    FileReader binaryFileTest(binaryFile);
    std::cout << "*  " << "Name of the file is"<<" "<< binaryFileTest.Name() << std::endl;
    std::cout << "*  "  <<"Size of the file is"<<" "<< binaryFileTest.Size() <<" "<<"bytes" << std::endl;
    std::vector< std::tuple<std::string, std::string, char>> bin = binaryFileTest.Parse(parseBin);       // Parse data

    std::cout <<  "Handling CSV file" << std::endl;
    std::cout <<  "*****************************************" << std::endl;
    FileReader csvFileTest(csvFile);
    std::cout << "*  " << "Name of the file is"<<" "<< csvFileTest.Name() << std::endl;
    std::cout << "*  "  <<"Size of the file is"<<" "<< csvFileTest.Size() <<" "<<"bytes" << std::endl;
    std::vector< std::tuple<std::string, std::string, char>> storeCSV = csvFileTest.Parse(parseCSV);       // Parse data

    for (auto i = storeCSV.begin(); i != storeCSV.end(); ++i) {
        std::cout << std::get<0>(*i) << ' ' << std::get<1>(*i) << ' ' << std::get<2>(*i) << std::endl;
    }

    // Vector Search test
    std::cout << std::endl << "Testing vector search" << std::endl;

    std::tuple<std::string, std::string, char> testTuple1 = std::make_tuple("", "25", '\0');        // Line 32
    std::tuple<std::string, std::string, char> testTuple2 = std::make_tuple("", "none", '\0');      // Line 58 
    std::tuple<std::string, std::string, char> testTuple3 = std::make_tuple("nothing", "", '\0');
    std::tuple<std::string, std::string, char> testTuple4 = std::make_tuple("X3", "", '\0');        // Line 23
    std::tuple<std::string, std::string, char> testTuple5 = std::make_tuple("", "", '#');           // Line 40

    char tempChar;
    tempChar = searchVector(storeCSV, testTuple1, 1);
    tempChar = searchVector(storeCSV, testTuple2, 1);
    tempChar = searchVector(storeCSV, testTuple3, 2);
    tempChar = searchVector(storeCSV, testTuple4, 0);
    tempChar = searchVector(storeCSV, testTuple5, 2);

    std::cout << std::endl;

    /* Punch card stuff */
    std::string punchCards = "pumchCards.txt";
    FileReader punchCardReader(punchCards);             // File Reader for punch card text data
    std::vector<char> punchCardData = punchCardReader.Raw();

    Decryptor decrypt(punchCardData, storeCSV);

    return 0;
}
