#include "eudaq/FileNamer.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/Configuration.hh"
#include "eudaq/LCEventConverter.hh"
#include <ostream>
#include <ctime>
#include <iomanip>


#include "lcio.h"
#include "EVENT/LCIO.h"
#include "IO/ILCFactory.h"
#include "IO/LCWriter.h"
#include "IMPL/LCEventImpl.h"
#include "IMPL/LCCollectionVec.h"
#include "IMPL/LCTOOLS.h"

#include "IMPL/LCRunHeaderImpl.h"

namespace eudaq {
  class AhcalLCFileWriter;

  namespace{
    auto dummy01 = Factory<FileWriter>::Register<AhcalLCFileWriter, std::string&>(cstr2hash("ahcalslcio"));
    auto dummy11 = Factory<FileWriter>::Register<AhcalLCFileWriter, std::string&&>(cstr2hash("ahcalslcio"));
  }

  class AhcalLCFileWriter : public FileWriter {
  public:
    AhcalLCFileWriter(const std::string &patt);
    void WriteEvent(EventSPC ev) override;
  private:
    bool wroteRunHeader;
    std::unique_ptr<lcio::LCWriter> m_lcwriter;
    std::string m_filepattern;
    uint32_t m_run_n;

  };

  AhcalLCFileWriter::AhcalLCFileWriter(const std::string &patt){
    m_filepattern = patt;
    wroteRunHeader = false;
  }

  void AhcalLCFileWriter::WriteEvent(EventSPC ev) {

    //std::cout<<"i am writing"<<std::endl;
    uint32_t run_n = ev->GetRunN();
    if(!m_lcwriter || m_run_n != run_n){
      try {
	m_lcwriter.reset(lcio::LCFactory::getInstance()->createLCWriter());

	std::time_t time_now = std::time(nullptr);
	char time_buff[16];
	time_buff[15] = 0;
	std::strftime(time_buff, sizeof(time_buff), "%Y%m%d_%H%M%S", std::localtime(&time_now));
	std::string time_str(time_buff);
	m_lcwriter->open(FileNamer(m_filepattern).Set('R', run_n).Set('D', time_str),
			 lcio::LCIO::WRITE_NEW);
  wroteRunHeader = false;
	m_run_n = run_n;
      } catch (const lcio::IOException &e) {
	EUDAQ_THROW(std::string("Fail to open LCIO file")+e.what());
      }
    }
    if(!m_lcwriter)
      EUDAQ_THROW("LCFileWriter: Attempt to write unopened file");

    if(ev->GetEventN() == 0){

      // create runheader and fill it
      //std::cout<<"writing runheader"<<std::endl;
      lcio::LCRunHeaderImpl* runheader = new lcio::LCRunHeaderImpl ;

      runheader->setRunNumber( run_n ) ;
      runheader->setDetectorName( std::string("AHCAL") ) ;
      std::cout <<          runheader->getRunNumber()<<std::endl;
      std::stringstream description ;
      description << "Data from AHCAL";
      runheader->setDescription( description.str() );
      m_lcwriter->writeRunHeader(runheader);
      wroteRunHeader = true;
      delete runheader;

    }
    LCEventSP lcevent(new lcio::LCEventImpl);
    LCEventConverter::Convert(ev, lcevent, GetConfiguration());

    lcevent->setTimeStamp(ev->GetTimestampBegin());

    auto timestampSec = (time_t)(ev->GetTimestampBegin() / 1000000000);

    for(const auto i : *(lcevent->getCollectionNames())){

      auto col=lcevent->getCollection(i);


      struct tm *tms = localtime(&timestampSec);
      char tmc[256];
      strftime(tmc, 256, "%a, %d %b %Y %T %z", tms);
      col->parameters().setValue("Timestamp", tmc);

    }
    m_lcwriter->writeEvent(lcevent.get());
  }
}
