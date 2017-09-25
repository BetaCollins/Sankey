
#include "System.h"
#include "Segment.h"

bool operator<(const Segment &a, const Segment &b)
{ 
  return a._left < b._left;
}

bool operator==(const Segment &a, const Segment &b)
{
  return (a._left == b._left) && (a._right == b._right);
}

bool operator!=(const Segment &a, const Segment &b)
{
  return !(a == b);
}

bool Intersect(const Segment &a, const Segment &b, Segment &result)
{
  if ((a.right() > b.left()) && (a.left() < b.right()))
  {
    result.SetLeft(max(a.left(), b.left()));
    result.SetRight(min(a.right(), b.right()));
    return true;
  }
  else
    return false;
}

bool Intersects(const Segment &a, const Segment &b)
{
  return (a.right() > b.left()) && (a.left() < b.right());
}
  

void Union(const Segment &a, const Segment &b, Segment &result)
{
  result.SetLeft(min(a.left(), b.left()));
  result.SetRight(max(a.right(), b.right()));
}

bool Union(const Segment &a, const Segment &b, Segment &result, Segment &gap)
{
  result.SetLeft(min(a.left(), b.left()));
  result.SetRight(max(a.right(), b.right()));

  gap.SetLeft(min(a.right(), b.right()));
  gap.SetRight(max(a.left(), b.left()));
  if ((a.right() > b.left()) & (a.left() < b.right()))
    return false;
  else
    return true;
}

bool Contains(const Segment &a, const Segment &b)
{
  return (b.left() >= a.left()) && (b.right() <= a.right());
}

bool StrictContains(const Segment &a, const Segment &b)
{
  return (b.left() > a.left()) && (b.right() < a.right());
}

int Subtract(const Segment &a, const Segment &b,
              Segment &result, Segment &result2)
{
  Segment intersect;
  if (!Intersect(a, b, intersect))
  {
    result = a;
    return 1;
  }

  if (a == intersect)
    return 0;

  if (a.left() == intersect.left())
  {
    result.SetLeft(intersect.right());
    result.SetRight(a.right());
    return 1;
  }

  if (a.right() == intersect.right())
  {
    result.SetLeft(a.left());
    result.SetRight(intersect.left());
    return 1;
  }

  result.SetLeft(a.left());
  result.SetRight(intersect.left());
  result2.SetLeft(intersect.right());
  result2.SetRight(a.right());
  return 2;
}

int Separation(const Segment &a, const Segment &b)
{
  if (a.left() >= b.right())
    return a.left() - b.right();
  else if (a.right() <= b.left())
    return b.left() - a.right();
  else
    return 0;
}

struct GSPOPoint
{
  vector<int> lefts, rights;
};

struct GSPOReorder
{
  Segment seg;
  int index;
};

struct GSPOReorderLess
{
  bool operator()(const GSPOReorder &a, const GSPOReorder &b) const
  {
    return a.seg.left() < b.seg.left();
  }
};

void GetSegmentPairOverlaps(const vector<Segment> &segments1,
                            const vector<Segment> &segments2,
                            vector< pair<int, int> > &overlaps)
{
  overlaps.clear();

  typedef map<int, GSPOPoint> GSPOExtrema;
  GSPOExtrema extrema;

  for (int i = 0; i < segments2.size(); ++i)
  {
    extrema[segments2[i].left()].lefts.push_back(i);
    extrema[segments2[i].right()].rights.push_back(i);
  }

  vector<GSPOReorder> sorted1(segments1.size());
  for (int i = 0; i < segments1.size(); ++i)
  {
    sorted1[i].seg = segments1[i];
    sorted1[i].index = i;
  }
  sort(sorted1.begin(), sorted1.end(), GSPOReorderLess());

  set<int> current;
  GSPOExtrema::iterator current_left = extrema.begin();
  GSPOExtrema::iterator current_right = extrema.begin();

  for (int i = 0; i < sorted1.size(); ++i)
  {
    while (current_right != extrema.begin())
    {
      GSPOExtrema::iterator new_right = current_right;
      --new_right;
      if (new_right->first >= sorted1[i].seg.right())
      {
        current_right = new_right;
        for (int j = 0; j < current_right->second.lefts.size(); ++j)
          current.erase(current_right->second.lefts[j]);
      }
      else
        break;
    }

    while ((current_right != extrema.end()) && 
           (current_right->first < sorted1[i].seg.right()))
    {
      current.insert(current_right->second.lefts.begin(),
                     current_right->second.lefts.end());
      ++current_right;
    }

    while ((current_left != extrema.end()) &&
           (current_left->first <= sorted1[i].seg.left()))
    {
      for (int j = 0; j < current_left->second.rights.size(); ++j)
        current.erase(current_left->second.rights[j]);
      ++current_left;
    }

    for (set<int>::iterator iter = current.begin();
         iter != current.end(); ++iter)
      overlaps.push_back(pair<int, int>(sorted1[i].index, *iter));
  }
}
