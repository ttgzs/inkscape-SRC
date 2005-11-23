/**
 * \brief Remove overlaps function
 *
 * Authors:
 *   Tim Dwyer <tgdwyer@gmail.com>
 *
 * Copyright (C) 2005 Authors
 *
 * Released under GNU GPL.  Read the file 'COPYING' for more information.
 */

#pragma once

class Block;
class Variable;
class Constraint;
class Blocks
{
	void dfsVisit(Variable *v);
public:
	static Blocks *instance;
	Block *head;
	Blocks(Variable* vs[], const int n);
	~Blocks(void);
	void add(Block *b);
	void remove(Block *b);
	void merge_left(Block *r);
	void merge_right(Block *l);
	void split(Block *b, Block *&l, Block *&r, Constraint *c);
	double cost();
};
