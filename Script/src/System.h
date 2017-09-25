// System.h : Declares all platform-dependent stuff
//
#ifndef SYSTEM_H
#define SYSTEM_H

namespace std
{
};
using namespace std;
namespace __gnu_cxx
{
};
using namespace __gnu_cxx;

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <vector>
#include <list>
#include <ext/slist>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <streambuf>
#include <fstream>
#include <iomanip>
#include <deque>
#include <algorithm>
#include <limits>
#include <set>
#include <map>
#include <queue>
#include <ext/functional>
//#include <ext/hash_set>
#include <unordered_set>
//#include <ext/hash_map>
#include <unordered_map>
#include <bzlib.h>
#include <zlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned long long uint64;
typedef signed long long int64;
typedef unsigned int uword;
typedef signed int word;
#define UWORDBITS ((sizeof(uword) / sizeof(uint8)) * 8)
#define WORDBITS ((sizeof(word) / sizeof(uint8)) * 8)
#define INTBITS ((sizeof(int) / sizeof(uint8)) * 8)
#define CHARBITS ((sizeof(char) / sizeof(uint8)) * 8)

#define UINT8MIN 0
#define UINT8MAX 255
#define INT8MIN -128
#define INT8MAX 127
#define UINT16MIN 0
#define UINT16MAX 65535
#define INT16MIN -32768
#define INT16MAX 32767
#define UINT32MIN 0
#define UINT32MAX 4294967295
#define INT32MIN -2147483648
#define INT32MAX 2147483647
#define UINTMIN 0
#define UINTMAX 4294967295
#define INTMIN -2147483648
#define INTMAX 2147483647
#define SIZE_TMIN 0
#define SIZE_TMAX 4294967295

#define INT64_CONST(x) (x##LL)




// Pause for user acknowledgement
void UserPause();

// Exit program
void Exit(int status);

void debug_assert(bool result, string expression, string file, int line);
void debug_assert_msg(bool result, string expression, string msg,
                      string file, int line);
void debug_assert_break();

#define ASSERT_TERMINATE

#ifdef ASSERT_TERMINATE
#define assert_terminate() \
  do { \
    /*UserPause();*/ \
    Exit(1); \
  } while (false)
#else
#define assert_terminate()
#endif

#define Assert(expression) \
  do { \
    if (!(expression)) \
    { \
      debug_assert(false, #expression, __FILE__, __LINE__); \
    } \
  } while (false)

#define AssertMsg(expression, msg) \
  do { \
    if (!(expression)) \
    { \
      debug_assert_msg(false, #expression, msg, __FILE__, __LINE__); \
    } \
  } while (false)



template <typename T>
class Vector
{
public:
  typedef typename vector<T>::iterator iterator;
  typedef typename vector<T>::const_iterator const_iterator;

  Vector() { }
  Vector(const Vector<T> &init) : v(init.v) { }
  Vector(const vector<T> &init) : v(init) { }
  Vector(int size, const T &init = T()) : v(size, init) { }
  template <typename Iterator>
  Vector(Iterator begin, Iterator end) : v(begin, end) { }

  void resize(int size, const T &init = T()) { v.resize(size, init); }
  void reserve(int size) { v.reserve(size); }
  void clear() { v.clear(); }
  void swap(Vector<T> &other) { v.swap(other.v); }

  bool empty() const { return v.empty(); }
  int size() const { return int(v.size()); }

  void push_back(const T &value) { v.push_back(value); }
  void pop_back() { Assert(!v.empty()); v.pop_back(); }
  void push_front(const T &value) { v.push_front(value); }
  void pop_front() { Assert(!v.empty()); v.pop_front(); }

  void erase(int i) { Assert((i >= 0) && (i < v.size())); v.erase(i); }

  template <typename Iterator>
  void erase(Iterator pos)
  {
    Assert((pos >= v.begin()) && (pos < v.end()));
    v.erase(pos);
  }
    
  template <typename Iterator>
  void erase(Iterator begin, Iterator end)
  {
    Assert((begin >= v.begin()) && (end <= v.end()));
    v.erase(begin, end);
  }

  void insert(int i, const T &value) 
  {
    Assert((i >= 0) && (i < v.size())); 
    v.insert(i, value);
  }

  template <typename MyIterator, typename Iterator>
  void insert(MyIterator pos, Iterator begin, Iterator end)
  {
    Assert((pos >= v.begin()) && (pos <= v.end()));
    v.insert(pos, begin, end);
  }
  
  
  const T &back() const { Assert(!v.empty()); return v.back(); }
  T &back() { Assert(!v.empty()); return v.back(); }
  const T &front() const { Assert(!v.empty()); return v.front(); }
  T &front() { Assert (!v.empty()); return v.front(); }
  
  const T &operator[](int i) const { Assert((i >= 0) && (i < v.size())); return v[i]; }
  T &operator[](int i) { Assert((i >= 0) && (i < v.size())); return v[i]; }

  typename vector<T>::const_iterator begin() const { return v.begin(); }
  typename vector<T>::iterator begin() { return v.begin(); }
  typename vector<T>::const_iterator end() const { return v.end(); }
  typename vector<T>::iterator end() { return v.end(); }

  operator const vector<T> &() const { return v; }
  operator vector<T> &() { return v; }

private:
  vector<T> v;
};


//#define DEBUG_VECTOR
#ifdef DEBUG_VECTOR
#define vector Vector
#endif




uint32 TimerSeconds();
uint32 RandomSeed();

double drand();

int irand();

void SplitPath(const string &path, string &dir, string &file);
void RelPath(const string &dir, const string &file, string &path);
string RelPath(const string &dir, const string &file);
bool RemoveFile(const string &filename);
bool RenameFile(const string &source, const string &dest);
bool MakeDir(const string &path);
bool RemoveDir(const string &path);
bool RemoveRecursiveDir(const string &path);
bool FileReadable(const string &filename);
string GetExtension(const string &path);
string RemoveExtension(const string &path);
string RemoveBZ2Extension(const string &path);
bool BZ2Extension(const string &path);
string RemoveGZExtension(const string &path);
bool GZExtension(const string &path);
string RemoveCompressedExtension(const string &path);
string Fas2QualFilename(const string &fas_filename);

const uint64 SYS_MEM = 1500000000;


extern string cmd_dir, cmd_name;

int CallMAligner(const string &filename1, const string &filename2,
                 const string &outfilename);

#define zapeof(c) ((c) & 0377)

class bz2streambuf : public streambuf
{
public:
  bz2streambuf();
  ~bz2streambuf();
  void Init(FILE *f_, bool write_);
  void Done();

  bool Success() const { return !error; }

  void Open(FILE *f_);
  void Close();
  void Reset();

protected:
  virtual int overflow(int c = EOF);
  virtual int underflow();
  virtual int sync();

private:
  FILE *f;
  BZFILE *bzfile;
  int bzerror;
  bool write, error;
  static const int bufsize = 65536;
  char *buf;
};

class bz2base : virtual public ios
{
protected:
  bool bzOpen(FILE *f_, bool write_);
  bool bzClose();
  void bzReset();

  bz2streambuf buf;
};


class bz2istream : public bz2base, public istream
{
public:
  bz2istream(const string &bz2_filename);
  ~bz2istream();
  void Reset();

private:
  FILE *f;
};

class bz2ostream : public bz2base, public ostream
{
public:
  bz2ostream(const string &bz2_filename_);
  ~bz2ostream();

private:
  string bz2_filename;
  FILE *f;
};


class gzstreambuf : public streambuf
{
public:
  gzstreambuf();
  ~gzstreambuf();
  void Init(FILE *f_, bool write_);
  void Done();

  bool Success() const { return !error; }

  void Open(FILE *f_);
  void Close();
  void Reset();

protected:
  virtual int overflow(int c = EOF);
  virtual int underflow();
  virtual int sync();

private:
  int fd;
  bool open;
  gzFile gz;
  bool write, error;
  static const int bufsize = 65536;
  char *buf;
};

class gzbase : virtual public ios
{
protected:
  bool gzOpen(FILE *f_, bool write_);
  bool gzClose();
  void gzReset();

  gzstreambuf buf;
};


class gzistream : public gzbase, public istream
{
public:
  gzistream(const string &gz_filename);
  ~gzistream();
  void Reset();

private:
  FILE *f;
};

class gzostream : public gzbase, public ostream
{
public:
  gzostream(const string &gz_filename_);
  ~gzostream();

private:
  string gz_filename;
  FILE *f;
};

//#define strcasecmp ?

int StringVersionCmp(const string &s1, const string &s2);

bool GetDirectoryList(const string &dir, 
                      vector<string> &files, vector<string> &subdirs);

int CallProgram(const string &program, const vector<string> &args,
                const string &stdin_file, const string &stdout_file,
                const string &stderr_file);


// Main entry point to program
int Main(vector<string> args);


#endif
