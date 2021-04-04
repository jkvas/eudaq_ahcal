#include "eudaq/RunControl.hh"
#include "eudaq/TransportServer.hh"
using namespace eudaq;

class AhcalRunControl: public eudaq::RunControl {
   public:
      AhcalRunControl(const std::string & listenaddress);
      void Configure() override;
      void StartRun() override;
      void StopRun() override;
      void Exec() override;
      static const uint32_t m_id_factory = eudaq::cstr2hash("AhcalRunControl");

   private:
      void WaitForStates(const eudaq::Status::State state, const std::string message);
      void WaitForMessage(const std::string message);
      uint32_t m_stop_seconds; //maximum length of running in seconds
      uint32_t m_stop_filesize; //filesize in bytes. Run gets stopped after reaching this size
      uint32_t m_stop_events; // maximum number of events before the run gets stopped
      std::string m_next_conf_path; //filename of the next configuration file
      std::string m_monitored_collector; //collector name, which is used for checking of the increment
      uint32_t m_inactivity_timeout; //timeout in seconds. If no change in the monitored data collector after the timeout, run will stop.
      uint32_t m_monitored_event; //currently last seen event number
      uint32_t m_reprocessing_inactivity_timeout;// how long to wait for constant number of events after ReprocessingFinished is raised.
      std::chrono::time_point<std::chrono::steady_clock> m_last_change_time;
      bool m_flag_running;
      std::chrono::steady_clock::time_point m_tp_start_run;

};

namespace {
   auto dummy0 = eudaq::Factory<eudaq::RunControl>::Register<AhcalRunControl, const std::string&>(AhcalRunControl::m_id_factory);
}

AhcalRunControl::AhcalRunControl(const std::string & listenaddress) :
      RunControl(listenaddress) {
   m_flag_running = false;
}

void AhcalRunControl::StartRun() {
   std::cout << "AHCAL runcontrol StartRun " << GetRunN() << " - beginning" << std::endl;
   m_tp_start_run = std::chrono::steady_clock::now();
   RunControl::StartRun();
   m_monitored_event = 0;
   m_last_change_time = std::chrono::steady_clock::now();
   m_flag_running = true;
   m_inactivity_timeout = GetConfiguration()->Get("STOP_INACTIVITY_SECONDS", 1000000);
   std::cout << "AHCAL runcontrol StartRun " << GetRunN() << " - end" << std::endl;
}

void AhcalRunControl::StopRun() {
   std::cout << "AHCAL runcontrol StopRun " << GetRunN() << " - beginning" << std::endl;
   RunControl::StopRun();
   m_flag_running = false;
   std::cout << "AHCAL runcontrol StopRun " << GetRunN() << " - end" << std::endl;
}

void AhcalRunControl::Configure() {
   auto conf = GetConfiguration();
//   conf->SetSection("RunControl");
   m_stop_seconds = conf->Get("STOP_RUN_AFTER_N_SECONDS", 0);
   m_stop_filesize = conf->Get("STOP_RUN_AFTER_N_BYTES", 0);
   m_stop_events = conf->Get("STOP_RUN_AFTER_N_EVENTS", 0);
   m_next_conf_path = conf->Get("NEXT_RUN_CONF_FILE", "");
   m_inactivity_timeout = conf->Get("STOP_INCATIVITY_SECONDS", 1000000);
   m_monitored_collector = conf->Get("MONITORED_COLLECTOR_NAME", "");
   m_reprocessing_inactivity_timeout=conf->Get("REPROCESSING_INACTIVITY_TIMEOUT",20);
   if (conf->Get("RUN_NUMBER", 0)) {
      EUDAQ_INFO_STREAMOUT("Setting new Run number (from conf): " + std::to_string(conf->Get("RUN_NUMBER", 0)), std::cout, std::cerr);
      SetRunN(conf->Get("RUN_NUMBER", 0));
   }
   conf->Print();
   RunControl::Configure();
}

void AhcalRunControl::WaitForStates(const eudaq::Status::State state, const std::string message) {
   while (1) {
      bool waiting = false;
      auto map_conn_status = GetActiveConnectionStatusMap();
      for (auto &conn_status : map_conn_status) {
         auto state_conn = conn_status.second->GetState();
         if (state_conn != state) {
            waiting = true;
	    std::cout << "DEBUG: " << conn_status.first->GetName() << " --- " << conn_status.second->GetMessage() ;
	    std::cout << ", should be (" << state << "), but is (" << state_conn <<")"<< std::endl;
         }
      }
      if (!waiting) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      EUDAQ_INFO(message); //"Waiting for end of stop run");
   }
}

void AhcalRunControl::Exec() {
   //is being active all the time
   std::cout << "AHCAL runcontrol Exec - before StartRunControl" << std::endl;
   StartRunControl();
   std::cout << "AHCAL runcontrol Exec - after StartRunControl" << std::endl;
   //auto last_change_time = std::chrono::steady_clock::now();

   std::cout << "RUNNING" << std::endl;
   m_monitored_event = 0;

   while (IsActiveRunControl()) {
      //std::cout << "AHCAL runcontrol Exec - while loop" << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      bool restart_run = false; //whether to stop the run. There might be different reasons for it (timeout, number of evts, filesize...)

      if (m_flag_running) {
         //CHECK FOR CHANGE IN FILESIZE
         auto map_conn_status = GetActiveConnectionStatusMap();
         for (auto &conn_status : map_conn_status) {
            auto contype = conn_status.first->GetType();

            if (contype == "DataCollector") {
               if (m_monitored_collector.size() > 1) {               //skip unwanted data collectors
                  if (m_monitored_collector.compare(conn_status.first->GetName()) != 0) {               //strings do not match
//                     std::cout << "DEBUG: skipping collector " << conn_status.first->GetName() << std::endl;
                     continue;
                  }
               }
               for (auto &elem : conn_status.second->GetTags()) {
                  if (elem.first == "EventN") {
                     std::cout << conn_status.first->GetName() << std::endl;
                     if (stoi(elem.second) != m_monitored_event) {
                        //number of events changed
                        if (m_monitored_event == 0) {
                           std::cout << "New Run Started..." << std::endl;
                        }
                        std::cout << "EUDAQ received " << stoi(elem.second) - m_monitored_event << "new events." << std::endl;
                        m_monitored_event = stoi(elem.second);
                        m_last_change_time = std::chrono::steady_clock::now();
                     } else {
                        auto tp_now = std::chrono::steady_clock::now();
                        int duration_s = std::chrono::duration_cast<std::chrono::seconds>(tp_now - m_last_change_time).count();
                        int countdown = m_inactivity_timeout - duration_s;

                        std::cout << "EUDAQ has found " << m_monitored_event << "Events.  Waiting for further events, with " << countdown << "s remaining."
                              << std::endl;
                        if ((m_inactivity_timeout) && (countdown <= 0)) {
                           EUDAQ_INFO_STREAMOUT(
                                 "Runcontrol found " + std::to_string(m_monitored_event) + " Events. This value didn't change for " + std::to_string(duration_s)
                                       + " s.", std::cout, std::cerr);
                           restart_run = true;
                        }
                     }
                     std::cout << "PRINT DIFF " << stoi(elem.second) - m_inactivity_timeout << std::endl;
                  }
                  if (!conn_status.first->IsEnabled()) {
                     std::cout << "DEBUG collector not enabled" << std::endl;
                  };
               }
            }
            if (contype == "Producer") {
               for (auto &elem : conn_status.second->GetTags()) {
                  if (elem.first == "ReprocessingFinished") {
                     if (stoi(elem.second) == 1) {
//                        restart_run = true;
                        m_inactivity_timeout = m_reprocessing_inactivity_timeout;
                     }
                  }
               }
            }
         }

         //timeout condition
         if (m_stop_seconds) {
            auto tp_now = std::chrono::steady_clock::now();
            int duration_s = std::chrono::duration_cast<std::chrono::seconds>(tp_now - m_tp_start_run).count();
            std::cout << "Duration from start: " << duration_s;
            std::cout << ". Limit:" << m_stop_seconds << std::endl;
            if (duration_s > m_stop_seconds) {
               restart_run = true;
               EUDAQ_INFO_STREAMOUT("Timeout" + std::to_string(duration_s) + "reached.", std::cout, std::cerr);
            }

         }



         //number of event condition
         if (m_stop_events) {
	   //std::vector<ConnectionSPC> conn_events;
	   std::cout << "event sizes ";
	   auto map_conn_status = GetActiveConnectionStatusMap();
	   for (auto &conn_status : map_conn_status) {
	     auto contype = conn_status.first->GetType();
	     if (contype == "DataCollector") {	  
	       for (auto &elem : conn_status.second->GetTags()) {
		 if (elem.first == "EventN") {
		   std::cout << conn_status.first->GetName();
		   std::cout << "=" << stoi(elem.second) << " ";
		   if (m_monitored_collector.size() > 1) {               //skip unwanted data collectors
		     if (m_monitored_collector.compare(conn_status.first->GetName()) != 0) {               //strings do not match
		       std::cout << "(ignored) ";
		       // std::cout << "DEBUG: skipping collector " << conn_status.first->GetName() << std::endl;
		       continue;
		     }
		   }
		   std::cout << " ";
		   if (stoi(elem.second) > m_stop_events) {
		     restart_run = true;
		     EUDAQ_INFO_STREAMOUT("Max event number" + elem.second + "reached.", std::cout, std::cerr);
		   }
		 }
	       }
	     }
            }
            std::cout << "limit=" << m_stop_events << std::endl;
         }
         if (m_stop_filesize) {
            //TODO
         }
         //restart and reconfiguration handling
         if (restart_run) {
            std::cout << "AHCAL runcontrol Exec - restarting" << std::endl;
            EUDAQ_INFO("Stopping the run and restarting a new one.");
            StopRun(); // will stop and wait for all modules to stop daq
            //wait for everything to stop
            WaitForStates(eudaq::Status::STATE_STOPPED, "Waiting for end of stop run");

            if (m_next_conf_path.size()) {
               //TODO: check if file exists
               EUDAQ_INFO("Reading new config file: " + m_next_conf_path);
               ReadConfigureFile(m_next_conf_path);
               Configure();
               std::this_thread::sleep_for(std::chrono::seconds(2));
               //wait until everything is configured
               WaitForMessage("Configured");
//               WaitForStates(eudaq::Status::STATE_CONF, "Waiting for end of configure");
            }
            StartRun();
         }
      }
   }
}

inline void AhcalRunControl::WaitForMessage(const std::string message) {
   while (1) {
      bool waiting = false;
      auto map_conn_status = GetActiveConnectionStatusMap();
      for (auto &conn_status : map_conn_status) {
         auto conn_message = conn_status.second->GetMessage();
         if (conn_message.compare("Configured")) {
            //does not match
            std::cout << "DEBUG: waiting for " << conn_status.first->GetName() << ", which is " << conn_message << " instead of " << message << std::endl;
            waiting = true;
         }
      }
      if (!waiting) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
   }
}
