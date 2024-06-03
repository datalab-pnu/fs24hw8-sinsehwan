//btnode.tc

#include "btree.h"
//#include <iostream>

const int MaxHeight = 5;
template <class keyType>
BTree<keyType>::BTree(int order, int keySize, int unique)
: Buffer (1+2*order,sizeof(int)+order*keySize+order*sizeof(int)),
	BTreeFile (Buffer), Root (order)
{
	Height = 1;
	Order = order;
	PoolSize = MaxHeight*2;
	Nodes = new BTNode * [PoolSize];
	BTNode::InitBuffer(Buffer,order);
	Nodes[0] = &Root;
}

template <class keyType>
BTree<keyType>::~BTree()
{
	Close();
	delete[] Nodes;
}

template <class keyType>
int BTree<keyType>::Open (char * name, ios_base::openmode mode)
//int BTree<keyType>::Open (char * name, int mode)
{
	int result;
	result = BTreeFile.Open(name, mode);
	if (!result) return result;
	// load root
	BTreeFile.Read(Root);
	Height = 1; // find height from BTreeFile!
	return 1;
}

template <class keyType>
int BTree<keyType>::Create (char * name, ios_base::openmode mode)
//int BTree<keyType>::Create (char * name, int mode)
{
	int result;
	result = BTreeFile.Create(name, mode);
	if (!result) return result;
	// append root node
	result = BTreeFile.Write(Root);
	Root.RecAddr=result;
	return result != -1;	
}

template <class keyType>
int BTree<keyType>::Close ()
{
	int result;
	result = BTreeFile.Rewind();
	if (!result) return result;
	result = BTreeFile.Write(Root);
	if (result==-1) return 0;
	return BTreeFile.Close();
}


template <class keyType>
int BTree<keyType>::Insert (const keyType key, const int recAddr)
{
	//InorderTraversal();
	int result; int level = Height-1; 
	int newLargest=0; keyType prevKey, largestKey; 
	BTNode * thisNode, * newNode, * parentNode;
	thisNode = FindLeaf (key);

	// test for special case of new largest key in tree
	if (key > thisNode->LargestKey())
		{newLargest = 1; prevKey=thisNode->LargestKey();}
	
	result = thisNode -> Insert (key, recAddr);

	// handle special case of new largest key in tree
	if (newLargest)
		for (int i = 0; i<Height-1; i++) 
		{
			Nodes[i]->UpdateKey(prevKey,key);
			if (i>0) Store (Nodes[i]);
		}

	while (result==-1) // if overflow and not root
	{
		//remember the largest key
		largestKey=thisNode->LargestKey();
		// split the node
		newNode = NewNode();
		thisNode->Split(newNode);
		Store(thisNode); Store(newNode);
		level--; // go up to parent level
		if (level < 0) break;
		// insert newNode into parent of thisNode
		parentNode = Nodes[level];
		cout<<"Before----"<<endl;
		parentNode->Print(cout);
		result = parentNode->UpdateKey(largestKey,thisNode->LargestKey());
		result = parentNode->Insert (newNode->LargestKey(),newNode->RecAddr);
		cout<<"After----"<<endl;
		parentNode->Print(cout);
		thisNode=parentNode;
	}
	Store(thisNode);
	if (level >= 0) return 1;// insert complete
	// else we just split the root
	int newAddr = BTreeFile.Append(Root); // put previous root into file
	// insert 2 keys in new root node
	Root.Keys[0]=thisNode->LargestKey();
	Root.RecAddrs[0]=newAddr;
	Root.Keys[1]=newNode->LargestKey();
	Root.RecAddrs[1]=newNode->RecAddr;
	Root.NumKeys=2; 
	Height++;
	//InorderTraversal(); // segmentation fault발생
	return 1;	
}

template <class keyType>
void BTree<keyType>::InorderTraversal()
{
	InorderTraversal(&Root);
	cout << endl;
}

template <class keyType>
void BTree<keyType>::InorderTraversal(BTreeNode<keyType>* node)
{
	if(node == nullptr)
	{
		return;
	}
	cout << "IsLeaf : " << node->IsLeaf() << endl;
	if(!node->IsLeaf())
	{
		for(int i = 0; i < node->NumKeys; i++)
		{	
			cout << i << " " << node->NumKeys << endl;
			InorderTraversal(Fetch(node->RecAddrs[i]));
			if(i < node->NumKeys)
			{
				cout << node->Keys[i] << " ";
			}
		}
		InorderTraversal(Fetch(node->RecAddrs[node->NumKeys - 1]));
	}

	else
	{
		
		for(int i = 0; i < node->NumKeys; i++)
		{
			cout << i << endl;
			cout << node->Keys[i] << " ";
		}
	}
}

template <class keyType>
int BTree<keyType>::Remove (const keyType key, const int recAddr)
{
	std::cout << "Attempting to remove key: " << key << std::endl;//
	// find the leaf node containing the key
	BTNode* leafNode = FindLeaf(key);

	// remove the key from the leaf node
	int result = leafNode->Remove(key, recAddr);
	if(result == 0)
	{
		// 키가 없음
		return 0;
	}

	std::cout << "Removed key from leaf node. Checking for underflow..." << std::endl;//

	// handle underflow
	int level = Height - 1;
	while(result == -1 && level > 0)
	{
		std::cout << "Underflow at level " << level << ". Attempting to resolve..." << std::endl;//
		// fetch parent node
		BTNode* parentNode = Nodes[level - 1];

		// find the index of the leaf node in the parent node
		int nodeIndex = parentNode->Search(leafNode->LargestKey(), -1, 0);
		if(nodeIndex < 0) nodeIndex = 0;

		// try to borrow a key from a sibling node or merge
		BTNode* leftSibling = nullptr;
		BTNode* rightSibling = nullptr;

		if(nodeIndex > 0)
		{
			leftSibling = Fetch(parentNode->RecAddrs[nodeIndex - 1]);
		}
		if(nodeIndex < parentNode->numKeys() - 1)
		{
			rightSibling = Fetch(parentNode->RecAddrs[nodeIndex + 1]);
		}

		if(leftSibling && leftSibling->numKeys() > leftSibling->MinKeys)
		{
			std::cout << "Borrowing key from left sibling..." << std::endl;//
			// borrow from leftSibiling
			keyType borrowedKey = leftSibling->Keys[leftSibling->numKeys() - 1];
			int borrowedAddr = leftSibling->RecAddrs[leftSibling->numKeys() - 1];

			leftSibling->Remove(borrowedKey, borrowedAddr);
			leafNode->Insert(borrowedKey, borrowedAddr);
			parentNode->UpdateKey(leftSibling->LargestKey(), leafNode->LargestKey());
			Store(leftSibling);
			Store(leafNode);
			result = 1;
		}
		else if(rightSibling && rightSibling->numKeys() > rightSibling->MinKeys)
		{
			std::cout << "Borrowing key from right sibling..." << std::endl;//

			keyType borrowedKey = rightSibling->Keys[0];
			int borrowedAddr = rightSibling->RecAddrs[0];
			rightSibling->Remove(borrowedKey, borrowedAddr);
			leafNode->Insert(borrowedKey, borrowedAddr);
			//parentNode->UpdateKey(borrowedKey, rightSibling->LargestKey());
			parentNode->UpdateKey(leafNode->LargestKey(), rightSibling->Keys[0]);
			Store(rightSibling);
			Store(leafNode);
			result = 1;
		}
		else if(leftSibling)
		{
			std::cout << "Merging with left sibling..." << std::endl;//
			// merge with leftSibling
			leftSibling->Merge(leafNode);
			parentNode->Remove(leafNode->LargestKey());

			Store(leftSibling);
			result = (parentNode->numKeys() < parentNode->MinKeys) ? -1 : 1;
		}
		else if(rightSibling)
		{
			std::cout << "Merging with right sibling..." << std::endl;//

			// merge with rightSibling
			leafNode->Merge(rightSibling);
			parentNode->Remove(rightSibling->LargestKey());
			Store(leafNode);
			result = (parentNode->numKeys() < parentNode->MinKeys) ? -1 : 1;
		}

		Store(parentNode);
		leafNode = parentNode;
		--level;
	}

	// handle the root node case
	if(Height > 1 && Root.numKeys() == 1)
	{
		std::cout << "Reducing height of the tree." << std::endl;//

		// 루트에 키가 하나만 있는 경우 -> reduce height
		BTNode* newRoot = Fetch(Root.RecAddrs[0]);
		Root = *newRoot;
		Height--;
	}

	Store(&Root);
	return 1;
}

template <class keyType>
int BTree<keyType>::Search (const keyType key, const int recAddr)
{
	BTNode * leafNode;
	leafNode = FindLeaf (key); 
	return leafNode -> Search (key, recAddr);
}

template <class keyType>
void BTree<keyType>::Print (ostream & stream) 
{
	stream << "BTree of height "<<Height<<" is "<<endl;
	Root.Print(stream);
	if (Height>1)
		for (int i = 0; i<Root.numKeys(); i++)
		{
			Print(stream, Root.RecAddrs[i], 2);
		}
	stream <<"end of BTree"<<endl;
}

template <class keyType>
void BTree<keyType>::Print 
	(ostream & stream, int nodeAddr, int level) 
{
	BTNode * thisNode = Fetch(nodeAddr);
	stream<<"Node at level "<<level<<" address "<<nodeAddr<<' ';
	thisNode -> Print(stream);
	if (Height>level)
	{
		level++;
		for (int i = 0; i<thisNode->numKeys(); i++)
		{
			Print(stream, thisNode->RecAddrs[i], level);
		}
		stream <<"end of level "<<level<<endl;
	}
}

template <class keyType>
BTreeNode<keyType> * BTree<keyType>::FindLeaf (const keyType key)
// load a branch into memory down to the leaf with key
{
	std::cout << "Finding leaf for key: " << key << std::endl;//
	int recAddr, level;

	// Load root node into the Nodes array
    Nodes[0] = &Root;

	for (level = 1; level < Height; level++)
	{
		recAddr = Nodes[level-1]->Search(key,-1,0);//inexact search
		Nodes[level]=Fetch(recAddr);
		cout << Nodes[level] << endl;
	}
	return Nodes[level-1];
}


template <class keyType>
BTreeNode<keyType> * BTree<keyType>::NewNode ()
{// create a fresh node, insert into tree and set RecAddr member
	BTNode * newNode = new BTNode(Order);
	int recAddr = BTreeFile . Append(*newNode);
	newNode -> RecAddr = recAddr;
	return newNode;
}

template <class keyType>
BTreeNode<keyType> * BTree<keyType>::Fetch(const int recaddr)
{// load this node from File into a new BTreeNode
	int result;
	BTNode * newNode = new BTNode(Order);
	result = BTreeFile.Read (*newNode, recaddr);
	if (result == -1) return NULL;
	newNode -> RecAddr = result;
	return newNode;
}

template <class keyType>
int BTree<keyType>::Store (BTreeNode<keyType> * thisNode)
{
	return BTreeFile.Write(*thisNode, thisNode->RecAddr);
}


// for template separate compile
template class BTree<char>;