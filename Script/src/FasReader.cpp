
#include "System.h"
#include "Utility.h"
#include "FasReader.h"

FasReader::FasReader()
{
  fas_f = NULL;
  fas_ftype = None;
  qual_f = NULL;
  qual_ftype = None;
}

FasReader::FasReader(const string &fas_filename, 
                     bool qual, const string &qual_filename)
{
  fas_f = NULL;
  fas_ftype = None;
  qual_f = NULL;
  qual_ftype = None;
  Open(fas_filename, qual, qual_filename);
}

FasReader::FasReader(istream &fas_i)
{
  fas_f = qual_f = NULL;
  fas_ftype = qual_ftype = None;
  Open(fas_i);
}

FasReader::FasReader(istream &fas_i, istream &qual_i)
{
  fas_f = qual_f = NULL;
  fas_ftype = qual_ftype = None;
  Open(fas_i, qual_i);
}

FasReader::~FasReader()
{
  Close();
}

void FasReader::Open(const string &fas_filename, 
                     bool qual, const string &qual_filename)
{
  Close();

  if (!BZ2Extension(fas_filename))
  {
    ifstream *ff = new ifstream;
    ff->open(fas_filename.c_str(), ifstream::in);
    AssertMsg(ff->is_open(), "FasReader: Unable to open file \"" + fas_filename + "\"");
    fas_f = (istream *)ff;
    fas_ftype = File;
  }
  else
  {
    bz2istream *ff = new bz2istream(fas_filename);
    fas_f = (istream *)ff;
    fas_ftype = Bz2File;
  }

  if (qual)
  {
    string qual_fname = qual_filename;
    if (qual_fname.empty())
    {
      qual_fname = Fas2QualFilename(fas_filename);
      AssertMsg(!qual_fname.empty(), "Quality file for \"" + fas_filename + "\" not found");
    }

    if (!BZ2Extension(qual_fname))
    {
      ifstream *ff = new ifstream;
      ff->open(qual_fname.c_str(), ifstream::in);
      AssertMsg(ff->is_open(), "FasReader: Unable to open file \"" + qual_fname + "\"");
      qual_f = (istream *)ff;
      qual_ftype = File;
    }
    else
    {
      bz2istream *ff = new bz2istream(qual_fname);
      qual_f = (istream *)ff;
      qual_ftype = Bz2File;
    }
  }
  else
    Assert(qual_filename.empty());

  SeekNextHeader();
}

void FasReader::Open(istream &fas_i)
{
  Close();

  fas_f = &fas_i;
  fas_ftype = Stream;

  SeekNextHeader();
}

void FasReader::Open(istream &fas_i, istream &qual_i)
{
  Close();

  fas_f = &fas_i;
  fas_ftype = Stream;

  qual_f = &qual_i;
  qual_ftype = Stream;

  SeekNextHeader();
}

void FasReader::Close()
{
  switch (fas_ftype)
  {
  case None:
  case Stream:
    break;

  case File:
    ((ifstream *)fas_f)->close();
    delete fas_f;
    break;

  case Bz2File:
    delete fas_f;
    break;
  }
  fas_ftype = None;

  switch (qual_ftype)
  {
  case None:
  case Stream:
    break;

  case File:
    ((ifstream *)qual_f)->close();
    delete qual_f;
    break;
    
  case Bz2File:
    delete qual_f;
    break;
  }
  qual_ftype = None;
}

void FasReader::Reset()
{
  if (fas_ftype != None)
  {
    fas_f->clear();
    if (fas_ftype == File)
      fas_f->seekg(0, ifstream::beg);
    else if (fas_ftype == Bz2File)
      ((bz2istream *)fas_f)->Reset();
  
    if (qual_ftype != None)
    {
      qual_f->clear();
    if (qual_ftype == File)
      qual_f->seekg(0, ifstream::beg);
    else if (qual_ftype == Bz2File)
      ((bz2istream *)qual_f)->Reset();
    }

    SeekNextHeader();
  }
}


bool FasReader::IsOpen()
{
  return (fas_ftype != None);
}

bool FasReader::HasNext()
{
  bool has_next = (validnextfasline && 
                   (nextfasline.length() >= 1) && (nextfasline[0] == '>'));

  if (!has_next)
  {
    Assert(!(validnextqualline &&
             (nextqualline.length() >= 1) && (nextqualline[0] == '>')));
  }
  return has_next;
}

bool FasReader::HasNextQual()
{
  return (validnextqualline && 
          (nextqualline.length() >= 1) && (nextqualline[0] == '>'));
}

void FasReader::ReadNext()
{
  Assert(HasNext());
  
  header = nextfasline.substr(1, nextfasline.length() - 1);
  seq.clear();

  while (true)
  {
    GetNextFasLine();
    if (!validnextfasline || 
        ((nextfasline.length() >= 1) && (nextfasline[0] == '>')))
      break;

    int i = 0;
    while (i < nextfasline.length())
    {
      while ((i < nextfasline.length()) && !isprint(nextfasline[i]))
        ++i;
      int j = i;
      while ((j < nextfasline.length()) && isprint(nextfasline[j]))
        ++j;

      seq.append(nextfasline, i, j - i);
      i = j;
    }
  }

  if (qual_ftype != None)
  {
    Assert(HasNextQual());
    Assert(header == nextqualline.substr(1, nextqualline.length() - 1));
    qual.clear();

    while (true)
    {
      GetNextQualLine();
      if (!validnextqualline || 
          ((nextqualline.length() >= 1) && (nextqualline[0] == '>')))
        break;
      
      int pos = 0;
      while (pos < nextqualline.length())
      {
        while ((pos < nextqualline.length()) && isspace(nextqualline[pos]))
          ++pos;
        
        if (pos < nextqualline.length())
        {
          int value;
          Assert(StringToInt(nextqualline, value, pos, pos));
          qual += Qual2Char(value);
          Assert((pos == nextqualline.length()) || isspace(nextqualline[pos]));
        }
      }
    }

    Assert(seq.length() == qual.length());
  }
}

void FasReader::GetNextFasLine()
{
  if (fas_f->eof())
  {
    validnextfasline = false;
    return;
  }

  validnextfasline = true;
  getline(*fas_f, nextfasline);
  while (nextfasline.length() > 0)
  {
    if (!isprint(nextfasline[nextfasline.length() - 1]))
      nextfasline.resize(nextfasline.length() - 1);
    else
      break;
  }
}

void FasReader::GetNextQualLine()
{
  if (qual_f->eof())
  {
    validnextqualline = false;
    return;
  }

  validnextqualline = true;
  getline(*qual_f, nextqualline);
  while (nextqualline.length() > 0)
  {
    if (!isprint(nextqualline[nextqualline.length() - 1]))
      nextqualline.resize(nextqualline.length() - 1);
    else
      break;
  }
}

void FasReader::SeekNextHeader()
{
  while (true)
  {
    GetNextFasLine();
    if (!validnextfasline || 
        ((nextfasline.length() >= 1) && (nextfasline[0] == '>')))
      break;
  }

  if (qual_ftype != None)
  {
    while (true)
    {
      GetNextQualLine();
      if (!validnextqualline ||
          ((nextqualline.length() >= 1) && (nextqualline[0] == '>')))
        break;
    }
  }
}

