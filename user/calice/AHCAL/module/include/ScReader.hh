#ifndef SCREADER_HH
#define SCREADER_HH

#include "AHCALProducer.hh"
#include "eudaq/RawEvent.hh"

#include <deque>

#define TERMCOLOR_RED_BOLD "\033[31;1m"
#define TERMCOLOR_RED "\033[31m"
#define TERMCOLOR_GREEN_BOLD "\033[32;1m"
#define TERMCOLOR_GREEN "\033[32m"
#define TERMCOLOR_YELLOW_BOLD "\033[33;1m"
#define TERMCOLOR_YELLOW "\033[33m"
#define TERMCOLOR_BLUE_BOLD "\033[34;1m"
#define TERMCOLOR_BLUE "\033[34m"
#define TERMCOLOR_MAGENTA_BOLD "\033[35;1m"
#define TERMCOLOR_MAGENTA "\033[35m"
#define TERMCOLOR_CYAN_BOLD "\033[36;1m"
#define TERMCOLOR_CYAN "\033[36m"
#define TERMCOLOR_RESET "\033[0m"

#define DAQ_ERRORS_INCOMPLETE        0x0001
#define DAQ_ERRORS_OUTSIDE_ACQ       0x0002
#define DAQ_ERRORS_MULTIPLE_TRIGGERS 0x0004
#define DAQ_ERRORS_MISSED_DUMMY      0x0008
#define DAQ_ERRORS_MISSING_START     0x0010
#define DAQ_ERRORS_MISSING_STOP      0x0020

namespace eudaq {

   class ScReader: public AHCALReader {
      public:
         virtual void Read(std::deque<unsigned char> & buf, std::deque<eudaq::EventUP> & deqEvent) override;
         virtual void OnStart(int runNo) override;
         virtual void OnStop(int waitQueueTimeS) override;
         virtual void OnConfigLED(std::string _fname) override; //chose configuration file for LED runs
         virtual void buildEvents(std::deque<eudaq::EventUP> &EventQueue, bool dumpAll) override;

//         virtual std::deque<eudaq::RawEvent *> NewEvent_createRawDataEvent(std::deque<eudaq::RawEvent *> deqEvent, bool tempcome, int LdaRawcycle,
//         bool newForced);
         virtual void readTemperature(std::deque<unsigned char>& buf);

         virtual void setTbTimestamp(uint32_t ts) override;
         virtual uint32_t getTbTimestamp() const override;

         int updateCntModulo(const int oldCnt, const int newCntModulo, const int bits, const int maxBack);
         void appendOtherInfo(eudaq::RawEvent * ev);

         ScReader(AHCALProducer *r); //:
//               AHCALReader(r),
//                     _runNo(-1),
//                     _buffer_inside_acquisition(false),
//                     _lastBuiltEventNr(0),
//                     _cycleNo(0),
//                     _tempmode(false),
//                     _trigID(0),
//                     _unfinishedPacketState(UnfinishedPacketStates::DONE),
//                     length(0) {
//         }
         virtual ~ScReader();

         struct LDATimeData {
               uint64_t TS_Start; //start of acquisition
               uint64_t TS_Stop; //stop of acquisition
               std::vector<int> TriggerIDs; //trigger IDs from the LDA packet
               std::vector<uint64_t> TS_Triggers; //triggers Timestamps inside acquisition

         };

         struct RunTimeStatistics {
               void clear();
               void append(const RunTimeStatistics& otherStats);
               void print(std::ostream &out, int colorOutput) const;
               uint64_t first_TS; //the very first TS in the data
               uint64_t last_TS; //the very last recorded TS in the data
               uint64_t previous_start_TS;
               uint64_t previous_stop_TS;
               uint64_t ontime; //accumulate the active time
               uint64_t offtime; //accumulate the switched-off time
               uint64_t cycles;
               int cycle_triggers; //triggers inside acq counted only for last ROC
               int triggers_inside_roc;
               int triggers_outside_roc;
               int triggers_lost; //completely missed triggers due to data loss
               int triggers_empty;
               int builtBXIDs; //how many BXIDs was made during event building
               std::vector<uint64_t> length_acquisitions;
               std::vector<uint64_t> length_processing;
               std::map<int, int> triggers_in_cycle_histogram;
               std::map<int,int> trigger_multiplicity_in_bxid;// <triggers,count> how many triggers have been present in the bxid
         };

         const ScReader::RunTimeStatistics& getRunTimesStatistics() const;
         unsigned int getCycleNo() const;
         unsigned int getTrigId() const;

         uint16_t grayRecode(const uint16_t partiallyDecoded);

      private:
         enum class UnfinishedPacketStates {
            DONE = (unsigned int) 0x0000,
            //            LEDINFO = (unsigned int) 0x0001,
            TEMPERATURE = (unsigned int) 0x0001,
            SLOWCONTROL = (unsigned int) 0x0002,
         };
         enum {
            e_sizeLdaHeader = (int)10 // 8bytes + 0xcdcd
         };
         enum BufferProcessigExceptions {
            ERR_INCOMPLETE_INFO_CYCLE, OK_ALL_READ, OK_NEED_MORE_DATA
         };

      private:
         void printLDATimestampTriggers(std::map<int, LDATimeData> &TSData);
         void printLDAROCInfo(std::ostream &out);
         void buildROCEvents(std::deque<eudaq::EventUP> &EventQueue, bool dumpAll);
         void buildTRIGIDEvents(std::deque<eudaq::EventUP> &EventQueue, bool dumpAll);
         void buildBXIDEvents(std::deque<eudaq::EventUP> &EventQueue, bool dumpAll);
         void buildValidatedBXIDEvents(std::deque<eudaq::EventUP> &EventQueue, bool dumpAll);
         void insertDummyEvent(std::deque<eudaq::EventUP> &EventQueue, int eventNumber, int triggerid, bool triggeridFlag);
         void prepareEudaqRawPacket(eudaq::RawEvent * ev);
         void colorPrint(const std::string &colorString, const std::string& msg);

         static const unsigned char C_TSTYPE_START_ACQ = 0x01;
         static const unsigned char C_TSTYPE_STOP_ACQ = 0x02;
         static const unsigned char C_TSTYPE_SYNC_ACQ = 0x03;
         static const unsigned char C_TSTYPE_TRIGID_INC = 0x10;
         static const unsigned char C_TSTYPE_ROC_INC = 0x11;
         static const unsigned char C_TSTYPE_BUSY_FALL = 0x20;
         static const unsigned char C_TSTYPE_BUSY_RISE = 0x21;
         static const unsigned int C_TS_IGNORE_ROC_JUMPS_UP_TO = 20;
         static const uint64_t C_MILLISECOND_TICS = 40000; //how many clock cycles make a millisecond

         void readAHCALData(std::deque<unsigned char> &buf, std::map<int, std::vector<std::vector<int> > > &AHCALData);
         void readLDATimestamp(std::deque<unsigned char> &buf, std::map<int, LDATimeData> &LDATimestamps);
         eudaq::EventUP insertMissedTrigger(const int roc, const uint64_t startTS, const int lastBuiltEventNr, const int ErrorStatus);

         bool IsBXIDComplete(const int roc, const int AhcalBxid, std::map<int, std::vector<std::vector<int> > >::const_iterator sameBxidPacketIterator,
               bool reportIncomplete);
         eudaq::EventUP insertEmptyEvent(const uint64_t stopTS, const uint64_t startTS, const int RawTriggerID, const uint64_t triggerTs, const int lastBuiltEventNr, const int triggerBxid, const int roc, int& ErrorStatus) ;
         UnfinishedPacketStates _unfinishedPacketState;

         int _runNo;
         int _cycleNo; //last successfully read readoutcycle: ASIC Data
         int _cycleNoTS;// last timestamp cycle number
         unsigned int _trigID; //last successfully read trigger ID from LDA timestamp. Next trigger should be _trigID+1
         bool _trigidNotKnown;
         int length; //length of the packed derived from LDA Header

         //bool _tempmode; // during the temperature readout time
         bool _buffer_inside_acquisition; //the reader is reading data from within the acquisition, defined by start and stop commands
         //uint64_t _last_stop_ts; //timestamp of the last stop of acquisition

         std::vector<std::pair<std::pair<int, int>, int> > _vecTemp;            // (lda, port), data;
         std::vector<int> slowcontrol;
         std::vector<int> ledInfo;
         std::vector<int> HVAdjInfo; // Bias Voltage adjustments. Data structure: entry is an 8x integer in following format: LDA, Port, Module, 0 HV1, HV2, HV3, 0
         std::vector<uint32_t> cycleData;

         int _lastBuiltEventNr;            //last event number for keeping track of missed events in the stream (either ROC, trigger number or arbitrary number)
         int _lastROC;

         std::map<int, LDATimeData> _LDATimestampData; //maps READOUTCYCLE to LDA timestamps for that cycle (comes asynchronously with the data and tends to arrive before the ASIC packets)

         std::map<int, std::vector<std::vector<int> > > _LDAAsicData;              //maps readoutcycle to vector of "infodata"

         std::map<int, int> _DaqErrors;              // <ReadoutCycleNumber, ErrorMask> if errormas is 0, everything is OK

         std::map<int, int> minLastBxid_Detector;              //stores the minimum! lowest bxid number  for any memory cell 16 (cell 15 when counting from 0)
                                                               //throughout the Detector

         std::map<int, std::map<int, int> > minLastBxid_Asic;

         uint32_t _timestampTbCampaign;

         RunTimeStatistics _RunTimesStatistics;
   };
}

#endif // SCREADER_HH
