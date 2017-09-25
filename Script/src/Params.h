#ifndef PARAMS_H
#define PARAMS_H

#include "System.h"

class ParamValue
{
public:
  ParamValue();
  ParamValue(const string &value);

  void SetNil();

  string GetString() const;
  void   SetString(const string &s);
  string GetUpperString() const;
  void   SetUpperString(const string &s);
  string GetLowerString() const;
  void   SetLowerString(const string &s);

  bool   GetBool() const;
  void   SetBool(bool x);
  int64  GetInt() const;
  void   SetInt(int64 x);
  double GetDouble() const;
  void   SetDouble(double x);
  
  vector<ParamValue> GetList() const;
  void SetList(const vector<ParamValue> &x);

  vector<string> GetStringList() const;
  void SetStringList(const vector<string> &x);

  vector< vector<string> > GetStringListList() const;
  void SetStringListList(const vector< vector<string> > &x);

  void Set(const string &s) { SetString(s); }
  void Get(string &s) const { s = GetString(); }
  void Set(bool x) { SetBool(x); }
  void Get(bool &x) const { x = GetBool(); }
  void Set(int x) { SetInt(x); }
  void Get(int &x) const { x = GetInt(); }
  void Set(int16 x) { SetInt(x); }
  void Get(int16 &x) const { x = GetInt(); }
  void Set(int64 x) { SetInt(x); }
  void Get(int64 &x) const { x = GetInt(); }
  void Set(double x) { SetDouble(x); }
  void Get(double &x) const { x = GetDouble(); }
  void Set(float x) { SetDouble(x); }
  void Get(float &x) const { x = GetDouble(); }
  void Set(const vector<ParamValue> &x) { SetList(x); }
  void Get(vector<ParamValue> &x) const { x = GetList(); }

  void Import(const string &value);
  string Export() const;
  
  typedef enum {
    TypeUnknown, TypeNil, TypeList,
    TypeString, TypeBool, TypeInt, TypeDouble
  } ValueType;

  mutable ValueType type;

  ValueType GuessType() const;
  bool CheckType(ValueType type_) const;
  
private:
  mutable string value_unknown;
  mutable string value_string;
  mutable bool   value_bool;
  mutable int64  value_int;
  mutable double value_double;
  mutable vector<ParamValue> value_list;

  void ForceString() const;
  void ForceBool() const;
  void ForceInt() const;
  void ForceDouble() const;
  void ForceList() const;
};

class ParamNameValue
{
public:
  ParamNameValue();
  ParamNameValue(const string &text_);

  void New(const string &name_);
  void Import(const string &text_);
  string Export() const;

  string GetName() const { return name; }
  bool CompareName(const string &s) const
  {
    return (strcasecmp(name.c_str(), s.c_str()) == 0);
  }

  void Touch() { modified = true; }
  const ParamValue &GetValue() const { return value; }
  ParamValue &SetValue() { Touch(); return value; }

private:
  string name;
  ParamValue value;
  string text;
  bool modified;
};

struct ParamDefine
{
  string name;
  ParamValue::ValueType type;

  ParamDefine() : name(""), type(ParamValue::TypeNil) {}

  ParamDefine(const string &name_, const ParamValue::ValueType type_)
    : name(name_), type(type_) {}
};

class Params
{
public:
  Params(bool check_defines_ = true);
  Params(const vector<string> &params, bool check_defines_ = true);

  void Clear();
  void Import(const vector<string> &params);
  void ImportArgs(const vector<string> &params);
  vector<string> Export() const;
  void Load(const string &filename);
  void Save(const string &filename) const;

  void Append(const Params &params);
  void ExpandImports();
  void TouchAll();

  bool Contains(const string &name);
  ParamNameValue &Get(const string &name);
  void Erase(const string &name);

  const ParamValue &operator[](const string &name) const;
  ParamValue &operator[](const string &name);

  void Require(const string &name);
  void Require(const vector<string> &names);
  void SetDefines(const vector<ParamDefine> &defines_);
  static const vector<ParamDefine> &DefaultDefines();

private:
  vector<ParamNameValue> namevalues;
  vector<ParamDefine> defines;

  void Insert(int index, const Params &params);
  void CheckDefines(int start_index, int end_index);

  static bool default_defines_init;
  static vector<ParamDefine> default_defines;
  static void InitDefaultDefines();
  bool check_defines;
};

ostream &operator<<(ostream &o, const Params &x);
istream &operator>>(istream &i, Params &x);


enum ParallelCommand
{
  ParallelNone,
  ParallelInit,
  ParallelRunJob,
  ParallelCheck,
  ParallelDone
};

ParallelCommand GetParallelCommand(Params &params);
int GetParallelJob(Params &params);


#endif
