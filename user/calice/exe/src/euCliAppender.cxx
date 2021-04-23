#include "eudaq/OptionParser.hh"
#include "eudaq/DataConverter.hh"
#include "eudaq/FileWriter.hh"
#include "eudaq/FileReader.hh"
#include <iostream>

#include "eudaq/DataCollector.hh"
#include "eudaq/Event.hh"

uint64_t m_lastTluTimestamp = 0;
uint64_t m_globalTluTimestamp = 0;
bool m_monotonic = 0;

void cloneEvent(const eudaq::EventSPC &ev1, const eudaq::EventSP &nev, int RunNr, uint32_t EventN, uint32_t TriggerN)
		{
	nev->SetRunN(RunNr);
	nev->SetEventN(EventN);
	nev->SetTriggerN(TriggerN, 0);
	if (m_monotonic && (ev1->GetDescription().find("TluRawDataEvent") != std::string::npos)) {
		if ((m_lastTluTimestamp > ev1->GetTimestampBegin())) {
			m_globalTluTimestamp += m_lastTluTimestamp;
		}
		m_lastTluTimestamp = ev1->GetTimestampBegin();
	}
	nev->SetTimestamp(ev1->GetTimestampBegin()+m_globalTluTimestamp, ev1->GetTimestampEnd()+m_globalTluTimestamp, 0);
	nev->SetType(ev1->GetType());
	nev->SetDeviceN(ev1->GetDeviceN());
	nev->SetEventID(ev1->GetEventID());
	nev->SetExtendWord(ev1->GetExtendWord());
	nev->SetVersion(ev1->GetVersion());
	nev->SetFlag(ev1->GetFlag());
	for (auto f : ev1->GetTags()) {
//		std::cout << "DEBUG TAG " << f.first << std::endl;
		nev->SetTag(f.first, f.second);
	}
//	if (firstFile && ev1->IsBORE())
//		nev->SetBORE();
	for (auto bi : ev1->GetBlockNumList()) {
		nev->AddBlock(bi, ev1->GetBlock(bi));
	}
	for (auto se : ev1->GetSubEvents()) {
		auto nsev = eudaq::Event::MakeShared(se->GetDescription());
		cloneEvent(se, nsev, RunNr, EventN, TriggerN);
		nev->AddSubEvent(nsev);
	}
}

int main(int /*argc*/, const char **argv) {

	eudaq::OptionParser op("EUDAQ Command Line Appender", "2.0",
			"Simply appends one file to another. The first file is untouched, the second file is updated (eventN and triggerN");
	eudaq::Option<std::string> file_input_file1(op, "i", "file1", "", "string", "input file 1");
	eudaq::Option<std::string> file_input_file2(op, "j", "file2", "", "string", "input file 2");
	eudaq::OptionFlag monotonic(op, "m", "monotonic", "adjust TLU timestmps so that it does not go back in time");

	eudaq::Option<std::string> file_output(op, "o", "output", "", "string", "output file");
	eudaq::OptionFlag iprint(op, "ip", "iprint", "enable print of input Event");

	try {
		op.Parse(argv);
	} catch (...) {
		return op.HandleMainException();
	}

	std::string infile_path_file1 = file_input_file1.Value();
	std::string infile_path_file2 = file_input_file2.Value();
	if (infile_path_file1.empty() || infile_path_file2.empty()) {
		std::cout << "Missing one of the input file" << std::endl;
		std::cout << "option --help to get help" << std::endl;
		return 1;
	}

	std::string outfile_path = file_output.Value();
	std::string type_in_file1 = infile_path_file1.substr(infile_path_file1.find_last_of(".") + 1);
	std::string type_in_file2 = infile_path_file2.substr(infile_path_file2.find_last_of(".") + 1);
	std::string type_out = outfile_path.substr(outfile_path.find_last_of(".") + 1);
	bool print_ev_in = iprint.Value();
	m_monotonic = monotonic.Value();

	if (type_in_file1 == "raw" && type_in_file2 == "raw") {
		type_in_file1 = "native";
		type_in_file2 = "native";
	}
	if (type_out == "raw")
		type_out = "native";

	eudaq::FileReaderUP reader_file1;
	eudaq::FileReaderUP reader_file2;
	eudaq::FileWriterUP writer;

	reader_file1 = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in_file1), infile_path_file1);
	reader_file2 = eudaq::Factory<eudaq::FileReader>::MakeUnique(eudaq::str2hash(type_in_file2), infile_path_file2);
	if (!type_out.empty())
		writer = eudaq::Factory<eudaq::FileWriter>::MakeUnique(eudaq::str2hash(type_out), outfile_path);

	uint32_t event_count = 0;
	uint32_t trigger_count = 0;

	int firstevent = 1;
	int firstFile = 1; //1 when reading the first file
	int file1triggers = 0;
	int file1events = 0;
	int file1runnr = 0;
	while (1) {
		auto ev1 = reader_file1->GetNextEvent(); // was ev_slow
		if (ev1) {
			event_count = ev1->GetEventN();
			trigger_count = ev1->GetTriggerN();
			file1triggers = trigger_count;
			file1events = event_count;
			file1runnr = ev1->GetRunN();
		} else {
			ev1 = reader_file2->GetNextEvent();
			if (!ev1) break;
			firstFile = 0;
			event_count = ev1->GetEventN() + file1events;
			trigger_count = ev1->GetTriggerN() + file1triggers;
		}
		//std::cout << "DEBUG number of subevents = " << ev1->GetNumSubEvent() << " triggerN=" << ev1->GetTriggerN() << "\teventN" << ev1->GetEventN()
		//		<< std::endl;

		// ev_file1->ClearFlagBit(eudaq::Event::FLAG_EORE);
//		if (ev1->IsEORE())
//			continue;
		auto nev = eudaq::Event::MakeShared(ev1->GetDescription());
		cloneEvent(ev1, nev, file1runnr, event_count, trigger_count);
		firstevent = 0;
		//std::cout << "DEBUG numblocks = " << ev1->GetNumBlock() << std::endl;
		writer->WriteEvent(std::move(nev));
	}
	// while(1) {
	//   auto ev_file2 = reader_file2->GetNextEvent(); // was ev_fast
	//   if (!ev_file2) break;
	//   if (firstevent && (ev_file2->GetEventN() == 0)) event_count++;
	//   if (firstevent && (ev_file2->GetTriggerN() == 0)) trigger_count++;
	//   firstevent = 0;
	//   std::cout<<"DEBUG number of subevents = "<< ev_file2->GetNumSubEvent() << std::endl;

	//   //create a new event
	//   // auto evt2=eudaq::Event::MakeShared(ev_file2->GetDescription());
	//   // evt2->SetType(ev_file2->GetType());

	//   if (ev_file2->GetNumSubEvent()){
	//     for (auto& subev : ev_file2->GetSubEvents()){
	// 	std::cout << "  DEBUG subevent " << subev->GetDescription() << " EVT=" << subev->GetEventN() << " \tTRG=" <<subev->GetTriggerN() << std::endl;

	// 	// subev->SetEventN(event_count + subev->GetEventN());
	// 	// subev->SetTriggerN(trigger_count + subev->GetTriggerN())
	//     }
	//   }
	//   writer->WriteEvent(std::move(ev_file2));
	// }

	// int sync_event_number = 0;
	// const uint32_t run_number = ev_file1->GetRunN();

	// while(1){
	//   if(!ev_file1)
	//     {
	//       std::cout << "No more TLU events..." << std::endl;
	//       break;
	//     }
	//   // initialise new event
	//   auto ev_sync =  eudaq::Event::MakeUnique("MimosaTlu");
	//   ev_sync->SetFlagPacket(); // copy from Ex0Tg
	//   ev_sync->SetTriggerN(ev_file1->GetTriggerN());
	//   ev_sync->SetEventN(sync_event_number);
	//   ev_sync->SetRunN(run_number);
	//   std::cout << "Sync Event created..." << std::endl;
	//   //Now get the NI events matching the TLU event trigger ID
	//   if(ev_file2){
	//     while(ev_file1->GetEventN() > ev_file2->GetEventN()){
	// 	ev_file2 = reader_file2->GetNextEvent();

	// 	if(!ev_file2){
	// 	  std::cout << "No more NI events..." << std::endl;
	// 	  break;
	// 	}
	//     }
	//   }
	//   if(!ev_file2){
	//     std::cout << "No more NI events..." << std::endl;
	//     break;
	//   }
	//   std::cout << "NI matched with TLU..." << std::endl;
	//   //Now the TLU and NI should be in ev_sync, get the DUT in sync too
	//   if(ev_dut){
	//     while(ev_file1->GetEventN() > ev_dut->GetEventN()){
	// 	ev_dut = reader_dut->GetNextEvent();
	// 	if(!ev_dut){
	// 	  std::cout << "No more DUT events..." << std::endl;
	// 	  break;
	// 	}
	//     }
	//   }
	//   if(!ev_dut){
	//     std::cout << "No more DUT events..." << std::endl;
	//     break;
	//   }
	//   std::cout << "FEI4 matched with TLU..." << std::endl;
	//   ev_sync->AddSubEvent(ev_file1);
	//   if(ev_file1->GetEventN() == ev_file2->GetEventN()) ev_sync->AddSubEvent(ev_file2);
	//   if(ev_file1->GetEventN() == ev_dut->GetEventN()) ev_sync->AddSubEvent(ev_dut);
	//   std::cout << "Sub events added..." << std::endl;
	//   if(writer && ev_file1->GetEventN() == ev_file2->GetEventN() && ev_file1->GetEventN() == ev_dut->GetEventN()){
	//     writer->WriteEvent(std::move(ev_sync));
	//     sync_event_number++;
	//   }
	//   std::cout << "Synch event written..." << std::endl;
	//   ev_file1 = reader_file1->GetNextEvent();
	//   if(!ev_file1){
	//     std::cout << "No more TLU events..." << std::endl;
	//     break;
	//   }
	//   std::cout << "Next TLU event retrieved..." << std::endl;
	// }
	return 0;
}
