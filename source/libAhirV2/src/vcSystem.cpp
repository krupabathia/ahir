#include <vcIncludes.hpp>
#include <vcRoot.hpp>
#include <vcType.hpp>
#include <vcObject.hpp>
#include <vcMemorySpace.hpp>
#include <vcControlPath.hpp>
#include <vcDataPath.hpp>
#include <vcModule.hpp>
#include <vcSystem.hpp>

bool vcSystem::_error_flag = false;

vcSystem::vcSystem(string id):vcRoot(id)
{
}
void vcSystem::Print(ostream& ofile)
{
  this->Print_Pipes(ofile);

  // memory spaces
  for(map<string,vcMemorySpace*>::iterator msiter = _memory_space_map.begin();
      msiter != _memory_space_map.end();
      msiter++)
    {
      (*msiter).second->Print(ofile);
    }

  // modules
  for(map<string,vcModule*>::iterator moditer = _modules.begin();
      moditer != _modules.end();
      moditer++)
    {
      (*moditer).second->Print(ofile);
    }

  // attributes
  this->Print_Attributes(ofile);
}


void vcSystem::Print_Pipes(ostream& ofile)
{
  for(map<string,int>::iterator piter = this->_pipe_map.begin();
      piter != this->_pipe_map.end();
      piter++)
    {
      ofile << vcLexerKeywords[__PIPE] << " [" << (*piter).first << "] " << (*piter).second << endl;
    }
}

void vcSystem::Add_Module(vcModule* module)
{
  assert(this->_modules.find(module->Get_Id()) == this->_modules.end());
  this->_modules[module->Get_Id()] = module;
}

void vcSystem::Set_As_Top_Module(string module_name)
{
  vcModule* m = this->Find_Module(module_name);
  if(m == NULL)
    vcSystem::Error("did not find module " + module_name + " in the system");
  else
    this->Set_As_Top_Module(m);
}

void vcSystem::Set_As_Top_Module(vcModule* module)
{
  this->_top_module_set.insert(module);
}

bool vcSystem::Is_A_Top_Module(vcModule* m)
{
  return(this->_top_module_set.find(m) != this->_top_module_set.end());
}

void vcSystem::Add_Memory_Space(vcMemorySpace* ms)
{
  assert(ms != NULL);
  string m_id = ms->Get_Id();
  
  assert(this->_memory_space_map.find(m_id) == this->_memory_space_map.end());
  this->_memory_space_map[m_id] = ms;
}

vcMemorySpace* vcSystem::Find_Memory_Space(string module_name, string ms_name)
{
  if(module_name == "")
    return(this->Find_Memory_Space(ms_name));

  vcModule* m = this->Find_Module(module_name);
  if(m==NULL)
    return(NULL);

  return(m->Find_Memory_Space(ms_name));
}

vcMemorySpace* vcSystem::Find_Memory_Space(string ms_name)
{
  vcMemorySpace* ret_space = NULL;
  map<string, vcMemorySpace*>::iterator iter = this->_memory_space_map.find(ms_name);
  if(iter != this->_memory_space_map.end())
    ret_space = (*iter).second;
  return(ret_space);
}
vcModule* vcSystem::Find_Module(string m_name)
{
  vcModule* ret_module = NULL;
  map<string, vcModule*>::iterator iter = this->_modules.find(m_name);
  if(iter != this->_modules.end())
    ret_module = (*iter).second;
  return(ret_module);
}

void vcSystem::Elaborate()
{
  this->Check_Control_Structure();
  this->Compute_Compatibility_Labels();
  this->Compute_Maximal_Groups();

}
 

void vcSystem::Error(string err_msg)
{
  cerr << "Error: " << err_msg << endl;
  vcSystem::_error_flag = true;
}

void vcSystem::Warning(string err_msg)
{
  cerr << "Warning: " << err_msg << endl;
}


bool vcSystem::Get_Error_Flag()
{
  return(vcSystem::_error_flag);
}




void vcSystem::Check_Control_Structure()
{
  for(map<string,vcModule*>::iterator moditer = _modules.begin();
      moditer != _modules.end();
      moditer++)
    {
      (*moditer).second->Check_Control_Structure();
    }
}
void vcSystem::Compute_Compatibility_Labels()
{
  for(map<string,vcModule*>::iterator moditer = _modules.begin();
      moditer != _modules.end();
      moditer++)
    {
      (*moditer).second->Compute_Compatibility_Labels();
    }
}

void vcSystem::Compute_Maximal_Groups()
{
  for(map<string,vcModule*>::iterator moditer = _modules.begin();
      moditer != _modules.end();
      moditer++)
    {
      (*moditer).second->Compute_Maximal_Groups();
    }
}

void vcSystem::Print_Control_Structure(ostream& ofile)
{
  for(map<string,vcModule*>::iterator moditer = _modules.begin();
      moditer != _modules.end();
      moditer++)
    {
      (*moditer).second->Print_Control_Structure(ofile);
    }
}

void  vcSystem::Print_VHDL(ostream& ofile)
{
  // print modules
  for(map<string,vcModule*>::iterator moditer = _modules.begin();
      moditer != _modules.end();
      moditer++)
    {
      (*moditer).second->Print_VHDL(ofile);
    }

  this->Print_VHDL_Inclusions(ofile);
  this->Print_VHDL_Entity(ofile);
  this->Print_VHDL_Architecture(ofile);
}

void vcSystem::Print_VHDL_Test_Bench(ostream& ofile) 
{
  this->Print_VHDL_Inclusions(ofile);

  ofile << "entity " << this->Get_Id() << "_Test_Bench is -- {" << endl;
  ofile << "-- }\n end entity;" << endl;

  ofile << "architecture Default of " << this->Get_Id() << "_Test_Bench is -- {" << endl;
  this->Print_VHDL_Component(ofile);
  this->Print_VHDL_Test_Bench_Signals(ofile);
  ofile << "-- }\n begin --{" << endl;
  this->Print_VHDL_Instance(ofile);
  ofile << "-- }\n end Default;" << endl;
}

void vcSystem::Print_VHDL_Instance(ostream& ofile)
{
  ofile << this->Get_Id() << "_instance: " << this->Get_Id() << " -- {" << endl;
  ofile << "port map ( -- {" << endl;
  string comma;
  comma = this->Print_VHDL_Instance_Port_Map(comma,ofile);
  ofile << "); -- }}" << endl;
  
}

string vcSystem::Print_VHDL_Instance_Port_Map(string comma, ostream& ofile)
{
  for(set<vcModule*,vcRoot_Compare>::iterator iter = _top_module_set.begin();
      iter != _top_module_set.end();
      iter++)
    {
      comma = (*iter)->Print_VHDL_System_Instance_Port_Map(comma, ofile);
    }

  ofile << comma << endl << "clk => clk," << endl;
  ofile << "reset => reset";
  comma = ",";
  comma = this->Print_VHDL_System_Instance_Pipe_Port_Map(comma,ofile);
  return(comma);
}

string vcSystem::Print_VHDL_System_Instance_Pipe_Port_Map(string comma, ostream& ofile)
{
  for(map<string, int>::iterator pipe_iter = _pipe_map.begin();
      pipe_iter != _pipe_map.end();
      pipe_iter++)
    {
      string pipe_id = (*pipe_iter).first;
      int pipe_width = (*pipe_iter).second;
      
      int num_reads = this->Get_Num_Pipe_Reads(pipe_id);
      int num_writes = this->Get_Num_Pipe_Writes(pipe_id);
     
      if(num_reads > 0 && num_writes ==  0)
	{
	  // input pipe
	  ofile << comma << endl;
	  comma = ",";
	  ofile << pipe_id << "_pipe_write_data " 
		<< " => "
		<< pipe_id << "_pipe_write_data, "  << endl;

	  ofile << pipe_id << "_pipe_write_req => "
		<< " => "
		<< pipe_id 
		<< "_pipe_write_req, " << endl;
	  ofile << pipe_id 
		<< "_pipe_write_ack => "
		<< " => "
		<< pipe_id << "_pipe_write_ack";
	  
	}


      if(num_writes > 0 && num_reads == 0)
	{
	  // output
	  ofile << comma << endl;
	  comma = ",";
	  ofile << pipe_id << "_pipe_read_data "
		<< " => "
		<< pipe_id << "_pipe_read_data, " << endl;
	  ofile << pipe_id << "_pipe_read_req " 
		<< " => " 
		<< pipe_id << "_pipe_read_req, "  << endl;
	  ofile << pipe_id << "_pipe_read_ack "
		<< " => "
		<< pipe_id << "_pipe_read_ack ";
	}
    }
  return(comma);
}

void vcSystem::Print_VHDL_Test_Bench_Signals(ostream& ofile)
{
  ofile << "signal clk: std_logic := '0';" << endl;
  ofile << "signal reset: std_logic := '1';" << endl;
  for(set<vcModule*,vcRoot_Compare>::iterator iter = _top_module_set.begin();
      iter != _top_module_set.end();
      iter++)
    {
      (*iter)->Print_VHDL_System_Argument_Signals(ofile);
    }

  this->Print_VHDL_Pipe_Port_Signals(ofile);
}

void vcSystem::Print_VHDL_Component(ostream& ofile)
{
  ofile << "component " << this->Get_Id() << " is -- { " << endl;
  string semi_colon;
  semi_colon = this->Print_VHDL_System_Ports(semi_colon, ofile);
  ofile << "-- }\n end component;" << endl;
  
}

string vcSystem::Print_VHDL_System_Ports(string semi_colon, ostream& ofile)
{
  ofile << "port (-- {";
  for(set<vcModule*,vcRoot_Compare>::iterator iter = _top_module_set.begin();
      iter != _top_module_set.end();
      iter++)
    {
      semi_colon = (*iter)->Print_VHDL_System_Argument_Ports(semi_colon, ofile);
    }

  ofile << semi_colon << endl << "clk : in std_logic;" << endl;
  ofile << "reset : in std_logic";
  semi_colon = ";" ;
  this->Print_VHDL_Pipe_Ports(semi_colon,ofile);
  ofile << "); -- }" << endl;
  return(semi_colon);
}

void vcSystem::Print_VHDL_Entity(ostream& ofile)
{
  ofile << "entity " << this->Get_Id() << " is  -- system {" << endl;
  string semi_colon;
  semi_colon = this->Print_VHDL_System_Ports(semi_colon, ofile);
  ofile << "-- }\n end entity; " << endl;
}

void vcSystem::Print_VHDL_Pipe_Port_Signals(ostream& ofile)
{
  for(map<string, int>::iterator pipe_iter = _pipe_map.begin();
      pipe_iter != _pipe_map.end();
      pipe_iter++)
    {
      string pipe_id = (*pipe_iter).first;
      int pipe_width = (*pipe_iter).second;
      
      int num_reads = this->Get_Num_Pipe_Reads(pipe_id);
      int num_writes = this->Get_Num_Pipe_Writes(pipe_id);
     
      if(num_reads > 0 && num_writes ==  0)
	{
	  ofile << "-- write to pipe " << pipe_id << endl;
	  ofile << "signal " 
		<< pipe_id 
		<< "_pipe_write_data: in std_logic_vector(" << pipe_width-1 << " downto 0);" << endl;
	  ofile << "signal " << pipe_id << "_pipe_write_req : std_logic := '0';" << endl;
	  ofile << "signal " << pipe_id << "_pipe_write_ack : std_logic;" << endl;
	}


      if(num_writes > 0 && num_reads == 0)
	{
	  ofile << "-- read from pipe " << pipe_id << endl;
	  ofile << "signal "
		<< pipe_id << "_pipe_read_data: std_logic_vector(" << pipe_width-1 << " downto 0);" << endl;
	  ofile << "signal " << pipe_id << "_pipe_read_req : std_logic := '0';" << endl;
	  ofile << "signal " << pipe_id << "_pipe_read_ack : std_logic;" << endl;
	}
    }
}

string vcSystem::Print_VHDL_Pipe_Ports(string semi_colon, ostream& ofile)
{
  for(map<string, int>::iterator pipe_iter = _pipe_map.begin();
      pipe_iter != _pipe_map.end();
      pipe_iter++)
    {
      string pipe_id = (*pipe_iter).first;
      int pipe_width = (*pipe_iter).second;
      
      int num_reads = this->Get_Num_Pipe_Reads(pipe_id);
      int num_writes = this->Get_Num_Pipe_Writes(pipe_id);
     
      if(num_reads > 0 && num_writes ==  0)
	{
	  // input pipe
	  ofile << semi_colon << endl;
	  semi_colon = ";";
	  ofile << pipe_id << "_pipe_write_data: in std_logic_vector(" << pipe_width-1 << " downto 0);" << endl;
	  ofile << pipe_id << "_pipe_write_req : in std_logic;" << endl;
	  ofile << pipe_id << "_pipe_write_ack : out std_logic";
	}


      if(num_writes > 0 && num_reads == 0)
	{
	  // output
	  ofile << semi_colon << endl;
	  semi_colon = ";";
	  ofile << pipe_id << "_pipe_read_data: out std_logic_vector(" << pipe_width-1 << " downto 0);" << endl;
	  ofile << pipe_id << "_pipe_read_req : in std_logic;" << endl;
	  ofile << pipe_id << "_pipe_read_ack : out std_logic";
	}
    }

  return(semi_colon);
}


void vcSystem::Print_VHDL_Pipe_Signals(ostream& ofile)
{
  for(map<string, int>::iterator pipe_iter = _pipe_map.begin();
      pipe_iter != _pipe_map.end();
      pipe_iter++)
    {
      string pipe_id = (*pipe_iter).first;
      int pipe_width = (*pipe_iter).second;
      
      int num_reads = this->Get_Num_Pipe_Reads(pipe_id);
      int num_writes = this->Get_Num_Pipe_Writes(pipe_id);
     
      if(num_writes >  0)
	{
	  ofile << "-- aggregate signals for write to pipe " << pipe_id << endl;
	  ofile << "signal " << pipe_id << "_pipe_write_data: std_logic_vector(" << (num_writes*pipe_width)-1 << " downto 0);" << endl;
	  ofile << "signal " << pipe_id << "_pipe_write_req: std_logic_vector(" << num_writes-1 << " downto 0);" << endl;
	  ofile << "signal " << pipe_id << "_pipe_write_ack: std_logic_vector(" << num_writes-1 << " downto 0);" << endl;
	}

      if(num_reads > 0)
	{
	  ofile << "-- aggregate signals for read from pipe " << pipe_id << endl;
	  ofile << "signal " << pipe_id << "_pipe_read_data: std_logic_vector(" << (num_reads*pipe_width)-1 << " downto 0);" << endl;
	  ofile << "signal " << pipe_id << "_pipe_read_req: std_logic_vector(" << num_reads-1 << " downto 0);" << endl;
	  ofile << "signal " << pipe_id << "_pipe_read_ack: std_logic_vector(" << num_reads-1 << " downto 0);" << endl;
	}

      ofile << "-- the pipe itself. " << pipe_id << endl;
      ofile << "signal " << pipe_id << "_pipe_data: std_logic_vector(" << pipe_width-1 << " downto 0);" << endl;
      ofile << "signal " << pipe_id << "_pipe_req: std_logic;" << endl;
      ofile << "signal " << pipe_id << "_pipe_ack: std_logic;" << endl;
    }
}

bool vcSystem::Get_Pipe_Module_Section(string pipe_id, 
				       vcModule* caller_module, 
				       string read_or_write, 
				       int& hindex, 
				       int& lindex)
{
  bool ret_val = false;
  hindex = ((read_or_write == "read") ? this->Get_Num_Pipe_Reads(pipe_id)-1 : this->Get_Num_Pipe_Writes(pipe_id) -1);
  map<vcModule*,vector<int> >& module_map = ((read_or_write == "read") ? _pipe_read_map[pipe_id] :
					     _pipe_write_map[pipe_id]);
  for(map<vcModule*, vector<int> >::iterator iter = module_map.begin();
      iter != module_map.end();
      iter++
      )
    {
      if(caller_module == (*iter).first)
	{
	  lindex = (hindex + 1) - (*iter).second.size();
	  ret_val = true;
	  break;
	}
      else
	{
	  hindex -= (*iter).second.size();
	}
    }
  return(ret_val);
}

string vcSystem::Get_VHDL_Pipe_Interface_Port_Name(string pipe_id, string pid)
{
  return(pipe_id + "_pipe_" + pid);
}

string vcSystem::Get_Pipe_Aggregate_Section(string pipe_id,
					    string pid, 
					    int hindex, 
					    int lindex) 
{
  int data_width;
  string ret_string = this->Get_VHDL_Pipe_Interface_Port_Name(pipe_id,pid);

  // find data_width.
  if((pid.find("req") != string::npos) || (pid.find("ack") != string::npos))
    data_width = 1;
  else if(pid.find("data") != string::npos)
    data_width = this->Get_Pipe_Width(pipe_id);
  else
    assert(0); // fatal

  ret_string += "(";
  ret_string += IntToStr(((hindex+1)*data_width)-1);
  ret_string += " downto ";
  ret_string += IntToStr(lindex*data_width);
  ret_string += ")";
  return(ret_string);
}


void vcSystem::Print_VHDL_Architecture(ostream& ofile)
{

  ofile << "architecture Default of " << this->Get_Id() << " is -- system-architecture {" << endl;


  for(map<string,vcMemorySpace*>::iterator iter = _memory_space_map.begin();
      iter != _memory_space_map.end();
      iter++)
    {
      vcMemorySpace* ms  = (*iter).second;
      ofile << " -- interface signals to connect to memory space " << ms->Get_Id() << endl;
      ms->Print_VHDL_Interface_Signal_Declarations(ofile);
    }

  for(map<string,vcModule*>::iterator moditer = _modules.begin();
      moditer != _modules.end();
      moditer++)
    {
      ofile << "-- declarations related to module " << (*moditer).second->Get_Id() << endl;
      // module component declarations
      (*moditer).second->Print_VHDL_Component(ofile);

      if(!this->Is_A_Top_Module((*moditer).second))
	{

	  ofile << "-- argument signals for module " << (*moditer).second->Get_Id() << endl;
	  (*moditer).second->Print_VHDL_Argument_Signals(ofile);
	  if((*moditer).second->Get_Num_Calls() > 0)
	    {
	      ofile << endl << "-- caller side aggregated signals for module " << (*moditer).second->Get_Id() << endl;
	      (*moditer).second->Print_VHDL_Caller_Aggregate_Signals(ofile);
	    }
	}
      else if((*moditer).second->Get_Num_Calls() > 0)
	{
	  vcSystem::Error("top-module " + (*moditer).second->Get_Id() + " cannot be called from within the system!");
	}
    }

  this->Print_VHDL_Pipe_Signals(ofile);

  ofile << "-- } " << endl << "begin -- {" << endl;
  for(map<string,vcModule*>::iterator moditer = _modules.begin();
      moditer != _modules.end();
      moditer++)
    {
      ofile << "-- module " << (*moditer).second->Get_Id() << endl;
      if(!this->Is_A_Top_Module((*moditer).second))
	{
	  if((*moditer).second->Get_Num_Calls() > 0)
	    {
	      (*moditer).second->Print_VHDL_In_Arg_Disconcatenation(ofile);
	      (*moditer).second->Print_VHDL_Out_Arg_Concatenation(ofile);
	      (*moditer).second->Print_VHDL_Call_Arbiter_Instantiation(ofile);
	    }
	}

      (*moditer).second->Print_VHDL_Instance(ofile);
    }

  this->Print_VHDL_Pipe_Instances(ofile);

  for(map<string,vcMemorySpace*>::iterator iter = _memory_space_map.begin();
      iter != _memory_space_map.end();
      iter++)
    {
      vcMemorySpace* ms  = (*iter).second;
      ms->Print_VHDL_Instance(ofile);
    }

  ofile << "-- } " << endl << "end Default;" << endl;
}


void vcSystem::Print_VHDL_Pipe_Instances(ostream& ofile)
{
  for(map<string, int>::iterator pipe_iter = _pipe_map.begin();
      pipe_iter != _pipe_map.end();
      pipe_iter++)
    {
      string pipe_id = (*pipe_iter).first;
      int pipe_width = (*pipe_iter).second;
      
      int num_reads = this->Get_Num_Pipe_Reads(pipe_id);
      int num_writes = this->Get_Num_Pipe_Writes(pipe_id);
     
      if(num_reads > 0)
	{
	  // input port level..
	  ofile << pipe_id << "_ReadMux: InputPortLevel -- {" << endl;
	  ofile << "generic map( -- { " << endl;
	  ofile << "num_reqs => " << num_reads << "," << endl;
	  ofile << "data_width => " << pipe_width << "," << endl;
	  ofile << "no_arbitration => false -- }\n)" << endl;
	  ofile << "port map( -- { " << endl;
	  ofile << "req => " << pipe_id << "_pipe_read_req," << endl 
		<< "ack => " << pipe_id << "_pipe_read_ack," << endl 
		<< "data => "<< pipe_id << "_pipe_write_data," << endl 
		<< "oreq => "<< pipe_id << "_pipe_ack, -- cross-over" << endl
		<< "oack => "<< pipe_id << "_pipe_req, -- cross-over" << endl
		<< "odata => "<< pipe_id << "_pipe_data," << endl
		<< "clk => clk,"
		<< "reset => reset -- }\n ); -- }" << endl;

	  if(num_writes == 0)
	    {
	      ofile << pipe_id << "_pipe_data <= " << pipe_id << "_pipe_write_data;" << endl;
	      ofile << pipe_id << "_pipe_req <= "  << pipe_id << "_pipe_write_req; -- no cross-over"
		    << endl;
	      ofile << pipe_id << "_pipe_write_ack <= "  << pipe_id << "_pipe_ack; -- no cross-over"
		    << endl;
	    }
	}

      if(num_writes > 0)
	{
	  ofile << pipe_id << "_WriteMux: OutputPortLevel -- {" << endl;
	  ofile << "generic map( -- { " << endl;
	  ofile << "num_reqs => " << num_writes << "," << endl;
	  ofile << "data_width => " << pipe_width << "," << endl;
	  ofile << "no_arbitration => false -- }\n)" << endl;
	  ofile << "port map( -- { " << endl;
	  ofile << "req => " << pipe_id << "_pipe_write_req," << endl 
		<< "ack => " << pipe_id << "_pipe_write_ack," << endl 
		<< "data => "<< pipe_id << "_pipe_write_data," << endl 
		<< "oreq => "<< pipe_id << "_pipe_req, -- no cross-over" << endl
		<< "oack => "<< pipe_id << "_pipe_ack, -- no cross-over" << endl
		<< "odata => "<< pipe_id << "_pipe_data," << endl
		<< "clk => clk,"
		<< "reset => reset -- }\n ); -- }" << endl;

	  if(num_reads == 0)
	    {
	      ofile << pipe_id << "_pipe_read_data <= " << pipe_id << "_pipe_data;" << endl;
	      ofile << pipe_id << "_pipe_ack <= "  << pipe_id << "_pipe_read_req; -- cross-over" << endl;
	      ofile << pipe_id << "_pipe_read_ack <= "  << pipe_id << "_pipe_req; -- cross-over" << endl;
	    }
	}
    }
}

void  vcSystem::Print_VHDL_Inclusions(ostream& ofile)
{
  ofile << "library ieee;\n\
use ieee.std_logic_1164.all;\n			\
library ahir;\n					\
use ahir.memory_subsystem_package.all;\n	\
use ahir.types.all;\n				\
use ahir.subprograms.all;\n			\
use ahir.components.all;\n			\
use ahir.basecomponents.all;\n			\
use ahir.operatorpackage.all;\n";
}
