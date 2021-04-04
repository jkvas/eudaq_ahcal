//#include <Configuration.hh>
//#include <Factory.hh>
//#include <Logger.hh>
#include <iostream>
#include <math.h>
#include "eudaq/Producer.hh"
#include "eudaq/OptionParser.hh"
#include "eudaq/Logger.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/Utils.hh"

#ifndef _WIN32
#include <sys/file.h>
#endif

#include "DesyTableProducer.hh"
#include "DesyTableCommunication.hh"

namespace {
auto dummy0 = eudaq::Factory<eudaq::Producer>::Register<DesyTableProducer, const std::string&, const std::string&>(DesyTableProducer::m_id_factory);
}
DesyTableProducer::DesyTableProducer(const std::string & name, const std::string & runcontrol) :
      eudaq::Producer(name, runcontrol), m_exit_of_run(false) {
}
void DesyTableProducer::DoInitialise() {
   std::cout << "DEBUG: start of initialization" << std::endl;
   auto ini = GetInitConfiguration();
   m_comm = std::unique_ptr<DesyTableCommunication>(new DesyTableCommunication(ini->Get("IP_ADDRESS", "192.168.1.66"), ini->Get("TCP_PORT", 8000)));
   std::cout << "communication module loaded" << std::endl;
   horizontalAddress = ini->Get("HORIZONTAL_ADDRESS", 0);
   verticalAddress = ini->Get("VERTICAL_ADDRESS", 1);
   if (horizontalAddress == verticalAddress) {
      EUDAQ_ERROR_STREAMOUT("Wrong setting of addresses. Both counters cannot have same address", std::cout, std::cerr);
      //change the addresses to something hopefully unmeaningful.
      horizontalAddress = 100;
      verticalAddress = 101;
   }
   m_comm->setDebugLevel(ini->Get("DEBUG_VERBOSITY_LEVEL", 0));
   m_comm->setMmToBins(ini->Get("MM_TO_BINS", 10.0));
//   std::cout << "DEBUG: doinit 1" << std::endl;
   std::unique_lock<std::mutex> lock(m_communication_manipulation);
   //setting P1 to current posotion enables switching on of the automatic steering on the remote controller (there is a mechanical switch, which switches only when P1==C1
   if (ini->Get("RESET_P1_TO_ACTUAL_POSITION", 0)) {
      EUDAQ_INFO_STREAMOUT("Setting P1 to C1", std::cout, std::cerr);
      for (uint8_t address = 0; address < 2; address++) { //loop over 2 sub-devices
         int position = m_comm->getActualPosition(address);
         m_comm->getActualPositionmm(address);
         std::cout << "Read position = " << position << std::endl;
         m_comm->setPresetP1(position, address);
      }
   } else {
      EUDAQ_INFO_STREAMOUT("Keeping P1 as it is", std::cout, std::cerr);
   }
//   m_comm = std::make_unique<DesyTableCommunication>(std::string("localhost"), 5627);
   controllerSetup = m_comm->getControllerSetup(0);
//   std::this_thread::sleep_for(std::chrono::milliseconds(1000));
   std::cout << "DEBUG: end of initialization" << std::endl;
}

void DesyTableProducer::DoConfigure() {
   std::cout << "DEBUG mark 0" << std::endl;
//   POSITION_READ_INTERVAL_SECONDS = 10 #how iften will be the position checked durig data taking.
//   #in order to approch the position from a specific direction, a relative approach
//   # start position can be set, as a relative position from the desired end position
//   HORIZONTAL_APROACH_RELATIVE_POSITION_RAW = -400
//   VERTICAL_APROACH_RELATIVE_POSITION_RAW = -400
//   #mm has a priority
//   HORIZONTAL_APROACH_RELATIVE_POSITION_MM = -40.0
//   VERTICAL_APROACH_RELATIVE_POSITION_MM = -40.0
//
//   #the position where will the moving stage travel.
//   HORIZONTAL_POSITION_RAW = 602
//   VERTICAL_POSITION_RAW = 600
//   HORIZONTAL_POSITION_MM = 60.2 #mm has a priority
//   VERTICAL_POSITION_MM = 60.0 #mm has a priority
//
//   CHECK_STABILITY_SECONDS = 1.0 # how long to wait for the stable position
//   HORIZONTAL_SLOW_LENGTH_RAW = 200
//   HORIZONTAL_SLOW_LENGTH_MM = 20.0 #region from destination where the stage travels slow
//   VERTICAL_SLOW_LENGTH_RAW = 200
//   VERTICAL_SLOW_LENGTH_MM = 20.0 #region from destination where the stage travels slow
   this->SetStatus(eudaq::Status::STATE_UNCONF, "not finished");
   auto conf = GetConfiguration();
   //conf->Print(std::cout);
   readInterval = std::chrono::duration<float>(conf->Get("POSITION_READ_INTERVAL_SECONDS", 10.0));
   checkStabilitySeconds = std::chrono::duration<float>(conf->Get("CHECK_STABILITY_SECONDS", 1.0));
   m_comm->trashRecvBuffer();
   const double invalid = 1000000000.0; //infinity
   const double cinvalid = invalid - 1.0; //constant to compare with invalid. Has to be lower because of the floating point

   double h_slow_mm = conf->Get("HORIZONTAL_SLOW_LENGTH_MM", 0.0);
   if (h_slow_mm < 0.01) h_slow_mm = conf->Get("HORIZONTAL_SLOW_LENGTH_RAW", 0.0) / m_comm->getMmToBins();
   if (h_slow_mm > 0.01) m_comm->setPresetP5mm(h_slow_mm, horizontalAddress); //send only values > 0

   double v_slow_mm = conf->Get("VERTICAL_SLOW_LENGTH_MM", 0.0);
   if (v_slow_mm < 0.01) v_slow_mm = conf->Get("VERTICAL_SLOW_LENGTH_RAW", 0.0) / m_comm->getMmToBins();
   if (v_slow_mm > 0.01) m_comm->setPresetP5mm(h_slow_mm, verticalAddress); //send only values > 0

   double h_pos_mm = conf->Get("HORIZONTAL_POSITION_MM", invalid);
//   std::cout << "HORIZONTAL_POSITION_MM = " << h_pos_mm << std::endl;
   double h_pos_raw = conf->Get("HORIZONTAL_POSITION", invalid);
   if (h_pos_mm >= cinvalid) h_pos_mm = (h_pos_raw >= cinvalid) ? invalid : h_pos_raw / m_comm->getMmToBins();

   double v_pos_mm = conf->Get("VERTICAL_POSITION_MM", invalid);
//   std::cout << "VERTICAL_POSITION_MM = " << v_pos_mm << std::endl;
   double v_pos_raw = conf->Get("VERTICAL_POSITION_RAW", invalid);
   if (v_pos_mm >= cinvalid) v_pos_mm = (v_pos_raw >= cinvalid) ? invalid : v_pos_raw / m_comm->getMmToBins();

   double h_approach_mm = conf->Get("HORIZONTAL_APROACH_RELATIVE_POSITION_MM", invalid);
//   std::cout << "HORIZONTAL_APROACH_RELATIVE_POSITION_MM = " << h_approach_mm << std::endl;
   double h_approach_raw = conf->Get("HORIZONTAL_APROACH_RELATIVE_POSITION_RAW", invalid);
   if (h_approach_mm >= cinvalid) h_approach_mm == (h_approach_raw >= cinvalid) ? invalid : h_approach_raw / m_comm->getMmToBins();
   if (h_approach_mm <= 0.01) h_approach_mm = invalid;

   double v_approach_mm = conf->Get("VERTICAL_APROACH_RELATIVE_POSITION_MM", invalid);
//   std::cout << "VERTICAL_APROACH_RELATIVE_POSITION_MM = " << v_approach_mm << std::endl;
   double v_approach_raw = conf->Get("VERTICAL_APROACH_RELATIVE_POSITION_RAW", invalid);
   if (v_approach_mm >= cinvalid) v_approach_mm == (v_approach_raw >= cinvalid) ? invalid : v_approach_raw / m_comm->getMmToBins();
   if (v_approach_mm <= 0.01) v_approach_mm = invalid;
   if ((h_approach_mm < cinvalid) && (h_pos_mm < cinvalid)) {
      EUDAQ_INFO_STREAMOUT("Preparing for the approach position H=" + std::to_string(h_pos_mm + h_approach_mm) + "mm (pos=" + std::to_string(h_pos_mm) + "mm,approach=" + std::to_string(h_approach_mm) + ")", std::cout, std::cerr);
      m_comm->setPresetP1mm(h_pos_mm + h_approach_mm, horizontalAddress);
   }
   if ((v_approach_mm < cinvalid) && (v_pos_mm < cinvalid)) {
      EUDAQ_INFO_STREAMOUT("Preparing for the approach position V=" + std::to_string(v_pos_mm + v_approach_mm) + "mm (pos=" + std::to_string(v_pos_mm) + "mm,approach=" + std::to_string(v_approach_mm) + ")", std::cout, std::cerr);
      m_comm->setPresetP1mm(v_pos_mm + v_approach_mm, verticalAddress);
   }
   //wait for the stable position
   if (((h_approach_mm < cinvalid) && (h_pos_mm < cinvalid)) || ((v_approach_mm < cinvalid) && (v_pos_mm < cinvalid))) {
      m_comm->trashRecvBuffer();
      auto tp_change = std::chrono::steady_clock::now();
      double last_h_mm = 0.0;
      double last_v_mm = 0.0;
      bool done = false;
      while (!done) {
         double h_mm = m_comm->getActualPositionmm(horizontalAddress);
         double v_mm = m_comm->getActualPositionmm(verticalAddress);
         if ((abs(last_h_mm - h_mm) > 0.05) || (abs(last_v_mm - v_mm) > 0.05)) {
            last_h_mm = h_mm;
            last_v_mm = v_mm;
            tp_change = std::chrono::steady_clock::now();
         }
         if (std::chrono::duration<float>(std::chrono::steady_clock::now() - tp_change) > checkStabilitySeconds) {
            std::cout << "DEBUG: stable position achieved. H=" << last_h_mm << " V=" << last_v_mm << std::endl;
            done = true;
         } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "DEBUG: waiting for the position. H=" << last_h_mm << " V=" << last_v_mm << std::endl;
         }
      }
   } else {
      EUDAQ_INFO_STREAMOUT("No approch position set" + std::to_string(h_pos_mm + h_approach_mm) + "mm (pos=" + std::to_string(h_pos_mm) + "mm,approach=" + std::to_string(h_approach_mm) + ")", std::cout, std::cerr);
      m_comm->setPresetP1mm(h_pos_mm + h_approach_mm, horizontalAddress);
   }
   //set the final position
   if (h_pos_mm < cinvalid) {
      EUDAQ_INFO_STREAMOUT("Setting the table position H=" + std::to_string(h_pos_mm) + "mm", std::cout, std::cerr);
      m_comm->setPresetP1mm(h_pos_mm, horizontalAddress);
   }
   if (v_pos_mm < cinvalid) {
      EUDAQ_INFO_STREAMOUT("Setting the table position V=" + std::to_string(v_pos_mm) + "mm", std::cout, std::cerr);
      m_comm->setPresetP1mm(v_pos_mm, verticalAddress);
   }
   //wait for the stable position
   if ((h_pos_mm < cinvalid) || (v_pos_mm < cinvalid)) {
      m_comm->trashRecvBuffer();
      auto tp_change = std::chrono::steady_clock::now();
      double last_h_mm = 0.0;
      double last_v_mm = 0.0;
      bool done = false;
      while (!done) {
         double h_mm = m_comm->getActualPositionmm(horizontalAddress);
         double v_mm = m_comm->getActualPositionmm(verticalAddress);
         if ((abs(last_h_mm - h_mm) > 0.05) || (abs(last_v_mm - v_mm) > 0.05)) {
            last_h_mm = h_mm;
            last_v_mm = v_mm;
            tp_change = std::chrono::steady_clock::now();
         }
         if (std::chrono::duration<float>(std::chrono::steady_clock::now() - tp_change) > checkStabilitySeconds) {
            std::cout << "DEBUG: stable position achieved. H=" << last_h_mm << " V=" << last_v_mm << std::endl;
            done = true;
         } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::cout << "DEBUG: waiting for the position. H=" << last_h_mm << " V=" << last_v_mm << std::endl;
         }
      }
   }
}

void DesyTableProducer::DoStartRun() {
   m_exit_of_run = false;
}
void DesyTableProducer::DoStopRun() {
   m_exit_of_run = true;
}

void DesyTableProducer::DoReset() {
   m_exit_of_run = true;
//   if (m_file_lock) {
//#ifndef _WIN32
//      flock(fileno(m_file_lock), LOCK_UN);
//#endif
//      fclose(m_file_lock);
//      m_file_lock = 0;
//   }
//   m_ms_busy = std::chrono::milliseconds();
//   m_exit_of_run = false;
}
void DesyTableProducer::DoTerminate() {
   m_exit_of_run = true;
//   if (m_file_lock) {
//      fclose(m_file_lock);
//      m_file_lock = 0;
//   }
}
void DesyTableProducer::RunLoop() {
   auto tp_readout = std::chrono::steady_clock::now();
   bool firstCycle = true;
   while (!m_exit_of_run) {
      m_comm->trashRecvBuffer();
      double h_mm = m_comm->getActualPositionmm(horizontalAddress);
      double v_mm = m_comm->getActualPositionmm(verticalAddress);
      if ((firstCycle) || (std::chrono::duration<double>(std::chrono::steady_clock::now() - tp_readout) > readInterval)) {
         firstCycle = false;
         tp_readout = std::chrono::steady_clock::now();
         auto ev = eudaq::Event::MakeUnique("DesyTableRaw");
         std::cout << "Position: H=" << h_mm << "mm, V=" << v_mm << "mm" << std::endl;
         ev->SetTag("POS_H_MM", std::to_string(h_mm));
         ev->SetTag("POS_V_MM", std::to_string(v_mm));
         SendEvent(std::move(ev));
         SetStatusTag("Y", std::to_string(v_mm));
         SetStatusTag("X", std::to_string(h_mm));
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
   }
//   uint32_t trigger_n = 0;
//   uint8_t x_pixel = 16;
//   uint8_t y_pixel = 16;
//   std::random_device rd;
//   std::mt19937 gen(rd());
//   std::uniform_int_distribution<uint32_t> position(0, x_pixel * y_pixel - 1);
//   std::uniform_int_distribution<uint32_t> signal(0, 255);
//   while (!m_exit_of_run) {
//      auto ev = eudaq::Event::MakeUnique("DesyTableRaw");
//      auto tp_trigger = std::chrono::steady_clock::now();
//      auto tp_end_of_busy = tp_trigger + m_ms_busy;
//      if (m_flag_ts) {
//         std::chrono::nanoseconds du_ts_beg_ns(tp_trigger - tp_start_run);
//         std::chrono::nanoseconds du_ts_end_ns(tp_end_of_busy - tp_start_run);
//         ev->SetTimestamp(du_ts_beg_ns.count(), du_ts_end_ns.count());
//      }
//      if (m_flag_tg)
//         ev->SetTriggerN(trigger_n);
//
//      std::vector<uint8_t> hit(x_pixel * y_pixel, 0);
//      hit[position(gen)] = signal(gen);
//      std::vector<uint8_t> data;
//      data.push_back(x_pixel);
//      data.push_back(y_pixel);
//      data.insert(data.end(), hit.begin(), hit.end());
//
//      uint32_t block_id = m_plane_id;
//      ev->AddBlock(block_id, data);
//      SendEvent(std::move(ev));
//      trigger_n++;
//      std::this_thread::sleep_until(tp_end_of_busy);
//   }
}
