#include <iostream>
#include "mythread.h"
#include "server.h"
#include <stdexcept> 

using namespace std;

int main() {
  cout << "Running!" << endl;

  Server *s;
  try{
    s = new Server();
  }
  catch(std::runtime_error &e){
    cout << "Server exception occurred: " << e.what() << endl;
  }
  //Main loop
  try {
    s->AcceptAndDispatch();
  }
  catch(std::runtime_error &e){
    cout << "AcceptAndDispatch exception occurred: " << e.what() << endl;
  }
  return 0;
}
