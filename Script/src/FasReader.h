// FasReader.h: Reads and buffers fasta files

#ifndef FASREADER_H
#define FASREADER_H

#include "System.h"

class FasReader
{
public:
  FasReader();
  FasReader(const string &fas_filename, bool qual = false,
            const string &qual_filename = "");
  FasReader(istream &fas_i);
  FasReader(istream &fas_i, istream &qual_i);
  ~FasReader();

  void Open(const string &fas_filename, bool qual = false,
            const string &qual_filename = "");
  void Open(istream &fas_i);
  void Open(istream &fas_i, istream &qual_i);
  void Close();
  void Reset();
  bool IsOpen();

  bool HasNext();
  void ReadNext();

  string header;
  string seq;
  string qual;

private:
  istream *fas_f, *qual_f;
  enum { None, File, Bz2File, Stream } fas_ftype, qual_ftype;
  bool validnextfasline, validnextqualline;
  string nextfasline, nextqualline;

  bool HasNextQual();
  void GetNextFasLine();
  void GetNextQualLine();
  void SeekNextHeader();
};

#endif
