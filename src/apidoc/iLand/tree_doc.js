/**
A `Tree` object allows accessing individual trees (>4m height) in iLand via Javascript.

Note that using Javascript with individual trees can be slow.

Accessing a tree
----------------
Trees can be accessed via the {{#crossLink "Management"}}{{/crossLink}} object or {{#crossLink "TreeList"}}{{/crossLink}} list within ABE.
The {{#crossLink "Management"}}{{/crossLink}} object provides {{#crossLink "Management/tree:method"}}{{/crossLink}} and
 {{#crossLink "Management/treeObject:method"}}{{/crossLink}} functions. Similarly, the TreeList includes
{{#crossLink "TreeList/tree:method"}}{{/crossLink}} and {{#crossLink "TreeList/treeObject:method"}}{{/crossLink}}.



Reference vs object
-------------------
iLand provides two distinct ways to access trees, namely as reference (to an internal object) or as a Javascript object.
Access via reference is faster, but tree references can not be used for later access. See the example:

    // load all trees of resource unit 0:
    management.loadResourceUnit(0);
    // access via reference: every call to the tree() functions modifies the reference:
    var t1=management.tree(0); // the first tree (dbh=10)
    var t2=management.tree(1); // the second tree (dbh=20)
    console.log("Tree1: " + t1.dbh + ", Tree2: " + t2.dbh); // -> yields "Tree1: 20, Tree2: 20"!!!!

    // access via object:
    var t1=management.treeObject(0); // the first tree (dbh=10)
    var t2=management.treeObject(1); // the second tree (dbh=20)
    console.log("Tree1: " + t1.dbh + ", Tree2: " + t2.dbh); // -> yields "Tree1: 10, Tree2: 20"

    // References are, however, useful for iterating over a list of trees:
    var sum_dbh=0;
    for (var i=0;i<management.count;++i)
        sum_dbh += management.tree(i).dbh;
    console.log("Mean dbh: " + sum_dbh / management.count);



@module iLand
@class Tree
*/

Tree = {

/**
The species (4-character species ID), e.g. 'piab'.

@property species
@type string
*/
/**
The dbh (diamter at breast height), in cm.

@property dbh
@type double
*/
/**
The dbh (diamter at breast height), in cm.

@property dbh
@type double
*/
/**
The x-coordinate of the tree, in m.

@property x
@type double
*/
/**
The y-coordinate of the tree, in m.

@property y
@type double
*/
/**
A string with memory address, Id, dbh, height, and coordinates of the tree.

@method info
@return {string} A human-readable string with key characteristics of the tree.
*/
}
