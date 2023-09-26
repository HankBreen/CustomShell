// Copyright Hank Breen 2023
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <utility>
#include <vector>
#include <algorithm>
#include <numeric>
#include <unordered_map>
#include <unistd.h>
#include <sys/wait.h>
#include <boost/asio.hpp>


using namespace std;
using namespace std::string_literals;
using StrVec = std::vector<std::string>;

// Convenience namespace declarations to streamline the code below
using namespace boost::asio;
using namespace boost::asio::ip;
using namespace std;

// method headers
void process();
StrVec splitCSV(std::string& line);
void myExec(StrVec argList);
int forkNexec(const StrVec& argList);
void runGen(StrVec args, std::string& line);
int runPara(StrVec args, std::string& line);
void ifPara(StrVec args, std::string& line);
void ifSer(StrVec args, std::string& line);
int wait(const int& childPid);
void setupDownload(const std::string& hostName, const std::string& path,
    tcp::iostream& data);
void getStream(std::string url, tcp::iostream& in);
size_t getNIndex(std::string str, char c, size_t N);



/**
 * The main function that calls the process method to handle the main
 * functionality of this program.
 *
 * \param[in] argc The number of command-line arguments.  
 *
 * \param[in] argv The actual command-line argument.
 */
int main(int argc, char *argv[]) {
    // create main loop
    process();
}

/**
 * Method with main loop to process the user input
 */
void process() { 
    const std::string& prompt = "> ";
    // Adapt the following loop as you see fit 
    std::string line; 
    while (std::cout << prompt, std::getline(std::cin, line), line != "exit") { 
        if (!line.empty()) {
            // get the input line into the string vector
            StrVec args = splitCSV(line);

            // if it needs to be run in serial
            if (args[0] == "SERIAL") {
                ifSer(args, line);

            // or if its being run in parallel
            } else if (args[0] == "PARALLEL") {
                ifPara(args, line);
            } else {
                // or if its just a regular case
                runGen(args, line);
            }
        }
    } 
}

/**
 * base processing method, runs each command and generates output.
 * @param args the vector of arguments on the input line
 * @param line the base string of the input line.
 */
void runGen(StrVec args, std::string& line) {
// as long as the first argument is not a pound or new line
    if (!line.empty()) {
        std::string comm = "";
        // no pound sign or new line
        if (args[0] != "#" && args[0] != "\n") {
            std::cout << "Running: ";
            // output the running command with no extra spaces
            for (std::string s : args) {
                comm += s + " ";
            }
            comm = comm.substr(0, comm.size() - 1);
            std::cout << comm << std::endl;
            int child = forkNexec(args);
            std::cout << "Exit code: " << wait(child) << std::endl;
        }
    }
}

/**
 * Method to run the parallel processes call.
 * @param args the vector of arguments on the input line.
 * @param line the base string of the input line.
 * @return an integer which is the childpid of process.
 */
int runPara(StrVec args, std::string& line) {
    // as long as the first argument is not a pound or new line
    if (!line.empty()) {
        std::string comm = "";
        if (args[0] != "#" && args[0] != "\n") {
            std::cout << "Running: ";
            for (std::string s : args) {
                comm += s + " ";
            }
            comm = comm.substr(0, comm.size() - 1);
            std::cout << comm << std::endl;
            // return the childPid of the command
            return forkNexec(args);
        }
    }
    return 0;
}


/**
 * Method to run the url commands in parallel.
 * @param args the vector of arguments on the input line.
 * @param line the base string of the input line.
 */
void ifPara(StrVec args, std::string& line) {
    // we have the argument vector, as well as the entire line
    // so the url should be the second item in the vector
    std::string url = args[1];

    // now we need to access the data in the url
    int secBack = getNIndex(url, '/', 2);
    int thirdBack = getNIndex(url, '/', 3);
    std::string firstUrl = url.substr(secBack + 1, thirdBack - (secBack + 1));
    std::string secondUrl = url.substr(thirdBack, url.size() - thirdBack);

    tcp::iostream in;
    setupDownload(firstUrl, secondUrl, in);

    // in object should be tied to url now, so we can use standard streaming
    // loop through and get each line, calling the splitCSV method on each
    // one and then the runGen method.
    std::string str;
    std::vector<int> pids;
    // skip until the empty line
    while (std::getline(in, str) && !str.empty() && (str != "\r")) {}

    while (std::getline(in, str)) {
        // if the string is not empty
        if (!str.empty()) {
            StrVec args2 = splitCSV(str);
            if (args2[0] != "#") {
                pids.push_back(runPara(args2, str));
            }
        }
    }
    // now call wait method for each method call to forkNexec
    // in the pids vector, which is the exit code
    for (int i = pids.size() - 1 ; i >= 0 ; i--) {
        std::cout << "Exit code: " << wait(pids[i]) << std::endl;
    }
}

/**
 * Method to run the url commands in serial.
 * @param args the vector of arguments on the input line.
 * @param line the base string of the input line.
 */
void ifSer(StrVec args, std::string& line) {
    // we have the argument vector, as well as the entire line
    // so the url should be the second item in the vector
    std::string url = args[1];

    // now we need to access the data in the url
    int secBack = getNIndex(url, '/', 2);
    int thirdBack = getNIndex(url, '/', 3);
    std::string firstUrl = url.substr(secBack + 1, thirdBack - (secBack + 1));
    std::string secondUrl = url.substr(thirdBack, url.size() - thirdBack);

    tcp::iostream in;
    setupDownload(firstUrl, secondUrl, in);

    // in object should be tied to url now, so we can use standard streaming
    // loop through and get each line, calling the splitCSV method on each
    // one and then the runGen method.
    std::string str;

    // skip until the empty line
    while (std::getline(in, str) && !str.empty() && (str != "\r")) {}

    while (std::getline(in, str)) {
        // if the string is not empty
        if (!str.empty()) {
            StrVec args2 = splitCSV(str);
            runGen(args2, str);
        }
    }
}


/**
 * Helper method to read the command line arguments into
 * the needed vector.
 * 
 * @param line the string of command line arguments.
 * @return a StrVec of arguments passed by the terminal.
 */
StrVec splitCSV(std::string& line) {
// Convenience stream to read from a string
std::istringstream is(line);
// The vector to be returned
StrVec retVal; 

// Use the usual pattern to read
for (std::string word ; is >> std::quoted(word);) {
    retVal.push_back(word);
}

// return string vector
return retVal;
}

/**
 * Method to execvp system call for process.
 * @param argList the argument list vector.
 */
void myExec(StrVec argList) {
    std::vector<char*> args;    // list of pointers to args
    for (auto& s : argList) {
        args.push_back(&s[0]);  // address of 1st character
    }
    // nullptr is very important
    args.push_back(nullptr);
    // Make execvp system call to run desired process
    execvp(args[0], &args[0]);
    // In case execvp ever fails, we throw a runtime execption
    throw std::runtime_error("Call to execvp failed!");
}


/**
 * Method to call fork function and give childpid.
 * @param argList the vector of arguments.
 * @return an int which is the childpid.
 */
int forkNexec(const StrVec& argList) {
    int childPid = fork();
    if (childPid == 0) {
        myExec(argList);
    } else {
        return childPid;
    }
    return 0;
}


/**
 * method which uses waitpid call to get
 * exitCode.
 * @param childPid the childPid.
 * @return the exit code int.
 */
int wait(const int& childPid) {
    int exitCode;
    waitpid(childPid, &exitCode, 0);
    return exitCode;
}

/**
 * Helper method to setup a TCP stream for downloading data from an
 * web-server.
 * 
 * @param host The host name of the web-server. 
 *
 * @param path The path to the file being download.  
 *
 * @param socket The TCP stream (aka socket) to be setup by this
 * method. 
 *
 * @param port An optional port number. The default port number is "80"
 *
 */
void setupDownload(const std::string& hostName, const std::string& path,
                   tcp::iostream& data) {
    // Create a boost socket and request the log file from the server.
    const std::string& port = "80";
    data.connect(hostName, port);
    data << "GET "   << path     << " HTTP/1.1\r\n"
         << "Host: " << hostName << "\r\n"
         << "Connection: Close\r\n\r\n";
}


 /** Helper method to get the Nth occurence of
 * a character in a string
 * @param str the string to search
 * @param c the char to look for
 * @param N the number of chars to pass
 * @return a size_t containing the index of the char
 * */
size_t getNIndex(std::string str, char c, size_t N) {
    // counter
    size_t count = 0;

    // go through and count each time it reads the
    // char, then return the index once it reaches count.
    for (size_t i = 0; i < str.length(); i++) {
        if (str[i] == c) {
            count++;
        }
        if (count == N) {
            return i;
        }
    }
    return 0;
}



// End of source code
