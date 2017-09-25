#ifndef UTILITY_H
#define UTILITY_H

/* grand(): Returns a random number with a Guassian distribution
 * with mean 0 and standard deviation 1.0 */
double grand();


double npdf(double z);
double ncdf(double z);


inline bool xorbool(bool a, bool b)
{
  return (a && !b) || (!a && b);
}

extern char base2char[4];
extern int char2base[256];
extern int basecomplement[4];
extern char charcomplement[256];
extern char multibase2char[16];
extern uint8 rc_1mer[4];
extern uint8 rc_2mer[16];
extern uint8 rc_3mer[64];
extern uint8 rc_4mer[256];

extern char symbol2char[7];
extern int char2symbol[256];
extern int symbolcomplement[7];

extern double qualprob[200];

extern uint32 crc32tab[256];

void InitCRC32Table();
uint32 CRC32UpdateChar(char data, uint32 crc);
uint32 CRC32UpdateBuf(const char *buf, size_t bytes, uint32 crc);

template <typename T>
uint32 CRC32Update(const T &data, uint32 crc)
{
  return CRC32UpdateBuf((const char *)&data, sizeof(data), crc);
}

template <>
uint32 CRC32Update<char *>(char * const &data, uint32 crc);

template <>
uint32 CRC32Update<string>(const string &data, uint32 crc);


void InitBaseTables();
bool BaseTablesReady();

template <int kmer_size>
uint64 ReverseComplement(uint64 kmer)
{
  uint64 rc = 0;

  for (int i = 0; i <= (kmer_size - 4); i += 4)
  {
    rc = (rc << 8) | rc_4mer[kmer & 255];
    kmer >>= 8;
  }

  int remainder = kmer_size & 3;
  if (remainder > 0)
  {
    rc <<= 2 * remainder;
    rc |= rc_4mer[kmer & 255] >> ((4 - remainder) * 2);
  }
  
  return rc;
}

void ReverseComplement(string &s, bool retain_nonacgt = true);
void ReverseComplement(const string &source, string &dest, 
                       bool retain_nonacgt = true);
void Reverse(string &s);


void ReadFileList(const string &listfname, vector<string> &files);
size_t GetFileSize(const string &filename);
void ReadFile(const string &filename, vector<string> &lines, bool discard_empty = false);
void WriteFile(const string &filename, const vector<string> &lines);

void SplitFields(const string &s, vector<string> &fields, const string &delim);
void SplitStrictFields(const string &s, vector<string> &fields, 
                       const string &delim);
void SplitTabFields(const string &s, vector<string> &fields);

template <typename T>
void StrTo(const string &s, T &x)
{
  stringstream ss(s);
  ss >> x;
}

template <typename T>
string ToStr(T x, int width = 0)
{
  stringstream ss;
  if (width != 0)
    ss << setw(width) << x;
  else
    ss << x;
  return ss.str();
}

template <typename T>
string ToStr(T x, int width, int decimals = 0)
{
  stringstream ss;
  ss.setf(ios::fixed, ios::floatfield);
  ss.precision(decimals);
  if (width != 0)
    ss << setw(width) << x;
  else
    ss << x;
  return ss.str();
}

template <typename T>
string ToHex(T x, bool pad = false, bool upper = true)
{
  bool negative = false;
  if (x < 0)
  {
    negative = true;
    x = -x;
  }

  int width = 0;
  if (pad)
    width = sizeof(T) * 2;
  
  string result;
  do
  {
    int nibble = (x & 0xF);
    char c = (nibble < 10) ? ('0' + nibble) : ('a' + (nibble - 10));
    if (upper)
      c = toupper(c);
    result += c;
    x >>= 4;
  } while ((x != 0) || (result.length() < width));
  
  Reverse(result);
  if (negative)
    result = '-' + result;
  return result;
}

template <typename T>
string ToBinary(T x, bool pad = false)
{
  int width = 0;
  if (pad)
    width = sizeof(T) * 8;

  string result;
  do
  {
    char c = ((x & 1) != 0) ? '1' : '0';
    result += c;
    x >>= 1;
  } while ((x != 0) || (result.length() < width));

  Reverse(result);
  return result;
}

template <typename T>
string FloatToStr(T x, int decimal_digits = 2)
{
  stringstream ss;
  ss << x;
  string s = ss.str();
  if (s.find('.') == string::npos)
    s += '.';
  int decimal_pos = s.find('.');
  while (s.length() < (decimal_pos + 1 + decimal_digits))
    s += '0';
  if (decimal_digits == 0)
    s.resize(decimal_pos);
  else
    s.resize(decimal_pos + 1 + decimal_digits);
  return s;
}

extern const int small_primes[];
extern const int num_small_primes;


extern const int nice_steps[];
extern const int num_nice_steps;

int FindNiceStep(int x);


template <typename T>
pair<double, double> Statistics(const vector<T> &data)
{
  double avg = 0.0;
  double avg2 = 0.0;
  for (int i = 0; i < data.size(); ++i)
  {
    avg += double(data[i]);
    avg2 += double(data[i]) * double(data[i]);
  }

  avg /= double(data.size());
  avg2 /= double(data.size());
  
  return pair<double, double>(avg, sqrt(avg2 - avg*avg));
}

template <typename T>
void Histogram(const vector<T> &data, vector<int> &histogram, 
               T bucket_size = 1)
{
  for (int i = 0; i < data.size(); ++i)
  {
    int value = int(data[i] / bucket_size);
    Assert(value >= 0);
    if (value >= histogram.size())
      histogram.resize(value + 1);
    ++histogram[value];
  }
}

template <typename T>
void SignedHistogram(const vector<T> &data, map<int, int> &histogram,
                     T bucket_size = 1)
{
  for (int i = 0; i < data.size(); ++i)
  {
    if (data[i] >= 0)
    {
      int value = int(data[i] / bucket_size);
      ++histogram[value];
    }
    else
    {
      int value = -int((-data[i] - 1) / bucket_size) - 1;
      ++histogram[value];
    }
  }
}

template <typename T>
int LogBucket(T size, T value)
{
  int i = 0;
  T limit = 1;
  while (value >= limit)
  {
    ++i;
    limit *= size;
  }
  return i;
}

template <typename T>
void LogHistogram(const vector<T> &data, vector<int> &histogram,
                  T bucket_size = 10)
{
  for (int i = 0; i < data.size(); ++i)
  {
    int value = LogBucket(bucket_size, data[i]);
    Assert(value >= 0);
    if (value >= histogram.size())
      histogram.resize(value + 1);
    ++histogram[value];
  }
}

template <typename T>
void LogSignedHistogram(const vector<T> &data, map<int, int> &histogram,
                        T bucket_size = 10)
{
  for (int i = 0; i < data.size(); ++i)
  {
    if (data[i] >= 0)
    {
      int value = LogBucket(bucket_size, data[i]);
      ++histogram[value];
    }
    else
    {
      int value = -LogBucket(bucket_size, -data[i]);
      ++histogram[value];
    }
  }
}


template <typename T>
void CustomHistogram(const vector<T> &data, vector<int> &histogram,
                     const vector<T> &bucket_limits)
{
  histogram.resize(bucket_limits.size() + 1);
  for (int i = 0; i < data.size(); ++i)
  {
    int bucket;
    for (bucket = 0; bucket < bucket_limits.size(); ++bucket)
      if (data[i] <= bucket_limits[bucket])
        break;
    ++histogram[bucket];
  }
}


class SensSpec
{
public:
  SensSpec() { Clear(); }

  void Clear()
  {
    hits[0][0] = hits[0][1] = hits[1][0] = hits[1][1] = 0;
  }

  void Accumulate(bool true_hit, bool my_hit, int value = 1)
  {
    hits[true_hit ? 1 : 0][my_hit ? 1 : 0] += value;
  }

  void Accumulate(const SensSpec &sens_spec)
  {
    for (int i = 0; i < 2; ++i)
      for (int j = 0; j < 2; ++j)
        hits[i][j] += sens_spec.hits[i][j];
  }

  int Get(bool true_hit, bool my_hit) const
  {
    return hits[true_hit ? 1 : 0][my_hit ? 1 : 0];
  }

  double Sensitivity() const
  {
    return double(Get(true, true)) / double(Get(true, false) + Get(true, true));
  }

  double Specificity() const
  {
    return double(Get(true, true)) / double(Get(false, true) + Get(true, true));
  }

  void Divide(int x)
  {
    for (int i = 0; i < 2; ++i)
      for (int j = 0; j < 2; ++j)
        hits[i][j] /= x;
  }

private:
  int hits[2][2];
};

ostream &operator<<(ostream &o, const SensSpec &sens_spec);



template <typename T>
void Write(ostream &o, const T &x)
{
  o << x;
}

template <typename T>
void Read(istream &i, T &x)
{
  i >> x;
}

template <>
void Write<int16>(ostream &o, const int16 &x);

template <>
void Read<int16>(istream &i, int16 &x);

template <>
void Write<uint16>(ostream &o, const uint16 &x);

template <>
void Read<uint16>(istream &i, uint16 &x);

template <>
void Write<int32>(ostream &o, const int32 &x);

template <>
void Read<int32>(istream &i, int32 &x);

template <>
void Write<uint32>(ostream &o, const uint32 &x);

template <>
void Read<uint32>(istream &i, uint32 &x);

template <>
void Write<int64>(ostream &o, const int64 &x);

template <>
void Read<int64>(istream &i, int64 &x);

template <>
void Write<uint64>(ostream &o, const uint64 &x);

template <>
void Read<uint64>(istream &i, uint64 &x);

template <>
void Write<double>(ostream &o, const double &x);

template <>
void Read<double>(istream &i, double &x);

template <>
void Write<float>(ostream &o, const float &x);

template <>
void Read<float>(istream &i, float &x);

template <>
void Write<string>(ostream &o, const string &x);

template <>
void Read<string>(istream &i, string &x);

template <typename T>
void Write(ostream &o, const vector<T> &x)
{
  int size = x.size();
  o.write((const char *)&size, sizeof(size));
  for (int j = 0; j < size; ++j)
    Write(o, x[j]);
}

#include <typeinfo>

template <typename T>
void Read(istream &i, vector<T> &x)
{
	//create int (we'll use it for its memory)
	int size;
	//treating the int as a char array, read in sizeof(int) bytes from the instream
	i.read((char *)&size, sizeof(size));
	//remove all elements from vector x, setting its size to 0
	x.clear();
	//resize the vector based on the contents we read into the integer (so does this
	//mean the first bytes read will compose an integer value of something?
	Assert(size >= 0);
	x.resize(size);
	for (int j = 0; j < size; ++j)
		Read(i, x[j]);
}


template <>
void Write<bool>(ostream &o, const vector<bool> &x);

template <>
void Read<bool>(istream &i, vector<bool> &x);


template <typename T1, typename T2, typename Compare>
void Write(ostream &o, const map<T1, T2, Compare> &x)
{
  int size = x.size();
  o.write((const char *)&size, sizeof(size));

  for (typename map<T1, T2, Compare>::const_iterator iter = x.begin();
       iter != x.end(); ++iter)
  {
    Write(o, iter->first);
    Write(o, iter->second);
  }
}

template <typename T1, typename T2, typename Compare>
void Read(istream &i, map<T1, T2, Compare> &x)
{
  int size;
  i.read((char *)&size, sizeof(size));
  
  x.clear();
  for (int j = 0; j < size; ++j)
  {
    T1 key;
    Read(i, key);
    Read(i, x[key]);
  }
}


#if 0
template <typename T>
void Write(ostream &o, const vector< vector<T> > &x)
{
  int size = x.size();
  o.write((const char *)&size, sizeof(size));

  for (int i = 0; i < x.size(); ++i)
  {
    size = x[i].size();
    o.write((const char *)&size, sizeof(size));
    o.write((const char *)&x[i][0], sizeof(x[i][0]) * size);
  }
}

template <typename T>
void Read(istream &i, vector< vector<T> > &x)
{
  x.clear();

  int size;
  i.read((char *)&size, sizeof(size));
  x.resize(size);

  for (int j = 0; j < x.size(); ++j)
  {
    i.read((char *)&size, sizeof(size));
    x[j].resize(size);

    i.read((char *)&(x[j][0]), sizeof(x[j][0]) * size);
  }
}
#endif

template <typename T>
ostream &operator<<(ostream &o, const vector< vector<T> > &x)
{
  int size = x.size();
  o.write((const char *)&size, sizeof(size));

  for (int i = 0; i < x.size(); ++i)
  {
    size = x[i].size();
    o.write((const char *)&size, sizeof(size));

    for (int j = 0; j < x[i].size(); ++j)
      o << x[i][j];
  }

  return o;
}

template <typename T>
istream &operator>>(istream &i, vector< vector<T> > &x)
{
  x.clear();

  int size;
  i.read((char *)&size, sizeof(size));
  x.resize(size);

  for (int j = 0; j < x.size(); ++j)
  {
    i.read((char *)&size, sizeof(size));
    x[j].resize(size);

    for (int k = 0; k < x[j].size(); ++k)
      i >> x[j][k];
  }
  
  return i;
}

template <typename T1, typename T2>
void Write(ostream &o, const pair<T1, T2> &x)
{
  Write(o, x.first);
  Write(o, x.second);
}

template <typename T1, typename T2>
void Read(istream &i, pair<T1, T2> &x)
{
  Read(i, x.first);
  Read(i, x.second);
}

template <typename T1, typename T2>
ostream &operator<<(ostream &o, const pair<T1, T2> &x)
{
  o << x.first;
  o << x.second;
}

template <typename T1, typename T2>
istream &operator>>(istream &i, pair<T1, T2> &x)
{
  i >> x.first;
  i >> x.second;
}


void Write(ostream &o, const char *buf, int64 bytes);
void Read(istream &i, char *buf, int64 bytes);



template <typename T>
void WriteBin(ostream &o, const T &x)
{
  o.write((const char *)&x, sizeof(x));
}

template <typename T>
void ReadBin(istream &i, T &x)
{
	i.read((char *)&x, sizeof(x));
}


template <typename T>
void WriteBin(ostream &o, const vector<T> &x)
{
  int32 size = x.size();
  o.write((const char *)&size, sizeof(size));
  Write(o, (const char *)&(x[0]), sizeof(x[0]) * int64(size));
}

template <typename T>
void ReadBin(istream &i, vector<T> &x)
{
  int32 size;
  i.read((char *)&size, sizeof(size));
  {
    vector<T> temp;
    x.swap(temp);
  }
  x.resize(size);
  Read(i, (char *)&(x[0]), sizeof(x[0]) * int64(size));
}


void ToUpper(string &s);
void ToLower(string &s);
string GetUpper(const string &s);
string GetLower(const string &s);
string TrimSpace(const string &s);

template <typename IntType>
bool StringToInt(const string &s, IntType &value, int &endpos, int startpos = 0)
{
  bool sign = false;
  int base = 10;
  value = 0;

  int pos = startpos;
  if (pos < (int)s.length())
  {
    if (s[pos] == '+')
    {
      sign = false;
      ++pos;
    }
    else if (s[pos] == '-')
    {
      if (IntType(-1) > 0)
        return false;
      sign = true;
      ++pos;
    }
  }

  if (pos == (int)s.length())
    return false;

  if (s[pos] == '0')
  {
    ++pos;
    if (pos == (int)s.length())
    {
      endpos = pos;
      return true;
    }

    base = 8;

    if (pos < (int)s.length())
    {
      if (s[pos] == 'x')
      {
        base = 16;
        ++pos;
      }
      else if (s[pos] == 'b')
      {
        base = 2;
        ++pos;
      }
    }
  }

  if (pos == (int)s.length())
    return false;

  while (pos < (int)s.length())
  {
    int digit;
    if ((s[pos] >= '0') && (s[pos] <= '9'))
      digit = s[pos] - '0';
    else if ((s[pos] >= 'a') && (s[pos] <= 'z'))
      digit = s[pos] - 'a' + 10;
    else if ((s[pos] >= 'A') && (s[pos] <= 'Z'))
      digit = s[pos] - 'A' + 10;
    else
      break;

    if (digit >= base)
      break;

    value = value * base + digit;
    ++pos;
  }

  if (sign)
    value = -value;

  endpos = pos;
  return true;
}

template <typename IntType>
bool StringToInt(const string &s, IntType &value)
{
  int endpos;
  bool result = StringToInt(s, value, endpos);
  return (result && (endpos == s.length()));
}


bool StringToInt64(const string &s, int64 &value, int &endpos, int startpos = 0);

template <typename T>
string Str(const T &x)
{
  stringstream temp;
  temp << x;
  return temp.str();
}

template <typename T>
string StrPad(const T &x, int width, char fill = ' ')
{
  stringstream temp;
  temp << setw(width) << setfill(fill) << x;
  return temp.str();
}

template <typename T>
string StrPadLeft(const T &x, int width, char fill = ' ')
{
  stringstream temp;
  temp << x;
  string s = temp.str();
  return s + string(max(0, width - int(s.length())), fill);
}

template <typename T>
string StrFitLeft(const T &x, int width, char fill = ' ')
{
  stringstream temp;
  temp << x;
  string s = temp.str();
  s = s.substr(0, min(width, int(s.length())));
  s += string(width - s.length(), fill);
  return s;
}

template <typename T>
string StrFitRight(const T &x, int width, int fill = ' ')
{
  stringstream temp;
  temp << x;
  string s = temp.str();
  s = s.substr(s.length() - min(width, int(s.length())), 
               min(width, int(s.length())));
  s = string(width - s.length(), fill) + s;
  return s;
}

string TempFilename();

template <typename T>
struct more
{
  bool operator()(const T &a, const T &b)
  {
    return a > b;
  }
};


void InitFactTables();
int IntFactorial(int n);
double Factorial(int n);
int IntCombinations(int n, int k);
double Combinations(int n, int k);
double Binomial(int n, int k, double p);

string SecondsToTime(int seconds);


template <typename T>
void set_subtract(set<T> &from, const set<T> &these)
{
  typename set<T>::const_iterator titer = these.begin();

  for (typename set<T>::iterator iter = from.begin(); iter != from.end(); )
  {
    while ((titer != these.end()) && (*titer < *iter))
      ++titer;
 
    if (titer == these.end())
      break;

    if (*iter < *titer)
      ++iter;
    else
    {
      typename set<T>::iterator next = iter;
      ++next;
      from.erase(iter);
      iter = next;
    }
  }
}


template <typename T1, typename T2>
struct FirstLess
{
  bool operator()(const pair<T1, T2> &x, const pair<T1, T2> &y) const
  {
    return x.first < y.first;
  }
};

template <typename T1, typename T2>
struct FirstMore
{
  bool operator()(const pair<T1, T2> &x, const pair<T1, T2> &y) const
  {
    return x.first > y.first;
  }
};

template <typename T1, typename T2>
struct FirstEqual
{
  bool operator()(const pair<T1, T2> &x, const pair<T1, T2> &y) const
  {
    return x.first == y.first;
  }
};

template <typename T1, typename T2>
struct SecondLess
{
  bool operator()(const pair<T1, T2> &x, const pair<T1, T2> &y) const
  {
    return x.second < y.second;
  }
};

template <typename T1, typename T2>
struct SecondMore
{
  bool operator()(const pair<T1, T2> &x, const pair<T1, T2> &y) const
  {
    return x.second > y.second;
  }
};

template <typename T1, typename T2>
struct SecondEqual
{
  bool operator()(const pair<T1, T2> &x, const pair<T1, T2> &y) const
  {
    return x.second == y.second;
  }
};


typedef uint64 Word;

struct WordHash
{
  size_t operator()(const Word x) const
  {
    return size_t(x & 0xFFFFFFFF) ^ size_t(x >> 32);
  }
};

struct WordEqual
{
  bool operator()(const Word a, const Word b) const
  {
    return a == b;
  }
};


template <typename T>
void Intersect(const set<T> &X, const set<T> &Y, set<T> &result)
{
  typename set<T>::const_iterator x = X.begin();
  typename set<T>::const_iterator y = Y.begin();

  if ((x != X.end()) && (y != Y.end()))
  {
    while (true)
    {
      if ((*x) < (*y))
      {
        if ((++x) == X.end())
          break;
      }
      else if ((*y) < (*x))
      {
        if ((++y) == Y.end())
          break;
      }
      else
      {
        result.insert(*x);
        ++x;
        ++y;
        if ((x == X.end()) || (y == Y.end()))
          break;
      }
    }
  }
}

template <typename T, typename Compare>
void Intersect(const set<T> &X, const set<T> &Y, set<T> &result, 
               Compare compare = Compare())
{
  typename set<T>::const_iterator x = X.begin();
  typename set<T>::const_iterator y = Y.begin();

  if ((x != X.end()) && (y != Y.end()))
  {
    while (true)
    {
      if (compare(*x, *y))
      {
        if ((++x) == X.end())
          break;
      }
      else if (compare(*y, *x))
      {
        if ((++y) == Y.end())
          break;
      }
      else
      {
        result.insert(*x);
        ++x;
        ++y;
        if ((x == X.end()) || (y == Y.end()))
          break;
      }
    }
  }
}


template <typename T>
void Union(const set<T> &X, const set<T> &Y, set<T> &result)
{
  for (typename set<T>::const_iterator x = X.begin();
       x != X.end(); ++x)
    result.insert(*x);

  for (typename set<T>::const_iterator y = Y.begin();
       y != Y.end(); ++y)
    result.insert(*y);
}

template <typename T>
void Subtract(const set<T> &X, const set<T> &Y, set<T> &result)
{
  typename set<T>::const_iterator x = X.begin();
  typename set<T>::const_iterator y = Y.begin();

  if (x == X.end())
    return;

  if (y != Y.end())
  {
    while (true)
    {
      if ((*x) < (*y))
      {
        result.insert(*x);
        if ((++x) == X.end())
          return;
      }
      else if ((*y) < (*x))
      {
        if ((++y) == Y.end())
          break;
      }
      else
      {
        ++x;
        ++y;
        if (x == X.end())
          return;
        if (y == Y.end())
          break;
      }
    }
  }

  for ( ; x != X.end(); ++x)
    result.insert(*x);
}  

template <typename T>
void VennDiagram(const set<T> &X, const set<T> &Y, 
                 set<T> &onlyX, set<T> &onlyY, set<T> &shared)
{
  typename set<T>::const_iterator x = X.begin();
  typename set<T>::const_iterator y = Y.begin();

  if ((x != X.end()) && (y != Y.end()))
  {
    while (true)
    {
      if ((*x) < (*y))
      {
        onlyX.insert(*x);
        if ((++x) == X.end())
          break;
      }
      else if ((*y) < (*x))
      {
        onlyY.insert(*y);
        if ((++y) == Y.end())
          break;
      }
      else
      {
        shared.insert(*x);
        ++x;
        ++y;
        if ((x == X.end()) || (y == Y.end()))
          break;
      }
    }
  }

  for ( ; x != X.end(); ++x)
    onlyX.insert(*x);

  for ( ; y != Y.end(); ++y)
    onlyY.insert(*y);
}

template <typename T1, typename T2>
void MapIndices(const map<T1, T2> &m, set<T1> &s)
{
  for (typename map<T1, T2>::const_iterator iter = m.begin(); 
       iter != m.end(); ++iter)
    s.insert(iter->first);
}

template <typename T1, typename T2>
void MapValues(const map<T1, T2> &m, set<T2> &s)
{
  for (typename map<T1, T2>::const_iterator iter = m.begin(); 
       iter != m.end(); ++iter)
    s.insert(iter->second);
}

template <typename T1, typename T2, typename HashT1, typename EqualT1>
void HashMapIndices(const unordered_map<T1, T2, HashT1, EqualT1> &m, set<T1> &s)
{
  for (typename unordered_map<T1, T2, HashT1, EqualT1>::const_iterator 
         iter = m.begin(); iter != m.end(); ++iter)
    s.insert(iter->first);
}

template <typename T1, typename T2, typename HashT1, typename EqualT1>
void HashMapValues(const unordered_map<T1, T2, HashT1, EqualT1> &m, set<T2> &s)
{
  for (typename unordered_map<T1, T2, HashT1, EqualT1>::const_iterator 
         iter = m.begin(); iter != m.end(); ++iter)
    s.insert(iter->second);
}

template <typename RandomAccessIterator, class Compare>
struct SortOrderCompare
{
  typedef typename iterator_traits<RandomAccessIterator>::value_type T;

  RandomAccessIterator first;
  RandomAccessIterator last;
  Compare &compare;

  SortOrderCompare(RandomAccessIterator first_, RandomAccessIterator last_,
                   Compare &compare_)
    : first(first_), last(last_), compare(compare_)
  {
  }

  bool operator()(int x, int y)
  {
    return compare(*(first + x), *(first + y));
  }
};

template <typename RandomAccessIterator>
void SortOrder(RandomAccessIterator first, RandomAccessIterator last,
               vector<int> &order)
{
  typedef typename iterator_traits<RandomAccessIterator>::value_type T;

  int size = last - first;
  order.clear();
  order.resize(size);

  for (int i = 0; i < size; ++i)
    order[i] = i;

  less<T> compare;
  SortOrderCompare< RandomAccessIterator, less<T> > order_compare(first, last, compare);
                                 
  sort(order.begin(), order.end(), order_compare);
}
  
template <typename RandomAccessIterator, class Compare>
void SortOrder(RandomAccessIterator first, RandomAccessIterator last,
               vector<int> &order, Compare compare)
{
  typedef typename iterator_traits<RandomAccessIterator>::value_type T;

  int size = last - first;
  order.clear();
  order.resize(size);

  for (int i = 0; i < size; ++i)
    order[i] = i;
  
  SortOrderCompare<RandomAccessIterator, Compare> order_compare(first, last, compare);
  sort(order.begin(), order.end(), order_compare);
}

template <typename T1, typename T2, typename T3>
class triple
{
public:
  T1 first;
  T2 second;
  T3 third;

  triple() { }

  triple(const T1 &first_ = T1(), const T2 &second_ = T2(), 
         const T3 &third_ = T3())
    : first(first_), second(second_), third(third_)
  {
  }
};

template <typename T1, typename T2, typename T3>
triple<T1, T2, T3> make_triple(const T1 &first,
                               const T2 &second,
                               const T3 &third)
{
  return triple<T1, T2, T3>(first, second, third);
}


int Char2Qual(char c);
char Qual2Char(int q);
void MakeQualString(int len, int q, string &qual);

int StringCaseCmp(const string &s1, const string &s2);

struct StringCaseLess
{
  bool operator()(const string &x, const string &y) const
  {
    return (StringCaseCmp(x, y) < 0);
  }
};

struct StringVersionLess
{
  bool operator()(const string &x, const string &y) const
  {
    return (StringVersionCmp(x, y) < 0);
  }
};


ostream *OutFileStream(const string &filename);
istream *InFileStream(const string &filename);
  
#endif
