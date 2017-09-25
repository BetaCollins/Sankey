#include <algorithm>
#include "Classification.h"
#include <iostream>
#include <numeric>

TreeNode::TreeNode(std::string label, int depth) : _label(label), _value(0.f), _depth(depth) {}
TreeNode::TreeNode(std::string label, int depth, double value) : _label(label), _value(value), _depth(depth) {}
TreeNode::~TreeNode() {}
TreeNode::TreeNode(const TreeNode& src)
	: _label(src._label)
	, _value(src._value)
	, _depth(src._depth)
	, _children(src._children)
{}
TreeNode::TreeNode(TreeNode&& src)
	: _label(src._label)
	, _value(src._value)
	, _depth(src._depth)
	, _children(std::move(src._children))
{}

void TreeNode::Insert(R16_read c)
{
	//if the classification is empty, do nothing
	if (c.classification.empty()) return;
	//if the front of the classification is the same as my label, remove it from the list
	if (c.classification.front() == _label)
	{
		c.classification.erase(c.classification.begin());
	}

	//again check if the classification is emptysince we just modified it.
	if (c.classification.empty()) return;
	//if the front of the classification is an empty string, add a new leaf child and done, but only if the current node is not an empty string label.
	if (c.classification.front().empty())
	{
		_children.push_back(TreeNode(c.classification.front(), _depth + 1, c.value));
		return;
	}

	//if i have children, look for a child with a matching label and insert the classification there.
	if (!_children.empty())
	{
		std::string s = c.classification.front();
		auto it = std::find_if(std::begin(_children), std::end(_children), [&s](TreeNode other)->bool { return other.GetLabel() == s; });
		if (it != std::end(_children))
		{
			it->Insert(c);
			return;
		}
	}
	//lastly, if i have no children matching the classification, add new leaf child and insert this classification there.
	if (c.classification.empty()) return;
	std::string newLabel = c.classification.front();
	_children.push_back(TreeNode(newLabel, _depth + 1, c.value));
	_children.back().Insert(c);
	return;
}
//traverses the tree, depth first, doing 2 things:
//1.  (preorder) clears the value of any node with 1 or more children.
//2.  (postorder) accumulates all values from leaf nodes up to the top.  This sets the value percentage correctly for all nodes with children.  Each
//node's value will be a sum of it's children's values.
void TreeNode::UpdateValues()
{
	DepthFirst(
			[](TreeNode& t)
			{
				if(!t.GetChildren().empty())
				{
					t.SetValue(0.f);
				}
			},
			[](TreeNode& t)
			{
				if(!t.GetChildren().empty())
				{
					double value = std::accumulate(std::begin(t.GetChildren()), std::end(t.GetChildren()), 0.0, [](double sum, const TreeNode& x)->double { return sum + x.GetValue(); });
					t.SetValue(value);
				}
			}
	);
}
