/*
 * Classification.h
 *
 *  Created on: Feb 10, 2016
 *      Author: Matt
 */

#ifndef SRC_CLASSIFICATION_H_
#define SRC_CLASSIFICATION_H_


#include <string>
#include <vector>
#include <algorithm>

struct R16_read
{
	R16_read()
		: value(0.f)
	{}
	double value;
	//values which identify the phylogeny classification of this read, in order. [kingdom, phylum, class, order, family, genus, species]
	std::vector<std::string> classification;
};

class TreeNode
{
public:
	TreeNode(std::string label, int depth);
	TreeNode(std::string label, int depth, double value);
	~TreeNode();
	TreeNode(const TreeNode& src);
	TreeNode(TreeNode&& src);

	std::string GetLabel()  const { return _label; }
	void SetLabel(std::string label) { _label = label; }
	double GetValue()  const { return _value; }
	void SetValue(double value) { _value = value; }
	int GetDepth() const { return _depth; }

	const std::vector<TreeNode>& GetChildren() { return _children; }

	void Insert(R16_read c);

	//traverses the tree depth first and executes UnaryPreorderFunction on each node as it is visited.  UnaryPostOrderFunction is
	//executed once all of the node's children have been recursively visited before returning.
	//Both UnaryPreorderFunction and UnaryPostOrderFunction are functions which matches the signature [void f(TreeNode&)]
	template <class UnaryPreorderFunction, class UnaryPostOrderFunction>
	void DepthFirst(UnaryPreorderFunction preOrder, UnaryPostOrderFunction postOrder)
	{
		preOrder(*this);
		std::for_each(std::begin(_children), std::end(_children), [&preOrder, &postOrder](TreeNode& t) { t.DepthFirst(preOrder, postOrder); });
		postOrder(*this);
		return;
	}

	//traverses the tree, depth first, doing 2 things:
	//1.  (preorder) clears the value of any node with 1 or more children.
	//2.  (postorder) accumulates all values from leaf nodes up to the top.  This sets the value percentage correctly for all nodes with children.  Each
	//node's value will be a sum of it's children's values.
	void UpdateValues();

private:
	std::string _label;
	double _value;
	int _depth;
	std::vector<TreeNode> _children;
};


#endif /* SRC_CLASSIFICATION_H_ */
