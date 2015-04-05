/* Network Programming Homework 4: MALEA GRUBB */

#include <string.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <map>
#include <vector>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BACKLOG 8

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
   with the port as a key and a vector of commands as value. Using a vector
   for the command(s) will ensure that the case of a port having multiple commands
   issued does not fail */
void parse_config(map<string,vector<string>> &config_entries, const char* filename) {
  ifstream config_file;
  string line;
  config_file.open(filename);
  // check to be sure the configuration file opened successfully
  if (!config_file.is_open()) {
    cerr << "open config file failed" << endl;
    exit(1);
  }
  // for each line, place the port as key and add command to command vector in map
  while (getline(config_file,line)) {
    vector<string> tokens = tokenize(line);
    vector<string> tmp;
    config_entries.emplace(make_pair(tokens[0], tmp));
    config_entries[tokens[0]].push_back(tokens[1]);
  }
}

/* Helper function that uses SA_NOCLDWAIT flag to prevent turning child processes into
   zombie processes */
void setup_sa_nocldwait() {
  struct sigaction sa;
  memset(&sa, 0, sizeof(sa));

  // continue to use the default SIGCHLD handler
  sa.sa_handler = SIG_DFL;

  // prevent child processes being turned into zombies
  sa.sa_flags = SA_NOCLDWAIT;

  if (sigaction(SIGCHLD, &sa, NULL) != 0) {
    perror("sigaction");
    cerr << "warning: failed to set SA_NOCLDWAIT" << endl;
  }
}

/* Helper function that will create a listener for each port in the map of ports/commands */
vector<pair<int,string>> create_listeners(const map<string,vector<string>> &config_entries) {
  // vector of listener sockets that will be filled with a socket from each port
  vector<pair<int,string>> sockets;

  // set fields in bindaddr
  struct sockaddr_in6 bindaddr;
  memset(&bindaddr, 0, sizeof(bindaddr));
  bindaddr.sin6_family = AF_INET6;
  memcpy(&bindaddr.sin6_addr, &in6addr_any, sizeof(in6addr_any));

  // for each port in config
  for (auto itr = config_entries.begin(); itr != config_entries.end(); itr++) {
    // need port in integer form
    int port = atoi(itr->first.c_str());

    // create a listening socket
    int sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (sockfd == -1) {
      perror("socket");
      exit(1);
    }

    // bind the socket to the proper port
    bindaddr.sin6_port = htons(port);
    if (bind(sockfd, (struct sockaddr *)&bindaddr, sizeof(bindaddr)) != 0) {
      cout << port << endl;
      perror("bind");
      exit(1);
    }

    // listen
    if (listen(sockfd, BACKLOG) != 0) {
      perror("listen");
      exit(1);
    }
    
    // add socket to vector
    sockets.push_back(make_pair(sockfd,itr->first));
  }
  return sockets;
}

/* Helper function that will execute the command on the proper socket */
void handle_client(int peer_sockfd, const struct sockaddr_in6 *peer_addr, socklen_t peer_addrlen, string command) {
  char host[80];
  char svc[80];

  // get name info so we can see useful info if connection failed! Print out error in case of failure
  int ret = getnameinfo((struct sockaddr *)peer_addr, peer_addrlen, host, sizeof(host), svc, sizeof(svc), NI_NUMERICSERV);
  if (ret != 0) {
    fprintf(stderr, "getnameinfo() failed: %s\n", gai_strerror(ret));
  } 
  // I chose to include these, to verify accepted connections (was useful for debugging)
  else {
    fprintf(stdout, "[process %d] accepted connection from %s:%s\n", getpid(), host, svc);
  }

  // wait 2 seconds before executing command
  sleep(2);

  // redirect STDIN and STDOUT to socket
  dup2(peer_sockfd, STDIN_FILENO);
  dup2(peer_sockfd, STDOUT_FILENO);

  // execute command with system shell
  execl("/bin/sh", "/bin/sh", "-c", command.c_str(), NULL);
}

/* Helper function that will accept the connection, fork a child, then call the helper to
   execute the command */
void do_accept(int sockfd, string port, map<string,vector<string>> &config_entries) {
  int peer_sockfd;
  struct sockaddr_in6 src;
  socklen_t srclen;
  pid_t child;

  // accept the connection
  srclen = sizeof(src);
  peer_sockfd = accept(sockfd, (struct sockaddr *)&src, &srclen);
  if (peer_sockfd < 0) {
    perror("accept");
    return;
  }

  // look up all commands for a particular port (stored in a reference)
  vector<string> &commands = config_entries[port];

  //for each command fork a child process, and if successful, call helper to execute command
  for (string &command : commands) {
    child = fork();
    if (child == -1) {
      perror("fork");
    }
    if (child == 0) {
      // if fork successful, call helper function to execute the command
      handle_client(peer_sockfd, &src, srclen, command);
      exit(0);
    }
  } 
  // either the fork failed or in the parent. Close socket.
  close(peer_sockfd);
}

/* MAIN */
int main(int argc, char *argv[]) {
  map<string,vector<string>> config_entries;
  vector<pair<int,string>> listeners;
  fd_set fdset;
  int maxfd = 0;

  // set up SIGCHLD handler with SA_NOCLDWAIT (ensure no zombie children)
  setup_sa_nocldwait();

  // ensure program is given correct number of arguments
  if (argc != 2) {
    cerr << "Wrong number of Arguments! Please supply a config file!" << endl;
    return EXIT_FAILURE;
  }

  // parse the config file
  parse_config(config_entries, argv[1]);

  // start listening on each port
  listeners = create_listeners(config_entries); 

  // main loop
  while (true) {

    // initialize fdset
    FD_ZERO(&fdset);
    maxfd = 0;

    // handle each listener
    for (auto listener: listeners) {
      FD_SET(listener.first, &fdset);
      if (listener.first > maxfd) {
        maxfd = listener.first;
      }
    }
    
    // select 
    if (select(maxfd + 1, &fdset, NULL, NULL, NULL) == -1) {
      if (errno == EINTR) {
        continue;
      }
      else {
        perror("select");
        return EXIT_FAILURE;
      }
    }
    
    // loop through listeners and handle as they become ready
    for (auto listener: listeners) {
      if (FD_ISSET(listener.first, &fdset)) {
        do_accept(listener.first, listener.second, config_entries);
      }
    }
  }
  return EXIT_SUCCESS;
}
