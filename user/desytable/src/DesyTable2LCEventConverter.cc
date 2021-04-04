/*
 * DesyTable2LCEventConverter.cc
 *
 *  Created on: Dec 22, 2017
 *      Author: kvas
 */
#include <IMPL/LCCollectionVec.h>

#include "eudaq/LCEventConverter.hh"
#include "eudaq/RawEvent.hh"
#include "eudaq/Logger.hh"

namespace eudaq {

   using namespace std;
   using namespace lcio;

   class DesyTable2LCEventConverter: public LCEventConverter {
      public:
         bool Converting(EventSPC d1, LCEventSP d2, ConfigurationSPC conf) const override;
         static const uint32_t m_id_factory = cstr2hash("DesyTableRaw");
         private:
   };

   namespace {
      auto dummy0 = Factory<LCEventConverter>::
            Register<DesyTable2LCEventConverter>(DesyTable2LCEventConverter::m_id_factory);
   }

   static const char* EVENT_TYPE = "DesyTableRaw";

   class DesyTableLCGenericObject: public lcio::LCGenericObjectImpl {
      public:
         DesyTableLCGenericObject() {
            _typeName = EVENT_TYPE;
         }
         void setTags(std::string &s) {
            _dataDescription = s;
         }
         void setIntDataInt(std::vector<int> &vec) {
            _intVec.resize(vec.size());
            std::copy(vec.begin(), vec.end(), _intVec.begin());
         }

         std::string getTags() const {
            return _dataDescription;
         }
         const std::vector<int>& getDataInt() const {
            return _intVec;
         }
   };

   bool DesyTable2LCEventConverter::Converting(EventSPC d1, LCEventSP d2, ConfigurationSPC conf) const {
      // try to cast the Event
      auto& source = *(d1.get());
      lcio::LCEvent& result = *(d2.get());

      eudaq::RawDataEvent const * rawev = 0;
      try {
         eudaq::RawDataEvent const & rawdataevent = dynamic_cast<eudaq::RawDataEvent const &>(source);
         rawev = &rawdataevent;
      } catch (std::bad_cast& e) {
         std::cout << e.what() << std::endl;
         return false;
      }
      // should check the type
      if (rawev->GetExtendWord() != eudaq::cstr2hash(EVENT_TYPE)) {
         cout << "ERROR in DesyTable2LCEventConverter: type failed!" << endl;
         return false;
      }

      // if thereno contents -ignore
      if (rawev->NumBlocks() > 0) return true;

      LCCollectionVec *col = 0;
      string colName = "DesyTableCollection";
      string dataDesc = "TODO_description";
      time_t timestamp = 0; //
      try {
         // looking for existing collection
         col = dynamic_cast<IMPL::LCCollectionVec *>(result.getCollection(colName));
      } catch (DataNotAvailableException &e) {
         // create new collection
         col = new IMPL::LCCollectionVec(LCIO::LCGENERICOBJECT);
         result.addCollection(col, colName);
      }
      col->parameters().setValue("DataDescription", dataDesc);
      //add timestamp (set by the Producer, is EUDAQ, not real timestamp!!)
      struct tm *tms = localtime(&timestamp);
      char tmc[256];
      strftime(tmc, 256, "%a, %d %b %Y %T %z", tms);
      col->parameters().setValue("Timestamp", tmc);
      col->parameters().setValue("POS_H_MM", rawev->GetTag("POS_H_MM", "0"));
      col->parameters().setValue("POS_V_MM", rawev->GetTag("POS_V_MM", "0"));

      DesyTableLCGenericObject *obj = new DesyTableLCGenericObject;
      try {
         col->addElement(obj);
      } catch (ReadOnlyException &e) {
         cout << "ERROR in DesyTable2LCEventConverter: the collection to add is read only! skipped..." << endl;
         delete obj;
      }
      return true;
   }
}

