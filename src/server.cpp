#include "server.h"
#include <stdexcept>   // for exception, runtime_error, out_of_range

using namespace std;

//Actually allocate clients
vector<Client> Server::clients;

Server::Server() {

  //Initialize static mutex from MyThread
  MyThread::InitMutex(); {
    //1. 소켓을 만든다 // 에러 처리 필요 (socket 이 -1 return 할 경우 감안)
    // 에러 처리 exception 적용 (https://stdcxx.apache.org/doc/stdlibug/18-4.html)

    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0){
      throw std::runtime_error ("serverSock returned -1"); 
    }
    
    //2. For setsock opt (REUSEADDR)//(optional) 만들어진 소켓에 추가적인 설정 
    //Avoid bind error if the socket was not close()'d last time;
    int yes = 1;
    int setsocketopt_result = setsockopt(serverSock,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
    if(setsocketopt_result < 0){
      throw std::runtime_error ("setsockopt returned -1"); 
    }

    //3. 메모리 clear - 0채워줌
    memset(&serverAddr, 0, sizeof(sockaddr_in));
    
    //4. 만들어진 소켓에 주소 바인딩
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);
    int bind_result=bind(serverSock, (struct sockaddr *) &serverAddr, sizeof(sockaddr_in));
    if (bind_result < 0){
      throw std::runtime_error ("bind returned -1"); 
    }
  
    //5. 연결된 주소에서 접속 대기
    int listen_result = listen(serverSock, 5);
    if (listen_result < 0){
      throw std::runtime_error ("listen returned -1"); 
    }
  };
};
/*
	AcceptAndDispatch();

	Main loop:
		Blocks at accept(), until a new connection arrives.
		When it happens, create a new thread to handle the new client.
*/
void Server::AcceptAndDispatch() {
  
  Client *c;
  MyThread *t;

  socklen_t cliSize = sizeof(sockaddr_in); 
  while(1) {

    c = new Client(); //이들도 delete 되어야 하는데 안 되어있다.
	  t = new MyThread();

	  //Blocks here;
	  // clientAddr 선언도 이동하는것이 나음
    c->sock = accept(serverSock, (struct sockaddr *) &clientAddr, &cliSize);

	  if(c->sock < 0) {
      throw std::runtime_error ("accept returned -1"); 
	    //cerr << "Error on accept";
	  }
	  else {
	    t->Create((void *) Server::HandleClient, c); //실행중인 thread(들)에 대해 모두 join 한 뒤에 t가 delete 되어야 한다.
	  }
  }
}

//Static
void *Server::HandleClient(void *args) {

  //Pointer to accept()'ed Client
  Client *c = (Client *) args;
  char buffer[256-25], message[256];
  int index;
  int n;

  //Add client in Static clients <vector> (Critical section!)
  MyThread::LockMutex((const char *) c->name);
   // lock된 구간이 짧은 것이 성능에중요.
    {
      //Before adding the new client, calculate its id. (Now we have the lock)
    c->SetId(Server::clients.size()); //id를 ip로 할수도 있다 (바꿔보기)
      sprintf(buffer, "Client n.%d", c->id);
      c->SetName(buffer);
      cout << "Adding client with id: " << c->id << endl;
      Server::clients.push_back(*c);
    }
  MyThread::UnlockMutex((const char *) c->name);

  while(1) {
    memset(buffer, 0, sizeof buffer);
    n = recv(c->sock, buffer, sizeof buffer, 0);

    //Client disconnected?
    if(n == 0) {
      cout << "Client " << c->name << " diconnected" << endl;
      close(c->sock);
      
      //Remove client in Static clients <vector> (Critical section!)
      MyThread::LockMutex((const char *) c->name);

        index = Server::FindClientIndex(c);
        cout << "Erasing user in position " << index << " whose name id is: " 
	  << Server::clients[index].id << endl;
        Server::clients.erase(Server::clients.begin() + index);

      MyThread::UnlockMutex((const char *) c->name);

      break;
    }
    else if(n < 0) {
      cerr << "Error while receiving message from client: " << c->name << endl;
    }
    else {
      //Message received. Send to all clients.
      snprintf(message, sizeof message, "<%s>: %s", c->name, buffer); 
      cout << "Will send to all: " << message << endl;
      Server::SendToAll(message);
    }
  }

  //End thread
  return NULL;
}

void Server::SendToAll(char *message) {
  int n;

  //Acquire the lock
  MyThread::LockMutex("'SendToAll()'");
 
    for(size_t i=0; i<clients.size(); i++) {
      n = send(Server::clients[i].sock, message, strlen(message), 0);
      cout << n << " bytes sent." << endl;
    }
   
  //Release the lock  
  MyThread::UnlockMutex("'SendToAll()'");
}

void Server::ListClients() {
  for(size_t i=0; i<clients.size(); i++) {
    cout << clients.at(i).name << endl;
  }
}

/*
  Should be called when vector<Client> clients is locked!
*/
int Server::FindClientIndex(Client *c) {
  for(size_t i=0; i<clients.size(); i++) {
    if((Server::clients[i].id) == c->id) return (int) i;
  }
  cerr << "Client id not found." << endl;
  return -1;
}
