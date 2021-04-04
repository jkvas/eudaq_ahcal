#include <DesyTableCommunication.hh>


#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#include <winsock.h>
#include <io.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#include <stddef.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <thread>





DesyTableCommunication::DesyTableCommunication(std::string address, int port) {
   std::unique_lock<std::mutex> mt(communication_mutex);
   //TODO to be written for the windows. Only a stub here:
#ifdef _WIN32
   WSADATA wsaData;
   int wsaRet=WSAStartup(MAKEWORD(2, 2), &wsaData); //initialize winsocks 2.2
   if (wsaRet) {std::cout << "ERROR: WSA init failed with code " << wsaRet << std::endl; return;}
   std::cout << "DEBUG: WSAinit OK" << std::endl;
   _fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   if (_fd == INVALID_SOCKET) {
	   std::cout << "ERROR: invalid socket" << std::endl;
      WSACleanup;
      return;
   }
   //set the timeout to 100 ms

   int timeout = 100;//timeout in ms
   // unsigned int tosize = sizeof(timeout);
   int ret2=setsockopt(_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
   if (ret2 == SOCKET_ERROR) {
	   std::cout << "ERROR: timeout not set correctly" << std::endl;
   } else {
	   std::cout << "DEBUG: timeout set successfully" << std::endl;
   }
   std::cout << "DEBUG: Socket OK" << std::endl;
   struct sockaddr_in dstAddr; //win ok
   //??     memset(&dstAddr, 0, sizeof(dstAddr));
   dstAddr.sin_family = AF_INET;
   dstAddr.sin_port = htons(port);
   dstAddr.sin_addr.s_addr = inet_addr(address.c_str());

   int ret = connect(_fd, (struct sockaddr *) &dstAddr, sizeof(dstAddr));
   if (ret != 0) {
      throw std::runtime_error("Could not open TCP to the DESY stage");
   }
   std::cout << "DEBUG: Connect OK" <<std::endl;
#else
   std::cout << "DEBUG: initializating DesyTableCommunication" << std::endl;
   struct sockaddr_in dstAddr;
   memset(&dstAddr, 0, sizeof(dstAddr));
   dstAddr.sin_port = htons(port);
   dstAddr.sin_family = AF_INET;
   dstAddr.sin_addr.s_addr = inet_addr(address.c_str());
   _fd = socket(AF_INET, SOCK_STREAM, 0);
   std::cout << "DEBUG: DesyTableCommunication socket created" << std::endl;
   struct timeval tv; /* timeval and timeout stuff added by davekw7x */
   tv.tv_sec = 0; //set timeout in seconds
   tv.tv_usec = 100000; //set timeout in us
   if (setsockopt(_fd, SOL_SOCKET, SO_RCVTIMEO, (char *) &tv, sizeof tv)) {
      std::cout << "DEBUG: communication error 1" << std::endl;
      throw std::runtime_error("Could not set setsockopt in DesyTableCommunication::DesyTableCommunication");
   }
   std::cout << "DEBUG: DesyTableCommunication socket options set" << std::endl;
   int ret = connect(_fd, (struct sockaddr *) &dstAddr, sizeof(dstAddr));
   if (ret != 0) {
      std::cout << "DEBUG: desy table TCP socket initialization error. ret=" << ret << std::endl;
      throw std::runtime_error("Could not open TCP to the DESY stage");
   }
   std::cout << "DEBUG: Successfully opened a link to the DESY stage TCP server" << std::endl;
#endif //_WIN32
}

DesyTableCommunication::~DesyTableCommunication()
{
   //TODO put the controller to the local mode???
   std::cout << "destroying Communicating library" << std::endl;
   std::unique_lock<std::mutex> myLock(communication_mutex);
#ifdef _WIN32
   closesocket(_fd);
   WSACleanup;
#else
   close(_fd);
#endif
   _fd = 0;
   std::cout << "Connection of Communicating library closed" << std::endl;
}

int DesyTableCommunication::transmit(std::string command, uint8_t address) {
   std::unique_lock<std::mutex> myLock(communication_mutex);
   char sendBuff[512];
   int length = sprintf(sendBuff, "Z%02d%s\r", address, command.c_str());

   if (debugLevel >= 1) std::cout << "DEBUG: sending " << length << " bytes:" << sendBuff << std::endl;
#ifdef _WIN32
   size_t bytesWritten = send(_fd, sendBuff, length, 0);
#else
   size_t bytesWritten = write(_fd, sendBuff, length);
#endif
   if (bytesWritten < 0) {
      std::cout << "There was an error writing to the TCP socket" << std::endl;
   } else {
//      std::cout << "DEBUG: "<<bytesWritten << " out of " << length << " bytes is  written to the TCP socket" << std::endl;
      return bytesWritten;
   }
}

std::string DesyTableCommunication::receive() {
   char buf[recv_buf_length];
   int numbytes = recv(_fd, buf, recv_buf_length, 0);
   if (numbytes == -1) {
      std::cout << "ERROR? TODO received -1" << std::endl;
      return std::string("");
   }
   buf[numbytes] = '\0';
   std::cout << "read " << numbytes << "bytes. Message=\"" << std::string(buf) << "\"" << std::endl;
   return std::string(buf);
}

void DesyTableCommunication::setHysteresis(int value, uint8_t address) {
   //TODO it is a more complex task Better to set manually by setting 99 to P1 (enters the setup mode) and by changing the E9 setting to desired value
}

int DesyTableCommunication::getActualPosition(uint8_t address) {
   return requestValue(address, "H\r", "@V99");
}

std::string DesyTableCommunication::receive(int bytes, std::chrono::milliseconds timeout) {
   auto tp_start_read = std::chrono::steady_clock::now();
   char buf[recv_buf_length];
   int received = 0;
   buf[0] = 0;
   if (bytes > recv_buf_length) {
      std::cout << "ERROR: too big buffer requested: " << bytes << std::endl;
      return std::string("");
   }
   while ((received < bytes) && ((std::chrono::steady_clock::now() - tp_start_read) < timeout)) {
      if (debugLevel >= 3) std::cout << "DEBUG recv start (" << bytes - received << ")." << std::flush;
      int numbytes = recv(_fd, buf + received, bytes - received, 0);
      if (debugLevel >= 3) std::cout << "recv end (" << numbytes << ")." << std::endl;
      if (numbytes == -1) {
         std::cout << "TODO read -1" << std::endl;
         return buf;
      }
      buf[received + numbytes] = '\0';
      received += numbytes;
   }
   if (debugLevel >= 1) std::cout << "read finnished: " << received << "bytes. Message=\"" << std::string(buf) << "\"" << std::endl;
   return std::string(buf);
}

int DesyTableCommunication::trashRecvBuffer() {
   char buf[512];
   int received = 0;
   int total = 0;
   while (true) {
      received = recv(_fd, buf, 512, 0);
      if (received > 0) {
		  total += received;
         std::cout << "DEBUG: trashing " << received << " bytes." << std::endl;
      } else break;
   }
   return total;
}

int DesyTableCommunication::getDebugLevel() const {
   return debugLevel;
}

void DesyTableCommunication::setDebugLevel(int debugLevel = 0) {
   this->debugLevel = debugLevel;
}

int DesyTableCommunication::requestValue(uint8_t address, const char* command, const char* expectedResponse) {
   transmit(command, address);
   std::string returnString = receive(12, std::chrono::milliseconds(100));
   if (returnString.length() != 12) {
      std::cout << "ERROR: wrong length of response in requestValue(" << address << ")" << returnString.length() << ", \"" << returnString << "\"" << std::endl;
      return 1000000; //return an error. no number greater or lower than six "9" is allowed
   }
   //compare return 0 when comparison successfull
   if ((returnString.compare(0, 4, expectedResponse) == 0) && (returnString.compare(11, 1, "@") == 0)) {
      if (debugLevel > 2) std::cout << "DEBUG: return packet is as expected" << std::endl;
   } else {
      std::cout << "ERROR: wrong packet in getPosition(" << address << ")" << returnString << std::endl;
      return 1000000; //return an error
   }
   int value = std::stoi(returnString.substr(4, 7));
   if (debugLevel >= 1) std::cout << "DEBUG: position=" << value << std::endl;
   return value;
}

int DesyTableCommunication::getPresetP1(uint8_t address) {
   return requestValue(address, "K\r", "@V96");
}

int DesyTableCommunication::getPresetP5(uint8_t address) {
   return requestValue(address, "O\r", "@V92");
}

void DesyTableCommunication::takeOver(uint8_t address) {
	transmit("U\r", address);
}

std::string DesyTableCommunication::getControllerSetup(uint8_t address) {
   //   DEBUG: sending 6 bytes:Z01S
   //   "ead 78bytes. Message="T000600222222333333000000000100>040020303000100100100002500000000000200000000
   // TODO to be decoded according to the manual
	transmit("S\r", address);
   std::string answer = receive(78, std::chrono::milliseconds(10000));
   return answer;
}

void DesyTableCommunication::setActualPosition(const int value, const uint8_t address) {
   char sign = value < 0 ? '-' : '+';
   char buf[20];
   sprintf(buf, "A%c%06d\r", sign, abs(value));
   if (debugLevel > 2) std::cout << "DEBUG: set position string: \"" << buf << "\"" << std::endl;
   if (debugLevel > 0) std::cout << "DEBUG: sending command " << buf << " to address " << address << std::endl;
   transmit(std::string(buf), address);
   if (debugLevel > 0) std::cout << "DEBUG: sending command " << buf << " to address " << address << std::endl;
   takeOver(address);
}

void DesyTableCommunication::setPresetP1(int value, uint8_t address) {
   char sign = value < 0 ? '-' : '+';
   char buf[20];
   sprintf(buf, "B%c%06d\r", sign, abs(value));
   if (debugLevel > 2) std::cout << "DEBUG: set position string: \"" << buf << "\"" << std::endl;
   if (debugLevel > 0) std::cout << "DEBUG: sending command " << buf << " to address " << address << std::endl;
   transmit(std::string(buf), address);
   if (debugLevel > 0) std::cout << "DEBUG: sending command " << buf << " to address " << address << std::endl;
   takeOver(address);
}

void DesyTableCommunication::setPresetP5(int value, uint8_t address) {
   char buf[20];
   sprintf(buf, "F%06d\r", abs(value));
   if (debugLevel > 2) std::cout << "DEBUG: set position string: \"" << buf << "\"" << std::endl;
   if (debugLevel > 0) std::cout << "DEBUG: sending command " << buf << " to address " << address << std::endl;
   transmit(std::string(buf), address);
   if (debugLevel > 0) std::cout << "DEBUG: sending command " << buf << " to address " << address << std::endl;
   takeOver(address);
}

float DesyTableCommunication::getMmToBins() const {
   return mmToBins;
}

void DesyTableCommunication::setMmToBins(float mmToBins) {
   this->mmToBins = mmToBins;
}

float DesyTableCommunication::getActualPositionmm(const uint8_t address) {
   float pos = (1.0 / getMmToBins()) * getActualPosition(address);
   if (getDebugLevel() > 0) std::cout << "Actual position is " << pos << " mm" << std::endl;
   return pos;
}

float DesyTableCommunication::getPresetP1mm(const uint8_t address) {
   float pos = (1.0 / getMmToBins()) * getPresetP1(address);
   if (getDebugLevel() > 0) std::cout << "Actual P1 preset is " << pos << " mm" << std::endl;
   return pos;
}

float DesyTableCommunication::getPresetP5mm(const uint8_t address) {
   float pos = (1.0 / getMmToBins()) * getPresetP5(address);
   if (getDebugLevel() > 0) std::cout << "Actual P5 position is " << pos << " mm" << std::endl;
   return pos;
}

void  DesyTableCommunication::setActualPositionmm(const float value, const uint8_t address) {
   int rounded = (value >= 0) ? (int) (value * getMmToBins() + 0.5) : (int) (value * getMmToBins() - 0.5);
   setActualPosition(rounded, address);
}

void DesyTableCommunication::setPresetP1mm(const float value, const uint8_t address) {
   int rounded = (value >= 0) ? (int) (value * getMmToBins() + 0.5) : (int) (value * getMmToBins() - 0.5);
   setPresetP1(rounded, address);
}

void DesyTableCommunication::setPresetP5mm(const float value, const uint8_t address) {
   int rounded = (value >= 0) ? (int) (value * getMmToBins() + 0.5) : (int) (value * getMmToBins() - 0.5);
   setPresetP5(rounded, address);
}

void DesyTableCommunication::transmit(std::string command) {
}
