
#include "System.h"
#include "Utility.h"
#include "Segment.h"
#include "FasReader.h"
#include "Params.h"
#include "DGNode.h"
#ifdef GD
#include <gd.h>
#include <gdfonts.h>
#include <gdfontl.h>
#include <gdfontg.h>
#endif

Params params;

vector<string> database_files;
vector<string> phylogeny_files;
vector<string> reads_files;
string readhits_file;
string readhits_list;
string phylogeny_structure_file;

void ParseFiles(string s, vector<string> &files)
{
  bool list = false;
  if ((s.length() > 1) && (s[0] == '@'))
  {
    s = s.substr(1, s.length() - 1);
    list = true;
  }
  string ext = GetExtension(s);
  ToLower(ext);
  if ((ext == "lst") || (ext == "list"))
    list = true;

  files.clear();
  if (list)
    ReadFileList(s, files);
  else
    files.push_back(s);
}


string required_param[] =
{
  "database",
  "phylogeny",
  "reads",
  "phylogeny_structure_file",
};
int num_required_param = sizeof(required_param) / sizeof(required_param[0]);

void InitParams()
{
  params.ExpandImports();
  params.Require(vector<string>(&required_param[0],
                                &required_param[num_required_param]));

  ParseFiles(params["database"].GetString(), database_files);
  ParseFiles(params["phylogeny"].GetString(), phylogeny_files);
  ParseFiles(params["reads"].GetString(), reads_files);
  if (params.Contains("readhits"))
    readhits_file = params["readhits"].GetString();
  else if (params.Contains("readhits_list"))
    readhits_list = params["readhits_list"].GetString();
  else
  {
    cerr << "Required parameter: readhits OR readhits_list" << endl;
    Exit(0);
  }
  phylogeny_structure_file = params["phylogeny_structure_file"].GetString();
}

vector<string> phylogeny_levels;
deque<int> phylogeny_level;
vector<bool> phylogeny_optional;
int max_level;


struct Contig
{
  string header, seq, qual;
  Contig() { }
  Contig(const string &header_, 
         const string &seq_ = "", const string &qual_ = "")
    : header(header_), seq(seq_), qual(qual_) { }
};

const int database_reserve = 250000;
vector<Contig> database;
map<string, int> database_names;


void LoadDatabase()
{
  database.reserve(database_reserve);

  cerr << "Loading database..." << flush;
  FasReader reader;
  int64 chars_read_dots = 0;
  string name;

  database_names["nil"] = database.size();
  database.push_back(Contig("nil"));

  for (int i = 0; i < database_files.size(); ++i)
  {
    reader.Open(database_files[i]);
    while (reader.HasNext())
    {
      reader.ReadNext();
      chars_read_dots += int64(reader.seq.length());
      while (chars_read_dots >= 10000000)
      {
        cerr << "." << flush;
        chars_read_dots -= 10000000;
      }

      int endpos;
      if (reader.header.find(' ') == string::npos)
        endpos = reader.header.length();
      else
        endpos = reader.header.find(' ');
      name = reader.header.substr(0, endpos);

      database_names[name] = database.size();
      database.push_back(Contig(name, reader.seq));
    }
  }
  cerr << " done." << endl;
}

const string whitespace = " \t\n";

void TrimWhitespace(string &s)
{
  string::size_type begin = s.find_first_not_of(whitespace);
  if (begin == string::npos)
  {
    s.clear();
    return;
  }

  string::size_type end = s.find_last_not_of(whitespace);
  Assert(end != string::npos);
  
  s = s.substr(begin, end - begin + 1);
}

struct StringNoCaseLess
{
  bool operator()(const string &a, const string &b) const
  {
    return (StringCaseCmp(a, b) < 0);
  }
};

class PhyloBranch
{
public:
  PhyloBranch(PhyloBranch *parent_ = NULL, const string *name_ = &empty_name) 
    : parent(parent_), node_hits(0), child_hits(0), name(name_)
  {
    if (!parent)
      depth = 0;
    else
      depth = parent->depth + 1;
  }
  
  ~PhyloBranch()
  {
    for (PhyloBranches::iterator iter = branches.begin(); 
         iter != branches.end(); ++iter)
      delete iter->second;
  }

  bool ChildExists(const string &name) const
  {
    PhyloBranches::const_iterator find = branches.find(name);
    return find != branches.end();
  }

  PhyloBranch *Descend(const string &name, bool create = true)
  {
    PhyloBranches::iterator find = branches.find(name);
    if (find != branches.end())
      return find->second;
    else
    {
      AssertMsg(create, "No child: " + Name() + " => " + name);
      UpdateNameLen(depth + 1, name);
      branches[name] = NULL;
      PhyloBranches::iterator find = branches.find(name);
      return (find->second = new PhyloBranch(this, &find->first));
    }
  }

  PhyloBranch *Ascend() { return parent; }

  template<typename Iterator>
  PhyloBranch *Descend(const Iterator &begin, const Iterator &end, 
                       bool create = true)
  {
    PhyloBranch *current = this;
    for (Iterator iter = begin; iter != end; ++iter)
      current = current->Descend(*iter, create); 
    return current;
  }

  double ComputeHits()
  {
    child_hits = 0;
    for (PhyloBranches::iterator iter = branches.begin();
         iter != branches.end(); ++iter)
      child_hits += iter->second->ComputeHits();

    return node_hits + child_hits;
  }      
 
  void IncNodeHits(double delta_node_hits) { node_hits += delta_node_hits; }
  void SetNodeHits(double node_hits_) { node_hits = node_hits_; }
  double NodeHits() const { return node_hits; }

  double ChildHits() const { return child_hits; }
  double TotalHits() const { return node_hits + child_hits; }

  int MaxDepth() const
  {
    int max_depth = depth;
    for (PhyloBranches::const_iterator iter = branches.begin();
         iter != branches.end(); ++iter)
      max_depth = max(max_depth, iter->second->MaxDepth());
    return max_depth;
  }
  
  int Depth() const { return depth; }

  template <class Func>
  void PreOrder(Func &func)
  {
    if (func(this))
    {
      for (PhyloBranches::iterator iter = branches.begin();
           iter != branches.end(); ++iter)
        iter->second->PreOrder(func);
    }
  }

  template <class Func>
  void PostOrder(Func &func)
  {
    for (PhyloBranches::iterator iter = branches.begin();
         iter != branches.end(); ++iter)
      iter->second->PostOrder(func);

    func(this);
  }

  template <class PreFunc, class PostFunc>
  void PrePostOrder(PreFunc &pre_func, PostFunc &post_func)
  {
    if (pre_func(this))
    {
      for (PhyloBranches::iterator iter = branches.begin();
           iter != branches.end(); ++iter)
        iter->second->PrePostOrder(pre_func, post_func);
    }
    post_func(this);
  }

  const string &Name() const
  {
    return *name;
  }

  string FullName(int max_len = -1) const
  {
    string full_name;
    FullName(full_name, max_len);
    return full_name;
  }

  void FullName(string &full_name, int max_len = -1) const
  {
    if (!parent)
      full_name.clear();
    else
    {
      parent->FullName(full_name, max_len);

      if (!full_name.empty())
        full_name += ' ';
      full_name += "/ ";
      
      const string &name = Name();
      if (max_len <= 0)
      {
        full_name += name;
        if (max_len == 0)
        {
          int pad = max_name_len[depth] - name.length();
          full_name.append(pad, ' ');
        }
      }
      else
      {
        if (max_len > max_name_len[depth])
          max_len = max_name_len[depth];
        if (name.length() < max_len)
        {
          full_name += name;
          int pad = max_len - name.length();
          full_name.append(pad, ' ');
        }
        else
          full_name += name.substr(0, max_len);
      }
    }
  }

  void Names(vector<string> &names, bool optional = false) const
  {
    if (!parent)
      names.clear();
    else
      parent->Names(names, optional);
    if (optional && (phylogeny_level[Depth()] >= 0))
      names.push_back(Name());
  }

  static vector<int> max_name_len;

  int ParentChildEnum() const
  {
    Assert(parent);
    if (Name() == "unknown")
      return -1;
    int count = 0;
    for (PhyloBranches::const_iterator iter = parent->branches.begin();
         iter != parent->branches.end(); ++iter)
    {
      if (iter->second == this)
        return count;
      if (iter->second->Name() != "unknown")
        ++count;
    }
    Assert(false);
  }

  string RootRoute() const
  {
    if (!this->parent)
      return "()";

    string result;
    for (const PhyloBranch *branch = this; branch->parent; branch = branch->parent)
      result = ToStr(branch->ParentChildEnum()) + "," + result;
    return "(" + result.substr(0, result.length() - 1) + ")";
  }

  PhyloBranch *DecodeRoute(const string &s)
  {
    PhyloBranch *branch = this;
    for (int pos = 0; pos < s.length(); ++pos)
    {
      if (!isdigit(s[pos]))
        continue;
      int value, end_pos;
      AssertMsg(StringToInt(s, value, end_pos, pos), s.substr(pos, s.length() - pos));
      PhyloBranches::const_iterator iter = branch->branches.begin();
      while (value > 0)
      {
        ++iter;
        Assert(iter != branch->branches.end());
        --value;
      }
      branch = iter->second;
      pos = end_pos;
    }
    return branch;
  }

private:
  typedef map<string, PhyloBranch *, StringNoCaseLess> PhyloBranches;
  PhyloBranches branches;
  PhyloBranch *parent;

  double node_hits;
  double child_hits;
  int depth;
  const string *name;

  static string empty_name;

  static void UpdateNameLen(int depth, const string &name)
  {
    if (depth >= max_name_len.size())
      max_name_len.resize(depth + 1);
    max_name_len[depth] = max(max_name_len[depth], int(name.length()));
  }
};

string PhyloBranch::empty_name;
vector<int> PhyloBranch::max_name_len;


vector<PhyloBranch *> database_branches;
PhyloBranch *phylogeny;

void LoadPhylogenyFile(const string &file)
{
  istream *in = new ifstream(file.c_str(), ifstream::in);

  string line;
  vector<string> fields;
  while (getline(*in, line))
  {
    SplitTabFields(line, fields);
    PhyloBranch *branch = phylogeny->Descend(fields.begin() + 1, fields.end());

    map<string, int>::iterator find = database_names.find(fields[0]);
    AssertMsg(find != database_names.end(), fields[0]);
    int database_index = find->second;
    database_branches[database_index] = branch;
  }
  
  delete in;
}

const string root_name = "root";

void LoadPhylogeny()
{
  phylogeny = new PhyloBranch(NULL, &root_name);
  database_branches.resize(database.size(), NULL);

  cerr << "Loading phylogeny..." << flush;
  for (int i = 0; i < phylogeny_files.size(); ++i)
    LoadPhylogenyFile(phylogeny_files[i]);
  cerr << " done." << endl;

  phylogeny->ComputeHits();
  cerr << "Phylogeny max depth = " << phylogeny->MaxDepth() << endl;
}

const int reads_reserve = 1000000;
int num_reads;
vector<Contig> reads;
vector<bool> reads_hit;
vector<int> reads_len;
map<string, int> reads_names;

void ScanReads()
{
  reads_len.reserve(reads_reserve);
  reads.reserve(reads_reserve);

  cerr << "Scanning reads..." << flush;
  FasReader reader;
  vector<string> fields;
  for (int i = 0; i < reads_files.size(); ++i)
  {
    reader.Open(reads_files[i], true);
    while (reader.HasNext())
    {
      reader.ReadNext();
      SplitFields(reader.header, fields, " \t,;");
      map<string, int>::iterator rfind = reads_names.find(fields[0]);
      Assert(rfind == reads_names.end());

      string read_name = fields[0];
      reads_names[read_name] = reads.size();
      reads.push_back(Contig(read_name, reader.seq, reader.qual));
      reads_len.push_back(reader.seq.length());
    }
  }
  num_reads = reads.size();
  reads_hit.resize(num_reads, false);
  cerr << " done." << endl;
}


void LoadReadHitsFile(string readhits_file, double weight = 1.0)
{
  cerr << "Loading read hits from \"" << readhits_file << "\" ..." << flush;
  istream *in = InFileStream(readhits_file);
  AssertMsg(in, readhits_file);

  string line;
  vector<string> fields;
  while (getline(*in, line))
  {
    SplitTabFields(line, fields);

    map<string, int>::iterator find = reads_names.find(fields[0]);
    AssertMsg(find != reads_names.end(), fields[0]);
    int reads_index = find->second;

    bool unknown = false;
    while (fields.back() == "unknown")
    {
      unknown = true;
      fields.pop_back();
    }
    PhyloBranch *branch = phylogeny->Descend(fields.begin() + 1, fields.begin() + min(fields.size(), phylogeny_levels.size() + 1), false);
    branch = branch->Descend(fields.begin() + min(fields.size(), phylogeny_levels.size() + 1), fields.end(), true);
    if (unknown)
      branch = branch->Descend("unknown", true);
    while (branch && !branch->Name().empty() && (branch->Name()[0] == '{'))
      branch = branch->Ascend();
    
    branch->IncNodeHits(weight);
    reads_hit[reads_index] = true;
  }

  delete in;
  cerr << " done." << endl;
}


void LoadReadHitsList()
{
  istream *in = InFileStream(readhits_list);
  AssertMsg(in, readhits_list);
  
  string listdir, listfile;
  SplitPath(readhits_list, listdir, listfile);

  string line;
  vector<string> fields;
  while (getline(*in, line))
  {
    SplitFields(line, fields, " \t,;");
    if (fields.empty())
      continue;
    Assert(fields.size() <= 2);
    
    string file;
    RelPath(listdir, fields[0], file);
    double weight = 1.0;
    if (fields.size() == 2)
      StrTo(fields[1], weight);

    LoadReadHitsFile(file, weight);
  }

  delete in;
}

void ProcessHits()
{
  for (int i = 0; i < num_reads; ++i)
  {
#if 0
    Assert(reads_hit[i]);
#else
    if (!reads_hit[i])
    {
      PhyloBranch *branch = phylogeny;
      branch = branch->Descend("unknown", true);
      branch->IncNodeHits(1.0);
    }
#endif
  }
  phylogeny->ComputeHits();
}


struct PhylogenyDepthBranches
{
  int depth;
  vector<PhyloBranch *> &branches;

  PhylogenyDepthBranches(int depth_, vector<PhyloBranch *> &branches_)
    : depth(depth_), branches(branches_) { }

  bool operator()(PhyloBranch *branch)
  {
    Assert(phylogeny_level[branch->Depth()] <= depth);
    if ((phylogeny_level[branch->Depth()] >= 0) && 
        (phylogeny_level[branch->Depth()] <= depth))
      branches.push_back(branch);

    return phylogeny_level[branch->Depth()] < depth;
  }
};

struct SortBranchBases
{
  int depth;
  SortBranchBases(int depth_) : depth(depth_) { }

  bool operator()(const PhyloBranch *a, const PhyloBranch *b)
  {
    double a_hits = (phylogeny_level[a->Depth()] == depth) ? a->TotalHits() : a->NodeHits();
    double b_hits = (phylogeny_level[b->Depth()] == depth) ? b->TotalHits() : b->NodeHits();
    return a_hits > b_hits;
  }
};

string Parenthesize(const string &name, int depth)
{
  string result;
  for (int i = 0; i < depth; ++i)
    result += '(';
  result += name;
  for (int i = 0; i < depth; ++i)
    result += ')';
  return result;
}

#ifdef GD

#if 1

int image_size_x = 2400;
int image_size_y = 1500;

int graph_level_sep = 200;
int graph_width = 600;
int graph_label_height = 20;
int graph_bottom_sep = 900;
int graph_label_stagger = 20;
int graph_label_width_threshold = 100;
int graph_branch_sep = 30;

#else

int image_size_x = 1800;
int image_size_y = 1200;

int graph_level_sep = 160;
int graph_width = 600;
int graph_label_height = 20;
int graph_bottom_sep = 900;
int graph_label_stagger = 20;
int graph_label_width_threshold = 100;
int graph_branch_sep = 30;

#endif

double graph_min_label_width = 0.005;

int rnd(double x)
{
  return int(floor(x + 0.5));
}

int gdDarkenColor(gdImagePtr im, int color, double shade)
{
  int r = rnd(double(gdImageRed(im, color)) * (1.0 - shade));
  int g = rnd(double(gdImageGreen(im, color)) * (1.0 - shade));
  int b = rnd(double(gdImageBlue(im, color)) * (1.0 - shade));
  return gdImageColorAllocate(im, r, g, b);
}

int gdLightenColor(gdImagePtr im, int color, double shade)
{
  int r = 255 - rnd(double(255 - gdImageRed(im, color)) * (1.0 - shade));
  int g = 255 - rnd(double(255 - gdImageGreen(im, color)) * (1.0 - shade));
  int b = 255 - rnd(double(255 - gdImageBlue(im, color)) * (1.0 - shade));
  return gdImageColorAllocate(im, r, g, b);
}


void gdImageFilledQuad(gdImagePtr im, 
                       int x0, int y0, int x1, int y1, 
                       int x2, int y2, int x3, int y3, int color)
{
  gdPoint points[4];
  points[0].x = x0;
  points[0].y = y0;
  points[1].x = x1;
  points[1].y = y1;
  points[2].x = x2;
  points[2].y = y2;
  points[3].x = x3;
  points[3].y = y3;
  gdImageFilledPolygon(im, points, 4, color);
}

void gdVerticalGradientTrapezoid(gdImagePtr im,
                                 int left0, int right0, int left1, int right1,
                                 int y0, int y1, int color0, int color1)
{
  int r0 = gdImageRed(im, color0);
  int g0 = gdImageGreen(im, color0);
  int b0 = gdImageBlue(im, color0);
  int r1 = gdImageRed(im, color1);
  int g1 = gdImageGreen(im, color1);
  int b1 = gdImageBlue(im, color1);
  
  for (int y = y0; true; y += ((y0 <= y1) ? 1 : -1))
  {
    double p = double(y - y0) / double(y1 - y0);
#if 0
    int left = rnd(double(left1 - left0) * p + left0);
    int right = rnd(double(right1 - right0) * p + right0);
    int r = rnd(double(r1 - r0) * p + r0);
    int g = rnd(double(g1 - g0) * p + g0);
    int b = rnd(double(b1 - b0) * p + b0);

    gdImageLine(im, left, y, right, y, gdImageColorAllocate(im, r, g, b));
#else
    double dleft = double(left1 - left0) * p + left0 + 0.5;
    double dright = double(right1 - right0) * p + right0 + 0.5;
    int left = int(ceil(dleft));
    int right = int(floor(dright));
    int r = rnd(double(r1 - r0) * p + r0);
    int g = rnd(double(g1 - g0) * p + g0);
    int b = rnd(double(b1 - b0) * p + b0);
    int col = gdImageColorAllocate(im, r, g, b);
    gdImageLine(im, left, y, right - 1, y, col);
    gdImageSetPixel(im, left - 1, y, gdLightenColor(im, col, 1.0 - (left - dleft)));
    gdImageSetPixel(im, right, y, gdLightenColor(im, col, 1.0 - (dright - right)));
#endif
    if (y == y1)
      break;
  }
}

void LayoutLabel(gdImagePtr im, gdFontPtr font, string s,
                 int width_threshold, vector<string> &lines);

void gdImageCenteredString(gdImagePtr im, gdFontPtr font,
                           int x, int y, const string &s, int color)
{
  gdImageString(im, font, x - (font->w/2) * s.length(), y - font->h/2, (unsigned char *)s.c_str(), color);
}

void gdImageCenteredLayoutString(gdImagePtr im, gdFontPtr font,
                                 int x, int y, const string &s, int color,
                                 int width_threshold)
{
  vector<string> lines;
  LayoutLabel(im, font, s, width_threshold, lines);
  int y_base = y - (lines.size() * font->h) / 2;
  for (int i = 0; i < lines.size(); ++i)
    gdImageCenteredString(im, font, x, y_base + font->h * i, lines[i], color);
}

void gdImageCenteredStringBordered(gdImagePtr im, gdFontPtr font,
                                   int x, int y, const string &s, 
                                   int color, int border_color)
{
  gdImageCenteredString(im, font, x - 1, y, s, border_color);
  gdImageCenteredString(im, font, x, y - 1, s, border_color);
  gdImageCenteredString(im, font, x + 1, y, s, border_color);
  gdImageCenteredString(im, font, x, y + 1, s, border_color);
  gdImageCenteredString(im, font, x, y, s, color);
}

void gdImageCenteredLayoutStringBordered(gdImagePtr im, gdFontPtr font,
                                         int x, int y, const string &s, 
                                         int color, int border_color,
                                         int width_threshold)
{
  vector<string> lines;
  LayoutLabel(im, font, s, width_threshold, lines);
  int y_base = y - (lines.size() * font->h) / 2;
  for (int i = 0; i < lines.size(); ++i)
  {
    gdImageCenteredString(im, font, x - 1, y_base + font->h * i, lines[i], border_color);
    gdImageCenteredString(im, font, x, y_base + font->h * i - 1, lines[i], border_color);
    gdImageCenteredString(im, font, x + 1, y_base + font->h * i, lines[i], border_color);
    gdImageCenteredString(im, font, x, y_base + font->h * i + 1, lines[i], border_color);
    gdImageCenteredString(im, font, x, y_base + font->h * i, lines[i], color);
  }
}

void gdImageCenteredStringShadowed(gdImagePtr im, gdFontPtr font,
                                   int x, int y, const string &s, 
                                   int color)
{
  gdImageCenteredString(im, font, x + 1, y + 1, s, gdDarkenColor(im, color, 0.5));
  gdImageCenteredString(im, font, x + 1, y, s, gdDarkenColor(im, color, 0.25));
  gdImageCenteredString(im, font, x, y, s, color);
}

void gdImageCenteredLayoutStringShadowed(gdImagePtr im, gdFontPtr font,
                                         int x, int y, const string &s, 
                                         int color, int width_threshold)
{
  vector<string> lines;
  LayoutLabel(im, font, s, width_threshold, lines);
  int y_base = y - (lines.size() * font->h) / 2;
  for (int i = 0; i < lines.size(); ++i)
  {
    gdImageCenteredString(im, font, x + 1, y_base + font->h * i + 1, lines[i], gdLightenColor(im, color, 0.5));
    gdImageCenteredString(im, font, x + 1, y_base + font->h * i, lines[i], gdLightenColor(im, color, 0.25));
    gdImageCenteredString(im, font, x, y_base + font->h * i, lines[i], color);
  }
}

struct GraphNode
{
  int index;
  pair<int, int> parent;
  vector< pair<int, int> > children;

  string label;
  double count, cumu_count;
  int x;
  bool polarity;
};

struct CreateGraphNodesPre
{
  vector< vector<GraphNode> > &nodes;
  int max_depth;
  double min_count;
  
  CreateGraphNodesPre(vector< vector<GraphNode> > &nodes_,
                      int max_depth_, double min_count_) 
    : nodes(nodes_), max_depth(max_depth_), min_count(min_count_) { }
  
  bool operator()(PhyloBranch *branch)
  {
    if ((branch->TotalHits() == 0.0) || (branch->TotalHits() < min_count))
      return false;

    int level = phylogeny_level[branch->Depth()];
    if (level >= 0)
    {
      if (level >= nodes.size())
        nodes.resize(level + 1);
      
      nodes[level].push_back(GraphNode());
      nodes[level].back().parent = 
        pair<int, int>(level - 1, 
                       (level > 0) ? (int(nodes[level - 1].size()) - 1) : -1);
      if (level > 0)
        nodes[level - 1].back().children.push_back(pair<int, int>(level, nodes[level].size() - 1));
      
      nodes[level].back().label = branch->Name();
      if (!nodes[level].back().label.empty() && (nodes[level].back().label[0] == '{'))
        nodes[level].back().label = "noname";
      nodes[level].back().count = branch->TotalHits();
    }
    return (level < max_depth);
  }
};

struct CreateGraphNodesPost
{
  vector< vector<GraphNode> > &nodes;
  int max_depth;
  double min_count;
  
  CreateGraphNodesPost(vector< vector<GraphNode> > &nodes_,
                       int max_depth_, double min_count_) 
    : nodes(nodes_), max_depth(max_depth_), min_count(min_count_) { }
  
  void operator()(PhyloBranch *branch)
  {
    int level = phylogeny_level[branch->Depth()];
    if ((level < 0) || (level >= nodes.size()))
      return;

    GraphNode &node = nodes[level].back();
    if (node.children.empty())
      return;

    double children_count = 0;
    for (int i = 0; i < node.children.size(); ++i)
      children_count += nodes[node.children[i].first][node.children[i].second].count;
    
    double remaining_count = branch->TotalHits() - children_count;
    if (remaining_count < min_count)
      return;

    nodes[level + 1].push_back(GraphNode());
    nodes[level + 1].back().parent = 
      pair<int, int>(level, nodes[level].size() - 1);
    node.children.push_back(pair<int, int>(level + 1, nodes[level + 1].size() - 1));

    nodes[level + 1].back().label = "";
    nodes[level + 1].back().count = remaining_count;
  }
};

void DrawGraph(string filename)
{
  gdImagePtr im = gdImageCreateTrueColor(image_size_x, image_size_y);

  int white = gdImageColorAllocate(im, 255, 255, 255);
  int black = gdImageColorAllocate(im, 0, 0, 0);
  int gray = gdImageColorAllocate(im, 128, 128, 128);
  int lightgray = gdImageColorAllocate(im, 192, 192, 192);
  int darkgray = gdImageColorAllocate(im, 64, 64, 64);
  int red = gdImageColorAllocate(im, 255, 0, 0);
  int darkred = gdImageColorAllocate(im, 128, 0, 0);
  int lightred = gdImageColorAllocate(im, 255, 192, 192);
  int green = gdImageColorAllocate(im, 0, 255, 0);
  int darkgreen = gdImageColorAllocate(im, 0, 128, 0);
  int lightgreen = gdImageColorAllocate(im, 192,255, 192);
  int blue = gdImageColorAllocate(im, 0, 0, 255);
  int darkblue = gdImageColorAllocate(im, 0, 0, 128);
  int lightblue = gdImageColorAllocate(im, 192, 192, 255);
  int cyan = gdImageColorAllocate(im, 0, 255, 255);
  int darkcyan = gdImageColorAllocate(im, 0, 128, 128);
  int lightcyan = gdImageColorAllocate(im, 192, 255, 255);
  int purple = gdImageColorAllocate(im, 255, 0, 255);
  int darkpurple = gdImageColorAllocate(im, 128, 0, 128);
  int lightpurple = gdImageColorAllocate(im, 255, 192, 255);
  int yellow = gdImageColorAllocate(im, 255, 255, 0);
  int darkyellow = gdImageColorAllocate(im, 128, 128, 0);
  int lightyellow = gdImageColorAllocate(im, 255, 255, 192);

  gdImageFilledRectangle(im, 0, 0, image_size_x - 1, image_size_y - 1, white);

  gdFontPtr label_font = gdFontGetLarge();

  double total_count = phylogeny->TotalHits();
  vector< vector<GraphNode> > nodes;
  {
    CreateGraphNodesPre run_pre(nodes, max_level - 2, rnd(double(total_count) / double(graph_width)));
    CreateGraphNodesPost run_post(nodes, max_level - 2, rnd(double(total_count) / double(graph_width)));
    phylogeny->PrePostOrder(run_pre, run_post);
  }

  for (int level = 0; level < nodes.size(); ++level)
  {
    for (int branch = 0; branch < nodes[level].size(); ++branch)
    {
      GraphNode &node = nodes[level][branch];
      double cumu_count = 0;
      for (int i = 0; i < node.children.size(); ++i)
      {
        GraphNode &child_node = nodes[node.children[i].first][node.children[i].second];
        child_node.cumu_count = cumu_count;
        cumu_count += child_node.count;
      }
    }
  }

#if 0
  Assert(!nodes.back().empty());
  if (nodes.back().size() == 1)
    nodes.back()[0].x = (image_size_x / 2);
  else
  {
    for (int branch = 0; branch < nodes.back().size(); ++branch)
      nodes.back()[branch].x = (image_size_x / 2) - (graph_bottom_sep / 2) +
        rnd(double(graph_bottom_sep) * double(branch) / double(nodes.back().size() - 1));
  }

  for (int level = nodes.size() - 2; level >= 0; --level)
  {
    for (int branch = 0; branch < nodes[level].size(); ++branch)
    {
      int sum_children_x = 0;
      for (int i = 0; i < nodes[level][branch].children.size(); ++i)
      {
        pair<int, int> child = nodes[level][branch].children[i];
        sum_children_x += nodes[child.first][child.second].x;
      }
      int avg_children_x = rnd(double(sum_children_x) / double(nodes[level][branch].children.size()));

      int parent_x = rnd(0.9 * double(avg_children_x) + 0.1 * double(image_size_x / 2));
    }
  }
#endif

#if 1
  for (int level = 0; level < nodes.size(); ++level)
  {
    for (int branch = 0; branch < nodes[level].size(); ++branch)
      nodes[level][branch].x = rnd(double(image_size_x) / double(nodes[level].size() + 1) * double(branch + 1));
  }


  for (int iteration = 0; iteration < 100; ++iteration)
  {
    double damping = double(100 - iteration) / 1000.0;

    vector< vector<int> > new_x;
    new_x.resize(nodes.size());
    for (int level = 0; level < nodes.size(); ++level)
      new_x[level].resize(nodes[level].size());

    for (int level = 0; level < nodes.size(); ++level)
    {
      for (int branch = 0; branch < nodes[level].size(); ++branch)
      {
        GraphNode &node = nodes[level][branch];
        double force_x = 0;
        
        if (node.parent.first >= 0)
        {
          int parent_x = nodes[node.parent.first][node.parent.second].x;
          force_x += double(parent_x - node.x) * 0.5;
        }
        for (int i = 0; i < node.children.size(); ++i)
        {
          int child_x = nodes[node.children[i].first][node.children[i].second].x;
          force_x += 0.5 / double(node.children.size()) * (child_x - node.x);
        }

        if (branch > 0)
        {
          int left_x = nodes[level][branch - 1].x;
          force_x += 1200.0 / double(node.x - left_x) / double(nodes[level].size() + 1) * (0.3 * double(nodes[level][branch - 1].count + nodes[level][branch].count) / double(total_count) + 0.02) * double(graph_width);
          //force_x += 1000.0 / double(node.x - left_x) / double(nodes[level].size() + 1);
        }
        if (branch < (nodes[level].size() - 1))
        {
          int right_x = nodes[level][branch + 1].x;
          force_x -= 1200.0 / double(right_x - node.x) / double(nodes[level].size() + 1) * (0.3 * double(nodes[level][branch].count + nodes[level][branch + 1].count) / double(total_count) + 0.02) * double(graph_width);
          //force_x -= 30000.0 / double(right_x - node.x) / double(nodes[level].size() + 1);
        }

        if (node.x < (image_size_x / 2))
          force_x += double(min((image_size_x / 2) - node.x, 150)) * 0.05;
        else
          force_x -= double(min(node.x - (image_size_x / 2), 150)) * 0.05;

        new_x[level][branch] = node.x + rnd(force_x * damping);
      }
    }

    for (int level = 0; level < nodes.size(); ++level)
      for (int branch = 0; branch < nodes[level].size(); ++branch)
        nodes[level][branch].x = new_x[level][branch];

    nodes[0][0].x = image_size_x / 2;
    for (int branch = 0; branch < nodes[1].size(); ++branch)
    {
      nodes[1][branch].x = rnd(double(image_size_x - graph_width) / double(nodes[1].size() + 1) * double(branch + 1) + double(graph_width) * double(nodes[1][branch].cumu_count + nodes[1][branch].count / 2) / double(total_count));
    }
  }
#endif

#if 0
  for (int level = 0; level < nodes.size(); ++level)
#endif

#if 0
  for (int level = 0; level < nodes.size(); ++level)
  {
    for (int branch = 0; branch < nodes[level].size(); ++branch)
    {
      GraphNode &node = nodes[level][branch];
      cout << StrFitLeft("(" + ToStr(level) + ", " + ToStr(branch) + ")", 8, ' ');
      cout << StrFitLeft("(" + ToStr(node.parent.first) + ", " + ToStr(node.parent.second) + ")", 8, ' ');
      cout << "\"" << StrFitLeft(node.label, 24, ' ') << "\"  ";
      cout << StrFitLeft(node.count, 8, ' ');
      cout << StrFitLeft(node.x, 8, ' ');
      for (int i = 0; i < node.children.size(); ++i)
      {
        cout << StrFitLeft("(" + ToStr(node.children[i].first) + ", " + ToStr(node.children[i].second) + ")", 8, ' ');
      }
      cout << endl;
    }
  }
#endif

  for (int level = 0; level < nodes.size(); ++level)
  {
    bool polarity = false;
    for (int branch = 0; branch < nodes[level].size(); ++branch)
    {
      nodes[level][branch].polarity = polarity;
      if (!nodes[level][branch].label.empty())
        polarity = !polarity;
    }
  }

  vector<int> level_colors(8);
  level_colors[0] = gdImageColorAllocate(im, 96, 96, 96);
  level_colors[1] = gdImageColorAllocate(im, 224, 96, 96);
  level_colors[2] = gdImageColorAllocate(im, 224, 96, 224);
  level_colors[3] = gdImageColorAllocate(im, 96, 96, 224);
  level_colors[4] = gdImageColorAllocate(im, 96, 224, 224);
  level_colors[5] = gdImageColorAllocate(im, 96, 224, 96);
  level_colors[6] = gdImageColorAllocate(im, 224, 224, 96);
  level_colors[7] = gdImageColorAllocate(im, 224, 224, 224);;

  for (int level = 0; level < nodes.size(); ++level)
  {
    int level_y0 = (image_size_y / 2) - 
      ((nodes.size() - 1) * graph_level_sep / 2) +
      level * graph_level_sep - (graph_label_stagger / 2);
    int level_y1 = level_y0 + graph_label_stagger;

    int parent_level_y0 = (image_size_y / 2) - 
      ((nodes.size() - 1) * graph_level_sep / 2) +
      (level - 1) * graph_level_sep - (graph_label_stagger / 2);
    int parent_level_y1 = parent_level_y0 + graph_label_stagger;

    for (int branch = 0; branch < nodes[level].size(); ++branch)
    {
      GraphNode &node = nodes[level][branch];
      int level_y = node.polarity ? level_y0 : level_y1;
      int left_x = rnd(double(node.x) - 0.5*double(node.count) / double(total_count) * graph_width);
      int right_x = left_x + rnd(double(node.count) / double(total_count) * graph_width);
      
      if (level > 0)
      {
        GraphNode &parent = nodes[node.parent.first][node.parent.second];
        int parent_level_y = parent.polarity ? parent_level_y0 : parent_level_y1;
        int parent_left_x = rnd(double(parent.x) - 0.5*double(parent.count) / double(total_count) * graph_width);
        int parent_right_x = parent_left_x + rnd(double(parent.count) / double(total_count) * graph_width);
        int child_left_x = parent_left_x + rnd(double(node.cumu_count) / double(total_count) * graph_width);
        int child_right_x = parent_left_x + rnd(double(node.cumu_count + node.count) / double(total_count) * graph_width);

        
        gdVerticalGradientTrapezoid(im, child_left_x, child_right_x,
                                    left_x, right_x,
                                    parent_level_y + graph_label_height / 2,
                                    level_y - graph_label_height / 2,
                                    gdDarkenColor(im, level_colors[level - 1], 0.5),
                                    (node.label.empty() ? white : level_colors[level]));
        gdImageCenteredStringBordered(im, label_font,
                                      (child_left_x + child_right_x + left_x + right_x) / 4,
                                      (parent_level_y + graph_label_height / 2 + level_y - graph_label_height / 2) / 2, "(" + FloatToStr(100.0 * double(node.count) / double(total_count), 2) + "%)", 
                                      darkred, darkgray);
      }
      
      if (!node.label.empty())
      {
        gdVerticalGradientTrapezoid(im, left_x, right_x, left_x, right_x,
                                    level_y - graph_label_height / 2,
                                    level_y + graph_label_height / 2,
                                    level_colors[level], gdDarkenColor(im, level_colors[level], 0.5));
      }
    }
  }

  for (int level = 0; level < nodes.size(); ++level)
  {
    int level_y0 = (image_size_y / 2) - 
      ((nodes.size() - 1) * graph_level_sep / 2) +
      level * graph_level_sep - (graph_label_stagger / 2);
    int level_y1 = level_y0 + graph_label_stagger;

    for (int branch = 0; branch < nodes[level].size(); ++branch)
    {
      GraphNode &node = nodes[level][branch];
      int level_y = node.polarity ? level_y0 : level_y1;
#if 1
      gdImageCenteredString(im, label_font, node.x + 1, level_y + 1, 
                            node.label, gray);
      gdImageCenteredString(im, label_font, node.x, level_y + 1, 
                            node.label, darkgray);
      gdImageCenteredString(im, label_font, node.x, level_y, 
                            node.label, black);
#else
      gdImageCenteredStringBordered(im, label_font, node.x, level_y, 
                                    node.label, black, darkgray);
#endif
    }
  }


  FILE *f = fopen(filename.c_str(), "wb");
  gdImagePng(im, f);
  fclose(f);

  gdImageDestroy(im);
}

double sqr(double x)
{
  return x * x;
}

double negsqr(double x)
{
  if (x < 0.0)
    return x * x;
  else
    return 0.0;
}

double dnegsqr(double x)
{
  if (x < 0.0)
    return 2.0 * x;
  else
    return 0.0;
}

struct GraphLayout
{
  int num_nodes;
  vector<double> w, t, b, B, A, L, G1, G2, H1, H2;
  vector<int> p, l, r;
  vector< set<int> > C;
  double Bw, Lw, G1w, G2w, Hw;
  double min_space, min_x, max_x;

  GraphLayout(int num_nodes_)
    : num_nodes(num_nodes_), w(num_nodes), t(num_nodes), b(num_nodes),
      B(num_nodes), A(num_nodes), L(num_nodes), 
      G1(num_nodes), G2(num_nodes), H1(num_nodes), H2(num_nodes),
      p(num_nodes, -1), l(num_nodes, -1), r(num_nodes, -1), C(num_nodes),
      Bw(1.0), Lw(3.0), G1w(1.0), G2w(10.0), Hw(10.0)
  {
    min_space = double(graph_branch_sep);
    min_x = -0.5 * double(image_size_x) + min_space;
    max_x = 0.5 * double(image_size_x) - min_space;
  }

  void Preprocess()
  {
    // initialize w[], t[], p[], l[] beforehand

    for (int i = 0; i < num_nodes; ++i)
    {
      if (l[i] >= 0)
        r[l[i]] = i;
      if (r[i] >= 0)
        l[r[i]] = i;
      if (p[i] >= 0)
        C[p[i]].insert(i);
    }

    for (int i = 0; i < num_nodes; ++i)
    {
      if (!C[i].empty())
      {
        int j = *(C[i].begin());
        while ((l[j] >= 0) && (C[i].find(l[j]) != C[i].end()))
          j = l[j];
        
        double cur_b = -0.5*w[i];
        while ((j >= 0) && (C[i].find(j) != C[i].end()))
        {
          b[j] = cur_b + 0.5*w[j];
          cur_b += w[j];
          j = r[j];
        }
      }
    }
  }

  void computeB(const vector<double> &x)
  {
    for (int i = 0; i < num_nodes; ++i)
    {
      if (p[i] >= 0)
        B[i] = w[i] * sqr(x[i] - (x[p[i]] + b[i]));
      else
        B[i] = 0.0;
    }
  }

  void computeA(const vector<double> &x)
  {
    for (int i = 0; i < num_nodes; ++i)
    {
      if (!C[i].empty())
      {
        A[i] = 0.0;
        for (set<int>::const_iterator iter = C[i].begin(); 
             iter != C[i].end(); ++iter)
        {
          int j = *iter;
          A[i] += w[j] * (x[j] - b[j]);
        }
        A[i] = A[i] / w[i];
      }
    }
  }

  void computeL(const vector<double> &x)
  {
    for (int i = 0; i < num_nodes; ++i)
    {
      if ((p[i] >= 0) && !C[i].empty())
        L[i] = w[i] * sqr(x[i] - 0.5 * (x[p[i]] + A[i]));
      else
        L[i] = 0.0;
    }
  }

  void computeG(const vector<double> &x)
  {
    for (int i = 0; i < num_nodes; ++i)
    {
      if (l[i] >= 0)
      {
        G1[i] = x[i] - x[l[i]] - 0.5*(w[i] + w[l[i]]) - min_space;
        G2[i] = x[i] - x[l[i]] - 0.5*max(w[i] + w[l[i]], t[i] + t[l[i]]) - min_space;
      }
      else
      {
        G1[i] = 0.0;
        G2[i] = 0.0;
      }
    }
  }

  void computeH(const vector<double> &x)
  {
    for (int i = 0; i < num_nodes; ++i)
    {
      double tw = max(w[i], t[i]);
      double mi = min_x + 0.5 * tw;
      double ma = max_x - 0.5 * tw;
      H1[i] = x[i] - mi;
      H2[i] = ma - x[i];
    }
  }

  double computeF(const vector<double> &x)
  {
    computeB(x);
    computeA(x);
    computeL(x);
    computeG(x);
    computeH(x);

    double total = 0.0;
    for (int i = 0; i < num_nodes; ++i)
    {
#if 0
      total += Bw*B[i] + Lw*L[i] + 
        exp(-G1w*G1[i]) + exp(-G2w*G2[i]) + exp(-Hw*H1[i]) + exp(-Hw*H2[i]);
#else
      total += Bw*B[i] + Lw*L[i] +
        exp(-G1w*G1[i]) + negsqr(G2w*G2[i]) + exp(-Hw*H1[i]) + exp(-Hw*H2[i]);
#endif
    }

    return total;
  }

  void gradientF(const vector<double> &x, vector<double> &dx)
  {
    dx.resize(x.size());
    computeA(x);
    computeG(x);
    computeH(x);
    
    for (int k = 0; k < num_nodes; ++k)
    {
      double dB = 0.0;
      for (int i = 0; i < num_nodes; ++i)
      {
        if (p[i] >= 0)
        {
          if (k == i)
            dB += 2.0 * w[i] * (x[i] - (x[p[i]] + b[i]));
          else if (k == p[i])
            dB -= 2.0 * w[i] * (x[i] - (x[p[i]] + b[i]));
        }
      }

      double dL = 0.0;
      for (int i = 0; i < num_nodes; ++i)
      {
        if ((p[i] >= 0) && !C[i].empty())
        {
          if (k == i)
            dL += 2.0 * w[i] * (x[i] - 0.5*(x[p[i]] + A[i]));
          else if (k == p[i])
            dL -= w[i] * (x[i] - 0.5*(x[p[i]] + A[i]));
          else if (C[i].find(k) != C[i].end())
            dL -= w[k] * (x[i] - 0.5*(x[p[i]] + A[i]));
        }
      }

      double dG = 0.0;
      dG -= exp(-G1w*G1[k]) * G1w;
#if 0
      dG -= exp(-G2w*G2[k]) * G2w;
#else
      dG += dnegsqr(G2w*G2[k]) * G2w;
#endif
      if (r[k] >= 0)
      {
        dG += exp(-G1w*G1[r[k]]) * G1w;
#if 0
        dG += exp(-G2w*G2[r[k]]) * G2w;
#else
        dG -= dnegsqr(G2w*G2[r[k]]) * G2w;
#endif
      }

      double dH = 0.0;
      dH -= exp(-Hw*H1[k]) * Hw;
      dH += exp(-Hw*H2[k]) * Hw;

      dx[k] = Bw*dB + Lw*dL + dG + dH;
    }
  }

  double FollowGradient(vector<double> &x, double step)
  {
    vector<double> gx(x.size());
    gradientF(x, gx);

    double mag = 0.0;
    for (int i = 0; i < num_nodes; ++i)
      mag += sqr(gx[i]);
    mag = sqrt(mag);

    double scale = step / mag;

    //cerr << StrFitLeft("x", 16, ' ') << StrFitLeft("gx", 16, ' ') << StrFitLeft("dx", 16, ' ') << endl;
    for (int i = 0; i < num_nodes; ++i)
    {
      x[i] += gx[i] * scale;
      //cerr << StrFitLeft(x[i], 16, ' ') << StrFitLeft(gx[i], 16, ' ') << StrFitLeft(gx[i] * scale, 16, ' ') << endl;
    }

    return mag;
  }
};

int gdStringWidth(gdImagePtr im, gdFontPtr font, const string &s)
{
  return font->w * s.length();
}

void LayoutLabel(gdImagePtr im, gdFontPtr font, string s,
                 int width_threshold, vector<string> &lines)
{
  TrimWhitespace(s);
  lines.clear();
  while (!s.empty())
  {
    string::size_type pos = s.length();
    while (pos != string::npos)
    {
      string ss = s.substr(0, pos);
      TrimWhitespace(ss);
      if (gdStringWidth(im, font, ss) <= width_threshold)
      {
        lines.push_back(ss);
        break;
      }
      else
        pos = s.find_last_of(whitespace, pos - 1);
    }
    if (pos == string::npos)
    {
      pos = s.find_first_of(whitespace);
      if (pos == string::npos)
        pos = s.length();
      string ss = s.substr(0, pos);
      TrimWhitespace(ss);
      lines.push_back(ss);
    }
    s = s.substr(pos, s.length() - pos);
    TrimWhitespace(s);
  }
}

int LayoutLabelWidth(gdImagePtr im, gdFontPtr font, const string &s,
                     int width_threshold)
{
  vector<string> lines;
  LayoutLabel(im, font, s, width_threshold, lines);
  int max_width = 0;
  for (int i = 0; i < lines.size(); ++i)
    max_width = max(max_width, gdStringWidth(im, font, lines[i]));
  return max_width;
}


void DrawGraph2(string filename)
{
  gdImagePtr im = gdImageCreateTrueColor(image_size_x, image_size_y);

  int white = gdImageColorAllocate(im, 255, 255, 255);
  int black = gdImageColorAllocate(im, 0, 0, 0);
  int gray = gdImageColorAllocate(im, 128, 128, 128);
  int lightgray = gdImageColorAllocate(im, 192, 192, 192);
  int darkgray = gdImageColorAllocate(im, 64, 64, 64);
  int red = gdImageColorAllocate(im, 255, 0, 0);
  int darkred = gdImageColorAllocate(im, 128, 0, 0);
  int lightred = gdImageColorAllocate(im, 255, 192, 192);
  int green = gdImageColorAllocate(im, 0, 255, 0);
  int darkgreen = gdImageColorAllocate(im, 0, 128, 0);
  int lightgreen = gdImageColorAllocate(im, 192,255, 192);
  int blue = gdImageColorAllocate(im, 0, 0, 255);
  int darkblue = gdImageColorAllocate(im, 0, 0, 128);
  int lightblue = gdImageColorAllocate(im, 192, 192, 255);
  int cyan = gdImageColorAllocate(im, 0, 255, 255);
  int darkcyan = gdImageColorAllocate(im, 0, 128, 128);
  int lightcyan = gdImageColorAllocate(im, 192, 255, 255);
  int purple = gdImageColorAllocate(im, 255, 0, 255);
  int darkpurple = gdImageColorAllocate(im, 128, 0, 128);
  int lightpurple = gdImageColorAllocate(im, 255, 192, 255);
  int yellow = gdImageColorAllocate(im, 255, 255, 0);
  int darkyellow = gdImageColorAllocate(im, 128, 128, 0);
  int lightyellow = gdImageColorAllocate(im, 255, 255, 192);

  gdImageFilledRectangle(im, 0, 0, image_size_x - 1, image_size_y - 1, white);

  gdFontPtr label_font = gdFontGetLarge();

  double total_count = phylogeny->TotalHits();
  vector< vector<GraphNode> > nodes;
  {
    CreateGraphNodesPre run_pre(nodes, max_level - 2, rnd(double(total_count) / double(graph_width)));
    CreateGraphNodesPost run_post(nodes, max_level - 2, rnd(double(total_count) / double(graph_width)));
    phylogeny->PrePostOrder(run_pre, run_post);
  }

  for (int level = 0; level < nodes.size(); ++level)
  {
    for (int branch = 0; branch < nodes[level].size(); ++branch)
    {
      GraphNode &node = nodes[level][branch];
      double cumu_count = 0;
      for (int i = 0; i < node.children.size(); ++i)
      {
        GraphNode &child_node = nodes[node.children[i].first][node.children[i].second];
        child_node.cumu_count = cumu_count;
        cumu_count += child_node.count;
      }
    }
  }

  for (int level = 0; level < nodes.size(); ++level)
  {
    bool polarity = false;
    for (int branch = 0; branch < nodes[level].size(); ++branch)
    {
      nodes[level][branch].polarity = polarity;
      if (!nodes[level][branch].label.empty() && 
          ((double(nodes[level][branch].count) / double(total_count)) >= graph_min_label_width))
        polarity = !polarity;
    }
  }

  int num_nodes;
  {
    int index = 0;
    for (int level = 0; level < nodes.size(); ++level)
      for (int branch = 0; branch < nodes[level].size(); ++branch)
        nodes[level][branch].index = index++;
    num_nodes = index;
  }

  GraphLayout layout(num_nodes);
  {
    int index = 0;
    for (int level = 0; level < nodes.size(); ++level)
    {
      for (int branch = 0; branch < nodes[level].size(); ++branch)
      {
        GraphNode &node = nodes[level][branch];
        layout.w[index] = double(graph_width) * 
          double(node.count) / double(total_count);
        layout.t[index] = LayoutLabelWidth(im, label_font, node.label,
                                           graph_label_width_threshold);
        if (level > 0)
          layout.p[index] = nodes[node.parent.first][node.parent.second].index;
        if (branch > 0)
          layout.l[node.index] = nodes[level][branch - 1].index;
        ++index;
      }
    }
  }

  vector<double> x(layout.num_nodes);
  {
    for (int level = 0; level < nodes.size(); ++level)
    {
      double level_w = 0.0;
      for (int branch = 0; branch < nodes[level].size(); ++branch)
      {
        int index = nodes[level][branch].index;
        level_w += max(layout.w[index], layout.t[index]);
      }
      level_w += layout.min_space * (nodes[level].size() - 1);

      double pos_w = -0.5 * level_w;
      for (int branch = 0; branch < nodes[level].size(); ++branch)
      {
        int index = nodes[level][branch].index;
        x[index] = pos_w + 0.5*max(layout.w[index], layout.t[index]);
        x[index] = 0.0;
        pos_w += max(layout.w[index], layout.t[index]) + layout.min_space;
      }
    }
  }

  layout.Preprocess();

#if 1
  cerr << "Laying out graph..." << flush;
  for (int iteration = 0; iteration < 100000; ++iteration)
  {
    double step = double(100000 - iteration) / 100000.0;
    double mag = layout.FollowGradient(x, -step);
    //cerr << StrFitLeft(layout.computeF(x), 16, ' ') << StrFitLeft(mag, 16, ' ') << endl;
    x[0] = 0.0;
    if ((iteration % 10000) == 0)
      cerr << "." << flush;
  }
  cerr << endl;

  {
    int index = 0;
    for (int level = 0; level < nodes.size(); ++level)
    {
      for (int branch = 0; branch < nodes[level].size(); ++branch)
      {
        nodes[level][branch].x = rnd(x[index] + 0.5*double(image_size_x));
        ++index;
      }
    }
  }
#endif

#if 0
  for (int level = 0; level < nodes.size(); ++level)
  {
    for (int branch = 0; branch < nodes[level].size(); ++branch)
    {
      GraphNode &node = nodes[level][branch];
      cerr << StrFitLeft("(" + ToStr(level) + ", " + ToStr(branch) + ")", 8, ' ');
      cerr << StrFitLeft("(" + ToStr(node.parent.first) + ", " + ToStr(node.parent.second) + ")", 8, ' ');
      cerr << "\"" << StrFitLeft(node.label, 24, ' ') << "\"  ";
      //cerr << StrFitLeft(node.count, 8, ' ');
      //cerr << StrFitLeft(node.x, 8, ' ');
#if 0
      for (int i = 0; i < node.children.size(); ++i)
      {
        cerr << StrFitLeft("(" + ToStr(node.children[i].first) + ", " + ToStr(node.children[i].second) + ")", 8, ' ');
      }
#endif

      cerr << StrFitLeft(layout.p[node.index], 4, ' ') << StrFitLeft(layout.l[node.index], 4, ' ') << StrFitLeft(layout.r[node.index], 4, ' ');
      cerr << StrFitLeft(layout.w[node.index], 10, ' ') << StrFitLeft(layout.b[node.index], 10, ' ');

      cerr << endl;
    }
  }
#endif

  vector<int> level_colors(8);
  level_colors[0] = gdImageColorAllocate(im, 96, 96, 96);
  level_colors[1] = gdImageColorAllocate(im, 224, 96, 96);
  level_colors[2] = gdImageColorAllocate(im, 224, 96, 224);
  level_colors[3] = gdImageColorAllocate(im, 96, 96, 224);
  level_colors[4] = gdImageColorAllocate(im, 96, 224, 224);
  level_colors[5] = gdImageColorAllocate(im, 96, 224, 96);
  level_colors[6] = gdImageColorAllocate(im, 224, 224, 96);
  level_colors[7] = gdImageColorAllocate(im, 224, 224, 224);;

  for (int level = 0; level < nodes.size(); ++level)
  {
    int level_y0 = (image_size_y / 2) - 
      ((nodes.size() - 1) * graph_level_sep / 2) +
      level * graph_level_sep - (graph_label_stagger / 2);
    int level_y1 = level_y0 + graph_label_stagger;

    int parent_level_y0 = (image_size_y / 2) - 
      ((nodes.size() - 1) * graph_level_sep / 2) +
      (level - 1) * graph_level_sep - (graph_label_stagger / 2);
    int parent_level_y1 = parent_level_y0 + graph_label_stagger;

    for (int branch = nodes[level].size() - 1; branch >= 0; --branch)
    {
      GraphNode &node = nodes[level][branch];
      int level_y = node.polarity ? level_y0 : level_y1;
      int left_x = rnd(double(node.x) - 0.5*double(node.count) / double(total_count) * graph_width);
      int right_x = left_x + rnd(double(node.count) / double(total_count) * graph_width);
      
      if (level > 0)
      {
        GraphNode &parent = nodes[node.parent.first][node.parent.second];
        int parent_level_y = parent.polarity ? parent_level_y0 : parent_level_y1;
        int parent_left_x = rnd(double(parent.x) - 0.5*double(parent.count) / double(total_count) * graph_width);
        int parent_right_x = parent_left_x + rnd(double(parent.count) / double(total_count) * graph_width);
        int child_left_x = parent_left_x + rnd(double(node.cumu_count) / double(total_count) * graph_width);
        int child_right_x = parent_left_x + rnd(double(node.cumu_count + node.count) / double(total_count) * graph_width);

        
        gdVerticalGradientTrapezoid(im, child_left_x, child_right_x,
                                    left_x, right_x,
                                    parent_level_y + graph_label_height / 2,
                                    level_y - graph_label_height / 2,
                                    gdDarkenColor(im, level_colors[level - 1], 0.5),
                                    (node.label.empty() ? white : level_colors[level]));
      }
      
      if (!node.label.empty())
      {
        gdVerticalGradientTrapezoid(im, left_x, right_x, left_x, right_x,
                                    level_y - graph_label_height / 2,
                                    level_y + graph_label_height / 2,
                                    level_colors[level], gdDarkenColor(im, level_colors[level], 0.5));
      }
    }
  }

  for (int level = 0; level < nodes.size(); ++level)
  {
    int level_y0 = (image_size_y / 2) - 
      ((nodes.size() - 1) * graph_level_sep / 2) +
      level * graph_level_sep - (graph_label_stagger / 2);
    int level_y1 = level_y0 + graph_label_stagger;

    int parent_level_y0 = (image_size_y / 2) - 
      ((nodes.size() - 1) * graph_level_sep / 2) +
      (level - 1) * graph_level_sep - (graph_label_stagger / 2);
    int parent_level_y1 = parent_level_y0 + graph_label_stagger;

    for (int branch = 0; branch < nodes[level].size(); ++branch)
    {
      GraphNode &node = nodes[level][branch];
      if ((double(node.count) / double(total_count)) < graph_min_label_width)
        continue;

      int level_y = node.polarity ? level_y0 : level_y1;
      int left_x = rnd(double(node.x) - 0.5*double(node.count) / double(total_count) * graph_width);
      int right_x = left_x + rnd(double(node.count) / double(total_count) * graph_width);
      
      if (level > 0)
      {
        GraphNode &parent = nodes[node.parent.first][node.parent.second];
        int parent_level_y = parent.polarity ? parent_level_y0 : parent_level_y1;
        int parent_left_x = rnd(double(parent.x) - 0.5*double(parent.count) / double(total_count) * graph_width);
        int parent_right_x = parent_left_x + rnd(double(parent.count) / double(total_count) * graph_width);
        int child_left_x = parent_left_x + rnd(double(node.cumu_count) / double(total_count) * graph_width);
        int child_right_x = parent_left_x + rnd(double(node.cumu_count + node.count) / double(total_count) * graph_width);

        
        gdImageCenteredStringBordered(im, label_font,
                                      rnd(0.1*(child_left_x + child_right_x) + 0.4*(left_x + right_x)),
                                      rnd(0.3*(parent_level_y + graph_label_height / 2) + 0.7*(level_y - graph_label_height / 2)), "(" + FloatToStr(100.0 * double(node.count) / double(total_count), 2) + "%)", 
                                      darkred, darkgray);
      }
    }
  }

  for (int level = 0; level < nodes.size(); ++level)
  {
    int level_y0 = (image_size_y / 2) - 
      ((nodes.size() - 1) * graph_level_sep / 2) +
      level * graph_level_sep - (graph_label_stagger / 2);
    int level_y1 = level_y0 + graph_label_stagger;

    for (int branch = 0; branch < nodes[level].size(); ++branch)
    {
      GraphNode &node = nodes[level][branch];

      if ((double(node.count) / double(total_count)) < graph_min_label_width)
        continue;
      if (node.label == "noname")
        continue;

      int level_y = node.polarity ? level_y0 : level_y1;
      gdImageCenteredLayoutStringShadowed(im, label_font, node.x, level_y, 
                                          node.label, black, 
                                          graph_label_width_threshold);
    }
  }


  FILE *f = fopen(filename.c_str(), "wb");
  gdImagePng(im, f);
  fclose(f);

  gdImageDestroy(im);
}

#endif

void DGOrderNodes(vector< pair<int, string> > &nodes)
{
  vector< pair<int, string> > temp_nodes;
  for (int i = 0; i < nodes.size(); ++i)
  {
    if ((nodes[i].second != "unknown") && (nodes[i].second != "noname") &&
        !nodes[i].second.empty())
      temp_nodes.push_back(nodes[i]);
  }
  
  sort(temp_nodes.begin(), temp_nodes.end(), SecondLess<int, string>());

  for (int i = 0; i < nodes.size(); ++i)
    if (nodes[i].second == "noname")
      temp_nodes.push_back(pair<int, string>(nodes[i].first, "-"));

  for (int i = 0; i < nodes.size(); ++i)
    if (nodes[i].second == "unknown")
      temp_nodes.push_back(pair<int, string>(nodes[i].first, "?"));

  for (int i = 0; i < nodes.size(); ++i)
    if (nodes[i].second.empty())
      temp_nodes.push_back(nodes[i]);

  nodes.swap(temp_nodes);
}

//struct DGNode
//{
//  int parent;
//  int first_child, num_children;
//
//  string label;
//  double count;
//};
//
//template <>
//void Write<DGNode>(ostream &out, const DGNode &x)
//{
//  Write(out, x.label);
//  WriteBin(out, x.count);
//  WriteBin(out, x.parent);
//  WriteBin(out, x.first_child);
//  WriteBin(out, x.num_children);
//}
//
//template <>
//void Read<DGNode>(istream &in, DGNode &x)
//{
//  Read(in, x.label);
//  ReadBin(in, x.count);
//  ReadBin(in, x.parent);
//  ReadBin(in, x.first_child);
//  ReadBin(in, x.num_children);
//}

void DumpGraph(string filename)
{
  double total_count = phylogeny->TotalHits();
  vector< vector<GraphNode> > nodes;
  {
    CreateGraphNodesPre run_pre(nodes, max_level - 2, 0.0);
    CreateGraphNodesPost run_post(nodes, max_level - 2, 0.0);
    phylogeny->PrePostOrder(run_pre, run_post);
  }

  vector< pair<int, string> > node_order;
  {
    vector< pair<int, string> > top_nodes;
    for (int branch = 0; branch < nodes[0].size(); ++branch)
      top_nodes.push_back(pair<int, string>(branch, nodes[0][branch].label));
    DGOrderNodes(top_nodes);
    node_order.insert(node_order.end(), top_nodes.begin(), top_nodes.end());
  }

  vector< vector<DGNode> > dgnodes;
  dgnodes.resize(nodes.size());
  for (int level = 0; level < nodes.size(); ++level)
  {
    vector< pair<int, string> > next_node_order;
    for (int order = 0; order < node_order.size(); ++order)
    {
      int branch = node_order[order].first;
      GraphNode &node = nodes[level][branch];

      dgnodes[level].push_back(DGNode());
      dgnodes[level].back().parent = -1;
      dgnodes[level].back().label = node_order[order].second;
      dgnodes[level].back().count = node.count;
      
      vector< pair<int, string> > temp_nodes;
      for (int i = 0; i < node.children.size(); ++i)
      {
        GraphNode &child_node = nodes[node.children[i].first][node.children[i].second];
        temp_nodes.push_back(pair<int, string>(node.children[i].second, child_node.label));
      }
      DGOrderNodes(temp_nodes);
      dgnodes[level].back().first_child = next_node_order.size();
      dgnodes[level].back().num_children = temp_nodes.size();
      next_node_order.insert(next_node_order.end(), temp_nodes.begin(), temp_nodes.end());
    }
    node_order.swap(next_node_order);
  }

#if 0
  dgnodes.insert(dgnodes.begin(), vector<DGNode>());
  dgnodes[0].push_back(DGNode());
  dgnodes[0][0].parent = -1;
  dgnodes[0][0].first_child = 0;
  dgnodes[0][0].num_children = dgnodes[1].size();
  dgnodes[0][0].label = "root";
  dgnodes[0][0].count = total_count;
#endif

  for (int level = 0; level < dgnodes.size(); ++level)
  {
    for (int branch = 0; branch < dgnodes[level].size(); ++branch)
    {
      int first_child = dgnodes[level][branch].first_child;
      for (int child = 0; child < dgnodes[level][branch].num_children; ++child)
        dgnodes[level + 1][first_child + child].parent = branch;
    }
  }

  ostream *out = OutFileStream(filename);
  AssertMsg(out, filename);
  Write(*out, dgnodes);
  delete out;
}

int Main(vector<string> args)
{
  cerr << "PhyloStats" << endl;
  InitBaseTables();
  
  params.ImportArgs(vector<string>(args.begin() + 1, args.end()));
  InitParams();

  srand(TimerSeconds());
  if (params.Contains("rand_seed"))
    srand(params["rand_seed"].GetInt());

  cerr << "Phylogeny levels = { ";
  {
    istream *in = InFileStream(phylogeny_structure_file);
    AssertMsg(in, phylogeny_structure_file);
    string line;
    int level = 1;
    while (getline(*in, line))
    {
      if (line.empty())
        continue;
      if (!isspace(line[0]))
      {
        bool optional = false;
        if (line[line.length() - 1] == '*')
        {
          optional = true;
          line = line.substr(0, line.length() - 1);
        }
        phylogeny_levels.push_back(line);
        phylogeny_optional.push_back(optional);
        phylogeny_level.push_back(optional ? -1 : level++);
        cerr << (optional ? "(" : "") << line << (optional ? ")" : "") << " ";
      }
    }
    delete in;
    cerr << "}" << endl;
    phylogeny_level.push_front(0);
    phylogeny_level.push_back(-1);
    max_level = level - 1;
  }

  LoadDatabase();
  LoadPhylogeny();
  ScanReads();
  if (!readhits_file.empty())
    LoadReadHitsFile(readhits_file, 1.0);
  else
    LoadReadHitsList();
  ProcessHits();

  bool tabify = params.Contains("_tabify");
  bool rootroute = params.Contains("_rootroute");
  
  int max_depth = max_level - 2;
  double total_hits = phylogeny->TotalHits();
  vector<int> column_widths(max_depth + 1, 20);
  for (int depth = 0; depth <= 6; ++depth)
    column_widths[depth] = 20;
  for (int depth = 7; depth < column_widths.size(); ++depth)
    column_widths[depth] = 40;
  for (int depth = 1; depth <= max_depth; ++depth)
  {
    vector<PhyloBranch *> branches;
    PhylogenyDepthBranches extract(depth, branches);
    phylogeny->PreOrder(extract);
    sort(branches.begin(), branches.end(), SortBranchBases(depth));

    int ll = -1;
    for (int i = 0; i < phylogeny_level.size(); ++i)
      if (depth == phylogeny_level[i])
        ll = i;
    Assert(ll >= 0);
    cout << endl << "DEPTH " << depth << " " << phylogeny_levels[ll - 1] << endl;
    double shown_hits = 0;
    double identified_hits = 0;
    double unknown_hits = 0;
    for (int i = 0; i < branches.size(); ++i)
    {
      //if (i == 100)
      //  break;
      double hits = (phylogeny_level[branches[i]->Depth()] == depth) ? branches[i]->TotalHits() : branches[i]->NodeHits();
      if (hits == 0)
        break;

      vector<string> names;
      branches[i]->Names(names, true);
      if (!tabify)
      {
        for (int j = 1; j < names.size(); ++j)
          cout << StrFitLeft(names[j], column_widths[j] - 1, ' ') << ' ';
        for (int j = names.size(); j <= depth; ++j)
          cout << StrFitLeft(Parenthesize(names.back(), j - names.size() + 1), column_widths[j] - 1, ' ') << ' ';
        
        cout << StrFitRight(FloatToStr(100.0 * double(hits) / double(total_hits), 5), 12, ' ') << "%" << StrFitRight(hits, 10, ' ');
        if (rootroute)
          cout << "  " << StrFitRight(branches[i]->Depth(), 2, ' ') << "  " << branches[i]->RootRoute();
        cout << "\n";
      }
      else
      {
        cout << (100.0 * double(hits) / double(total_hits)) << "\t" << hits;
        if (rootroute)
          cout << "\t" << branches[i]->Depth() << "\t" << branches[i]->RootRoute();
        for (int j = 1; j < names.size(); ++j)
          cout << "\t" << names[j];
        for (int j = names.size(); j <= depth; ++j)
          cout << "\tother";
        cout << "\n";
      }

      shown_hits += hits;
      if ((names.size() > depth) && (names.back()[0] != '{') && (names.back() != "unknown"))
        identified_hits += hits;
      if (names.back() == "unknown")
        unknown_hits += hits;
    }

    if (!tabify)
    {
      cout << StrFitLeft("other", column_widths[1], ' ');
      for (int i = 2; i <= depth; ++i)
        cout << StrFitLeft("", column_widths[i], ' ');
      cout << StrFitRight(FloatToStr(100.0 * double(total_hits - shown_hits) / double(total_hits), 5), 12, ' ') << "%" << StrFitRight(total_hits - shown_hits, 10, ' ') << "\n";

      cout << StrFitLeft("unidentified", column_widths[1], ' ');
      for (int i = 2; i <= depth; ++i)
        cout << StrFitLeft("", column_widths[i], ' ');
      cout << StrFitRight(FloatToStr(100.0 * double(total_hits - identified_hits) / double(total_hits), 5), 12, ' ') << "%" << StrFitRight(total_hits - identified_hits, 10, ' ') << "\n";

      cout << StrFitLeft("unknown", column_widths[1], ' ');
      for (int i = 2; i <= depth; ++i)
        cout << StrFitLeft("", column_widths[i], ' ');
      cout << StrFitRight(FloatToStr(100.0 * double(unknown_hits) / double(total_hits), 5), 12, ' ') << "%" << StrFitRight(unknown_hits, 10, ' ') << "\n";
      cout << endl;
    }
    else
    {
      cout << (100.0 * double(total_hits - shown_hits) / double(total_hits)) << "\t" << (total_hits - shown_hits);
      for (int i = 1; i <= depth; ++i)
        cout << "\tother";
      cout << "\n";

      cout << (100.0 * double(total_hits - identified_hits) / double(total_hits)) << "\t" << (total_hits - identified_hits);
      for (int i = 1; i <= depth; ++i)
        cout << "\tunidentified";
      cout << "\n";

      cout << (100.0 * double(unknown_hits) / double(total_hits)) << "\t" << (unknown_hits);
      for (int i = 1; i <= depth; ++i)
        cout << "\tunknown";
      cout << "\n";
    }
  }

#ifdef GD
  if (params.Contains("_graph"))
    DrawGraph2(params["_graph"].GetString());
#endif

  if (params.Contains("_dump_graph"))
    DumpGraph(params["_dump_graph"].GetString());

  return 0;
}
