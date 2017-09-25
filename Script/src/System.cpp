// System.cpp : Defines all platform-dependent code
//

#include "System.h"
#include <time.h>
#include <unistd.h>

string cmd_dir, cmd_name;

int main(int argc, char **argv)
{
  vector<string> args(argc);
  for (int i = 0; i < argc; ++i)
    args[i] = argv[i];

  SplitPath(args[0], cmd_dir, cmd_name);
  int result = Main(args);
  return result;
}

void UserPause()
{
  
  getchar();
}

void Exit(int status)
{
  exit(status);
}

uint32 TimerSeconds()
{
  return (uint32)time((time_t *)NULL);
}

uint32 RandomSeed()
{
  uint32 p = uint32(getpid());
  return TimerSeconds() + (p >> 16) + (p << 16);
}

double drand()
{
  if (RAND_MAX == 0x7FFF)
  {
    uint64 r = rand();
    r <<= 15;
    r += rand();
    r <<= 15;
    r += rand();
    return (double)r / (double)(32768.0 * 32768.0 * 32768.0);
  }
  else if (RAND_MAX == 0x7FFFFFFF)
  {
    uint64 r = rand();
    r <<= 31;
    r += rand();
    return (double)r / (double)(2147483648.0 * 2147483648.0);
  }
  else
    Assert(false);
  return 0.0;
}

int irand()
{
  if (RAND_MAX == 0x7FFF)
  {
    uint32 r = rand();
    r <<= 15;
    r += rand();
    return (int)r;
  }
  else if (RAND_MAX == 0x7FFFFFFF)
    return (int)rand();
  else
    Assert(false);
  return 0;
}

void SplitPath(const string &path, string &dir, string &file)
{
  int i = path.length() - 1;
  while (i > 0)
  {
    if ((path[i - 1] == '/') || (path[i - 1] == '\\'))
      break;
    --i;
  }
  
  dir = path.substr(0, i);
  file = path.substr(i, path.length() - i);
}

void RelPath(const string &dir, const string &file, string &path)
{
  if ((file.length() > 0) && ((file[0] == '/') || (file[0] == '\\')))
    path = file;
  else
  {
    if ((dir.length() > 0) && 
        (dir[dir.length() - 1] != '/') && (dir[dir.length() - 1] != '\\'))
      path = dir + "/" + file;
    else
      path = dir + file;
  }
}

string RelPath(const string &dir, const string &file)
{
  string path;
  RelPath(dir, file, path);
  return path;
}

bool RemoveFile(const string &filename)
{
  return (remove(filename.c_str()) == 0);
}

bool RenameFile(const string &source, const string &dest)
{
  return (rename(source.c_str(), dest.c_str()) == 0);
}
  

bool MakeDir(const string &path)
{
  return (mkdir(path.c_str(), 0777) == 0);
}

bool RemoveDir(const string &path)
{
  return (rmdir(path.c_str()) == 0);
}

bool RemoveRecursiveDir(const string &path)
{
  vector<string> files, subdirs;
  if (!GetDirectoryList(path, files, subdirs))
    return false;
  
  for (int i = 0; i < subdirs.size(); ++i)
    if (!RemoveRecursiveDir(RelPath(path, subdirs[i])))
      return false;

  for (int i = 0; i < files.size(); ++i)
    if (!RemoveFile(RelPath(path, files[i])))
      return false;

  return RemoveDir(path);
}

int CallMAligner(const string &filename1, const string &filename2,
                 const string &outfilename)
{
  string cmdpath;
  RelPath(cmd_dir, "MAligner", cmdpath);
  stringstream ss;
  ss << cmdpath << " " << filename1 << " " << filename2 << " " << outfilename;
  string cmdline = ss.str();

  cout << "Executing \"" << cmdline << "\"" << endl;
  int status = system(cmdline.c_str());
  Assert(status >= 0);
  return status;
}

int debug_assert_failed = 0;

void debug_assert_break()
{
  ++debug_assert_failed;
}

void debug_assert(bool result, string expression, string file, int line)
{
  if (result)
    return;

  cerr << "Assertion \"" << expression << "\" failed" << endl;
  cerr << "  in file " << file << " line " << line << endl;
  debug_assert_break();
  assert_terminate();
}

void debug_assert_msg(bool result, string expression, string msg, string file, int line)
{
  if (result)
    return;

  cerr << "Assertion \"" << expression << "\" failed" << endl;
  cerr << "  in file " << file << " line " << line << endl;
  cerr << msg << endl;
  debug_assert_break();
  assert_terminate();
}


bool FileReadable(const string &filename)
{
  ifstream f(filename.c_str(), ifstream::in);
  return f.is_open();
}


string GetExtension(const string &path)
{
  int i = path.length();
  while (i > 0)
  {
    if (path[i - 1] == '.')
      return path.substr(i, path.length() - i);
    if ((path[i - 1] == '/') || (path[i - 1] == '\\'))
      break;
    --i;
  }
  return "";
}

string RemoveExtension(const string &path)
{
  int i = path.length();
  while (i > 0)
  {
    if (path[i - 1] == '.')
      return path.substr(0, i - 1);
    if ((path[i - 1] == '/') || (path[i - 1] == '\\'))
      break;
    --i;
  }
  return path;
}

string RemoveBZ2Extension(const string &path)
{
  if (BZ2Extension(path))
    return RemoveExtension(path);
  else
    return path;
}

bool BZ2Extension(const string &path)
{
  string ext = GetExtension(path);
  for (int i = 0; i < ext.length(); ++i)
    ext[i] = char(tolower(int(ext[i])));
  return (ext == "bz") || (ext == "bz2");
}

string RemoveGZExtension(const string &path)
{
  if (GZExtension(path))
    return RemoveExtension(path);
  else
    return path;
}

bool GZExtension(const string &path)
{
  string ext = GetExtension(path);
  for (int i = 0; i < ext.length(); ++i)
    ext[i] = char(tolower(int(ext[i])));
  return (ext == "z") || (ext == "gz");
}

string RemoveCompressedExtension(const string &path)
{
  return RemoveGZExtension(RemoveBZ2Extension(path));
}

string Fas2QualFilename(const string &fas_filename)
{
  string name = fas_filename;
  if (BZ2Extension(name))
    name = RemoveExtension(name);
  if (GZExtension(name))
    name = RemoveExtension(name);
  
  if (FileReadable(name + ".qual"))
    return name + ".qual";
  if (FileReadable(name + ".qual.bz2"))
    return name + ".qual.bz2";
  if (FileReadable(name + ".qual.gz"))
    return name + ".qual.gz";
  
  if (!GetExtension(name).empty())
    name = RemoveExtension(name);
  
  if (FileReadable(name + ".qual"))
    return name + ".qual";
  if (FileReadable(name + ".qual.bz2"))
    return name + ".qual.bz2";
  if (FileReadable(name + ".qual.gz"))
    return name + ".qual.gz";
  
  return "";
}


/***/

bz2streambuf::bz2streambuf()
{
  error = true;
  buf = NULL;
  bzfile = NULL;
}

bz2streambuf::~bz2streambuf()
{
}

void bz2streambuf::Init(FILE *f_, bool write_)
{
  write = write_;
  Open(f_);
  if (error)
    return;
  
  buf = (char *)malloc(sizeof(char) * bufsize);
  AssertMsg(buf, "bz2streambuf: Unable to allocate buffer");
  //Assert(setbuf(buf, bufsize));

  if (write)
    setp(buf, buf + bufsize - 1);
  else
    setg(buf, buf + bufsize, buf + bufsize);
  
  error = false;
}

void bz2streambuf::Done()
{
  Close();
  
  if (buf)
  {
    free((void *)buf);
    buf = NULL;
  }
}

void bz2streambuf::Open(FILE *f_)
{
  f = f_;
  if (!write)
    bzfile = BZ2_bzReadOpen(&bzerror, f, 0, 0, NULL, 0);
  else
    bzfile = BZ2_bzWriteOpen(&bzerror, f, 5, 0, 0);
  error = (bzerror != BZ_OK);
}

void bz2streambuf::Close()
{
  if (bzfile)
  {
    if (sync() != 0)
      error = true;
    
    if (!write)
      BZ2_bzReadClose(&bzerror, bzfile);
    else
    {
      unsigned int nbytes_in, nbytes_out;
      BZ2_bzWriteClose(&bzerror, bzfile, error ? 1 : 0, 
                       &nbytes_in, &nbytes_out);
    }
    bzfile = NULL;
  }
}

void bz2streambuf::Reset()
{
  Assert(!write);
  Close();
  fseek(f, 0, SEEK_SET);
  Open(f);
  setg(buf, buf + bufsize, buf + bufsize);
}

int bz2streambuf::overflow(int c)
{
  Assert(write);
  
  int w = pptr() - pbase();
  if (c != EOF)
  {
    *pptr() = c;
    ++w;
  }
  
  BZ2_bzWrite(&bzerror, bzfile, pbase(), w * sizeof(char));
  if (bzerror == BZ_OK)
  {
    setp(buf, buf + bufsize - 1);
    return 0;
  }
  else
  {
    error = true;
    setp(0, 0);
    return EOF;
  }
}

int bz2streambuf::underflow()
{
  Assert(!write);
  
  int charsread = BZ2_bzRead(&bzerror, bzfile, (void *)buf, bufsize * sizeof(char)) / sizeof(char);
  if ((bzerror == BZ_OK) || (bzerror == BZ_STREAM_END))
  {
    if (charsread == 0)
    {
      setg(0, 0, 0);
      return EOF;
    }
    else
    {
      setg(buf, buf, buf + charsread);
      return zapeof(*buf);
    }
  }
  else
  {
    error = true;
    setg(0, 0, 0);
    return EOF;
  }
}

int bz2streambuf::sync()
{
  if (write)
  {
    if (pptr() && pptr() > pbase()) 
      return overflow(EOF);
  }
  
  return 0;
}


bool bz2base::bzOpen(FILE *f_, bool write_)
{
  buf.Init(f_, write_);
  return buf.Success();
}

bool bz2base::bzClose()
{
  buf.Done();
  return buf.Success();
}

void bz2base::bzReset()
{
  buf.Reset();
}


bz2istream::bz2istream(const string &bz2_filename) : istream(&buf)
{
  f = fopen(bz2_filename.c_str(), "rb");
  AssertMsg(f, "bz2istream: Unable to open file \"" + bz2_filename + "\"");
  
  if (!bzOpen(f, false))
    AssertMsg(false, "bz2istream: Unable to decompress file \"" + bz2_filename + "\"");
}

bz2istream::~bz2istream()
{
  bzClose();
  fclose(f);
}

void bz2istream::Reset()
{
  bzReset();
}

bz2ostream::bz2ostream(const string &bz2_filename_) 
  : bz2_filename(bz2_filename_), ostream(&buf)
{
  f = fopen(bz2_filename.c_str(), "wb");
  AssertMsg(f, "bz2ostream: Unable to create file \"" + bz2_filename + "\"");
  
  if (!bzOpen(f, true))
    AssertMsg(false, "bz2ostream: Unable to compress file \"" + bz2_filename + "\"");
}

bz2ostream::~bz2ostream()
{
  if (!bzClose())
  {
    fclose(f);
    RemoveFile(bz2_filename);
    AssertMsg(false, "bz2ostream: Unable to compress file \"" + bz2_filename + "\"");
  }
  else
    fclose(f);
}


/***/

gzstreambuf::gzstreambuf()
{
  error = true;
  open = false;
  buf = NULL;
}

gzstreambuf::~gzstreambuf()
{
  Assert(!buf);
}

void gzstreambuf::Init(FILE *f_, bool write_)
{
  write = write_;
  Open(f_);
  if (error)
    return;
  
  buf = (char *)malloc(sizeof(char) * bufsize);
  AssertMsg(buf, "gzstreambuf: Unable to allocate buffer");
  //Assert(setbuf(buf, bufsize));

  if (write)
    setp(buf, buf + bufsize - 1);
  else
    setg(buf, buf + bufsize, buf + bufsize);
  
  error = false;
}

void gzstreambuf::Done()
{
  Close();
  
  if (buf)
  {
    free((void *)buf);
    buf = NULL;
  }
}

void gzstreambuf::Open(FILE *f_)
{
  fd = fileno(f_);
  if (!write)
    gz = gzdopen(fd, "rb");
  else
    gz = gzdopen(fd, "wb");
  error = (gz == NULL);
  open = !error;
}

void gzstreambuf::Close()
{
  if (open)
  {
    if (sync() != 0)
      error = true;
    
    int result = gzclose(gz);
    if (result != Z_OK)
      error = true;

    open = false;
  }
}

void gzstreambuf::Reset()
{
  Assert(!write);
  int result = gzrewind(gz);
  if (result != Z_OK)
    error = true;
  else
    setg(buf, buf + bufsize, buf + bufsize);
}

int gzstreambuf::overflow(int c)
{
  Assert(write);
  
  int w = pptr() - pbase();
  if (c != EOF)
  {
    *pptr() = c;
    ++w;
  }
  
  int byteswritten = gzwrite(gz, (void *)pbase(), w * sizeof(char));
  if (byteswritten == (w * sizeof(char)))
  {
    setp(buf, buf + bufsize - 1);
    return 0;
  }
  else
  {
    error = true;
    setp(0, 0);
    return EOF;
  }
}

int gzstreambuf::underflow()
{
  Assert(!write);
  
  int bytesread = gzread(gz, (void *)buf, bufsize * sizeof(char));
  if (bytesread >= 0)
  {
    if (bytesread == 0)
    {
      setg(0, 0, 0);
      return EOF;
    }
    else
    {
      setg(buf, buf, buf + (bytesread / sizeof(char)));
      return zapeof(*buf);
    }
  }
  else
  {
    error = true;
    setg(0, 0, 0);
    return EOF;
  }
}

int gzstreambuf::sync()
{
  if (write)
  {
    if (pptr() && pptr() > pbase()) 
      return overflow(EOF);
  }
  
  return 0;
}


bool gzbase::gzOpen(FILE *f_, bool write_)
{
  buf.Init(f_, write_);
  return buf.Success();
}

bool gzbase::gzClose()
{
  buf.Done();
  return buf.Success();
}

void gzbase::gzReset()
{
  buf.Reset();
}


gzistream::gzistream(const string &gz_filename) : istream(&buf)
{
  f = fopen(gz_filename.c_str(), "rb");
  AssertMsg(f, "gzistream: Unable to open file \"" + gz_filename + "\"");
  
  if (!gzOpen(f, false))
    AssertMsg(false, "gzistream: Unable to decompress file \"" + gz_filename + "\"");
}

gzistream::~gzistream()
{
  gzClose();
  fclose(f);
}

void gzistream::Reset()
{
  gzReset();
}

gzostream::gzostream(const string &gz_filename_) 
  : gz_filename(gz_filename_), ostream(&buf)
{
  f = fopen(gz_filename.c_str(), "wb");
  AssertMsg(f, "gzostream: Unable to create file \"" + gz_filename + "\"");
  
  if (!gzOpen(f, true))
    AssertMsg(false, "gzostream: Unable to compress file \"" + gz_filename + "\"");
}

gzostream::~gzostream()
{
  if (!gzClose())
  {
    fclose(f);
    RemoveFile(gz_filename);
    AssertMsg(false, "gzostream: Unable to compress file \"" + gz_filename + "\"");
  }
  else
    fclose(f);
}

/***/

int StringVersionCmp(const string &s1, const string &s2)
{
  return strcmp(s1.c_str(), s2.c_str());
}


bool GetDirectoryList(const string &dir, 
                      vector<string> &files, vector<string> &subdirs)
{
  DIR *d = opendir(dir.c_str());
  if (!d)
    return false;

  string name = dir + '/';
  int base_dir_len = name.length();

  struct dirent *de;
  struct stat stat_buf;
  while (de = readdir(d))
  {
    if ((strcmp(de->d_name, ".") == 0) || (strcmp(de->d_name, "..") == 0))
      continue;
    name.resize(base_dir_len);
    name += de->d_name;
    if (!stat(name.c_str(), &stat_buf))
    {
      if (S_ISREG(stat_buf.st_mode))
        files.push_back(de->d_name);
      else if (S_ISDIR(stat_buf.st_mode))
        subdirs.push_back(de->d_name);
    }
  }

  closedir(d);
  return true;
}

int CallProgram(const string &program, const vector<string> &args,
                const string &stdin_file, const string &stdout_file,
                const string &stderr_file)
{
  string cmdline = program;
  for (int i = 0; i < args.size(); ++i)
  {
    cmdline += ' ';
    if ((args[i].find('"') == string::npos) &&
        (args[i].find('\'') == string::npos))
      cmdline += args[i];
    else
    {
      for (int j = 0; j < args[i].length(); ++j)
      {
        if ((args[i][j] == '"') || (args[i][j] == '\''))
          cmdline += '\\';
        cmdline += args[i][j];
      }
    }
  }

  if (!stdin_file.empty())
  {
    cmdline += " < ";
    cmdline += stdin_file;
  }
  if (!stdout_file.empty())
  {
    cmdline += " > ";
    cmdline += stdout_file;
  }
  if (!stderr_file.empty())
    cmdline = "( " + cmdline + " ) >& " + stderr_file;

  //cerr << "Executing \"" << cmdline << "\"";
  int result = system(cmdline.c_str());
  //cerr << " done." << endl;
  return result;
}
