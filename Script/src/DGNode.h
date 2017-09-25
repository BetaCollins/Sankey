/*
 * DGNode.h
 *
 *  Created on: Feb 3, 2016
 *      Author: Matt
 */

#ifndef DGNODE_H
#define DGNODE_H

#include <string>
#include <iostream>
#include "Utility.h"

struct DGNode
{
  int parent;
  int first_child, num_children;

  std::string label;
  double count;
};

template <>
void Write<DGNode>(std::ostream &out, const DGNode &x)
{
  Write(out, x.label);
  WriteBin(out, x.count);
  WriteBin(out, x.parent);
  WriteBin(out, x.first_child);
  WriteBin(out, x.num_children);
}

template <>
void Read<DGNode>(std::istream &in, DGNode &x)
{
	Read(in, x.label);
	ReadBin(in, x.count);
	ReadBin(in, x.parent);
	ReadBin(in, x.first_child);
	ReadBin(in, x.num_children);
}


#endif /* SRC_DGNODE_H_ */
