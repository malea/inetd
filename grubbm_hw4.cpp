/* Network Programming Homework 4: MALEA GRUBB*/

#include <string.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <map>
#include <vector>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>

using namespace std;

/* Helper function that splits a line into a port and then command */
vector<string> tokenize(string &str) {
  vector<string> tokens;
  int start = 0;
  int start_command;
  string command = "";
  for (int i = 0; i < str.size(); i++) {
    if (str[i] != ' ') {
      continue;
    }
    // push back the port number
    tokens.push_back(str.substr(start, i));
    start_command = i + 1;
    break;
  }
  // now push back the command (eliminate the starting space)
  for (int i = start_command; i < str.size(); i++) {
    command += str[i];
  }
  tokens.push_back(command);
  return tokens;
}

/* Helper function that parses the config file and puts it into a map
   with the port as a key and the command as value */
void parse_config(map<string,string> &config_entries, const char* filename) {
  ifstream config_file;
  string line;
  config_file.open(filename);
  // check to be sure the configuration file opened successfully
  if (!config_file.is_open()) {
    cerr << "open config file failed" << endl;
    exit(1);
  }
  while (getline(config_file,line)) {
    vector<string> tokens = tokenize(line);
    config_entries[tokens[0]] = tokens[1];
  }
}

/* Helper function that handles the client */
void handle_client(int peer_sockfd, const struct sockaddr_in6 *peer_address

/* MAIN */
int main(int argc, char *argv[]) {
  map<string,string> config_entries;

  // ensure program is given correct number of arguments
  if (argc != 2) {
    cerr << "Wrong number of Arguments! Please supply a config file!" << endl;
    return EXIT_FAILURE;
  }

  // parse the config file
  parse_config(config_entries, argv[1]);  
 
   
  return EXIT_SUCCESS;
}
