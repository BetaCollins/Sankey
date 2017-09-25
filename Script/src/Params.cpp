#include "System.h"
#include "Params.h"
#include "Utility.h"

ParamValue::ParamValue()
{
  SetNil();
}

ParamValue::ParamValue(const string &value)
{
  Import(value);
}

void ParamValue::SetNil()
{
  type = TypeNil;
}

string ParamValue::GetString() const
{
  if (type != TypeString)
    ForceString();
  return value_string;
}

void ParamValue::SetString(const string &s)
{
  type = TypeString;
  value_string = s;
}

string ParamValue::GetUpperString() const
{
  if (type != TypeString)
    ForceString();
  string s = value_string;
  ToUpper(s);
  return s;
}

void ParamValue::SetUpperString(const string &s)
{
  type = TypeString;
  value_string = s;
  ToUpper(value_string);
}

string ParamValue::GetLowerString() const
{
  if (type != TypeString)
    ForceString();
  string s = value_string;
  ToLower(s);
  return s;
}

void ParamValue::SetLowerString(const string &s)
{
  type = TypeString;
  value_string = s;
  ToLower(value_string);
}

bool ParamValue::GetBool() const
{
  if (type != TypeBool)
    ForceBool();
  return value_bool;
}

void ParamValue::SetBool(bool x)
{
  type = TypeBool;
  value_bool = x;
}

int64 ParamValue::GetInt() const
{
  if (type != TypeInt)
    ForceInt();
  return value_int;
}

void ParamValue::SetInt(int64 x)
{
  type = TypeInt;
  value_int = x;
}

double ParamValue::GetDouble() const
{
  if (type != TypeString)
    ForceDouble();
  return value_double;
}

void ParamValue::SetDouble(double x)
{
  type = TypeDouble;
  value_double = x;
}

vector<ParamValue> ParamValue::GetList() const
{
  if (type != TypeList)
    ForceList();
  return value_list;
}

void ParamValue::SetList(const vector<ParamValue> &x)
{
  type = TypeList;
  value_list = x;
}

vector<string> ParamValue::GetStringList() const
{
  if (type != TypeList)
    ForceList();
  
  vector<string> result;
  for (int i = 0; i < value_list.size(); ++i)
    result.push_back(value_list[i].GetString());
  return result;
}

void ParamValue::SetStringList(const vector<string> &x)
{
  vector<ParamValue> values(x.size());
  for (int i = 0; i < x.size(); ++i)
    values[i].SetString(x[i]);

  type = TypeList;
  value_list = values;
}

vector< vector<string> > ParamValue::GetStringListList() const
{
  if (type != TypeList)
    ForceList();
  
  vector< vector<string> > result;
  for (int i = 0; i < value_list.size(); ++i)
    result.push_back(value_list[i].GetStringList());
  return result;
}

void ParamValue::SetStringListList(const vector< vector<string> > &x)
{
  vector<ParamValue> values(x.size());
  for (int i = 0; i < x.size(); ++i)
    values[i].SetStringList(x[i]);

  type = TypeList;
  value_list = values;
}

void ParamValue::Import(const string &value)
{
  type = TypeUnknown;
  value_unknown = value;
}

string ParamValue::Export() const
{
  switch (type)
  {
  case TypeUnknown:
    return value_unknown;
    
  case TypeNil:
    return "";

  case TypeList:
    {
      string s = "(";
      for (int i = 0; i < value_list.size(); ++i)
        if (i == 0)
          s += " " + value_list[i].Export();
        else
          s += ", " + value_list[i].Export();
      s += " )";
      return s;
    }

  case TypeString:
    {
      if (value_string.find_first_of(" \t,;()") != string::npos)
        return "\"" + value_string + "\"";
      else
        return value_string;
    }

  case TypeBool:
    if (value_bool)
      return "true";
    else
      return "false";

  case TypeInt:
    {
      stringstream s;
      s << value_int;
      return s.str();
    }

  case TypeDouble:
    {
      stringstream s;
      s << value_double;
      return s.str();
    }
    
  default:
    Assert(false);
    return "";
  }
}

ParamValue::ValueType ParamValue::GuessType() const
{
  return TypeUnknown;
}

bool ParamValue::CheckType(ValueType type_) const
{
  if ((type == TypeNil) && (type_ != TypeNil))
  {
    cerr << "Parameter value must be specified" << endl;
    Exit(1);
  }

  switch (type_)
  {
  case TypeNil:
    return (type == TypeNil);

  case TypeList:
    ForceList();
    return true;

  case TypeString:
    ForceString();
    return true;

  case TypeBool:
    ForceBool();
    return true;

  case TypeInt:
    ForceInt();
    return true;

  case TypeDouble:
    ForceDouble();
    return true;

  default:
    Assert(false);
    return false;
  }
}

void ParamValue::ForceString() const
{
  switch (type)
  {
  case TypeString:
    break;

  case TypeUnknown:
    value_string = TrimSpace(value_unknown);
    if (value_string.length() >= 2)
    {
      if (((value_string[0] == '\"') && (value_string[value_string.length() - 1] == '\"')) ||
          ((value_string[0] == '\'') && (value_string[value_string.length() - 1] == '\'')))
        value_string = value_string.substr(1, value_string.length() - 2);
    }
    break;

  case TypeNil:
    value_string = "";
    break;

  default:
    Assert(0);
  }
  type = TypeString;
}

void ParamValue::ForceBool() const
{
  switch (type)
  {
  case TypeBool:
    break;

  case TypeUnknown:
    {
      string s = TrimSpace(value_unknown);
      if (s.length() == 0)
        value_bool = true;
      else
      {
        ToLower(s);
        if ((s == "t") || (s == "true"))
          value_bool = true;
        else if ((s == "f") || (s == "false"))
          value_bool = false;
        else
        {
          cerr << "\"True\"/\"false\" parameter value expected" << endl;
          Exit(1);
        }
      }
      break;
    }

  default:
    Assert(0);
  }
  type = TypeBool;
}

void ParamValue::ForceInt() const
{
  switch (type)
  {
  case TypeInt:
    break;

  case TypeUnknown:
    {
      string s = TrimSpace(value_unknown);
      int endpos;
      if (!StringToInt64(s, value_int, endpos) ||
          (endpos != s.length()))
      {
        cerr << "Integer parameter value expected" << endl;
        Exit(1);
      }
      break;
    }

  default:
    Assert(0);
  }
  type = TypeInt;
}

void ParamValue::ForceDouble() const
{
  switch (type)
  {
  case TypeDouble:
    break;

  case TypeUnknown:
    {
      string s = TrimSpace(value_unknown);
      char *endptr;
      value_double = strtod(s.c_str(), &endptr);
      if ((s.c_str() == endptr) || *endptr)
      {
        cerr << "Decimal parameter value expected" << endl;
        Exit(1);
      }
      break;
    }

  default:
    Assert(0);
  }
  type = TypeDouble;
}

void ParamValue::ForceList() const
{
  switch (type)
  {
  case TypeList:
    break;

  case TypeUnknown:
    {
      value_list.resize(0);

      string s = TrimSpace(value_unknown);
      if ((s.length() >= 2) &&
          (((s[0] == '(') && (s[s.length() - 1] == ')')) ||
           ((s[0] == '[') && (s[s.length() - 1] == ']')) ||
           ((s[0] == '{') && (s[s.length() - 1] == '}'))))
        s = s.substr(1, s.length() - 2);

      int depth = 0;
      int start = 0;
      for (int pos = 0; pos < s.length(); ++pos)
      {
        if (depth == 0)
        {
          if ((s[pos] == ',') || (s[pos] == ';'))
          {
            value_list.push_back(ParamValue(s.substr(start, pos - start)));
            start = pos + 1;
          }
        }

        if ((s[pos] == '(') || (s[pos] == '[') || (s[pos] == '{'))
          ++depth;
        else if ((s[pos] == ')') || (s[pos] == ']') || (s[pos] == '}'))
        {
          --depth;
          if (depth < 0)
          {
            cerr << "Invalid parameter list nesting" << endl;
            Exit(1);
          }
        }
      }
      if (depth != 0)
      {
        cerr << "Invalid parameter list nesting" << endl;
        Exit(1);
      }
      value_list.push_back(ParamValue(s.substr(start, s.length() - start)));
      break;
    }

  default:
    Assert(0);
  }
  type = TypeList;
}


ParamNameValue::ParamNameValue()
{
}

ParamNameValue::ParamNameValue(const string &text_)
{
  Import(text_);
}

void ParamNameValue::New(const string &name_)
{
  name = name_;
  value.SetNil();
  modified = true;
}

void ParamNameValue::Import(const string &text_)
{
  text = text_;
  modified = false;

  int equal = text.find_first_of('=');
  if (equal == string::npos)
  {
    value.SetNil();
    equal = text.length();
  }
  else
  {
    string value_text = TrimSpace(text.substr(equal + 1, 
                                              text.length() - (equal + 1)));
    value.Import(value_text);
  }

  name = TrimSpace(text.substr(0, equal));
}

string ParamNameValue::Export() const
{
  if (!modified)
    return text;
  else
  {
    string text_name;
    if (name.find_first_of(" \t,;()") != string::npos)
      text_name = "\"" + name + "\"";
    else
      text_name = name;

    if (value.type == ParamValue::TypeNil)
      return text_name;
    else
      return text_name + " = " + value.Export();
  }
}

Params::Params(bool check_defines_)
{
  check_defines = check_defines_;
  SetDefines(DefaultDefines());
}

Params::Params(const vector<string> &params, bool check_defines_)
{
  check_defines = check_defines_;
  SetDefines(DefaultDefines());
  Import(params);
}

void Params::Clear()
{
  namevalues.clear();
}

void Params::Import(const vector<string> &params)
{
  int start_index = namevalues.size();
  for (int i = 0; i < params.size(); ++i)
  {
    if (TrimSpace(params[i]).length() > 0)
      namevalues.push_back(ParamNameValue(params[i]));
  }
  int end_index = namevalues.size();
  CheckDefines(start_index, end_index);
}

void Params::ImportArgs(const vector<string> &params)
{
  vector<string> new_params;
  int state = 0;
  string param;
  for (int i = 0; i < params.size(); ++i)
  {
    param = TrimSpace(params[i]);
    int equal = param.find_first_of('=');
    if (equal != string::npos)
    {
      if (param.length() == 1)
      {
        if (state != 1)
        {
          cerr << "Error parsing arguments: \"" << param << "\"" << endl;
          Exit(1);
        }
        new_params.back() += '=';
        state = 2;
      }
      else
      {
        if (param[0] == '=')
        {
          if (state != 1)
          {
            cerr << "Error parsing arguments: \"" << param << "\"" << endl;
            Exit(1);
          }
          new_params.back() += param;
          state = 0;
        }
        else
        {
          new_params.push_back(param);
          if (param[param.length() - 1] == '=')
            state = 2;
          else
            state = 1;
        }
      }
    }
    else
    {
      if (state == 2)
        new_params.back() += param;
      else
        new_params.push_back(param);
      state = 1;
    }
  }

  Import(new_params);
}

vector<string> Params::Export() const
{
  vector<string> result;
  for (int i = 0; i < namevalues.size(); ++i)
    result.push_back(namevalues[i].Export());
  return result;
}

void Params::Load(const string &filename)
{
  vector<string> lines;
  ReadFile(filename, lines);
  Import(lines);
}

void Params::Save(const string &filename) const
{
  vector<string> lines = Export();
  WriteFile(filename, lines);
}

void Params::Append(const Params &params)
{
  int start_index = namevalues.size();
  namevalues.insert(namevalues.end(),
                    params.namevalues.begin(), params.namevalues.end());
  int end_index = namevalues.size();
  CheckDefines(start_index, end_index);
}

void Params::ExpandImports()
{
  for (int i = 0; i < namevalues.size(); ++i)
  {
    if (namevalues[i].CompareName("import") ||
        namevalues[i].CompareName("include") ||
        namevalues[i].CompareName("param") ||
        namevalues[i].CompareName("params"))
    {
      string fname = namevalues[i].GetValue().GetString();
      ifstream f(fname.c_str(), ifstream::in);
      if (!f.is_open())
      {
        cerr << "Unable to open parameter file \"" << fname << "\"" << endl;
        Exit(1);
      }
      Params new_params;
      f >> new_params;
      Insert(i + 1, new_params);
      namevalues.erase(namevalues.begin() + i);
    }
  }
}

void Params::TouchAll()
{
  for (int i = 0; i < namevalues.size(); ++i)
    namevalues[i].Touch();
}

bool Params::Contains(const string &name)
{
  for (int i = 0; i < namevalues.size(); ++i)
    if (namevalues[i].CompareName(name))
      return true;
  return false;
}

ParamNameValue &Params::Get(const string &name)
{
  for (int i = namevalues.size() - 1; i >= 0; --i)
    if (namevalues[i].CompareName(name))
      return namevalues[i];

  namevalues.push_back(ParamNameValue());
  namevalues.back().New(name);
  return namevalues.back();
}

void Params::Erase(const string &name)
{
  for (int i = 0; i < namevalues.size(); )
  {
    if (namevalues[i].CompareName(name))
      namevalues.erase(namevalues.begin() + i);
    else
      ++i;
  }
}

const ParamValue &Params::operator[](const string &name) const
{
  for (int i = namevalues.size() - 1; i >= 0; --i)
    if (namevalues[i].CompareName(name))
      return namevalues[i].GetValue();
  Assert(false);
  return namevalues[0].GetValue();
}

ParamValue &Params::operator[](const string &name)
{
  ParamNameValue &nv = Get(name);
  return nv.SetValue();
}

void Params::Insert(int index, const Params &params)
{
  int start_index = index;
  namevalues.insert(namevalues.begin() + index,
                    params.namevalues.begin(), params.namevalues.end());
  int end_index = start_index + params.namevalues.size();
  CheckDefines(start_index, end_index);
}

ostream &operator<<(ostream &o, const Params &x)
{
  vector<string> lines = x.Export();
  for (int i = 0; i < lines.size(); ++i)
    o << lines[i] << "\n";
  return o;
}

istream &operator>>(istream &i, Params &x)
{
  vector<string> lines;
  string line;
  while (!i.eof())
  {
    getline(i, line);
    if (TrimSpace(line).length() > 0)
      lines.push_back(line);
  }
  x.Import(lines);
  return i;
}

void Params::Require(const string &name)
{
  if (!Contains(name))
  {
    cerr << "Parameter \"" << name << "\" not specified" << endl;
    Exit(1);
  }
}

void Params::Require(const vector<string> &names)
{
  vector<bool> contains(names.size(), true);
  bool all = true;

  for (int i = 0; i < names.size(); ++i)
  {
    if (!Contains(names[i]))
    {
#ifndef DEBUG_VECTOR
      contains[i] = false;
#endif
      all = false;
    }
  }

  if (!all)
  {
    cerr << "Parameters not specified:" << endl;
#ifndef DEBUG_VECTOR
    for (int i = 0; i < names.size(); ++i)
      if (!contains[i])
        cerr << "  \"" << names[i] << "\"" << endl;
#endif
    Exit(1);
  }
}

void Params::SetDefines(const vector<ParamDefine> &defines_)
{
  defines.clear();
  defines.push_back(ParamDefine("import", ParamValue::TypeString));
  defines.push_back(ParamDefine("include", ParamValue::TypeString));
  defines.push_back(ParamDefine("param", ParamValue::TypeString));
  defines.push_back(ParamDefine("params", ParamValue::TypeString));

  defines.insert(defines.end(), defines_.begin(), defines_.end());
  CheckDefines(0, namevalues.size());
}

void Params::CheckDefines(int start_index, int end_index)
{
  if (!check_defines)
    return;
  for (int i = start_index; i < end_index; ++i)
  {
    if (namevalues[i].GetName()[0] == '#')
      continue;
    if (namevalues[i].GetName()[0] == '_')
      continue;

    int which = -1;
    for (int j = 0; j < defines.size(); ++j)
      if (namevalues[i].CompareName(defines[j].name))
      {
        which = j;
        break;
      }

    if (which < 0)
    {
      cerr << "Parameter \"" << namevalues[i].GetName() << "\" unknown" << endl;
      Exit(1);
    }

    if (!namevalues[i].GetValue().CheckType(defines[which].type))
    {
      cerr << "Parameter \"" << namevalues[i].GetName() << "\" has invalid type" << endl;
      Exit(1);
    }
  }
}

//ParamDefine default_defines_table[] =
//{
////#include "ParamDefines.inc"
//  //ParamDefine("dummy", ParamValue::TypeNil)
//		ParamDefine("Test", ParamValue::TypeInt),
//		ParamDefine("Test2", ParamValue::TypeInt)
//};


bool Params::default_defines_init = false;
vector<ParamDefine> Params::default_defines;

void Params::InitDefaultDefines()
{
	ParamDefine default_defines_table[] = {
		#include "ParamDefines.inc"
		ParamDefine("dummy", ParamValue::TypeNil)
	};
	int default_defines_count =
	  sizeof(default_defines_table) / sizeof(default_defines_table[0]);

	if (!default_defines_init)
	{
	for (int i = 0; i < default_defines_count; ++i)
	{
	  default_defines.push_back(default_defines_table[i]);
	}
	//default_defines_init = true;
	}
}

const vector<ParamDefine> &Params::DefaultDefines()
{
  if (!default_defines_init)
    InitDefaultDefines();
  return default_defines;
}

ParallelCommand GetParallelCommand(Params &params)
{
  if (params.Contains("parallel"))
  {
    string parallel = params["parallel"].GetString();
    ToLower(parallel);
    if (parallel == "init")
      return ParallelInit;
    else if (parallel == "runjob")
      return ParallelRunJob;
    else if (parallel == "check")
      return ParallelCheck;
    else if (parallel == "done")
      return ParallelDone;
    else
    {
      Assert(0);
      return ParallelNone;
    }
  }
  else
    return ParallelNone;
}

int GetParallelJob(Params &params)
{
  params.Require("parallel_job");
  return params["parallel_job"].GetInt();
}

