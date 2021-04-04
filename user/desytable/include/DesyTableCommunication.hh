#ifndef DESYTABLECOMMUNICATION_HH
#define DESYTABLECOMMUNICATION_HH

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>

#ifdef _WIN32
#pragma comment(lib, "Ws2_32.lib")
#include <winsock.h>
#include <io.h>
#else
#endif

class DesyTableCommunication {
   public:
      DesyTableCommunication(std::string address, int port);
      ~DesyTableCommunication();

      int trashRecvBuffer(); //throws away anything what is in the receive buffer
      int transmit(std::string command, uint8_t address); //just sends the command to the given counter module address
      std::string receive(); //receives whatever is in the buffer
      std::string receive(int bytes, std::chrono::milliseconds timeout); //receives n bytes, exits only on timeout

//      bool setPosition(int position, uint8_t address); //sets the position for a specific address. Returns a success code
      void setHysteresis(int value, uint8_t address); //sets the hysteresis (inactive region for the control). Returns a success code//TODO requires the full configuration

      //functions to read available values
      int getActualPosition(const uint8_t address);
      int getPresetP1(const uint8_t address); //the desired position
      int getPresetP5(const uint8_t address); //limit for the fast/slow motion of the moving stage
      float getActualPositionmm(const uint8_t address);
      float getPresetP1mm(const uint8_t address); //the desired position
      float getPresetP5mm(const uint8_t address); //limit for the fast/slow motion of the moving stage

      //functions to set values
      void setActualPosition(const int value, const uint8_t address); //to change the C1 to specific value (necessary to define zero)
      void setPresetP1(const int value, const uint8_t address); // change the P1 (where will the stage move)
	  void setPresetP5(const int value, const uint8_t address); // change P5 (the limit were the fast movement is switched off)
	  void setActualPositionmm(const float value, const uint8_t address); //to change the C1 to specific value (necessary to define zero)
	  void setPresetP1mm(const float value, const uint8_t address); // change the P1 (where will the stage move)
	  void setPresetP5mm(const float value, const uint8_t address); // change P5 (the limit were the fast movement is switched off)
      void takeOver(const uint8_t address); // necessary to call after "set" functions, otherwise the command doesn't take any effect

      std::string getControllerSetup(uint8_t address);

      int getDebugLevel() const;
      void setDebugLevel(int debugLevel);
      float getMmToBins() const;
      void setMmToBins(float mmToBins);

   private:
      int requestValue(uint8_t address, const char* command, const char* expectedResponse); //to request P1..P5. THey have common format
      int debugLevel = 0; //debug level. 0=no messages, 9==max messages
      void transmit(std::string command); //send the complete string with the address
      std::mutex communication_mutex;
      static const int recv_buf_length = 512;
      float mmToBins=10.0;
      //      int _tcpPort; //communication port
//      std::string _ipAddress; // address for 20mA Schnittstelle communication gateway (remote controller of the moving stage)

#ifdef _WIN32
      SOCKET _fd; //the TCP socket to the stage
#else
      int _fd; //the TCP socket to the stage
#endif

};

#endif // DESYTABLECOMMUNICATION_HH
