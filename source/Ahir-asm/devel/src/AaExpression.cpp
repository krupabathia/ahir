using namespace std;
#include <AaIncludes.h>
#include <AaEnums.h>
#include <AaUtil.h>
#include <AaRoot.h>
#include <AaScope.h>
#include <AaType.h>
#include <AaValue.h>
#include <AaExpression.h>
#include <AaObject.h>
#include <AaStatement.h>
#include <AaModule.h>
#include <AaProgram.h>
#include <Aa2VC.h>

/***************************************** EXPRESSION  ****************************/
//---------------------------------------------------------------------
// AaExpression
//---------------------------------------------------------------------
AaExpression::AaExpression(AaScope* parent_tpr):AaRoot() 
{
  this->_scope = parent_tpr;
  this->_type = NULL; // will be determined by dependency traversal
  this->_expression_value = NULL; // if constant will be calculated in Evaluate traversal.
  this->_already_evaluated = false;
}
AaExpression::~AaExpression() {};

void AaExpression::Set_Type(AaType* t)
{
  if(this->_type == NULL)
    {
      this->_type = t;
      for(set<AaExpression*>::iterator siter = this->_targets.begin();
	  siter != this->_targets.end();
	  siter++)
	{
	  AaExpression* ref = *siter;
	  if(ref->Is("AaBinaryExpression"))
	    ((AaBinaryExpression*)ref)->Update_Type();
	}
    }
  else
    {
      if(t != this->_type)
	{
	  string err_msg = "Error: type of expression ";
	  this->Print(err_msg);
	  err_msg += " is ambiguous, is it  ";
	  this->_type->Print(err_msg);
	  err_msg += " or ";
	  t->Print(err_msg);
	  err_msg += " ? ";
	  AaRoot::Error(err_msg, this);
	}
    }
}

string AaExpression::Get_VC_Name()
{
  string ret_string = "expr_" + Int64ToStr(this->Get_Index());
  return(ret_string);
}

void AaExpression::Write_VC_Control_Path(ostream& ofile)
{
  ofile << "// CP for expression: ";
  this->Print(ofile);
  ofile << endl;

  ofile << ";;[" << this->Get_VC_Name() << "] {"
	<< "$T [dummy] " << endl
	<< "}" << endl;
}

void AaExpression::Assign_Expression_Value(AaValue* expr_value)
{
  if(this->Get_Type()->Is_Integer_Type())
    {
      AaIntValue* nv = (AaIntValue*) Make_Aa_Value(this->Get_Scope(),
						   this->Get_Type());
						   
      nv->Assign(this->Get_Type(),expr_value);
      _expression_value = nv;
    }
  else if(this->Get_Type()->Is_Float_Type())
    {
      AaFloatValue* nv = new AaFloatValue(this->Get_Scope(),
					  ((AaFloatType*)this->Get_Type())->Get_Characteristic(),
					  ((AaFloatType*)this->Get_Type())->Get_Mantissa());
      nv->Assign(this->Get_Type(),expr_value);
      _expression_value = nv;

    }
  else if(this->Get_Type()->Is("AaArrayType"))
    {
      AaArrayValue* nv = new AaArrayValue(this->Get_Scope(),
					  ((AaArrayType*)this->Get_Type())->Get_Element_Type(),
					  ((AaArrayType*)this->Get_Type())->Get_Dimension_Vector());
      assert(expr_value->Is("AaArrayValue") && 
	     (((AaArrayValue*)expr_value)->_value_vector.size() == nv->_value_vector.size()));

      nv->Assign(this->Get_Type(),expr_value);
      _expression_value = nv;
    }
}

//---------------------------------------------------------------------
// AaObjectReference
//---------------------------------------------------------------------
AaObjectReference::AaObjectReference(AaScope* parent_tpr, string object_id):AaExpression(parent_tpr)
{
  this->_object_ref_string = object_id;
  this->_search_ancestor_level = 0; 
  this->_object = NULL;
}

AaObjectReference::~AaObjectReference() {};
void AaObjectReference::Print(ostream& ofile)
{
  ofile << this->Get_Object_Ref_String();
}

void AaObjectReference::Map_Source_References(set<AaRoot*>& source_objects)
{
  AaScope* search_scope = NULL;
  if(this->Get_Search_Ancestor_Level() > 0)
    {
      search_scope = this->Get_Scope()->Get_Ancestor_Scope(this->Get_Search_Ancestor_Level());
    }
  else if(this->_hier_ids.size() > 0)
    search_scope = this->Get_Scope()->Get_Descendant_Scope(this->_hier_ids);
  else
    search_scope = this->Get_Scope();


  AaRoot* child = NULL;
  if(search_scope == NULL)
    {
      child = AaProgram::Find_Object(this->_object_root_name);
    }
  else
    {
      child = search_scope->Find_Child(this->_object_root_name);
    }

  if(child == NULL)
    {
      AaRoot::Error("did not find object reference " + this->Get_Object_Ref_String(), this);
    }
  else
    {
      // child -> obj_ref
      if(child != this)
	{
	  this->Set_Object(child);
	  
	  child->Add_Source_Reference(this);  // child -> this (this uses child as a source)
	  this->Add_Target_Reference(child);  // this  -> child (child uses this as a target)
	  
	  if(child->Is_Object())
	    source_objects.insert(child);
	}
    }
}

void AaObjectReference::Add_Target_Reference(AaRoot* referrer)
{
  this->AaRoot::Add_Target_Reference(referrer);
  if(referrer->Is("AaInterfaceObject"))
    {
      this->Set_Type(((AaInterfaceObject*)referrer)->Get_Type());
    }
}
void AaObjectReference::Add_Source_Reference(AaRoot* referrer)
{
  this->AaRoot::Add_Source_Reference(referrer);
  if(referrer->Is("AaInterfaceObject"))
    {
      this->Set_Type(((AaInterfaceObject*)referrer)->Get_Type());
    }
}

void AaObjectReference::PrintC(ofstream& ofile, string tab_string)
{
  assert(this->Get_Object());

  if(this->Get_Object()->Is_Object())
    {// this refers to an object
      if(((AaObject*)(this->Get_Object()))->Get_Scope() != NULL)
	ofile << ((AaObject*)this->Get_Object())->Get_Scope()->Get_Struct_Dereference();
    }
  else if(this->Get_Object()->Is_Statement())
    {// this refers to a statement
      ofile << this->Get_Scope()->Get_Struct_Dereference();
    }
  else if(this->Get_Object()->Is_Expression())
    { // this refers to an object reference?
      ofile << ((AaExpression*)this->Get_Object())->Get_Scope()->Get_Struct_Dereference();
    }
}

void AaObjectReference::Evaluate()
{
  AaValue* expr_value;
  if(!this->_already_evaluated)
    {
      if(this->_object->Is_Expression())
	{
	  ((AaExpression*)this->_object)->Evaluate();
	  expr_value = ((AaExpression*)this->_object)->Get_Expression_Value();
	}
      else if(this->_object->Is("AaConstantObject"))
	{
	  expr_value = ((AaConstantObject*)_object)->Get_Value()->Get_Expression_Value();
	}
    }

  this->_already_evaluated = true;
  this->Assign_Expression_Value(expr_value);
}

//---------------------------------------------------------------------
// AaConstantLiteralReference: public AaObjectReference
//---------------------------------------------------------------------
AaConstantLiteralReference::AaConstantLiteralReference(AaScope* parent_tpr, 
						       string literal_string,
						       vector<string>& literals):
  AaObjectReference(parent_tpr,literal_string) 
{
  for(unsigned int i= 0; i < literals.size(); i++)
    this->_literals.push_back(literals[i]);
};
AaConstantLiteralReference::~AaConstantLiteralReference() {};
void AaConstantLiteralReference::PrintC(ofstream& ofile, string tab_string)
{
  ofile << tab_string;
  if(this->_literals.size() > 0)
    {
      ofile << "{ ";
      ofile << this->_literals[0];
      for(unsigned int i= 1; i < this->_literals.size(); i++)
	ofile << ", " << this->_literals[i];
      ofile << "} ";
    }
  else
    {
      ofile << this->Get_Object_Ref_String() << " ";
    }
}

void AaConstantLiteralReference::Write_VC_Control_Path( ostream& ofile)
{
  // null region.
}

void AaConstantLiteralReference::Evaluate()
{
  if(!_already_evaluated)
    {
      assert(this->_type);
      _expression_value = Make_Aa_Value(this->Get_Scope(), this->Get_Type(), _literals);
      _already_evaluated = true;
    }
}

void AaConstantLiteralReference::Write_VC_Constant_Wire_Declarations(ostream& ofile)
{
  ofile << "// Constant-declaration for expression: ";
  this->Print(ofile);
  ofile << endl;

  Write_VC_Constant_Declaration(this->Get_VC_Constant_Name(),
				this->Get_Type(),
				this->_expression_value,
				ofile);
}

//---------------------------------------------------------------------
//AaSimpleObjectReference
//---------------------------------------------------------------------
AaSimpleObjectReference::AaSimpleObjectReference(AaScope* parent_tpr, string object_id):AaObjectReference(parent_tpr, object_id) {};
AaSimpleObjectReference::~AaSimpleObjectReference() {};
void AaSimpleObjectReference::Set_Object(AaRoot* obj)
{
  if(obj->Is_Object())
    this->Set_Type(((AaObject*)obj)->Get_Type());
  else if(obj->Is_Expression())
    {
      AaProgram::Add_Type_Dependency(this,obj);
      this->Add_Target((AaExpression*) obj);
    }
  this->_object = obj;
}
void AaSimpleObjectReference::PrintC(ofstream& ofile, string tab_string)
{
  ofile << "(";
  this->AaObjectReference::PrintC(ofile,tab_string);
  ofile << this->Get_Object_Root_Name() << ").__val";
}

string AaSimpleObjectReference::Get_VC_Driver_Name()
{
  if(this->_object == NULL)
    {// implicit variable.
      return(this->AaExpression::Get_VC_Driver_Name());
    }
  else if(this->_object->Is_Object())
    {
      // if it points to an object, get the object's name
      // to avoid double declaration...
      if(this->_object->Is("AaInterfaceObject"))
	return(this->_object->Get_VC_Name());
      else
	return(this->AaExpression::Get_VC_Driver_Name());
    }
  else if(this->_object->Is_Expression())
    {
      return(((AaExpression*)this->_object)->Get_VC_Driver_Name());
    }
  else if(this->_object->Is_Statement())
    {
      return(To_Alphanumeric(this->_object_ref_string));
    }
  else
    assert(0);
}

string AaSimpleObjectReference::Get_VC_Receiver_Name()
{
  // _object can be either an expression.
  if(this->_object == NULL)
    {
      return(this->AaExpression::Get_VC_Receiver_Name());
    }
  else if(this->_object->Is_Object())
    {
      if(this->_object->Is("AaInterfaceObject"))
	return(this->_object->Get_VC_Name());
      else
	return(this->AaExpression::Get_VC_Receiver_Name());
    }
  else if(this->_object->Is_Expression())
    {
      return(((AaExpression*)this->_object)->Get_VC_Receiver_Name());
    }
  else if(this->_object->Is_Statement())
    {
      return(To_Alphanumeric(this->_object_ref_string));
    }
  else
    assert(0);
}


string AaSimpleObjectReference::Get_VC_Constant_Name()
{
 if(this->_object == NULL)
    {// implicit variable.
      return(this->AaExpression::Get_VC_Constant_Name());
    }
  else if(this->_object->Is_Object())
    {
      // if it points to an object, get the object's name
      // to avoid double declaration...
      if(this->_object->Is("AaInterfaceObject"))
	return(this->_object->Get_VC_Name());
      else
	return(this->AaExpression::Get_VC_Constant_Name());
    }
  else if(this->_object->Is_Expression())
    {
      return(((AaExpression*)this->_object)->Get_VC_Constant_Name());
    }
  else if(this->_object->Is_Statement())
    {
      return(To_Alphanumeric(this->_object_ref_string));
    }
  else
    assert(0);
}


void AaSimpleObjectReference::Write_VC_Control_Path( ostream& ofile)
{
  string ps;
  this->AaRoot::Print(ps);



  if(!this->Is_Constant())
    {
      ofile << "// CP for expression: ";
      this->Print(ofile);
      ofile << endl;

      // if this is a statement...
      if(this->Is_Implicit_Variable_Reference())
	{
	  // do nothing..
	}
      // else, if the object being referred to is 
      // a storage object, then it is a load operation,
      // instantiate a series r-a-r-a chain..
      // if is_store is set, instantiate a store operation
      // as well.
      else if(this->_object->Is("AaStorageObject"))
	{
	  
	  ofile << ";;[" << this->Get_VC_Name() << "] { // load: " << ps  << endl;
	  ofile << "$T [rr] $T [ra] $T[cr] $T[ca]" << endl;
	  ofile << "}" << endl;
	}
      
      
      // else if the object being referred to is
      // a pipe, instantiate a series r-a
      // chain for the inport operation
      else if(this->_object->Is("AaPipeObject"))
	{
	  ofile << ";;[" << this->Get_VC_Name() << "] { // pipe read: " << ps << endl;
	  ofile << "$T [req] $T [ack] " << endl;
	  ofile << "}" << endl;
	}
    }
}


void AaSimpleObjectReference::Write_VC_Control_Path_As_Target( ostream& ofile)
{

  ofile << "// CP for expression: ";
  this->Print(ofile);
  ofile << endl;

  // else, if the object being referred to is 
  // a storage object, then it is a load operation,
  // instantiate a series r-a-r-a chain..
  // if is_store is set, instantiate a store operation
  // as well.
  if(this->_object == NULL)
    {
      // nothing.
    }
  else if(this->_object->Is("AaStorageObject"))
    {
      ofile << ";;[" << this->Get_VC_Name() << "] { // store ";
      this->Print(ofile);
      ofile << endl;
      ofile << "$T [srr] $T [sra] $T[scr] $T[sca]" << endl;
      ofile << "}" << endl;
    }
  // else if the object being referred to is
  // a pipe, instantiate a series r-a
  // chain for the inport operation
  else if(this->_object->Is("AaPipeObject"))
    {
      ofile << ";;[" << this->Get_VC_Name() << "] { // pipe write ";
      this->Print(ofile);
      ofile << endl;
      ofile << "$T [pipe_wreq] $T [pipe_wack] " << endl;
      ofile << "}" << endl;
    }
}

bool AaSimpleObjectReference::Is_Implicit_Object() 
{
  return(this->_object == NULL);
}

// return true if the expression points
// to an implicitly defined variable reference.
// (either a statement or an interface object).
bool AaSimpleObjectReference::Is_Implicit_Variable_Reference()
{
  return((this->_object == NULL) ||
	 this->_object->Is("AaInterfaceObject") ||
	 this->_object->Is_Statement() ||
	 (this->_object->Is_Expression() && 
	  ((AaExpression*)this->_object)->Is_Implicit_Variable_Reference()));
}

void AaSimpleObjectReference::Evaluate()
{
  if(this->_object && this->_object->Is_Expression())
    ((AaExpression*)(this->_object))->Evaluate();

  if(this->_object && this->_object->Is_Constant())
    {
      if(this->_object->Is("AaConstantObject"))
	{
	  this->Assign_Expression_Value(((AaConstantObject*)_object)->Get_Expression_Value());
	}
      else if(this->_object->Is_Expression())
	{
	  this->Assign_Expression_Value(((AaExpression*)_object)->Get_Expression_Value());
	}
    }
}


void AaSimpleObjectReference::Write_VC_Constant_Wire_Declarations(ostream& ofile)
{
  if(this->Is_Constant() && !this->Is_Implicit_Variable_Reference())
    Write_VC_Constant_Declaration(this->Get_VC_Constant_Name(),
				  this->Get_Type(),
				  this->Get_Expression_Value(),
				  ofile);
}
void AaSimpleObjectReference::Write_VC_Wire_Declarations(bool skip_immediate, ostream& ofile)
{
  if(!skip_immediate && !this->Is_Constant() && !this->Is_Implicit_Variable_Reference())
    {
      Write_VC_Wire_Declaration(this->Get_VC_Driver_Name(),
				this->Get_Type(),
				ofile);
      if(this->_object->Is("AaStorageObject"))
	{
	  AaUintType* t = AaProgram::Make_Uinteger_Type(1);
	  string v = "_b0";
	  Write_VC_Constant_Pointer_Declaration(((AaStorageObject*)this->_object)->Get_VC_Memory_Space_Name(),
						this->Get_VC_Wire_Name() + "_addr",
						t,
						v,
						ofile);
	}
    }
}

void AaSimpleObjectReference::Write_VC_Wire_Declarations_As_Target(ostream& ofile)
{

  if(!this->Is_Constant() )
    {

      // if _object is a statement, declare it, otherwise not.
      if(this->_object->Is_Statement())
	{
	  Write_VC_Wire_Declaration(this->Get_VC_Receiver_Name(),
				    this->Get_Type(),
				    ofile);
	}

      // if _object is a storage object, then,
      // make a single-bit constant pointer
      // for use in subsequence store instance.
      if(this->_object->Is("AaStorageObject"))
	{
	  AaUintType* t = AaProgram::Make_Uinteger_Type(1);
	  string v = "_b0";
	  Write_VC_Constant_Pointer_Declaration(((AaObject*)this->_object)->Get_VC_Memory_Space_Name(),
						this->Get_VC_Wire_Name() + "_addr",
						t,
						v,
						ofile);
	}
    }
}

void AaSimpleObjectReference:: Write_VC_Datapath_Instances_As_Target( ostream& ofile, AaExpression* source) 
{
  if(!this->Is_Constant()  && !this->Is_Implicit_Variable_Reference())
    {
      if(this->_object->Is("AaStorageObject"))
	{
	  // a store operator..
	  Write_VC_Store_Operator((AaStorageObject*)this->_object,
				  this->Get_VC_Datapath_Instance_Name(),
				  source->Get_VC_Driver_Name(),
				  this->Get_VC_Wire_Name() + "_addr",
				  ofile);
	}
      else if(this->_object->Is("AaPipeObject"))
	{
	  string src_name = (source->Is_Constant() ? source->Get_VC_Constant_Name() :
			     source->Get_VC_Driver_Name());
	  // io write.
	  Write_VC_IO_Output_Port((AaPipeObject*) this->_object,
				  this->Get_VC_Datapath_Instance_Name(),
				  src_name,
				  ofile);
	}
    }
}

void AaSimpleObjectReference:: Write_VC_Datapath_Instances(AaExpression* target,  ostream& ofile) 
{
  if(!this->Is_Constant() && !this->Is_Implicit_Variable_Reference())
    {
      if(this->_object->Is("AaStorageObject"))
	{
	  // a store operator..
	  Write_VC_Load_Operator((AaStorageObject*)this->_object,
				 this->Get_VC_Datapath_Instance_Name(),
				 (target != NULL ? target->Get_VC_Receiver_Name() : 
				  this->Get_VC_Receiver_Name()),
				 this->Get_VC_Wire_Name() + "_addr",
				 ofile);
	}
      else if(this->_object->Is("AaPipeObject"))
	{
	  // io write.
	  Write_VC_IO_Input_Port((AaPipeObject*) this->_object,
				 this->Get_VC_Datapath_Instance_Name(),
				 (target != NULL ? target->Get_VC_Name() : this->Get_VC_Receiver_Name()),
				 ofile);
	}
    }
}
void AaSimpleObjectReference::Write_VC_Links(string hier_id, ostream& ofile) 
{
  if(!this->Is_Constant() && !this->Is_Implicit_Variable_Reference())
    {
      ofile << "// CP-DP links for expression: ";
      this->Print(ofile);
      ofile << endl;


      vector<string> reqs;
      vector<string> acks;
      string inst_name = this->Get_VC_Datapath_Instance_Name();
      if(this->_object->Is("AaStorageObject"))
	{
	  reqs.push_back(hier_id + "/" + this->Get_VC_Name() + "/rr");
	  reqs.push_back(hier_id + "/" + this->Get_VC_Name() + "/cr");
	  acks.push_back(hier_id + "/" + this->Get_VC_Name() + "/ra");
	  acks.push_back(hier_id + "/" + this->Get_VC_Name() + "/ca");
	}
      else if(this->_object->Is("AaPipeObject"))
	{
	  reqs.push_back(hier_id + "/" + this->Get_VC_Name() + "/req");
	  acks.push_back(hier_id + "/" + this->Get_VC_Name() + "/ack");
	}
      Write_VC_Link(inst_name, reqs,acks,ofile);
    }
}
void AaSimpleObjectReference:: Write_VC_Links_As_Target(string hier_id, ostream& ofile) 
{
  if(!this->Is_Constant() && !this->Is_Implicit_Variable_Reference())
    {

      ofile << "// CP-DP links for expression: ";
      this->Print(ofile);
      ofile << endl;

      vector<string> reqs;
      vector<string> acks;
      string inst_name = this->Get_VC_Datapath_Instance_Name();
      if(this->_object->Is("AaStorageObject"))
	{
	  reqs.push_back(hier_id + "/" + this->Get_VC_Name() + "/srr");
	  reqs.push_back(hier_id + "/" + this->Get_VC_Name() + "/scr");
	  acks.push_back(hier_id + "/" + this->Get_VC_Name() + "/sra");
	  acks.push_back(hier_id + "/" + this->Get_VC_Name() + "/sca");
	}
      else if(this->_object->Is("AaPipeObject"))
	{
	  reqs.push_back(hier_id + "/" + this->Get_VC_Name() + "/pipe_wreq");
	  acks.push_back(hier_id + "/" + this->Get_VC_Name() + "/pipe_wack");
	}
      Write_VC_Link(inst_name, reqs,acks,ofile);
    }
}

//---------------------------------------------------------------------
// AaArrayObjectReference
//---------------------------------------------------------------------
AaArrayObjectReference::AaArrayObjectReference(AaScope* parent_tpr, 
					       string object_id, 
					       vector<AaExpression*>& index_list):AaObjectReference(parent_tpr,object_id)
{
  for(unsigned int i  = 0; i < index_list.size(); i++)
    this->_indices.push_back(index_list[i]);
}
AaArrayObjectReference::~AaArrayObjectReference()
{
}
void AaArrayObjectReference::Print(ostream& ofile)
{
  ofile << this->Get_Object_Ref_String();
  ofile << "[";
  for(unsigned int i = 0; i < this->Get_Number_Of_Indices(); i++)
    {
      if(i > 0)
	ofile << " ";
      this->Get_Array_Index(i)->Print(ofile);
    }
  ofile << "]";
}
AaExpression*  AaArrayObjectReference::Get_Array_Index(unsigned int idx)
{
  assert (idx < this->Get_Number_Of_Indices());
  return(this->_indices[idx]);
}

void AaArrayObjectReference::Set_Object(AaRoot* obj) 
{
  bool ok_flag = false;
  if(obj->Is_Object())
    {
      if(((AaObject*)obj)->Get_Type() && 
	 ((AaObject*)obj)->Get_Type()->Is("AaArrayType"))
	{
	  if(((AaArrayType*)(((AaObject*)obj)->Get_Type()))->Get_Number_Of_Dimensions() == this->_indices.size())
	    {
	      this->_object = obj;
	      this->Set_Type(((AaArrayType*)(((AaObject*)obj)->Get_Type()))->Get_Element_Type());
	      ok_flag = true;
	    }
	}
    }
  if(!ok_flag)
    {
      AaRoot::Error("type mismatch in object reference " + this->Get_Object_Ref_String(),this);
    }
}


void AaArrayObjectReference::Map_Source_References(set<AaRoot*>& source_objects)
{
  this->AaObjectReference::Map_Source_References(source_objects);
  for(unsigned int i=0; i < this->_indices.size(); i++)
    this->_indices[i]->Map_Source_References(source_objects);
}

void AaArrayObjectReference::PrintC(ofstream& ofile, string tab_string)
{
  ofile << "(";
  this->AaObjectReference::PrintC(ofile,tab_string);
  ofile << this->Get_Object_Root_Name();
  for(unsigned int i = 0; i < this->Get_Number_Of_Indices(); i++)
    {
      ofile << "[";
      this->Get_Array_Index(i)->PrintC(ofile,"");
      ofile << "]";
    }
  ofile << ").__val";
}

void AaArrayObjectReference::Write_VC_Control_Path( ostream& ofile)
{
  string ps;
  this->AaRoot::Print(ps);

  if(!this->Is_Constant())
    {

      ofile << "// CP for expression: ";
      this->Print(ofile);
      ofile << endl;

      // else, if the object being referred to is 
      // a storage object, then it is a load operation,
      // instantiate a series r-a-r-a chain..
      if(this->_object->Is("AaStorageObject") || this->_object->Is("AaConstantObject"))
	{
	  
	  ofile << ";;[" << this->Get_VC_Name() << "] { // array object reference: " << ps << endl;
	  this->Write_VC_Address_Gen_Control_Path(ofile);
	  ofile << "$T [rr] $T [ra] $T[cr] $T[ca]" << endl;
	  ofile << "}" << endl;
	}
      // this should never happen!
      else
	{
	  AaRoot::Error("illegal array reference", this);
	}
    }
}


void AaArrayObjectReference::Write_VC_Address_Gen_Control_Path(ostream& ofile)
{
  string ps;
  this->AaRoot::Print(ps);

  ofile << ";;[" << this->Get_VC_Name() << "_AddressGen] { // address generation for " << ps << endl;

  // in parallel, compute each of the indices..
  ofile << "// calculate index1 = idx1*dim1, index2 = idx2*dim2 ... " << endl;
  ofile << "||[IndexGen] { // indices" << endl;
  for(int idx = 0; idx < _indices.size(); idx++)
    {
      _indices[idx]->Write_VC_Control_Path(ofile);
    }
  ofile << "}" << endl;
  
  // followed by a computation of the address
  // as a weighted sum of the computed indices..
  if(_indices.size() > 1)
    {
      AaRoot::Error("Aa2VC currently does not support multiple dimensional arrays", this);
      //\todo:  need to instantiate a tree of adders here..
    }

  // resize operation..
  if(!_indices[0]->Is_Constant())
    ofile << "$T [arr] $T [ara] $T [acr] $T [aca] // resize " << endl;

  ofile << "}" << endl;
}

void AaArrayObjectReference::Write_VC_Control_Path_As_Target( ostream& ofile)
{
  this->Write_VC_Control_Path(ofile);
}

void AaArrayObjectReference::Evaluate()
{
  
  AaArrayType* at;
  AaType* t = NULL;
  if(this->_object->Is_Expression())
    {
      t = ((AaExpression*)(this->_object))->Get_Type();
    }
  else if(this->_object->Is_Object())
    {
      t = ((AaObject*)(this->_object))->Get_Type();
    }

  assert(t->Is("AaArrayType"));
  
  at = (AaArrayType*)t;

  if(!_already_evaluated)
    {
      _already_evaluated = true;
      bool all_indices_constants = true;
      vector<int> index_vector;
      for(int idx = 0; idx < _indices.size(); idx++)
	{

	  // need to evaluate the indices!
	  if(!_indices[idx]->Get_Type())
	    _indices[idx]->Set_Type(AaProgram::Make_Uinteger_Type(at->Get_Dimension(idx)));
	  _indices[idx]->Evaluate();


	  if(!_indices[idx]->Is_Constant())
	    {
	      all_indices_constants = false;
	    }
	  else
	    index_vector.push_back(_indices[idx]->Get_Expression_Value()->To_Integer());
	}

      AaValue* expr_value = NULL;
      if(this->_object->Is_Expression())
	{
	  ((AaExpression*)this->_object)->Evaluate();
	}
      else if(this->_object->Is_Object() && this->_object->Is_Constant())
	{
	  expr_value = ((AaObject*)(this->_object))->Get_Value()->Get_Expression_Value();
	}

      if(!all_indices_constants || !this->_object->Is_Constant())
	return;
      
      assert(expr_value != NULL && expr_value->Is("AaArrayValue"));
      this->Assign_Expression_Value(((AaArrayValue*)expr_value)->Get_Element(index_vector));
    }
}

void AaArrayObjectReference::Write_VC_Constant_Wire_Declarations(ostream& ofile)
{

  if(this->Is_Constant())
    {
      ofile << "// constant-declarations for expression: ";
      this->Print(ofile);
      ofile << endl;

      Write_VC_Constant_Declaration(this->Get_VC_Constant_Name(),
				    this->Get_Type(),
				    this->Get_Expression_Value(),
				    ofile);
    }
  else
    {
      for(int idx = 0; idx < _indices.size(); idx++)
	{
	  _indices[idx]->Write_VC_Constant_Wire_Declarations(ofile);
	}
    }
}

void AaArrayObjectReference::Write_VC_Wire_Declarations(bool skip_immediate, ostream& ofile)
{
  assert(this->_object->Is_Object());

  if(this->Is_Constant())
    return;

  for(int idx = 0; idx < _indices.size(); idx++)
    {
      _indices[idx]->Write_VC_Wire_Declarations(false,ofile);
    }
  // the final address.
  assert(this->Get_Type()->Is("AaArrayType"));
  AaUintType* t = AaProgram::Make_Uinteger_Type(CeilLog2(((AaArrayType*)this->Get_Type())->Number_Of_Elements()));


  ofile << "// wire-declarations for expression: ";
  this->Print(ofile);
  ofile << endl;


  if(!_indices[0]->Is_Constant())
    Write_VC_Pointer_Declaration(((AaStorageObject*)this->_object)->Get_VC_Name(),
				 this->Get_VC_Name() + "_read_ptr",
				 t,
				 ofile);
  else
    {
      string v = _indices[0]->Get_Expression_Value()->To_VC_String();
      Write_VC_Constant_Pointer_Declaration(((AaStorageObject*)this->_object)->Get_VC_Memory_Space_Name(),
					    this->Get_VC_Name() + "_constant_read_ptr",
					    t,
					    v,
					    ofile);
					  
    }
  // the final load-data.
  if(!skip_immediate)
    Write_VC_Wire_Declaration(this->Get_VC_Driver_Name(),
			      ((AaArrayType*)this->Get_Type())->Get_Element_Type(),
			      ofile);
  
}

void AaArrayObjectReference::Write_VC_Wire_Declarations_As_Target(ostream& ofile)
{

  if(this->Is_Constant())
    return;

  assert(this->_object->Is("AaStorageObject"));

  for(int idx = 0; idx < _indices.size(); idx++)
    {
      _indices[idx]->Write_VC_Wire_Declarations(false,ofile);
    }
  // the final address.
  assert(this->Get_Type()->Is("AaArrayType"));
  AaUintType* t = AaProgram::Make_Uinteger_Type(CeilLog2(((AaArrayType*)this->Get_Type())->Number_Of_Elements()));

  ofile << "// wire-declarations for expression: ";
  this->Print(ofile);
  ofile << endl;


  if(!_indices[0]->Is_Constant())
    Write_VC_Pointer_Declaration(((AaStorageObject*)this->_object)->Get_VC_Memory_Space_Name(),
				 this->Get_VC_Name() + "_write_ptr",
				 t,
				 ofile);
  else
    {
      string v = _indices[0]->Get_Expression_Value()->To_VC_String();
      Write_VC_Constant_Pointer_Declaration(((AaStorageObject*)this->_object)->Get_VC_Memory_Space_Name(),
					    this->Get_VC_Name() + "_constant_write_ptr",
					    t,
					    v,
					    ofile);
    }
}

void AaArrayObjectReference::Write_VC_Datapath_Instances_As_Target( ostream& ofile, AaExpression* source)
{

  assert(this->_object && this->_object->Is("AaStorageObject"));
    return;


  for(int idx = 0; idx < _indices.size(); idx++)
    {
      _indices[idx]->Write_VC_Datapath_Instances(NULL,ofile);
    }

  // one resize instance
  string index_addr;
  if(_indices[0]->Is_Constant())
    {
      index_addr = this->Get_VC_Wire_Name() + "_write_ptr";
    }
  else
    {
      index_addr =  this->Get_VC_Name() + "_constant_write_ptr";
    }
    
  AaUintType* t = AaProgram::Make_Uinteger_Type(CeilLog2(((AaArrayType*)this->Get_Type())->Number_Of_Elements()));

  ofile << "// data-path-instances for expression: ";
  this->Print(ofile);
  ofile << endl;

  if(!_indices[0]->Is_Constant())
    {
      Write_VC_Unary_Operator(__NOP,
			      this->Get_VC_Name() + "_addr_resize",
			      _indices[0]->Get_VC_Wire_Name(),
			      _indices[0]->Get_Type(),
			      index_addr,
			      t,
			      ofile);
    }


  // one store instance, 
  Write_VC_Store_Operator((AaStorageObject*)this->_object,
			  this->Get_VC_Datapath_Instance_Name(),
			  source->Get_VC_Name(),
			  index_addr,
			  ofile);

}
void AaArrayObjectReference::Write_VC_Datapath_Instances(AaExpression* target, ostream& ofile)
{

  if(this->Is_Constant())
    return;


  for(int idx = 0; idx < _indices.size(); idx++)
    {
      _indices[idx]->Write_VC_Datapath_Instances(NULL,ofile);
    }

  // one resize instance
  string index_addr;
  if(_indices[0]->Is_Constant())
    {
      index_addr = this->Get_VC_Wire_Name() + "_read_ptr";
    }
  else
    {
      index_addr =  this->Get_VC_Name() + "_constant_read_ptr";
    }
    
  AaUintType* t = AaProgram::Make_Uinteger_Type(CeilLog2(((AaArrayType*)this->Get_Type())->Number_Of_Elements()));

  ofile << "// data-path-instances for expression: ";
  this->Print(ofile);
  ofile << endl;


  if(this->_object->Is("AaStorageObject"))
    {
      if(!_indices[0]->Is_Constant())
	{
	  Write_VC_Unary_Operator(__NOP,
				  this->Get_VC_Name() + "_addr_resize",
				  _indices[0]->Get_VC_Wire_Name(),
				  _indices[0]->Get_Type(),
				  index_addr,
				  t,
				  ofile);
	}
      
      // one store instance, 
      Write_VC_Load_Operator((AaStorageObject*)this->_object,
			     this->Get_VC_Datapath_Instance_Name(),
			     (target != NULL ? target->Get_VC_Name() : this->Get_VC_Name()),
			     index_addr,
			     ofile);
    }
  else if(this->_object->Is("AaConstantObject"))
    {
      //\todo. instantiate a bit-sel operator...
    }
}

void AaArrayObjectReference::Write_VC_Links(string hier_id, ostream& ofile)
{
  if(this->Is_Constant())
    return;

  // index calculation links.
  for(int idx = 0; idx < _indices.size(); idx++)
    {
      _indices[idx]->Write_VC_Links(hier_id + "/" + this->Get_VC_Name() + "_AddressGen/IndexGen",ofile);
    }

  ofile << "// CP-DP links for expression: ";
  this->Print(ofile);
  ofile << endl;

  vector<string> acks, reqs;
  // link to resize 
  if(!_indices[0]->Is_Constant())
    {
      string inst_name = this->Get_VC_Name() + "_addr_resize";

      reqs.push_back(hier_id + "/" + this->Get_VC_Name() + "_AddressGen/arr");
      reqs.push_back(hier_id + "/" + this->Get_VC_Name() + "_AddressGen/acr");

      acks.push_back(hier_id + "/" + this->Get_VC_Name() + "_AddressGen/ara");
      acks.push_back(hier_id + "/" + this->Get_VC_Name() + "_AddressGen/aca");
      Write_VC_Link(inst_name,reqs,acks,ofile);

      reqs.clear();
      acks.clear();
    }


  // and link to load/constant bit-sel..
  string inst_name = this->Get_VC_Datapath_Instance_Name();
  reqs.push_back(hier_id + "/" + this->Get_VC_Name() + "/rr");
  reqs.push_back(hier_id + "/" + this->Get_VC_Name() + "/cr");
  acks.push_back(hier_id + "/" + this->Get_VC_Name() + "/ra");
  acks.push_back(hier_id + "/" + this->Get_VC_Name() + "/ca");
  Write_VC_Link(inst_name,reqs,acks,ofile);
}
void AaArrayObjectReference::Write_VC_Links_As_Target(string hier_id, ostream& ofile)
{
  this->Write_VC_Links(hier_id,ofile);
}


//---------------------------------------------------------------------
// type cast expression (is unary)
//---------------------------------------------------------------------
AaTypeCastExpression::AaTypeCastExpression(AaScope* parent, AaType* ref_type,AaExpression* rest):AaExpression(parent)
{
  this->_to_type = ref_type;
  this->_type = ref_type;
  this->_rest = rest;
  if(rest)
    rest->Add_Target(this);
}

AaTypeCastExpression::~AaTypeCastExpression() {};
void AaTypeCastExpression::Print(ostream& ofile)
{
  ofile << "( (" ;
  this->Get_To_Type()->Print(ofile);
  ofile << ") ";
  this->Get_Rest()->Print(ofile);
  ofile << " )";
}


void AaTypeCastExpression::Write_VC_Control_Path(ostream& ofile)
{
  string ps;
  this->AaRoot::Print(ps);


  if(!this->Is_Constant())
    {
      ofile << "// control-path for expression: ";
      this->Print(ofile);
      ofile << endl;

      ofile << ";;[" << this->Get_VC_Name() << "] { // type-cast expression: " << ps << endl;
      this->_rest->Write_VC_Control_Path(ofile);
      ofile << "$T [rr] $T [ra] $T [cr] $T [ca] // int<->float conversion. " << endl;
      ofile << "}" << endl;
    }
}

void AaTypeCastExpression::Evaluate()
{
  if(!_already_evaluated)
    {
      _already_evaluated = true;
      this->_rest->Evaluate();
      if(this->_rest->Is_Constant())
	this->Assign_Expression_Value(this->_rest->Get_Expression_Value());
    }
}

void AaTypeCastExpression::Write_VC_Constant_Wire_Declarations(ostream& ofile)
{
  if(this->Is_Constant())
    {

      ofile << "// constant-declarations for expression: ";
      this->Print(ofile);
      ofile << endl;
      Write_VC_Constant_Declaration(this->Get_VC_Constant_Name(),
				    this->Get_Type(),
				    this->Get_Expression_Value(),
				    ofile);
    }
  else
    {
      this->_rest->Write_VC_Constant_Wire_Declarations(ofile);
    }
}
void AaTypeCastExpression::Write_VC_Wire_Declarations(bool skip_immediate, ostream& ofile)
{
  if(!this->Is_Constant() && !skip_immediate)
    {

      this->_rest->Write_VC_Wire_Declarations(false,ofile);

      ofile << "// wire-declarations for expression: ";
      this->Print(ofile);
      ofile << endl;

      Write_VC_Wire_Declaration(this->Get_VC_Driver_Name(),
				this->Get_Type(),
				ofile);

    }
}
void AaTypeCastExpression::Write_VC_Datapath_Instances(AaExpression* target, ostream& ofile)
{
  if(!this->Is_Constant())
    {


      this->_rest->Write_VC_Datapath_Instances(NULL,ofile);

      ofile << "// data-path instances for expression: ";
      this->Print(ofile);
      ofile << endl;

      Write_VC_Unary_Operator(__NOP,
			      this->Get_VC_Datapath_Instance_Name(),
			      this->_rest->Get_VC_Wire_Name(),
			      this->_rest->Get_Type(),
			      (target != NULL ? target->Get_VC_Wire_Name() : this->Get_VC_Wire_Name()),
			      this->Get_Type(),
			      ofile);
    }
}
void AaTypeCastExpression::Write_VC_Links(string hier_id, ostream& ofile)
{



  if(!this->Is_Constant())
    {

      this->_rest->Write_VC_Links(hier_id + "/" + this->Get_VC_Name(), ofile);

      ofile << "// CP-DP links for expression: ";
      this->Print(ofile);
      ofile << endl;
      
      vector<string> reqs,acks;
      reqs.push_back(hier_id + "/" +this->Get_VC_Name() + "/rr");
      reqs.push_back(hier_id + "/" +this->Get_VC_Name() + "/cr");
      acks.push_back(hier_id + "/" +this->Get_VC_Name() + "/ra");
      acks.push_back(hier_id + "/" +this->Get_VC_Name() + "/ca");
      Write_VC_Link(this->Get_VC_Datapath_Instance_Name(),reqs,acks,ofile);
    }
}

//---------------------------------------------------------------------
// AaUnaryExpression
//---------------------------------------------------------------------
AaUnaryExpression::AaUnaryExpression(AaScope* parent_tpr,AaOperation op, AaExpression* rest):AaExpression(parent_tpr)
{
  this->_operation  = op;
  this->_rest       = rest;
  
  AaProgram::Add_Type_Dependency(this,rest);
  if(rest)
    rest->Add_Target(this);
}
AaUnaryExpression::~AaUnaryExpression() {};
void AaUnaryExpression::Print(ostream& ofile)
{
  ofile << " ( ";
  ofile << Aa_Name(this->Get_Operation());
  ofile << " ";
  this->Get_Rest()->Print(ofile);
  ofile << " )";
}

void AaUnaryExpression::Write_VC_Control_Path(ostream& ofile)
{
  string ps;
  this->AaRoot::Print(ps);

  ofile << "// control-path for expression: ";
  this->Print(ofile);
  ofile << endl;


  if(!this->Is_Constant())
    {
      ofile << ";;[" << this->Get_VC_Name() << "] { // unary expression: " << ps << endl;
      this->_rest->Write_VC_Control_Path(ofile);
      ofile << "$T [rr] $T [ra] $T [cr] $T [ca] //(split) unary operation" << endl;
      ofile << "}" << endl;
    }
}


void AaUnaryExpression::Evaluate()
{
  if(!_already_evaluated)
    {
      _already_evaluated = true;
      this->_rest->Evaluate();
      if(this->_rest->Is_Constant())
	this->Assign_Expression_Value(Perform_Unary_Operation(this->_operation, 
							 this->_rest->Get_Expression_Value()));
      
    }
}

void AaUnaryExpression::Write_VC_Constant_Wire_Declarations(ostream& ofile)
{
  if(this->Is_Constant())
    {

      ofile << "// constant-declarations for expression: ";
      this->Print(ofile);
      ofile << endl;


      Write_VC_Constant_Declaration(this->Get_VC_Constant_Name(),
				    this->Get_Type(),
				    this->Get_Expression_Value(),
				    ofile);
    }
  else
    this->_rest->Write_VC_Constant_Wire_Declarations(ofile);
}
void AaUnaryExpression::Write_VC_Wire_Declarations(bool skip_immediate, ostream& ofile)
{
  if(!this->Is_Constant() && !skip_immediate)
    {

      this->_rest->Write_VC_Wire_Declarations(false,ofile);

      ofile << "// wire-declarations for expression: ";
      this->Print(ofile);
      ofile << endl;

      Write_VC_Wire_Declaration(this->Get_VC_Driver_Name(),
				this->Get_Type(),
				ofile);

    }

}
void AaUnaryExpression::Write_VC_Datapath_Instances(AaExpression* target, ostream& ofile)
{
  if(!this->Is_Constant())
    {


      this->_rest->Write_VC_Datapath_Instances(NULL,ofile);


      ofile << "// data-path instances for expression: ";
      this->Print(ofile);
      ofile << endl;

      Write_VC_Unary_Operator(this->Get_Operation(),
			      this->Get_VC_Datapath_Instance_Name(),
			      this->_rest->Get_VC_Driver_Name(),
			      this->_rest->Get_Type(),
			      (target != NULL ? target->Get_VC_Receiver_Name() : 
			       this->Get_VC_Receiver_Name()),
			    (target != NULL ? target->Get_Type() : this->Get_Type()),
			      ofile);
    }

}
void AaUnaryExpression::Write_VC_Links(string hier_id, ostream& ofile)
{
  if(!this->Is_Constant())
    {

      this->_rest->Write_VC_Links(hier_id + "/" + this->Get_VC_Name(), ofile);


      ofile << "// CP-DP links for expression: ";
      this->Print(ofile);
      ofile << endl;

       vector<string> reqs,acks;
      reqs.push_back(hier_id + "/" +this->Get_VC_Name() + "/rr");
      reqs.push_back(hier_id + "/" +this->Get_VC_Name() + "/cr");
      acks.push_back(hier_id + "/" +this->Get_VC_Name() + "/ra");
      acks.push_back(hier_id + "/" +this->Get_VC_Name() + "/ca");
      Write_VC_Link(this->Get_VC_Datapath_Instance_Name(),reqs,acks,ofile);
    }
}

//---------------------------------------------------------------------
// AaBinaryExpression
//---------------------------------------------------------------------
AaBinaryExpression::AaBinaryExpression(AaScope* parent_tpr,AaOperation op, AaExpression* first, AaExpression* second):AaExpression(parent_tpr)
{
  this->_operation = op;

  if(Is_Bitsel_Operation(op))
    { // bitsel: the output is always a single bit
      // there is no dependence betweem the two 
      // inputs
      this->Set_Type(AaProgram::Make_Uinteger_Type(1));
    }
  else if(Is_Compare_Operation(op))
    {
      this->Set_Type(AaProgram::Make_Uinteger_Type(1));
      AaProgram::Add_Type_Dependency(first,second);
    }
  else if(Is_Shift_Operation(op))
    {
      AaProgram::Add_Type_Dependency(first,this);
    }
  else if(!Is_Concat_Operation(op))
    {
      AaProgram::Add_Type_Dependency(first,this);
      AaProgram::Add_Type_Dependency(second,this);
    }

  this->_first = first;
  if(first)
    first->Add_Target(this);
  this->_second = second;
  if(second)
    second->Add_Target(this);

  this->Update_Type();
}

AaBinaryExpression::~AaBinaryExpression() {};
void AaBinaryExpression::Print(ostream& ofile)
{
  ofile << "(" ;
  this->Get_First()->Print(ofile);
  ofile << " ";
  ofile << Aa_Name(this->Get_Operation());
  ofile << " ";
  this->Get_Second()->Print(ofile);
  ofile << ")";
}

void AaBinaryExpression::Update_Type()
{
  if(Is_Concat_Operation(this->_operation) && (this->Get_Type() == NULL))
    {
      // check the types of both sources.
      // they must both be uintegers and
      // the type of this expression must
      // be a uinteger whose width is the
      // sume of those of the sources.
      AaType* t1 = this->Get_First()->Get_Type();
      AaType* t2 = this->Get_Second()->Get_Type();

      if(t1 != NULL && t2 != NULL)
	{
	  if(t1->Is("AaUintType") && t2->Is("AaUintType"))
	    {
	      AaType* nt = AaProgram::Make_Uinteger_Type(((AaUintType*)t1)->Get_Width()+((AaUintType*)t2)->Get_Width());
	      this->AaExpression::Set_Type(nt);
	    }
	  else
	    {
	      AaRoot::Error("source arguments of concatenate expression must have uint types",this);
	    }
	}
    }
}


bool AaBinaryExpression::Is_Trivial()
{
  if(this->_operation == __OR || this->_operation == __AND ||
     this->_operation == __NOR || this->_operation == __NAND ||
     this->_operation == __XOR || this->_operation == __XNOR ||
     this->_operation == __CONCAT || this->_operation == __BITSEL)
    return(true);
  else
    return(false);

}

void AaBinaryExpression::Write_VC_Control_Path(ostream& ofile)
{

  string ps;
  this->AaRoot::Print(ps);

  if(!this->Is_Constant())
    {

      ofile << "// control-path for expression: ";
      this->Print(ofile);
      ofile << endl;

      ofile << ";;[" << this->Get_VC_Name() << "] { // binary expression: " << ps << endl;

      ofile << "||[" << this->Get_VC_Name() << "_inputs] { " << endl;
      this->_first->Write_VC_Control_Path(ofile);
      this->_second->Write_VC_Control_Path(ofile);
      ofile << "}" << endl;

      ofile << "$T [rr] $T [ra] $T [cr] $T [ca] // (split) binary operation " << endl;
      ofile << "}" << endl;
    }
}

void AaBinaryExpression::Write_VC_Constant_Wire_Declarations(ostream& ofile)
{
  if(this->Is_Constant())
    {

      ofile << "// constant-declarations for expression: ";
      this->Print(ofile);
      ofile << endl;


      Write_VC_Constant_Declaration(this->Get_VC_Constant_Name(),
				    this->Get_Type(),
				    this->Get_Expression_Value(),
				    ofile);
    }
  else
    {
      this->_first->Write_VC_Constant_Wire_Declarations(ofile);
      this->_second->Write_VC_Constant_Wire_Declarations(ofile);
    }
}
void AaBinaryExpression::Write_VC_Wire_Declarations(bool skip_immediate, ostream& ofile)
{


  if(!this->Is_Constant())
    {
      this->_first->Write_VC_Wire_Declarations(false,ofile);
      this->_second->Write_VC_Wire_Declarations(false, ofile);
      
      if(!this->Is_Constant() && !skip_immediate)
	{
	  ofile << "// wire-declarations for expression: ";
	  this->Print(ofile);
	  ofile << endl;
	  Write_VC_Wire_Declaration(this->Get_VC_Driver_Name(),
				    this->Get_Type(),
				    ofile);
	}
    }



}
void AaBinaryExpression::Write_VC_Datapath_Instances(AaExpression* target, ostream& ofile)
{


  if(!this->Is_Constant())
    {

      this->_first->Write_VC_Datapath_Instances(NULL,ofile);
      this->_second->Write_VC_Datapath_Instances(NULL,ofile);

      ofile << "// data-path-instances for expression: ";
      this->Print(ofile);
      ofile << endl;

      Write_VC_Binary_Operator(this->Get_Operation(),
			       this->Get_VC_Datapath_Instance_Name(),
			       _first->Get_VC_Driver_Name(),
			       _first->Get_Type(),
			       _second->Get_VC_Driver_Name(),
			       _second->Get_Type(),
			       (target != NULL ? target->Get_VC_Receiver_Name() : 
				this->Get_VC_Receiver_Name()),
			       (target != NULL ? target->Get_Type() : this->Get_Type()),
			       ofile);
    }
			  
}
void AaBinaryExpression::Write_VC_Links(string hier_id, ostream& ofile)
{

  if(!this->Is_Constant())
    {

      string input_hier_id = hier_id + "/"  + this->Get_VC_Name() + "/"
	+ this->Get_VC_Name() + "_inputs";

      this->_first->Write_VC_Links(input_hier_id, ofile);
      this->_second->Write_VC_Links(input_hier_id, ofile);

      ofile << "// CP-DP links for expression: ";
      this->Print(ofile);
      ofile << endl;

       vector<string> reqs,acks;
       reqs.push_back(hier_id + "/" +this->Get_VC_Name() + "/rr");
       reqs.push_back(hier_id + "/" +this->Get_VC_Name() + "/cr");
       acks.push_back(hier_id + "/" +this->Get_VC_Name() + "/ra");
       acks.push_back(hier_id + "/" +this->Get_VC_Name() + "/ca");
       Write_VC_Link(this->Get_VC_Datapath_Instance_Name(),reqs,acks,ofile);
    }
}


void AaBinaryExpression::Evaluate()
{
  if(!_already_evaluated)
    {
      _already_evaluated = true;
      this->_first->Evaluate();
      this->_second->Evaluate();
      if(this->_first->Is_Constant() && this->_second->Is_Constant())
	this->Assign_Expression_Value(Perform_Binary_Operation(this->_operation, 
							       this->_first->Get_Expression_Value(),
							       this->_second->Get_Expression_Value()));
    }
}


//---------------------------------------------------------------------
// AaTernaryExpression
//---------------------------------------------------------------------
AaTernaryExpression::AaTernaryExpression(AaScope* parent_tpr,
					 AaExpression* test,
					 AaExpression* iftrue, 
					 AaExpression* iffalse):AaExpression(parent_tpr)
{

  assert(test != NULL);

  
  this->_test = test;
  test->Add_Target(this);

  assert(test->Get_Type() && test->Get_Type()->Is("AaUintType") &&
	 (((AaUintType*)(test->Get_Type()))->Get_Width() == 1));

  if(iftrue)
    {
      AaProgram::Add_Type_Dependency(iftrue,this);
      iftrue->Add_Target(this);
    }
  if(iffalse)
    {
      AaProgram::Add_Type_Dependency(iffalse,this);
      iffalse->Add_Target(this);
    }

  this->_if_true = iftrue;
  this->_if_false = iffalse;
}
AaTernaryExpression::~AaTernaryExpression() {};
void AaTernaryExpression::Print(ostream& ofile)
{
  ofile << "( $mux ";
  this->Get_Test()->Print(ofile);
  ofile << " ";
  this->Get_If_True()->Print(ofile);
  ofile << "  ";
  this->Get_If_False()->Print(ofile);
  ofile << " ) ";
}

void AaTernaryExpression::Write_VC_Control_Path(ostream& ofile)
{

  ofile << "// control-path for expression: ";
  this->Print(ofile);
  ofile << endl;

  // if _test is constant, print dummy.
  string ps;
  this->AaRoot::Print(ps);
  if(!this->Is_Constant())
    {
      ofile << ";;[" << this->Get_VC_Name() << "] { // ternary expression: " << ps << endl;
      ofile << "||[" << this->Get_VC_Name() << "_inputs] { " << endl;
      this->_test->Write_VC_Control_Path(ofile);
      if(this->_if_true)
	this->_if_true->Write_VC_Control_Path(ofile);

      if(this->_if_false)
	this->_if_false->Write_VC_Control_Path(ofile);
      ofile << "}" << endl;

      ofile << "$T [req] $T [ack] // select req/ack" << endl;
      ofile << "}" << endl;
    }
}


void AaTernaryExpression::Evaluate()
{
  if(!_already_evaluated)
    {
      _already_evaluated = true;
      this->_test->Evaluate();
      this->_if_true->Evaluate();
      this->_if_false->Evaluate();

      if(this->_test->Is_Constant() && this->_if_true->Is_Constant() && this->_if_false->Is_Constant())
	{
	  if(this->_test->Get_Expression_Value()->To_Boolean())
	      this->Assign_Expression_Value(this->_if_true->Get_Expression_Value());
	  else
	      this->Assign_Expression_Value(this->_if_false->Get_Expression_Value());
	}
    }
}


void AaTernaryExpression::Write_VC_Constant_Wire_Declarations(ostream& ofile)
{

  ofile << "// constant-declarations for expression: ";
  this->Print(ofile);
  ofile << endl;


  if(this->Is_Constant())
    {
      Write_VC_Constant_Declaration(this->Get_VC_Constant_Name(),
				    this->Get_Type(),
				    this->Get_Expression_Value(),
				    ofile);
    }
  else
    {
      this->_test->Write_VC_Constant_Wire_Declarations(ofile);
      this->_if_true->Write_VC_Constant_Wire_Declarations(ofile);
      this->_if_false->Write_VC_Constant_Wire_Declarations(ofile);
    }

}
void AaTernaryExpression::Write_VC_Wire_Declarations(bool skip_immediate, ostream& ofile)
{



  if(!this->Is_Constant())
    {
      this->_test->Write_VC_Wire_Declarations(false,ofile);
      this->_if_true->Write_VC_Wire_Declarations(false,ofile);
      this->_if_false->Write_VC_Wire_Declarations(false,ofile);
    }

  if(!skip_immediate && !this->Is_Constant())
    {
      ofile << "// wire-declarations for expression: ";
      this->Print(ofile);
      ofile << endl;
      Write_VC_Wire_Declaration(this->Get_VC_Driver_Name(),
				this->Get_Type(),
				ofile);
    }
}
void AaTernaryExpression::Write_VC_Datapath_Instances(AaExpression* target, ostream& ofile)
{
  if(!this->Is_Constant())
    {


      this->_test->Write_VC_Datapath_Instances(NULL,ofile);
      this->_if_true->Write_VC_Datapath_Instances(NULL,ofile);
      this->_if_false->Write_VC_Datapath_Instances(NULL,ofile);

      ofile << "// data-path-instances for expression: ";
      this->Print(ofile);
      ofile << endl;

      Write_VC_Select_Operator(this->Get_VC_Datapath_Instance_Name(),
			       this->_test->Get_VC_Driver_Name(),
			       this->_test->Get_Type(),
			       this->_if_true->Get_VC_Driver_Name(),
			       this->_if_true->Get_Type(),
			       this->_if_false->Get_VC_Driver_Name(),
			       this->_if_false->Get_Type(),
			       (target != NULL ? target->Get_VC_Driver_Name() : this->Get_VC_Driver_Name()),
			       (target != NULL ? target->Get_Type() : this->Get_Type()),
			       ofile);
			       
    }
}
void AaTernaryExpression::Write_VC_Links(string hier_id, ostream& ofile)
{
  if(!this->Is_Constant())
    {

      this->_test->Write_VC_Links(hier_id + "/" + this->Get_VC_Name() + "/" +
				  this->Get_VC_Name() + "_inputs", ofile);
      this->_if_true->Write_VC_Links(hier_id + "/" + this->Get_VC_Name() + "/"
				     + this->Get_VC_Name() + "_inputs", ofile); 
      this->_if_false->Write_VC_Links(hier_id + "/" + this->Get_VC_Name() + "/"
				     + this->Get_VC_Name() + "_inputs", ofile); 


      ofile << "// CP-DP links for expression: ";
      this->Print(ofile);
      ofile << endl;

      vector<string> reqs,acks;
      reqs.push_back(hier_id + "/" + this->Get_VC_Name() + "/req");
      acks.push_back(hier_id + "/" + this->Get_VC_Name() + "/ack");

      Write_VC_Link(this->Get_VC_Datapath_Instance_Name(),
		    reqs,
		    acks,
		    ofile);
    }
}



