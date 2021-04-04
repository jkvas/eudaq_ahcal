#ifndef DESYTABLEPRODUCER_HH
#define DESYTABLEPRODUCER_HH

//#include <Producer.hh>
//#include <Utils.hh>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

class DesyTableCommunication;

class DesyTableProducer: public eudaq::Producer {
   public:
      DesyTableProducer(const std::string & name, const std::string & runcontrol);
      void DoInitialise() override;
      void DoConfigure() override;
      void DoStartRun() override;
      void DoStopRun() override;
      void DoTerminate() override;
      void DoReset() override;
      void RunLoop() override;

      static const uint32_t m_id_factory = eudaq::cstr2hash("DesyTableProducer");

   private:
      std::mutex m_communication_manipulation;
      bool m_exit_of_run;
      std::unique_ptr<DesyTableCommunication> m_comm;
      std::string controllerSetup;//configuration of the counter. I can be then added as a TAG to the stream
      std::chrono::duration<float> readInterval;
      std::chrono::duration<float> checkStabilitySeconds;
      uint8_t horizontalAddress;
      uint8_t verticalAddress;
};

#endif // DESYTABLEPRODUCER_HH
