# Network Programming Homework 4: Malea Grubb

## Compilation Instructions

    g++ grubbm_hw3.cpp -std=c++11 -g

## To Run

    ./a.out CONFIG_FILE_PATH 

## Notes

  I wrote this homework to handle the case where a port can have multiple commands issued on
  it. It still works as expected when only one command is issued per port, but just wanted to
  clarify my design choices. I was not sure if we were supposed to handle this case or not, but I
  did, just in case. Also, I print out accepted connections to stdout (was useful for debugging).
