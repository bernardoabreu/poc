#include "node.h"


Node::Node(const Pattern &pat, int size){
    this->children = std::vector<bool>(size);
    this->pat = pat;
}


// inline void Node::add_child(int mol, Node* np){
//     if(!this->children[mol]){
//         this->children[mol] = true;
//         this->next.push_back(np);
//     }
// }