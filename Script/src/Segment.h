
#ifndef SEGMENT_H
#define SEGMENT_H

#include "System.h"

class Segment
{
private:
  int _left, _right;
public:
  Segment() : _left(0), _right(0) { }
  Segment(int coord1, int coord2, bool leftright = false) 
    : _left(coord1), _right(leftright ? coord2 : (coord1 + coord2)) { }

  int start() const { return _left; }
  int len() const { return _right - _left; }
  int left() const { return _left; }
  int right() const { return _right; }
  void SetLeft(int left_) { _left = left_; }
  void SetRight(int right_) { _right = right_; }
  void SetStart(int start_) 
  { 
    int delta = start_ - _left;
    _right += delta;
    _left = start_;
  }
  void SetLen(int len_) { _right = _left + len_; }
  void Set(int coord1, int coord2, bool leftright = false)
  {
    _left = coord1;
    _right = (leftright ? coord2 : (coord1 + coord2));
  }

  void Translate(int offset)
  {
    _left += offset;
    _right += offset;
  }
  void SetLeftRight(int left_, int right_)
  {
    _left = left_;
    _right = right_;
  }
  void SetStartLen(int start_, int len_)
  {
    _left = start_;
    _right = start_ + len_;
  }

  bool contains(int coord) const 
  { 
    return (_left <= coord) && (coord < _right); 
  }
  
  friend bool operator<(const Segment &a, const Segment &b);
  friend bool operator==(const Segment &a, const Segment &b);
};

bool Intersect(const Segment &a, const Segment &b, Segment &result);
bool Intersects(const Segment &a, const Segment &b);
void Union(const Segment &a, const Segment &b, Segment &result);
bool Union(const Segment &a, const Segment &b, Segment &result, Segment &gap);
bool Contains(const Segment &a, const Segment &b);
bool StrictContains(const Segment &a, const Segment &b);
int Subtract(const Segment &a, const Segment &b, 
             Segment &result, Segment &result2);
int Separation(const Segment &a, const Segment &b);

struct LessSegmentLeft
{
  bool operator()(const Segment &a, const Segment &b) const
  {
    return a.left() < b.left();
  }
};

struct MoreSegmentLeft
{
  bool operator()(const Segment &a, const Segment &b) const
  {
    return a.left() > b.left();
  }
};

struct LessSegmentRight
{
  bool operator()(const Segment &a, const Segment &b) const
  {
    return a.right() < b.right();
  }
};

struct MoreSegmentRight
{
  bool operator()(const Segment &a, const Segment &b) const
  {
    return a.right() > b.right();
  }
};

struct LessSegmentMid
{
  bool operator()(const Segment &a, const Segment &b) const
  {
    return (a.left() + a.right()) < (b.left() + b.right());
  }
};

struct MoreSegmentMid
{
  bool operator()(const Segment &a, const Segment &b) const
  {
    return (a.left() + a.right()) > (b.left() + b.right());
  }
};

struct LessSegmentLen
{
  bool operator()(const Segment &a, const Segment &b) const
  {
    return a.len() < b.len();
  }
};

struct MoreSegmentLen
{
  bool operator()(const Segment &a, const Segment &b) const
  {
    return a.len() > b.len();
  }
};

void GetSegmentPairOverlaps(const vector<Segment> &segments1,
                            const vector<Segment> &segments2,
                            vector< pair<int, int> > &overlaps);

#endif
