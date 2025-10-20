#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>
#include <memory>
#include <cctype>

// ============================================================================
// DATA STRUCTURES
// ============================================================================

// Represents a term in a predicate (variable or constant)
struct Term {
    std::string value;
    bool is_variable;
    
    Term(const std::string& v = "", bool var = true) 
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
    std::string relation;
    std::vector<Term> terms;
    
    Atom(const std::string& rel = "") : relation(rel) {}
    
    void addTerm(const Term& t) {
        terms.push_back(t);
    }
    
    std::string toString() const {
        std::string result = relation + "(";
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
    std::string name;
    std::vector<Term> head;
    std::vector<Atom> body;
    
    ConjunctiveQuery(const std::string& n = "") : name(n) {}
    
    std::set<std::string> getVariables() const {
        std::set<std::string> vars;
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
    
    std::set<std::string> getHeadVariables() const {
        std::set<std::string> vars;
        for (const auto& t : head) {
            if (t.is_variable) vars.insert(t.value);
        }
        return vars;
    }
    
    std::string toString() const {
        std::string result = name + "(";
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
using Mapping = std::map<std::string, std::string>;

// MCD: MiniCon Description
struct MCD {
    int view_index;
    std::set<int> covered_subgoals;  // Indices of query subgoals covered
    Mapping variable_mapping;         // View variables -> Query variables
    std::set<std::string> distinguished_vars; // Head variables of query covered
    
    std::string toString() const {
        std::stringstream ss;
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
    std::vector<int> view_indices;
    std::vector<Mapping> mappings;
    std::set<int> covered_subgoals;
    
    std::string toString(const std::vector<ConjunctiveQuery>& views) const {
        std::stringstream ss;
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
                const std::string& var = view.head[j].value;
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
    
    std::string toSQL(const std::vector<ConjunctiveQuery>& views, 
                      const ConjunctiveQuery& original_query) const {
        std::stringstream ss;
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
    static std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\n\r");
        size_t end = s.find_last_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        return s.substr(start, end - start + 1);
    }
    
    static std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), 
                      [](unsigned char c){ return std::tolower(c); });
        return s;
    }
    
    static std::vector<std::string> split(const std::string& s, char delim) {
        std::vector<std::string> result;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            std::string trimmed = trim(item);
            if (!trimmed.empty()) {
                result.push_back(trimmed);
            }
        }
        return result;
    }
    
    static std::vector<std::string> split(const std::string& s, const std::string& delim) {
        std::vector<std::string> result;
        size_t start = 0;
        size_t end = s.find(delim);
        
        while (end != std::string::npos) {
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
        std::vector<std::string> select_attrs;
        std::vector<std::string> tables;
        std::vector<std::pair<std::string, std::string>> joins;
        std::map<std::string, std::string> table_aliases;
    };
    
    SQLParsed parseSQL(const std::string& sql) {
        SQLParsed parsed;
        std::string sql_lower = Utils::toLower(sql);
        
        // Find SELECT, FROM, WHERE positions
        size_t select_pos = sql_lower.find("select");
        size_t from_pos = sql_lower.find("from");
        size_t where_pos = sql_lower.find("where");
        
        if (select_pos == std::string::npos || from_pos == std::string::npos) {
            std::cerr << "Invalid SQL: missing SELECT or FROM\n";
            return parsed;
        }
        
        // Parse SELECT clause
        std::string select_clause = sql.substr(select_pos + 6, from_pos - select_pos - 6);
        parsed.select_attrs = Utils::split(select_clause, ',');
        
        // Parse FROM clause
        std::string from_clause;
        if (where_pos != std::string::npos) {
            from_clause = sql.substr(from_pos + 4, where_pos - from_pos - 4);
        } else {
            from_clause = sql.substr(from_pos + 4);
        }
        
        auto table_parts = Utils::split(from_clause, ',');
        for (const auto& part : table_parts) {
            auto tokens = Utils::split(part, ' ');
            if (tokens.size() >= 1) {
                parsed.tables.push_back(tokens[0]);
                if (tokens.size() >= 2) {
                    // Handle alias (e.g., "Customer c" or "Customer AS c")
                    std::string alias = tokens.back();
                    parsed.table_aliases[alias] = tokens[0];
                }
            }
        }
        
        // Parse WHERE clause
        if (where_pos != std::string::npos) {
            std::string where_clause = sql.substr(where_pos + 5);
            auto predicates = Utils::split(where_clause, "and");
            
            for (const auto& pred : predicates) {
                size_t eq_pos = pred.find('=');
                if (eq_pos != std::string::npos) {
                    std::string left = Utils::trim(pred.substr(0, eq_pos));
                    std::string right = Utils::trim(pred.substr(eq_pos + 1));
                    parsed.joins.push_back({left, right});
                }
            }
        }
        
        return parsed;
    }
    
    // Extract table and attribute from qualified name (e.g., "customer.name" -> {"customer", "name"})
    std::pair<std::string, std::string> splitQualifiedName(const std::string& name) {
        size_t dot_pos = name.find('.');
        if (dot_pos != std::string::npos) {
            return {name.substr(0, dot_pos), name.substr(dot_pos + 1)};
        }
        return {"", name};
    }
    
    // Generate variable name for an attribute
    std::string generateVarName(const std::string& attr, 
                               std::map<std::string, std::string>& attr_to_var,
                               int& var_counter) {
        if (attr_to_var.find(attr) != attr_to_var.end()) {
            return attr_to_var[attr];
        }
        
        std::string var = "v" + std::to_string(var_counter++);
        attr_to_var[attr] = var;
        return var;
    }
    
public:
    ConjunctiveQuery convert(const std::string& sql, const std::string& query_name = "Q") {
        ConjunctiveQuery cq(query_name);
        SQLParsed parsed = parseSQL(sql);
        
        if (parsed.tables.empty()) {
            return cq;
        }
        
        // Map attributes to variables
        std::map<std::string, std::string> attr_to_var;
        int var_counter = 1;
        
        // Build a map of which attributes belong to which tables
        std::map<std::string, std::vector<std::string>> table_attrs;
        
        // Process SELECT attributes for head
        for (const auto& attr : parsed.select_attrs) {
            auto [table, attr_name] = splitQualifiedName(attr);
            std::string var = generateVarName(attr, attr_to_var, var_counter);
            cq.head.push_back(Term(var, true));
            
            if (!table.empty()) {
                // Resolve alias if exists
                if (parsed.table_aliases.find(table) != parsed.table_aliases.end()) {
                    table = parsed.table_aliases[table];
                }
                table_attrs[table].push_back(attr_name);
            }
        }
        
        // Process joins to determine which attributes belong to which tables
        for (const auto& [left, right] : parsed.joins) {
            auto [left_table, left_attr] = splitQualifiedName(left);
            auto [right_table, right_attr] = splitQualifiedName(right);
            
            // Resolve aliases
            if (parsed.table_aliases.find(left_table) != parsed.table_aliases.end()) {
                left_table = parsed.table_aliases[left_table];
            }
            if (parsed.table_aliases.find(right_table) != parsed.table_aliases.end()) {
                right_table = parsed.table_aliases[right_table];
            }
            
            std::string left_var = generateVarName(left, attr_to_var, var_counter);
            std::string right_var = generateVarName(right, attr_to_var, var_counter);
            
            // Make sure they map to the same variable (join condition)
            if (left_var != right_var) {
                // Update all occurrences of right_var to left_var
                for (auto& [key, val] : attr_to_var) {
                    if (val == right_var) {
                        val = left_var;
                    }
                }
                // Update head if necessary
                for (auto& term : cq.head) {
                    if (term.value == right_var) {
                        term.value = left_var;
                    }
                }
            }
            
            if (!left_table.empty()) {
                table_attrs[left_table].push_back(left_attr);
            }
            if (!right_table.empty()) {
                table_attrs[right_table].push_back(right_attr);
            }
        }
        
        // Create atoms for each table
        for (const auto& table : parsed.tables) {
            std::string resolved_table = table;
            Atom atom(resolved_table);
            
            // Get all attributes for this table
            std::set<std::string> table_vars;
            for (const auto& [attr, var] : attr_to_var) {
                auto [t, a] = splitQualifiedName(attr);
                if (parsed.table_aliases.find(t) != parsed.table_aliases.end()) {
                    t = parsed.table_aliases[t];
                }
                if (t == resolved_table || (t.empty() && table_attrs[resolved_table].empty())) {
                    table_vars.insert(var);
                }
            }
            
            // If no specific attributes found, use all attributes that might belong here
            if (table_vars.empty()) {
                for (const auto& [attr, var] : attr_to_var) {
                    auto [t, a] = splitQualifiedName(attr);
                    if (t.empty()) {
                        table_vars.insert(var);
                    }
                }
            }
            
            // Add terms to atom
            for (const auto& var : table_vars) {
                atom.addTerm(Term(var, true));
            }
            
            if (!atom.terms.empty()) {
                cq.body.push_back(atom);
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
    std::vector<ConjunctiveQuery> views;
    std::vector<MCD> mcds;
    
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
    
    // Check if a combination of MCDs covers all subgoals and head variables
    bool isValidRewriting(const std::vector<MCD>& mcd_combo) {
        std::set<int> all_covered;
        std::set<std::string> all_distinguished;
        
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
        
        // Check if all head variables are covered
        auto query_head_vars = query.getHeadVariables();
        if (all_distinguished != query_head_vars) {
            return false;
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
    void generateRewritings(std::vector<QueryRewriting>& rewritings) {
        int n_subgoals = query.body.size();
        
        // Generate all subsets of MCDs
        int n_mcds = mcds.size();
        for (int mask = 1; mask < (1 << n_mcds); ++mask) {
            std::vector<MCD> combo;
            
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
    
    std::vector<QueryRewriting> rewrite() {
        mcds.clear();
        
        std::cout << "\n=== Step 1: Finding MCDs for each view ===\n";
        // Find all MCDs
        for (size_t i = 0; i < views.size(); ++i) {
            std::cout << "\nProcessing View " << i << ": " 
                     << views[i].toString() << "\n";
            findMCDsForView(i);
        }
        
        std::cout << "\nFound " << mcds.size() << " MCDs:\n";
        for (size_t i = 0; i < mcds.size(); ++i) {
            std::cout << "  MCD " << i << ": View V" << mcds[i].view_index 
                     << " covers subgoals {";
            bool first = true;
            for (int sg : mcds[i].covered_subgoals) {
                if (!first) std::cout << ", ";
                std::cout << sg;
                first = false;
            }
            std::cout << "}\n    Mapping: {";
            first = true;
            for (const auto& [v, q] : mcds[i].variable_mapping) {
                if (!first) std::cout << ", ";
                std::cout << v << "->" << q;
                first = false;
            }
            std::cout << "}\n    Distinguished vars: {";
            first = true;
            for (const auto& dv : mcds[i].distinguished_vars) {
                if (!first) std::cout << ", ";
                std::cout << dv;
                first = false;
            }
            std::cout << "}\n";
        }
        
        std::cout << "\n=== Step 2: Combining MCDs to form rewritings ===\n";
        // Generate rewritings
        std::vector<QueryRewriting> rewritings;
        generateRewritings(rewritings);
        
        return rewritings;
    }
    
    void printQuery() const {
        std::cout << "Query: " << query.toString() << "\n";
    }
    
    void printViews() const {
        std::cout << "Views:\n";
        for (size_t i = 0; i < views.size(); ++i) {
            std::cout << "  V" << i << ": " << views[i].toString() << "\n";
        }
    }
};

// ============================================================================
// MAIN - EXAMPLES
// ============================================================================

int main() {
    SQLToConjunctiveQuery converter;
    
    std::cout << "=================================================\n";
    std::cout << "  MiniCon Algorithm for Query Rewriting (SQL)\n";
    std::cout << "=================================================\n";
    
    // Example 1: Classic join query
    std::cout << "\n\n### Example 1: Classic Join Query ###\n";
    std::cout << "-------------------------------------\n";
    
    std::string sql_q1 = "SELECT R.x, S.z FROM R, S WHERE R.y = S.y";
    std::string sql_v1 = "SELECT R.x, R.y FROM R";
    std::string sql_v2 = "SELECT S.y, S.z FROM S";
    
    std::cout << "Query SQL: " << sql_q1 << "\n";
    std::cout << "View V1 SQL: " << sql_v1 << "\n";
    std::cout << "View V2 SQL: " << sql_v2 << "\n\n";
    
    MiniCon minicon1;
    ConjunctiveQuery q1 = converter.convert(sql_q1, "Q");
    ConjunctiveQuery v1 = converter.convert(sql_v1, "V1");
    ConjunctiveQuery v2 = converter.convert(sql_v2, "V2");
    
    minicon1.setQuery(q1);
    minicon1.addView(v1);
    minicon1.addView(v2);
    
    std::cout << "Converted to Conjunctive Queries:\n";
    minicon1.printQuery();
    minicon1.printViews();
    
    auto rewritings1 = minicon1.rewrite();
    
    std::cout << "\n=== Rewritings Found: " << rewritings1.size() << " ===\n";
    for (size_t i = 0; i < rewritings1.size(); ++i) {
        std::cout << "\nRewriting " << i + 1 << ":\n";
        std::cout << "  Conjunctive form: " << rewritings1[i].toString(minicon1.views) << "\n";
        std::cout << "  SQL form: " << rewritings1[i].toSQL(minicon1.views, q1) << "\n";
    }
    
    // Example 2: Pre-joined view
    std::cout << "\n\n### Example 2: Pre-Joined View ###\n";
    std::cout << "-----------------------------------\n";
    
    std::string sql_q2 = "SELECT R.x, S.z FROM R, S WHERE R.y = S.y";
    std::string sql_v3 = "SELECT R.x, S.z FROM R, S WHERE R.y = S.y";
    
    std::cout << "Query SQL: " << sql_q2 << "\n";
    std::cout << "View V3 SQL: " << sql_v3 << "\n\n";
    
    MiniCon minicon2;
    ConjunctiveQuery q2 = converter.convert(sql_q2, "Q");
    ConjunctiveQuery v3 = converter.convert(sql_v3, "V3");
    
    minicon2.setQuery(q2);
    minicon2.addView(v3);
    
    std::cout << "Converted to Conjunctive Queries:\n";
    minicon2.printQuery();
    minicon2.printViews();
    
    auto rewritings2 = minicon2.rewrite();
    
    std::cout << "\n=== Rewritings Found: " << rewritings2.size() << " ===\n";
    for (size_t i = 0; i < rewritings2.size(); ++i) {
        std::cout << "\nRewriting " << i + 1 << ":\n";
        std::cout << "  Conjunctive form: " << rewritings2[i].toString(minicon2.views) << "\n";
        std::cout << "  SQL form: " << rewritings2[i].toSQL(minicon2.views, q2) << "\n";
    }
    
    // Example 3: Complex query (similar to paper example)
    std::cout << "\n\n### Example 3: Complex Query (Paper Example) ###\n";
    std::cout << "-----------------------------------------------\n";
    
    std::string sql_q3 = "SELECT c.name, n.name, s.name "
                        "FROM Customer c, Nation n, Supplier s "
                        "WHERE c.nationkey = n.nationkey AND n.nationkey = s.nationkey";
    std::string sql_v4 = "SELECT c.name, c.nationkey FROM Customer c";
    std::string sql_v5 = "SELECT n.nationkey, n.name FROM Nation n";
    std::string sql_v6 = "SELECT s.name, s.nationkey FROM Supplier s";
    
    std::cout << "Query SQL:\n  " << sql_q3 << "\n";
    std::cout << "View V4 SQL: " << sql_v4 << "\n";
    std::cout << "View V5 SQL: " << sql_v5 << "\n";
    std::cout << "View V6 SQL: " << sql_v6 << "\n\n";
    
    MiniCon minicon3;
    ConjunctiveQuery q3 = converter.convert(sql_q3, "Q");
    ConjunctiveQuery v4 = converter.convert(sql_v4, "V4");
    ConjunctiveQuery v5 = converter.convert(sql_v5, "V5");
    ConjunctiveQuery v6 = converter.convert(sql_v6, "V6");
    
    minicon3.setQuery(q3);
    minicon3.addView(v4);
    minicon3.addView(v5);
    minicon3.addView(v6);
    
    std::cout << "Converted to Conjunctive Queries:\n";
    minicon3.printQuery();
    minicon3.printViews();
    
    auto rewritings3 = minicon3.rewrite();
    
    std::cout << "\n=== Rewritings Found: " << rewritings3.size() << " ===\n";
    for (size_t i = 0; i < rewritings3.size(); ++i) {
        std::cout << "\nRewriting " << i + 1 << ":\n";
        std::cout << "  Conjunctive form: " << rewritings3[i].toString(minicon3.views) << "\n";
        std::cout << "  SQL form: " << rewritings3[i].toSQL(minicon3.views, q3) << "\n";
    }
    
    // Example 4: No valid rewriting
    std::cout << "\n\n### Example 4: No Valid Rewriting ###\n";
    std::cout << "-------------------------------------\n";
    
    std::string sql_q4 = "SELECT R.x, R.y FROM R, S WHERE R.y = S.y";
    std::string sql_v7 = "SELECT R.x FROM R";
    
    std::cout << "Query SQL: " << sql_q4 << "\n";
    std::cout << "View V7 SQL: " << sql_v7 << "\n\n";
    
    MiniCon minicon4;
    ConjunctiveQuery q4 = converter.convert(sql_q4, "Q");
    ConjunctiveQuery v7 = converter.convert(sql_v7, "V7");
    
    minicon4.setQuery(q4);
    minicon4.addView(v7);
    
    std::cout << "Converted to Conjunctive Queries:\n";
    minicon4.printQuery();
    minicon4.printViews();
    
    auto rewritings4 = minicon4.rewrite();
    
    std::cout << "\n=== Rewritings Found: " << rewritings4.size() << " ===\n";
    if (rewritings4.empty()) {
        std::cout << "No valid rewriting exists!\n";
        std::cout << "Reason: Views cannot cover all query subgoals and head variables.\n";
    }
    
    // Example 5: TPC-H style query
    std::cout << "\n\n### Example 5: TPC-H Style Query ###\n";
    std::cout << "------------------------------------\n";
    
    std::string sql_q5 = "SELECT o.orderkey, c.name, l.quantity "
                        "FROM Orders o, Customer c, LineItem l "
                        "WHERE o.custkey = c.custkey AND o.orderkey = l.orderkey";
    std::string sql_v8 = "SELECT o.orderkey, o.custkey FROM Orders o";
    std::string sql_v9 = "SELECT c.custkey, c.name FROM Customer c";
    std::string sql_v10 = "SELECT l.orderkey, l.quantity FROM LineItem l";
    
    std::cout << "Query SQL:\n  " << sql_q5 << "\n";
    std::cout << "View V8 SQL: " << sql_v8 << "\n";
    std::cout << "View V9 SQL: " << sql_v9 << "\n";
    std::cout << "View V10 SQL: " << sql_v10 << "\n\n";
    
    MiniCon minicon5;
    ConjunctiveQuery q5 = converter.convert(sql_q5, "Q");
    ConjunctiveQuery v8 = converter.convert(sql_v8, "V8");
    ConjunctiveQuery v9 = converter.convert(sql_v9, "V9");
    ConjunctiveQuery v10 = converter.convert(sql_v10, "V10");
    
    minicon5.setQuery(q5);
    minicon5.addView(v8);
    minicon5.addView(v9);
    minicon5.addView(v10);
    
    std::cout << "Converted to Conjunctive Queries:\n";
    minicon5.printQuery();
    minicon5.printViews();
    
    auto rewritings5 = minicon5.rewrite();
    
    std::cout << "\n=== Rewritings Found: " << rewritings5.size() << " ===\n";
    for (size_t i = 0; i < rewritings5.size(); ++i) {
        std::cout << "\nRewriting " << i + 1 << ":\n";
        std::cout << "  Conjunctive form: " << rewritings5[i].toString(minicon5.views) << "\n";
        std::cout << "  SQL form: " << rewritings5[i].toSQL(minicon5.views, q5) << "\n";
    }
    
    std::cout << "\n=================================================\n";
    std::cout << "         MiniCon Algorithm Completed\n";
    std::cout << "=================================================\n";
    
    return 0;
}
