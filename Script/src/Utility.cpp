#include "System.h"
#include "Utility.h"

/* grand(): Returns a random number with a Guassian distribution
 * with mean 0 and standard deviation 1.0 */

double grand()
{
  static bool polarity = false;
  static double next;

  if (!polarity)
  {
    double x1, x2, w;
    do
    {
      x1 = 2.0 * drand() - 1.0;
      x2 = 2.0 * drand() - 1.0;
      w = x1 * x1 + x2 * x2;
    } while (w >= 1.0);
    
    w = sqrt((-2.0 * log(w)) / w);
    next = x2 * w;
    polarity = true;
    return  x1 * w;
  }
  else
  {
    polarity = false;
    return next;
  }
}

double npdf(double z)
{
  return exp(-0.5*z*z)/sqrt(2.0 * M_PI);
}

double ncdf(double z)
{
  double x = (z >= 0.0) ? z : -z;
  double t = 1.0 / (1.0 + 0.33267*x);
  double c = 1.0 - npdf(x) * t * (0.4361836 + t * (-0.1201676 + t * 0.9372980));
  if (z >= 0.0)
    return c;
  else
    return 1.0 - c;
}


// BaseTablesReady -- indicates whether InitBaseTables() has been called
bool BaseTablesReadyFlag = false;

char base2char[4];
int char2base[256];
int basecomplement[4];
char charcomplement[256];
char multibase2char[16];
uint8 rc_1mer[4];
uint8 rc_2mer[16];
uint8 rc_3mer[64];
uint8 rc_4mer[256];

char symbol2char[7];
int char2symbol[256];
int symbolcomplement[7];

double qualprob[200];

uint32 crc32tab[256];


void InitBaseTables()
{
  base2char[0] = 'A';
  base2char[1] = 'C';
  base2char[2] = 'G';
  base2char[3] = 'T';

  symbol2char[0] = 'A';
  symbol2char[1] = 'C';
  symbol2char[2] = 'G';
  symbol2char[3] = 'T';
  symbol2char[4] = 'N';
  symbol2char[5] = '-';
  symbol2char[6] = '?';

  for (int i = 0; i < 256; ++i)
    char2base[i] = -1;

  char2base[(int)'A'] = 0;
  char2base[(int)'a'] = 0;
  char2base[(int)'C'] = 1;
  char2base[(int)'c'] = 1;
  char2base[(int)'G'] = 2;
  char2base[(int)'g'] = 2;
  char2base[(int)'T'] = 3;
  char2base[(int)'t'] = 3;

  for (int i = 0; i < 256; ++i)
    char2symbol[i] = 6;

  char2symbol[(int)'A'] = 0;
  char2symbol[(int)'a'] = 0;
  char2symbol[(int)'C'] = 1;
  char2symbol[(int)'c'] = 1;
  char2symbol[(int)'G'] = 2;
  char2symbol[(int)'g'] = 2;
  char2symbol[(int)'T'] = 3;
  char2symbol[(int)'t'] = 3;
  char2symbol[(int)'N'] = 4;
  char2symbol[(int)'n'] = 4;
  char2symbol[(int)'-'] = 5;

  basecomplement[0] = 3;
  basecomplement[1] = 2;
  basecomplement[2] = 1;
  basecomplement[3] = 0;

  symbolcomplement[0] = 3;
  symbolcomplement[1] = 2;
  symbolcomplement[2] = 1;
  symbolcomplement[3] = 0;
  symbolcomplement[4] = 4;
  symbolcomplement[5] = 5;
  symbolcomplement[6] = 6;

  for (int i = 0; i < 256; ++i)
    charcomplement[i] = 0;

  for (int i = 0; i < 256; ++i)
  {
    if (char2symbol[i] >= 0)
      charcomplement[i] = symbol2char[symbolcomplement[char2symbol[i]]];
  }

  for (int i = 0; i < 4; ++i)
    rc_1mer[i] = basecomplement[i];

  for (int i = 0; i < 16; ++i)
    rc_2mer[i] = 
      (basecomplement[(i >> 0) & 3] << 2) | 
      (basecomplement[(i >> 2) & 3] << 0);

  for (int i = 0; i < 64; ++i)
    rc_3mer[i] = 
      (basecomplement[(i >> 0) & 3] << 4) |
      (basecomplement[(i >> 2) & 3] << 2) |
      (basecomplement[(i >> 4) & 3] << 0);

  for (int i = 0; i < 256; ++i)
    rc_4mer[i] = 
      (basecomplement[(i >> 0) & 3] << 6) |
      (basecomplement[(i >> 2) & 3] << 4) |
      (basecomplement[(i >> 4) & 3] << 2) |
      (basecomplement[(i >> 6) & 3] << 0);

  for (int i = 0; i < 200; ++i)
  {
    double qual = -double(i) / 10.0;
    qualprob[i] = pow(10.0, qual);
  }

  for (int i = 0; i < 16; ++i)
    multibase2char[i] = 0;
  multibase2char[(1 << 0)] = 'A';
  multibase2char[(1 << 1)] = 'C';
  multibase2char[(1 << 2)] = 'G';
  multibase2char[(1 << 3)] = 'T';
  multibase2char[(1 << 0) | (1 << 1)] = 'M';
  multibase2char[(1 << 0) | (1 << 2)] = 'R';
  multibase2char[(1 << 0) | (1 << 3)] = 'W';
  multibase2char[(1 << 1) | (1 << 2)] = 'S';
  multibase2char[(1 << 1) | (1 << 3)] = 'Y';
  multibase2char[(1 << 2) | (1 << 3)] = 'K';
  multibase2char[(1 << 0) | (1 << 1) | (1 << 2)] = 'V';
  multibase2char[(1 << 0) | (1 << 1) | (1 << 3)] = 'H';
  multibase2char[(1 << 0) | (1 << 2) | (1 << 3)] = 'D';
  multibase2char[(1 << 1) | (1 << 2) | (1 << 3)] = 'B';
  multibase2char[(1 << 0) | (1 << 1) | (1 << 2) | (1 << 3)] = 'N';

  BaseTablesReadyFlag = true;
}

bool BaseTablesReady()
{
  return BaseTablesReadyFlag;
}

uint64 ReverseComplement(int kmer_size, uint64 kmer)
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

void ReverseComplement(string &s, bool retain_nonacgt)
{
  int len = s.length();
  int mid = len / 2;
  for (int i = 0; i < mid; ++i)
  {
    char c1 = charcomplement[s[i]];
    char c2 = charcomplement[s[len - 1 - i]];
    if (retain_nonacgt)
    {
      if (!c1)
        c1 = s[i];
      if (!c2)
        c2 = s[len - 1 - i];
    }
    s[i] = c2;
    s[len - 1 - i] = c1;
  }

  if ((mid * 2) != len)
  {
    char c = charcomplement[s[mid]];
    if (retain_nonacgt && !c)
      c = s[mid];
    s[mid] = c;
  }
}

void ReverseComplement(const string &source, string &dest, bool retain_nonacgt)
{
  int len = source.length();
  dest.resize(len, ' ');
  for (int i = 0; i < len; ++i)
  {
    char c = charcomplement[source[i]];
    if (retain_nonacgt && !c)
      c = source[i];
    dest[len - 1 - i] = c;
  }
}


void Reverse(string &s)
{
  int len = s.length();
  int mid = len / 2;
  for (int i = 0; i < mid; ++i)
    swap(s[i], s[len - 1 - i]);
}


void ReadFileList(const string &listfname, vector<string> &files)
{
  string listdir, listfile;
  SplitPath(listfname, listdir, listfile);

  ifstream f;
  f.open(listfname.c_str(), ifstream::in);
  AssertMsg(f.is_open(), "Unable to open file \"" + listfname + "\"");
  
  files.clear();
  string line;
  while (getline(f, line))
  {
    if (!line.empty())
      files.push_back(line);
  }

  for (int i = 0; i < (int)files.size(); ++i)
      RelPath(listdir, files[i], files[i]);

  f.close();
}


size_t GetFileSize(const string &filename)
{
  ifstream f(filename.c_str(), ifstream::in | ifstream::binary);
  AssertMsg(f.is_open(), "Unable to open file \"" + filename + "\"");

  f.seekg(0, ifstream::end);
  size_t pos = f.tellg();
  f.close();

  return pos;
}


void ReadFile(const string &filename, vector<string> &lines, bool discard_empty)
{
  ifstream f(filename.c_str(), ifstream::in);
  if (!f.is_open())
  {
    cerr << "Unable to open file \"" << filename << "\"" << endl;
    Exit(1);
  }

  lines.clear();
  string line;
  while (getline(f, line))
  {
    if (!discard_empty || !line.empty())
      lines.push_back(line);
  }
  f.close();
}

void WriteFile(const string &filename, const vector<string> &lines)
{
  ofstream f(filename.c_str(), ofstream::in | ofstream::trunc);
  if (!f.is_open())
  {
    cerr << "Unable to create file \"" << filename << "\"" << endl;
    Exit(1);
  }

  for (int i = 0; i < (int)lines.size(); ++i)
    f << lines[i] << "\n";
  f.close();
}


void SplitFields(const string &s, vector<string> &fields, const string &delim)
{
  fields.clear();
  for (int pos = 0; pos < s.length(); ++pos)
  {
    string temp;
    while ((pos < s.length()) && (delim.find(s[pos]) == string::npos))
      temp += s[pos++];
    if (!temp.empty())
      fields.push_back(temp);
  }
}

void SplitStrictFields(const string &s, vector<string> &fields,
                       const string &delim)
{
  fields.clear();
  fields.push_back(string());
  for (int pos = 0; pos < s.length(); ++pos)
  {
    if (delim.find(s[pos]) != string::npos)
      fields.push_back(string());
    else
      fields.back() += s[pos];
  }
}

void SplitTabFields(const string &s, vector<string> &fields)
{
#if 1
  int num_fields = 1;
  for (int pos = 0; pos < s.length(); ++pos)
    if (s[pos] == '\t')
      ++num_fields;

  fields.resize(num_fields);
  int index = 0;
  int last = 0;
  for (int pos = 0; pos < s.length(); ++pos)
    if (s[pos] == '\t')
    {
      fields[index++] = s.substr(last, pos - last);
      last = pos + 1;
    }
  fields[index] = s.substr(last, s.length() - last);
#else
  fields.clear();

  int start = 0;
  while (start < (int)s.length())
  {
    int pos = s.find('\t', start);
    if (pos == (int)string::npos)
      pos = s.length();

    fields.push_back(s.substr(start, pos - start));
    if (pos == (int)s.length())
      start = pos;
    else
      start = pos + 1;
  }
#endif
}
  

const int small_primes[] =
{
#include "SmallPrimes.inc"
};

const int num_small_primes = sizeof(small_primes) / sizeof(small_primes[0]);


const int nice_steps[] =
{
  1, 2, 5, 10, 20, 25, 50, 100, 200, 250, 500, 1000, 2000, 2500, 5000,
  10000, 20000, 25000, 50000, 100000, 200000, 250000, 500000,
  1000000, 2000000, 2500000, 5000000, 10000000, 20000000, 25000000, 50000000,
  100000000, 200000000, 250000000, 500000000, 1000000000
};

const int num_nice_steps = sizeof(nice_steps) / sizeof(nice_steps[0]);

int FindNiceStep(int x)
{
  return *upper_bound(nice_steps, nice_steps + num_nice_steps, x);
}


template <>
void Write<int16>(ostream &o, const int16 &x)
{
  o.write((const char *)&x, sizeof(x));
}

template <>
void Read<int16>(istream &i, int16 &x)
{
  i.read((char *)&x, sizeof(x));
}

template <>
void Write<uint16>(ostream &o, const uint16 &x)
{
  o.write((const char *)&x, sizeof(x));
}

template <>
void Read<uint16>(istream &i, uint16 &x)
{
  i.read((char *)&x, sizeof(x));
}

template <>
void Write<int32>(ostream &o, const int32 &x)
{
  o.write((const char *)&x, sizeof(x));
}

template <>
void Read<int32>(istream &i, int32 &x)
{
  i.read((char *)&x, sizeof(x));
}

template <>
void Write<uint32>(ostream &o, const uint32 &x)
{
  o.write((const char *)&x, sizeof(x));
}

template <>
void Read<uint32>(istream &i, uint32 &x)
{
  i.read((char *)&x, sizeof(x));
}

template <>
void Write<int64>(ostream &o, const int64 &x)
{
  o.write((const char *)&x, sizeof(x));
}

template <>
void Read<int64>(istream &i, int64 &x)
{
  i.read((char *)&x, sizeof(x));
}

template <>
void Write<uint64>(ostream &o, const uint64 &x)
{
  o.write((const char *)&x, sizeof(x));
}

template <>
void Read<uint64>(istream &i, uint64 &x)
{
  i.read((char *)&x, sizeof(x));
}

template <>
void Write<double>(ostream &o, const double &x)
{
  o.write((const char *)&x, sizeof(x));
}

template <>
void Read<double>(istream &i, double &x)
{
  i.read((char *)&x, sizeof(x));
}

template <>
void Write<float>(ostream &o, const float &x)
{
  o.write((const char *)&x, sizeof(x));
}

template <>
void Read<float>(istream &i, float &x)
{
  i.read((char *)&x, sizeof(x));
}

template <>
void Write<string>(ostream &o, const string &x)
{
  int size = x.length();
  o.write((const char *)&size, sizeof(size));
  o.write((const char *)x.data(), sizeof(char) * size);
}

template <>
void Read<string>(istream &i, string &x)
{
  int size;
  i.read((char *)&size, sizeof(size));
  vector<char> buf(size);
  i.read((char *)&buf[0], sizeof(char) * size);
  x.assign((const char *)&buf[0], size);
}

template <>
void Write<bool>(ostream &o, const vector<bool> &x)
{
  int size = x.size();
  o.write((const char *)&size, sizeof(size));

  int bufbytes = (size + 7) / 8;
  char *buf = (char *)calloc(1, sizeof(char) * bufbytes);
  Assert(buf);
  for (int i = 0; i < size; ++i)
    if (x[i])
      buf[i >> 3] |= (1 << (i & 7));
    
  o.write((const char *)buf, bufbytes);
  free((void *)buf);
}

template <>
void Read<bool>(istream &i, vector<bool> &x)
{
  int size;
  i.read((char *)&size, sizeof(size));
  int bufbytes = (size + 7) / 8;
  char *buf = (char *)malloc(sizeof(char) * bufbytes);
  Assert(buf);
  i.read((char *)buf, bufbytes);

  x.clear();
  x.resize(size, false);
  for (int j = 0; j < size; ++j)
    if (buf[j >> 3] & (1 << (j & 7)))
      x[j] = true;

  free((void *)buf);
}


void Write(ostream &o, const char *buf, int64 bytes)
{
  const int64 block_size = 1048576;
  while (bytes >= block_size)
  {
    o.write(buf, block_size);
    bytes -= block_size;
    buf += block_size;
  }

  if (bytes > 0)
    o.write(buf, bytes);
}

void Read(istream &i, char *buf, int64 bytes)
{
  const int64 block_size = 1048576;
  while (bytes >= block_size)
  {
    i.read(buf, block_size);
    bytes -= block_size;
    buf += block_size;
  }

  if (bytes > 0)
    i.read(buf, bytes);
}



void ToUpper(string &s)
{
  for (int i = 0; i < (int)s.length(); ++i)
    s[i] = toupper(s[i]);
}

void ToLower(string &s)
{
  for (int i = 0; i < (int)s.length(); ++i)
    s[i] = tolower(s[i]);
}

string GetUpper(const string &s)
{
  string result = s;
  ToUpper(result);
  return result;
}

string GetLower(const string &s)
{
  string result = s;
  ToLower(result);
  return result;
}

string TrimSpace(const string &s)
{
  int start = 0;
  while ((start < (int)s.length()) && isspace(s[start]))
    ++start;

  int end = s.length();
  while ((end > 0) && isspace(s[end - 1]))
    --end;

  return s.substr(start, end - start);
}


bool StringToInt64(const string &s, int64 &value, int &endpos, int startpos)
{
  return StringToInt(s, value, endpos, startpos);
}

ostream &operator<<(ostream &o, const SensSpec &sens_spec)
{
  o << "Hits:\tmy miss\tmy hit" << "\n";

  o << "miss\t" << sens_spec.Get(false, false);
  o << "\t" << sens_spec.Get(false, true);
  o << "\t" << "Sensitivity = ";
  o << (100.0 * sens_spec.Sensitivity()) << "%\n";

  o << "hit\t" << sens_spec.Get(true, false);
  o << "\t" << sens_spec.Get(true, true);
  o << "\t" << "Specificity = ";
  o << (100.0 * sens_spec.Specificity()) << "%\n";
 
  return o;
}


string TempFilename()
{
  stringstream s;
  s << "tmp_";

  uint32 rand_seed = uint32(TimerSeconds()) + rand();

  for (int i = 0; i < 4; ++i)
  {
    rand_seed = rand_seed * 1103515245 + 12345;

    int digit = (rand_seed / 65536) % 36;
    if (digit < 10)
      s << char('0' + digit);
    else
      s << char('a' + (digit - 10));
  }

  s << ".tmp";
  return s.str();
}

const int max_int_factorial_table = 12;
int int_factorial_table[max_int_factorial_table + 1];

const int max_factorial_table = 500;
double factorial_table[max_factorial_table + 1];

void InitFactTables()
{
  int_factorial_table[0] = 1;
  {
    int product = 1;
    for (int i = 1; i <= max_int_factorial_table; ++i)
    {
      product *= i;
      int_factorial_table[i] = product;
    }
  }

  factorial_table[0] = 1.0;
  {
    double product = 1.0;
    for (int i = 1; i <= max_factorial_table; ++i)
    {
      product *= double(i);
      factorial_table[i] = product;
    }
  }
}

int IntFactorial(int n)
{
  Assert(n <= max_int_factorial_table);
  return int_factorial_table[n];
}

double Factorial(int n)
{
  Assert(n <= max_factorial_table);
  return factorial_table[n];
}

int IntCombinations(int n, int k)
{
  Assert(n <= max_int_factorial_table);
  Assert(k <= n);

  return IntFactorial(n) / (IntFactorial(n - k) * IntFactorial(k));
}

double Combinations(int n, int k)
{
  Assert(n <= max_factorial_table);
  Assert(k <= n);

  return Factorial(n) / (Factorial(n - k) * Factorial(k));
}

double Binomial(int n, int k, double p)
{
  return Combinations(n, k) * pow(p, k) * pow(1.0 - p, n - k);
}

string SecondsToTime(int seconds)
{
  int hours = seconds / 3600;
  seconds -= hours * 3600;

  int minutes = seconds / 60;
  seconds -= minutes * 60;

  stringstream result;
  bool pad = false;

  if (pad || (hours > 0))
  {
    if (pad)
      result << StrPad(hours, 2, '0');
    else
      result << hours;
    result << ":";
    pad = true;
  }

  if (pad || (minutes > 0))
  {
    if (pad)
      result << StrPad(minutes, 2, '0');
    else
      result << minutes;
    result << ":";
    pad = true;
  }

  if (pad)
    result << StrPad(seconds, 2, '0');
  else
    result << seconds;

  return result.str();
}


int Char2Qual(char c) 
{ 
  return int(c - '!'); 
}

char Qual2Char(int q)
{
  if (q < 0) q = 0;
  if (q > 90) q = 90;
  return char('!' + q);
}

void MakeQualString(int len, int q, string &qual)
{
  char c = Qual2Char(q);
  qual.clear();
  qual.resize(len, c);
}

int StringCaseCmp(const string &s1, const string &s2)
{
  string::const_iterator iter1 = s1.begin();
  string::const_iterator iter2 = s2.begin();
  while ((iter1 != s1.end()) && (iter2 != s2.end()))
  {
    int c1 = toupper(*iter1);
    int c2 = toupper(*iter2);
    if (c1 != c2)
      return (c1 < c2) ? -1 : 1;
    ++iter1;
    ++iter2;
  }

  if (iter1 != s1.end())
    return 1;
  else if (iter2 != s2.end())
    return -1;
  return 0;
}

void InitCRC32Table()
{
  for (int i = 0; i < 256; ++i)
  {
    uint32 crc = uint32(i) << 24;
    for (int j = 0; j < 8; ++j)
    {
      bool hibit = (crc & 0x80000000) != 0;
      crc = (crc << 1);
      if (hibit)
        crc ^= 0x04c11db7;
    }
    crc32tab[i] = crc;
  }
}

uint32 CRC32UpdateChar(char data, uint32 crc)
{
  return (crc << 8) ^ (crc32tab[(crc >> 24) ^ data]);
}

uint32 CRC32UpdateBuf(char *buf, size_t bytes, uint32 crc)
{
  for (int i = 0; i < bytes; ++i)
    crc = CRC32UpdateChar(buf[i], crc);
  return crc;
}

template <>
uint32 CRC32Update<char *>(char * const &data, uint32 crc)
{
  for (int i = 0; data[i]; ++i)
    crc = CRC32UpdateChar(data[i], crc);
  return crc;
}

template <>
uint32 CRC32Update<string>(const string &data, uint32 crc)
{
  int len = data.length();
  for (int i = 0; i < len; ++i)
    crc = CRC32UpdateChar(data[i], crc);
  return crc;
}


ostream *OutFileStream(const string &filename)
{
  if (BZ2Extension(filename))
    return new bz2ostream(filename);
  else if (GZExtension(filename))
    return new gzostream(filename);
  else
  {
    ofstream *f = new ofstream(filename.c_str());
    if (!f->is_open())
    {
      delete f;
      return NULL;
    }
    else
      return (ostream *)f;
  }
}

istream *InFileStream(const string &filename)
{
  if (BZ2Extension(filename))
    return new bz2istream(filename);
  else if (GZExtension(filename))
    return new gzistream(filename);
  else
  {
    ifstream *f = new ifstream(filename.c_str());
    if (!f->is_open())
    {
      delete f;
      return NULL;
    }
    else
      return (istream *)f;
  }
}
  
