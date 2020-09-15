#include <iostream>
#include "mythread.h"
#include "server.h"

using namespace std;

int main() {
  cout << "Running!" << endl;

  Server *s;
  s = new Server(); // new 와 delete 는 쌍을 이뤄야한다.

  //Main loop
  s->AcceptAndDispatch();

  // added as new-delete pair
  delete s;
  
  return 0;
 
}
