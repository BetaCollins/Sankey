
#include "System.h"
#include "Utility.h"
#include "Params.h"
#include "DGNode.h"
#ifdef CAIRO
#include <cairo.h>
#include <cairo-ps.h>
#include <cairo-pdf.h>
#else
#error Must compile with cairo!
#endif

Params params;

string input_file, output_file;
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
  "input",
  "output",
  "phylogeny_structure_file",
};
int num_required_param = sizeof(required_param) / sizeof(required_param[0]);

void InitParams()
{
  params.ExpandImports();
  vector<string> exportedParams = params.Export();
  for(int i = 0; i < exportedParams.size(); i++)
  {
	  cerr << exportedParams[i] << endl;
  }
  params.Require(vector<string>(&required_param[0],
                                &required_param[num_required_param]));

  input_file = params["input"].GetString();
  output_file = params["output"].GetString();
  phylogeny_structure_file = params["phylogeny_structure_file"].GetString();
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




vector<string> phylogeny_levels;
vector< vector<string> > phylogeny_names;
vector<int> phylogeny_size;

void LoadPhylogenyStructure()
{
  phylogeny_levels.push_back("");
  phylogeny_names.push_back(vector<string>());

  cerr << "Phylogeny levels = { ";
  istream *in = InFileStream(phylogeny_structure_file);
  AssertMsg(in, phylogeny_structure_file);
  string line;
  bool include = false;
  while (getline(*in, line))
  {
    if (line.empty())
      continue;
    if (!isspace(line[0]))
    {
      include = false;
      if (line[line.length() - 1] != '*')
      {
        if (phylogeny_levels.size() < 8)
        {
          include = true;
          phylogeny_levels.push_back(line);
          phylogeny_names.push_back(vector<string>());
          cerr << line << " ";
        }
      }
    }
    else
    {
      if (include)
      {
        TrimWhitespace(line);
        phylogeny_names.back().push_back(line);
      }
    }
  }
  delete in;
  cerr << "}" << endl;

  for (int level = 0; level < phylogeny_names.size(); ++level)
  {
    vector<int> sizes;
    for (int i = 0; i < phylogeny_names[level].size(); ++i)
      sizes.push_back(phylogeny_names[level][i].length());

    if (!sizes.empty())
    {
      sort(sizes.begin(), sizes.end());
      phylogeny_size.push_back(sizes[int(0.75 * double(sizes.size()))]);
    }
    else
      phylogeny_size.push_back(1);
  }
}


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


string global_cairo_filename;
cairo_t *global_cairo;
cairo_surface_t *global_cairo_surface;

double cairo_scale_x, cairo_scale_y;

cairo_t *InitCairoPNG(double view_size_x, double view_size_y,
                      const string &filename, 
                      int image_size_x, int image_size_y, 
                      bool keep_aspect = false)
{
  global_cairo_filename = filename;

  double cairo_scale_x = double(image_size_x) / view_size_x;
  double cairo_scale_y = double(image_size_y) / view_size_y;
  if (keep_aspect)
  {
    cairo_scale_x = cairo_scale_y = min(cairo_scale_x, cairo_scale_y);
    image_size_x = int(view_size_x * cairo_scale_x + 0.5);
    image_size_y = int(view_size_y * cairo_scale_y + 0.5);
  }

  global_cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, image_size_x, image_size_y);
  Assert(cairo_surface_status(global_cairo_surface) == CAIRO_STATUS_SUCCESS);

  global_cairo = cairo_create(global_cairo_surface);
  Assert(cairo_status(global_cairo) == CAIRO_STATUS_SUCCESS);

  cairo_set_antialias(global_cairo, CAIRO_ANTIALIAS_GRAY);
  cairo_scale(global_cairo, cairo_scale_x, cairo_scale_y);
  cairo_save(global_cairo);

  return global_cairo;
}

void FinalizeCairoPNG(cairo_t *cairo)
{
  cairo_restore(global_cairo);
  Assert(cairo_surface_write_to_png(global_cairo_surface, global_cairo_filename.c_str()) == CAIRO_STATUS_SUCCESS);

  cairo_destroy(global_cairo);
  cairo_surface_destroy(global_cairo_surface);
}

cairo_t *InitCairoEPS(double view_size_x, double view_size_y,
                      const string &filename, 
                      double image_size_x, double image_size_y, 
                      bool keep_aspect = false)
{
  global_cairo_filename = filename;

  double scale_x = double(image_size_x) / view_size_x;
  double scale_y = double(image_size_y) / view_size_y;
  if (keep_aspect)
  {
    scale_x = scale_y = min(scale_x, scale_y);
    image_size_x = view_size_x * scale_x;
    image_size_y = view_size_y * scale_y;
  }

  global_cairo_surface = cairo_ps_surface_create(global_cairo_filename.c_str(), image_size_x, image_size_y);
  Assert(cairo_surface_status(global_cairo_surface) == CAIRO_STATUS_SUCCESS);

  global_cairo = cairo_create(global_cairo_surface);
  Assert(cairo_status(global_cairo) == CAIRO_STATUS_SUCCESS);

  cairo_set_antialias(global_cairo, CAIRO_ANTIALIAS_GRAY);
  cairo_surface_set_fallback_resolution(global_cairo_surface, 600, 600);
  cairo_scale(global_cairo, scale_x, scale_y);
  cairo_save(global_cairo);

  return global_cairo;
}

void FinalizeCairoEPS(cairo_t *cairo)
{
  cairo_restore(global_cairo);
  cairo_show_page(global_cairo);
  cairo_destroy(global_cairo);
  cairo_surface_destroy(global_cairo_surface);
}


cairo_t *InitCairoPDF(double view_size_x, double view_size_y,
                      const string &filename, 
                      double image_size_x, double image_size_y, 
                      bool keep_aspect = false)
{
  global_cairo_filename = filename;

  double scale_x = double(image_size_x) / view_size_x;
  double scale_y = double(image_size_y) / view_size_y;
  if (keep_aspect)
  {
    scale_x = scale_y = min(scale_x, scale_y);
    image_size_x = view_size_x * scale_x;
    image_size_y = view_size_y * scale_y;
  }

  global_cairo_surface = cairo_pdf_surface_create(global_cairo_filename.c_str(), image_size_x, image_size_y);
  Assert(cairo_surface_status(global_cairo_surface) == CAIRO_STATUS_SUCCESS);

  global_cairo = cairo_create(global_cairo_surface);
  Assert(cairo_status(global_cairo) == CAIRO_STATUS_SUCCESS);

  cairo_set_antialias(global_cairo, CAIRO_ANTIALIAS_GRAY);
  cairo_scale(global_cairo, scale_x, scale_y);
  cairo_save(global_cairo);

  return global_cairo;
}

void FinalizeCairoPDF(cairo_t *cairo)
{
  cairo_restore(global_cairo);
  cairo_show_page(global_cairo);
  cairo_destroy(global_cairo);
  cairo_surface_destroy(global_cairo_surface);
}


struct RenderTextParams
{
  double size;
  double y_offset;
  cairo_font_slant_t slant;
  cairo_font_weight_t weight;

  RenderTextParams() { }

  RenderTextParams(double size_, double y_offset_, 
                   cairo_font_slant_t slant_, cairo_font_weight_t weight_)
    : size(size_), y_offset(y_offset_), slant(slant_), weight(weight_)
  {
  }
};

void RenderTextAddChar(vector<RenderTextParams> &params, vector<string> &texts,
                       double size, double y_offset, cairo_font_slant_t slant,
                       cairo_font_weight_t weight, char c)
{
  if (params.empty() || (size != params.back().size) ||
      (y_offset != params.back().y_offset) ||
      (slant != params.back().slant) || (weight != params.back().weight))
  {
    params.push_back(RenderTextParams(size, y_offset, slant, weight));
    texts.push_back("");
  }
  texts.back() += c;
}

void ParseRenderText(const string &text, 
                     vector<RenderTextParams> &params, vector<string> &texts, 
                     double font_size, double font_height)
{
  double cur_size = font_size;
  double cur_y_offset = 0.0;
  cairo_font_slant_t cur_slant = CAIRO_FONT_SLANT_NORMAL;
  cairo_font_weight_t cur_weight = CAIRO_FONT_WEIGHT_NORMAL;

  for (int i = 0; i < text.length(); ++i)
  {
    if (text[i] != '~')
      RenderTextAddChar(params, texts, cur_size, cur_y_offset, 
                        cur_slant, cur_weight, text[i]);
    else
    {
      Assert(++i < text.length());
      if (text[i] == '~')
      {
        RenderTextAddChar(params, texts, cur_size, cur_y_offset, 
                          cur_slant, cur_weight, '~');
        continue;
      }
      else if (text[i] == '0')
      {
        cur_size = font_size;
        cur_y_offset = 0.0;
        cur_slant = CAIRO_FONT_SLANT_NORMAL;
        cur_weight = CAIRO_FONT_WEIGHT_NORMAL;
      }
      else if (toupper(text[i]) == 'B')
      {
        Assert(++i < text.length());
        switch (text[i])
        {
        case '0':
          cur_weight = CAIRO_FONT_WEIGHT_NORMAL;
          break;
        case '1':
          cur_weight = CAIRO_FONT_WEIGHT_BOLD;
          break;
        default:
          Assert(0);
        }
      }
      else if (toupper(text[i]) == 'I')
      {
        Assert(++i < text.length());
        switch (text[i])
        {
        case '0':
          cur_slant = CAIRO_FONT_SLANT_NORMAL;
          break;
        case '1':
          cur_slant = CAIRO_FONT_SLANT_ITALIC;
          break;
        case '2':
          cur_slant = CAIRO_FONT_SLANT_OBLIQUE;
          break;
        default:
          Assert(0);
        }
      }
      else if (toupper(text[i]) == 'S')
      {
        Assert(++i < text.length());
        switch (text[i])
        {
        case '0':
          cur_size = font_size;
          cur_y_offset = 0.0;
          break;
        case '1':
          cur_size = 0.6 * font_size;
          cur_y_offset = -0.7 * font_height; 
          break;
        case '2':
          cur_size = 0.6 * font_size;
          cur_y_offset = 0.3 * font_height;
          break;
        default:
          Assert(0);
        }
      }
    }
  }
  Assert(params.size() == texts.size());
}

void RenderText(cairo_t *cairo, const string &font_face, double font_size,
                int x_align, int y_align, const string &text)
{
/* ~~ = backspace
   ~b0 = weight normal
   ~b1 = weight bold
   ~i0 = slant normal
   ~i1 = slant italic
   ~i2 = slant oblique
   ~s0 = full script
   ~s1 = super script
   ~s2 = sub script
   ~0 = normal everything
*/
  if (text.empty())
    return;

  cairo_select_font_face(cairo, font_face.c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cairo, font_size);
  cairo_font_extents_t font_extents;
  cairo_font_extents(cairo, &font_extents);

  double font_height = font_extents.ascent - font_extents.descent;

  vector<RenderTextParams> params;
  vector<string> texts;
  ParseRenderText(text, params, texts, font_size, font_height);
  
  double text_left, text_right, text_x_offset = 0.0;
  for (int i = 0; i < params.size(); ++i)
  {
    cairo_select_font_face(cairo, font_face.c_str(), 
                           params[i].slant, params[i].weight);
    cairo_set_font_size(cairo, params[i].size);

    cairo_text_extents_t extents;
    cairo_text_extents(cairo, texts[i].c_str(), &extents);
    
    if (i == 0)
      text_left = extents.x_bearing;
    if (i == (params.size() - 1))
      text_right = text_x_offset + extents.x_bearing + extents.width;

    text_x_offset += extents.x_advance;
  }

  if (x_align > 0)
    cairo_rel_move_to(cairo, -text_left, 0.0);
  else if (x_align < 0)
    cairo_rel_move_to(cairo, -text_right, 0.0);
  else
    cairo_rel_move_to(cairo, -0.5 * (text_left + text_right), 0.0);

  if (y_align > 0)
    cairo_rel_move_to(cairo, 0.0, font_height);
  else if (y_align < 0)
    cairo_rel_move_to(cairo, 0.0, 0.0);
  else
    cairo_rel_move_to(cairo, 0.0, 0.5 * font_height);

  for (int i = 0; i < params.size(); ++i)
  {
    cairo_select_font_face(cairo, font_face.c_str(),
                           params[i].slant, params[i].weight);
    cairo_set_font_size(cairo, params[i].size);

    cairo_rel_move_to(cairo, 0.0, params[i].y_offset);
    cairo_show_text(cairo, texts[i].c_str());
    cairo_rel_move_to(cairo, 0.0, -params[i].y_offset);
  }
}


double RenderTextWidth(cairo_t *cairo, const string &font_face, 
                       double font_size, const string &text)
{
/* ~~ = backspace
   ~b0 = weight normal
   ~b1 = weight bold
   ~i0 = slant normal
   ~i1 = slant italic
   ~i2 = slant oblique
   ~s0 = full script
   ~s1 = super script
   ~s2 = sub script
   ~0 = normal everything
*/
  cairo_select_font_face(cairo, font_face.c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cairo, font_size);
  cairo_font_extents_t font_extents;
  cairo_font_extents(cairo, &font_extents);

  double font_height = font_extents.ascent - font_extents.descent;

  vector<RenderTextParams> params;
  vector<string> texts;
  ParseRenderText(text, params, texts, font_size, font_height);
  if (params.empty())
    return 0.0;

  double text_left, text_right, text_x_offset = 0.0;
  for (int i = 0; i < params.size(); ++i)
  {
    cairo_select_font_face(cairo, font_face.c_str(), 
                           params[i].slant, params[i].weight);
    cairo_set_font_size(cairo, params[i].size);

    cairo_text_extents_t extents;
    cairo_text_extents(cairo, texts[i].c_str(), &extents);
    
    if (i == 0)
      text_left = extents.x_bearing;
    if (i == (params.size() - 1))
      text_right = text_x_offset + extents.x_bearing + extents.width;

    text_x_offset += extents.x_advance;
  }
 
  return (text_right - text_left);
}

double RenderTextHeight(cairo_t *cairo, const string &font_face, 
                       double font_size)
{
/* ~~ = backspace
   ~b0 = weight normal
   ~b1 = weight bold
   ~i0 = slant normal
   ~i1 = slant italic
   ~i2 = slant oblique
   ~s0 = full script
   ~s1 = super script
   ~s2 = sub script
   ~0 = normal everything
*/
  cairo_select_font_face(cairo, font_face.c_str(), CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_font_size(cairo, font_size);
  cairo_font_extents_t font_extents;
  cairo_font_extents(cairo, &font_extents);

  double font_height = font_extents.ascent - font_extents.descent;
  return font_height;
}

#if 1
double view_size_x = 1000;
double view_size_y = (1800.0/2700.0) * view_size_x;
#else
double view_size_x = 2078;
double view_size_y = (1800.0/2700.0) * view_size_x;
#endif




vector< vector<DGNode> > nodes;


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
  static constexpr double graph_branch_sep = 10.0;

  GraphLayout(int num_nodes_, double image_size_x)
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
      if (H1[k] >= -10.0)
        dH -= exp(-Hw*H1[k]) * Hw;
      else
        dH -= exp(-Hw*(-10.0)) * Hw;
      if (H2[k] >= -10.0)
        dH += exp(-Hw*H2[k]) * Hw;
      else
        dH += exp(-Hw*(-10.0)) * Hw;

      dx[k] = Bw*dB + Lw*dL + dG + dH;
    }
  }

  double FollowGradient(vector<double> &x, double step)
  {
    vector<double> gx(x.size());
    gradientF(x, gx);
#if 0
    cerr << "gradient: ";
    for (int i = 0; i < gx.size(); ++i)
      cerr << gx[i] << " ";
    cerr << endl;
#endif

    double mag = 0.0;
    for (int i = 0; i < num_nodes; ++i)
      mag += sqr(gx[i]);
    mag = sqrt(mag);
    Assert(mag > 0.0);

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


double color_r(uint32 color)
{
  return double((color >> 16) & 0xff) / 255.0;
}

double color_g(uint32 color)
{
  return double((color >> 8) & 0xff) / 255.0;
}

double color_b(uint32 color)
{
  return double(color & 0xff) / 255.0;
}

#define color_rgb(color) color_r(color), color_g(color), color_b(color)

double interpolate(double x0, double x1, double s)
{
  return x0 + s * (x1 - x0);
}

#define color_interpolate_rgb(color0, color1, s) interpolate(color_r(color0), color_r(color1), s), interpolate(color_g(color0), color_g(color1), s), interpolate(color_b(color0), color_b(color1), s)


vector<double> level_text_widths;
vector<double> level_x;
vector< vector<int> > node_index;
vector<int> level_nodes;
vector< vector<double> > node_y, node_height, node_width;
vector< vector<double> > node_y_top;
vector< vector<double> > node_y_offset;
vector< vector<double> > node_slope;
vector<uint32> level_color;


/* old drawing code
#if 0
        cairo_move_to(cairo, level_x[level - 1], node_y_top[level - 1][parent] + node_y_offset[level][i]);
        cairo_line_to(cairo, level_x[level] + 1, node_y_top[level][i]);
        cairo_line_to(cairo, level_x[level] + 1, node_y_top[level][i] + node_height[level][i]);
        cairo_line_to(cairo, level_x[level - 1], node_y_top[level - 1][parent] + node_y_offset[level][i] + node_height[level][i]);
#else
#if 0
        cairo_move_to(cairo, level_x[level - 1], node_y_top[level - 1][parent] + node_y_offset[level][i]);
        cairo_curve_to(cairo, level_x[level - 1] + 25.0, node_y_top[level - 1][parent] + node_y_offset[level][i], level_x[level] - 25.0, node_y_top[level][i], level_x[level] + 0.5, node_y_top[level][i]);
        cairo_line_to(cairo, level_x[level] + 0.5, node_y_top[level][i] + node_height[level][i]);
        cairo_curve_to(cairo, level_x[level] - 25.0, node_y_top[level][i] + node_height[level][i], level_x[level - 1] + 25.0, node_y_top[level - 1][parent] + node_y_offset[level][i] + node_height[level][i], level_x[level - 1], node_y_top[level - 1][parent] + node_y_offset[level][i] + node_height[level][i]);
#else
        cairo_move_to(cairo, level_x[level - 1], node_y_top[level - 1][parent] + node_y_offset[level][i]);
        cairo_curve_to(cairo, level_x[level - 1] + 25.0, node_y_top[level - 1][parent] + node_y_offset[level][i] + 25.0 * node_slope[level - 1][parent], level_x[level] - 25.0, node_y_top[level][i] - 25.0 * node_slope[level][i], level_x[level] + 0.5, node_y_top[level][i]);
        cairo_line_to(cairo, level_x[level] + 0.5, node_y_top[level][i] + node_height[level][i]);
        cairo_curve_to(cairo, level_x[level] - 25.0, node_y_top[level][i] + node_height[level][i] - 25.0 * node_slope[level][i], level_x[level - 1] + 25.0, node_y_top[level - 1][parent] + node_y_offset[level][i] + node_height[level][i] + 25.0 * node_slope[level - 1][parent], level_x[level - 1], node_y_top[level - 1][parent] + node_y_offset[level][i] + node_height[level][i]);
#endif
#endif
*/

uint32 WHICH_LEFT = 1;
uint32 WHICH_TOP = 2;
uint32 WHICH_RIGHT = 4;
uint32 WHICH_BOTTOM = 8;
uint32 WHICH_BORDER = 16;
uint32 WHICH_ALL = WHICH_LEFT | WHICH_TOP | WHICH_RIGHT | WHICH_BOTTOM;

void DrawLeftEdge(cairo_t *cairo, int level, int i, uint32 which = WHICH_ALL)
{
  cairo_move_to(cairo, level_x[level], node_y_top[level][i] + node_height[level][i]);
  if (which & WHICH_LEFT)
    cairo_rel_line_to(cairo, 0.0, -node_height[level][i]);
  else
    cairo_rel_move_to(cairo, 0.0, -node_height[level][i]);
}

void DrawLeftEdge(cairo_t *cairo, int level, int i, int child, uint32 which = WHICH_ALL)
{
  int first_child = nodes[level][i].first_child;
  cairo_move_to(cairo, level_x[level], node_y_top[level][i] + node_y_offset[level + 1][first_child + child] + node_height[level + 1][first_child + child]);
  if (which & WHICH_LEFT)
    cairo_rel_line_to(cairo, 0.0, -node_height[level + 1][first_child + child]);
  else
    cairo_rel_move_to(cairo, 0.0, -node_height[level + 1][first_child + child]);
}

void DrawRightEdge(cairo_t *cairo, int level, int i, uint32 which = WHICH_ALL)
{
  if (which & WHICH_RIGHT)
    cairo_rel_line_to(cairo, 0.0, node_height[level][i]);
  else
    cairo_rel_move_to(cairo, 0.0, node_height[level][i]);
}

void DrawCutRightEdge(cairo_t *cairo, int level, int i, int child, uint32 which = WHICH_ALL)
{
  int first_child = nodes[level][i].first_child;
  if ((which & WHICH_RIGHT) && !(which & WHICH_BORDER))
    cairo_line_to(cairo, level_x[level], node_y_top[level][i] + node_y_offset[level + 1][first_child + child] + node_height[level + 1][first_child + child]);
  else
    cairo_move_to(cairo, level_x[level], node_y_top[level][i] + node_y_offset[level + 1][first_child + child] + node_height[level + 1][first_child + child]);
}

void DrawLastRightEdge(cairo_t *cairo, int level, int i, uint32 which = WHICH_ALL)
{
  if ((which & WHICH_RIGHT) && !(which & WHICH_BORDER))
    cairo_line_to(cairo, level_x[level], node_y_top[level][i] + node_height[level][i]);
  else
    cairo_move_to(cairo, level_x[level], node_y_top[level][i] + node_height[level][i]);
}

void DrawTopEdge(cairo_t *cairo, int level, int i, uint32 which = WHICH_ALL)
{
  int parent = nodes[level][i].parent;
  if (which & WHICH_TOP)
    cairo_curve_to(cairo, level_x[level - 1] + 25.0, node_y_top[level - 1][parent] + node_y_offset[level][i] + 25.0 * node_slope[level - 1][parent], level_x[level] - 25.0, node_y_top[level][i] - 25.0 * node_slope[level][i], level_x[level], node_y_top[level][i]);
  else
    cairo_move_to(cairo, level_x[level], node_y_top[level][i]);
}

void DrawBottomEdge(cairo_t *cairo, int level, int i, uint32 which = WHICH_ALL)
{
  int parent = nodes[level][i].parent;
  if (which & WHICH_BOTTOM)
    cairo_curve_to(cairo, level_x[level] - 25.0, node_y_top[level][i] + node_height[level][i] - 25.0 * node_slope[level][i], level_x[level - 1] + 25.0, node_y_top[level - 1][parent] + node_y_offset[level][i] + node_height[level][i] + 25.0 * node_slope[level - 1][parent], level_x[level - 1], node_y_top[level - 1][parent] + node_y_offset[level][i] + node_height[level][i]);
  else
    cairo_move_to(cairo, level_x[level - 1], node_y_top[level - 1][parent] + node_y_offset[level][i] + node_height[level][i]);
}


void RecurseOutline(cairo_t *cairo, int level, int i, uint32 which = WHICH_ALL)
{
  DrawTopEdge(cairo, level, i, which);
  int first_child = nodes[level][i].first_child;
  for (int child = 0; child < nodes[level][i].num_children; ++child)
    if (node_index[level + 1][first_child + child] >= 0)
      RecurseOutline(cairo, level + 1, first_child + child, which);
    else
      DrawCutRightEdge(cairo, level, i, child, which);
  DrawLastRightEdge(cairo, level, i, which);
  DrawBottomEdge(cairo, level, i, which);
}

void TopOutline(cairo_t *cairo, uint32 which = WHICH_ALL)
{
  DrawLeftEdge(cairo, 0, 0, which);
  int first_child = nodes[0][0].first_child;
  for (int child = 0; child < nodes[0][0].num_children; ++child)
    if (node_index[1][first_child + child] >= 0)
      RecurseOutline(cairo, 1, first_child + child, which);
    else
      DrawCutRightEdge(cairo, 0, 0, child, which);
  DrawLastRightEdge(cairo, 0, 0, which);
}

string label_text_font = "Arial";
double label_text_size = 11.0;
string pct_label_text_font = "Arial";
double pct_label_text_size = 7.0;


int Main(vector<string> args)
{
  cerr << "GraphPhylogeny" << endl;
  InitBaseTables();
  
  params.ImportArgs(vector<string>(args.begin() + 1, args.end()));
  InitParams();

  srand(TimerSeconds());
  if (params.Contains("rand_seed"))
    srand(params["rand_seed"].GetInt());


  LoadPhylogenyStructure();

  {
    istream *in = InFileStream(input_file);
    AssertMsg(in, input_file);
    Read(*in, nodes);
    delete in;
  }



  Assert(nodes.size() == 8);
  Assert(phylogeny_levels.size() == 8);
  int levels = 8;


  string output_ext = GetLower(GetExtension(output_file));
  cairo_t *cairo;

  if (output_ext == "png")
  {
#if 1
    cairo = InitCairoPNG(view_size_x, view_size_y, output_file,
                         2079, 1386, true);
#else
    cairo = InitCairoPNG(view_size_x, view_size_y, output_file,
                         2400, 1500, true);
#endif
  }
  else if (output_ext == "eps")
  {
    cairo = InitCairoEPS(view_size_x, view_size_y, output_file,
                         (11.0 - 2.0) * 300.0, (8.5 - 2.0) * 300.0, true);
  }
  else if (output_ext == "pdf")
  {
    cairo = InitCairoPDF(view_size_x, view_size_y, output_file,
                         (11.0 - 2.0) * 300.0, (8.5 - 2.0) * 300.0, true);
  }
  else
  {
    cerr << "Unknown output image file extension: " << output_ext << endl;
    Exit(1);
  }


  cairo_set_source_rgba(cairo, 1.0, 1.0, 1.0, 1.0);
  cairo_rectangle(cairo, 0, 0, view_size_x, view_size_y);
  cairo_fill(cairo);

  level_text_widths.resize(levels);
  double total_text_widths = 0.0;
  for (int i = 0; i < levels; ++i)
  {
    level_text_widths[i] = 8.0 * double(phylogeny_size[i]);
    total_text_widths += level_text_widths[i];
    //cerr << i << "\t" << phylogeny_size[i] << "\t" << total_text_widths << endl;
  }

  level_x.resize(levels);
  {
    //double x = (view_size_x - total_text_widths) / double(levels) * 0.5;
    double x = 5.0;
    for (int i = 0; i < levels; ++i)
    {
      level_x[i] = x + 0.5 * level_text_widths[i];
      //x += level_text_widths[i] + (view_size_x - total_text_widths) / double(levels);
      x += level_text_widths[i] + (view_size_x - total_text_widths) / double(levels - 1);
    }
  }

  cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 1.0);
  for (int i = 0; i < levels; ++i)
  {
    cairo_move_to(cairo, level_x[i], view_size_y - 10.0);
    RenderText(cairo, label_text_font, label_text_size, 0, 0, "~b1" + phylogeny_levels[i]);
  }

  {
    cairo_set_source_rgba(cairo, color_rgb(0x808080), 1.0);
    cairo_set_line_width(cairo, 1.0);
    vector<double> dashes(2);
    dashes[0] = 2.0;
    dashes[1] = 3.0;
    cairo_set_dash(cairo, (const double *)&dashes[0], dashes.size(), 0.0);
    for (int level = 1; level < (levels - 1); ++level)
    {
      double x = 0.5 * (level_x[level] + 0.5 * level_text_widths[level] + level_x[level + 1] - 0.5 * level_text_widths[level + 1]);
      cairo_move_to(cairo, x, 0.0);
      cairo_line_to(cairo, x, view_size_y);
      cairo_stroke(cairo);
    }
    cairo_set_dash(cairo, (const double *)NULL, 0, 0.0);
  }

  double graph_height = 400.0;
  double total_count = nodes[0][0].count;
  double count_threshold = total_count / graph_height;

  node_index.resize(levels);
  level_nodes.resize(levels, 0);
  node_y.resize(levels);
  node_height.resize(levels);
  node_width.resize(levels);
  int num_nodes = 0;
  for (int level = 0; level < levels; ++level)
  {
    node_index[level].resize(nodes[level].size(), -1);
    node_y[level].resize(nodes[level].size(), 0.0);
    node_height[level].resize(nodes[level].size(), 0.0);
    node_width[level].resize(nodes[level].size(), 0.0);

    for (int i = 0; i < nodes[level].size(); ++i)
    {
      node_height[level][i] = nodes[level][i].count / total_count * graph_height;
      node_width[level][i] = RenderTextWidth(cairo, label_text_font, label_text_size, ((level >= 6) ? "~i1" : "~i0") + nodes[level][i].label);
      if (nodes[level][i].count >= count_threshold)
      {
        node_index[level][i] = num_nodes++;
        ++level_nodes[level];
      }
    }
  }
  //cerr << num_nodes << endl;

  {
    GraphLayout layout(num_nodes, view_size_y - 30.0);

    for (int level = 0; level < levels; ++level)
    {
      int last_index = -1;
      for (int i = 0; i < nodes[level].size(); ++i)
      {
        int index = node_index[level][i];
        if (index >= 0)
        {
          layout.w[index] = node_height[level][i];
          layout.t[index] = (label_text_size + pct_label_text_size) * 2.5;
          if (level > 0)
            layout.p[index] = node_index[level - 1][nodes[level][i].parent];
          if (last_index >= 0)
            layout.l[index] = last_index;
          last_index = index++;
        }
      }
    }

    vector<double> x(layout.num_nodes);
    {
      for (int level = 0; level < levels; ++level)
      {
        double level_w = 0.0;
        for (int i = 0; i < nodes[level].size(); ++i)
        {
          int index = node_index[level][i];
          if (index >= 0)
            level_w += max(layout.w[index], layout.t[index]);
        }
        level_w += layout.min_space * (level_nodes[level] - 1);
        
        double pos_w = -0.5 * level_w;
        for (int i = 0; i < nodes[level].size(); ++i)
        {
          int index = node_index[level][i];
          if (index >= 0)
          {
            x[index] = pos_w + 0.5*max(layout.w[index], layout.t[index]);
            //x[index] = 0.0;
            pos_w += max(layout.w[index], layout.t[index]) + layout.min_space;
          }
        }
      }
    }
#if 0
    cerr << "x: ";
    for (int i = 0; i < x.size(); ++i)
      cerr << x[i] << " ";
    cerr << endl;
#endif

    layout.Preprocess();
    cerr << "Laying out graph..." << flush;
    int total_iterations = 20000;
    for (int iteration = 0; iteration < total_iterations; ++iteration)
    {
      double step = double(total_iterations - iteration) / double(total_iterations);
      double mag = layout.FollowGradient(x, -step);
      x[0] = 0.0;
      if ((iteration % 2000) == 0)
        cerr << "." << flush;
#if 0
    cerr << "x: ";
      for (int i = 0; i < x.size(); ++i)
        cerr << x[i] << " ";
      cerr << endl;
#endif
    }
    cerr << endl;

    for (int level = 0; level < levels; ++level)
    {
      for (int i = 0; i < nodes[level].size(); ++i)
      {
        int index = node_index[level][i];
        if (index >= 0)
        {
          node_y[level][i] = 0.5 * (10.0 + view_size_y - 20.0) + x[index];
          //cerr << node_y[level][i] << endl;
        }
      }
    }
  }

  level_color.resize(levels, 0x000000);
  level_color[0] = 0x808080;
  level_color[1] = 0x2020c0;
  level_color[2] = 0x00c0c0;
  level_color[3] = 0x00c000;
  level_color[4] = 0xc0c000;
  level_color[5] = 0xc06000;
  level_color[6] = 0xc00000;
  level_color[7] = 0x8000c0;

  node_y_top.resize(levels);
  for (int level = 0; level < levels; ++level)
  {
    node_y_top[level].resize(nodes[level].size(), 0.0);
    for (int i = 0; i < nodes[level].size(); ++i)
      if (node_index[level][i] >= 0)
        node_y_top[level][i] = node_y[level][i] - 0.5 * node_height[level][i];
  }

  node_y_offset.resize(levels);
  for (int level = 0; level < levels; ++level)
    node_y_offset[level].resize(nodes[level].size(), 0.0);
  for (int level = 0; level < levels; ++level)
  {
//		cerr << ">>>>>>>LEVEL:" << level << endl;
		for (int i = 0; i < nodes[level].size(); ++i)
		{
			double offset = 0.0;
			int first_child = nodes[level][i].first_child;
//			cerr << "NODE[" << level << "," << i << "]" << "::NumChildren" << nodes[level][i].num_children << endl;
			for (int child = 0; child < nodes[level][i].num_children; ++child)
			{
//				cerr << "Child::" << first_child + child << endl;
				node_y_offset[level + 1][first_child + child] = offset;
				offset += node_height[level + 1][first_child + child];
			}
		}
  }

  node_slope.resize(levels);
  for (int level = 0; level < levels; ++level)
  {
    node_slope[level].resize(nodes[level].size(), 0.0);
    if (level == 0)
      continue;
    for (int i = 0; i < nodes[level].size(); ++i)
    {
      int parent = nodes[level][i].parent;
      node_slope[level][i] = (node_y_top[level][i] - (node_y_top[level - 1][parent] + node_y_offset[level][i])) / (level_x[level] - level_x[level - 1]);
    }
  }

  DrawLeftEdge(cairo, 0, 0);
  TopOutline(cairo, WHICH_ALL);
  cairo_pattern_t *pattern = cairo_pattern_create_linear(level_x[0], 0.0, level_x[levels - 1], 0.0);
  for (int level = 0; level < levels; ++level)
    cairo_pattern_add_color_stop_rgba(pattern, (level_x[level] - level_x[0]) / (level_x[levels - 1] - level_x[0]), color_rgb(level_color[level]), 1.0);
  cairo_set_source(cairo, pattern);
  cairo_fill(cairo);
  cairo_pattern_destroy(pattern);

  for (int level = 1; level < levels; ++level)
  {
    for (int i = 0; i < nodes[level].size(); ++i)
      if (node_index[level][i] >= 0)
        if (nodes[level][i].label.empty() || (nodes[level][i].label == "?"))
        {
          int parent = nodes[level][i].parent;
          DrawLeftEdge(cairo, level - 1, parent, i - nodes[level - 1][parent].first_child);
          DrawTopEdge(cairo, level, i);
          DrawRightEdge(cairo, level, i);
          DrawBottomEdge(cairo, level, i);

          cairo_pattern_t *pattern = cairo_pattern_create_linear(level_x[level - 1], 0.0, level_x[level], 0.0);
          cairo_pattern_add_color_stop_rgba(pattern, 0.0, color_rgb(level_color[level - 1]), 1.0);
          if (nodes[level][i].label.empty())
            cairo_pattern_add_color_stop_rgba(pattern, 1.0, color_rgb(0xffffff), 1.0);
          else
            cairo_pattern_add_color_stop_rgba(pattern, 1.0, color_interpolate_rgb(level_color[level - 1], 0x000000, 0.75), 1.0);
          cairo_set_source(cairo, pattern);
          cairo_fill(cairo);
          cairo_pattern_destroy(pattern);
        }
  }

  cairo_new_path(cairo);
  cairo_set_source_rgba(cairo, color_rgb(0x000000), 0.25);
  cairo_set_line_width(cairo, 2.0);
  cairo_set_line_cap(cairo, CAIRO_LINE_CAP_BUTT);
  DrawLeftEdge(cairo, 0, 0);
  TopOutline(cairo, WHICH_BOTTOM | WHICH_RIGHT);
  cairo_stroke(cairo);

  cairo_new_path(cairo);
  cairo_set_source_rgba(cairo, color_rgb(0xffffff), 0.5);
  cairo_set_line_width(cairo, 2.0);
  cairo_set_line_cap(cairo, CAIRO_LINE_CAP_ROUND);
  DrawLeftEdge(cairo, 0, 0);
  TopOutline(cairo, WHICH_TOP | WHICH_LEFT);
  cairo_stroke(cairo);
        
  double text_height = RenderTextHeight(cairo, label_text_font, label_text_size);
  for (int level = 0; level < levels; ++level)
  {
    for (int i = 0; i < nodes[level].size(); ++i)
    {
      if (node_index[level][i] >= 0)
      {
        string label = nodes[level][i].label;
        if (label == "-")
          label = "";
        double width = RenderTextWidth(cairo, label_text_font, label_text_size, ((level >= 6) ? "~i1" : "~i0") + label);
        double height = node_height[level][i];
        //height = text_height;
        double x = level_x[level];
        double y = node_y[level][i];
        double xo = 0.5 * width;
        double xb = xo + 5.0;
        double yo = max(0.5 * height - 5.0, 0.0);
        double yb = 0.5 * height;
        
        if (!nodes[level][i].label.empty() &&
            (nodes[level][i].label != "root") &&
            (nodes[level][i].label != "?"))
        {
#if 0
          cairo_set_source_rgba(cairo, color_rgb(0x000080), 0.5);
          cairo_move_to(cairo, x - xo, y - yb);
          cairo_line_to(cairo, x + xo, y - yb);
          cairo_curve_to(cairo, x + xb, y - yb, x + xb, y - yb, x + xb, y - yo);
          cairo_line_to(cairo, x + xb, y + yo);
          cairo_curve_to(cairo, x + xb, y + yb, x + xb, y + yb, x + xo, y + yb);
          cairo_line_to(cairo, x - xo, y + yb);
          cairo_curve_to(cairo, x - xb, y + yb, x - xb, y + yb, x - xb, y + yo);
          cairo_line_to(cairo, x - xb, y - yo);
          cairo_curve_to(cairo, x - xb, y - yb, x - xb, y - yb, x - xo, y - yb);
          cairo_fill(cairo);
#endif
          
          cairo_set_source_rgba(cairo, color_rgb(0xffffff), 0.4);
          cairo_move_to(cairo, level_x[level] + 0.5, node_y[level][i] + 0.5);
          RenderText(cairo, label_text_font, label_text_size, 0, 0, ((level >= 6) ? "~i1" : "~i0") + label);
          cairo_set_source_rgba(cairo, color_rgb(0xffffff), 0.4);
          cairo_move_to(cairo, level_x[level] + 0.5, node_y[level][i] - 0.5);
          RenderText(cairo, label_text_font, label_text_size, 0, 0, ((level >= 6) ? "~i1" : "~i0") + label);
          cairo_set_source_rgba(cairo, color_rgb(0xffffff), 0.4);
          cairo_move_to(cairo, level_x[level] - 0.5, node_y[level][i] + 0.5);
          RenderText(cairo, label_text_font, label_text_size, 0, 0, ((level >= 6) ? "~i1" : "~i0") + label);
          cairo_set_source_rgba(cairo, color_rgb(0xffffff), 0.4);
          cairo_move_to(cairo, level_x[level] - 0.5, node_y[level][i] - 0.5);
          RenderText(cairo, label_text_font, label_text_size, 0, 0, ((level >= 6) ? "~i1" : "~i0") + label);
          cairo_set_source_rgba(cairo, color_rgb(0x000000), 1.0);
          cairo_move_to(cairo, level_x[level], node_y[level][i]);
          RenderText(cairo, label_text_font, label_text_size, 0, 0, ((level >= 6) ? "~i1" : "~i0") + label);
        }

        string pct_label = FloatToStr(100.0 * nodes[level][i].count / total_count, 2) + "%";
        if (level > 0)
        {
          int x_align = (nodes[level][i].label.empty() || (nodes[level][i].label == "?") || ((nodes[level][i].label == "-") && (level == (levels - 1)))) ? 1 : 0;
          double x_offset = (x_align > 0) ? 2.0 : 0.0;
          double y_offset = ((label == "") || (label == "?")) ? 0.0 : (label_text_size + pct_label_text_size) * 0.4375;
          if (x_align > 0)
            y_offset += x_offset * node_slope[level][i];
          cairo_set_source_rgba(cairo, color_rgb(0xc0c0ff), 0.5);
          cairo_move_to(cairo, level_x[level] + x_offset + 0.5, node_y[level][i] + y_offset + 0.5);
          RenderText(cairo, pct_label_text_font, pct_label_text_size, x_align, 0, pct_label);
          cairo_set_source_rgba(cairo, color_rgb(0x202060), 1.0);
          cairo_move_to(cairo, level_x[level] + x_offset, node_y[level][i] + y_offset);
          RenderText(cairo, pct_label_text_font, pct_label_text_size, x_align, 0, pct_label);
        }
      }
    }
  }

  if (output_ext == "png")
    FinalizeCairoPNG(cairo);
  else if (output_ext == "eps")
    FinalizeCairoEPS(cairo);
  else if (output_ext == "pdf")
    FinalizeCairoPDF(cairo);
  else
    Assert(false);

  return 0;
}
