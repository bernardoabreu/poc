#include "process.h"
#include "util.h"

using namespace std;


void insert_map(int point, Graph *g, vector<vector<mol_info> > *points, 
    HashMolMap *mol_map, vector<int> *mol_set, string key, 
    int current_mol, int level, int min_group_size, node **last){

    node *np;
    node n;
    pattern pat;

    pair<HashMolMap::iterator, bool> search_result;
    int gap = (*points)[point][level].second - (*points)[point][level+1].second;
    // cout << "Insert_map:" << key << ' ' << level << ' ' << min_group_size << endl;
    search_result = mol_map->insert(HashMolMap::value_type(key, NULL));

    // cout << key << ':' << search_result.second<<endl;
    if(search_result.second){
        // cout << "Key " << key << " does not exist" << endl;
        pat.mol_set = key;
        pat.quality = 0;
        pat.best_quality = 0;
        pat.gap = 0;
        pat.molecules = vector<int>(mol_set->begin(), mol_set->begin()+level+min_group_size);
        n.pat = pat;

        g->level[level-min_group_size + 1].push_back(n);

        np = &(g->level[level - min_group_size + 1].back());
        // np->molecules = vector<int>(mol_set->begin(), mol_set->begin()+level+min_group_size);
        // np->pat.molecules = vector<int>(mol_set->begin(), mol_set->begin()+level+min_group_size);

        for(int k = 0; k < 4; k++){
            np->children.push_back(0);
        }

        (search_result.first)->second = np;
    }
    else{
        // cout << "Key " << key << " exists" << endl;
        np = (search_result.first)->second;
    }

    np->pat.quality += (gap*gap);

    np->pat.gap += gap;

    np->pat.points.push_back(point);


    if((*last) != NULL ){
        // cout << last->mol_set << '>' << np->mol_set << ' ' << (current_mol>>5) << ' ' << (current_mol&31) << endl;
        if(!(((*last)->children[current_mol>>5]>>(current_mol%32))&1)){
            (*last)->children[current_mol>>5] |= 1<<(current_mol & 31);
            // cout << "Current:" << last->mol_set << '>' << np->mol_set << endl;
            (*last)->next.push_back(np);
        }
    }
    (*last) = np;

}

void apply_sqrt(Graph *g){
    vector<list<node> >::iterator it;
    list<node>::iterator jt;

    for (it=g->level.begin(); it != g->level.end(); ++it){
        for (jt=(*it).begin(); jt != (*it).end(); ++jt){
            (*jt).pat.quality = sqrt((*jt).pat.quality);
            (*jt).pat.best_quality = (*jt).pat.quality;
        }
    }
}

void add_vertices_edges_hashed(Graph *g, vector<vector<mol_info> > *points, int min_group_size){
    int level_size = g->level.size();

    if(min_group_size < 1) min_group_size = 1;

    vector<int> mol_set(level_size+min_group_size);
    // cout << mol_set.size() <<endl;

    string key;
    int current_mol, points_size;

    node *last;
    node n;

    HashMolMap mol_map;

    points_size = points->size();
    pair<HashMolMap::iterator, bool> search_result;
    for (int i = 0; i < points_size; i++){
        last = NULL;

        for(int k = 0; k < min_group_size; k++){
            current_mol = (*points)[i][k].first;
            insert_sorted(mol_set, current_mol, k+1);
        }
        key = join(mol_set, ',', min_group_size);
        insert_map(i, g, points, &mol_map, &mol_set, key, current_mol, min_group_size-1, min_group_size, &last);

        for (int level = min_group_size; level < level_size ; level++){
            current_mol = (*points)[i][level].first;
            insert_sorted(mol_set, current_mol, level+1);
            key = join(mol_set, ',', level+1);
            insert_map(i, g, points, &mol_map, &mol_set, key, current_mol, level, min_group_size, &last);
        }
    }

    apply_sqrt(g);
}


void build_graph(Graph *g, vector<vector<mol_info> > *points, int min_group_size){
    // MolMap mol_map;

    // cout << (*points).size() << ' ' << (*points)[0].size() << ' ' << min_group_size << endl;
    int level_size = (*points)[0].size() - 2*(min_group_size - 1) - 1;
    // cout << level_size << endl;
    // cout << (*points)[0].size() << endl;
    // int level_size = min_group_size;
    // cout << level_size << endl;
    g->level.resize(level_size);

    add_vertices_edges_hashed(g, points, min_group_size);
    // add_vertices(g, points, &mol_map, level_size);
    // add_edges(g, &mol_map, level_size);
}





void level1(Graph *g, vector<pattern> *sel){
    list<node>::iterator it;
    list<node*>::iterator jt;

    bool possible;
    int level_size = g->level.size();

    node *n;

    // First step - going down
    for(int i = 0; i < level_size; i++){
        for (it=g->level[i].begin(); it != g->level[i].end(); ++it){
            n = &(*it);

            for(jt=n->next.begin(); jt != n->next.end(); ++jt){
                (**jt).pat.best_quality = max(n->pat.best_quality, (**jt).pat.best_quality);
            }
        }
    }

    // Second step - going up
    for (int i = level_size-1; i >= 0; i--){
        for (it=g->level[i].begin(); it != g->level[i].end(); ++it){
            n = &(*it);
            possible = (n->pat.quality >= n->pat.best_quality);
            n->pat.best_quality = n->pat.quality;

            for(jt=n->next.begin(); jt != n->next.end(); ++jt){
                n->pat.best_quality = max(n->pat.best_quality, (**jt).pat.best_quality);
            }

            if(possible && (n->pat.quality == n->pat.best_quality)){
                sel->push_back(n->pat);
                n->pat.best_quality++;
            }
        }
    }

}


bool myfunction (pattern i,pattern j) { return (i.quality > j.quality); }


bool is_subset(vector<int> *small, vector<int> *big){
    if(small->size() > big->size()) return false;

    vector<int>::iterator small_it;
    vector<int>::iterator big_it;


    for (big_it=big->begin(), small_it=small->begin();
         big_it != big->end() && small_it != small->end(); ++big_it){
        if((*big_it) == (*small_it)) small_it++;
    }

    return small_it == small->end();
}

void post_process(vector<pattern> *selected, list<pattern> *out){
    sort(selected->begin(), selected->end(), myfunction);
    
    int size = selected->size();
    int* valid = new int[size];

    for(int i = 0; i < size; i++){
        valid[i] = 1;
    }

    // cout << endl;
    // for(vector<node>::iterator it=selected->begin(); it != selected->end(); ++it){
    //     cout << (*it).mol_set << " - ";
    // }
    // cout << endl;

    for(int i = 0; i < size; i++){
        for(int j = i+1; j < size; j++){
            // cout << (*selected)[i].mol_set;

            // if(is_subset(&((*selected)[i].molecules), &((*selected)[j].molecules))){
            //     cout << " is subset of " << (*selected)[j].mol_set << '.';
            //     valid[j] = 0;
            // }
            // else if(is_subset(&((*selected)[j].molecules), &((*selected)[i].molecules))){
            //     cout << " is superset of " << (*selected)[j].mol_set << '.';
            //     valid[j] = 0;
            // }
            // 
            if((is_subset(&((*selected)[i].molecules), &((*selected)[j].molecules))) ||
               (is_subset(&((*selected)[j].molecules), &((*selected)[i].molecules)))){
                valid[j] = 0;
            }

            // cout << endl;
        }
    }


    for(int i = 0; i < size; i++){
        if(valid[i]){
            out->push_back((*selected)[i]);
        }
    }

}
