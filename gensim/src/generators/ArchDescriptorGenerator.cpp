/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "arch/ArchDescription.h"
#include "isa/ISADescription.h"
#include "generators/ArchDescriptorGenerator.h"

using namespace gensim::generator;

ArchDescriptorGenerator::ArchDescriptorGenerator(GenerationManager& manager) : GenerationComponent(manager, "ArchDescriptor") {
	
}

bool ArchDescriptorGenerator::Generate() const
{
	util::cppformatstream header, source;
	GenerateHeader(header);
	GenerateSource(source);
	
	std::ofstream hfile ((Manager.GetTarget() + "/arch.h").c_str());
	hfile << header.str();
	std::ofstream sfile ((Manager.GetTarget() + "/arch.cpp").c_str());
	sfile << source.str();
	
	return true;
}

std::string ArchDescriptorGenerator::GetFunction() const
{
	return "ArchDescriptor";
}

const std::vector<std::string> ArchDescriptorGenerator::GetSources() const
{
	return {"arch.cpp"};
}

bool ArchDescriptorGenerator::GenerateSource(util::cppformatstream &str) const
{
	str << "#include \"arch.h\"\n";
	str << "#include \"decode.h\"\n";
	
	str << "using namespace gensim::" << Manager.GetArch().Name << ";";
	
	auto rfile = Manager.GetArch().GetRegFile();
	std::string entry_list;
	for(auto i : rfile.GetBanks()) {
		str << "static archsim::RegisterFileEntryDescriptor rfd_entry_" << i->ID << "(\"" << i->ID <<"\", " << i->GetIndex() << ", " << i->GetRegFileOffset() << ", " << i->GetRegisterCount() << ", " << i->GetRegisterWidth() << ", " << i->GetRegisterStride() << ");";
		if(entry_list.size()) { entry_list += ", "; }
		entry_list += "rfd_entry_" + i->ID;
	}
	
	for(auto i : rfile.GetSlots()) {
		std::string tag = "";
		if(i->HasTag()) {
			tag = i->GetTag();
		}
		str << "static archsim::RegisterFileEntryDescriptor rfd_entry_" << i->GetID() << "(\"" << i->GetID() <<"\", " << i->GetIndex() << ", " << i->GetRegFileOffset() << ", " << 1 << ", " << i->GetWidth() << ", " << i->GetWidth() << ", \"" << tag << "\");";
		if(entry_list.size()) { entry_list += ", "; }
		entry_list += "rfd_entry_" + i->GetID();
	}
	
	str << "static archsim::RegisterFileDescriptor rfd(" << rfile.GetSize() << ", {" << entry_list << "});";
	
	std::string mem_interface_list;
	for(const auto &mem_interface : Manager.GetArch().GetMemoryInterfaces().GetInterfaces()) {
		str << "static archsim::MemoryInterfaceDescriptor mem_" << mem_interface.second.GetName() << "(\"" << mem_interface.second.GetName() << "\", " << mem_interface.second.GetAddressWidthInBytes() << ", " << mem_interface.second.GetDataWidthInBytes() << ", " << (uint64_t)mem_interface.second.IsBigEndian() << ", " << mem_interface.second.GetID() << ");";
		
		if(mem_interface_list.size()) {
			mem_interface_list += ", ";
		}
		mem_interface_list += "mem_" + mem_interface.second.GetName();
	}
	
	str << "static archsim::MemoryInterfacesDescriptor misd ({" << mem_interface_list << "},\"" << Manager.GetArch().GetMemoryInterfaces().GetFetchInterface().GetName() << "\");";
	
	std::string feature_list;
	for(const auto &feature : Manager.GetArch().GetFeatures()) {
		if(!feature_list.empty()) {
			feature_list += ", ";
		}
		feature_list += "feature_" + feature.GetName();
		str << "static archsim::FeatureDescriptor feature_" << feature.GetName() << "(\"" << feature.GetName() << "\", " << feature.GetId() << ", " << feature.GetDefaultLevel() << ");";
	}
	str << "static archsim::FeaturesDescriptor features ({" << feature_list << "});";
	
	std::string isa_list;
	for(const auto &isa : Manager.GetArch().ISAs) {
		if(!isa_list.empty()) {
			isa_list += ", ";
		}
		isa_list += "isa_" + isa->ISAName;
		
		str << "namespace " << Manager.GetArch().Name << " { ";
		str << "extern archsim::ISABehavioursDescriptor get_behaviours_" << isa->ISAName << "();";
		str << "}";
		
		
		str << "static archsim::ISADescriptor isa_" << isa->ISAName << " (\"" << isa->ISAName << "\", " << (uint32_t)isa->isa_mode_id << ", [](archsim::Address addr, archsim::MemoryInterface *interface, gensim::BaseDecode &decode){ return ((gensim::" << Manager.GetArch().Name << "::Decode&)decode).DecodeInstr(addr, " << (uint32_t)isa->isa_mode_id << ", *interface); }, " << Manager.GetArch().Name << "::get_behaviours_" << isa->ISAName << "());";
		
	}
	
	str << "ArchDescriptor::ArchDescriptor() : archsim::ArchDescriptor(rfd, misd, features, {" << isa_list << "}) {}";
	
	return true;
}

bool ArchDescriptorGenerator::GenerateHeader(util::cppformatstream &str) const
{	
	str << "#ifndef ARCH_DESC_H\n";
	str << "#define ARCH_DESC_H\n";
	str << "#include <gensim/ArchDescriptor.h>\n";
	str << "#include <gensim/ThreadInstance.h>\n";
	str << "#include <util/Vector.h>\n";
	str << "namespace gensim {";
	str << "namespace " << Manager.GetArch().Name << " {";
	str << "class ArchDescriptor : public archsim::ArchDescriptor {";
	str << "public: ArchDescriptor();";
	str << "};";
	
	GenerateThreadInterface(str);
	
	
	str << "} }"; // namespace
		
	str << "\n#endif\n";
	
	return true;
}

bool ArchDescriptorGenerator::GenerateThreadInterface(util::cppformatstream &str) const {
	str << "class ArchInterface {";
	str << "public: ArchInterface(archsim::ThreadInstance *thread) : thread_(thread), reg_file_((char*)thread->GetRegisterFile()) {}";
	
	for(gensim::arch::RegBankViewDescriptor *rbank : Manager.GetArch().GetRegFile().GetBanks()) {
		std::string read_trace_string = "";
		std::string write_trace_string = "";
		if(rbank->GetRegisterIRType().VectorWidth > 1) {
			read_trace_string = ""; // not currently supported
			write_trace_string = ""; // not currently supported
		} else {
			read_trace_string = "if(trace) { thread_->GetTraceSource()->Trace_Bank_Reg_Read(true, " + std::to_string(rbank->GetIndex()) + ", idx, value); }";
			write_trace_string = "if(trace) { thread_->GetTraceSource()->Trace_Bank_Reg_Write(true, " + std::to_string(rbank->GetIndex()) + ", idx, value); }";
		}
		str << "template<bool trace=false> " << rbank->GetRegisterIRType().GetCType() << " read_register_bank_" << rbank->ID << "(uint32_t idx) const { auto value = *(" << rbank->GetRegisterIRType().GetCType() << "*)(reg_file_ + " << rbank->GetRegFileOffset() << " + (idx * " << rbank->GetRegisterStride() << ")); " << read_trace_string << " return value; }";
		str << "template<bool trace=false> void write_register_bank_" << rbank->ID << "(uint32_t idx, " << rbank->GetRegisterIRType().GetCType() << " value) { *(" << rbank->GetRegisterIRType().GetCType() << "*)(reg_file_ + " << rbank->GetRegFileOffset() << " + (idx * " << rbank->GetRegisterStride() << ")) = value; " << write_trace_string << " }";
	}
	for(gensim::arch::RegSlotViewDescriptor *slot : Manager.GetArch().GetRegFile().GetSlots()) {
		str << "template<bool trace=false> " << slot->GetIRType().GetCType() << " read_register_" << slot->GetID() << "() const { auto value = *(" << slot->GetIRType().GetCType() << "*)(reg_file_ + " << slot->GetRegFileOffset() << "); if(trace) { thread_->GetTraceSource()->Trace_Reg_Read(1, " << slot->GetIndex() << ", value); } return value; }";
		str << "template<bool trace=false> " << "void write_register_" << slot->GetID() << "(" << slot->GetIRType().GetCType() << " value) { *(" << slot->GetIRType().GetCType() << "*)(reg_file_ + " << slot->GetRegFileOffset() << ") = value; if(trace) { thread_->GetTraceSource()->Trace_Reg_Write(1, " << slot->GetIndex() << ", value); } }";
	}
	
	// read/write pc
	auto pc_descriptor = Manager.GetArch().GetRegFile().GetTaggedRegSlot("PC");
	
	str << "archsim::Address read_pc() const {";
	str << "return archsim::Address(read_register_" << pc_descriptor->GetID() << "());";
	str << "}";
	
	str << "void write_pc(archsim::Address new_pc) {";
	str << "write_register_" << pc_descriptor->GetID() << "(new_pc.Get());";
	str << "}";
	
	
	
	str << "private: "
		"archsim::ThreadInstance *thread_;"
		"char *reg_file_;";
	str << "};";
	
	return true;
}



DEFINE_COMPONENT(ArchDescriptorGenerator, arch);