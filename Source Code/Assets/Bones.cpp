#include "Bones.h"

Bone *boneAddChild(Bone *root, I16 id, const mat4& _absMatrix)
{
 	Bone *t;	I16 i;
 
 	if (!root) /* If there is no root, create one */
 	{
 		if (!(root = (Bone *)malloc(sizeof(Bone))))
 			return NULL;
 		root->_parent = NULL;
 	}
 	else if (root->_childCount < MAX_BONE_CHILD_COUNT) /* If there is space for another child */
 	{
 		/* Allocate the child */
 		if (!(t = (Bone *)malloc(sizeof(Bone))))
 			return NULL; /* Error! */
 
 		t->_parent = root; /* Set it's parent */
 		root->_child[root->_childCount++] = t; /* Increment the childCounter and set the pointer */
 		root = t; /* Change the root */
 	}
 	else /* Can't add a child */
 		return NULL;
 
 	root->_childCount = 0;
	root->_id = id;
 
 	for (i = 0; i < MAX_BONE_CHILD_COUNT; i++)
 		root->_child[i] = NULL;
 
 	return root;
}

Bone *boneFreeTree(Bone *root)
{
	I16 i;

	if (!root)
		return NULL;

	for (i = 0; i < root->_childCount; i++)
		boneFreeTree(root->_child[i]);

	free(root);

	return NULL;
}

Bone *boneFindById(Bone *root, I16 id)
{
	I16 i;
	Bone *p;

	/* No bone */
	if (!root)
		return NULL;

	if (root->_id == id)
		return root;

	for (i = 0; i < root->_childCount; i++)
	{
		/* Search recursively */
		p = boneFindById(root->_child[i], id);

		/* Found a bone in this subtree! */
		if (p)
			return p;
	}

	/* No such bone */
	return NULL;
}