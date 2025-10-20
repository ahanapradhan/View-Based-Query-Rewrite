#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
using namespace std;
// ============================================================================
// DATA STRUCTURES
// ============================================================================

// Represents a term in a predicate (variable or constant)
struct Term {
    string value;
    bool is_variable;
    
    Term(const string& v = "", bool var = true) 
        : value(v), is_variable(var) {}
    
    bool operator==(const Term& other) const {
        return value == other.value && is_variable == other.is_variable;
    }
    
    bool operator<(const Term& other) const {
        if (is_variable != other.is_variable) return is_variable;
        return value < other.value;
    }
};

// Represents a relational atom: R(t1, t2, ..., tn)
struct Atom {
    string relation;
    vector<Term> terms;
    
    Atom(const string& rel = "") : relation(rel) {}
    
    void addTerm(const Term& t) {
        terms.push_back(t);
    }
    
    string toString() const {
        string result = relation + "(";
        for (size_t i = 0; i < terms.size(); ++i) {
            if (i > 0) result += ", ";
            result += terms[i].value;
        }
        result += ")";
        return result;
    }
};

// Represents a conjunctive query: Q(head) :- body
struct ConjunctiveQuery {
    string name;
    vector<Term> head;
    vector<Atom> body;
    
    ConjunctiveQuery(const string& n = "") : name(n) {}
    
    set<string> getVariables() const {
        set<string> vars;
        for (const auto& t : head) {
            if (t.is_variable) vars.insert(t.value);
        }
        for (const auto& atom : body) {
            for (const auto& t : atom.terms) {
                if (t.is_variable) vars.insert(t.value);
            }
        }
        return vars;
    }
    
    set<string> getHeadVariables() const {
        set<string> vars;
        for (const auto& t : head) {
            if (t.is_variable) vars.insert(t.value);
        }
        return vars;
    }
    
    string toString() const {
        string result = name + "(";
        for (size_t i = 0; i < head.size(); ++i) {
            if (i > 0) result += ", ";
            result += head[i].value;
        }
        result += ") :- ";
        for (size_t i = 0; i < body.size(); ++i) {
            if (i > 0) result += ", ";
            result += body[i].toString();
        }
        return result;
    }
};

// Variable mapping for homomorphism
using Mapping = map<string, string>;

// MCD: MiniCon Description
struct MCD {
    int view_index;
    set<int> covered_subgoals;  // Indices of query subgoals covered
    Mapping variable_mapping;         // View variables -> Query variables
    set<string> distinguished_vars; // Head variables of query covered
    
    string toString() const {
        stringstream ss;
        ss << "View V" << view_index << " covers subgoals {";
        bool first = true;
        for (int sg : covered_subgoals) {
            if (!first) ss << ", ";
            ss << sg;
            first = false;
        }
        ss << "} with mapping: {";
        first = true;
        for (const auto& [v_var, q_var] : variable_mapping) {
            if (!first) ss << ", ";
            ss << v_var << "->" << q_var;
            first = false;
        }
        ss << "}";
        return ss.str();
    }
};

// Query Rewriting: a combination of views
struct QueryRewriting {
    vector<int> view_indices;
    vector<Mapping> mappings;
    set<int> covered_subgoals;
    
    string toString(const vector<ConjunctiveQuery>& views) const {
        stringstream ss;
        ss << "Q_rewritten(";
        
        // Build head based on original query structure
        ss << "...) :- ";
        
        for (size_t i = 0; i < view_indices.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << views[view_indices[i]].name << "(";
            
            // Apply mapping to view head
            const auto& view = views[view_indices[i]];
            for (size_t j = 0; j < view.head.size(); ++j) {
                if (j > 0) ss << ", ";
                const string& var = view.head[j].value;
                if (mappings[i].find(var) != mappings[i].end()) {
                    ss << mappings[i].at(var);
                } else {
                    ss << var;
                }
            }
            ss << ")";
        }
        
        return ss.str();
    }
    
    string toSQL(const vector<ConjunctiveQuery>& views, 
                      const ConjunctiveQuery& original_query) const {
        stringstream ss;
        ss << "SELECT ";
        
        // Build SELECT clause from original query head
        for (size_t i = 0; i < original_query.head.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << original_query.head[i].value;
        }
        
        ss << " FROM ";
        
        // Build FROM clause using view names
        for (size_t i = 0; i < view_indices.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << views[view_indices[i]].name;
        }
        
        // Build WHERE clause from mappings (joins between views)
        bool first_where = true;
        for (size_t i = 0; i < view_indices.size(); ++i) {
            for (size_t j = i + 1; j < view_indices.size(); ++j) {
                // Find common variables between views
                for (const auto& [vi_var, q_var1] : mappings[i]) {
                    for (const auto& [vj_var, q_var2] : mappings[j]) {
                        if (q_var1 == q_var2) {
                            if (first_where) {
                                ss << " WHERE ";
                                first_where = false;
                            } else {
                                ss << " AND ";
                            }
                            ss << views[view_indices[i]].name << "." << vi_var
                               << " = " << views[view_indices[j]].name << "." << vj_var;
                        }
                    }
                }
            }
        }
        
        return ss.str();
    }
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

class Utils {
public:
    static string trim(const string& s) {
        size_t start = s.find_first_not_of(" \t\n\r");
        size_t end = s.find_last_not_of(" \t\n\r");
        if (start == string::npos) return "";
        return s.substr(start, end - start + 1);
    }
    
    static string toLower(string s) {
        transform(s.begin(), s.end(), s.begin(), 
                      [](unsigned char c){ return tolower(c); });
        return s;
    }
    
    static vector<string> split(const string& s, char delim) {
        vector<string> result;
        stringstream ss(s);
        string item;
        while (getline(ss, item, delim)) {
            string trimmed = trim(item);
            if (!trimmed.empty()) {
                result.push_back(trimmed);
            }
        }
        return result;
    }
    
    static vector<string> split(const string& s, const string& delim) {
        vector<string> result;
        size_t start = 0;
        size_t end = s.find(delim);
        
        while (end != string::npos) {
            result.push_back(trim(s.substr(start, end - start)));
            start = end + delim.length();
            end = s.find(delim, start);
        }
        result.push_back(trim(s.substr(start)));
        return result;
    }
};

// ============================================================================
// SQL TO CONJUNCTIVE QUERY CONVERTER
// ============================================================================

class SQLToConjunctiveQuery {
private:
    struct SQLParsed {
        vector<string> select_attrs;
        vector<string> tables;
        vector<pair<string, string>> joins; // pairs of qualified names (left, right)
        map<string, string> table_aliases;  // alias -> base table
    };

    // Toggle debug output
    static constexpr bool DEBUG = true;

    // Helper: lower-case & trim already exist in Utils; reuse them as needed.

    // Parse a very small subset of SQL (SELECT ... FROM ... WHERE ... AND ...)
    SQLParsed parseSQL(const string& sql) {
        SQLParsed parsed;
        string sql_lower = Utils::toLower(sql);

        // Find SELECT, FROM, WHERE positions
        size_t select_pos = sql_lower.find("select");
        size_t from_pos   = sql_lower.find("from");
        size_t where_pos  = sql_lower.find("where");

        if (select_pos == string::npos || from_pos == string::npos) {
            cerr << "Invalid SQL: missing SELECT or FROM\n";
            return parsed;
        }

        // Parse SELECT clause
        string select_clause = sql.substr(select_pos + 6, from_pos - select_pos - 6);
        parsed.select_attrs = Utils::split(select_clause, ',');

        // Parse FROM clause
        string from_clause;
        if (where_pos != string::npos) {
            from_clause = sql.substr(from_pos + 4, where_pos - from_pos - 4);
        } else {
            from_clause = sql.substr(from_pos + 4);
        }

        auto table_parts = Utils::split(from_clause, ',');
        for (const auto& part : table_parts) {
            auto tokens = Utils::split(part, ' ');
            if (tokens.empty()) continue;
            // tokens[0] is table name, optional last token is alias
            string base = tokens[0];
            string alias;
            if (tokens.size() >= 2) {
                // handle "AS"
                if (tokens.size() >= 3 && Utils::toLower(tokens[tokens.size()-2]) == "as") {
                    alias = tokens.back();
                } else {
                    alias = tokens.back();
                }
            }
            // store base and alias relationship
            parsed.tables.push_back(base);
            if (!alias.empty()) {
                parsed.table_aliases[alias] = base;
            }
        }

        // Parse WHERE clause: only equality predicates combined with AND
        if (where_pos != string::npos) {
            string where_clause = sql.substr(where_pos + 5);
            auto predicates = Utils::split(where_clause, "AND");
            for (const auto& pred : predicates) {
                size_t eq_pos = pred.find('=');
                if (eq_pos != string::npos) {
                    string left = Utils::trim(pred.substr(0, eq_pos));
                    string right = Utils::trim(pred.substr(eq_pos + 1));
                    parsed.joins.push_back({left, right});
                }
            }
        }

        // Normalize: replace any alias mention in parsed.tables with base table name
        for (auto &t : parsed.tables) {
            if (parsed.table_aliases.find(t) != parsed.table_aliases.end()) {
                t = parsed.table_aliases[t];
            }
        }

        if (DEBUG) {
            cerr << "DEBUG parseSQL:\n";
            cerr << " Tables: ";
            for (auto &t : parsed.tables) cerr << t << " ";
            cerr << "\n Aliases:\n";
            for (auto &kv : parsed.table_aliases) cerr << "  " << kv.first << " -> " << kv.second << "\n";
            cerr << " Select attrs:\n";
            for (auto &a : parsed.select_attrs) cerr << "  " << a << "\n";
            cerr << " Joins:\n";
            for (auto &j : parsed.joins) cerr << "  " << j.first << " = " << j.second << "\n";
        }

        return parsed;
    }

    // Extract table and attribute from qualified name (e.g., "customer.name" -> {"customer", "name"})
    pair<string, string> splitQualifiedName(const string& name) {
        size_t dot_pos = name.find('.');
        if (dot_pos != string::npos) {
            string left = Utils::trim(name.substr(0, dot_pos));
            string right = Utils::trim(name.substr(dot_pos + 1));
            return {left, right};
        }
        return {"", Utils::trim(name)};
    }

    // Generate variable name for a canonical attribute key "Table.attr"
    // Generate variable name for a canonical attribute key "Table.attr"
    string generateVarName(const string& canonical_attr, map<string, string>& attr_to_var, int& var_counter) {
        auto it = attr_to_var.find(canonical_attr);
        if (it != attr_to_var.end()) return it->second;
        // Make a deterministic variable name from canonical_attr, e.g., "Customer.name" -> "Customer_name"
        string var = canonical_attr;
        for (char &c : var) {
            if (c == '.') c = '_';
            // optionally sanitize other characters if needed
        }
        // ensure uniqueness (rare): if var already used as a value, append a counter
        bool used = false;
        for (const auto &kv : attr_to_var) {
            if (kv.second == var) {
                used = true;
                break;
            }
        }
        if (used) {
            var += "_" + to_string(var_counter++);
        }
        attr_to_var[canonical_attr] = var;
        return var;
    }
public:
    ConjunctiveQuery convert(const string& sql, const string& query_name = "Q") {
        ConjunctiveQuery cq(query_name);
        SQLParsed parsed = parseSQL(sql);

        // If parsing failed or no tables, return an empty CQ
        if (parsed.tables.empty()) {
            return cq;
        }

        // --- PATCH A: normalize aliases to base table names (ensure parsed.tables contain base names)
        // Already applied in parseSQL but double-check joins/aliases usage below.

        // Map attributes to canonical variables: canonical key = BaseTable.attr
        map<string, string> attr_to_var;
        int var_counter = 1;

        // Step 1: Build mapping for SELECT attributes (head) using canonical keys
        for (const auto& sel_attr : parsed.select_attrs) {
            auto [tbl, attr_name] = splitQualifiedName(sel_attr);
            // resolve alias if present
            if (!tbl.empty() && parsed.table_aliases.find(tbl) != parsed.table_aliases.end()) {
                tbl = parsed.table_aliases[tbl];
            }
            string canonical_key;
            if (!tbl.empty()) canonical_key = tbl + "." + attr_name;
            else canonical_key = attr_name; // fallback

            string var = generateVarName(canonical_key, attr_to_var, var_counter);
            cq.head.push_back(Term(var, true));
        }

        // Step 2: Process joins -> ensure both sides map to same canonical variable
        for (const auto& pr : parsed.joins) {
            const string& left = pr.first;
            const string& right = pr.second;

            auto [left_table, left_attr] = splitQualifiedName(left);
            auto [right_table, right_attr] = splitQualifiedName(right);

            // Resolve aliases
            if (!left_table.empty() && parsed.table_aliases.find(left_table) != parsed.table_aliases.end())
                left_table = parsed.table_aliases[left_table];
            if (!right_table.empty() && parsed.table_aliases.find(right_table) != parsed.table_aliases.end())
                right_table = parsed.table_aliases[right_table];

            // If either side lacks explicit table (unqualified), we don't change it
            string left_key  = (!left_table.empty())  ? left_table  + "." + left_attr  : left_attr;
            string right_key = (!right_table.empty()) ? right_table + "." + right_attr : right_attr;

            // Pick a canonical key deterministically (lexicographic or left_key)
            string canonical_key = left_key; // left as canonical
            // If neither key is present in attr_to_var yet, create canonical var.
            string join_var = generateVarName(canonical_key, attr_to_var, var_counter);

            // Force both sides to refer to the same variable
            attr_to_var[left_key] = join_var;
            attr_to_var[right_key] = join_var;
        }

        // Step 3: For any remaining select attributes or implied attributes that do not yet have a var, assign one
        // This helps when select attributes aren't part of any join
        for (const auto& sel_attr : parsed.select_attrs) {
            auto [tbl, attr_name] = splitQualifiedName(sel_attr);
            if (!tbl.empty() && parsed.table_aliases.find(tbl) != parsed.table_aliases.end())
                tbl = parsed.table_aliases[tbl];
            string canonical_key = (!tbl.empty()) ? tbl + "." + attr_name : attr_name;
            if (attr_to_var.find(canonical_key) == attr_to_var.end()) {
                generateVarName(canonical_key, attr_to_var, var_counter);
            }
        }

        // Step 4: Create atoms for each table deterministically using canonical attr_to_var keys
        for (const auto& table : parsed.tables) {
            string resolved_table = table;
            Atom atom(resolved_table);

            // Collect canonical attributes that belong to this table (prefix "Table.")
            vector<string> attrs_for_table;
            for (const auto& kv : attr_to_var) {
                const string& canonical_attr = kv.first; // e.g., Customer.nationkey
                size_t dot = canonical_attr.find('.');
                if (dot != string::npos) {
                    string tname = canonical_attr.substr(0, dot);
                    if (tname == resolved_table) {
                        attrs_for_table.push_back(canonical_attr);
                    }
                }
            }

            // Sort to keep deterministic order across query and views
            sort(attrs_for_table.begin(), attrs_for_table.end());

            // Add terms (variables) to atom in that deterministic order
            for (const auto& canon : attrs_for_table) {
                string var = attr_to_var[canon];
                atom.addTerm(Term(var, true));
            }

            // If the table had no canonical attributes but it was listed in FROM, we add a single placeholder
            // variable so the atom appears and MiniCon can try to map by relation name.
            if (atom.terms.empty()) {
                // create a placeholder var that is unique to this table in this query
                string placeholder_canon = resolved_table + "._placeholder";
                string pvar = generateVarName(placeholder_canon, attr_to_var, var_counter);
                atom.addTerm(Term(pvar, true));
            }

            cq.body.push_back(atom);
        }

        // DEBUG dumps to help diagnose mismatches if rewrites are still 0
        if (DEBUG) {
            cerr << "DEBUG: attr_to_var for SQL (" << query_name << "):\n";
            for (const auto &kv : attr_to_var) {
                cerr << "  " << kv.first << " -> " << kv.second << "\n";
            }
            cerr << "DEBUG: Constructed atoms for " << query_name << ":\n";
            for (const auto &atom : cq.body) {
                cerr << "  " << atom.toString() << "  terms:";
                for (const auto &t : atom.terms) cerr << " " << t.value;
                cerr << "\n";
            }
        }

        return cq;
    }
};

// ============================================================================
// MINICON ALGORITHM
// ============================================================================

class MiniCon {
public:
    ConjunctiveQuery query;
    vector<ConjunctiveQuery> views;
    vector<MCD> mcds;
    
    // Check if a mapping is consistent (no conflicts)
    bool isConsistentMapping(const Mapping& m1, const Mapping& m2) {
        for (const auto& [key, val] : m1) {
            if (m2.find(key) != m2.end() && m2.at(key) != val) {
                return false;
            }
        }
        return true;
    }
    
    // Merge two mappings
    Mapping mergeMappings(const Mapping& m1, const Mapping& m2) {
        Mapping merged = m1;
        for (const auto& [key, val] : m2) {
            merged[key] = val;
        }
        return merged;
    }
    
    // Check if view atom can map to query atom
    bool canMap(const Atom& view_atom, const Atom& query_atom, 
                Mapping& mapping) {
        if (view_atom.relation != query_atom.relation) return false;
        if (view_atom.terms.size() != query_atom.terms.size()) return false;
        
        Mapping temp_mapping;
        
        for (size_t i = 0; i < view_atom.terms.size(); ++i) {
            const Term& v_term = view_atom.terms[i];
            const Term& q_term = query_atom.terms[i];
            
            if (v_term.is_variable) {
                // View variable must map consistently
                if (temp_mapping.find(v_term.value) != temp_mapping.end()) {
                    if (temp_mapping[v_term.value] != q_term.value) {
                        return false;
                    }
                } else {
                    temp_mapping[v_term.value] = q_term.value;
                }
            } else {
                // View constant must match query term exactly
                if (q_term.is_variable || v_term.value != q_term.value) {
                    return false;
                }
            }
        }
        
        // Check consistency with existing mapping
        if (!isConsistentMapping(mapping, temp_mapping)) {
            return false;
        }
        
        mapping = mergeMappings(mapping, temp_mapping);
        return true;
    }
    
    // Find all possible MCDs for a view
    void findMCDsForView(int view_idx) {
        const ConjunctiveQuery& view = views[view_idx];
        int n_subgoals = query.body.size();
        
        // Try to cover each query subgoal
        for (int sg_idx = 0; sg_idx < n_subgoals; ++sg_idx) {
            const Atom& query_atom = query.body[sg_idx];
            
            // Try to match with each view subgoal
            for (size_t v_sg_idx = 0; v_sg_idx < view.body.size(); ++v_sg_idx) {
                const Atom& view_atom = view.body[v_sg_idx];
                
                Mapping mapping;
                if (canMap(view_atom, query_atom, mapping)) {
                    // Found a potential MCD, now extend it
                    MCD mcd;
                    mcd.view_index = view_idx;
                    mcd.covered_subgoals.insert(sg_idx);
                    mcd.variable_mapping = mapping;
                    
                    // Try to extend by covering more subgoals
                    extendMCD(view_idx, mcd);
                }
            }
        }
    }
    
    // Extend MCD by covering additional subgoals
    void extendMCD(int view_idx, MCD& mcd) {
        const ConjunctiveQuery& view = views[view_idx];
        bool extended = true;
        
        while (extended) {
            extended = false;
            
            // Try to cover additional query subgoals
            for (size_t sg_idx = 0; sg_idx < query.body.size(); ++sg_idx) {
                if (mcd.covered_subgoals.find(sg_idx) != mcd.covered_subgoals.end()) {
                    continue; // Already covered
                }
                
                const Atom& query_atom = query.body[sg_idx];
                
                // Try each view subgoal
                for (const auto& view_atom : view.body) {
                    Mapping temp_mapping = mcd.variable_mapping;
                    if (canMap(view_atom, query_atom, temp_mapping)) {
                        mcd.covered_subgoals.insert(sg_idx);
                        mcd.variable_mapping = temp_mapping;
                        extended = true;
                        break;
                    }
                }
            }
        }
        
        // Check which distinguished variables are covered
        for (const auto& head_term : query.head) {
            if (head_term.is_variable) {
                // Check if this variable is in the mapping range
                for (const auto& [v_var, q_var] : mcd.variable_mapping) {
                    if (q_var == head_term.value) {
                        // Check if v_var is in view's head
                        for (const auto& v_head_term : view.head) {
                            if (v_head_term.value == v_var) {
                                mcd.distinguished_vars.insert(head_term.value);
                                break;
                            }
                        }
                    }
                }
            }
        }
        
        // Only add MCD if it covers at least one subgoal
        if (!mcd.covered_subgoals.empty()) {
            mcds.push_back(mcd);
        }
    }
    
    /// Check if a combination of MCDs covers all subgoals and head variables
    bool isValidRewriting(const vector<MCD>& mcd_combo) {
        set<int> all_covered;
        set<string> all_distinguished;

        for (const auto& mcd : mcd_combo) {
            all_covered.insert(mcd.covered_subgoals.begin(),
                             mcd.covered_subgoals.end());
            all_distinguished.insert(mcd.distinguished_vars.begin(),
                                   mcd.distinguished_vars.end());
        }

        // Check if all subgoals are covered
        if (all_covered.size() != query.body.size()) {
            return false;
        }

        // Check if all head variables are covered (require subset)
        auto query_head_vars = query.getHeadVariables();
        for (const auto &hv : query_head_vars) {
            if (all_distinguished.find(hv) == all_distinguished.end()) {
                return false;
            }
        }

        // Check compatibility of mappings
        for (size_t i = 0; i < mcd_combo.size(); ++i) {
            for (size_t j = i + 1; j < mcd_combo.size(); ++j) {
                if (!isConsistentMapping(mcd_combo[i].variable_mapping,
                                        mcd_combo[j].variable_mapping)) {
                    return false;
                                        }
            }
        }
        return true;
    }

    
    // Generate all combinations of MCDs
    void generateRewritings(vector<QueryRewriting>& rewritings) {
        int n_subgoals = query.body.size();
        
        // Generate all subsets of MCDs
        int n_mcds = mcds.size();
        for (int mask = 1; mask < (1 << n_mcds); ++mask) {
            vector<MCD> combo;
            
            for (int i = 0; i < n_mcds; ++i) {
                if (mask & (1 << i)) {
                    combo.push_back(mcds[i]);
                }
            }
            
            if (isValidRewriting(combo)) {
                QueryRewriting rewriting;
                for (const auto& mcd : combo) {
                    rewriting.view_indices.push_back(mcd.view_index);
                    rewriting.mappings.push_back(mcd.variable_mapping);
                }
                rewriting.covered_subgoals.insert(
                    combo[0].covered_subgoals.begin(),
                    combo[0].covered_subgoals.end()
                );
                for (size_t i = 1; i < combo.size(); ++i) {
                    rewriting.covered_subgoals.insert(
                        combo[i].covered_subgoals.begin(),
                        combo[i].covered_subgoals.end()
                    );
                }
                rewritings.push_back(rewriting);
            }
        }
    }
    
public:
    ConjunctiveQuery query_cq; // Store original query for SQL output
    
    void setQuery(const ConjunctiveQuery& q) {
        query = q;
        query_cq = q;
    }
    
    void addView(const ConjunctiveQuery& v) {
        views.push_back(v);
    }
    
    vector<QueryRewriting> rewrite() {
        mcds.clear();
        
        cout << "\n=== Step 1: Finding MCDs for each view ===\n";
        // Find all MCDs
        for (size_t i = 0; i < views.size(); ++i) {
            cout << "\nProcessing View " << i << ": " 
                     << views[i].toString() << "\n";
            findMCDsForView(i);
        }
        
        cout << "\nFound " << mcds.size() << " MCDs:\n";
        for (size_t i = 0; i < mcds.size(); ++i) {
            cout << "  MCD " << i << ": View V" << mcds[i].view_index 
                     << " covers subgoals {";
            bool first = true;
            for (int sg : mcds[i].covered_subgoals) {
                if (!first) cout << ", ";
                cout << sg;
                first = false;
            }
            cout << "}\n    Mapping: {";
            first = true;
            for (const auto& [v, q] : mcds[i].variable_mapping) {
                if (!first) cout << ", ";
                cout << v << "->" << q;
                first = false;
            }
            cout << "}\n    Distinguished vars: {";
            first = true;
            for (const auto& dv : mcds[i].distinguished_vars) {
                if (!first) cout << ", ";
                cout << dv;
                first = false;
            }
            cout << "}\n";
        }
        
        cout << "\n=== Step 2: Combining MCDs to form rewritings ===\n";
        // Generate rewritings
        vector<QueryRewriting> rewritings;
        generateRewritings(rewritings);
        
        return rewritings;
    }
    
    void printQuery() const {
        cout << "Query: " << query.toString() << "\n";
    }
    
    void printViews() const {
        cout << "Views:\n";
        for (size_t i = 0; i < views.size(); ++i) {
            cout << "  V" << i << ": " << views[i].toString() << "\n";
        }
    }
};


void paperExample(SQLToConjunctiveQuery converter) {
    // Example 6: TPC-H style paper query
    cout << "\n\n### Example 5: TPC-H Style Query ###\n";
    cout << "------------------------------------\n";

    string sql_q = "SELECT c.name, s.name, n.name "
                        "FROM Supplier s, Customer c, Nation n "
                        "WHERE c.nationkey = s.nationkey AND s.nationkey = n.nationkey AND n.nationkey = c.nationkey";
    string sql_v2 = "SELECT c.nationkey, c.name, n.name FROM Customer c, Nation n "
                         "WHERE c.nationkey = n.nationkey";
    string sql_v1 = "SELECT c.nationkey, c.name FROM Customer c";
    string sql_v3 = "SELECT c.nationkey, c.name, s.name FROM Customer c, Supplier s "
                         "WHERE c.nationkey = s.nationkey";

    cout << "Query SQL:\n  " << sql_q << "\n";
    cout << "View V2 SQL: " << sql_v2 << "\n";
    cout << "View V1 SQL: " << sql_v1 << "\n";
    cout << "View V3 SQL: " << sql_v3 << "\n\n";

    MiniCon minicom;
    ConjunctiveQuery q = converter.convert(sql_q, "Q");
    ConjunctiveQuery v2 = converter.convert(sql_v2, "V2");
    ConjunctiveQuery v1 = converter.convert(sql_v1, "V1");
    ConjunctiveQuery v3 = converter.convert(sql_v3, "V3");

    minicom.setQuery(q);
    minicom.addView(v2);
    minicom.addView(v1);
    minicom.addView(v3);

    cout << "Converted to Conjunctive Queries:\n";
    minicom.printQuery();
    minicom.printViews();

    auto rewritings = minicom.rewrite();

    cout << "\n=== Rewritings Found: " << rewritings.size() << " ===\n";
    for (size_t i = 0; i < rewritings.size(); ++i) {
        cout << "\nRewriting " << i + 1 << ":\n";
        cout << "  Conjunctive form: " << rewritings[i].toString(minicom.views) << "\n";
        cout << "  SQL form: " << rewritings[i].toSQL(minicom.views, q) << "\n";
    }
}

// ============================================================================
// MAIN - EXAMPLES
// ============================================================================

int main() {
    SQLToConjunctiveQuery converter;
    
    cout << "=================================================\n";
    cout << "  MiniCon Algorithm for Query Rewriting (SQL)\n";
    cout << "=================================================\n";
    
   /* // Example 1: Classic join query
    cout << "\n\n### Example 1: Classic Join Query ###\n";
    cout << "-------------------------------------\n";
    
    string sql_q1 = "SELECT R.x, S.z FROM R, S WHERE R.y = S.y";
    string sql_v1 = "SELECT R.x, R.y FROM R";
    string sql_v2 = "SELECT S.y, S.z FROM S";
    
    cout << "Query SQL: " << sql_q1 << "\n";
    cout << "View V1 SQL: " << sql_v1 << "\n";
    cout << "View V2 SQL: " << sql_v2 << "\n\n";
    
    MiniCon minicon1;
    ConjunctiveQuery q1 = converter.convert(sql_q1, "Q");
    ConjunctiveQuery v1 = converter.convert(sql_v1, "V1");
    ConjunctiveQuery v2 = converter.convert(sql_v2, "V2");
    
    minicon1.setQuery(q1);
    minicon1.addView(v1);
    minicon1.addView(v2);
    
    cout << "Converted to Conjunctive Queries:\n";
    minicon1.printQuery();
    minicon1.printViews();
    
    auto rewritings1 = minicon1.rewrite();
    
    cout << "\n=== Rewritings Found: " << rewritings1.size() << " ===\n";
    for (size_t i = 0; i < rewritings1.size(); ++i) {
        cout << "\nRewriting " << i + 1 << ":\n";
        cout << "  Conjunctive form: " << rewritings1[i].toString(minicon1.views) << "\n";
        cout << "  SQL form: " << rewritings1[i].toSQL(minicon1.views, q1) << "\n";
    }
    
    // Example 2: Pre-joined view
    cout << "\n\n### Example 2: Pre-Joined View ###\n";
    cout << "-----------------------------------\n";
    
    string sql_q2 = "SELECT R.x, S.z FROM R, S WHERE R.y = S.y";
    string sql_v3 = "SELECT R.x, S.z FROM R, S WHERE R.y = S.y";
    
    cout << "Query SQL: " << sql_q2 << "\n";
    cout << "View V3 SQL: " << sql_v3 << "\n\n";
    
    MiniCon minicon2;
    ConjunctiveQuery q2 = converter.convert(sql_q2, "Q");
    ConjunctiveQuery v3 = converter.convert(sql_v3, "V3");
    
    minicon2.setQuery(q2);
    minicon2.addView(v3);
    
    cout << "Converted to Conjunctive Queries:\n";
    minicon2.printQuery();
    minicon2.printViews();
    
    auto rewritings2 = minicon2.rewrite();
    
    cout << "\n=== Rewritings Found: " << rewritings2.size() << " ===\n";
    for (size_t i = 0; i < rewritings2.size(); ++i) {
        cout << "\nRewriting " << i + 1 << ":\n";
        cout << "  Conjunctive form: " << rewritings2[i].toString(minicon2.views) << "\n";
        cout << "  SQL form: " << rewritings2[i].toSQL(minicon2.views, q2) << "\n";
    }
    
    // Example 3: Complex query (similar to paper example)
    cout << "\n\n### Example 3: Complex Query (Paper Example) ###\n";
    cout << "-----------------------------------------------\n";
    
    string sql_q3 = "SELECT c.name, n.name, s.name "
                        "FROM Customer c, Nation n, Supplier s "
                        "WHERE c.nationkey = n.nationkey AND n.nationkey = s.nationkey";
    string sql_v4 = "SELECT c.name, c.nationkey FROM Customer c";
    string sql_v5 = "SELECT n.nationkey, n.name FROM Nation n";
    string sql_v6 = "SELECT s.name, s.nationkey FROM Supplier s";
    
    cout << "Query SQL:\n  " << sql_q3 << "\n";
    cout << "View V4 SQL: " << sql_v4 << "\n";
    cout << "View V5 SQL: " << sql_v5 << "\n";
    cout << "View V6 SQL: " << sql_v6 << "\n\n";
    
    MiniCon minicon3;
    ConjunctiveQuery q3 = converter.convert(sql_q3, "Q");
    ConjunctiveQuery v4 = converter.convert(sql_v4, "V4");
    ConjunctiveQuery v5 = converter.convert(sql_v5, "V5");
    ConjunctiveQuery v6 = converter.convert(sql_v6, "V6");
    
    minicon3.setQuery(q3);
    minicon3.addView(v4);
    minicon3.addView(v5);
    minicon3.addView(v6);
    
    cout << "Converted to Conjunctive Queries:\n";
    minicon3.printQuery();
    minicon3.printViews();
    
    auto rewritings3 = minicon3.rewrite();
    
    cout << "\n=== Rewritings Found: " << rewritings3.size() << " ===\n";
    for (size_t i = 0; i < rewritings3.size(); ++i) {
        cout << "\nRewriting " << i + 1 << ":\n";
        cout << "  Conjunctive form: " << rewritings3[i].toString(minicon3.views) << "\n";
        cout << "  SQL form: " << rewritings3[i].toSQL(minicon3.views, q3) << "\n";
    }
    
    // Example 4: No valid rewriting
    cout << "\n\n### Example 4: No Valid Rewriting ###\n";
    cout << "-------------------------------------\n";
    
    string sql_q4 = "SELECT R.x, R.y FROM R, S WHERE R.y = S.y";
    string sql_v7 = "SELECT R.x FROM R";
    
    cout << "Query SQL: " << sql_q4 << "\n";
    cout << "View V7 SQL: " << sql_v7 << "\n\n";
    
    MiniCon minicon4;
    ConjunctiveQuery q4 = converter.convert(sql_q4, "Q");
    ConjunctiveQuery v7 = converter.convert(sql_v7, "V7");
    
    minicon4.setQuery(q4);
    minicon4.addView(v7);
    
    cout << "Converted to Conjunctive Queries:\n";
    minicon4.printQuery();
    minicon4.printViews();
    
    auto rewritings4 = minicon4.rewrite();
    
    cout << "\n=== Rewritings Found: " << rewritings4.size() << " ===\n";
    if (rewritings4.empty()) {
        cout << "No valid rewriting exists!\n";
        cout << "Reason: Views cannot cover all query subgoals and head variables.\n";
    }
    
    // Example 5: TPC-H style query
    cout << "\n\n### Example 5: TPC-H Style Query ###\n";
    cout << "------------------------------------\n";
    
    string sql_q5 = "SELECT o.orderkey, c.name, l.quantity "
                        "FROM Orders o, Customer c, LineItem l "
                        "WHERE o.custkey = c.custkey AND o.orderkey = l.orderkey";
    string sql_v8 = "SELECT o.orderkey, o.custkey FROM Orders o";
    string sql_v9 = "SELECT c.custkey, c.name FROM Customer c";
    string sql_v10 = "SELECT l.orderkey, l.quantity FROM LineItem l";
    
    cout << "Query SQL:\n  " << sql_q5 << "\n";
    cout << "View V8 SQL: " << sql_v8 << "\n";
    cout << "View V9 SQL: " << sql_v9 << "\n";
    cout << "View V10 SQL: " << sql_v10 << "\n\n";
    
    MiniCon minicon5;
    ConjunctiveQuery q5 = converter.convert(sql_q5, "Q");
    ConjunctiveQuery v8 = converter.convert(sql_v8, "V8");
    ConjunctiveQuery v9 = converter.convert(sql_v9, "V9");
    ConjunctiveQuery v10 = converter.convert(sql_v10, "V10");
    
    minicon5.setQuery(q5);
    minicon5.addView(v8);
    minicon5.addView(v9);
    minicon5.addView(v10);
    
    cout << "Converted to Conjunctive Queries:\n";
    minicon5.printQuery();
    minicon5.printViews();
    
    auto rewritings5 = minicon5.rewrite();
    
    cout << "\n=== Rewritings Found: " << rewritings5.size() << " ===\n";
    for (size_t i = 0; i < rewritings5.size(); ++i) {
        cout << "\nRewriting " << i + 1 << ":\n";
        cout << "  Conjunctive form: " << rewritings5[i].toString(minicon5.views) << "\n";
        cout << "  SQL form: " << rewritings5[i].toSQL(minicon5.views, q5) << "\n";
    }
    */

    paperExample(converter);
    
    cout << "\n=================================================\n";
    cout << "         MiniCon Algorithm Completed\n";
    cout << "=================================================\n";
    
    return 0;
}
